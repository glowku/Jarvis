#include <stdint.h>
#include <kernel/interrupt.h>

extern void serial_puts(const char* s);
extern void wm_update_mouse(int32_t dx, int32_t dy, uint8_t buttons);
extern void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, a_write);
}

static uint8_t mouse_read() {
    mouse_wait(0); return inb(0x60);
}

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

// --- LE VRAI HANDLER D'INTERRUPTION PS/2 ---
__attribute__((interrupt, target("general-regs-only"))) 
void mouse_isr(interrupt_frame_t *frame) {
    (void)frame;
    
    uint8_t status = inb(0x64);
    while (status & 0x01) {
        int8_t mouse_in = inb(0x60);
        if (status & 0x20) {
            switch(mouse_cycle) {
                case 0:
                    mouse_byte[0] = mouse_in;
                    if (!(mouse_in & 0x08)) break; // Bit 3 doit toujours être à 1
                    mouse_cycle++;
                    break;
                case 1:
                    mouse_byte[1] = mouse_in;
                    mouse_cycle++;
                    break;
                case 2:
                    mouse_byte[2] = mouse_in;
                    mouse_cycle = 0;
                    // Envoi au Window Manager !
                    wm_update_mouse(mouse_byte[1], mouse_byte[2], mouse_byte[0] & 0x07);
                    break;
            }
        }
        status = inb(0x64);
    }
    
    // Acquittement VITAL pour le contrôleur PIC
    outb(0xA0, 0x20); // Slave
    outb(0x20, 0x20); // Master
}

void mouse_init(void) {
    uint8_t status;
    mouse_wait(1); outb(0x64, 0xA8); 
    mouse_wait(1); outb(0x64, 0x20); 
    status = mouse_read() | 2;       
    mouse_wait(1); outb(0x64, 0x60); 
    mouse_wait(1); outb(0x60, status);
    
    mouse_write(0xF6); mouse_read(); 
    mouse_write(0xF4); mouse_read(); 
    
    // On accroche notre fonction à l'interruption matérielle 44
    idt_set_descriptor(44, mouse_isr, 0x8E);
    serial_puts("[DRV] Souris PS/2 (Mode IRQ) initialisee.\n");
}
