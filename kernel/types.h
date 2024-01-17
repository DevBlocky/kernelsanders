#ifndef __TYPES_H
#define __TYPES_H

// basic integer types
typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned long long uint64_t;
typedef signed long long int64_t;

typedef unsigned int uint32_t;
typedef signed int int32_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned long usize_t;
typedef signed long isize_t;


// boolean type
typedef int boolean;
#define BOOL boolean
#define TRUE 1
#define FALSE 0


#define NULL ((void*)0)

#endif // __TYPES_H
