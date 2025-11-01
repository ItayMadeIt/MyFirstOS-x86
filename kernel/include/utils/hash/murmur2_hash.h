#include "core/num_defs.h"

// implementation from https://github.com/tnm/murmurhash-java/blob/master/src/main/java/ie/ucd/murmur/MurmurHash.java

u32 murmur2_hash32_seed(const u8* data, u32 length, u32 seed);
u32 murmur2_hash32     (const u8* data, u32 length);

u64 murmur2_hash64_seed(const u8* data, u64 length, u32 seed);
u64 murmur2_hash64     (const u8* data, u64 length);