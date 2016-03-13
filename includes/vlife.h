#ifndef VLIFE_H_
#define VLIFE_H_

#include <stdint.h>
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <utility>
#include <cpuid.h>

#include "params.h"

#define TVSPACE (ROWS - 4)

const urow_t globarray[] = {MIDDLE28, MIDDLE28, MIDDLE28, MIDDLE28, MIDDLE28, MIDDLE28, MIDDLE28, MIDDLE28, 1, 2, 3, 4, 5, 6, 7, 0};

int __best_instruction_set = 0;

/*
* Apple Bottom's vector instruction set detector (modified)
*/
int get_instruction_set() {

    if (__best_instruction_set == 0) {

        uint32_t eax, ebx, ecx, edx;

        uint32_t have_sse2 = 0;
        uint32_t have_avx1 = 0;
        uint32_t have_avx2 = 0;

        __cpuid(0, eax, ebx, ecx, edx);
        uint32_t max_level = eax;
        __cpuid(1, eax, ebx, ecx, edx);

        have_sse2 = (edx & (1 << 26));
        have_avx1 = (ecx & (1 << 28));

        if(max_level >= 7) {
            __cpuid_count(7, 0, eax, ebx, ecx, edx);
            have_avx2 = (ebx & (1 << 5));
        }

        if (have_avx2) {
            std::cout << "Instruction set \033[1mAVX2\033[0m supported." << std::endl;
            __best_instruction_set = 3;
        } else if (have_avx1) {
            std::cout << "Instruction set \033[1mAVX1\033[0m supported." << std::endl;
            __best_instruction_set = 2;
        } else if (have_sse2) {
            std::cout << "Instruction set \033[1mSSE2\033[0m supported." << std::endl;
            __best_instruction_set = 1;
        } else {
            std::cout << "SSE2 unsupported; please use apgsearch 1.x." << std::endl;
            __best_instruction_set = -1;
        }


    }

    return __best_instruction_set;

}

struct VersaTile {

    urow_t d[ROWS] /* __attribute__((aligned (32))) */;
    urow_t hist[ROWS] /* __attribute__((aligned (32))) */;

    struct VersaTile *neighbours[6];

    int updateflags;
    int population;
    int tx;
    int tw;

    uint64_t hash;

    bool populationCurrent;
    bool hashCurrent;

};


class vlife {

public:
    std::map<std::pair<int, int>, VersaTile> tiles;
    std::vector<VersaTile*> modified;
    std::vector<VersaTile*> temp_modified;

    int tilesProcessed;

    VersaTile* getNeighbour(VersaTile* sqt, int i) {

        // We cache pointers to adjacent VersaTiles in an array, thereby
        // limiting the number of times we access the std::map.
        if (!(sqt->neighbours[i])) {
            int x = sqt->tx;
            int w = sqt->tw;

            if ((i >= 1) && (i <= 2))
                x += 1;

            if ((i >= 3) && (i <= 4))
                w += 1;

            if ((i >= 4) && (i <= 5))
                x -= 1;

            if (i <= 1)
                w -= 1;

            sqt->neighbours[i] = &tiles[std::make_pair(x, w)];

            sqt->neighbours[i]->tx = x;
            sqt->neighbours[i]->tw = w;
        }

        return sqt->neighbours[i];
    }

    /*
     * Alert the neighbour that its neighbour (the original tile) has changed:
     */
    void updateNeighbour(VersaTile* sqt, int i) {

        if (getNeighbour(sqt, i)->updateflags == 0)
            modified.push_back(getNeighbour(sqt, i));

        getNeighbour(sqt, i)->updateflags |= (1 << ((i + 3) % 6));
    }

    void updateBoundary(VersaTile* sqt) {

#if BITTAGE == 64
        const urow_t right30  = 0x3fffffffffffffffull;
        const urow_t left30   = 0xfffffffffffffffcull;
        const urow_t right16  = 0x00000000ffffffffull;
        const urow_t left16   = 0xffffffff00000000ull;
        const urow_t middle28 = 0x3ffffffffffffffcull;
#else
        const urow_t right30  = 0x3fffffffu;
        const urow_t left30   = 0xfffffffcu;
        const urow_t right16  = 0x0000ffffu;
        const urow_t left16   = 0xffff0000u;
        const urow_t middle28 = 0x3ffffffcu;
#endif


        if (sqt->updateflags & (1 << 0)) {
            VersaTile* n = getNeighbour(sqt, 0);
            sqt->d[0] = ((n->d[TVSPACE] & middle28) << (THSPACE / 2)) | (sqt->d[0] & right16);
            sqt->d[1] = ((n->d[TVSPACE + 1] & middle28) << (THSPACE / 2)) | (sqt->d[1] & right16);
        }

        if (sqt->updateflags & (1 << 1)) {
            VersaTile* n = getNeighbour(sqt, 1);
            sqt->d[0] = ((n->d[TVSPACE] & middle28) >> (THSPACE / 2)) | (sqt->d[0] & left16);
            sqt->d[1] = ((n->d[TVSPACE + 1] & middle28) >> (THSPACE / 2)) | (sqt->d[1] & left16);
        }

        if (sqt->updateflags & (1 << 2)) {
            VersaTile* n = getNeighbour(sqt, 2);
            for (int i = 2; i < TVSPACE + 2; i++) {
                sqt->d[i] = ((n->d[i] & middle28) >> THSPACE) | (sqt->d[i] & left30);
            }
        }

        if (sqt->updateflags & (1 << 3)) {
            VersaTile* n = getNeighbour(sqt, 3);
            sqt->d[TVSPACE + 2] = ((n->d[2] & middle28) >> (THSPACE / 2)) | (sqt->d[TVSPACE + 2] & left16);
            sqt->d[TVSPACE + 3] = ((n->d[3] & middle28) >> (THSPACE / 2)) | (sqt->d[TVSPACE + 3] & left16);
        }

        if (sqt->updateflags & (1 << 4)) {
            VersaTile* n = getNeighbour(sqt, 4);
            sqt->d[TVSPACE + 2] = ((n->d[2] & middle28) << (THSPACE / 2)) | (sqt->d[TVSPACE + 2] & right16);
            sqt->d[TVSPACE + 3] = ((n->d[3] & middle28) << (THSPACE / 2)) | (sqt->d[TVSPACE + 3] & right16);
        }

        if (sqt->updateflags & (1 << 5)) {
            VersaTile* n = getNeighbour(sqt, 5);
            for (int i = 2; i < TVSPACE + 2; i++) {
                sqt->d[i] = ((n->d[i] & middle28) << THSPACE) | (sqt->d[i] & right30);
            }
        }

        sqt->updateflags = 0;
        temp_modified.push_back(sqt);
    }

    #include "lifeasm-avx2.h"
    #include "lifeasm-avx1.h"
    #include "lifeasm-sse2.h"

    uint64_t hashTile(VersaTile* sqt) {

        if (sqt->hashCurrent)
            return sqt->hash;

        uint64_t partialhash = 0;

        for (int i = 2; i < TVSPACE; i++) {
            partialhash = partialhash * (partialhash +447840759955ull) + i * sqt->d[i];
        }

        sqt->hash = partialhash;
        sqt->hashCurrent = true;
        return partialhash;

    }

    /*
     * Advance the entire universe by two generations.
     */
    void run2gens(bool history) {
        while (!modified.empty()) {
            updateBoundary(modified.back());
            modified.pop_back();
        }

        int instruction_set = get_instruction_set();

        while (!temp_modified.empty()) {
            if (instruction_set == 3) {
                if (history) {
                    updateTile_avx2_history(temp_modified.back());
                } else {
                    updateTile_avx2_nohistory(temp_modified.back());
                }
            } else if (instruction_set == 2) {
                if (history) {
                    updateTile_avx1_history(temp_modified.back());
                } else {
                    updateTile_avx1_nohistory(temp_modified.back());
                }
            } else {
                if (history) {
                    updateTile_sse2_history(temp_modified.back());
                } else {
                    updateTile_sse2_nohistory(temp_modified.back());
                }
            }
            temp_modified.pop_back();
            tilesProcessed += 1;
        }
    }

    int countPopulation(VersaTile* sqt) {

        if (sqt->populationCurrent)
            return sqt->population;

        int pop = 0;

        for (int i = 2; i < TVSPACE + 2; i++) {
            pop += __builtin_popcount(sqt->d[i] & MIDDLE28);
        }

        sqt->population = pop;
        sqt->populationCurrent = true;
        return pop;

    }

    bool nonempty(VersaTile* sqt) {

        // return true;
        // std::cout << "Call to nonempty()" << std::endl;

        // if (sqt->populationCurrent) {
        //     return (sqt->population > 0);
        // }

        urow_t mask = 0;
        for (int i = 0; i < ROWS; i++) {
            mask |= sqt->d[i];
        }

        /*
        if (mask & MIDDLE28) {
            return true;
        } else {
            sqt->population = 0;
            sqt->populationCurrent = true;
            return false;
        }
        */

        return (mask > 0);

    }

    int totalPopulation() {

        int population = 0;

        std::map<std::pair<int, int>, VersaTile>::iterator it;
        for (it = tiles.begin(); it != tiles.end(); it++)
        {
            VersaTile* sqt = &(it->second);
            population += countPopulation(sqt);
        }

        return population;
    }

    uint64_t totalHash(int radius) {

        uint64_t globalhash = 0;

        std::map<std::pair<int, int>, VersaTile>::iterator it;
        for (it = tiles.begin(); it != tiles.end(); it++)
        {
            VersaTile* sqt = &(it->second);
            uint64_t abscissa = sqt->tx;
            uint64_t ordinate = sqt->tw;
            if (sqt->tx * sqt->tx + sqt->tw * sqt->tw + sqt->tx * sqt->tw < radius * radius) {
                globalhash += hashTile(sqt) * (abscissa * 3141592653589793ull + ordinate * 2384626433832795ull);
            }
        }

        return globalhash;
    }

    void setcell(VersaTile* sqt, int x, int y, int state, bool overclock) {

        urow_t mask = (1ull << ((BITTAGE - 1) - x));

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

        if (overclock)
            return;

        if (sqt->updateflags == 0)
            modified.push_back(sqt);

        sqt->updateflags |= 64;

        if (y <= 3) {
            updateNeighbour(sqt, 0);
            updateNeighbour(sqt, 1);
        }

        if (y >= TVSPACE) {
            updateNeighbour(sqt, 3);
            updateNeighbour(sqt, 4);
        }

        if (x <= 3) {
            updateNeighbour(sqt, 5);
        }

        if (x >= (BITTAGE - 4)) {
            updateNeighbour(sqt, 2);
        }

    }

    void setcell(int x, int y, int state) {

        int oy = y % TVSPACE;
        if (oy < 0)
            oy += TVSPACE;
        int tw = (y - oy) / TVSPACE;

        int nx = x - tw * (THSPACE / 2);

        int ox = nx % THSPACE;
        if (ox < 0)
            ox += THSPACE;
        int tx = (nx - ox) / THSPACE;

        VersaTile* sqt = &tiles[std::make_pair(tx, tw)];
        sqt->tx = tx;
        sqt->tw = tw;

        setcell(sqt, ox + 2, oy + 2, state, false);
    }

    int getcell(VersaTile* sqt, int x, int y) {

        urow_t mask = (1ull << ((BITTAGE - 1) - x));

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

    std::vector<int> getComponent(VersaTile* sqt, int x, int y, int maxnorm) {

        int i = 0;
        std::vector<VersaTile*> tileList;
        std::vector<int> intList;
        int ll = 1;

        tileList.push_back(sqt);
        intList.push_back(x);
        intList.push_back(y);
        intList.push_back(getcell(sqt, x, y));
        setcell(sqt, x, y, 0, true);

        int population = 1;

        while (i < ll) {

            VersaTile* sqt1 = tileList[i];
            int ox = intList[3*i];
            int oy = intList[3*i + 1];

            // std::cout << i << ": " << ox << "," << oy << std::endl;

            for (int rx = ox - 2; rx <= ox + 2; rx++) {
                for (int ry = oy - 2; ry <= oy + 2; ry++) {

                    int px = rx;
                    int py = ry;

                    int norm = (px - ox)*(px - ox) + (py - oy)*(py - oy);

                    if (norm <= maxnorm) {
                        VersaTile* sqt2 = sqt1;

                        if (py <= 1) {
                            py += TVSPACE;
                            if (px < BITTAGE / 2) {
                                px += (THSPACE / 2);
                                sqt2 = getNeighbour(sqt2, 0);
                            } else {
                                px -= (THSPACE / 2);
                                sqt2 = getNeighbour(sqt2, 1);
                            }
                        } else if (py >= TVSPACE + 2) {
                            py -= TVSPACE;
                            if (px < BITTAGE / 2) {
                                px += (THSPACE / 2);
                                sqt2 = getNeighbour(sqt2, 4);
                            } else {
                                px -= (THSPACE / 2);
                                sqt2 = getNeighbour(sqt2, 3);
                            }
                        }

                        if (px <= 1) {
                            px += THSPACE;
                            sqt2 = getNeighbour(sqt2, 5);
                        } else if (px >= THSPACE + 2) {
                            px -= THSPACE;
                            sqt2 = getNeighbour(sqt2, 2);
                        }

                        int v = getcell(sqt2, px, py);

                        if (v > 0) {
                            setcell(sqt2, px, py, 0, true);

                            tileList.push_back(sqt2);
                            intList.push_back(px);
                            intList.push_back(py);
                            intList.push_back(v);
                            ll += 1;

                            if (v == 1) {
                                population += 1;
                            }
                        }
                    }
                }
            }

            i += 1;
        }

        for (int j = 0; j < ll; j++) {
            VersaTile* sqt3 = tileList[j];
            intList[j*3] += THSPACE * sqt3->tx + (THSPACE / 2) * sqt3->tw - 2;
            intList[j*3 + 1] += TVSPACE * sqt3->tw - 2;
        }

        intList.push_back(population);

        return intList;

    }

    std::vector<int> getComponent(VersaTile* sqt, int x, int y) {
        return getComponent(sqt, x, y, 5);
    }

    std::vector<int> getIsland(VersaTile* sqt, int x, int y) {
        std::vector<int> island = getComponent(sqt, x, y, 2);
        return island;
    }


};


void copycells(vlife* curralgo, vlife* destalgo, bool eraseHistory) {

    std::map<std::pair<int, int>, VersaTile>::iterator it;
    for (it = curralgo->tiles.begin(); it != curralgo->tiles.end(); it++)
    {
        VersaTile* sqt = &(it->second);

        if ((!eraseHistory) || curralgo->nonempty(sqt)) {
            // Only copy non-empty tiles:

            int tx = sqt->tx;
            int tw = sqt->tw;

            VersaTile* sqt2 = &(destalgo->tiles[std::make_pair(tx, tw)]);
            sqt2->tx = tx;
            sqt2->tw = tw;

            std::memcpy(sqt2->d, sqt->d, ROWS * sizeof(urow_t));
            if (eraseHistory) {
                std::memset(sqt2->hist, 0, ROWS * sizeof(urow_t));
            } else {
                std::memcpy(sqt2->hist, sqt->hist, ROWS * sizeof(urow_t));
            }

            if (sqt2->updateflags == 0)
                (destalgo->modified).push_back(sqt2);

            sqt2->updateflags |= 64;
            for (int i = 0; i < 6; i++) {
                destalgo->updateNeighbour(sqt2, i);
            }

        }

    }



}


std::vector<int> getcells(vlife* curralgo)
{

    std::vector<int> celllist;
    std::map<std::pair<int, int>, VersaTile>::iterator it;
    for (it = curralgo->tiles.begin(); it != curralgo->tiles.end(); it++)
    {
        VersaTile* sqt = &(it->second);
        int relx = THSPACE * sqt->tx + (THSPACE / 2) * sqt->tw - 2;
        int rely = TVSPACE * sqt->tw - 2;

        for (int j = 2; j < TVSPACE + 2; j++) {
            urow_t row = sqt->d[j] | sqt->hist[j];
            if (row) {
                for (int i = 2; i < THSPACE + 2; i++) {
                    int v = curralgo->getcell(sqt, i, j);
                    if (v > 0) {
                        celllist.push_back(relx + i);
                        celllist.push_back(rely + j);
                        celllist.push_back(v);
                    }
                }
            }
        }
    }

    return celllist;

}


int testmain() {

    std::cout << "VersaTile size: " << sizeof(VersaTile) << std::endl;

    clock_t start = clock();

    // Do this twenty times so that we can accurately measure the time.
    for (int i = 0; i < 50; i++) {
        vlife universe;
        universe.tilesProcessed = 0;
        VersaTile* sqt = &(universe.tiles[std::make_pair(0, 0)]);

        sqt->d[10] = 128 << 8;
        sqt->d[11] = 320 << 8;
        sqt->d[12] = 128 << 8;
        sqt->d[20] = 1 << 8;
        sqt->d[21] = 5 << 8;
        sqt->d[22] = 13 << 8;
        sqt->d[24] = 28 << 8;

        sqt->updateflags = 64;
        universe.modified.push_back(sqt);

        for (int k = 0; k < 15000; k++) {
            universe.run2gens(false);
        }

        std::cout << "Population count: " << universe.totalPopulation() << std::endl;
        std::cout << "Tiles processed: " << universe.tilesProcessed << std::endl;
    }

    clock_t end = clock();

    std::cout << "Lidka + 30k in " << ((double) (end-start) / CLOCKS_PER_SEC * 20.0) << " ms." << std::endl;

    return 0;

}

#endif

