#include <iostream>
#include <limits>
#include <string>

#include "huffman.h"
#include "LZ77.h"

using namespace std;

int promptInt(const string& prompt, int minValue, int maxValue);
string promptLine(const string& prompt);
string defaultOutputName(const string& inputFile, bool compress);
bool runOperation(int actionChoice, int methodChoice, const string& inputFile, string& outputFile);
void removeQuotations(string &fileName);

int main() {
    cout << "=======================================\n";
    cout << "         Basic File Compressor\n";
    cout << "=======================================\n";

    while (true) {
        cout << "\nMenu:\n";
        cout << "  1) Compress a file\n";
        cout << "  2) Decompress a file\n";
        cout << "  3) Help me\n";
        cout << "  4) Exit\n";

        int actionChoice = promptInt("Choose an option: ", 1, 4);

        if (actionChoice == 4) {
            cout << "Exiting program.\n";
            break;
        }

        if (actionChoice == 3) {
            cout << "IDK LOL\n";
        } else {
            cout << "\nCompression methods:\n";
            cout << "  1) Huffman\n";
            cout << "  2) LZ77\n";
            int methodChoice = promptInt("Choose a method: ", 1, 2);

            string inputFile = promptLine("\nEnter the input file name/path: ");

            cout << "\nEnter the output file name/path\n";
            cout << "(press Enter to use the default name): ";
            string outputFile;
            getline(cin, outputFile);
            if (outputFile.empty()) {
                outputFile = defaultOutputName(inputFile, actionChoice == 1);
            }

            cout << "\nRunning "
                    << ((actionChoice == 1) ? "compression" : "decompression")
                    << " using "
                    << ((methodChoice == 1) ? "Huffman" : "LZ77")
                    << "...\n";

            bool success = runOperation(actionChoice, methodChoice, inputFile, outputFile);

            if (success) {
                cout << "Operation completed successfully.\n";
                cout << "Output: " << outputFile << "\n";
            } else {
                cout << "Operation failed. Please verify the file path and selected method.\n";
            }
        }
    }

    return 0;
}

int promptInt(const string& prompt, int minValue, int maxValue) { // integer input for prompt
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }

        cout << "Invalid selection. Please enter a number from "
                  << minValue << " to " << maxValue << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

string promptLine(const string& prompt) {
    string value;
    while (true) {
        cout << prompt;
        getline(cin, value);
        if (!value.empty()) {
            if (value[0] == '"') removeQuotations(value);
            return value;
        }
        cout << "Input cannot be empty. Please try again.\n";
    }
}

string defaultOutputName(const string& inputFile, bool compress) {
    string fileName;

    // Gets rid of file type at the end of input file name
    for (int i = 0; i < inputFile.length(); i++) {
        if (inputFile[i] == '.') {
            fileName = inputFile.substr(0, i);
            break;
        }
    }

    return fileName;
}

bool runOperation(int actionChoice, int methodChoice, const string& inputFile, string& outputFile) {
    bool success = false;
    bool dotFound = false;

    for (int i = 0; i < outputFile.length(); i++) {
        if (outputFile[i] == '.') dotFound = true;
    }

    if (!dotFound) {
        if (actionChoice == 1)
            outputFile += ".bin";
        else
            outputFile += ".txt";
    }

    if (methodChoice == 1) {
        Huffman huffman;
        if (actionChoice == 1) {
            success = huffman.encodeFile(inputFile, outputFile);
        } else {
            success = huffman.decodeFile(inputFile, outputFile);
        }
    } else {
        LZ77 lz77;
        if (actionChoice == 1) {
            success = lz77.encodeFile(inputFile, outputFile);
        } else {
            success = lz77.decodeFile(inputFile, outputFile);
        }
    }

    return success;
}

void removeQuotations(string &fileName) {
    fileName = fileName.substr(1, fileName.length() - 2);
}