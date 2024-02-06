#ifndef __RISCV_H
#define __RISCV_H

#include "types.h"

extern char kstart[];
extern char ktextend[];
extern char kend[];
#define PHYSTOP (kstart + 128 * 1024 * 1024)

#define PGSIZE 4096
#define PGCEIL(a) (((a) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGFLOOR(a) ((a) & ~(PGSIZE - 1))

#define CLINT_MMIO 0x2000000
// for some reason, the offsets of CLINT registers are not well documented
// (especially the CLINT_MTIME) I could only find it here (section 6.1):
// https://sifive.cdn.prismic.io/sifive%2Fc89f6e5a-cf9e-44c3-a3db-04420702dcc1_sifive+e31+manual+v19.08.pdf
#define CLINT_MTIMECMP (volatile u64 *)(CLINT_MMIO + 0x4000)
#define CLINT_MTIME (volatile u64 *)(CLINT_MMIO + 0xBFF8)

#define PLIC_MMIO 0xc000000UL

#define UART0_MMIO 0x10000000UL

#define PCI_MMIO 0x30000000UL

//
// machine-mode csr registers
//

#define MSTATUS_MIE (1 << 3)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S (1 << 11)

#define MIE_MTIE (1 << 7)

static inline usize r_mstatus() {
  usize status;
  asm volatile("csrr %0, mstatus" : "=r"(status));
  return status;
}
static inline void w_mstatus(usize mstatus) {
  asm volatile("csrw mstatus, %0" : : "r"(mstatus));
}

static inline void w_mtvec(usize vector) {
  asm volatile("csrw mtvec, %0" : : "r"(vector));
}
static inline void w_mepc(usize addr) {
  asm volatile("csrw mepc, %0" : : "r"(addr));
}
static inline void w_mie(usize flags) {
  asm volatile("csrw mie, %0" : : "r"(flags));
}
static inline void w_mscratch(usize any) {
  asm volatile("csrw mscratch, %0" : : "r"(any));
}

static inline void w_mideleg(usize flags) {
  asm volatile("csrw mideleg, %0" : : "r"(flags));
}
static inline void w_medeleg(usize flags) {
  asm volatile("csrw medeleg, %0" : : "r"(flags));
}

static inline void w_pmpaddr0(usize addr) {
  asm volatile("csrw pmpaddr0, %0" : : "r"(addr));
}
static inline void w_pmpcfg0(usize cfg) {
  asm volatile("csrw pmpcfg0, %0" : : "r"(cfg));
}

//
// supervisor-mode csr registers
//

#define SSTATUS_SIE (1 << 1)

#define SIP_SSIP (1 << 1)
#define SIE_SSIE (1 << 1)
#define SIE_SEIE (1 << 9)

static inline usize r_sstatus() {
  usize status;
  asm volatile("csrr %0, sstatus" : "=r"(status));
  return status;
}
static inline void w_sstatus(usize status) {
  asm volatile("csrw sstatus, %0" : : "r"(status));
}

static inline void w_stvec(usize addr) {
  asm volatile("csrw stvec, %0" : : "r"(addr));
}
static inline void w_sie(usize flags) {
  asm volatile("csrw sie, %0" : : "r"(flags));
}
static inline usize r_sip() {
  usize flags;
  asm volatile("csrr %0, sip" : "=r"(flags));
  return flags;
}
static inline void w_sip(usize flags) {
  asm volatile("csrw sip, %0" : : "r"(flags));
}

static inline usize r_scause() {
  usize code;
  asm volatile("csrr %0, scause" : "=r"(code));
  return code;
}

static inline void w_satp(usize addr) {
  asm volatile("csrw satp, %0" : : "r"(addr));
}
static inline void sfence_vma() { asm volatile("sfence.vma zero, zero"); }

static inline void intr_on() { w_sstatus(r_sstatus() | SSTATUS_SIE); }
static inline void intr_off() { w_sstatus(r_sstatus() & ~SSTATUS_SIE); }

#endif // __RISCV_H
