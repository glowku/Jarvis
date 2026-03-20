OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

/* Les PHDRS (Program Headers) forcent Limine et le Linker à séparer
   strictement le code exécutable des données pour éviter l'erreur RWX ! */
PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    /* On démarre en Higher Half */
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    /* Alignement impératif sur 4096 octets pour séparer les droits ! */
    . = ALIGN(0x1000);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(0x1000);

    .data : {
        /* Nos requêtes Limine */
        KEEP(*(.limine_reqs))
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
