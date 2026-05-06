#include "huffman.h"
#include <fstream>
#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <cstdint>

Huffman::Huffman() 
{
}

Huffman::~Huffman() 
{
}

/**
 * @brief Get the next bit from input file stream
 * Funny placement
 * 
 * @param packedBytes 
 * @param index 
 * @return unsigned char 
 */
unsigned char getBit(std::vector<unsigned char> packedBytes, uint32_t index) {
    unsigned char byte = packedBytes[index / 8];
    int shift = 7 - static_cast<int>(index % 8);
    return (byte >> shift) & 1;
}

/**
 * @brief Compress input file size using Huffman coding algorithm
 * 
 * @param inputFile 
 * @param outputFile 
 * @return true File compressing successful
 * @return false File compressing failed
 */
bool Huffman::encodeFile(const std::string &inputFile, std::string &outputFile) {
    Node* root = 0;
    std::string inputData;

    int freqTable[256] = {0};
    encoding codeTable[256] = {0};
    int tableSize = sizeof(codeTable) / sizeof(codeTable[0]);

    if (readFile(inputFile, inputData) == false) {
        std::cerr << "Failed to read input file: " << inputFile << "\n";
    }

    buildFreqTable(freqTable, inputData);

    root = createHuffmanTree(freqTable, tableSize);
    generateCodeLengths(root, 0, codeTable);
    // recursiveTreePrint(root, 0);

    buildCodeTable(codeTable, tableSize);
    rearrange(codeTable, tableSize);

    std::vector<unsigned char> packedBytes = bytePack(codeTable, tableSize, inputData);

    // test print to make sure table is generated properly
    // std::cout << std::endl;
    // for (int i = 0; i < tableSize; i++) {
    //     std::cout << i << ") " << codeTable[i].ch<<"  "<<codeTable[i].code<<"  "<<codeTable[i].length<<"\n";
    // }

    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        clearHuffmanTree(root);
        return false;
    }

    // Header: only information on encoding bit length, in the order of ASCII table
    for (int i = 0; i < tableSize; i++) {
        out.write(reinterpret_cast<const char*>(&codeTable[i].length), sizeof(codeTable[0].length));
    }

    // Header: number of valid bits in encoded file
    // Calculating total number of encoded bits
    int encodedBits = 0;
    for (int i = 0; i < tableSize; i++) {
        encodedBits += codeTable[i].length * freqTable[codeTable[i].ch];
    }
    uint32_t bitCount = static_cast<uint32_t>(encodedBits);
    out.write(reinterpret_cast<const char*>(&bitCount), sizeof(bitCount));

    // Output packed bytes of encoded data
    if (!packedBytes.empty()) {
        out.write(reinterpret_cast<const char*>(packedBytes.data()),
                  static_cast<std::streamsize>(packedBytes.size()));
    } else {
        std::cout << "\nWARNING: Byte vector is empty\n";
        clearHuffmanTree(root);
        return false;
    }

    clearHuffmanTree(root);
    return true;
}

/**
 * @brief Rebuilds canonical Huffman table out of pre-defined bit lengths in input file header.
 * Encoded bits are then compared to the values from the rebuilt table.
 * 
 * @param inputFile 
 * @param outputFile 
 * @return true Success
 * @return false Failed to decode
 */
bool Huffman::decodeFile(const std::string &inputFile, std::string &outputFile) {
    std::ifstream in(inputFile, std::ios::binary);
    if (!in) std::cerr << "Error opening compressed file\n";

    // Read array of bit length from header
    encoding codeTable[256];
    const int maxLength = 255;
    int totalCount[maxLength + 1] = {0};  // Contains amount of times each bit length appears
    int tableSize = sizeof(codeTable) / sizeof(codeTable[0]);
    int lengthFromFile;

    for (int i = 0; i < tableSize; i++) {
        in.read(reinterpret_cast<char*>(&lengthFromFile), sizeof(lengthFromFile));
        codeTable[i].length = lengthFromFile;
        codeTable[i].ch = i;
        
        if (lengthFromFile > 0 && lengthFromFile < maxLength) totalCount[lengthFromFile]++;
    }

    buildCodeTable(codeTable, tableSize);

    // Read bit count from header
    uint32_t bitCount = 0;
    in.read(reinterpret_cast<char*>(&bitCount), sizeof(bitCount));
    if (!in) std::cerr << "Failed to read bit count from header" << std::endl;

    uint32_t byteCount = (bitCount + 7) / 8;

    // Read binary file into a vector; each element holds 1 byte
    std::vector<unsigned char> packedBytes;
    char byte = 0;
    for (int i = 0; i < byteCount; i++) {
        in.get(byte);
        packedBytes.push_back(static_cast<unsigned char>(byte));
    }

    std::ofstream out(outputFile, std::ios::binary);
    if (!out) std::cerr << "Failed to output decompressed file";

    std::string decoded;            // contains output text
    uint32_t bitIndex = 0;          // amount of bits processed from input file

    while (bitIndex < bitCount) {
        int currentCode = 0;    // current Huffman code being analysed
        int first = 0;          // value of first code within a group of a certain bit length
        int tableIndex = 0;     // keeps track of current index of canonical table

        for (int len = 1; len <= maxLength; len++) {
            unsigned char bit = getBit(packedBytes, bitIndex);
            int count = totalCount[len];

            bitIndex++;
            currentCode |= bit;

            if (currentCode - totalCount[len] < first) {
                decoded += codeTable[tableIndex + (currentCode - first)].ch;
                break;
            } else {
                tableIndex += count;
                first = (first + count) << 1;
                currentCode <<= 1;
            }
        }
        currentCode = 0;
        first = 0;
        tableIndex = 0;
    }

    out.write(decoded.c_str(), static_cast<std::streamsize>(decoded.size()));

    return true;
}

/**
 * @brief Creates binary Huffman tree using greedy algorithm
 * Each leaf node contains information on character and frequency
 * Non-leaf nodes contain sum of child frequencies
 * 
 * @param fTable Array containing frequencies; index of array corresponds to ASCII index
 * @return Huffman::Node* Pointer to root node
 */
Huffman::Node* Huffman::createHuffmanTree(int freqTable[], int size) {
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> pq;

    for (int i = 0; i < size; i++) {
        if (freqTable[i] > 0) {
            Node* leaf = new Node(static_cast<unsigned char>(i), freqTable[i]);
            pq.push(leaf);
        }
    }

    if (pq.empty()) return nullptr;
    if (pq.size() == 1) return pq.top();

    while (pq.size() > 1) {
        Node* left = pq.top();
        pq.pop();
        Node* right = pq.top();
        pq.pop();
        
        // create new parent node containing sum of child frequencies
        Node* parent = new Node((left->freq + right->freq), left, right);
        pq.push(parent);
    }

    return pq.top();
}

/**
 * @brief Clears Huffman tree by removing nodes starting from root,
 * deleting from left to right, then top to bottom
 * 
 * @param root Node pointer to the root of the binary tree you want removed
 */
void Huffman::clearHuffmanTree(Node* root) {
    if (root == nullptr) return;

    std::queue<Node*> q;
    q.push(root);

    while (!q.empty()) {
        Node* current = q.front();
        q.pop();

        if (current->left != nullptr) {
            q.push(current->left);
        }
        if (current->right != nullptr) {
            q.push(current->right);
        }

        delete current;
    }

    root = nullptr;
}

/**
 * @brief Takes in a file name and puts file content in a string
 * 
 * @param inFile input file name as a string
 * @param data string that contains file content
 * @return true  Data read successful
 * @return false Data read failed
 */
bool Huffman::readFile(const std::string &inFile, std::string &data) {
    std::ifstream in(inFile, std::ios::binary);
    if (!in) return false;
    data.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return true;
}

/**
 * @brief Populate array with frequency of each character
 * Note: Each index of array corresponds to the equivalent ASCII index
 * 
 * @param table  Empty array for storing frequency amount
 * @param data Input data string
 */
void Huffman::buildFreqTable(int table[], const std::string data) {
    for (int i = 0; i < data.size(); i++) {
        unsigned char c = data[i];
        table[c]++;
    }
}

/**
 * @brief Recursive tree traversal using BFS and recording encoding length as the depth of the tree
 * Note: In Huffman code, the length of the encoding corresponds to the depth that the character node is found
 * 
 * @param n Node Pointer
 * @param depth Current level in tree structure
 * @param table
 */
void Huffman::generateCodeLengths(Node* n, int depth, encoding table[]) {
    if (n == nullptr) return;

    if (n->isLeaf()) {
        table[n->ch].ch = n->ch;
        table[n->ch].length = depth;
        return;
    }

    depth++;
    generateCodeLengths(n->left, depth, table);
    generateCodeLengths(n->right, depth, table);
}

/**
 * @brief Builds canonical Huffman code table by sorting elements by bit length
 * At the end of function, rearranges array so that eachc index corresponds to ASCII table equivalent
 * 
 * @param codeTable 
 * @param size 
 */
void Huffman::buildCodeTable(encoding codeTable[], int size) {
    // Sort encoding by bit length; uses bubble sort
    // Characters are already in 'alphabetical' order, due to our array indexes corresponding to the ASCII table
    int numCompares = size - 1;
    while (numCompares > 0) {
        int last = 0;
        for (int i = 0; i < numCompares; i++) {
            int lower = codeTable[i].length;
            int upper = codeTable[i+1].length;
            if ((lower!=0 && upper!=0 && lower > upper)
                || (lower == 0 && upper != 0)) {
                std::swap(codeTable[i], codeTable[i+1]);
                last = i + 1;
            }
        }
        numCompares = last - 1;
    }

    // Starting from zero, the value for each encoding element gets incremented by 1
    // If the next element has higher bit length, shift bits to the left (pad zero on the right)
    // Note: Encoding bit length HAS to equal pre-determined length obtained from the tree
    // Result of numerical operations are then turned into a string (will be used to output)
    int binary = 0;

    for (int i = 0; codeTable[i].length != 0; i++) {
        codeTable[i].code = binaryToString(binary, codeTable[i].length);
        binary++;
        if (codeTable[i].length < codeTable[i + 1].length) {
            binary <<= 1; // left shift when next element's length is higher
        }
    }
}

/**
 * @brief "Sorts" canonical table by ASCII order
 * It could've been more cleaner but I'm afraid it's too late to change
 * 
 * @param table 
 * @param size 
 */
void Huffman::rearrange(encoding table[], int size) {
    // Rearranging table
    encoding temp[256] = {0};
    for (int i = 0; i < size; i++)
        temp[table[i].ch] = table[i];
    for (int i = 0; i < size; i++)
        table[i] = temp[i];
}

/**
 * @brief Converts binary int to a string based on desired length
 * 
 * @param bin Binary
 * @param length 
 * @return std::string 
 */
std::string Huffman::binaryToString(int bin, int length) {
    std::string result(length, '0'); // create string of 0's of encoding length
        for (int i = length - 1; i >= 0; i--) {
            if (bin & 1) {       // bit mask with right-most bit and check if it is 1
                result[i] = '1'; // replace specific element on string with 1
            }
            bin >>= 1;           // right shift 'binary' to change right-most bit
        }
    return result;
}

/**
 * @brief Encodes input string by converting based on code table
 * Then packs bits in groups of eight (bytes); pads zeros if last byte is not 8 bits long
 * 
 * @param table 
 * @param data 
 * @return std::vector<unsigned char> 
 */
std::vector<unsigned char> Huffman::bytePack(encoding table[], int size, std::string &data) {
    unsigned char currentByte = 0;
    int bitIndex = 0;
    std::string bits;
    std::vector<unsigned char> bytes;

    for (unsigned char c : data)
    {
        // Search table for matching character index
        for (int i = 0; i < size; i++) {
            if (table[i].ch == c) {
                bits += table[i].code;
                break;
            }
        }
    }

    data = bits;

    for (char bit : data)
    {
        currentByte <<= 1;
        if (bit == '1') {
            currentByte |= 1;
        }

        ++bitIndex;

        if (bitIndex == 8) {
            bytes.push_back(currentByte);
            currentByte = 0;
            bitIndex = 0;
        }
    }

    // Pad remaining bits on the right
    if (bitIndex != 0) {
        currentByte <<= (8 - bitIndex);
        bytes.push_back(currentByte);
    }

    return bytes;
}

// *** Test functions ***
// Super, incredibly scuffed way of printing a binary tree
// Root of the tree starts from the left-most column in terminal
void Huffman::recursiveTreePrint(Node* n, int index) {
    if (n->right) {
        recursiveTreePrint(n->right, index + 1);
    }

    std::cout << std::endl;
    for (int i = 0; i < index-1; i++) {
        std::cout << "      ";
    }
    if (index) {
        std::cout << "----- ";
    }

    if (n->isLeaf()) {
        if (n->ch == 10)
            std::cout << "\\n" <<"("<<n->freq<<")";
        else if(n->ch == 13)
            std::cout << "\\r" <<"("<<n->freq<<")";
        else
            std::cout << n->ch <<"("<<n->freq<<")";
    }
    else {
        std::cout << n->freq;
    }

    if (n->left) {
        recursiveTreePrint(n->left, index + 1);
    }
}