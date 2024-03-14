#include "kernel.h"

void memset(void *ptr, usize val, usize size) {
  // set by cpu word size
  usize *dst = (usize *)ptr;
  for (usize i = 0; i < size / sizeof(usize); i++)
    *dst++ = val;

  // set remaining bytes
  u8 *dst2 = (u8 *)dst;
  for (usize i = 0; i < size % sizeof(usize); i++)
    *dst2++ = ((u8 *)&val)[i];
}
void memcpy(void *dst, void *src, usize size) {
  // copy by cpu word size
  usize *ldst = dst, *lsrc = src;
  for (usize i = 0; i < size / sizeof(usize); i++)
    *ldst++ = *lsrc++;

  // copy remaining bytes
  u8 *sdst = (u8 *)ldst, *ssrc = (u8 *)lsrc;
  for (usize i = 0; i < size % sizeof(usize); i++)
    *sdst++ = *ssrc++;
}

usize strlen(const char *c) {
  usize len = 0;
  while (*c++)
    len++;
  return len;
}
int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return (int)*a - (int)*b;
}

u32 be2cpu32(u32 be) {
  // swap bytes around (assuming CPU is little endian)
  return ((be & 0xff) << 24) | ((be & 0xff00) << 8) | ((be & 0xff0000) >> 8) |
         ((be & 0xff000000) >> 24);
}
u64 be2cpu64(u64 be) {
  // swap bytes around (assuming CPU is little endian)
  return ((be & 0xff) << 56) | ((be & 0xff00) << 40) | ((be & 0xff0000) << 24) |
         ((be & 0xff000000) << 8) | ((be & 0xff00000000) >> 8) |
         ((be & 0xff0000000000) >> 24) | ((be & 0xff000000000000) >> 40) |
         ((be & 0xff00000000000000) >> 56);
}
