#pragma GCC target("sse,sse2") // <--- LE PRAGMA QUI RÈGLE L'ERREUR DE COMPILATION

#include <kernel/types.h>
#include <limine.h>
#include <stdint.h>
#include <stddef.h>

extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_module_request module_request;
extern void serial_puts(const char* s);
extern void* find_free_memory(size_t size);
extern int jarvis_decompress_zlib(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len);

static struct limine_framebuffer *fb = NULL;
static uint32_t *backbuffer = NULL;

struct AnimationHeader { 
    char magic[4]; 
    uint32_t width; 
    uint32_t height; 
    uint32_t frame_count; 
} __attribute__((packed));

struct PixelChange { 
    uint32_t index; 
    uint32_t color; 
} __attribute__((packed));

void gpu_init(void) {
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        for (;;) { __asm__ volatile("cli; hlt"); }
    }
    fb = framebuffer_request.response->framebuffers[0];
    
    if (!backbuffer) {
        backbuffer = (uint32_t*)find_free_memory(fb->width * fb->height * sizeof(uint32_t));
    }
}

// Étirement de ton animation vidéo pour remplir tout l'écran !
void draw_frame_fullscreen(uint32_t* frame_pixels, uint32_t frame_w, uint32_t frame_h) {
    if (!fb || !backbuffer || !frame_pixels) return;
    
    for (uint32_t y = 0; y < fb->height; y++) {
        uint32_t *screen_line = (uint32_t *)((uint8_t *)fb->address + (y * fb->pitch));
        uint32_t src_y = (y * frame_h) / fb->height;
        
        for (uint32_t x = 0; x < fb->width; x++) { 
            uint32_t src_x = (x * frame_w) / fb->width;
            screen_line[x] = frame_pixels[src_y * frame_w + src_x]; 
        }
    }
}

void play_boot_animation(void) {
    serial_puts("[+] DBG: Entree dans play_boot_animation.\n");
    gpu_init();
    
    if (module_request.response == NULL || module_request.response->module_count < 1) return;
    struct limine_file *anim_file = module_request.response->modules[0];
    
    uint8_t *file_data = (uint8_t *)anim_file->address;
    if (!file_data) return;
    
    struct AnimationHeader *header = (struct AnimationHeader *)file_data;
    if (header->magic[0] != 'Z' && header->magic[0] != 'L') return;
    
    uint8_t *ptr = file_data + sizeof(struct AnimationHeader);
    
    uint32_t frame_bytes = header->width * header->height * 4;
    uint8_t *raw_frame_buffer = (uint8_t*)find_free_memory(frame_bytes);
    if (!raw_frame_buffer) return;

    for (uint32_t i = 0; i < header->frame_count; i++) {
        uint32_t compressed_size = *(uint32_t*)ptr;
        ptr += 4;
        
        if (jarvis_decompress_zlib(ptr, compressed_size, raw_frame_buffer, frame_bytes) != 0) break;
        
        draw_frame_fullscreen((uint32_t*)raw_frame_buffer, header->width, header->height);
        
        ptr += compressed_size;
        for (volatile int j = 0; j < 5000000; j++); 
    }
}
