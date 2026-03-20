#include <stdint.h>
#include <kernel/interrupt.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Mapping QWERTY basique (Scancode 1)
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,   
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*', 0, ' '
};

extern void serial_puts(const char* s);
extern void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);

// Le gestionnaire d'interruption du clavier
// Ajout de target("general-regs-only") pour empêcher GCC d'utiliser MMX/SSE ici
__attribute__((interrupt, target("general-regs-only"))) 
void keyboard_isr(interrupt_frame_t *frame) {
    uint8_t scancode = inb(0x60); // Lit la touche matérielle
    
    // Si c'est une touche pressée (pas relâchée)
    if (scancode < 128) {
        char key = kbd_us[scancode];
        if (key) {
            // Pour l'instant on l'envoie sur le port série pour prouver que ça marche !
            char str[2] = {key, '\0'};
            serial_puts(str);
            // PLUS TARD : On enverra cette touche à LVGL ici !
        }
    }
    outb(0x20, 0x20); // Dit au contrôleur PIC "Message reçu, envoie le suivant"
}

void keyboard_init() {
    // L'IRQ du clavier est sur la broche 1 du PIC, qui est remappée sur 33 (0x21)
    idt_set_descriptor(33, keyboard_isr, 0x8E);
}
