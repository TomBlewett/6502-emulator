#pragma once
#include <cstdint>

enum StatusFlag : uint8_t {
    FLAG_C = 1 << 0,   // Carry
    FLAG_Z = 1 << 1,   // Zero
    FLAG_I = 1 << 2,   // Interrupt
    FLAG_D = 1 << 3,   // Decimal
    FLAG_B = 1 << 4,   // Break
    FLAG_U = 1 << 5,   // Unused
    FLAG_V = 1 << 6,   // Overflow
    FLAG_N = 1 << 7,   // Negative
};

struct Bus {
    // Initialise all of the 64KB of RAM to 0x00
    uint8_t ram[65536] = {0};

    // Reads the 16-bit address, returns the byte at that address
    uint8_t read(uint16_t addr) { return ram[addr]; }

    // Writes the value byte to the 16-bit address in the RAM
    void write(uint16_t addr, uint8_t value) { ram[addr] = value; }
};

class CPU {
    public:
        CPU();
        ~CPU();
        Bus *bus;
        uint8_t returnRegister(char reg);
        uint16_t returnProgramCounter();
        int returnCycleCount();
        void printByte(uint8_t val);
        void printBytes(uint16_t val);
        void restart();
        void step();
        bool isHalted();
    private:
        uint64_t cycles;
        uint8_t  accumulator;
        uint8_t  x_reg;
        uint8_t  y_reg;
        uint8_t  sp;
        uint16_t pc;
        uint8_t  p_status;
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