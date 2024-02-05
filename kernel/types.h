#ifndef __TYPES_H
#define __TYPES_H

// basic integer types
typedef unsigned char u8;
typedef signed char i8;

typedef unsigned long long u64;
typedef signed long long i64;

typedef unsigned int u32;
typedef signed int i32;

typedef unsigned short u16;
typedef signed short i16;

typedef unsigned long usize;
typedef signed long isize;

// float types
typedef float f32;
typedef double f64;
typedef long double f128;


// boolean type
typedef int boolean;
#define BOOL boolean
#define TRUE 1
#define FALSE 0


#define NULL ((void*)0)

#endif // __TYPES_H
