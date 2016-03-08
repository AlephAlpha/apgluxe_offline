
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#ifdef USE_OPEN_MP
#include <omp.h>
#endif

#include "gollybase/qlifealgo.h"
#include "gollybase/lifealgo.h"
#include "gollybase/util.h"

#include "includes/sha256.h"
#include "includes/md5.h"
#include "includes/payosha256.h"
#include "includes/vlife.h"
#include "includes/incubator.h"

#define APG_VERSION "v3.02"

/*
 * Produce a new seed based on the original seed, current time and PID:
 */
std::string reseed(std::string seed) {

    std::ostringstream ss;
    ss << seed << " " << clock() << " " << time(NULL) << " " << getpid();

    std::string prehash = ss.str();

    unsigned char digest[SHA256::DIGEST_SIZE];
    memset(digest,0,SHA256::DIGEST_SIZE);

    SHA256 ctx = SHA256();
    ctx.init();
    ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
    ctx.final(digest);

    const char alphabet[] = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";

    std::ostringstream newseed;
    newseed << "m_";
    for (int i = 0; i < 12; i++) {
        newseed << alphabet[digest[i] % 56];
    }

    return newseed.str();

}


lifealgo *createUniverse(const char *algoName) {

   staticAlgoInfo *ai = staticAlgoInfo::byName(algoName) ;
   if (ai == 0)
      lifefatal("Error: No such algorithm");
   lifealgo *imp = (ai->creator)() ;
   if (imp == 0)
      lifefatal("Could not create universe");
   imp->setMaxMemory(256);

   return imp ;
}

void hashsoup(VersaTile* sqt, std::string prehash) {

    // This takes about 16 microseconds, compared with 350 for the Python counterpart!

    unsigned char digest[SHA256::DIGEST_SIZE];
    memset(digest,0,SHA256::DIGEST_SIZE);

    SHA256 ctx = SHA256();
    ctx.init();
    ctx.update( (unsigned char*)prehash.c_str(), prehash.length());
    ctx.final(digest);

    // Dump the soup into the array:
    for (int j = 0; j < 16; j++) {
        #if (BITTAGE == 32)
        sqt->d[j+(ROWS / 2 - 8)] = (digest[2*j] << 16) + (digest[2*j+1] << 8);
        #else
        sqt->d[j+(ROWS / 2 - 8)] = (((uint64_t) digest[2*j]) << 32) + (((uint64_t) digest[2*j+1]) << 24);
        #endif
    }
}

void runPattern(lifealgo* curralgo, int duration) {

    curralgo->setIncrement(duration);
    curralgo->step();

}

void runPattern(vlife* curralgo, int duration) {

    for (int i = 0; i < duration; i += 2) {
        curralgo->run2gens(false);
    }

}

bool getBoundingBox(lifealgo* curralgo, int bounds[]) {

    if (curralgo->isEmpty()) {
        // Universe is empty:
        return false;
    } else {
        // Universe is non-empty:
        bigint top, left, bottom, right;
        curralgo->findedges(&top, &left, &bottom, &right);
        int x = left.toint();
        int y = top.toint();
        int wd = right.toint() - x + 1;
        int ht = bottom.toint() - y + 1;
        bounds[0] = x;
        bounds[1] = y;
        bounds[2] = wd;
        bounds[3] = ht;

        return true;
    }

}

std::string compare_representations(std::string a, std::string b)
{
    if (a.compare("#") == 0) {
        return b;
    } else if (b.compare("#") == 0) {
        return a;
    } else if (a.length() < b.length()) {
        return a;
    } else if (b.length() < a.length()) {
        return b;
    } else if (a.compare(b) < 0) {
        return a;
    } else {
        return b;
    }
}

std::string canonise_orientation(lifealgo* curralgo, int length, int breadth, int ox, int oy, int a, int b, int c, int d)
{

    std::string representation;

    char charnames[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    for (int v = 0; v < ((breadth-1)/5)+1; v++) {
        int zeroes = 0;
        if (v != 0)
            representation += 'z';

        for (int u = 0; u < length; u++) {
            int baudot = 0;
            for (int w = 0; w < 5; w++) {
                int x = ox + a*u + b*(5*v + w);
                int y = oy + c*u + d*(5*v + w);
                baudot = (baudot >> 1) + 16*curralgo->getcell(x, y);
            }
            if (baudot == 0) {
                zeroes += 1;
            } else {
                if (zeroes > 0) {
                    if (zeroes == 1) {
                        representation += '0';
                    } else if (zeroes == 2) {
                        representation += 'w';
                    } else if (zeroes == 3) {
                        representation += 'x';
                    } else {
                        representation += 'y';
                        representation += charnames[zeroes - 4];
                    }
                }
                zeroes = 0;
                representation += charnames[baudot];
            }
        }
    }

    return representation;

}

std::string linearlyse(lifealgo* curralgo, int maxperiod)
{
    int poplist[3 * maxperiod];
    int difflist[2 * maxperiod];

    // runPattern(curralgo, 100);

    for (int i = 0; i < 3 * maxperiod; i++) {
        runPattern(curralgo, 1);
        poplist[i] = curralgo->getPopulation().toint();
    }

    int period = -1;

    for (int p = 1; p < maxperiod; p++) {
        for (int i = 0; i < 2 * maxperiod; i++) {
            difflist[i] = poplist[i + p] - poplist[i];
        }

        bool correct = true;

        for (int i = 0; i < maxperiod; i++) {
            if (difflist[i] != difflist[i + p]) {
                correct = false;
            }
        }

        if (correct == true) {
            period = p;
            break;
        }
    }

    if (period == -1)
        return "PATHOLOGICAL";

    int qeriod = -1;

    for (int q = 1; q < maxperiod; q++) {
        bool correct = true;

        for (int i = 0; i < maxperiod; i++) {
            if (difflist[i] != difflist[i + q]) {
                correct = false;
            }
        }

        if (correct == true) {
            qeriod = q;
            break;
        }
    }

    if (qeriod == -1)
    {
        // std::cout << "Something is seriously wrong!" << std::endl;
        return "PATHOLOGICAL";
    }

    int moment0 = 0;
    int moment1 = 0;
    int moment2 = 0;

    for (int i = 0; i < period; i++) {

        int instadiff = (poplist[i + qeriod] - poplist[i]);

        moment0 += instadiff;
        moment1 += (instadiff * instadiff);
        moment2 += (instadiff * instadiff * instadiff);

    }

    if (moment0 == 0)
        return "PATHOLOGICAL";

    std::ostringstream ss_prehash;
    ss_prehash << moment1 << "#" << moment2;
    std::string posthash = md5(ss_prehash.str());
    std::ostringstream ss_repr;
    ss_repr << "yl" << period << "_" << qeriod << "_" << moment0 << "_" << posthash;

    std::string repr = ss_repr.str();

    // std::cout << "Linear-growth pattern identified: \033[1;32m" << repr << "\033[0m" << std::endl;

    return repr;

}

std::string canonise(lifealgo* curralgo, int duration)
{
    std::string representation = "#";

    for (int t = 0; t < duration; t++) {

        int rect[4];

        if (t != 0) {
            runPattern(curralgo, 1);
        }

        if (getBoundingBox(curralgo, rect)) {

            if (rect[2] <= 40 && rect[3] <= 40) {
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[2], rect[3], rect[0], rect[1], 1, 0, 0, 1));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[2], rect[3], rect[0]+rect[2]-1, rect[1], -1, 0, 0, 1));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[2], rect[3], rect[0], rect[1]+rect[3]-1, 1, 0, 0, -1));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[2], rect[3], rect[0]+rect[2]-1, rect[1]+rect[3]-1, -1, 0, 0, -1));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[3], rect[2], rect[0], rect[1], 0, 1, 1, 0));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[3], rect[2], rect[0]+rect[2]-1, rect[1], 0, -1, 1, 0));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[3], rect[2], rect[0], rect[1]+rect[3]-1, 0, 1, -1, 0));
                representation = compare_representations(representation, canonise_orientation(curralgo, rect[3], rect[2], rect[0]+rect[2]-1, rect[1]+rect[3]-1, 0, -1, -1, 0));
            }

        } else {
            return "xs0_0";
        }

    }

    return representation;
}

// /*

void copycells(vlife* curralgo, incubator* destalgo) {

    std::map<std::pair<int, int>, VersaTile>::iterator it;
    for (it = curralgo->tiles.begin(); it != curralgo->tiles.end(); it++)
    {
        VersaTile* sqt = &(it->second);
        for (int half = 0; half < 2; half++) {
            int mx = 2 * sqt->tx + sqt->tw + half;
            int my = sqt->tw;

            int lx = mx % 4;
            int ly = my % 4;

            if (lx < 0) {lx += 4;}
            if (ly < 0) {ly += 4;}

            int tx = (mx - lx) / 4;
            int ty = (my - ly) / 4;

            Incube* sqt2 = &(destalgo->tiles[std::make_pair(tx, ty)]);
            sqt2->tx = tx;
            sqt2->ty = ty;

            for (int i = 0; i < TVSPACE; i++) {

                uint64_t insert;
                if (half == 1) {
                    insert = (sqt->d[i+2] >> 2) & 0x3fff;
                } else {
                    insert = (sqt->d[i+2] >> 16) & 0x3fff;
                }
                sqt2->d[i + TVSPACE * ly] |= (insert << (14 * (3 - lx)));
                if (half == 1) {
                    insert = (sqt->hist[i+2] >> 2) & 0x3fff;
                } else {
                    insert = (sqt->hist[i+2] >> 16) & 0x3fff;
                }
                sqt2->hist[i + TVSPACE * ly] |= (insert << (14 * (3 - lx)));

            }

        }

    }

}

// */

void copycells(lifealgo* curralgo, vlife* destalgo, int left, int top, int wd, int ht)
{

    int right = left + wd - 1;
    int bottom = top + ht - 1;
    int cx, cy;
    int v = 0;

    for ( cy=top; cy<=bottom; cy++ ) {
        for ( cx=left; cx<=right; cx++ ) {
            int skip = curralgo->nextcell(cx, cy, v);
            if (skip >= 0) {
                // found next live cell in this row
                cx += skip;
                if (cx <= right) {
                    // std::cout << cx << "," << cy << "," << v << std::endl;
                    destalgo->setcell(cx, cy, v);
                }
            } else {
                cx = right;  // done this row
            }
        }
    }
}


std::vector<int> getcells(lifealgo* curralgo, int left, int top, int wd, int ht)
{

    int right = left + wd - 1;
    int bottom = top + ht - 1;
    int cx, cy;
    int v = 0;

    bool multistate = curralgo->NumCellStates() > 2;

    std::vector<int> celllist;

    for ( cy=top; cy<=bottom; cy++ ) {
        for ( cx=left; cx<=right; cx++ ) {
            int skip = curralgo->nextcell(cx, cy, v);
            if (skip >= 0) {
                // found next live cell in this row
                cx += skip;
                if (cx <= right) {
                    celllist.push_back(cx);
                    celllist.push_back(cy);
                    if (multistate) {
                        celllist.push_back(v);
                    }
                }
            } else {
                cx = right;  // done this row
            }
        }
    }

    // TODO: Possibly append an extra zero if multistate (but I've found this
    // abuse of the parity of the length of the cell list as a multistate flag
    // to be rather unhelpful from a scripting perspective, where I already
    // know whether to expect a single- or multistate cell list).

    return celllist;
}

/*
 * The equivalent of the 'hash()' function used in Golly scripts:
 */
unsigned long long hash_rectangle(lifealgo* curralgo, int x, int y, int wd, int ht)
{
    // calculate a hash value for pattern in given rect
    unsigned long long hash = 31415962;
    int right = x + wd - 1;
    int bottom = y + ht - 1;
    int cx, cy;
    int v = 0;
    bool multistate = curralgo->NumCellStates() > 2;

    for ( cy=y; cy<=bottom; cy++ ) {
        int yshift = cy - y;
        for ( cx=x; cx<=right; cx++ ) {
            int skip = curralgo->nextcell(cx, cy, v);
            if (skip >= 0) {
                // found next live cell in this row (v is >= 1 if multistate)
                cx += skip;
                if (cx <= right) {
                    // need to use a good hash function for patterns like AlienCounter.rle
                    hash = (hash * 1000003ull) ^ yshift;
                    hash = (hash * 1000003ull) ^ (cx - x);
                    if (multistate) hash = (hash * 1000003ull) ^ v;
                }
            } else {
                cx = right;  // done this row
            }
        }
    }

    return hash;
}

/*
 * Stabilisation detection by checking for population periodicity:
 */
int naivestab_awesome(vlife* curralgo) {

    // Copied almost verbatim from the apgsearch Python script...
    int depth = 0;
    int prevpop = 0;
    int currpop = 0;
    int period = 8;
    int security = 15;
    for (int i = 0; i < 1000; i++) {

        if (i == 25)
            security = 20;

        if (i == 50)
            security = 25;

        if (i == 75)
            security = 30;

        if (i == 200)
            period = 12;

        if (i == 400)
            period = 30;

        runPattern(curralgo, period);
        // currpop = curralgo->getPopulation().toint();
        if (curralgo->modified.size() == 0) {
            return 6;
        }
        currpop = curralgo->totalPopulation();
        if (currpop == prevpop) {
            depth += 1;
        } else {
            depth = 0;
            if (period < 30) {
                i += 1;
                runPattern(curralgo, 12);
            }
        }
        prevpop = currpop;
        if (depth == security) {
            // Population is periodic.
            return period;
        }
    }

    return 0;

}

/*
 * Run the universe until it stabilises:
 */
int stabilise3(vlife* curralgo) {

    int pp = naivestab_awesome(curralgo);

    if (pp > 0) {
        return pp;
    }

    // Phase II of stabilisation detection, which is much more rigorous and based on oscar.py.

    std::vector<long long> hashlist;
    std::vector<int> genlist;

    int generation = 0;

    for (int j = 0; j < 4000; j++) {

        runPattern(curralgo, 30);
        generation += 30;

        long long h = curralgo->totalHash(120);

        // determine where to insert h into hashlist
        int pos = 0;
        int listlen = hashlist.size();

        while (pos < listlen) {
            if (h > hashlist[pos]) {
                pos += 1;
            } else if (h < hashlist[pos]) {
                // shorten lists and append info below
                for (int i = pos; i < listlen; i++) {
                    hashlist.pop_back();
                    genlist.pop_back();
                }
                break;
            } else {
                int period = (generation - genlist[pos]);

                int prevpop = curralgo->totalPopulation();

                for (int i = 0; i < 20; i++) {
                    runPattern(curralgo, period);
                    int currpop = curralgo->totalPopulation();
                    if (currpop != prevpop) {
                        if (period < 1280)
                            period = 1280;
                        break;
                    }
                    prevpop = currpop;
                }

                return period;
            }
        }

        hashlist.push_back(h);
        genlist.push_back(generation);
    }

    std::cout << "Failed to detect periodic behaviour!" << std::endl;

    runPattern(curralgo, 65536);

    return 1280;


}

/*
 * Get the representation of a single object:
 */
std::string getRepresentation(lifealgo* curralgo, int maxperiod, int bounds[]) {

    int bbOrig[4];
    std::string repr = "aperiodic";

    int period = -1;
    int xdisplacement = 0;
    int ydisplacement = 0;

    if (getBoundingBox(curralgo, bbOrig)) {

        unsigned long long hash1 = hash_rectangle(curralgo, bbOrig[0], bbOrig[1], bbOrig[2], bbOrig[3]);

        for (int i = 1; i <= maxperiod; i++) {

            runPattern(curralgo, 1);

            int boundingBox[4];

            if (getBoundingBox(curralgo, boundingBox)) {

                unsigned long long hash2 = hash_rectangle(curralgo, boundingBox[0], boundingBox[1], boundingBox[2], boundingBox[3]);

                if (hash1 == hash2 && boundingBox[2] == bbOrig[2] && boundingBox[3] == bbOrig[3]) {
                    period = i;
                    xdisplacement = boundingBox[0] - bbOrig[0];
                    ydisplacement = boundingBox[1] - bbOrig[1];
                    break;
                }

            }
        }
    }

    if (period <= 0) {
        // std::cout << "Object is aperiodic (population = " << initpop << ")." << std::endl;
        repr = linearlyse(curralgo, 4100);
    } else {
        // std::cout << "Object has period " << period << "." << std::endl;
        std::ostringstream ss;
        std::string barecode = canonise(curralgo, period);
        // std::cout << "Final population: " << curralgo->getPopulation().toint() << endl;

        if (barecode.compare("#") == 0) {
            ss << "ov_";
        } else {
            ss << "x";
        }

        if (period == 1) {
            ss << "s" << curralgo->getPopulation().toint();
        } else {
            if (xdisplacement == 0 && ydisplacement == 0) {
                ss << "p" << period;
            } else {
                ss << "q" << period;
            }
        }

        if (barecode.compare("#") != 0) {
            ss << "_" << barecode;
        }

        repr = ss.str();
    }

    bounds[0] = period;
    bounds[1] = xdisplacement;
    bounds[2] = ydisplacement;

    return repr;

}

/*
 * Count the number of vertices of each degree:
 */
void degcount(lifealgo* curralgo, int degrees[], int generations) {

    for (int i = 0; i < generations; i++) {

        for (int j = 0; j < 9; j++) {
            degrees[9*i + j] = 0;
        }

        runPattern(curralgo, 1);
        int bb[4];

        if (getBoundingBox(curralgo, bb)) {
            std::vector<int> celllist = getcells(curralgo, bb[0], bb[1], bb[2], bb[3]);

            for (unsigned int k = 0; k < celllist.size(); k += 2) {
                int x = celllist[k];
                int y = celllist[k+1];

                int degree = -1;

                for (int ix = x - 1; ix <= x + 1; ix++) {
                    for (int iy = y - 1; iy <= y + 1; iy++) {
                        degree += curralgo->getcell(ix, iy);
                    }
                }

                degrees[9*i + degree] += 1;
            }
        }
    }
}

/*
 * Separate a collection of period-4 standard spaceships:
 */
std::vector<std::string> sss(lifealgo* curralgo) {

    std::vector<std::string> components;

    int degrees[36];
    degcount(curralgo, degrees, 4);

    for (int i = 0; i < 18; i++) {
        if (degrees[i] != degrees[18+i]) {
            // std::cout << "Non-alternating xq4 spaceship!" << std::endl;
            return components;
        }
    }

    int hwssa[18] = {1,4,6,2,0,0,0,0,0,0,0,0,4,4,6,1,2,1};
    int mwssa[18] = {2,2,5,2,0,0,0,0,0,0,0,0,4,4,4,1,2,0};
    int lwssa[18] = {1,2,4,2,0,0,0,0,0,0,0,0,4,4,2,2,0,0};
    int hwssb[18] = {0,0,0,4,4,6,1,2,1,1,4,6,2,0,0,0,0,0};
    int mwssb[18] = {0,0,0,4,4,4,1,2,0,2,2,5,2,0,0,0,0,0};
    int lwssb[18] = {0,0,0,4,4,2,2,0,0,1,2,4,2,0,0,0,0,0};
    int glida[18] = {0,1,2,1,1,0,0,0,0,0,2,1,2,0,0,0,0,0};
    int glidb[18] = {0,2,1,2,0,0,0,0,0,0,1,2,1,1,0,0,0,0};

    int hacount = degrees[17];
    int macount = degrees[16]/2 - hacount;
    int lacount = (degrees[15] - hacount - macount)/2;
    int hbcount = degrees[8];
    int mbcount = degrees[7]/2 - hbcount;
    int lbcount = (degrees[6] - hbcount - mbcount)/2;

    int gacount = 0;
    int gbcount = 0;

    if ((lacount == 0) && (lbcount == 0) && (macount == 0) && (mbcount == 0) && (hacount == 0) && (hbcount == 0)) {
        gacount = degrees[4];
        gbcount = degrees[13];
    }

    for (int i = 0; i < 18; i++) {
        int putativedegrees = 0;
        putativedegrees += hacount * hwssa[i];
        putativedegrees += hbcount * hwssb[i];
        putativedegrees += lacount * lwssa[i];
        putativedegrees += lbcount * lwssb[i];
        putativedegrees += macount * mwssa[i];
        putativedegrees += mbcount * mwssb[i];
        putativedegrees += gacount * glida[i];
        putativedegrees += gbcount * glidb[i];
        if (degrees[i] != putativedegrees) {
            // std::cout << "Non-standard xq4 spaceship!" << std::endl;
            return components;
        }
    }

    int hcount = 0;
    int lcount = 0;
    int mcount = 0;
    int gcount = 0;

    if (hacount >= 0 && hbcount >= 0) {
        hcount = hacount + hbcount;
    } else {
        std::cout << "Negative quantity of spaceships!" << std::endl;
        return components;
    }

    if (lacount >= 0 && lbcount >= 0) {
        lcount = lacount + lbcount;
    } else {
        std::cout << "Negative quantity of spaceships!" << std::endl;
        return components;
    }

    if (macount >= 0 && mbcount >= 0) {
        mcount = macount + mbcount;
    } else {
        std::cout << "Negative quantity of spaceships!" << std::endl;
        return components;
    }

    if (gacount >= 0 && gbcount >= 0) {
        gcount = gacount + gbcount;
    } else {
        std::cout << "Negative quantity of spaceships!" << std::endl;
        return components;
    }

    for (int i = 0; i < gcount; i++)
        components.push_back("xq4_153");
    for (int i = 0; i < lcount; i++)
        components.push_back("xq4_6frc");
    for (int i = 0; i < mcount; i++)
        components.push_back("xq4_27dee6");
    for (int i = 0; i < hcount; i++)
        components.push_back("xq4_27deee6");

    return components;

}

/*
 * Separate a pseudo-object into its constituent parts:
 */
std::vector<std::string> pseudoBangBang(lifealgo* curralgo, int period, bool moving) {

    vlife universe;
    vlife universe2;
    int boundingBox[4];
    int initpop = curralgo->getPopulation().toint();

    std::vector<std::string> components;

    if (getBoundingBox(curralgo, boundingBox)) {
        copycells(curralgo, &universe, boundingBox[0], boundingBox[1], boundingBox[2], boundingBox[3]);

        std::vector<int> celllist2 = getcells(&universe);

        for (int i = 0; i < period + 2; i += 2) {
            universe.run2gens(true);
        }

        std::vector<int> celllist3 = getcells(&universe);

        int label = 0;

        std::map<std::pair<int, int>, int> geography;
        std::map<std::pair<int, int>, VersaTile>::iterator it;
        for (it = universe.tiles.begin(); it != universe.tiles.end(); it++)
        {
            VersaTile* sqt = &(it->second);
            for (int y = 2; y <= TVSPACE + 1; y++) {
                if (sqt->d[y]) {
                    for (int x = 2; x <= THSPACE + 1; x++) {
                        if (universe.getcell(sqt,x,y) == 1) {

                            vector<int> intList;

                            if (moving) {
                                intList = universe.getComponent(sqt,x,y);
                            } else {
                                intList = universe.getIsland(sqt,x,y);
                            }
                            // int population = intList.back();
                            int ll = intList.size() - 1;
                            label += 1;

                            for (int j = 0; j < ll; j += 3) {
                                int xx = intList[j];
                                int yy = intList[j + 1];
                                geography[std::make_pair(xx, yy)] = label;
                            }
                        }
                    }
                }
            }
        }

        if (moving == false) {
            
            bool reiterate = true;

            int bb[4];
            
            while (reiterate) {
                
                reiterate = false;
            
                for (int i = 0; i < period; i++) {
                    runPattern(curralgo, 1);
                    if (getBoundingBox(curralgo, bb)) {
                        // std::cout << "PBB Generation " << i << "/" << period << ": pop = " << curralgo->getPopulation().toint() << std::endl;
                        std::vector<int> celllist = getcells(curralgo, bb[0], bb[1], bb[2], bb[3]);

                        for (unsigned int j = 0; j < celllist.size(); j += 2) {
                            int ox = celllist[j];
                            int oy = celllist[j+1];

                            for (int ix = ox - 1; ix <= ox + 1; ix++) {
                                for (int iy = oy - 1; iy <= oy + 1; iy++) {
                                    if (geography[std::make_pair(ix, iy)] == 0) {
                                        std::map<int, int> tally;
                                        for (int ux = ix - 1; ux <= ix + 1; ux++) {
                                            for (int uy = iy - 1; uy <= iy + 1; uy++) {
                                                int value = geography[std::make_pair(ux, uy)];
                                                if (curralgo->getcell(ux, uy)) {
                                                    tally[value] = tally[value] + 1;
                                                }
                                            }
                                        }

                                        int dominantColour = 0;

                                        std::map<int, int>::iterator it2;
                                        for (it2 = tally.begin(); it2 != tally.end(); it2++)
                                        {
                                            int colour = it2->first;
                                            int count = it2->second;

                                            if ((1 << count) & BIRTHS) {
                                                dominantColour = colour;
                                            }
                                        }

                                        // Resolve dependencies:
                                        if (dominantColour != 0) {
                                            std::map<std::pair<int, int>, int>::iterator it3;
                                            for (it3 = geography.begin(); it3 != geography.end(); it3++)
                                            {
                                                std::pair<int, int> coords = it3->first;
                                                int colour = it3->second;

                                                if (tally[colour] > 0) {
                                                    geography[coords] = dominantColour;
                                                    if (colour != dominantColour) {
                                                        // A change has occurred; keep iterating until we achieve stability:
                                                        reiterate = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Eliminate cells that are no longer alive:
        std::map<std::pair<int, int>, int>::iterator it3;
        for (it3 = geography.begin(); it3 != geography.end(); it3++)
        {
            std::pair<int, int> coords = it3->first;
            if (curralgo->getcell(coords.first, coords.second) == 0) {
                geography[coords] = 0;
                // std::cout << "Dead: " << coords.first << ", " << coords.second << std::endl;
            } else {
                // std::cout << "Alive (state = " << geography[coords] << "): " << coords.first << ", " << coords.second << std::endl;
            }
        }

        // Draw the object:
        for (int l = 1; l <= label; l++) {
            int lpop = 0;
            curralgo->clearall();
            std::map<std::pair<int, int>, int>::iterator it3;
            for (it3 = geography.begin(); it3 != geography.end(); it3++)
            {
                std::pair<int, int> coords = it3->first;
                if (it3->second == l) {
                    curralgo->setcell(coords.first, coords.second, 1);
                    lpop += 1;
                }

            }
            curralgo->endofpattern();

            // std::cout << "Label " << l << ": pop = " << lpop << std::endl;

            if (curralgo->getPopulation().toint() > 0) {
                int returns[3];
                std::string repr = getRepresentation(curralgo, period, returns);
                if (repr.compare("PATHOLOGICAL") == 0) {
                    std::cout << "ERROR: Pathological ";
                    std::cout << (moving ? "moving" : "oscillatory");
                    std::cout << " object produced by pseudoBangBang." << std::endl;
                    std::cout << "Initial population: " << initpop << std::endl;
                    std::cout << "Supposed period: " << period << std::endl;


                    for (uint32_t i = 0; i < celllist2.size(); i++) {
                        std::cout << celllist2[i] << ", ";
                    }
                    std::cout << std::endl;
                    for (uint32_t i = 0; i < celllist3.size(); i++) {
                        std::cout << celllist3[i] << ", ";
                    }
                    std::cout << std::endl;

                }
                // std::cout << "-- " << repr << endl;
                components.push_back(repr);
            }
        }
    }

    return components;

}


/*
 * This contains everything necessary for performing a soup search.
 */
class SoupSearcher {

public:

    std::map<unsigned long long, std::string> bitcache;
    std::map<std::string, std::vector<std::string> > decompositions;
    std::map<std::string, long long> census;
    std::map<std::string, std::vector<std::string> > alloccur;

    void aggregate(std::map<std::string, long long> *newcensus, std::map<std::string, std::vector<std::string> > *newoccur) {

        
        std::map<std::string, long long>::iterator it;
        for (it = newcensus->begin(); it != newcensus->end(); it++)
        {
            std::string apgcode = it->first;
            long long quantity = it->second;
            census[apgcode] += quantity;

        }

        std::map<std::string, std::vector<std::string> >::iterator it2;
        for (it2 = newoccur->begin(); it2 != newoccur->end(); it2++)
        {
            std::string apgcode = it2->first;
            std::vector<std::string> occurrences = it2->second;
            for (unsigned int i = 0; i < occurrences.size(); i++) {
                if (alloccur[apgcode].size() < 10) {
                    alloccur[apgcode].push_back(occurrences[i]);
                }
            }
        }


    }

    /*
     * Identify an object or pseudo-object:
     */
    int classify(int* celllist, int population, std::vector<std::string>& objlist, lifealgo* curralgo) {
        int pathologicals = 0;
        int ll = population * 2;

        int left = celllist[0];
        int top = celllist[1];
        int right = celllist[0];
        int bottom = celllist[1];

        for (int i = 0; i < ll; i += 2) {
            if (left > celllist[i])
                left = celllist[i];
            if (right < celllist[i])
                right = celllist[i];
            if (top > celllist[i+1])
                top = celllist[i+1];
            if (bottom < celllist[i+1])
                bottom = celllist[i+1];
        }

        for (int i = 0; i < ll; i += 2) {
            celllist[i] -= left;
            celllist[i+1] -= top;
        }

        right -= left;
        bottom -= top;

        unsigned long long bitstring = 0;

        if (right <= 7 && bottom <= 7) {
            for (int i = 0; i < ll; i += 2) {
                bitstring |= (1ull << (celllist[i] + 8*celllist[i+1]));
            }

            // cout << population << ": " << bitstring << endl;
        } else {
            // std::cout << "Large object with population " << population << std::endl;
        }

        std::string repr;
        std::vector<std::string> elements;

        if ((bitstring > 0) && bitcache[bitstring].size() > 0) {
            // Object has been cached:
            repr = bitcache[bitstring];
            elements = decompositions[repr];
        } else {

            // Load pattern into QuickLife:
            curralgo->clearall();
            for (int i = 0; i < ll; i += 2) {
                curralgo->setcell(celllist[i], celllist[i+1], 1);
            }
            curralgo->endofpattern();

            int bounds[3];
            // repr = getRepresentation(curralgo, 4000, bounds);
            repr = getRepresentation(curralgo, 1280, bounds);

            if (repr.compare("PATHOLOGICAL") == 0) {
                // std::cout << "Pathological object at " << left << ", " << top << ", " << right << ", " << bottom << std::endl;
                pathologicals += 1;
            }

            if (decompositions[repr].size() > 0) {
                elements = decompositions[repr];
            } else if (bounds[0] >= 1) {
                // Periodic object:
                if (bounds[1] == 0 && bounds[2] == 0) {
                    // Still-life or oscillator:
                    elements = pseudoBangBang(curralgo, bounds[0], false);
                } else {
                    // Spaceship:
                    if (bounds[0] <= 8) {
                        #ifdef STANDARD_LIFE
                        if (bounds[0] == 4)
                            elements = sss(curralgo);
                        #endif

                        if (elements.size() == 0)
                            elements = pseudoBangBang(curralgo, bounds[0], true);
                    } else {
                        elements.push_back(repr);
                    }
                }
                if (repr[0] == 'x') {
                    // Non-oversized periodic pseudo-object:
                    decompositions[repr] = elements;
                }
            } else {
                elements.push_back(repr);
            }

            if ((bitstring > 0) && (repr[0] == 'x')) {
                // Cache the object:
                bitcache[bitstring] = repr;
            }
        }

        for (unsigned int i = 0; i < elements.size(); i++) {
            objlist.push_back(elements[i]);
        }

        return pathologicals;
    }

    /*
     * Find all of the objects in the universe:
     */
    bool separate(/* vlife* */ incubator* universe, bool proceedNonetheless, lifealgo* curralgo, std::string suffix) {

        int nGliders = 0;
        int nBlocks = 0;
        int nBlinkers = 0;
        int nBoats = 0;
        int nBeehives = 0;

        uint64_t cachearray[I_HEIGHT];
        uint64_t emptymatrix[I_HEIGHT];
        /*
        urow_t cachearray[ROWS];
        urow_t emptymatrix[ROWS];
        */

        memset(emptymatrix,0, I_HEIGHT * sizeof(uint64_t));
        // memset(emptymatrix, 0, ROWS * sizeof(urow_t));

        int pathologicals = 0;

        std::vector<std::string> objlist;
        std::vector<int> gcoordlist;
        std::vector<Incube*> gpointerlist;

        std::map<std::pair<int, int>, Incube>::iterator it;
        for (it = universe->tiles.begin(); it != universe->tiles.end(); it++)
        {
            memset(cachearray, 0, I_HEIGHT * sizeof(uint64_t));
            // memset(cachearray, 0, ROWS * sizeof(urow_t));

                    // std::cout << "blah2" << std::endl;
            Incube* sqt = &(it->second);
            // VersaTile* sqt = &(it->second);
            for (int y = 0; y < I_HEIGHT; y++) {
            // for (int y = 2; y < TVSPACE + 2; y++) {
                if (sqt->d[y]) {
                    // std::cout << "blah" << std::endl;

                    for (int x = 0; x <= 55; x++) {
                    // for (int x = 2; x < THSPACE + 2; x++) {
                        if (universe->getcell(sqt,x,y) == 1) {
                            int annoyance = universe->isAnnoyance(sqt, x, y);
                            if (annoyance == 0) {
                                int gliderstatus = universe->isGlider(sqt, x, y, false, cachearray);
                                // int gliderstatus = 0;
                                if (gliderstatus == 1) {
                                    gcoordlist.push_back(x);
                                    gcoordlist.push_back(y);
                                    gpointerlist.push_back(sqt);
                                } else if (gliderstatus == 0) {
                                    vector<int> intList = universe->getComponent(sqt,x,y);

                                    int population = intList.back();
                                    int ll = intList.size() - 1;

                                    if (population > 0) {
                                        #ifdef STANDARD_LIFE
                                        if (population == 5) {
                                            if (ll == 15) {
                                                nBoats += 1;
                                            } else {
                                                nGliders += 1;
                                                // cout << "B: " << x << "," << y << endl;
                                            }
                                        } else {
                                        #endif
                                            // cout << population << " ";
                                            int coords[population*2];
                                            int i = 0;
                                            for (int j = 0; j < ll; j += 3) {
                                                if (intList[j + 2] == 1) {
                                                    int x = intList[j];
                                                    int y = intList[j + 1];
                                                    coords[i++] = x;
                                                    coords[i++] = y;
                                                }
                                            }

                                            pathologicals += classify(coords, population, objlist, curralgo);
                                        #ifdef STANDARD_LIFE
                                        }
                                        #endif
                                    }
                                }

                            } else if (annoyance == 3) {
                                nBlinkers += 1;
                                // cout << "_";
                            } else if (annoyance == 4) {
                                nBlocks += 1;
                                // cout << ".";
                            } else if (annoyance == 6) {
                                nBeehives += 1;
                            }
                        }
                    }
                }
            }
        }

        int gl = gpointerlist.size();

        for (int i = 0; i < gl; i++) {
            int x = gcoordlist[2*i];
            int y = gcoordlist[2*i+1];
            // VersaTile* sqt = gpointerlist[i];
            Incube* sqt = gpointerlist[i];

            if (universe->getcell(sqt, x, y) == 1) {
                if (universe->isGlider(sqt, x, y, true, emptymatrix)) {
                    nGliders += 1;
                }
            }
        }

        bool ignorePathologicals = false;

        for (unsigned int i = 0; i < objlist.size(); i++) {
            std::string apgcode = objlist[i];
            if (apgcode[0] == 'y') { ignorePathologicals = true; }
        }

        if (pathologicals > 0) {
            if (proceedNonetheless) {
                if (ignorePathologicals == false) { std::cout << "Pathological object detected!!!" << std::endl; }
            } else {
                return true;
            }
        }

        census["xs4_33"] += nBlocks;
        census["xp2_7"] += nBlinkers;
        census["xs6_696"] += nBeehives;
        census["xq4_153"] += nGliders;
        census["xs5_253"] += nBoats;

        for (unsigned int i = 0; i < objlist.size(); i++) {
            std::string apgcode = objlist[i];
            if ((ignorePathologicals == false) || (apgcode.compare("PATHOLOGICAL") != 0)) {
                census[apgcode] += 1;
                if (alloccur[apgcode].size() == 0 || alloccur[apgcode].back().compare(suffix) != 0) {
                    if (alloccur[apgcode].size() < 10) {
                        alloccur[apgcode].push_back(suffix);
                    }
                }
            }

            // Mention object in terminal:
            #ifdef STANDARD_LIFE
            if ((apgcode[0] == 'x') && (apgcode[1] == 'p')) {
                if ((apgcode[2] != '2') || (apgcode[3] != '_')) {
                    if (apgcode.compare("xp3_co9nas0san9oczgoldlo0oldlogz1047210127401") != 0 && apgcode.compare("xp15_4r4z4r4") != 0) {
                        // Interesting oscillator:
                        std::cout << "Rare oscillator detected: \033[1;31m" << apgcode << "\033[0m" << std::endl;
                    }
                }
            } else if ((apgcode[0] == 'x') && (apgcode[1] == 'q')) {
                if (apgcode.compare("xq4_153") != 0 && apgcode.compare("xq4_6frc") != 0 && apgcode.compare("xq4_27dee6") != 0 && apgcode.compare("xq4_27deee6") != 0) {
                    std::cout << "Rare spaceship detected: \033[1;34m" << apgcode << "\033[0m" << std::endl;
                }
            } else if ((apgcode[0] == 'y') && (apgcode[1] == 'l')) {
                std::cout << "Linear growth-pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            }
            #else
            if ((apgcode[0] == 'y') && (apgcode[1] == 'l')) {
                std::cout << "Linear growth-pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            }
            #endif
        }

        return false;

    }

    void censusSoup(std::string seedroot, std::string suffix, lifealgo* curralgo) {
        vlife btq;
        btq.tilesProcessed = 0;

        vlife* imp = &btq;

        VersaTile* sqt = &(btq.tiles[std::make_pair(0, 0)]);

        hashsoup(sqt, seedroot + suffix);

        sqt->updateflags = 256;
        imp->modified.push_back(sqt);

        int duration = stabilise3(imp);

        bool failure = true;
        int attempt = 0;

        // Repeat until there are no pathological objects, or until five attempts have elapsed:
        while (failure) {

            failure = false;

            if (imp->totalPopulation()) {

                // Convert to LifeHistory:
                vlife universe;
                copycells(imp, &universe, true);

                for (int j = 0; j < duration/2; j++) {
                    universe.run2gens(true);
                }

                incubator icb;
                copycells(&universe, &icb);

                // failure = separate(&universe, (attempt >= 5), curralgo, suffix);
                failure = separate(&icb, (attempt >= 5), curralgo, suffix);

            }

            // Pathological object detected:
            if (failure) {
                attempt += 1;
                runPattern(imp, 10000);
                duration = 4000;
            }
        }
    }


    std::vector<std::pair<long long, std::string> > getSortedList(long long &totobjs) {

        std::vector<std::pair<long long, std::string> > censusList;

        std::map<std::string, long long>::iterator it;
        for (it = census.begin(); it != census.end(); it++)
        {
            if (it->second != 0) {
                censusList.push_back(std::make_pair(it->second, it->first));
                totobjs += it->second;
            }
        }
        std::sort(censusList.begin(), censusList.end());

        return censusList;

    }

    std::string submitResults(std::string payoshakey, std::string root, long long numsoups, int local_log) {

        std::string authstring = authenticate(payoshakey.c_str(), "post_apgsearch_haul");

        // Authentication failed:
        if (authstring.length() == 0)
            return "";

        long long totobjs = 0;

        std::vector<std::pair<long long, std::string> > censusList = getSortedList(totobjs);

        std::ostringstream ss;

        ss << authstring << "\n";
        ss << "@VERSION " << APG_VERSION << "\n";
        ss << "@MD5 " << md5(root) << "\n";
        ss << "@ROOT " << root << "\n";
        ss << "@RULE " << RULESTRING << "\n";
        ss << "@SYMMETRY C1\n";
        ss << "@NUM_SOUPS " << numsoups << "\n";
        ss << "@NUM_OBJECTS " << totobjs << "\n";

        ss << "\n@CENSUS TABLE\n";

        for (int i = censusList.size() - 1; i >= 0; i--) {
            ss << censusList[i].second << " " << censusList[i].first << "\n";
        }

        ss << "\n@SAMPLE_SOUPIDS\n";

        for (int i = censusList.size() - 1; i >= 0; i--) {
            std::vector<std::string> occurrences = alloccur[censusList[i].second];

            ss << censusList[i].second;

            for (unsigned int j = 0; j < occurrences.size(); j++) {
                ss << " " << occurrences[j];
            }

            ss << "\n";
        }

        if(local_log) {
            std::ofstream resultsFile;
            std::ostringstream resultsFileName;

            std::time_t timestamp = std::time(NULL);

            resultsFileName << "log." << timestamp << "." << root << ".txt";

            std::cout << "Saving results to " << resultsFileName.str() << std::endl;

            resultsFile.open(resultsFileName.str().c_str());
            resultsFile << ss.str();
            resultsFile.close();
        }

        return catagolueRequest(ss.str().c_str(), "/apgsearch");

    }

};


std::string obtainWork(std::string payoshakey) {
    
    std::string authstring = authenticate(payoshakey.c_str(), "verify_apgsearch_haul");

    // Authentication failed:
    if (authstring.length() == 0) {
        std::cout << "Authentication failed." << std::endl;
        return "";
    }

    std::ostringstream ss;

    ss << authstring;
    ss << RULESTRING << "\nC1\n";

    return catagolueRequest(ss.str().c_str(), "/verify");

}



void verifySearch(std::string payoshakey) {

    std::string response = obtainWork(payoshakey);

    if (response.length() <= 3) {
        std::cout << "Received no response from /verify." << std::endl;
        return;
    }

    // std::cout << "[[[" << response << "]]]" << std::endl;

    std::stringstream iss(response);
    std::vector<std::string> stringlist;

    std::string sub;
    while (std::getline(iss, sub, '\n')) {
        stringlist.push_back(sub);
        // std::cout << sub << std::endl;
    }

    if ((stringlist.size() < 4)) {
        std::cout << "No more hauls to verify." << std::endl;
        return;
    }

    std::string authstring = authenticate(payoshakey.c_str(), "submit_verification");

    // Authentication failed:
    if (authstring.length() == 0) {
        std::cout << "Authentication failed." << std::endl;
        return;
    }

    std::ostringstream ss;
    ss << authstring << "\n";
    ss << "@MD5 " << stringlist[2] << "\n";
    ss << "@PASSCODE " << stringlist[3] << "\n";
    ss << "@RULE " << RULESTRING << "\n";
    ss << "@SYMMETRY C1\n";

    // Create an empty QuickLife universe:
    lifealgo *imp2 = createUniverse("QuickLife");
    std::cout << "Universe created." << std::endl;
    imp2->setrule(RULESTRING_SLASHED);

    SoupSearcher soup;

    for (unsigned int i = 4; i < stringlist.size(); i++)
    {

        std::string seed = stringlist[i];
        if ((seed.length() >= 4) && (seed.substr(0,3).compare("C1/") == 0)) {
            // std::cout << "[" << seed << "]" << std::endl;
            soup.censusSoup(seed.substr(3), "", imp2);
        } else {
            std::cout << "[" << seed << "]" << std::endl;
        }
    }

    long long totobjs;
    std::vector<std::pair<long long, std::string> > censusList = soup.getSortedList(totobjs);

    for (int i = censusList.size() - 1; i >= 0; i--) {
        ss << censusList[i].second << " " << censusList[i].first << "\n";
    }

    catagolueRequest(ss.str().c_str(), "/verify");

}

#ifdef USE_OPEN_MP

void parallelSearch(int n, int m, std::string payoshaKey, std::string seed, int local_log) {

    SoupSearcher globalSoup;

    long long offset = 0;
    bool finishedSearch = false;

    while (finishedSearch == false) {

        #pragma omp parallel num_threads(m)
        {
            int threadNumber = omp_get_thread_num();

            SoupSearcher localSoup;
            lifealgo* imp2;

            #pragma omp critical
            {
                imp2 = createUniverse("QuickLife");
                std::cout << "Universe " << (threadNumber + 1) << " of " << m << " created." << std::endl;
                imp2->setrule(RULESTRING_SLASHED);
            }

            long long elapsed = 0;

            #pragma omp for
            for (long long i = offset; i < offset + n; i++) {
                long long pseudoElapsed = offset + elapsed * m + threadNumber;
                elapsed += 1;
                if (pseudoElapsed % 10000 == 0) {
                    std::cout << pseudoElapsed << " soups processed..." << std::endl;
                }
                std::ostringstream ss;
                ss << i;
                localSoup.censusSoup(seed, ss.str(), imp2);
            }

            #pragma omp critical
            {

                globalSoup.aggregate(&(localSoup.census), &(localSoup.alloccur));

            }
        }

        offset += n;
        
        std::cout << "----------------------------------------------------------------------" << std::endl;
        std::cout << offset << " soups completed." << std::endl;
        std::cout << "Attempting to contact payosha256." << std::endl;
        std::string payoshaResponse = globalSoup.submitResults(payoshaKey, seed, offset, local_log);
        if (payoshaResponse.length() == 0) {
            std::cout << "Connection was unsuccessful; continuing search..." << std::endl;
        } else {
            std::cout << "Connection was successful; starting new search..." << std::endl;
            finishedSearch = true;
        }
        std::cout << "----------------------------------------------------------------------" << std::endl;
    }

}

#endif

void correctionHaul(std::string payoshaKey) {

    SoupSearcher soup;
    soup.alloccur["yl144_1_16_afb5f3db909e60548f086e22ee3353ac"].push_back("0");
    soup.alloccur["yl384_1_59_7aeb1999980c43b4945fb7fcdb023326"].push_back("0");

    soup.census["yl144_1_16_afb5f3db909e60548f086e22ee3353ac"] = 16807;
    soup.census["yl384_1_59_7aeb1999980c43b4945fb7fcdb023326"] = 6942;
    soup.census["yl384_1_59_14c650b58076b33bd689d25c6add2153"] = -1;

    soup.submitResults(payoshaKey, "#adjustment2", 1, 0);

}

void runSearch(int n, std::string payoshaKey, std::string seed, int local_log, bool testing) {

    // Create an empty QuickLife universe:
    lifealgo *imp2 = createUniverse("QuickLife");
    std::cout << "Universe created." << std::endl;
    imp2->setrule(RULESTRING_SLASHED);

    SoupSearcher soup;

    clock_t start = clock();

    std::cout << "Running " << n << " soups per haul:" << std::endl;


    long long i = 0;
    bool finishedSearch = false;

    while (finishedSearch == false) {

        std::ostringstream ss;
        ss << i;

        soup.censusSoup(seed, ss.str(), imp2);

        i += 1;

        if (i % 10000 == 0) {
            clock_t end = clock();

            std::cout << i << " soups completed (" << (int) (10000.0 / ((double) (end-start) / CLOCKS_PER_SEC)) << " soups per second)." << std::endl;

            start = clock();
        }

        if (i % n == 0) {
            if (testing) {
                finishedSearch = true;
            } else {
            std::cout << "----------------------------------------------------------------------" << std::endl;
            std::cout << i << " soups completed." << std::endl;
            std::cout << "Attempting to contact payosha256." << std::endl;
            std::string payoshaResponse = soup.submitResults(payoshaKey, seed, i, local_log);
            if (payoshaResponse.length() == 0) {
                std::cout << "Connection was unsuccessful; continuing search..." << std::endl;
            } else {
                std::cout << "Connection was successful; starting new search..." << std::endl;
                finishedSearch = true;
            }
            std::cout << "----------------------------------------------------------------------" << std::endl;
            }
        }

    }

}


int main (int argc, char *argv[]) {


    /*
    testmain();
    return 0;

    vlife universe;
    universe.setcell(1000, 500, 2);
    universe.setcell(800, 800, 1);
    universe.setcell(300, 300, 2);
    universe.setcell(1, -1, 1);

    std::vector<int> celllist = getcells(&universe);

    for (uint32_t i = 0; i < celllist.size(); i++) {
        std::cout << celllist[i] << ", ";
    }

    std::cout << std::endl;

    // return 0;
    */


    // Default values:
    int soups_per_haul = 10000000;
    std::string payoshaKey = "#anon";
    std::string seed = reseed("original seed");
    int verifications = 0;
    int parallelisation = 0;
    int local_log = 0;
    bool testing = false;

    std::cout << "\nGreetings, this is \033[1;33mapgmera " << APG_VERSION << "\033[0m, configured for \033[1;34m" << RULESTRING_SLASHED << "\033[0m.\n" << std::endl;

    // Extract options:
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            payoshaKey = argv[i+1];
        } else if (strcmp(argv[i], "-s") == 0) {
            seed = argv[i+1];
        } else if (strcmp(argv[i], "-n") == 0) {
            soups_per_haul = atoi(argv[i+1]);
            /*
            if (soups_per_haul < 1000000) {
                soups_per_haul = 1000000;
            } else if (soups_per_haul > 100000000) {
                soups_per_haul = 100000000;
            }
            */
        // } else if (strcmp(argv[i], "-v") == 0) {
        //    verifications = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-L") == 0) {
            local_log = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            testing = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            parallelisation = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "--adjustment") == 0) {
            correctionHaul(payoshaKey);
            return 0;
        }
    }

    // Initialise QuickLife:
    qlifealgo::doInitializeAlgoInfo(staticAlgoInfo::tick());

    while (true) {
        if (verifications > 0) {
            std::cout << "Peer-reviewing hauls:\n" << std::endl;
            // Verify some hauls:
            for (int j = 0; j < verifications; j++) {
                verifySearch(payoshaKey);
            }
            std::cout << "\nPeer-review complete; proceeding search.\n" << std::endl;
        }

        // Run the search:
        std::cout << "Using seed " << seed << std::endl;
        if (parallelisation > 0) {
            #ifdef USE_OPEN_MP
            parallelSearch(soups_per_haul, parallelisation, payoshaKey, seed, local_log);
            #else
            runSearch(soups_per_haul, payoshaKey, seed, local_log, false);
            #endif
        } else {
            runSearch(soups_per_haul, payoshaKey, seed, local_log, testing);
        }
        seed = reseed(seed);

        if (testing) { break; }
    }

    return 0;
}
