#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

class LZ77 {
private:
    struct Token {
        bool isMatch;
        uint16_t offset;
        uint16_t length;
        string literal;
    };

    static const uint16_t MIN_MATCH_LENGTH = 6;

    bool readFile(const string &inFile, string &data);
    bool writeFile(const string &outFile, const string &data);
    vector<Token> compressData(const string &inputData);
    string decompressData(const vector<Token> &tokens);

public:
    bool encodeFile(const string &inputFile, const string &outputFile);
    bool decodeFile(const string &inputFile, const string &outputFile);
};

#endif