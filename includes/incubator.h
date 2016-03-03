#ifndef INCUBATOR_H_
#define INCUBATOR_H_

#include <stdint.h>
#include <map>
#include <utility>
#include <vector>
#include <cstring>
#include <iostream>

#define I_HEIGHT 112

struct Incube {
    uint64_t d[I_HEIGHT];
    uint64_t hist[I_HEIGHT];

    struct Incube *neighbours[8];
    int updateflags;

    int tx;
    int ty;
};

class incubator {

public:
    std::map<std::pair<int, int>, Incube> tiles;

    /*
     * The integer i refers to the location of the returned SquareTile
     * relative to the parameter sqt.
     *
     * 0 = up;
     * 1 = up-right;
     * 2 = right;
     * ...
     * 7 = up-left.
     */
     Incube* getNeighbour(Incube* sqt, int i) {

        // We cache pointers to adjacent SquareTiles in an array, thereby
        // limiting the number of times we access the std::map.
        if (!(sqt->neighbours[i])) {
            int x = sqt->tx;
            int y = sqt->ty;

            if ((i >= 1) && (i <= 3))
                x += 1;

            if ((i >= 3) && (i <= 5))
                y += 1;

            if ((i >= 5) && (i <= 7))
                x -= 1;

            if ((i == 7) || (i <= 1))
                y -= 1;

            sqt->neighbours[i] = &tiles[std::make_pair(x, y)];

            sqt->neighbours[i]->tx = x;
            sqt->neighbours[i]->ty = y;
        }

        return sqt->neighbours[i];
    }

    void setcell(Incube* sqt, int x, int y, int state, bool overclock) {

        uint64_t mask = (1ull << (55 - x));

        if (state == 1) {
            sqt->d[y] |= mask;
        } else {
            sqt->d[y] &= ~mask;
            if (state == 2) {
                sqt->hist[y] |= mask;
            } else {
                sqt->hist[y] &= ~mask;
            }
        }

    }

    void setcell(int x, int y, int state) {
        int ox = x % 56;
        int oy = y % I_HEIGHT;
        if (ox < 0)
            ox += 56;
        if (oy < 0)
            oy += I_HEIGHT;

        int tx = (x - ox) / 56;
        int ty = (y - oy) / I_HEIGHT;

        Incube* sqt = &tiles[std::make_pair(tx, ty)];
        sqt->tx = tx;
        sqt->ty = ty;

        setcell(sqt, ox, oy, state, false);
    }

    int getcell(Incube* sqt, int x, int y) {

        uint64_t mask = (1ull << (55 - x));

        if (sqt->d[y] & mask) {
            return 1;
        } else {
            if (sqt->hist[y] & mask) {
                return 2;
            } else {
                return 0;
            }
        }
    }

    int getcell(int x, int y) {
        int ox = x % 56;
        int oy = y % I_HEIGHT;
        if (ox < 0)
            ox += 56;
        if (oy < 0)
            oy += I_HEIGHT;

        int tx = (x - ox) / 56;
        int ty = (y - oy) / I_HEIGHT;

        Incube* sqt = &tiles[std::make_pair(tx, ty)];
        sqt->tx = tx;
        sqt->ty = ty;

        return getcell(sqt, ox, oy);
    }

    int isGlider(Incube* sqt, int px, int py, bool nuke, uint64_t* cachearray) {

        if (cachearray[py] & (1ull << (55 - px)))
            return 2;

        if ((px < 2) || (py < 2) || (px > 53) || (py > I_HEIGHT - 5))
            return 0;

        int x = px;
        int y = py + 1;

        // The provided cell could be any of the five live cells, but we
        // want to find the centre of the glider instead.

        /*
        if ((sqt->d[y-1] & (31ull << (61 - x))) == 0) {
            y += 1;
            std::cout << "above ";
        } else if ((sqt->d[y+1] & (31ull << (61 - x))) == 0) {
            y -= 1;
            std::cout << "below ";
        } else {
            std::cout << "middle ";
        }
        */

        if ((sqt->d[y-2] | sqt->d[y+2]) & (31ull << (53 - x)))
            return 0;

        int projection = ((sqt->d[y+1] | sqt->d[y] | sqt->d[y-1]) >> (53 - x)) & 31ull;

        // std::cout << projection << " ";

        if (projection == 7) {
            x += 1; // ..ooo
        } else if (projection == 14) {
            // .ooo.
        } else if (projection == 28) {
            x -= 1; // ooo..
        } else {
            // The shadow does not match that of a glider.
            return 0;
        }

        // Now (x, y) should be the central cell of the putative glider.

        if ((x < 3) || (x > 52)) {
            return 0;
        } else if ((sqt->d[y] | sqt->d[y-1] | sqt->d[y+1]) & (99ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-2] | sqt->d[y+2]) & (127ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-3] | sqt->d[y+3]) & (60ull << (52 - x))) {
            return 0;
        } else {
            // 512 bits to indicate which 16 of the 512 3-by-3 bitpatterns correspond
            // to a glider in some orientation and phase.
            unsigned long long array [] = {
                    0x0000000000000000ull,
                    0x0400000000800000ull,
                    0x0000000000000000ull,
                    0x0010044000200000ull,
                    0x0400000000800000ull,
                    0x0010004002000800ull,
                    0x0000040002200800ull,
                    0x0000000000000000ull};

            int high3 = ((sqt->d[y]) >> (54 - x)) & 7ull;
            int low6 = (((sqt->d[y-1]) >> (54 - x)) & 7ull) | (((sqt->d[y+1]) >> (51 - x)) & 56ull);

            if (array[high3] & (1ull << low6)) {

                cachearray[y-1] |= (7ull << (54 - x));
                cachearray[y] |= (7ull << (54 - x));
                cachearray[y+1] |= (7ull << (54 - x));

                if (nuke) {
                    // Destroy the glider:
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            setcell(sqt, x+i, y+j, 0, true);
                        }
                    }
                }
                // std::cout << ((high3 << 6) | low6) << std::endl;
                return 1;
            } else {
                return 0;
            }
        }
    }

    int isBlinker(Incube* sqt, int x, int y) {
        if ((x < 3) || (y < 3) || (x > 52) || (y > I_HEIGHT - 4)) {
            return 0;
        } else if ((sqt->d[y] | sqt->hist[y]) & (99ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+1] | sqt->hist[y+1]) & (54ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+2] | sqt->hist[y+2]) & (62ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-3] | sqt->hist[y-3] | sqt->d[y+3] | sqt->hist[y+3]) & (8ull << (52 - x))) {
            return 0;
        } else {
            // std::cout << "Blinker detected." << std::endl;
            setcell(sqt, x, y, 0, true);
            setcell(sqt, x+1, y, 0, true);
            setcell(sqt, x, y+1, 0, true);
            setcell(sqt, x-1, y, 0, true);
            setcell(sqt, x, y-1, 0, true);
            return 3;
        }
    }

    int isVerticalBeehive(Incube* sqt, int x, int y) {
        if ((x < 3) || (y < 2) || (x > 52) || (y > I_HEIGHT - 6)) {
            return 0;
        } else if ((sqt->d[y+1] | sqt->hist[y+1] | sqt->d[y+2] | sqt->hist[y+2]) & (107ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y] | sqt->hist[y] | sqt->d[y+3] | sqt->hist[y+3]) & (119ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+4] | sqt->hist[y+4]) & (62ull << (52 - x))) {
            return 0;
        } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+5] | sqt->hist[y+5]) & (8ull << (52 - x))) {
            return 0;
        } else {
            setcell(sqt, x, y, 0, true);
            setcell(sqt, x+1, y+1, 0, true);
            setcell(sqt, x-1, y+1, 0, true);
            setcell(sqt, x-1, y+2, 0, true);
            setcell(sqt, x+1, y+2, 0, true);
            setcell(sqt, x, y+3, 0, true);
            return 6;
        }
    }

    int isAnnoyance(Incube* sqt, int x, int y) {

        if ((x < 2) || (y < 2) || (x > 52) || (y > I_HEIGHT - 4)) {
            return 0;
        } else {
            if ((sqt->d[y] >> (54 - x)) & 1) {
                if ((sqt->d[y+1] >> (55 - x)) & 1) {
                    if ((sqt->d[y] | sqt->hist[y] | sqt->d[y+1] | sqt->hist[y+1]) & (51ull << (52 - x))) {
                        return 0;
                    } else if ((sqt->d[y-1] | sqt->hist[y-1] | sqt->d[y+2] | sqt->hist[y+2]) & (63ull << (52 - x))) {
                        return 0;
                    } else if ((sqt->d[y-2] | sqt->hist[y-2] | sqt->d[y+3] | sqt->hist[y+3]) & (30ull << (52 - x))) {
                        return 0;
                    } else {
                        // std::cout << "Block detected." << std::endl;
                        setcell(sqt, x, y, 0, true);
                        setcell(sqt, x+1, y, 0, true);
                        setcell(sqt, x, y+1, 0, true);
                        setcell(sqt, x+1, y+1, 0, true);
                        return 4;
                    }
                } else {
                    return isBlinker(sqt, x+1, y);
                }
            } else {
                if ((sqt->d[y+1] >> (55 - x)) & 1) {
                    return isBlinker(sqt, x, y+1);
                } else {
                    return isVerticalBeehive(sqt, x, y);
                }
            }
        }
    }

    std::vector<int> getComponent(Incube* sqt, int x, int y, int maxnorm) {

        int i = 0;
        std::vector<Incube*> tileList;
        std::vector<int> intList;
        int ll = 1;

        tileList.push_back(sqt);
        intList.push_back(x);
        intList.push_back(y);
        intList.push_back(getcell(sqt, x, y));
        setcell(sqt, x, y, 0, true);

        int population = 1;

        while (i < ll) {

            Incube* sqt1 = tileList[i];
            int ox = intList[3*i];
            int oy = intList[3*i + 1];

            // std::cout << i << ": " << ox << "," << oy << std::endl;

            for (int rx = ox - 2; rx <= ox + 2; rx++) {
                for (int ry = oy - 2; ry <= oy + 2; ry++) {

                    int px = rx;
                    int py = ry;

                    int norm = (px - ox)*(px - ox) + (py - oy)*(py - oy);

                    if (norm <= maxnorm) {
                        Incube* sqt2 = sqt1;

                        if (px < 0) {
                            px += 56;
                            sqt2 = getNeighbour(sqt2, 6);
                        } else if (px >= 56) {
                            px -= 56;
                            sqt2 = getNeighbour(sqt2, 2);
                        }

                        if (py < 0) {
                            py += I_HEIGHT;
                            sqt2 = getNeighbour(sqt2, 0);
                        } else if (py >= I_HEIGHT) {
                            py -= I_HEIGHT;
                            sqt2 = getNeighbour(sqt2, 4);
                        }

                        int v = getcell(sqt2, px, py);

                        if (v > 0) {
                            setcell(sqt2, px, py, 0, true);

                            tileList.push_back(sqt2);
                            intList.push_back(px);
                            intList.push_back(py);
                            intList.push_back(v);
                            ll += 1;

                            if (v == 1)
                                population += 1;
                        }
                    }
                }
            }

            i += 1;
        }

        for (int j = 0; j < ll; j++) {
            Incube* sqt3 = tileList[j];
            intList[j*3] += 56 * sqt3->tx;
            intList[j*3 + 1] += I_HEIGHT * sqt3->ty;
        }

        intList.push_back(population);

        return intList;

    }

    std::vector<int> getComponent(Incube* sqt, int x, int y) {
        return getComponent(sqt, x, y, 5);
    }

    std::vector<int> getIsland(Incube* sqt, int x, int y) {
        std::vector<int> island = getComponent(sqt, x, y, 2);
        return island;
    }

};



#endif /* INCUBATOR_H_ */
