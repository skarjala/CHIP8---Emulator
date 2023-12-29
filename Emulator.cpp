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

    for(size_t i=0; i <= 0x65; i++) {
        tableF[i] = &Emulator::OP_NULL;
    }

    tableF[0x07] = &Emulator::OP_Fx07;
    tableF[0x0A] = &Emulator::OP_Fx0A;
    tableF[0x15] = &Emulator::OP_Fx15;
    tableF[0x18] = &Emulator::OP_Fx18;
    tableF[0x1E] = &Emulator::OP_Fx1E;
    tableF[0x29] = &Emulator::OP_Fx29;
    tableF[0x33] = &Emulator::OP_Fx33;
    tableF[0x55] = &Emulator::OP_Fx55;
    tableF[0x65] = &Emulator::OP_Fx65;

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

void Emulator::Table0() {
    ((*this).*(table0[opcode & 0x00Fu]))();
}
void Emulator::Table8() {
    ((*this).*(table8[opcode & 0x00Fu]))();
}
void Emulator::TableE() {
    ((*this).*(tableE[opcode & 0x000Fu]))();
}
void Emulator::TableF()
{
    ((*this).*(tableF[opcode & 0x00FFu]))();
}

void Emulator::OP_NULL() {

}
void Emulator::OP_00E0() {
    memset(video, 0, sizeof(video));
}
void Emulator::OP_00EE() {
    --sp;
    pc = stack[sp];
}
void Emulator::OP_1nnn(){
    uint16_t address = opcode & 0x0FFFu;
    pc = address;
}

void Emulator::OP_2nnn() {
    uint16_t address = opcode & 0x0FFFu;

    stack[sp] = pc;
    ++sp;
    pc = address;
}

void Emulator::OP_3xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if(registers[Vx] == byte) {
        pc += 2;
    }
}

void Emulator::OP_4xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte)
    {
        pc += 2;
    }
}

void Emulator::OP_5xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy])
    {
        pc += 2;
    }
}

void Emulator::OP_6xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

void Emulator::OP_7xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

void Emulator::OP_8xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

void Emulator::OP_8xy1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

void Emulator::OP_8xy2()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

void Emulator::OP_8xy3()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

void Emulator::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    if(sum > 255U) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] = sum & 0xFFu;
}

void Emulator::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] > registers[Vy]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}

void Emulator::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save LSB in VF
    registers[0xF] = (registers[Vx] & 0x1u);

    registers[Vx] >>= 1;
}

void Emulator::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vy] > registers[Vx]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] = registers[Vy] - registers[Vx];
}

void Emulator::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
    registers[Vx] <<= 1;
}

void Emulator::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] != registers[Vy]) {
        pc += 2;
    }
}

void Emulator::OP_Annn() {
    uint16_t address = opcode & 0x0FFFu;
    index = address;
}

void Emulator::OP_Bnnn()
{
    uint16_t address = opcode & 0x0FFFu;
    pc = registers[0] + address;
}

void Emulator::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

void Emulator::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    //wrap txt if going beyond screen boundaries
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;

    for(unsigned int row = 0; row < height; ++row) {
        uint8_t spriteByte = memory[index + row];
        for(unsigned int col = 0; col < 8; ++col) {
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

            //sprite pixel on
            if(spritePixel) {
                if(*screenPixel == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

void Emulator::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];
    
    if(keypad[key]) {
        pc += 2;
    }
}

void Emulator::OP_ExA1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (!keypad[key])
    {
        pc += 2;
    }
}

void Emulator::OP_Fx07()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

void Emulator::OP_Fx0A()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if (keypad[0])
    {
        registers[Vx] = 0;
    }
    else if (keypad[1])
    {
        registers[Vx] = 1;
    }
    else if (keypad[2])
    {
        registers[Vx] = 2;
    }
    else if (keypad[3])
    {
        registers[Vx] = 3;
    }
    else if (keypad[4])
    {
        registers[Vx] = 4;
    }
    else if (keypad[5])
    {
        registers[Vx] = 5;
    }
    else if (keypad[6])
    {
        registers[Vx] = 6;
    }
    else if (keypad[7])
    {
        registers[Vx] = 7;
    }
    else if (keypad[8])
    {
        registers[Vx] = 8;
    }
    else if (keypad[9])
    {
        registers[Vx] = 9;
    }
    else if (keypad[10])
    {
        registers[Vx] = 10;
    }
    else if (keypad[11])
    {
        registers[Vx] = 11;
    }
    else if (keypad[12])
    {
        registers[Vx] = 12;
    }
    else if (keypad[13])
    {
        registers[Vx] = 13;
    }
    else if (keypad[14])
    {
        registers[Vx] = 14;
    }
    else if (keypad[15])
    {
        registers[Vx] = 15;
    }
    else
    {
        pc -= 2;
    }
}

void Emulator::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    delayTimer = registers[Vx];
}

void Emulator::OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    soundTimer = registers[Vx];
}

void Emulator::OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    index += registers[Vx];
}

void Emulator::OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + (5* digit);
}

void Emulator::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    //ones place
    memory[index + 2] = value % 10;
    value /= 10;

    //tens place
    memory[index + 1] = value % 10;
    value /= 10;

    //hundreds place
    memory[index] = value % 10;
}

void Emulator::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for(uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
}

void Emulator::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for(uint8_t i=0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
}



