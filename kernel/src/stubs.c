#include <kernel/types.h>
#include <limine.h>

// On informe le fichier que module_request existe ailleurs (dans main.c)
extern volatile struct limine_module_request module_request;

// --- Fonctions matérielles basiques ---
static inline void outb_stub(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_puts(const char* s) {
    while (*s) { outb_stub(0x3F8, *s++); }
}

// --- KMALLOC / KFREE TEMPORAIRES (32 Mo pour l'animation !) ---
static u8 heap_memory[32 * 1024 * 1024];
static size_t heap_offset = 0;

void* kmalloc(size_t size) {
    size = ALIGN_UP(size, 8);
    if (heap_offset + size > sizeof(heap_memory)) return NULL;
    void* ptr = &heap_memory[heap_offset];
    heap_offset += size;
    return ptr;
}

void kfree(void* ptr) { (void)ptr; }

extern phys_addr_t pmm_alloc_page(void);
void* find_free_memory(size_t size) {
    return kmalloc(size);
}

void process_sleep(u64 ms) {
    volatile u64 count = ms * 10000;
    while (count--) { __asm__ volatile("pause"); }
}

void panic(const char* message) {
    serial_puts("\n!!! KERNEL PANIC !!!\n");
    serial_puts(message);
    serial_puts("\nSystem Halted.\n");
    while (1) { __asm__ volatile("cli; hlt"); }
}

// --- MATH ---
#pragma GCC target("sse,sse2")
void sincosf(float x, float *s, float *c) {
    *s = __builtin_sinf(x);
    *c = __builtin_cosf(x);
}

// --- ZLIB ---
extern size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

void* MZ_MALLOC(size_t size) { return kmalloc(size); }
void MZ_FREE(void* ptr) { kfree(ptr); }
void* MZ_REALLOC(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) { kfree(ptr); return NULL; }
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        for(size_t i=0; i<1024; i++) ((u8*)new_ptr)[i] = ((u8*)ptr)[i]; 
    }
    return new_ptr;
}

int jarvis_decompress_zlib(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len) {
    if (!src || !dst) return -1;
    size_t res = tinfl_decompress_mem_to_mem(dst, dst_len, src, src_len, 4);
    if (res == (size_t)-1 || res == 0) return -1;
    return 0;
}
