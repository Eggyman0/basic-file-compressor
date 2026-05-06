#include <iostream>
#include <limits>
#include <string>

#include "huffman.h"
#include "LZ77.h"
#include "LZ77Huffman.h"

using namespace std;

int promptInt(const string& prompt, int minValue, int maxValue);
string promptLine(const string& prompt);
string defaultOutputName(const string& inputFile, bool compress);
bool runOperation(int actionChoice, int methodChoice, int typeChoice, const string& inputFile, string& outputFile);
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
            cout << "\nThis program can compress/decompress text/csv files using three methods:\n"
                 << "  - Huffman Coding\n  - LZ77\n  - A hybrid algorithm that combines both\n"
                 << "\nThe user can input file names either by entering the file name (1) if it\n"
                 << "is in the same folder, or by pasting its direct path (2) into the terminal.\n"
                 << "  1) input.txt\n  2) C:\\misc_folder\\another_folder\\input.txt\n"
                 << "The same also applies for inputting the file name/path for your output file.\n"
                 << "NOTE: If only a file name is entered for the output file, the file will\n"
                 << "be automatically placed in the same location the input file originated from.\n"
                 << "\nIf the program gets stuck, press 'CTRL + C' in order to close the terminal.\n";

            cout << "\nPress enter to continue: ";
            string temp;
            getline(cin, temp);
        } else {
            int typeChoice = 0;
            if (actionChoice == 2) {
                cout << "\nOutput file type:\n";
                cout << "  1) .txt\n";
                cout << "  2) .csv\n";
                typeChoice = promptInt("Choose a file type: ", 1, 2);
            }
            cout << "\nMethod:\n";
            cout << "  1) Huffman\n";
            cout << "  2) LZ77\n";
            cout << "  3) Huffman/LZ77\n";
            int methodChoice = promptInt("Choose a method: ", 1, 3);

            string inputFile = promptLine("\nEnter the input file name/path: ");

            // Check for file location
            int lastSlash;
            for (int i = 0; i < inputFile.length(); i++) {
                if (inputFile[i] == '\\') lastSlash = i;
            }
            string filePath = inputFile.substr(0, lastSlash + 1);

            cout << "\nEnter the output file name/path\n";
            cout << "(press Enter to use the default name): ";
            string outputFile;
            getline(cin, outputFile);
            if (outputFile.empty()) {
                outputFile = defaultOutputName(inputFile, actionChoice == 1);
            } else {
                lastSlash = 0;
                for (int i = 0; i < outputFile.length(); i++) {
                    if (outputFile[i] == '\\') lastSlash = i;
                }
                if (lastSlash == 0) outputFile = filePath + outputFile;
            }

            cout << "\nRunning "
                 << ((actionChoice == 1) ? "compression" : "decompression")
                 << " using "
                 << ((methodChoice == 1) ? "Huffman" : ((methodChoice == 2) ? "LZ77" : "Huffman/LZ77"))
                 << "...\n";

            bool success = runOperation(actionChoice, methodChoice, typeChoice, inputFile, outputFile);

            if (success) {
                cout << "Operation completed successfully.\n";
                cout << "Output: " << outputFile << "\n";
                cout << "Press enter to continue: ";
                string temp;
                getline(cin, temp);
            } else {
                cout << "Operation failed. Please verify the file path and selected method.\n";
            }
        }
        system("cls");
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

bool runOperation(int actionChoice, int methodChoice, int typeChoice, const string& inputFile, string& outputFile) {
    bool success = false;
    bool dotFound = false;

    for (int i = 0; i < outputFile.length(); i++) {
        if (outputFile[i] == '.') dotFound = true;
    }

    if (!dotFound) {
        if (actionChoice == 1) {
            switch (methodChoice) {
            case 1:
                outputFile += ".huf";
                break;
            case 2:
                outputFile += ".lz";
                break;
            case 3:
                outputFile += ".lzh";
                break;
            }
        } else {
            switch (typeChoice) {
            case 1:
                outputFile += ".txt";
                break;
            case 2:
                outputFile += ".csv";
                break;
            }
        }
    }

    Huffman huffman;
    switch (methodChoice) {
    case 1:
        if (actionChoice == 1) {
            success = huffman.encodeFile(inputFile, outputFile);
        } else {
            success = huffman.decodeFile(inputFile, outputFile);
        }
        break;
    case 2:
        LZ77 lz77;
        if (actionChoice == 1) {
            success = lz77.encodeFile(inputFile, outputFile);
        } else {
            success = lz77.decodeFile(inputFile, outputFile);
        }
        break;
    case 3:
        LZ77Huffman lzh;
        if (actionChoice == 1) {
            success = lzh.encodeFile(inputFile, outputFile);
        } else {
            success = lzh.decodeFile(inputFile, outputFile);
        }
        break;
    }

    return success;
}

void removeQuotations(string &fileName) {
    fileName = fileName.substr(1, fileName.length() - 2);
}