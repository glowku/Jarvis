#include <stdint.h>
#include <stddef.h>
#include <kernel/interrupt.h>

extern void serial_puts(const char* s);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Les structures vitales de l'IDT
struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

__attribute__((aligned(0x10))) 
static struct idt_entry idt[256];
static struct idtr idtr;

__attribute__((interrupt, target("general-regs-only")))
void default_isr(interrupt_frame_t* frame) {
    (void)frame;
    // Acquittement des PIC (Master et Slave) pour ne pas figer QEMU
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;
    idt[vector].isr_low    = (uint16_t)(addr & 0xFFFF);
    idt[vector].kernel_cs  = 0x28; // Segment de code Limine x86_64
    idt[vector].ist        = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].isr_high   = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved   = 0;
}

void pic_remap(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFF); outb(0xA1, 0xFF);
}

void interrupt_init(void) {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;
    
    for (uint8_t vector = 0; vector < 255; vector++) {
        idt_set_descriptor(vector, default_isr, 0x8E);
    }
    
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    pic_remap();
    serial_puts("[IDT] Interruptions configurees.\n");
}
