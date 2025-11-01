#include "core/num_defs.h"
#include <utils/hash/murmur2_hash.h>

u32 murmur2_hash32_seed(const u8 *data, u32 length, u32 seed)
{
    // r & m are 32 constants
    const u32 m = 0x5BD1E995; 
    const u32 r = 24;

    // Init hash
    u32 hash = seed ^ length;

    // go over in sizeof(u32) chunks
    u32 length4 = length/4;
    for (u32 i4 = 0; i4 < length4; i4++)
    {
        const u32 index = i4 * 4;

        u32 k = (
            ((u32)(data[index + 0]&0xFF) << 0)  + 
            ((u32)(data[index + 1]&0xFF) << 8)  + 
            ((u32)(data[index + 2]&0xFF) << 16) + 
            ((u32)(data[index + 3]&0xFF) << 24)
        );

        k *= m;
        k ^= k >> r;
        k *= m;

        hash *= m;
        hash ^= k;
    }

    // Handle last few bytes
    switch (length%4) 
    {
    case 3: hash ^= ((u32)data[(length & ~(0b11)) + 2] & 0xFF) << 16;
    case 2: hash ^= ((u32)data[(length & ~(0b11)) + 1] & 0xFF) << 8;
    case 1: hash ^= ((u32)data[(length & ~(0b11)) + 0] & 0xFF) << 0;
            hash *= m;
    }

    // Last manipulation
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return hash;
}


u32 murmur2_hash32(const u8 *data, u32 length)
{
    // default seed
    const u32 seed = 0x9747B28C;

    return murmur2_hash32_seed(data, length, seed);
}

u64 murmur2_hash64_seed(const u8 *data, u64 length, u32 seed)
{
        // r & m are 32 constants
    const u64 m = 0xC6A4A7935BD1E995; 
    const u64 r = 47;

    // Init hash
    u64 hash = seed ^ length;

    // go over in sizeof(u64) chunks
    u64 length8 = length/8;
    for (u32 i8 = 0; i8 < length8; i8++)
    {
        const u32 index = i8 * 8;

        u64 k = (
            ((u64)(data[index + 0]&0xFF) << 0)  + 
            ((u64)(data[index + 1]&0xFF) << 8)  + 
            ((u64)(data[index + 2]&0xFF) << 16) + 
            ((u64)(data[index + 3]&0xFF) << 24) +
            ((u64)(data[index + 4]&0xFF) << 32) +
            ((u64)(data[index + 5]&0xFF) << 40) +
            ((u64)(data[index + 6]&0xFF) << 48) +
            ((u64)(data[index + 7]&0xFF) << 56)
        );

        k *= m;
        k ^= k >> r;
        k *= m;

        hash *= m;
        hash ^= k;
    }

    // Handle last few bytes
    switch (length%8) 
    {
    case 7: hash ^= (u64)(data[(length & ~(0b111)) + 6] & 0xFF) << 48;
    case 6: hash ^= (u64)(data[(length & ~(0b111)) + 5] & 0xFF) << 40;
    case 5: hash ^= (u64)(data[(length & ~(0b111)) + 4] & 0xFF) << 32;
    case 4: hash ^= (u64)(data[(length & ~(0b111)) + 3] & 0xFF) << 24;
    case 3: hash ^= (u64)(data[(length & ~(0b111)) + 2] & 0xFF) << 16;
    case 2: hash ^= (u64)(data[(length & ~(0b111)) + 1] & 0xFF) << 8;
    case 1: hash ^= (u64)(data[(length & ~(0b111)) + 0] & 0xFF) << 0;
            hash *= m;
    }

    // Last manipulation
    hash ^= hash >> r;
    hash *= m;
    hash ^= hash >> r;

    return hash;
}


u64 murmur2_hash64(const u8* data, u64 length) 
{
    return murmur2_hash64_seed(data, length, 0xe17a1465);
}