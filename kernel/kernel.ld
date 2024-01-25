OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
    /*
        define the start of our program in memory
        this is where qemu will jump to
    */
    . = 0x80000000;
    PROVIDE(kstart = .);

    .text : {
        *(.text._start .text*)
    }

    . = ALIGN(0x1000);
    PROVIDE(ktextend = .);

    .rodata : {
        . = ALIGN(0x10);
        *(.srodata*)
        . = ALIGN(0x10);
        *(.rodata*)
    }

    .data : {
        . = ALIGN(0x10);
        *(.sdata*)
        . = ALIGN(0x10);
        *(.data*)
    }

    .bss : {
        . = ALIGN(0x10);
        *(.sbss*)
        . = ALIGN(0x10);
        *(.bss*)
    }
    
    PROVIDE(kend = .);
}