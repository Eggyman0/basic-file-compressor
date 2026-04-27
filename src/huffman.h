/* Header for Huffman coding implementation
 * Uses a canonical huffman code approach to encode and decode text files
 */

#ifndef HUFFMAN
#define HUFFMAN

#include <string>
#include <queue>
#include <vector>

typedef unsigned char ElementType;

class Huffman {
private:
    struct Node {
        ElementType ch; // character
        int freq;
        Node *left, *right;

        // Only used for printing Huffman tree at the moment
        bool isLeaf() const {
            return left == nullptr && right == nullptr;
        }

        // *** Node Constructors ***
        // Node containing characters and freqeuncy (only leaves)
        Node(ElementType val, int freqIn) : 
            ch(val), freq(freqIn), left(0), right(0) {}

        // Node containing sum of child frequencies
        Node(int freqIn, Node* leftChild, Node* rightChild) :
            ch(0), freq(freqIn), left(leftChild), right(rightChild) {}
    };

    // Functor needed for std::priority_queue
    struct CompareNode {
        bool operator()(Node* a, Node* b) const {
            // returns a > b for a min-heap; lowest value is at the top (front) of queue
            return a->freq > b->freq;
        }
    };

    // Struct for containing encoded information
    struct encoding {
        unsigned char ch = 0;
        int length = 0;
        std::string code;
    };

    // For encoding (some are used for decoding as well)
    Node* createHuffmanTree(int freqTable[], int size);
    void clearHuffmanTree(Node* root);
    bool readFile(const std::string &inFile, std::string &inData);
    void buildFreqTable(int table[], const std::string inData);
    void generateCodeLengths(Node* n, int depth, encoding table[]);
    void buildCodeTable(encoding codeTable[], int size);
    std::string binaryToString(int bin, int length);
    std::vector<unsigned char> bytePack(encoding table[], int size, std::string &data);

    // For decoding
    std::string byteUnpack(std::vector<unsigned char> v, unsigned int validBits);

    // test functions
    void recursiveTreePrint(Node* n, int index);
public:
    Huffman();
    ~Huffman();
    
    bool encodeFile(const std::string &inputFile, const std::string &outputFile); // const used so you don't accidentally alter file names
    bool decodeFile(const std::string &inputFile, const std::string &outputFile); // WIP
};

#endif