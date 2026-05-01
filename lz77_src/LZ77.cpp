#include "LZ77.h"

#include <fstream>
#include <iostream>

using namespace std;

bool LZ77::readFile(const string &inFile, string &data) {
    ifstream in(inFile, ios::binary);
    if (!in) {
        return false;
    }

    data.assign((istreambuf_iterator<char>(in)),
                istreambuf_iterator<char>());
    return true;
}

bool LZ77::writeFile(const string &outFile, const string &data) {
    ofstream out(outFile, ios::binary);
    if (!out) {
        return false;
    }

    out.write(data.data(), static_cast<streamsize>(data.size()));
    return true;
}

vector<LZ77::Token> LZ77::compressData(const string &inputData) {
    vector<Token> tokens;
    size_t i = 0;
    string literalBuffer;

    while (i < inputData.size()) {
        uint16_t bestOffset = 0;
        uint16_t bestLength = 0;

        for (size_t j = 0; j < i; ++j) {
            uint16_t currentLength = 0;

            while (i + currentLength < inputData.size() &&
                   j + currentLength < i &&
                   currentLength < 65535 &&
                   inputData[j + currentLength] == inputData[i + currentLength]) {
                ++currentLength;
            }

            if (currentLength > bestLength) {
                bestLength = currentLength;
                bestOffset = static_cast<uint16_t>(i - j);
            }
        }

        if (bestLength >= MIN_MATCH_LENGTH) {
            if (!literalBuffer.empty()) {
                Token literalToken;
                literalToken.isMatch = false;
                literalToken.offset = 0;
                literalToken.length = static_cast<uint16_t>(literalBuffer.size());
                literalToken.literal = literalBuffer;
                tokens.push_back(literalToken);
                literalBuffer.clear();
            }

            Token matchToken;
            matchToken.isMatch = true;
            matchToken.offset = bestOffset;
            matchToken.length = bestLength;
            tokens.push_back(matchToken);
            i += bestLength;
        } else {
            literalBuffer.push_back(inputData[i]);
            if (literalBuffer.size() == 65535) {
                Token literalToken;
                literalToken.isMatch = false;
                literalToken.offset = 0;
                literalToken.length = static_cast<uint16_t>(literalBuffer.size());
                literalToken.literal = literalBuffer;
                tokens.push_back(literalToken);
                literalBuffer.clear();
            }
            ++i;
        }
    }

    if (!literalBuffer.empty()) {
        Token literalToken;
        literalToken.isMatch = false;
        literalToken.offset = 0;
        literalToken.length = static_cast<uint16_t>(literalBuffer.size());
        literalToken.literal = literalBuffer;
        tokens.push_back(literalToken);
    }

    return tokens;
}

string LZ77::decompressData(const vector<Token> &tokens) {
    string output;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token &t = tokens[i];

        if (!t.isMatch) {
            output += t.literal;
        } else {
            if (t.offset == 0 || t.offset > output.size()) {
                return string();
            }

            size_t start = output.size() - t.offset;
            for (uint16_t k = 0; k < t.length; ++k) {
                output.push_back(output[start + k]);
            }
        }
    }

    return output;
}

bool LZ77::encodeFile(const string &inputFile, const string &outputFile) {
    string inputData;
    if (!readFile(inputFile, inputData)) {
        cerr << "Failed to read input file: " << inputFile << "\n";
        return false;
    }

    vector<Token> tokens = compressData(inputData);

    ofstream out(outputFile, ios::binary);
    if (!out) {
        cerr << "Failed to create output file: " << outputFile << "\n";
        return false;
    }

    uint32_t tokenCount = static_cast<uint32_t>(tokens.size());
    out.write(reinterpret_cast<const char*>(&tokenCount), sizeof(tokenCount));

    for (size_t i = 0; i < tokens.size(); ++i) {
        unsigned char type = tokens[i].isMatch ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&type), sizeof(type));

        if (tokens[i].isMatch) {
            out.write(reinterpret_cast<const char*>(&tokens[i].offset), sizeof(tokens[i].offset));
            out.write(reinterpret_cast<const char*>(&tokens[i].length), sizeof(tokens[i].length));
        } else {
            out.write(reinterpret_cast<const char*>(&tokens[i].length), sizeof(tokens[i].length));
            out.write(tokens[i].literal.data(), static_cast<streamsize>(tokens[i].literal.size()));
        }
    }

    return true;
}

bool LZ77::decodeFile(const string &inputFile, const string &outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Failed to open compressed file: " << inputFile << "\n";
        return false;
    }

    uint32_t tokenCount = 0;
    in.read(reinterpret_cast<char*>(&tokenCount), sizeof(tokenCount));
    if (!in) {
        cerr << "Failed to read token count\n";
        return false;
    }

    vector<Token> tokens;
    tokens.reserve(tokenCount);

    for (uint32_t i = 0; i < tokenCount; ++i) {
        unsigned char type = 0;
        in.read(reinterpret_cast<char*>(&type), sizeof(type));
        if (!in) {
            cerr << "Compressed file is incomplete or corrupted\n";
            return false;
        }

        Token token;
        token.isMatch = (type == 1);
        token.offset = 0;
        token.length = 0;

        if (token.isMatch) {
            in.read(reinterpret_cast<char*>(&token.offset), sizeof(token.offset));
            in.read(reinterpret_cast<char*>(&token.length), sizeof(token.length));
        } else {
            in.read(reinterpret_cast<char*>(&token.length), sizeof(token.length));
            if (!in) {
                cerr << "Compressed file is incomplete or corrupted\n";
                return false;
            }

            token.literal.resize(token.length);
            in.read(&token.literal[0], token.length);
        }

        if (!in) {
            cerr << "Compressed file is incomplete or corrupted\n";
            return false;
        }

        tokens.push_back(token);
    }

    string decoded = decompressData(tokens);
    if (decoded.empty() && !tokens.empty()) {
        bool hadLiteralContent = false;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (!tokens[i].isMatch && !tokens[i].literal.empty()) {
                hadLiteralContent = true;
                break;
            }
        }
        if (!hadLiteralContent) {
            cerr << "Compressed file contains invalid match data\n";
            return false;
        }
    }

    if (!writeFile(outputFile, decoded)) {
        cerr << "Failed to write output file: " << outputFile << "\n";
        return false;
    }

    return true;
}