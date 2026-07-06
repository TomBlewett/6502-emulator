#include <cstdint>
#include <iostream>
#include "cpu.h"



CPU::CPU() : cycles(0), a(0), x(0), y(0), sp(0xFF), pc(0x0000), p(0x00), halted(false) {
    bus = new Bus();
}

CPU::~CPU()
{
    delete bus;
}

// Is the CPU running
bool CPU::isHalted() { return halted; }

// Return the 8-bit value of any 8-bit register in the CPU
uint8_t CPU::returnRegister(char reg) {
    switch (reg) {
        case 'a' | 'A': {
            return a;
        } break;
        case 'x' | 'X': {
            return x;
        } break;
        case 'y' | 'Y': {
            return y;
        } break;
        case 'sp' | 'SP': {
            return sp;
        } break;
        case 'p' | 'P': {
            return p;
        } break;
        default: {
            return 0x00;
        } break;
    }
    return 0x00;
}

uint16_t CPU::returnProgramCounter() {
    return pc;
}

int CPU::returnCycleCount() {
    return cycles;
}

// Push byte onto stack
void CPU::push(uint8_t value) { bus->write(0x0100 + sp, value); sp--; }

// Pull byte from stack
uint8_t CPU::pull() { sp++; return bus->read(0x0100 + sp); }

// Set or clear a status flag
void CPU::setFlag(StatusFlag flag, bool value) {
    if (value) p |= flag;
    else       p &= ~flag;
}

// Reads a status flag
bool CPU::getFlag(StatusFlag flag) { return (p & flag) != 0; }

// Print bits of any byte
void CPU::printByte(uint8_t val) {
    std::string s = "";
    for (int i = 7; i >= 0; i--) s += (char)('0' + ((val >> i) & 1));
    std::cout << "0b" << s << std::endl;
}

// Print bits of any 16-bit/2-byte address in RAM
void CPU::printBytes(uint16_t val) {
    std::string byte_1 = "";
    std::string byte_2 = "";
    for (int i = 15; i >= 8; i--) byte_2 += (char)('0' + ((val >> i) & 1));
    for (int i = 7; i >= 0; i--) byte_1 += (char)('0' + ((val >> i) & 1));
    std::cout << "0b" << byte_2 << '|' << byte_1 << std::endl;
}

// Fetches the next byte at the PC, advances the PC
uint8_t CPU::fetch() { return bus->read(pc++); }

// Loads values into a register, updates N/Z flags
void CPU::loadRegister(uint8_t& reg, uint8_t value) {
    reg = value;
    setFlag(FLAG_Z, reg == 0);
    setFlag(FLAG_N, reg & FLAG_N);
}

// Push PC and P to the stack, jumps to the interrupt vector address
void CPU::handleInterrupt(uint16_t vectorAddr, bool isBreak) {
    push((pc >> 8) & 0xFF);
    push(pc & 0xFF);
    push(p | FLAG_U | (isBreak ? FLAG_B : 0));
    setFlag(FLAG_I, true);
    uint8_t lo = bus->read(vectorAddr);
    uint8_t hi = bus->read(vectorAddr + 1);
    pc = (hi << 8) | lo;
}

// Restart the CPU state, jumps to restart vector
void CPU::restart() {
    a = 0; x = 0; y = 0;
    sp = 0xFF;
    p  = FLAG_U;
    uint8_t lo = bus->read(0xFFFC);
    uint8_t hi = bus->read(0xFFFD);
    pc = (hi << 8) | lo;
}

// Trigger non-maskable interrupt
void CPU::nmi() { handleInterrupt(0xFFFA, false); }

// Trigger maskable interrupt if FLAG_I clear
void CPU::irq() { if (!getFlag(FLAG_I)) handleInterrupt(0xFFFE, false); }

// Advances the CPU through one entire instruction given an Opcode assuming Operands follow
void CPU::step() {

    if (halted) return;
    
    uint8_t opcode = fetch();

    switch (opcode) {
        case 0x00: {                                                          // BRK
            pc++;
            handleInterrupt(0xFFFE, true);
            cycles += 7;
        } break;
        case 0x02: {              // STP halt signal
            halted = true;
        } break;
        case 0x08: {                                                          // PHP
            push(p | FLAG_B | FLAG_U);
            cycles += 3;
        } break;
        case 0x09: { loadRegister(a, a | fetch()); cycles += 2; } break;     // ORA Immediate
        case 0x10: {                                                          // BPL
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_N)) pc += o;
            cycles += 2;
        } break;
        case 0x18: { setFlag(FLAG_C, false); cycles += 2; } break;           // CLC
        case 0x20: {                                                          // JSR
            uint8_t lo = fetch(); uint8_t hi = fetch();
            push((pc - 1) >> 8);
            push((pc - 1) & 0xFF);
            pc = (hi << 8) | lo;
            cycles += 6;
        } break;
        case 0x28: {                                                          // PLP
            p = pull();
            setFlag(FLAG_U, true);
            cycles += 4;
        } break;
        case 0x29: { loadRegister(a, a & fetch()); cycles += 2; } break;     // AND Immediate
        case 0x30: {                                                          // BMI
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_N)) pc += o;
            cycles += 2;
        } break;
        case 0x38: { setFlag(FLAG_C, true); cycles += 2; } break;            // SEC
        case 0x40: {                                                          // RTI
            p = pull();
            setFlag(FLAG_U, true);
            uint8_t lo = pull(); uint8_t hi = pull();
            pc = (hi << 8) | lo;
            cycles += 6;
        } break;
        case 0x48: { push(a); cycles += 3; } break;                          // PHA
        case 0x49: { loadRegister(a, a ^ fetch()); cycles += 2; } break;     // EOR Immediate
        case 0x4C: {                                                          // JMP Absolute
            uint8_t lo = fetch(); uint8_t hi = fetch();
            pc = (hi << 8) | lo;
            cycles += 3;
        } break;
        case 0x50: {                                                          // BVC
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_V)) pc += o;
            cycles += 2;
        } break;
        case 0x60: {                                                          // RTS
            uint8_t lo = pull(); uint8_t hi = pull();
            pc = ((hi << 8) | lo) + 1;
            cycles += 6;
        } break;
        case 0x68: { loadRegister(a, pull()); cycles += 4; } break;          // PLA
        case 0x69: {                                                          // ADC Immediate
            uint8_t value = fetch();
            uint16_t result = (uint16_t)a + value + getFlag(FLAG_C);
            setFlag(FLAG_C, result > 0xFF);
            setFlag(FLAG_Z, (result & 0xFF) == 0);
            setFlag(FLAG_N, result & FLAG_N);
            setFlag(FLAG_V, (~(a ^ value) & (a ^ result) & 0x80));
            a = (uint8_t)result;
            cycles += 2;
        } break;
        case 0x6C: {                                                          // JMP Indirect
            uint8_t lo = fetch(); uint8_t hi = fetch();
            uint16_t ptr = (hi << 8) | lo;
            pc = bus->read(ptr) | (bus->read(ptr + 1) << 8);
            cycles += 5;
        } break;
        case 0x70: {                                                          // BVS
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_V)) pc += o;
            cycles += 2;
        } break;
        case 0x85: {                                                          // STA Zero Page
            bus->write(fetch(), a);
            cycles += 3;
        } break;
        case 0x8D: {                                                          // STA Absolute
            uint8_t lo = fetch(); uint8_t hi = fetch();
            bus->write((hi << 8) | lo, a);
            cycles += 4;
        } break;
        case 0x90: {                                                          // BCC
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_C)) pc += o;
            cycles += 2;
        } break;
        case 0x9D: {                                                          // STA Absolute,X
            uint8_t lo = fetch(); uint8_t hi = fetch();
            bus->write(((hi << 8) | lo) + x, a);
            cycles += 5;
        } break;
        case 0xA1: {                                                          // LDA (Zero Page,X)
            uint8_t ptr = fetch() + x;
            uint8_t lo  = bus->read(ptr);
            uint8_t hi  = bus->read((uint8_t)(ptr + 1));
            loadRegister(a, bus->read((hi << 8) | lo));
            cycles += 6;
        } break;
        case 0xA2: { loadRegister(x, fetch()); cycles += 2; } break;         // LDX Immediate
        case 0xA5: { loadRegister(a, bus->read(fetch())); cycles += 3; } break; // LDA Zero Page
        case 0xA9: { loadRegister(a, fetch()); cycles += 2; } break;         // LDA Immediate
        case 0xAD: {                                                          // LDA Absolute
            uint8_t lo = fetch(); uint8_t hi = fetch();
            loadRegister(a, bus->read((hi << 8) | lo));
            cycles += 4;
        } break;
        case 0xB0: {                                                          // BCS
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_C)) pc += o;
            cycles += 2;
        } break;
        case 0xB1: {                                                          // LDA (Zero Page),Y
            uint8_t ptr = fetch();
            uint8_t lo  = bus->read(ptr);
            uint8_t hi  = bus->read((uint8_t)(ptr + 1));
            loadRegister(a, bus->read(((hi << 8) | lo) + y));
            cycles += 5;
        } break;
        case 0xB5: {                                                          // LDA Zero Page,X
            uint8_t addr = fetch() + x;
            loadRegister(a, bus->read(addr));
            cycles += 4;
        } break;
        case 0xB8: { setFlag(FLAG_V, false); cycles += 2; } break;           // CLV
        case 0xB9: {                                                          // LDA Absolute,Y
            uint8_t lo = fetch(); uint8_t hi = fetch();
            loadRegister(a, bus->read(((hi << 8) | lo) + y));
            cycles += 4;
        } break;
        case 0xBD: {                                                          // LDA Absolute,X
            uint8_t lo = fetch(); uint8_t hi = fetch();
            loadRegister(a, bus->read(((hi << 8) | lo) + x));
            cycles += 4;
        } break;
        case 0xD0: {                                                          // BNE
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_Z)) pc += o;
            cycles += 2;
        } break;
        case 0xE0: {                                                          // CPX Immediate
            uint8_t value = fetch();
            uint8_t result = x - value;
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, x >= value);
            setFlag(FLAG_N, result & FLAG_N);
            cycles += 2;
        } break;
        case 0xE8: {                                                          // INX
            x++;
            setFlag(FLAG_Z, x == 0);
            setFlag(FLAG_N, x & FLAG_N);
            cycles += 2;
        } break;
        case 0xE9: {                                                          // SBC Immediate
            uint8_t value = fetch();
            uint8_t inv = ~value;
            uint16_t result = (uint16_t)a + inv + getFlag(FLAG_C);
            setFlag(FLAG_C, result > 0xFF);
            setFlag(FLAG_Z, (result & 0xFF) == 0);
            setFlag(FLAG_N, result & FLAG_N);
            setFlag(FLAG_V, (~(a ^ inv) & (a ^ result) & 0x80));
            a = (uint8_t)result;
            cycles += 2;
        } break;
        case 0xF0: {                                                          // BEQ
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_Z)) pc += o;
            cycles += 2;
        } break;
        default: break;
    }
}