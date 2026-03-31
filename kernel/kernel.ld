OUTPUT_ARCH("riscv")
ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);    /* R-X */
    rodata PT_LOAD FLAGS(4);  /* R-- */
    data PT_LOAD FLAGS(6);    /* RW- */
}

SECTIONS
{
    /*
        define the start of our program in memory
        this is where qemu will jump to
    */
    . = 0x80000000;
    PROVIDE(kstart = .);

    .text : {
        KEEP(*(.text._start)) /* make sure _start is first */
        *(.text*) /* rest of the code */
    } :text

    . = ALIGN(0x1000);
    PROVIDE(ktextend = .);

    .rodata : {
        . = ALIGN(0x10);
        *(.srodata*)
        . = ALIGN(0x10);
        *(.rodata*)
    } :rodata

    .data : {
        . = ALIGN(0x10);
        *(.sdata*)
        . = ALIGN(0x10);
        *(.data*)
    } :data

    .bss : {
        . = ALIGN(0x10);
        *(.sbss*)
        . = ALIGN(0x10);
        *(.bss*)
    } :data

    PROVIDE(kend = .);
}
