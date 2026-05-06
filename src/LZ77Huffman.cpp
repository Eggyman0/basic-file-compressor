#include "LZ77Huffman.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>

using std::array;
using std::deque;
using std::ifstream;
using std::ofstream;
using std::priority_queue;
using std::string;
using std::unordered_map;
using std::vector;

namespace {
    // Custom file signature. This identifies files made by this compressor.
    const char MAGIC[4] = {'L', 'Z', 'H', '1'};

    void writeU16(vector<uint8_t>& out, uint16_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xff));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    }

    void writeU32(vector<uint8_t>& out, uint32_t value) {
        for (int i = 0; i < 4; i++) {
            out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xff));
        }
    }

    void writeU64(vector<uint8_t>& out, uint64_t value) {
        for (int i = 0; i < 8; i++) {
            out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xff));
        }
    }

    bool readU16(const vector<uint8_t>& in, size_t& pos, uint16_t& value) {
        if (pos + 2 > in.size()) return false;
        value = static_cast<uint16_t>(in[pos]) |
                static_cast<uint16_t>(in[pos + 1] << 8);
        pos += 2;
        return true;
    }

    bool readU32(const vector<uint8_t>& in, size_t& pos, uint32_t& value) {
        if (pos + 4 > in.size()) return false;
        value = 0;
        for (int i = 0; i < 4; i++) {
            value |= static_cast<uint32_t>(in[pos++]) << (8 * i);
        }
        return true;
    }

    bool readU64(const vector<uint8_t>& in, size_t& pos, uint64_t& value) {
        if (pos + 8 > in.size()) return false;
        value = 0;
        for (int i = 0; i < 8; i++) {
            value |= static_cast<uint64_t>(in[pos++]) << (8 * i);
        }
        return true;
    }

    // Writes individual bits into a byte vector.
    class BitWriter {
    public:
        void writeBits(uint64_t bits, uint8_t length) {
            for (int i = length - 1; i >= 0; i--) {
                currentByte = static_cast<uint8_t>((currentByte << 1) | ((bits >> i) & 1));
                bitsUsed++;
                totalBits++;

                if (bitsUsed == 8) {
                    bytes.push_back(currentByte);
                    currentByte = 0;
                    bitsUsed = 0;
                }
            }
        }

        vector<uint8_t> finish() {
            if (bitsUsed > 0) {
                currentByte <<= (8 - bitsUsed);
                bytes.push_back(currentByte);
            }
            return bytes;
        }

        uint64_t getBitCount() const {
            return totalBits;
        }

    private:
        vector<uint8_t> bytes;
        uint8_t currentByte = 0;
        int bitsUsed = 0;
        uint64_t totalBits = 0;
    };

    // Reads individual bits from a byte vector.
    class BitReader {
    public:
        BitReader(const vector<uint8_t>& data, uint64_t bitCount)
            : bytes(data), totalBits(bitCount) {}

        bool readBit(uint8_t& bit) {
            if (bitsRead >= totalBits) return false;

            size_t byteIndex = static_cast<size_t>(bitsRead / 8);
            int shift = 7 - static_cast<int>(bitsRead % 8);

            if (byteIndex >= bytes.size()) return false;

            bit = static_cast<uint8_t>((bytes[byteIndex] >> shift) & 1);
            bitsRead++;
            return true;
        }

    private:
        const vector<uint8_t>& bytes;
        uint64_t totalBits;
        uint64_t bitsRead = 0;
    };

    struct HuffmanNode {
        uint64_t freq = 0;
        int symbol = -1;
        HuffmanNode* left = nullptr;
        HuffmanNode* right = nullptr;

        HuffmanNode(uint64_t f, int s) : freq(f), symbol(s) {}
        HuffmanNode(HuffmanNode* l, HuffmanNode* r)
            : freq(l->freq + r->freq), symbol(-1), left(l), right(r) {}
    };

    struct CompareHuffmanNode {
        bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
            if (a->freq != b->freq) return a->freq > b->freq;
            return a->symbol > b->symbol;
        }
    };

    void deleteTree(HuffmanNode* root) {
        if (!root) return;
        deleteTree(root->left);
        deleteTree(root->right);
        delete root;
    }

    void findCodeLengths(HuffmanNode* node, uint8_t depth, array<uint8_t, 256>& codeLengths) {
        if (!node) return;

        if (!node->left && !node->right) {
            // If the file has only one unique byte, give it a 1-bit code.
            codeLengths[static_cast<size_t>(node->symbol)] = std::max<uint8_t>(1, depth);
            return;
        }

        findCodeLengths(node->left, static_cast<uint8_t>(depth + 1), codeLengths);
        findCodeLengths(node->right, static_cast<uint8_t>(depth + 1), codeLengths);
    }

    uint32_t threeByteHash(const vector<uint8_t>& data, size_t pos) {
        return (static_cast<uint32_t>(data[pos]) << 16) |
               (static_cast<uint32_t>(data[pos + 1]) << 8) |
                static_cast<uint32_t>(data[pos + 2]);
    }

    void addLiteralToken(vector<uint8_t>& tokens, vector<uint8_t>& literalBytes) {
        if (literalBytes.empty()) return;

        // Token type 0 means: raw literal bytes follow.
        tokens.push_back(0);
        writeU16(tokens, static_cast<uint16_t>(literalBytes.size()));
        tokens.insert(tokens.end(), literalBytes.begin(), literalBytes.end());
        literalBytes.clear();
    }
}

bool LZ77Huffman::readFile(const string& path, vector<uint8_t>& data) {
    ifstream in(path, std::ios::binary);
    if (!in) return false;

    data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool LZ77Huffman::writeFile(const string& path, const vector<uint8_t>& data) {
    ofstream out(path, std::ios::binary);
    if (!out) return false;

    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return true;
}

bool LZ77Huffman::encodeFile(const string& inputPath, const string& outputPath) {
    vector<uint8_t> input;
    if (!readFile(inputPath, input)) return false;
    if (input.size() > UINT32_MAX) return false;

    // Step 1: Use LZ77 to convert repeated byte sequences into tokens.
    vector<uint8_t> lz77Tokens = lz77Compress(input);
    if (lz77Tokens.size() > UINT32_MAX) return false;

    // Step 2: Use Huffman coding to compress the LZ77 token byte stream.
    array<uint8_t, 256> codeLengths{};
    uint64_t huffmanBitCount = 0;
    vector<uint8_t> packedHuffmanData = huffmanCompress(lz77Tokens, codeLengths, huffmanBitCount);

    // Step 3: Write a small custom header, then the Huffman-compressed data.
    vector<uint8_t> output;
    output.insert(output.end(), MAGIC, MAGIC + 4);
    writeU32(output, static_cast<uint32_t>(input.size()));
    writeU32(output, static_cast<uint32_t>(lz77Tokens.size()));
    writeU64(output, huffmanBitCount);

    // Canonical Huffman only needs the code lengths to rebuild the codes.
    output.insert(output.end(), codeLengths.begin(), codeLengths.end());
    output.insert(output.end(), packedHuffmanData.begin(), packedHuffmanData.end());

    return writeFile(outputPath, output);
}

bool LZ77Huffman::decodeFile(const string& inputPath, const string& outputPath) {
    vector<uint8_t> compressed;
    if (!readFile(inputPath, compressed)) return false;

    size_t pos = 0;

    // Step 1: Read and validate the custom file header.
    if (compressed.size() < 4 || std::memcmp(compressed.data(), MAGIC, 4) != 0) return false;
    pos += 4;

    uint32_t originalSize = 0;
    uint32_t lz77TokenSize = 0;
    uint64_t huffmanBitCount = 0;

    if (!readU32(compressed, pos, originalSize)) return false;
    if (!readU32(compressed, pos, lz77TokenSize)) return false;
    if (!readU64(compressed, pos, huffmanBitCount)) return false;
    if (pos + 256 > compressed.size()) return false;

    array<uint8_t, 256> codeLengths{};
    std::copy(compressed.begin() + static_cast<std::ptrdiff_t>(pos),
              compressed.begin() + static_cast<std::ptrdiff_t>(pos + 256),
              codeLengths.begin());
    pos += 256;

    vector<uint8_t> packedHuffmanData(compressed.begin() + static_cast<std::ptrdiff_t>(pos), compressed.end());

    // Step 2: Huffman-decode the LZ77 token byte stream.
    vector<uint8_t> lz77Tokens;
    if (!huffmanDecompress(packedHuffmanData, codeLengths, huffmanBitCount, lz77TokenSize, lz77Tokens)) {
        return false;
    }

    // Step 3: LZ77-decode the token stream back into the original data.
    vector<uint8_t> output;
    if (!lz77Decompress(lz77Tokens, originalSize, output)) {
        return false;
    }

    return writeFile(outputPath, output);
}

vector<uint8_t> LZ77Huffman::lz77Compress(const vector<uint8_t>& input) {
    vector<uint8_t> tokens;
    vector<uint8_t> literalBytes;

    // Hash table: maps a 3-byte sequence to recent positions where it appeared.
    // This avoids scanning the entire previous input for every position.
    unordered_map<uint32_t, deque<size_t>> matchTable;

    auto addPosition = [&](size_t pos) {
        if (pos + MIN_MATCH_LENGTH > input.size()) return;

        uint32_t hash = threeByteHash(input, pos);
        deque<size_t>& positions = matchTable[hash];
        positions.push_back(pos);

        // Keep only positions inside the sliding window.
        while (!positions.empty() && pos > positions.front() && pos - positions.front() > WINDOW_SIZE) {
            positions.pop_front();
        }
    };

    size_t i = 0;
    while (i < input.size()) {
        size_t bestLength = 0;
        size_t bestOffset = 0;

        // Step 1: Search for the longest previous match starting at position i.
        if (i + MIN_MATCH_LENGTH <= input.size()) {
            uint32_t hash = threeByteHash(input, i);
            auto found = matchTable.find(hash);

            if (found != matchTable.end()) {
                deque<size_t>& positions = found->second;
                size_t checked = 0;

                // Check recent matches first. Limit candidates to keep compression fast.
                for (auto it = positions.rbegin();
                     it != positions.rend() && checked < MAX_HASH_CANDIDATES;
                     ++it, ++checked) {

                    size_t matchPos = *it;
                    if (matchPos >= i || i - matchPos > WINDOW_SIZE) continue;

                    size_t length = 0;
                    while (length < MAX_MATCH_LENGTH &&
                           i + length < input.size() &&
                           input[matchPos + length] == input[i + length]) {
                        length++;
                    }

                    if (length > bestLength) {
                        bestLength = length;
                        bestOffset = i - matchPos;
                    }
                }
            }
        }

        // Step 2: Emit either a match token or collect a literal byte.
        if (bestLength >= MIN_MATCH_LENGTH) {
            addLiteralToken(tokens, literalBytes);

            // Token type 1 means: copy length bytes from offset bytes behind output position.
            tokens.push_back(1);
            writeU16(tokens, static_cast<uint16_t>(bestOffset));
            writeU16(tokens, static_cast<uint16_t>(bestLength));

            for (size_t k = 0; k < bestLength; k++) {
                addPosition(i + k);
            }
            i += bestLength;
        } else {
            literalBytes.push_back(input[i]);
            addPosition(i);
            i++;

            if (literalBytes.size() == MAX_LITERAL_LENGTH) {
                addLiteralToken(tokens, literalBytes);
            }
        }
    }

    addLiteralToken(tokens, literalBytes);
    return tokens;
}

bool LZ77Huffman::lz77Decompress(const vector<uint8_t>& tokens,
                                 uint32_t originalSize,
                                 vector<uint8_t>& output) {
    output.clear();
    output.reserve(originalSize);

    size_t pos = 0;
    while (pos < tokens.size()) {
        uint8_t tokenType = tokens[pos++];

        if (tokenType == 0) {
            // Literal token: copy the next length bytes directly to output.
            uint16_t length = 0;
            if (!readU16(tokens, pos, length)) return false;
            if (pos + length > tokens.size()) return false;

            output.insert(output.end(),
                          tokens.begin() + static_cast<std::ptrdiff_t>(pos),
                          tokens.begin() + static_cast<std::ptrdiff_t>(pos + length));
            pos += length;
        } else if (tokenType == 1) {
            // Match token: copy bytes from the data already decompressed.
            uint16_t offset = 0;
            uint16_t length = 0;

            if (!readU16(tokens, pos, offset)) return false;
            if (!readU16(tokens, pos, length)) return false;
            if (offset == 0 || offset > output.size()) return false;

            size_t start = output.size() - offset;
            for (uint16_t i = 0; i < length; i++) {
                output.push_back(output[start + i]);
            }
        } else {
            return false;
        }
    }

    return output.size() == originalSize;
}

vector<uint8_t> LZ77Huffman::huffmanCompress(const vector<uint8_t>& input,
                                             array<uint8_t, 256>& codeLengths,
                                             uint64_t& bitCount) {
    codeLengths.fill(0);
    bitCount = 0;

    // Step 1: Count byte frequencies in the LZ77 token stream.
    array<uint64_t, 256> frequencies{};
    for (uint8_t byte : input) {
        frequencies[byte]++;
    }

    // Step 2: Build a standard Huffman tree with a min-priority queue.
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, CompareHuffmanNode> queue;
    for (int symbol = 0; symbol < 256; symbol++) {
        if (frequencies[symbol] > 0) {
            queue.push(new HuffmanNode(frequencies[symbol], symbol));
        }
    }

    if (queue.empty()) {
        return vector<uint8_t>();
    }

    while (queue.size() > 1) {
        HuffmanNode* left = queue.top();
        queue.pop();

        HuffmanNode* right = queue.top();
        queue.pop();

        queue.push(new HuffmanNode(left, right));
    }

    HuffmanNode* root = queue.top();

    // Step 3: Convert the tree into canonical Huffman code lengths.
    findCodeLengths(root, 0, codeLengths);
    deleteTree(root);

    // Step 4: Build canonical codes from the code lengths.
    array<HuffmanCode, 256> codes = buildCanonicalCodes(codeLengths);

    // Step 5: Pack Huffman codes into bytes.
    BitWriter writer;
    for (uint8_t byte : input) {
        writer.writeBits(codes[byte].bits, codes[byte].length);
    }

    bitCount = writer.getBitCount();
    return writer.finish();
}

bool LZ77Huffman::huffmanDecompress(const vector<uint8_t>& packedBytes,
                                    const array<uint8_t, 256>& codeLengths,
                                    uint64_t bitCount,
                                    uint32_t expectedOutputSize,
                                    vector<uint8_t>& output) {
    output.clear();
    output.reserve(expectedOutputSize);

    if (expectedOutputSize == 0) {
        return bitCount == 0;
    }

    // Step 1: Rebuild the same canonical Huffman codes from the saved lengths.
    array<HuffmanCode, 256> codes = buildCanonicalCodes(codeLengths);

    // Step 2: Build a small decoding tree from the canonical codes.
    struct DecodeNode {
        int symbol = -1;
        DecodeNode* zero = nullptr;
        DecodeNode* one = nullptr;
    };

    DecodeNode* root = new DecodeNode();

    for (int symbol = 0; symbol < 256; symbol++) {
        if (codes[symbol].length == 0) continue;

        DecodeNode* current = root;
        for (int i = codes[symbol].length - 1; i >= 0; i--) {
            bool bit = ((codes[symbol].bits >> i) & 1) != 0;

            if (bit) {
                if (!current->one) current->one = new DecodeNode();
                current = current->one;
            } else {
                if (!current->zero) current->zero = new DecodeNode();
                current = current->zero;
            }
        }
        current->symbol = symbol;
    }

    auto deleteDecodeTree = [](auto&& self, DecodeNode* node) -> void {
        if (!node) return;
        self(self, node->zero);
        self(self, node->one);
        delete node;
    };

    // Step 3: Read bits and walk the decoding tree until the expected size is restored.
    BitReader reader(packedBytes, bitCount);
    DecodeNode* current = root;
    uint8_t bit = 0;

    while (reader.readBit(bit)) {
        current = bit ? current->one : current->zero;
        if (!current) {
            deleteDecodeTree(deleteDecodeTree, root);
            return false;
        }

        if (current->symbol >= 0) {
            output.push_back(static_cast<uint8_t>(current->symbol));
            if (output.size() == expectedOutputSize) break;
            current = root;
        }
    }

    deleteDecodeTree(deleteDecodeTree, root);
    return output.size() == expectedOutputSize;
}

array<LZ77Huffman::HuffmanCode, 256>
LZ77Huffman::buildCanonicalCodes(const array<uint8_t, 256>& codeLengths) {
    vector<std::pair<uint8_t, int>> symbols;

    // Canonical order: shorter lengths first, then smaller symbol values.
    for (int symbol = 0; symbol < 256; symbol++) {
        if (codeLengths[symbol] > 0) {
            symbols.push_back(std::make_pair(codeLengths[symbol], symbol));
        }
    }

    std::sort(symbols.begin(), symbols.end());

    array<HuffmanCode, 256> codes{};
    uint64_t currentCode = 0;
    uint8_t previousLength = 0;

    for (const auto& item : symbols) {
        uint8_t length = item.first;
        int symbol = item.second;

        // This version stores codes in uint64_t.
        // For ordinary byte Huffman tables this is usually enough.
        if (length > 63) {
            return codes;
        }

        currentCode <<= (length - previousLength);
        codes[symbol].bits = currentCode;
        codes[symbol].length = length;

        currentCode++;
        previousLength = length;
    }

    return codes;
}
