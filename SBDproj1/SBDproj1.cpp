#include <iostream>
#include <fstream>
#include <time.h>
#include <string>
using namespace std;

#define LIMIT 100
#define BUFFER_SIZE 32//wielkosc bufora (liczac w ilosci rekordow)

int fileOperations = 0;
int phases = 0;
int runs = 1;
int writes = 0;
int reads = 0;

struct Record {
    float voltage;
    float current;
};

struct Tape {
    string fileName;
    Record* buffer;
    int recordIndex; //ile rekordow w buforze
    int readIndex; //od ktorego bajtu czytac
};

float randFloat()
{
    return (float)((rand()) / (float)(RAND_MAX)) * LIMIT; // podzielenie przez siebie 2 liczb zmiennoprzecinkowych (wynik od 0 do 1) i pomnozenie przez limit
}

void generateRecords(int number, string fileName) {

    ofstream records(fileName, ios::binary);

    Record record = {};

    for (int i = 0; i < number; i++) {
        record.voltage = randFloat();
        record.current = randFloat();

        records.write((char*)(&record), sizeof(Record));

    }

    records.close();
}

void printRecords(string fileName) {

    ifstream records(fileName, ios::binary);
    Record record;

    float temp;
    while (records.read((char*)(&record), sizeof(Record))) {
        printf("%f, %f, %f \n", record.voltage, record.current, record.voltage * record.current);

    }
    records.close();
}

int loadRecordsToBuffer(string file, Record buffer[], int begAddr) {

    ifstream stream(file, ios::binary);
    Record tempRecord;
    stream.seekg(begAddr, ios::beg);
    int recordsRead = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (stream.read((char*)(&tempRecord), sizeof(Record))) {
            buffer[i] = tempRecord;
            recordsRead++;
        }
        else {
            break;
        }

    }
    fileOperations++;
    reads++;
    stream.close();
    return recordsRead;

}

void clearBuffer(Tape& tape) { //reset bufora do poczatkowych wartosci
    for (int i = 0; i < BUFFER_SIZE; i++) {
        tape.buffer[i] = { -1.0f, -1.0f };
    }
    tape.recordIndex = 0;
}

float countPower(Record record) {
    return record.voltage * record.current;
}

void emptyBuffer(Tape& tape) { //wyrzuc reszte rekordow z bufora do pliku i ustaw poczatkowe wartosci
    ofstream stream(tape.fileName, ios::binary | ios::app);

    for (int i = 0; i < tape.recordIndex; i++) {
        stream.write((char*)(&tape.buffer[i]), sizeof(Record));
    }
    clearBuffer(tape);
    stream.close();
    fileOperations++;
    writes++;
    tape.recordIndex = 0;
}

Record getNextRecord(Tape& tape) {

    if (tape.recordIndex >= BUFFER_SIZE || tape.buffer[0].voltage == -1.0f) {
        clearBuffer(tape);
        int loadedRecords = loadRecordsToBuffer(tape.fileName, tape.buffer, tape.readIndex);
        tape.readIndex += loadedRecords * sizeof(Record);
        tape.recordIndex = 0;
    }
    tape.recordIndex += 1;
    return tape.buffer[tape.recordIndex - 1];
}

void writeToTape(Tape& tape, Record& record) {

    if (tape.recordIndex < BUFFER_SIZE) {
        tape.buffer[tape.recordIndex] = record;
        tape.recordIndex++;
    }
    else {
        emptyBuffer(tape);
        tape.buffer[tape.recordIndex] = record;
        tape.recordIndex++;
    }
}


void distributeToTapes(Tape& mainTape, Tape& tape1, Tape& tape2, bool countRuns) {

    Record prevRec = { 0,0 };
    Record currRec = { 0,0 };
    bool putOnTape1 = true;

    while (true) {
        currRec = getNextRecord(mainTape);
        if (currRec.voltage != -1.0f) {
            if (countPower(currRec) < countPower(prevRec)) {
                putOnTape1 = !putOnTape1;
                if (countRuns) {
                    runs++;
                }
            }
            if (putOnTape1) {
                writeToTape(tape1, currRec);
            }
            else {
                writeToTape(tape2, currRec);
            }
            prevRec = currRec;
        }
        else {
            break;
        }
    }
    emptyBuffer(tape1);
    emptyBuffer(tape2);
    clearBuffer(mainTape);
    mainTape.readIndex = 0;

    ofstream file(mainTape.fileName, std::ios::binary | std::ios::trunc);
    file.close();
}

bool mergeTapes(Tape& mainTape, Tape& tape1, Tape& tape2) {

    Record currRec1 = { 0,0 };
    Record prevRec1 = { 0,0 };
    Record currRec2 = { 0,0 };
    Record prevRec2 = { 0,0 };
    Record prevRecMain = { 0,0 };
    bool takeFromTape1 = true;
    bool isSorted = true;

    currRec1 = getNextRecord(tape1);
    currRec2 = getNextRecord(tape2);

    while (true) {
        if (countPower(currRec1) < countPower(prevRec1) && countPower(currRec2) < countPower(prevRec2)) {
            prevRec1 = { 0, 0 };
            prevRec2 = { 0 , 0 }; //jeżeli obie serie się skonczyly, to "reset"
        }
        if (currRec1.voltage != -1.0f && ((countPower(currRec1) >= countPower(prevRec1) &&
            countPower(currRec2) >= countPower(prevRec2) && countPower(currRec1) < countPower(currRec2)) || //seria na 1 i 2 trwa dalej, ale rekord z 1 jest mniejszy
            (countPower(currRec1) >= countPower(prevRec1) && countPower(currRec2) < countPower(prevRec2)) || //seria na 1 trwa dalej, ale na 2 już nie
            (currRec2.voltage == -1.0f))) { //na 2 już nie ma rekordów

            writeToTape(mainTape, currRec1);
            prevRec1 = currRec1;
            if (countPower(currRec1) < countPower(prevRecMain)) {
                isSorted = false;
            }
            prevRecMain = currRec1;
            currRec1 = getNextRecord(tape1);


        }
        else {
            if (currRec2.voltage != -1.0f) {
                writeToTape(mainTape, currRec2);
                prevRec2 = currRec2;
                if (countPower(currRec2) < countPower(prevRecMain)) {
                    isSorted = false;
                }
                prevRecMain = currRec2;
                currRec2 = getNextRecord(tape2);
            }

        }

        if (currRec1.voltage == -1.0f && currRec2.voltage == -1.0f) {
            emptyBuffer(mainTape);
            clearBuffer(tape1);
            ofstream file1(tape1.fileName, std::ios::binary | std::ios::trunc);
            file1.close();
            tape1.readIndex = 0;
            clearBuffer(tape2);
            ofstream file2(tape2.fileName, std::ios::binary | std::ios::trunc);
            file2.close();
            tape2.readIndex = 0;
            break;
        }
    }

    phases++;
    return isSorted;
}


bool doPhase(Tape& mainTape, Tape& tape1, Tape& tape2, bool print, bool countRuns) {

    if (print) printf("Phase number %d: \n", phases);

    if (print) {
        printf("Main file: \n");
        printRecords(mainTape.fileName);
    }

    distributeToTapes(mainTape, tape1, tape2, countRuns);
    if (print) {
        printf("Tape 1: \n");
        printRecords(tape1.fileName);

        printf("Tape 2: \n");
        printRecords(tape2.fileName);
    }


    bool isSorted = mergeTapes(mainTape, tape1, tape2);

    if (print) {
        printf("Main file: \n");
        printRecords(mainTape.fileName);

        printf("Tape 1: \n");
        printRecords(tape1.fileName);

        printf("Tape 2: \n");
        printRecords(tape2.fileName);
    }




    return isSorted;
}

int countFileOperationsInTheory(int N) {
    return ((4 * N * ceil(log2(runs))) / BUFFER_SIZE);
}

int countPhasesInTheory() {
    return ceil(log2(runs));
}

int inputMode(string fileName) {
    ofstream records(fileName, ios::binary);
    int N = 0;
    float temp;
    printf("Enter the number of records: \n");
    cin >> N;
    printf("Enter the positive float numbers that will be in the file as \"voltage current\" \n");
    for (int i = 0; i < N; i++) {
        cin >> temp;
        records.write((char*)(&temp), sizeof(float));
        cin >> temp;
        records.write((char*)(&temp), sizeof(float));
    }
    printf("Input ended \n");
    records.close();

    return N;
}

void generateAndSort() {

    string temp;
    printf("Choose file name: \n");
    cin >> temp;
    string mainFileName = temp;
    string tape1FileName = "tape1.bin";
    string tape2FileName = "tape2.bin";

    Record mainBuffer[BUFFER_SIZE] = {};
    Record tapeBuffer1[BUFFER_SIZE] = {};
    Record tapeBuffer2[BUFFER_SIZE] = {};

    Tape mainTape = { mainFileName, mainBuffer, 0, 0 };
    Tape tape1 = { tape1FileName,tapeBuffer1, 0 , 0 };
    Tape tape2 = { tape2FileName, tapeBuffer2, 0, 0 };

    clearBuffer(mainTape);
    clearBuffer(tape1);
    clearBuffer(tape2);

    ofstream file1(tape1FileName, std::ios::binary | std::ios::trunc);
    ofstream file2(tape2FileName, std::ios::binary | std::ios::trunc);
    file1.close();
    file2.close();

    bool countRuns = true;

    string mode;
    string printMode;
    bool printAll = false;
    printf("Choose the mode: \n");
    printf("1. Random generated records: 1 \n");
    printf("2. Input from keyboard 2 \n");
    cin >> mode;
    printf("Choose the print mode: \n");
    printf("1. Print all info after each phase: 1 \n");
    printf("2. Print only info about the file before and after sorting: 2 \n");
    cin >> printMode;

    if (printMode == "1") {
        printAll = true;
    }
    else if (printMode == "2") {
        printAll = false;
    }
    if (mode == "1") {
        int N;
        printf("Enter the number of records: \n");
        cin >> N;
        printf("Started generating \n");
        generateRecords(N, mainFileName);
        printf("Ended generating \n");

        if (printAll == false) {
            printf("File before: \n");
            printRecords(mainFileName);
        }

        while (!doPhase(mainTape, tape1, tape2, printAll, countRuns)) {
            if (countRuns) {
                countRuns = false;
            }
        }

        if (printAll == false) {
            printf("File after: \n");
            printRecords(mainFileName);
        }

        printf("Size of buffer: %d records \n", BUFFER_SIZE);
        printf("Number of records: %d \n", N);
        printf("Number of runs: %d \n", runs);
        printf("Phases: %d \n", phases);
        printf("Expected phases: %d \n", countPhasesInTheory());
        printf("Reads: %d \n", reads);
        printf("Writes: %d \n", writes);
        printf("Total file operations: %d \n", fileOperations);
        printf("Expected file operations: %d \n", countFileOperationsInTheory(N));
    }
    else if (mode == "2") {
        int N = inputMode(mainFileName);
        while (!doPhase(mainTape, tape1, tape2, printAll, countRuns)) {
            if (countRuns) {
                countRuns = false;
            }
        }

        if (printAll == false)
        {
            printf("File after: \n");
            printRecords(mainFileName);
        }

        printf("Size of buffer: %d records \n", BUFFER_SIZE);
        printf("Number of records: %d \n", N);
        printf("Number of runs: %d \n", runs);
        printf("Phases: %d \n", phases);
        printf("Expected phases: %d \n", countPhasesInTheory());
        printf("Reads: %d \n", reads);
        printf("Writes: %d \n", writes);
        printf("Total file operations: %d \n", fileOperations);
        printf("Expected file operations: %d \n", countFileOperationsInTheory(N));
    }

    fileOperations = 0;
    phases = 0;
    runs = 1;
    reads = 0;
    writes = 0;
}

void generateFile() {
    string fileName;
    int N;
    string mode;
    printf("Choose file name: \n");
    cin >> fileName;
    printf("Choose the print mode: \n");
    printf("1. Print file after generating: 1\n");
    printf("2. Don't print file after generating: 2\n");
    cin >> mode;
    printf("Choose number of records to generate: \n");
    cin >> N;
    generateRecords(N, fileName);
    if (mode == "1")
    {
        printRecords(fileName);
    }


}

void sortFromFile() {
    string fileName;
    printf("Choose file name to sort: \n");
    cin >> fileName;
    string mainFileName = fileName;
    string tape1FileName = "tape1.bin";
    string tape2FileName = "tape2.bin";

    Record mainBuffer[BUFFER_SIZE] = {};
    Record tapeBuffer1[BUFFER_SIZE] = {};
    Record tapeBuffer2[BUFFER_SIZE] = {};

    Tape mainTape = { mainFileName, mainBuffer, 0, 0 };
    Tape tape1 = { tape1FileName,tapeBuffer1, 0 , 0 };
    Tape tape2 = { tape2FileName, tapeBuffer2, 0, 0 };

    clearBuffer(mainTape);
    clearBuffer(tape1);
    clearBuffer(tape2);

    ofstream file1(tape1FileName, std::ios::binary | std::ios::trunc);
    ofstream file2(tape2FileName, std::ios::binary | std::ios::trunc);
    file1.close();
    file2.close();

    bool countRuns = true;
    string printMode;
    bool printAll = false;
    printf("Choose the print mode: \n");
    printf("1. Print all info after each phase: 1 \n");
    printf("2. Print only info about the file before and after sorting: 2 \n");
    cin >> printMode;

    if (printMode == "1") {
        printAll = true;
    }
    else if (printMode == "2") {
        printAll = false;
    }


    if (printAll == false)
    {
        printf("File before: \n");
        printRecords(mainFileName);
    }
    while (!doPhase(mainTape, tape1, tape2, printAll, countRuns)) {
        if (countRuns) {
            countRuns = false;
        }
    }

    ifstream file(mainFileName, ios::binary);
    file.seekg(0, ios::end);
    int fileSize = file.tellg();
    int N = fileSize / sizeof(Record);
    file.close();

    if (printAll == false)
    {
        printf("File after: \n");
        printRecords(mainFileName);
    }
    printf("Size of buffer: %d records \n", BUFFER_SIZE);
    printf("Number of records: %d \n", N);
    printf("Number of runs: %d \n", runs);
    printf("Phases: %d \n", phases);
    printf("Expected phases: %d \n", countPhasesInTheory());
    printf("Reads: %d \n", reads);
    printf("Writes: %d \n", writes);
    printf("Total file operations: %d \n", fileOperations);
    printf("Expected file operations: %d \n", countFileOperationsInTheory(N));



    fileOperations = 0;
    phases = 0;
    runs = 1;
    reads = 0;
    writes = 0;
}

int main()
{
    srand(time(NULL));
    string mode;
    while (true) {
        printf("\n");
        printf("Choose what to do: \n");
        printf("1. Only generate a file with records: 1 \n");
        printf("2. Generate a file with records and sort it: 2 \n");
        printf("3. Sort a file without generating new records: 3 \n");
        printf("4. Exit the program: 4 \n");

        cin >> mode;
        if (mode == "4") {
            break;
        }
        else if (mode == "1") {
            generateFile();
        }
        else if (mode == "2") {
            generateAndSort();
        }
        else if (mode == "3") {
            sortFromFile();
        }
    }


    return 0;
}