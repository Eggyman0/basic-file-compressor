#include "LZ77.h"
#include <iostream>

using namespace std;

int main() {
    LZ77 lz77;

    if (lz77.encodeFile("input.txt", "compressed.bin"))
        cout << "\nFile compressed successfully\n";
    else
        cerr << "Error in file compression occurred\n";

    if (lz77.decodeFile("compressed.bin", "output.txt"))
        cout << "\nFile decoded successfully\n";
    else
        cerr << "Error in file decoding occurred\n";

    return 0;
}