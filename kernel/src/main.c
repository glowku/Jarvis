#include <kernel/types.h>
#include <limine.h>

LIMINE_BASE_REVISION(1)

void kmain(void);

__attribute__((used, section(".limine_reqs"), aligned(8))) volatile struct limine_entry_point_request entry_request = {.id = LIMINE_ENTRY_POINT_REQUEST, .revision = 0, .entry = kmain};
__attribute__((used, section(".limine_reqs"), aligned(8))) volatile struct limine_framebuffer_request framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};
__attribute__((used, section(".limine_reqs"), aligned(8))) volatile struct limine_hhdm_request hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 0};
__attribute__((used, section(".limine_reqs"), aligned(8))) volatile struct limine_module_request module_request = {.id = LIMINE_MODULE_REQUEST, .revision = 0};

extern void interrupt_init(void);
extern void pmm_init(void);
extern void gui_init(void);
extern void mouse_init(void);
extern void play_boot_animation(void);
extern void serial_puts(const char* s);

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t ret; __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }

void unmask_mouse(void) {
    // 1. Autoriser le Slave PIC à parler au Master (IRQ 2)
    outb(0x21, inb(0x21) & ~(1 << 2));
    // 2. Autoriser la souris sur le Slave PIC (IRQ 12 -> bit 4)
    outb(0xA1, inb(0xA1) & ~(1 << 4));
}

void enable_sse(void) {
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9) | (1 << 10);
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
}

void kmain(void) {
    __asm__ volatile("cli");
    
    enable_sse();
    interrupt_init();
    pmm_init();
    
    // Animation de boot (Plein Ecran)
    play_boot_animation();
    
    // Initialisation
    gui_init();
    mouse_init();
    
    // ALLUMAGE DES INTERRUPTIONS (Le CPU est prêt à recevoir les clics de souris)
    unmask_mouse(); 
    __asm__ volatile("sti");
    
    serial_puts("[+] JARVIS EN LIGNE. Attente des IRQ souris...\n");
    
    while (1) {
        // Le processeur dort et se réveille dès que tu bouges la souris ! (Economie d'énergie)
        __asm__ volatile("hlt");
    }
}

void _start(void) {
    kmain();
}
