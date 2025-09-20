#include <stdint.h>

// implementation from https://github.com/tnm/murmurhash-java/blob/master/src/main/java/ie/ucd/murmur/MurmurHash.java

uint32_t murmur2_hash32_seed(const uint8_t* data, uint32_t length, uint32_t seed);
uint32_t murmur2_hash32     (const uint8_t* data, uint32_t length);

uint64_t murmur2_hash64_seed(const uint8_t* data, uint64_t length, uint32_t seed);
uint64_t murmur2_hash64     (const uint8_t* data, uint64_t length);