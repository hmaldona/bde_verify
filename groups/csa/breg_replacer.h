#ifndef BREG_CSV
#define BREG_CSV

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

namespace {

enum boolValue {
    trueValue = 1,
    falseValue = 0,
    defaultValue = 3
};

static void split(std::string s, char delim, std::vector<std::string>& tokens)
{
    std::stringstream stream(s);
    std::string str;
    while (std::getline(stream, str, delim)) {
        tokens.push_back(str);
    }
}

static std::vector<std::pair<std::string, boolValue>> readCSV(std::string file) 
{
    std::vector<std::pair<std::string, boolValue>> bregs;
    std::ifstream bregFile;
    bregFile.open(file);

    if ( bregFile.is_open() ) {
        std::vector<std::string> lines;
        std::string str;

        while ( std::getline(bregFile, str) ) {
            lines.push_back(str);
        }

        for (auto l : lines) {
            std::vector<std::string> t;
            split(l, ',', t);
            std::pair<std::string, boolValue> pair;
            pair.first = t[0];
            if ( t.size() == 2 ) {
                pair.second = (t[1] == "true" ? trueValue : falseValue);
            }
            else {
                pair.second = defaultValue;
            }

            bregs.push_back(pair);
        }
        bregFile.close();
    }
    else {
        std::cout << "file: "<< file << " was not found" << std::endl;
        return bregs;
    }
    return bregs;
}
}

#endif
