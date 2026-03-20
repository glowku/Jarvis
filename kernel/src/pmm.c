#include <kernel/memory.h>
#include <kernel/console.h>
#include <limine.h>
#include <stddef.h>
#include <stdbool.h>

__attribute__((used, section(".limine_reqs"), aligned(8)))
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0
};

extern volatile struct limine_hhdm_request hhdm_request;

static pmm_t pmm;

void pmm_init(void) {
    if (!memmap_request.response || !hhdm_request.response) return;
    
    struct limine_memmap_response *map = memmap_request.response;
    uint64_t highest_addr = 0;
    
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *en = map->entries[i];
        if (en->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = en->base + en->length;
            if (top > highest_addr) highest_addr = top;
        }
    }
    
    pmm.total_pages = highest_addr / 4096;
    pmm.bitmap_size = pmm.total_pages / 8;
    
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *en = map->entries[i];
        if (en->type == LIMINE_MEMMAP_USABLE && en->length >= pmm.bitmap_size) {
            pmm.bitmap = (uint8_t*)PHYS_TO_VIRT(en->base);
            pmm.bitmap_phys = en->base;
            en->base += pmm.bitmap_size;
            en->length -= pmm.bitmap_size;
            break;
        }
    }
    
    for (uint64_t i = 0; i < pmm.bitmap_size; i++) pmm.bitmap[i] = 0xFF; // Tout occupé
    
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *en = map->entries[i];
        if (en->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t j = 0; j < en->length; j += 4096) {
                uint64_t page_idx = (en->base + j) / 4096;
                if (page_idx < pmm.total_pages) {
                    pmm.bitmap[page_idx / 8] &= ~(1 << (page_idx % 8)); // Libérer
                }
            }
        }
    }
}

phys_addr_t pmm_alloc_page(void) {
    for (uint64_t i = 0; i < pmm.total_pages; i++) {
        if (!(pmm.bitmap[i / 8] & (1 << (i % 8)))) {
            pmm.bitmap[i / 8] |= (1 << (i % 8));
            return (phys_addr_t)(i * 4096);
        }
    }
    return 0;
}

void pmm_free_page(phys_addr_t addr) {
    uint64_t page_idx = (uint64_t)addr / 4096;
    if (page_idx < pmm.total_pages) {
        pmm.bitmap[page_idx / 8] &= ~(1 << (page_idx % 8));
    }
}

// --- FONCTIONS POUR LE GESTIONNAIRE DES TACHES (SECU RISÉES) ---
uint64_t pmm_get_total_memory(void) {
    if (!pmm.bitmap || pmm.total_pages == 0) return 32 * 1024 * 1024; // Fausse info de 32 Mo si erreur
    return pmm.total_pages * 4096;
}

uint64_t pmm_get_used_memory(void) {
    if (!pmm.bitmap || pmm.total_pages == 0) return 8 * 1024 * 1024; // Fausse info de 8 Mo si erreur
    
    uint64_t used = 0;
    for (uint64_t i = 0; i < pmm.total_pages; i++) {
        if (pmm.bitmap[i / 8] & (1 << (i % 8))) {
            used++;
        }
    }
    return used * 4096;
}

