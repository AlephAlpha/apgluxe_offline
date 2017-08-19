#include "../lifelib/bitworld.h"

namespace apg {

    // Reverse each byte in an integer:
    uint64_t uint64_hreflect(uint64_t x) {
        uint64_t y = ((x & 0xaaaaaaaaaaaaaaaaull) >> 1) | ((x & 0x5555555555555555ull) << 1);
                 y = ((y & 0xccccccccccccccccull) >> 2) | ((y & 0x3333333333333333ull) << 2);
                 y = ((y & 0xf0f0f0f0f0f0f0f0ull) >> 4) | ((y & 0x0f0f0f0f0f0f0f0full) << 4);
        return y;
    }

    // Reverse each byte in an integer:
    uint64_t uint64_vreflect(uint64_t x) {
        uint64_t y = ((x & 0xff00ff00ff00ff00ull) >> 8)  | ((x & 0x00ff00ff00ff00ffull) << 8);
                 y = ((y & 0xffff0000ffff0000ull) >> 16) | ((y & 0x0000ffff0000ffffull) << 16);
                 y = ((y & 0xffffffff00000000ull) >> 32) | ((y & 0x00000000ffffffffull) << 32);
        return y;
    }


    // Transpose an 8-by-8 matrix (from Hacker's Delight):
    void transpose8rS32(uint8_t* A, int m, int n, uint8_t* B) {
        unsigned x, y, t;

        // Load the array and pack it into x and y.

        x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
        y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];

        t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);

        t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
        t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);

        t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        x = t;

        B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x;
        B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y;
    }

    // Transpose a 16-by-16 matrix:
    void transpose16(unsigned char* A, unsigned char* B) {
        transpose8rS32(A, 2, 2, B);
        transpose8rS32(A+16, 2, 2, B+1);
        transpose8rS32(A+1, 2, 2, B+16);
        transpose8rS32(A+17, 2, 2, B+17);
    }

// Produce a SHA-256 hash of a string, and use it to generate a soup:
bitworld hashsoup(std::string prehash, std::string symmetry) {

    uint8_t digest[32];
    memset(digest, 0, 32);

    SHA256 ctx = SHA256();
    ctx.init();
    ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
    ctx.final(digest);

    uint8_t tsegid[32];
    memset(tsegid, 0, 32);
    transpose16(digest, tsegid);

    if ((symmetry == "D2_x") || (symmetry == "D8_4") || (symmetry == "D8_1") || (symmetry == "D4_x4") || (symmetry == "D4_x1")) {
        // We make our arrays diagonally symmetric:
        uint8_t diggid[32];
        memset(diggid, 0, 32);
        for (int i = 0; i < 8; i++) {
            diggid[2*i] = (digest[2*i] & ((1 << (8 - i)) - 1)) | (tsegid[2*i] & (256 - (1 << (8 - i))));
            diggid[2*i + 17] = (digest[2*i + 17] & ((1 << (8 - i)) - 1)) | (tsegid[2*i + 17] & (256 - (1 << (8 - i))));
            diggid[2*i + 1] = digest[2*i + 1];
            diggid[2*i + 16] = tsegid[2*i + 16];
        }

        for (int i = 0; i < 32; i++) {
            tsegid[i] ^= (diggid[i] ^ digest[i]);
        }
        std::memcpy(digest, diggid, 32);
    }

    bitworld bw;

    if ((symmetry == "8x32") || (symmetry == "4x64") || (symmetry == "2x128") || (symmetry == "1x256")) {
        int height = symmetry[0] - 48;
        int eighthwidth = 32 / height;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < eighthwidth; i++) {
                bw.world[std::pair<int32_t, int32_t>(eighthwidth - 1 - i, 0)] |= (((uint64_t) digest[eighthwidth*j+i]) << (8*j));
            }
        }
        return bw;
    }

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

    bw.world[std::pair<int32_t, int32_t>(0, 0)] = b;
    bw.world[std::pair<int32_t, int32_t>(1, 0)] = a;
    bw.world[std::pair<int32_t, int32_t>(0, 1)] = d;
    bw.world[std::pair<int32_t, int32_t>(1, 1)] = c;

    if ((symmetry == "C1") || (symmetry == "D2_x")) { return bw; }

    bitworld dbw;
    dbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_vreflect(uint64_hreflect(c));
    dbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_vreflect(uint64_hreflect(d));
    dbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_vreflect(uint64_hreflect(a));
    dbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_vreflect(uint64_hreflect(b));

    if ((symmetry == "C4_1") || (symmetry == "C4_4") || (symmetry == "D4_x1") || (symmetry == "D4_x4")) {
        a = 0; b = 0; c = 0; d = 0;
        for (int j = 0; j < 8; j++) {
            a |= (((uint64_t) tsegid[2*j]) << 8*j);
            b |= (((uint64_t) tsegid[2*j+1]) << 8*j);
            c |= (((uint64_t) tsegid[2*j+16]) << 8*j);
            d |= (((uint64_t) tsegid[2*j+17]) << 8*j);
        }
    }

    bitworld vbw;
    vbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_vreflect(d);
    vbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_vreflect(c);
    vbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_vreflect(b);
    vbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_vreflect(a);

    if (symmetry == "D2_+1") { bw += shift_bitworld(vbw, 0, -15); return bw; }
    if (symmetry == "D2_+2") { bw += shift_bitworld(vbw, 0, -16); return bw; }

    bitworld hbw;
    hbw.world[std::pair<int32_t, int32_t>(0, 0)] = uint64_hreflect(a);
    hbw.world[std::pair<int32_t, int32_t>(1, 0)] = uint64_hreflect(b);
    hbw.world[std::pair<int32_t, int32_t>(0, 1)] = uint64_hreflect(c);
    hbw.world[std::pair<int32_t, int32_t>(1, 1)] = uint64_hreflect(d);

    if (symmetry == "C2_4") {
        bw += shift_bitworld(dbw, 16, -16);
    } else if (symmetry == "C2_2") {
        bw += shift_bitworld(dbw, 16, -15);
    } else if (symmetry == "C2_1") {
        bw += shift_bitworld(dbw, 15, -15);
    } else if ((symmetry == "D8_4") || (symmetry == "D4_+4") || (symmetry == "D4_x4") || (symmetry == "C4_4")) {
        bw += shift_bitworld(vbw, 0, -16);
        bw += shift_bitworld(hbw, 16, 0);
        bw += shift_bitworld(dbw, 16, -16);
    } else if (symmetry == "D4_+2") {
        bw += shift_bitworld(vbw, 0, -15);
        bw += shift_bitworld(hbw, 16, 0);
        bw += shift_bitworld(dbw, 16, -15);
    } else if ((symmetry == "D8_1") || (symmetry == "D4_+1") || (symmetry == "D4_x1") || (symmetry == "C4_1")) {
        bw += shift_bitworld(vbw, 0, -15);
        bw += shift_bitworld(hbw, 15, 0);
        bw += shift_bitworld(dbw, 15, -15);
    }

    return bw;

}


}
