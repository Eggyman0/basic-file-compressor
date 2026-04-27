#include "Huffman.h"
#include <iostream>

using namespace std;

int main() {
    Huffman huff;
    if (huff.encodeFile("input.txt", "compressed.bin"))
        cout << "\nFile compressed successfully\n";
    else
        cerr << "Error in file compression occured\n";

    if (huff.decodeFile("compressed.bin", "output.txt"))
        cout << "\nFile decoded successfully\n";
    else
        cerr << "Error in file decoding occured\n";
        
    return 0;
}