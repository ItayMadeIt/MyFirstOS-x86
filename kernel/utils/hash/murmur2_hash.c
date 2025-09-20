#include <stdint.h>
#include <utils/hash/murmur2_hash.h>

uint32_t murmur2_hash32_seed(const uint8_t *data, uint32_t length, uint32_t seed)
{
    // r & m are 32 constants
    const uint32_t m = 0x5BD1E995; 
    const uint32_t r = 24;

    // Init hash
    uint32_t hash = seed ^ length;

    // go over in sizeof(uint32_t) chunks
    uint32_t length4 = length/4;
    for (uint32_t i4 = 0; i4 < length4; i4++)
    {
        const uint32_t index = i4 * 4;

        uint32_t k = (
            ((uint32_t)(data[index + 0]&0xFF) << 0)  + 
            ((uint32_t)(data[index + 1]&0xFF) << 8)  + 
            ((uint32_t)(data[index + 2]&0xFF) << 16) + 
            ((uint32_t)(data[index + 3]&0xFF) << 24)
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
    case 3: hash ^= ((uint32_t)data[(length & ~(0b11)) + 2] & 0xFF) << 16;
    case 2: hash ^= ((uint32_t)data[(length & ~(0b11)) + 1] & 0xFF) << 8;
    case 1: hash ^= ((uint32_t)data[(length & ~(0b11)) + 0] & 0xFF) << 0;
            hash *= m;
    }

    // Last manipulation
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return hash;
}


uint32_t murmur2_hash32(const uint8_t *data, uint32_t length)
{
    // default seed
    const uint32_t seed = 0x9747B28C;

    return murmur2_hash32_seed(data, length, seed);
}

uint64_t murmur2_hash64_seed(const uint8_t *data, uint64_t length, uint32_t seed)
{
        // r & m are 32 constants
    const uint64_t m = 0xC6A4A7935BD1E995; 
    const uint64_t r = 47;

    // Init hash
    uint64_t hash = seed ^ length;

    // go over in sizeof(uint64_t) chunks
    uint64_t length8 = length/8;
    for (uint32_t i8 = 0; i8 < length8; i8++)
    {
        const uint32_t index = i8 * 8;

        uint64_t k = (
            ((uint64_t)(data[index + 0]&0xFF) << 0)  + 
            ((uint64_t)(data[index + 1]&0xFF) << 8)  + 
            ((uint64_t)(data[index + 2]&0xFF) << 16) + 
            ((uint64_t)(data[index + 3]&0xFF) << 24) +
            ((uint64_t)(data[index + 4]&0xFF) << 32) +
            ((uint64_t)(data[index + 5]&0xFF) << 40) +
            ((uint64_t)(data[index + 6]&0xFF) << 48) +
            ((uint64_t)(data[index + 7]&0xFF) << 56)
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
    case 7: hash ^= (uint64_t)(data[(length & ~(0b111)) + 6] & 0xFF) << 48;
    case 6: hash ^= (uint64_t)(data[(length & ~(0b111)) + 5] & 0xFF) << 40;
    case 5: hash ^= (uint64_t)(data[(length & ~(0b111)) + 4] & 0xFF) << 32;
    case 4: hash ^= (uint64_t)(data[(length & ~(0b111)) + 3] & 0xFF) << 24;
    case 3: hash ^= (uint64_t)(data[(length & ~(0b111)) + 2] & 0xFF) << 16;
    case 2: hash ^= (uint64_t)(data[(length & ~(0b111)) + 1] & 0xFF) << 8;
    case 1: hash ^= (uint64_t)(data[(length & ~(0b111)) + 0] & 0xFF) << 0;
            hash *= m;
    }

    // Last manipulation
    hash ^= hash >> r;
    hash *= m;
    hash ^= hash >> r;

    return hash;
}


uint64_t murmur2_hash64(const uint8_t* data, uint64_t length) 
{
    return murmur2_hash64_seed(data, length, 0xe17a1465);
}