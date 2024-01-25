.extern strap

// traps while running kernel code are taken here
.section .text._trapkernel
.globl _trapkernel
.align 4
_trapkernel:
    // save program state (all registers) on stack
    addi sp, sp, -256
    sd x1,  0x0(sp)
    sd x2,  0x08(sp)
    sd x3,  0x10(sp)
    // skip x4 because it's the tp register (which is constant per-cpu)
    sd x5,  0x20(sp)
    sd x6,  0x28(sp)
    sd x7,  0x30(sp)
    sd x8,  0x38(sp)
    sd x9,  0x40(sp)
    sd x10, 0x48(sp)
    sd x11, 0x50(sp)
    sd x12, 0x58(sp)
    sd x13, 0x60(sp)
    sd x14, 0x68(sp)
    sd x15, 0x70(sp)
    sd x16, 0x78(sp)
    sd x17, 0x80(sp)
    sd x18, 0x88(sp)
    sd x19, 0x90(sp)
    sd x20, 0x98(sp)
    sd x21, 0xa0(sp)
    sd x22, 0xa8(sp)
    sd x23, 0xb0(sp)
    sd x24, 0xb8(sp)
    sd x25, 0xc0(sp)
    sd x26, 0xc8(sp)
    sd x27, 0xd0(sp)
    sd x28, 0xd8(sp)
    sd x29, 0xe0(sp)
    sd x30, 0xe8(sp)
    sd x31, 0xf0(sp)

    // call supervisor trap handler in trap.c
    call strap

    // restore program state from stack
    ld x1,  0x0(sp)
    ld x2,  0x08(sp)
    ld x3,  0x10(sp)
    // skip x4
    ld x5,  0x20(sp)
    ld x6,  0x28(sp)
    ld x7,  0x30(sp)
    ld x8,  0x38(sp)
    ld x9,  0x40(sp)
    ld x10, 0x48(sp)
    ld x11, 0x50(sp)
    ld x12, 0x58(sp)
    ld x13, 0x60(sp)
    ld x14, 0x68(sp)
    ld x15, 0x70(sp)
    ld x16, 0x78(sp)
    ld x17, 0x80(sp)
    ld x18, 0x88(sp)
    ld x19, 0x90(sp)
    ld x20, 0x98(sp)
    ld x21, 0xa0(sp)
    ld x22, 0xa8(sp)
    ld x23, 0xb0(sp)
    ld x24, 0xb8(sp)
    ld x25, 0xc0(sp)
    ld x26, 0xc8(sp)
    ld x27, 0xd0(sp)
    ld x28, 0xd8(sp)
    ld x29, 0xe0(sp)
    ld x30, 0xe8(sp)
    ld x31, 0xf0(sp)
    addi sp, sp, 256

    // return to whatever was happening
    sret
