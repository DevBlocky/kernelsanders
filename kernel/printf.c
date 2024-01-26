#include "kernel.h"
#include <stdarg.h>
#include "riscv.h"

static void uartputc(char c)
{
    *(volatile char *)UART0_MMIO = c;
}
static void uartputs(const char *s)
{
    while (*s != '\0')
        uartputc(*s++);
}

const char *digits = "0123456789abcdef";

static void printfunsigned(uint64_t val, int base)
{
    int i = 0;
    char buff[32];
    do
    {
        buff[i++] = digits[val % base];
        val /= base;
    } while (val != 0);

    while (--i >= 0)
        uartputc(buff[i]);
}
static void printfsigned(int64_t val, int base)
{
    if (val < 0)
    {
        uartputc('-');
        val = -val;
    }
    printfunsigned((uint64_t)val, base);
}

void printf(const char *format, ...)
{
    va_list args;
    char c;
    va_start(args, format);
    for (usize_t i = 0; (c = format[i]) != 0; i++)
    {
        if (c != '%')
        {
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

        switch (c)
        {
        case 'u': // usigned int
            printfunsigned((uint64_t)va_arg(args, uint32_t), base);
            break;
        case 'p': // unsigned long long
            printfunsigned((uint64_t)va_arg(args, uint64_t), base);
            break;
        case 'd': // signed decimal
            printfsigned((int64_t)va_arg(args, int32_t), base);
            break;
        case 's':
            const char *val = va_arg(args, char *);
            if (val != NULL)
                uartputs(val);
            break;
        case '%':
            uartputc('%');
            break;
        default:
            uartputs("*invalid*");
        }
    }
}

void panic(const char *s)
{
    // NOTE: if the OS is ever made multi-core, this will
    // need to panic other cores too
    printf("\n!!! kernel panic !!!\nmsg: %s\n", s);
    while (1)
        ;
}
