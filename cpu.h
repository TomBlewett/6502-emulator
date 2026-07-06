#pragma once
#include <cstdint>

enum StatusFlag : uint8_t {
    FLAG_C = 1 << 0,   // Carry
    FLAG_Z = 1 << 1,   // Zero
    FLAG_I = 1 << 2,   // Interrupt Disable
    FLAG_D = 1 << 3,   // Decimal Mode
    FLAG_B = 1 << 4,   // Break
    FLAG_U = 1 << 5,   // Unused
    FLAG_V = 1 << 6,   // Overflow
    FLAG_N = 1 << 7,   // Negative
};

struct Bus {
    uint8_t ram[65536] = {0};

    // Route address to correct device, return byte
    uint8_t read(uint16_t addr) { return ram[addr]; }

    // Route address to correct device, write byte
    void write(uint16_t addr, uint8_t value) { ram[addr] = value; }
};

class CPU {
    public:
        CPU();
        ~CPU();
        uint8_t returnRegister(char reg);
        uint16_t returnProgramCounter();
        int returnCycleCount();
        Bus *bus;
        void printByte(uint8_t val);
        void printBytes(uint16_t val);
        void restart();
        void step();
        bool isHalted();
    private:
        uint64_t cycles;
        uint8_t  a;
        uint8_t  x;
        uint8_t  y;
        uint8_t  sp;
        uint16_t pc;
        uint8_t  p;
        void nmi();
        void irq();
        void push(uint8_t value);
        void setFlag(StatusFlag flag, bool value);
        void loadRegister(uint8_t &reg, uint8_t value);
        void handleInterrupt(uint16_t vectorAddr, bool isBreak);
        bool getFlag(StatusFlag flag);
        bool halted;
        uint8_t pull();
        uint8_t fetch();

};