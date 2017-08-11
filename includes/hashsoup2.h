#include "../lifelib/bitworld.h"

// Produce a SHA-256 hash of a string, and use it to generate a soup:
apg::bitworld hashsoup(std::string prehash, std::string symmetry) {

    unsigned char digest[SHA256::DIGEST_SIZE];
    memset(digest,0,SHA256::DIGEST_SIZE);

    SHA256 ctx = SHA256();
    ctx.init();
    ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
    ctx.final(digest);

    uint64_t a = 0;
    uint64_t b = 0;
    uint64_t c = 0;
    uint64_t d = 0;
    for (int j = 0; j < 8; j++) {
        a |= (((uint64_t) digest[2*j]) << 8*j);
        b |= (((uint64_t) digest[2*j+1]) << 8*j);
        c |= (((uint64_t) digest[2*j+16]) << 8*j);
        d |= (((uint64_t) digest[2*j+17]) << 8*j);
    }

    apg::bitworld bw;
    bw.world[std::pair<int32_t, int32_t>(0, 0)] = b;
    bw.world[std::pair<int32_t, int32_t>(1, 0)] = a;
    bw.world[std::pair<int32_t, int32_t>(0, 1)] = d;
    bw.world[std::pair<int32_t, int32_t>(1, 1)] = c;
    return bw;

}
