
// Reverse each byte in an integer:
uint32_t reverse_uint8(uint32_t x) {

    uint32_t y = ((x & 0xaaaaaaaau) >> 1) | ((x & 0x55555555u) << 1);
    y = ((y & 0xccccccccu) >> 2) | ((y & 0x33333333u) << 2);
    y = ((y & 0xf0f0f0f0u) >> 4) | ((y & 0x0f0f0f0fu) << 4);
    return y;

}

// Transpose an 8-by-8 matrix (from Hacker's Delight):
void transpose8rS32(unsigned char* A, int m, int n, unsigned char* B) {
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
void hashsoup(vlife* imp, std::string prehash, std::string symmetry) {

    unsigned char digest[SHA256::DIGEST_SIZE];
    memset(digest,0,SHA256::DIGEST_SIZE);

    SHA256 ctx = SHA256();
    ctx.init();
    ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
    ctx.final(digest);

    // Find the central tile:
    VersaTile* sqt = &(imp->tiles[std::make_pair(0, 0)]);
    sqt->updateflags = 256;

    // Dump the soup into the array:
    #ifdef C1_SYMMETRY
    for (int j = 0; j < 16; j++) {
        sqt->d[j+(ROWS / 2 - 8)] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
    }
    #else
    if (symmetry == "D2_+1") {
        for (int j = 0; j < 16; j++) {
            sqt->d[(ROWS / 2) + j] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
            sqt->d[(ROWS / 2) - j] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
        }
    } else if (symmetry == "D2_+2") {
        for (int j = 0; j < 16; j++) {
            sqt->d[(ROWS / 2) + j] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
            sqt->d[(ROWS / 2) - j - 1] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
        }
    } else {

        // Find the tile immediately to the left of centre (off of the strip?)
        VersaTile* sqt2 = &(imp->tiles[std::make_pair(-1, 0)]);
        sqt2->updateflags = 260;
        sqt->updateflags = 288;

        if (symmetry == "8x32") {
            for (int j = 0; j < 8; j++) {
                sqt->d[(ROWS / 2) + j - 4] = (digest[4*j+2] << 22) + (digest[4*j+3] << 14);
                sqt2->d[(ROWS / 2) + j - 4] = (digest[4*j+1] << 2) + (digest[4*j] << 10);
            }
        } else if (symmetry == "D4_+4") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                sqt->d[(ROWS / 2) - j - 1] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                sqt2->d[(ROWS / 2) + j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
                sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else if (symmetry == "D4_+2") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                sqt->d[(ROWS / 2) - j] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                sqt2->d[(ROWS / 2) + j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
                sqt2->d[(ROWS / 2) - j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else if (symmetry == "D4_+1") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 23) + (digest[2*j+1] << 15);
                sqt->d[(ROWS / 2) - j] = (digest[2*j] << 23) + (digest[2*j+1] << 15);
                sqt2->d[(ROWS / 2) + j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
                sqt2->d[(ROWS / 2) - j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else if (symmetry == "C2_4") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else if (symmetry == "C2_2") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 23) + (digest[2*j+1] << 15);
                sqt2->d[(ROWS / 2) + j] = ((reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10)) & 7;
                sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else if (symmetry == "C2_1") {
            for (int j = 0; j < 16; j++) {
                sqt->d[(ROWS / 2) + j] = (digest[2*j] << 23) + (digest[2*j+1] << 15);
                sqt2->d[(ROWS / 2) + j] = ((reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10)) & 7;
                sqt2->d[(ROWS / 2) - j] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
            }
        } else {
            unsigned char tsegid[SHA256::DIGEST_SIZE];
            memset(tsegid,0,SHA256::DIGEST_SIZE);
            transpose16(digest, tsegid);

            if (symmetry == "C4_4") {
                for (int j = 0; j < 16; j++) {
                    sqt->d[(ROWS / 2) + j] = (digest[2*j] << 22) + (digest[2*j+1] << 14);
                    sqt->d[(ROWS / 2) - j - 1] = (tsegid[2*j] << 22) + (tsegid[2*j+1] << 14);
                    sqt2->d[(ROWS / 2) + j] = (reverse_uint8(tsegid[2*j]) << 2) + (reverse_uint8(tsegid[2*j+1]) << 10);
                    sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
                }
            } else if (symmetry == "C4_1") {
                for (int j = 0; j < 16; j++) {
                    sqt->d[(ROWS / 2) + j] |= (digest[2*j] << 23) + (digest[2*j+1] << 15);
                    sqt->d[(ROWS / 2) - j] |= (tsegid[2*j] << 23) + (tsegid[2*j+1] << 15);
                    sqt2->d[(ROWS / 2) + j] |= (reverse_uint8(tsegid[2*j]) << 2) + (reverse_uint8(tsegid[2*j+1]) << 10);
                    sqt2->d[(ROWS / 2) - j] |= (reverse_uint8(digest[2*j]) << 2) + (reverse_uint8(digest[2*j+1]) << 10);
                }
                for (int j = -15; j < 16; j++) {
                    sqt2->d[(ROWS / 2) + j] |= (sqt2->d[(ROWS / 2) - j] & 7);
                }
            } else {
                unsigned char diggid[SHA256::DIGEST_SIZE];
                memset(diggid,0,SHA256::DIGEST_SIZE);
                for (int i = 0; i < 8; i++) {
                    diggid[2*i] = (digest[2*i] & ((1 << (8 - i)) - 1)) | (tsegid[2*i] & (256 - (1 << (8 - i))));
                    diggid[2*i + 17] = (digest[2*i + 17] & ((1 << (8 - i)) - 1)) | (tsegid[2*i + 17] & (256 - (1 << (8 - i))));
                    diggid[2*i + 1] = digest[2*i + 1];
                    diggid[2*i + 16] = tsegid[2*i + 16];
                }
                if (symmetry == "D2_x") {
                    for (int j = 0; j < 16; j++) {
                        sqt->d[j+(ROWS / 2 - 8)] = (diggid[2*j] << 16) + (diggid[2*j+1] << 8);
                    }
                } else if (symmetry == "D8_4") {
                    for (int j = 0; j < 16; j++) {
                        sqt->d[(ROWS / 2) + j] = (diggid[2*j] << 22) + (diggid[2*j+1] << 14);
                        sqt->d[(ROWS / 2) - j - 1] = (diggid[2*j] << 22) + (diggid[2*j+1] << 14);
                        sqt2->d[(ROWS / 2) + j] = (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                        sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                    }
                } else if (symmetry == "D8_1") {
                    for (int j = 0; j < 16; j++) {
                        sqt->d[(ROWS / 2) + j] = (diggid[2*j] << 23) + (diggid[2*j+1] << 15);
                        sqt->d[(ROWS / 2) - j] = (diggid[2*j] << 23) + (diggid[2*j+1] << 15);
                        sqt2->d[(ROWS / 2) + j] = (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                        sqt2->d[(ROWS / 2) - j] = (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                    }
                } else {
                    unsigned char tseest[SHA256::DIGEST_SIZE];
                    memset(tseest,0,SHA256::DIGEST_SIZE);
                    for (int i = 0; i < 32; i++) {
                        tseest[i] = diggid[i] ^ digest[i] ^ tsegid[i];
                    }
                    if (symmetry == "D4_x4") {
                        for (int j = 0; j < 16; j++) {
                            sqt->d[(ROWS / 2) + j] = (diggid[2*j] << 22) + (diggid[2*j+1] << 14);
                            sqt->d[(ROWS / 2) - j - 1] = (tseest[2*j] << 22) + (tseest[2*j+1] << 14);
                            sqt2->d[(ROWS / 2) + j] = (reverse_uint8(tseest[2*j]) << 2) + (reverse_uint8(tseest[2*j+1]) << 10);
                            sqt2->d[(ROWS / 2) - j - 1] = (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                        }
                    } else if (symmetry == "D4_x1") {
                        for (int j = 0; j < 16; j++) {
                            sqt->d[(ROWS / 2) + j] |= (diggid[2*j] << 23) + (diggid[2*j+1] << 15);
                            sqt->d[(ROWS / 2) - j] |= (tseest[2*j] << 23) + (tseest[2*j+1] << 15);
                            sqt2->d[(ROWS / 2) + j] |= (reverse_uint8(tseest[2*j]) << 2) + (reverse_uint8(tseest[2*j+1]) << 10);
                            sqt2->d[(ROWS / 2) - j] |= (reverse_uint8(diggid[2*j]) << 2) + (reverse_uint8(diggid[2*j+1]) << 10);
                        }
                        for (int j = -15; j < 16; j++) {
                            sqt2->d[(ROWS / 2) + j] |= (sqt2->d[(ROWS / 2) - j] & 7);
                        }
                    } else {
                        std::cout << "Invalid symmetry" << std::endl;
                    }
                }
            }
        }

        // Indicate that the tile has been modified:
        imp->modified.push_back(sqt2);
    }
    #endif

    // Indicate that the tile has been modified:
    imp->modified.push_back(sqt);
}
