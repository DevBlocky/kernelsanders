#ifndef __ALLOC_H
#define __ALLOC_H

#include "types.h"

void allocinit(void);
void pgfree(void *page);
void *pgalloc(void);

void memset(void *ptr, usize_t val, usize_t size);

#endif // __ALLOC_H
