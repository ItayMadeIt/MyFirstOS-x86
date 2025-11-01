#ifndef __NUM_DEFS_H__
#define __NUM_DEFS_H__

#include <stdint.h>
#include <stddef.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u64 usize;
typedef i64 ssize; 

typedef uintptr_t uptr;
typedef intptr_t  sptr;

typedef uintptr_t usize_ptr;
typedef intptr_t  ssize_ptr;
#endif // __NUM_DEFS_H__