#pragma once

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

    bool separate(UPATTERN &pat, int duration, bool proceedNonetheless, apg::base_classifier<BITPLANES> &cfier, std::string suffix) {

        pat.decache();
        pat.advance(0, 1, duration);
        std::vector<apg::bitworld> bwv(BITPLANES + 1);

        #ifdef INCUBATE
        apg::incubator<56, 56> icb;
        apg::copycells(&pat, &icb);
        uint64_t excess[8] = {0};
        icb.purge(excess);
        icb.to_bitworld(bwv[0], 0);
        icb.to_bitworld(bwv[1], 1);
        #else
        pat.extractPattern(bwv);
        #endif

        std::map<std::string, int64_t> cm = cfier.census(bwv, &classifyAperiodic);

        #ifdef INCUBATE
        if (excess[3] > 0) { cm["xp2_7"] += excess[3]; }
        if (excess[4] > 0) { cm["xs4_33"] += excess[4]; }
        if (excess[5] > 0) { cm["xq4_153"] += excess[5]; }
        if (excess[6] > 0) { cm["xs6_696"] += excess[6]; }
        #endif

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

    void censusSoup(std::string seedroot, std::string suffix, apg::base_classifier<BITPLANES> &cfier) {

        apg::bitworld bw = apg::hashsoup(seedroot + suffix, SYMMETRY);
        std::vector<apg::bitworld> vbw;
        vbw.push_back(bw);
        UPATTERN pat;
        pat.insertPattern(vbw);

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
                pat.clearHistory();
                pat.decache();
                pat.advance(0, 0, 10000);
                duration = 4000;
            }
        }
    }


    std::vector<std::pair<long long, std::string> > getSortedList(long long &totobjs) {

        std::vector<std::pair<long long, std::string> > censusList;

        std::map<std::string, long long>::iterator it;
        for (it = census.begin(); it != census.end(); it++)
        {
            if ((it->second != 0) && (it->first != "xs0_0")) {
                censusList.push_back(std::make_pair(it->second, it->first));
                totobjs += it->second;
            }
        }
        std::sort(censusList.begin(), censusList.end());

        return censusList;

    }

    std::string submitResults(std::string payoshakey, std::string root, long long numsoups, int local_log, bool testing) {

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

        if (testing) { return "testing"; }

        return catagolueRequest(ss.str().c_str(), "/apgsearch");

    }

};
