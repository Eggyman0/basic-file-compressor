#include "Huffman.h"
#include <iostream>

using namespace std;
void choose(int choice, Huffman huff);

int main() {
    Huffman huff;
    char yesno;
    int choice;

    cout << "Would you like to encode or decode a file? (Y/N)\n";
    cin >> yesno;
    if (yesno == 'Y' || yesno == 'y') {
        cout << "1) Encode File\n2) Decode File\n";
        cin >> choice;
        choose(choice, huff);
    }
    
    return 0;
}

void choose (int choice, Huffman huff) {
    string file;
    switch (choice) {
    case 1:
        cout << "Enter input text file (.txt)\n";
        cin >> file;
        if (huff.encodeFile(file, "compressed.bin")) 
            cout << "\nFile compressed successfully\n";
        else
            cerr << "Error in file compression occured\n";
        break;

    case 2:
        cout << "Enter name of compressed file:\n";
        cin >> file;
        if (huff.decodeFile(file, "output.txt"))
            cout << "\nFile decoded successfully\n";
        else
            cerr << "Error in file decoding occured\n";
        
        break;

    default:
        
        break;
    }
}