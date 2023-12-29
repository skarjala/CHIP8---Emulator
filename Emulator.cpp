//
// Created by Sandeep Karjala on 12/28/23.
//

#include "Emulator.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

using namespace std;

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

uint8_t fontset[FONTSET_SIZE] =
        {
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
Emulator::Emulator(): randGen(chrono::system_clock::now().time_since_epoch().count()){
    pc = START_ADDRESS;

    for(unsigned int i=0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    randByte = uniform_int_distribution<uint8_t>(0, 255U);

    //setting up function pointer table
    table[0x0] = &Emulator::Table0;
    table[0x1] = &Emulator::OP_1nnn;
    table[0x2] = &Emulator::OP_2nnn;
    table[0x3] = &Emulator::OP_3xkk;
    table[0x4] = &Emulator::OP_4xkk;
    table[0x5] = &Emulator::OP_5xy0;
    table[0x6] = &Emulator::OP_6xkk;
    table[0x7] = &Emulator::OP_7xkk;
    table[0x8] = &Emulator::Table8;
    table[0x9] = &Emulator::OP_9xy0;
    table[0xA] = &Emulator::OP_Annn;
    table[0xB] = &Emulator::OP_Bnnn;
    table[0xC] = &Emulator::OP_Cxkk;
    table[0xD] = &Emulator::OP_Dxyn;
    table[0xE] = &Emulator::TableE;
    table[0xF] = &Emulator::TableF;

    for(size_t i=0; i <= 0xE; i++) {
        table0[i] = &Emulator::OP_NULL;
        table8[i] = &Emulator::OP_NULL;
        tableE[i] = &Emulator::OP_NULL;
    }

    table0[0x0] = &Emulator::OP_00E0;
    table0[0xE] = &Emulator::OP_00EE;

    table8[0x0] = &Emulator::OP_8xy0;
    table8[0x1] = &Emulator::OP_8xy1;
    table8[0x2] = &Emulator::OP_8xy2;
    table8[0x3] = &Emulator::OP_8xy3;
    table8[0x4] = &Emulator::OP_8xy4;
    table8[0x5] = &Emulator::OP_8xy5;
    table8[0x6] = &Emulator::OP_8xy6;
    table8[0x7] = &Emulator::OP_8xy7;
    table8[0xE] = &Emulator::OP_8xyE;

    tableE[0x1] = &Emulator::OP_ExA1;
    tableE[0xE] = &Emulator::OP_Ex9E;
}

void Emulator::LoadROM(char const* filename) {
    ifstream file(filename, ios::binary | ios::ate);

    if(file.is_open()) {
        streampos size = file.tellg();
        char* buffer = new char[size];
        file.seekg(0, ios::beg);
        file.read(buffer, size);
        file.close();

        for(long i=0; i < size; ++i) {
            memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
    }
}

void Emulator::Cycle() {
    opcode = (memory[pc] << 8u) | memory[pc+1];

    //increment pc before excuting anything
    pc += 2;

    //decode and execute
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    //decrement delay timer if its been set
    if(delayTimer > 0) {
        --delayTimer;
    }

    //Decrement sound if its been set
    if(soundTimer > 0) {
        --soundTimer;
    }
}


