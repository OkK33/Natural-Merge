
#include <iostream>
#include <fstream>
#include <time.h>
#include <string>
using namespace std;

#define LIMIT 100
#define BUFFER_SIZE 10 //wielkosc bufora (liczac w ilosci rekordow)

int fileOperations = 0;

struct Record {
    float voltage;
    float current;
};

struct Tape {
    string fileName;
    Record* buffer;
    int recordIndex;
    int readIndex;
};

float randFloat()
{
    return (float)((rand()) / (float)(RAND_MAX)) * LIMIT;
}

void generateRecords(int number, string fileName) {

    ofstream records(fileName, ios::binary);

    for (int i = 0; i < number; i++) {
        float voltage = randFloat();
        float current = randFloat();

        records.write(reinterpret_cast<char*>(&voltage), sizeof(float));
        records.write(reinterpret_cast<char*>(&current), sizeof(float));
    }

    records.close();
}

void printRecords(string fileName) {

    ifstream records(fileName, ios::binary);
    Record record;
    bool isVoltage = true;
    bool isReady = false;
    float temp;
    while (records.read(reinterpret_cast<char*>(&temp), sizeof(float))) {
        if (isVoltage) {
            record.voltage = temp;
            isVoltage = false;
        }
        else {
            record.current = temp;
            isVoltage = true;
            isReady = true;
        }

        if (isReady) {
            printf("%f, %f, %f \n", record.voltage, record.current, record.voltage * record.current);
            isReady = false;
        }

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
    stream.close();
    return recordsRead;

}

void clearBuffer(Record buffer[]) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = { -1.0f, -1.0f };
    }
}


void emptyBuffer(Tape& tape) {
    ofstream stream(tape.fileName, ios::binary | ios::app);

    for (int i = 0; i < tape.recordIndex; i++) {
        stream.write((char*)(&tape.buffer[i]), sizeof(Record));
    }
    clearBuffer(tape.buffer);
    stream.close();
    fileOperations++;
    tape.recordIndex = 0;
}

Record getNextRecord(Tape& tape) {

    if (tape.recordIndex >= BUFFER_SIZE || tape.buffer[0].voltage == -1.0f) {
        clearBuffer(tape.buffer);
        int loadedRecords = loadRecordsToBuffer(tape.fileName, tape.buffer, tape.readIndex);
        tape.readIndex += loadedRecords*sizeof(Record);
        tape.recordIndex = 0;
    }
    tape.recordIndex += 1;
    return tape.buffer[tape.recordIndex-1];
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


void distributeToTapes(Tape& mainTape, Tape& tape1, Tape& tape2) {

    Record prevRec = { 0,0 };
    Record currRec = { 0,0 };
    bool putOnTape1 = true;

    while (true) {
        currRec = getNextRecord(mainTape);
        if (currRec.voltage != -1.0f) {
            if (currRec.voltage * currRec.current < prevRec.voltage * prevRec.current) {
                putOnTape1 = !putOnTape1;
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
}


int main()
{
    //NIE ZMIENIAĆ TEST2
    srand(time(NULL));
    std::cout << "Hello World!\n";
    string mainFileName = "test3.bin";
    string tape1FileName = "tape1.bin";
    string tape2FileName = "tape2.bin";

    generateRecords(25, mainFileName);
    printf("Main file: \n");
    printRecords(mainFileName);

    Record mainBuffer[BUFFER_SIZE] = {};
    Record tapeBuffer1[BUFFER_SIZE] = {};
    Record tapeBuffer2[BUFFER_SIZE] = {};

    Tape mainTape = { mainFileName, mainBuffer, 0, 0};
    Tape tape1 = { tape1FileName,tapeBuffer1, 0 , 0};
    Tape tape2 = { tape2FileName, tapeBuffer2, 0, 0 };



    clearBuffer(mainBuffer);
    clearBuffer(tapeBuffer1);
    clearBuffer(tapeBuffer2);

    ofstream file1(tape1FileName, std::ios::binary | std::ios::trunc);
    ofstream file2(tape2FileName, std::ios::binary | std::ios::trunc);
    file1.close();
    file2.close();

    distributeToTapes(mainTape, tape1, tape2);
    printf("Tape 1: \n");
    printRecords(tape1FileName);

    printf("Tape 2: \n");
    printRecords(tape2FileName);

    return 0;
}

