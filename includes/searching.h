#pragma once

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
        std::cout << "Attempting to contact payosha256." << std::endl;
        std::string payoshaResponse = globalSoup.submitResults(payoshaKey, seed, offset, local_log, 0);
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
    apg::lifetree<uint32_t, BITPLANES> lt(400);
    apg::base_classifier<BITPLANES> cfier(&lt, RULESTRING);

    clock_t start = clock();

    std::cout << "Running " << n << " soups per haul:" << std::endl;

    int64_t i = 0;
    int64_t lasti = 0;

    bool finishedSearch = false;

    while (finishedSearch == false) {

        std::ostringstream ss;
        ss << i;

        soup.censusSoup(seed, ss.str(), cfier);

        i += 1;

        double elapsed = ((double) (clock() - start)) / CLOCKS_PER_SEC;

        if (elapsed >= 10.0) {
            std::cout << i << " soups completed (" << ((int) ((i - lasti) / elapsed)) << " soups per second)." << std::endl;
            lasti = i;
            start = clock();
        }

        if (i % n == 0) {
            std::cout << "----------------------------------------------------------------------" << std::endl;
            std::cout << i << " soups completed." << std::endl;
            std::cout << "Attempting to contact payosha256." << std::endl;
            std::string payoshaResponse = soup.submitResults(payoshaKey, seed, i, local_log, testing);
            if (payoshaResponse.length() == 0) {
                std::cout << "Connection was unsuccessful; continuing search..." << std::endl;
            } else {
                if (payoshaResponse == "testing") { std::cout << "testing mode" << std::endl; }
                std::cout << "\033[35m" << payoshaResponse << "\033[0m" << std::endl;
                std::cout << "Connection was successful; starting new search..." << std::endl;
                finishedSearch = true;
            }
            std::cout << "----------------------------------------------------------------------" << std::endl;
        }

    }

}


