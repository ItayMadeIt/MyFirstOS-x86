#include <stdatomic.h>
#include <stdbool.h>
#include "core/num_defs.h"


typedef _Atomic bool atom_bool;

typedef _Atomic u8   atom_u8;
typedef _Atomic u16  atom_u16;
typedef _Atomic u32  atom_u32;
typedef _Atomic u64  atom_u64;

typedef _Atomic i8   atom_i8;
typedef _Atomic i16  atom_i16;
typedef _Atomic i32  atom_i32;
typedef _Atomic i64  atom_i64;

typedef _Atomic usize atom_usize;
typedef _Atomic isize atom_isize;

typedef _Atomic uptr  atom_uptr;
typedef _Atomic iptr  atom_iptr;

typedef _Atomic usize_ptr atom_usize_ptr;
typedef _Atomic isize_ptr atom_isize_ptr;
