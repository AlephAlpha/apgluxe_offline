#pragma once

#include <stdio.h>
#include <sys/select.h>

// determine whether there's a keystroke waiting
int keyWaiting() {
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); // STDIN_FILENO is 0

    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

    return FD_ISSET(STDIN_FILENO, &fds);
}

void populateLuts() {

        apg::bitworld bw = apg::hashsoup("", SYMMETRY);
        std::vector<apg::bitworld> vbw;
        vbw.push_back(bw);
        UPATTERN pat;
        pat.insertPattern(vbw);
        pat.advance(0, 0, 8);

}

#ifdef USE_OPEN_MP

bool parallelSearch(int n, int m, std::string seed) {

    SoupSearcher globalSoup;

    long long offset = 0;
    bool finishedSearch = false;

    // Ensure the lookup tables are populated by the main thread:
    populateLuts();

    while (finishedSearch == false) {

        #pragma omp parallel num_threads(m)
        {
            int threadNumber = omp_get_thread_num();

            SoupSearcher localSoup;
            apg::lifetree<uint32_t, BITPLANES> lt(400);
            apg::base_classifier<BITPLANES> cfier(&lt, RULESTRING);

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
        std::cout << "Logging..." << std::endl;
        globalSoup.logResults(seed, offset);
        std::cout << "Starting new search..." << std::endl;
        finishedSearch = true;
        std::cout << "----------------------------------------------------------------------" << std::endl;
    }
    
    return false;

}

#endif

bool runSearch(int n, std::string seed) {

    SoupSearcher soup;
    apg::lifetree<uint32_t, BITPLANES> lt(400);
    apg::base_classifier<BITPLANES> cfier(&lt, RULESTRING);

    clock_t start = clock();

    std::cout << "Running " << n << " soups per haul:" << std::endl;

    int64_t i = 0;
    int64_t lasti = 0;

    bool finishedSearch = false;
    bool quitByUser = false;

    while ((finishedSearch == false) && (quitByUser == false)) {

        std::ostringstream ss;
        ss << i;

        soup.censusSoup(seed, ss.str(), cfier);

        i += 1;

        double elapsed = ((double) (clock() - start)) / CLOCKS_PER_SEC;

        if (elapsed >= 10.0) {
            std::cout << i << " soups completed (" << ((int) ((i - lasti) / elapsed)) << " soups per second)." << std::endl;
            lasti = i;
            start = clock();

            if(keyWaiting()) {
                char c = fgetc(stdin);
                if ((c == 'q') || (c == 'Q'))
                    quitByUser = true;
            }
            
        }

        if ((i % n == 0) || quitByUser) {
            std::cout << "----------------------------------------------------------------------" << std::endl;
            std::cout << i << " soups completed." << std::endl;
            std::cout << "Logging..." << std::endl;
            soup.logResults(seed, i);
            std::cout << "Starting new search..." << std::endl;
            finishedSearch = true;
            std::cout << "----------------------------------------------------------------------" << std::endl;
        }

    }
    
    return quitByUser;

}


