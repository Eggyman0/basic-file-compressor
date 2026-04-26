#include "Huffman.h"
#include <iostream>

using namespace std;

int main() {
    Huffman huff;
    if (huff.encodeFile("input.txt", "compressed.bin"))
        cout << "\nFile compressed\n";
    else
        cerr << "Error in file compression occured\n";

    return 0;
}