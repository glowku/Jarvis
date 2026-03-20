#pragma GCC target("sse,sse2")

#include <gui/jarvis_gui.h>
#include <kernel/console.h>
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <kernel/types.h>
#include <string.h>
#include <limine.h>

extern volatile struct limine_framebuffer_request framebuffer_request;
extern void serial_puts(const char* s);
extern uint64_t pmm_get_used_memory(void);
extern uint64_t pmm_get_total_memory(void);

#undef abs
static inline s32 abs(s32 x) { return (x < 0) ? -x : x; }

static window_manager_t* global_wm = NULL;
static window_t* main_window = NULL;
static window_t* task_manager_window = NULL;

static s32 mouse_x = 512;
static s32 mouse_y = 384;
static window_t* dragged_window = NULL;
static s32 drag_offset_x = 0;
static s32 drag_offset_y = 0;
static bool is_logged_in = false;

// ==========================================
// MOTEUR DE TEXTE JARVIS (Police 8x8 Minimaliste)
// ==========================================
// Lettres basiques A-Z pour notre OS
static const u8 font_8x8[26][8] = {
    {0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // A
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // B
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // C
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // D
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // E
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // F
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // G
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // H
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // I
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // J
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // K
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // L
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // M
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // N
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // O
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // P
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // Q
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // R
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // S
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // T
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // U
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // X
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // Y
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}  // Z
};

void draw_char(framebuffer_t* fb, char c, s32 x, s32 y, color_t color) {
    if (c < 'A' || c > 'Z') return; // On ne gère que A-Z pour le test
    int index = c - 'A';
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (font_8x8[index][row] & (1 << (7 - col))) {
                // On affiche les pixels en "gras" (2x2) pour que ça se lise bien en 1080p
                framebuffer_draw_pixel(fb, x + (col*2), y + (row*2), color);
                framebuffer_draw_pixel(fb, x + (col*2)+1, y + (row*2), color);
                framebuffer_draw_pixel(fb, x + (col*2), y + (row*2)+1, color);
                framebuffer_draw_pixel(fb, x + (col*2)+1, y + (row*2)+1, color);
            }
        }
    }
}

void draw_string(framebuffer_t* fb, const char* str, s32 x, s32 y, color_t color) {
    while (*str) {
        if (*str == ' ') { x += 16; } 
        else { draw_char(fb, *str, x, y, color); x += 16; }
        str++;
    }
}

// ==========================================
// RAPPEL DES FONCTIONS GRAPHIQUES
// ==========================================
framebuffer_t* framebuffer_create(s32 width, s32 height, bool double_buffered) {
    (void)width; (void)height; 
    struct limine_framebuffer *lfb = framebuffer_request.response->framebuffers[0];
    framebuffer_t* fb = kmalloc(sizeof(framebuffer_t));
    fb->width = lfb->width; fb->height = lfb->height; fb->pitch = lfb->pitch; fb->double_buffered = double_buffered;
    fb->pixels = (u32*)lfb->address;
    if (double_buffered) fb->back_buffer = kmalloc(lfb->height * lfb->pitch);
    return fb;
}

void framebuffer_present(framebuffer_t* fb) {
    if (fb && fb->back_buffer) memcpy(fb->pixels, fb->back_buffer, fb->height * fb->pitch);
}

void framebuffer_draw_pixel(framebuffer_t* fb, s32 x, s32 y, color_t color) {
    if (!fb || x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    u32* buffer = fb->double_buffered ? fb->back_buffer : fb->pixels;
    buffer[y * (fb->pitch/4) + x] = (color.r << 16) | (color.g << 8) | color.b;
}

void framebuffer_fill_rect(framebuffer_t* fb, rect_t rect, color_t color) {
    for(s32 y = rect.y; y < rect.y + rect.height; y++)
        for(s32 x = rect.x; x < rect.x + rect.width; x++)
            framebuffer_draw_pixel(fb, x, y, color);
}

void framebuffer_draw_line(framebuffer_t* fb, s32 x1, s32 y1, s32 x2, s32 y2, color_t color) {
    s32 dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    s32 dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    s32 err = dx + dy, e2;
    while (1) {
        framebuffer_draw_pixel(fb, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_mouse_cursor(framebuffer_t* fb, s32 x, s32 y) {
    color_t white = {255, 255, 255, 255};
    for(int i=0; i<15; i++) {
        if (x+i < fb->width && y+i < fb->height) framebuffer_draw_pixel(fb, x+i, y+i, white);
        if (y+i < fb->height && x < fb->width) framebuffer_draw_pixel(fb, x, y+i, white);
        if (x+i < fb->width && y < fb->height) framebuffer_draw_pixel(fb, x+i, y, white);
    }
}

window_t* window_create(const char* title, rect_t bounds) {
    window_t* win = kmalloc(sizeof(window_t));
    if (win) {
        memset(win, 0, sizeof(window_t));
        win->base.type = WIDGET_WINDOW;
        win->base.bounds = bounds;
        win->base.visible = true;
        // On copie le texte en forçant les majuscules pour notre police
        for(int i=0; title[i] && i<63; i++) {
            win->title[i] = (title[i] >= 'a' && title[i] <= 'z') ? title[i] - 32 : title[i];
        }
    }
    return win;
}

void wm_bring_to_front(window_manager_t* wm, s32 index) {
    if (index == wm->num_windows - 1) return;
    window_t* win = wm->windows[index];
    for (s32 i = index; i < wm->num_windows - 1; i++) wm->windows[i] = wm->windows[i+1];
    wm->windows[wm->num_windows - 1] = win;
}

// =====================================
// LE BUREAU JARVIS ET LE LOGIN
// =====================================
void wm_render(window_manager_t* wm) {
    if (!wm || !wm->framebuffer) return;
    
    // Fond d'écran
    for(int i=0; i<wm->framebuffer->width * wm->framebuffer->height; i++) {
        wm->framebuffer->back_buffer[i] = 0x0A0B10; // Noir profond
    }

    if (!is_logged_in) {
        // --- ECRAN DE LOGIN ---
        s32 cx = wm->framebuffer->width / 2;
        s32 cy = wm->framebuffer->height / 2;
        
        rect_t login_box = {cx - 150, cy - 100, 300, 200};
        framebuffer_fill_rect(wm->framebuffer, login_box, (color_t){15, 20, 35, 255});
        framebuffer_draw_line(wm->framebuffer, login_box.x, login_box.y, login_box.x+login_box.width, login_box.y, (color_t){0, 212, 255, 255});
        
        draw_string(wm->framebuffer, "JARVIS SYSTEM", cx - 100, cy - 70, (color_t){0, 212, 255, 255});
        draw_string(wm->framebuffer, "LOGIN REQUIRED", cx - 110, cy - 20, (color_t){255, 255, 255, 255});
        
        rect_t btn = {cx - 60, cy + 40, 120, 40};
        framebuffer_fill_rect(wm->framebuffer, btn, (color_t){0, 150, 255, 255});
        draw_string(wm->framebuffer, "ENTER", cx - 40, cy + 50, (color_t){255, 255, 255, 255});
        
    } else {
        // --- BUREAU ET FENÊTRES ---
        // Dessin d'une fausse icône de dossier sur le bureau
        framebuffer_fill_rect(wm->framebuffer, (rect_t){40, 40, 60, 50}, (color_t){255, 212, 0, 255});
        draw_string(wm->framebuffer, "DATA", 45, 100, (color_t){255, 255, 255, 255});

        for (s32 i = 0; i < wm->num_windows; i++) {
            window_t* win = wm->windows[i];
            if (win && win->base.visible) {
                rect_t b = win->base.bounds;
                // Fenêtre
                framebuffer_fill_rect(wm->framebuffer, b, (color_t){20, 25, 30, 255});
                // Barre de titre
                rect_t title_bar = {b.x, b.y, b.width, 25};
                framebuffer_fill_rect(wm->framebuffer, title_bar, (color_t){0, 160, 255, 255});
                
                // Texte de la barre de titre
                draw_string(wm->framebuffer, win->title, b.x + 10, b.y + 5, (color_t){255, 255, 255, 255});
                
                if (win == task_manager_window) {
                    draw_string(wm->framebuffer, "RAM USAGE", b.x + 20, b.y + 40, (color_t){200, 200, 200, 255});
                    uint64_t total = pmm_get_total_memory() / (1024*1024);
                    uint64_t used = pmm_get_used_memory() / (1024*1024);
                    if(total == 0) total = 1;
                    s32 gauge_width = (s32)((used * (b.width - 40)) / total);
                    framebuffer_fill_rect(wm->framebuffer, (rect_t){b.x + 20, b.y + 60, b.width - 40, 20}, (color_t){50, 50, 60, 255});
                    framebuffer_fill_rect(wm->framebuffer, (rect_t){b.x + 20, b.y + 60, gauge_width, 20}, (color_t){0, 255, 128, 255});
                }
                
                if (win == main_window) {
                    draw_string(wm->framebuffer, "C PROMPT", b.x + 10, b.y + 40, (color_t){0, 255, 0, 255});
                }
            }
        }
        
        // --- BARRE DES TÂCHES ---
        rect_t taskbar = {0, wm->framebuffer->height - 40, wm->framebuffer->width, 40};
        framebuffer_fill_rect(wm->framebuffer, taskbar, (color_t){10, 12, 18, 255});
        framebuffer_draw_line(wm->framebuffer, 0, wm->framebuffer->height - 40, wm->framebuffer->width, wm->framebuffer->height - 40, (color_t){0, 180, 255, 255});
        rect_t start_btn = {10, wm->framebuffer->height - 35, 40, 30};
        framebuffer_fill_rect(wm->framebuffer, start_btn, (color_t){0, 212, 255, 255});
    }
    
    draw_mouse_cursor(wm->framebuffer, mouse_x, mouse_y);
    framebuffer_present(wm->framebuffer);
}

void wm_update_mouse(int32_t dx, int32_t dy, uint8_t buttons) {
    if (!global_wm || !global_wm->framebuffer) return;
    
    mouse_x += dx;
    mouse_y -= dy;

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= global_wm->framebuffer->width) mouse_x = global_wm->framebuffer->width - 1;
    if (mouse_y >= global_wm->framebuffer->height) mouse_y = global_wm->framebuffer->height - 1;

    bool left_click = buttons & 0x01;
    
    if (left_click) {
        if (!is_logged_in) {
            s32 cx = global_wm->framebuffer->width / 2;
            s32 cy = global_wm->framebuffer->height / 2;
            if (mouse_x >= cx-60 && mouse_x <= cx+60 && mouse_y >= cy+40 && mouse_y <= cy+80) {
                is_logged_in = true;
            }
        } 
        else if (!dragged_window) {
            for (s32 i = global_wm->num_windows - 1; i >= 0; i--) {
                window_t* win = global_wm->windows[i];
                rect_t b = win->base.bounds;
                if (mouse_x >= b.x && mouse_x <= b.x + b.width && mouse_y >= b.y && mouse_y <= b.y + b.height) {
                    wm_bring_to_front(global_wm, i); 
                    if (mouse_y <= b.y + 25) { 
                        dragged_window = win;
                        drag_offset_x = mouse_x - b.x;
                        drag_offset_y = mouse_y - b.y;
                    }
                    break;
                }
            }
        }
    } else if (dragged_window) {
        dragged_window->base.bounds.x = mouse_x - drag_offset_x;
        dragged_window->base.bounds.y = mouse_y - drag_offset_y;
        dragged_window = NULL; // On arrête le drag au relâchement
    }
    
    wm_render(global_wm);
}

void jarvis_gui_init(void) {
    main_window = window_create("JARVIS TERMINAL", (rect_t){100, 100, 600, 400});
    wm_add_window(global_wm, main_window);
    
    task_manager_window = window_create("TASK MANAGER", (rect_t){750, 100, 300, 150});
    wm_add_window(global_wm, task_manager_window);
    
    wm_render(global_wm);
}

void gui_init(void) {
    framebuffer_t* fb = framebuffer_create(1024, 768, true);
    global_wm = wm_create();
    global_wm->framebuffer = fb;
    jarvis_gui_init();
}

window_manager_t* wm_create(void) {
    window_manager_t* wm = kmalloc(sizeof(window_manager_t));
    if (wm) { memset(wm, 0, sizeof(window_manager_t)); wm->capacity = 32; wm->windows = kmalloc(sizeof(window_t*) * wm->capacity); }
    return wm;
}
void wm_add_window(window_manager_t* wm, window_t* window) {
    if (wm && window && wm->num_windows < wm->capacity) wm->windows[wm->num_windows++] = window;
}
void wm_destroy(window_manager_t* wm) { if(wm) kfree(wm); }

void gui_shutdown(void) {}
void jarvis_gui_create_dashboard(void) {}
void jarvis_gui_create_terminal(void) {}
void draw_tech_border(framebuffer_t* fb, rect_t rect, color_t color) { (void)fb; (void)rect; (void)color; }
