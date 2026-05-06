#ifndef LZ77_HUFFMAN_H
#define LZ77_HUFFMAN_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// LZ77: replaces repeated byte sequences with (offset, length) matches.
// Huffman coding: compresses the byte stream produced by LZ77.
//
// The compressed output is a custom binary format, not .zip/.gz compatible.
class LZ77Huffman {
public:
    // Compress inputPath and write the custom compressed file to outputPath.
    bool encodeFile(const std::string& inputPath, const std::string& outputPath);

    // Decompress a file created by encodeFile().
    bool decodeFile(const std::string& inputPath, const std::string& outputPath);

private:
    // Tunable LZ77 constants. These are intentionally simple and readable.
    static const size_t WINDOW_SIZE = 32768;
    static const size_t MIN_MATCH_LENGTH = 3;
    static const size_t MAX_MATCH_LENGTH = 258;
    static const size_t MAX_LITERAL_LENGTH = 65535;
    static const size_t MAX_HASH_CANDIDATES = 128;

    struct HuffmanCode {
        uint64_t bits = 0;
        uint8_t length = 0;
    };

    // File helpers.
    bool readFile(const std::string& path, std::vector<uint8_t>& data);
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data);

    // LZ77 stage.
    std::vector<uint8_t> lz77Compress(const std::vector<uint8_t>& input);
    bool lz77Decompress(const std::vector<uint8_t>& tokens,
                        uint32_t originalSize,
                        std::vector<uint8_t>& output);

    // Huffman stage.
    std::vector<uint8_t> huffmanCompress(const std::vector<uint8_t>& input,
                                         std::array<uint8_t, 256>& codeLengths,
                                         uint64_t& bitCount);
    bool huffmanDecompress(const std::vector<uint8_t>& packedBytes,
                           const std::array<uint8_t, 256>& codeLengths,
                           uint64_t bitCount,
                           uint32_t expectedOutputSize,
                           std::vector<uint8_t>& output);

    // Canonical Huffman helper.
    std::array<HuffmanCode, 256> buildCanonicalCodes(const std::array<uint8_t, 256>& codeLengths);
};

#endif
