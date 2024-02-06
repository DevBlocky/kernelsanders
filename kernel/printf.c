#include "kernel.h"
#include "riscv.h"
#include <stdarg.h>

// uart register functions and defs

#define RBR 0
#define IER 1
#define IER_RDA (1 << 0)
#define IER_THRE (1 << 1)
#define LSR 5
#define LSR_HAS_DATA (1 << 0)
#define LSR_TX_IDLE (1 << 5)

#define UART0REG(reg) ((volatile u8 *)(UART0_MMIO + (reg)))

static void uartputc(char c) {
  // spin while uart cannot receive anymore data
  while ((*UART0REG(LSR) & LSR_TX_IDLE) == 0)
    ;
  *UART0REG(RBR) = (u8)c;
}
static void uartputs(const char *s) {
  while (*s != '\0')
    uartputc(*s++);
}

void uartintr(void) {
  // read data while available
  // ! this doesn't do anything with the data, but
  // ! it needs to be "read" so the intr clears
  while (*UART0REG(LSR) & LSR_HAS_DATA)
    *UART0REG(RBR);
}
void uartinit(void) {
  // enable interrupts
  // NOTE: also needs to be enabled from PLIC
  *UART0REG(IER) = IER_RDA;
}

static const char *digits = "0123456789abcdef";

static void printfunsigned(u64 val, int base) {
  int i = 0;
  char buff[32];
  do {
    buff[i++] = digits[val % base];
    val /= base;
  } while (val != 0);

  while (--i >= 0)
    uartputc(buff[i]);
}
static void printfsigned(i64 val, int base) {
  if (val < 0) {
    uartputc('-');
    val = -val;
  }
  printfunsigned((u64)val, base);
}

void printf(const char *format, ...) {
  va_list args;
  char c;
  va_start(args, format);
  for (usize i = 0; (c = format[i]) != 0; i++) {
    if (c != '%') {
      uartputc(c);
      continue;
    }
    c = format[++i];
    // turn on hexmode if %hx (hex decimal) where x in an integer format type
    int base = 10;
    if (c == 'h')
      base = 16;
    else if (c == 'b')
      base = 2;
    if (base != 10)
      c = format[++i];

    switch (c) {
    case 'u': // usigned int
      printfunsigned((u64)va_arg(args, u32), base);
      break;
    case 'l': // unsigned long
      printfunsigned((u64)va_arg(args, usize), base);
      break;
    case 'p': // unsigned long long
      printfunsigned((u64)va_arg(args, u64), base);
      break;
    case 'd': // signed int
      printfsigned((i64)va_arg(args, i32), base);
      break;
    case 's':
      const char *val = va_arg(args, char *);
      if (val != NULL)
        uartputs(val);
      break;
    case 'c':
      const char c = (char)va_arg(args, int);
      uartputc(c);
      break;
    case '%':
      uartputc('%');
      break;
    default:
      uartputs("*invalid*");
    }
  }
}
