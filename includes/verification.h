#pragma once

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

void verifySearch(std::string payoshakey) {

    std::string response = obtainWork(payoshakey);

    if (response.length() <= 3) {
        std::cout << "Received no response from /verify." << std::endl;
        return;
    }

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

    SoupSearcher soup;
    apg::lifetree<uint32_t, BITPLANES> lt(400);
    apg::base_classifier<BITPLANES> cfier(&lt, RULESTRING);

    for (unsigned int i = 4; i < stringlist.size(); i++)
    {

        std::string symslash = SYMMETRY "/";
        std::string seed = stringlist[i];
        if ((seed.length() >= 4) && (seed.substr(0,symslash.length()).compare(symslash) == 0)) {
            soup.censusSoup(seed.substr(symslash.length()), "", cfier);
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
