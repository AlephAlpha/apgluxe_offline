
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>

#ifdef USE_OPEN_MP
#include <omp.h>
#endif

#include "includes/sha256.h"
#include "includes/md5.h"
#include "includes/payosha256.h"

#include "lifelib/classifier.h"

#define APG_VERSION "v4.0"
#include "includes/params.h"
#include "includes/hashsoup2.h"

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

double regress(std::vector<std::pair<double, double> > pairlist) {

    double cumx = 0.0;
    double cumy = 0.0;
    double cumvar = 0.0;
    double cumcov = 0.0;

    std::vector<std::pair<double, double> >::iterator it;
    for(it = pairlist.begin(); it < pairlist.end(); it++) {
        cumx += it->first;
        cumy += it->second;
    }

    cumx = cumx / pairlist.size();
    cumy = cumy / pairlist.size();

    for(it = pairlist.begin(); it < pairlist.end(); it++) {
        cumvar += (it->first - cumx) * (it->first - cumx);
        cumcov += (it->first - cumx) * (it->second - cumy);
    }

    return (cumcov / cumvar);

}

std::string powerlyse(apg::pattern &ipat, int stepsize, int numsteps, int startgen) {

    apg::pattern pat = ipat;

    std::vector<std::pair<double, double> > pairlist;
    std::vector<std::pair<double, double> > pairlist2;
    double cumpop = 1.0;

    for (int i = 0; i < numsteps; i++) {
        pat = pat[stepsize];
        cumpop += pat.popcount((1 << 30) + 3);
        pairlist.push_back(std::make_pair(std::log(i*stepsize+startgen), std::log(cumpop)));
        pairlist2.push_back(std::make_pair(std::log(i+1), std::log(cumpop)));
    }

    double power = regress(pairlist);
    double power2 = regress(pairlist2);

    if (power2 < 1.1) {
        return "PATHOLOGICAL";
    } else if (power < 1.65) {
        return "zz_REPLICATOR";
    } else if (power < 2.1) {
        return "zz_LINEAR";
    } else if (power < 2.9) {
        return "zz_EXPLOSIVE";
    } else {
        return "zz_QUADRATIC";
    }

}

std::string linearlyse(apg::pattern ipat, int maxperiod)
{
    int poplist[3 * maxperiod];
    int difflist[2 * maxperiod];

    apg::pattern pat = ipat;

    // runPattern(curralgo, 100);

    for (int i = 0; i < 3 * maxperiod; i++) {
        pat = pat[1];
        poplist[i] = pat.popcount((1 << 30) + 3);
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

/*
 * Stabilisation detection by checking for population periodicity:
 */
int naivestab_awesome(apg::pattern &pat) {

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

        pat = pat[period];
        currpop = pat.popcount((1 << 30) + 3);
        if (currpop == prevpop) {
            depth += 1;
        } else {
            depth = 0;
            if (period < 30) {
                i += 1;
                pat = pat[12];
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
int stabilise3(apg::pattern &pat) {

    int pp = naivestab_awesome(pat);

    if (pp > 0) {
        return pp;
    }

    // Phase II of stabilisation detection, which is much more rigorous and based on oscar.py.

    std::vector<uint64_t> hashlist;
    std::vector<int> genlist;

    int generation = 0;

    for (int j = 0; j < 4000; j++) {

        pat = pat[30];
        generation += 30;

        uint64_t h = pat.subrect(-4096, -4096, 8192, 8192).digest();

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

                int prevpop = pat.popcount((1 << 30) + 3);

                for (int i = 0; i < 20; i++) {
                    pat = pat[period];
                    int currpop = pat.popcount((1 << 30) + 3);
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

    pat = pat[1280];

    return 1280;


}

/*
 * Get the representation of a single object:
 */
std::string classifyAperiodic(apg::pattern pat) {

    std::string repr = linearlyse(pat, 4100);
    if (repr[0] != 'y') {
        repr = powerlyse(pat, 32, 8000, 5380);
    }

    return repr;

}


/*
 * This contains everything necessary for performing a soup search.
 */
class SoupSearcher {

public:

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

    bool separate(apg::pattern pat, int duration, bool proceedNonetheless, apg::classifier &cfier, std::string suffix) {

        std::map<std::string, int64_t> cm = cfier.census(pat, duration, &classifyAperiodic);

        bool ignorePathologicals = false;
        int pathologicals = 0;

        for (auto it = cm.begin(); it != cm.end(); ++it) {
            if (it->first[0] == 'y') {
                ignorePathologicals = true;
            } else if (it->first == "PATHOLOGICAL") {
                pathologicals += 1;
            }
        }

        if (pathologicals > 0) {
            if (proceedNonetheless) {
                if (ignorePathologicals == false) { std::cout << "Pathological object detected!!!" << std::endl; }
            } else {
                return true;
            }
        }

        for (auto it = cm.begin(); it != cm.end(); ++it) {
            std::string apgcode = it->first;
            if ((ignorePathologicals == false) || (apgcode.compare("PATHOLOGICAL") != 0)) {
                census[apgcode] += it->second;
                if (alloccur[apgcode].size() == 0 || alloccur[apgcode].back().compare(suffix) != 0) {
                    if (alloccur[apgcode].size() < 10) {
                        alloccur[apgcode].push_back(suffix);
                    }
                }
            }

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
                std::cout << "Linear-growth pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            } else if ((apgcode[0] == 'z') && (apgcode[1] == 'z')) {
                std::cout << "Chaotic-growth pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            }
            #else
            if ((apgcode[0] == 'y') && (apgcode[1] == 'l')) {
                std::cout << "Linear-growth pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            } else if ((apgcode[0] == 'z') && (apgcode[1] == 'z')) {
                std::cout << "Chaotic-growth pattern detected: \033[1;32m" << apgcode << "\033[0m" << std::endl;
            }
            #endif


        }
        return false;

    }

    void censusSoup(std::string seedroot, std::string suffix, apg::classifier &cfier) {

        apg::bitworld bw = apg::hashsoup(seedroot + suffix, SYMMETRY);
        apg::pattern pat(cfier.lab, cfier.lab->demorton(bw, 1), RULESTRING);

        int duration = stabilise3(pat);

        bool failure = true;
        int attempt = 0;

        // Repeat until there are no pathological objects, or until five attempts have elapsed:
        while (failure) {

            failure = false;

            if (pat.nonempty()) {

                failure = separate(pat, duration, (attempt >= 5), cfier, suffix);

            }

            // Pathological object detected:
            if (failure) {
                attempt += 1;
                pat = pat[10000];
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
        ss << "@SYMMETRY " << SYMMETRY << "\n";
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
    ss << RULESTRING << "\n" << SYMMETRY << "\n";

    return catagolueRequest(ss.str().c_str(), "/verify");

}

/*

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
    ss << "@SYMMETRY " << SYMMETRY << "\n";

    // Create an empty QuickLife universe:
    lifealgo *imp2 = createUniverse("QuickLife");
    std::cout << "Universe created." << std::endl;
    imp2->setrule(RULESTRING_SLASHED);

    SoupSearcher soup;

    for (unsigned int i = 4; i < stringlist.size(); i++)
    {

        std::string symslash = SYMMETRY "/";
        std::string seed = stringlist[i];
        if ((seed.length() >= 4) && (seed.substr(0,symslash.length()).compare(symslash) == 0)) {
            // std::cout << "[" << seed << "]" << std::endl;
            soup.censusSoup(seed.substr(symslash.length()), "", imp2);
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

*/

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
            apg::lifetree<uint32_t, 1> lt(400);
            apg::classifier cfier(&lt, RULESTRING);

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
                localSoup.censusSoup(seed, ss.str(), cfier);
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


void runSearch(int n, std::string payoshaKey, std::string seed, int local_log, bool testing) {

    SoupSearcher soup;
    apg::lifetree<uint32_t, 1> lt(400);
    apg::classifier cfier(&lt, RULESTRING);

    clock_t start = clock();

    std::cout << "Running " << n << " soups per haul:" << std::endl;


    long long i = 0;
    bool finishedSearch = false;

    while (finishedSearch == false) {

        std::ostringstream ss;
        ss << i;

        soup.censusSoup(seed, ss.str(), cfier);

        i += 1;

        if (i % 100 == 0) {
            clock_t end = clock();

            std::cout << i << " soups completed (" << (int) (100.0 / ((double) (end-start) / CLOCKS_PER_SEC)) << " soups per second)." << std::endl;

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
    int verifications = -1;
    int parallelisation = 0;
    int local_log = 0;
    bool testing = false;
    int nullargs = 1;

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
        } else if (strcmp(argv[i], "-v") == 0) {
            verifications = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-L") == 0) {
            local_log = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-t") == 0) {
            testing = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            parallelisation = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "--rule") == 0) {
            std::cout << "\033[1;33mapgmera " << APG_VERSION << "\033[0m: ";
            std::string desired_rulestring = argv[i+1];
            if (strcmp(RULESTRING, argv[i+1]) == 0) {
                std::cout << "Rule \033[1;34m" << RULESTRING << "\033[0m is correctly configured." << std::endl;
                nullargs += 2;
            } else {
                std::cout << "Rule \033[1;34m" << RULESTRING << "\033[0m does not match desired rule \033[1;34m" << desired_rulestring << "\033[0m." << std::endl;
                execvp("./recompile.sh", argv);
                return 1;
            }
        } else if (strcmp(argv[i], "--symmetry") == 0) {
            std::cout << "\033[1;33mapgmera " << APG_VERSION << "\033[0m: ";
            std::string desired_symmetry = argv[i+1];
            if (strcmp(SYMMETRY, argv[i+1]) == 0) {
                std::cout << "Symmetry \033[1;34m" << SYMMETRY << "\033[0m is correctly configured." << std::endl;
                nullargs += 2;
            } else {
                std::cout << "Symmetry \033[1;34m" << SYMMETRY << "\033[0m does not match desired symmetry \033[1;34m" << desired_symmetry << "\033[0m." << std::endl;
                execvp("./recompile.sh", argv);
                return 1;
            }
        }
    }

    if ((argc == nullargs) && (argc > 1)) { return 0; }

    // Disable verification by default if running on a HPC;
    // otherwise verify three hauls per uploaded haul:
    if (verifications < 0) {
        verifications = (parallelisation <= 4) ? 3 : 0;
    }

    std::cout << "\nGreetings, this is \033[1;33mapgmera " << APG_VERSION << "\033[0m, configured for \033[1;34m" << RULESTRING << "/" << SYMMETRY << "\033[0m.\n" << std::endl;

    while (true) {
        if (verifications > 0) {
            std::cout << "Peer-reviewing hauls:\n" << std::endl;
            // Verify some hauls:
            for (int j = 0; j < verifications; j++) {
               // verifySearch(payoshaKey);
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
