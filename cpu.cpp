#include <cstdint>
#include <iostream>
#include "cpu.h"

CPU::CPU() : cycles(0), accumulator(0), x_reg(0), y_reg(0), sp(0xFF), pc(0x0000), p_status(0x00), halted(false) {
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
            return accumulator;
        } break;
        case 'x' | 'X': {
            return x_reg;
        } break;
        case 'y' | 'Y': {
            return y_reg;
        } break;
        case 'sp' | 'SP': {
            return sp;
        } break;
        case 'p' | 'P': {
            return p_status;
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

// Push byte onto stack, decrement stack pointer
void CPU::push(uint8_t value) { bus->write(0x0100 + sp, value); sp--; }

// Increment stack pointer, pull byte from stack
uint8_t CPU::pull() { sp++; return bus->read(0x0100 + sp); }

// Set or clear a status flag
void CPU::setFlag(StatusFlag flag, bool value) {
    if (value) p_status |= flag;
    else       p_status &= ~flag;
}

// Reads any status flag
bool CPU::getFlag(StatusFlag flag) { return (p_status & flag) != 0; }

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
    push(p_status | FLAG_U | (isBreak ? FLAG_B : 0));
    setFlag(FLAG_I, true);
    uint8_t lsb = bus->read(vectorAddr);
    uint8_t msb  = bus->read(vectorAddr + 1);
    pc = (msb << 8) | lsb;
}

// Restart the CPU state, jumps to restart vector
void CPU::restart() {
    accumulator = 0; x_reg = 0; y_reg = 0;
    sp = 0xFF;
    p_status  = FLAG_U;
    uint8_t lsb = bus->read(0xFFFC);
    uint8_t msb  = bus->read(0xFFFD);
    pc = (msb << 8) | lsb;
}

// Trigger non-maskable interrupt
void CPU::nmi() { handleInterrupt(0xFFFA, false); }

// Trigger maskable interrupt if FLAG_I clear
void CPU::irq() { if (!getFlag(FLAG_I)) handleInterrupt(0xFFFE, false); }

// Advances the CPU through one entire instruction given an Opcode assuming Operands follow.
// Can be assumed to be one whole fetch -> decode -> execute cycle.
void CPU::step() {

    if (halted) return;

    uint8_t opcode = fetch();

    switch (opcode) {
        case 0x00: {                                                          // BRK
            pc++;
            handleInterrupt(0xFFFE, true);
            cycles += 7;
        } break;
        case 0x02: {                                                          // STP halt signal
            halted = true;
        } break;
        case 0x08: {                                                          // PHP
            push(p_status | FLAG_B | FLAG_U);
            cycles += 3;
        } break;
        case 0x09: { 
            loadRegister(accumulator, accumulator | fetch());                // ORA Immediate
            cycles += 2; 
        } break;      
        case 0x10: {                                                          // BPL
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_N)) pc += o;
            cycles += 2;
        } break;
        case 0x18: { setFlag(FLAG_C, false); cycles += 2; } break;            // CLC
        case 0x20: {                                                          // JSR
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            push((pc - 1) >> 8);
            push((pc - 1) & 0xFF);
            pc = (msb << 8) | lsb;
            cycles += 6;
        } break;
        case 0x28: {                                                          // PLP
            p_status = pull();
            setFlag(FLAG_U, true);
            cycles += 4;
        } break;
        case 0x29: {                                                         // AND Immediate
            loadRegister(accumulator, accumulator & fetch()); 
            cycles += 2; 
        } break;      
        case 0x30: {                                                          // BMI
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_N)) pc += o;
            cycles += 2;
        } break;
        case 0x38: { setFlag(FLAG_C, true); cycles += 2; } break;             // SEC
        case 0x40: {                                                          // RTI
            p_status = pull();
            setFlag(FLAG_U, true);
            uint8_t lsb = pull(); 
            uint8_t msb  = pull();
            pc = (msb << 8) | lsb;
            cycles += 6;
        } break;
        case 0x48: { push(accumulator); cycles += 3; } break;                 // PHA
        case 0x49: {                                                          // EOR Immediate
            loadRegister(accumulator, accumulator ^ fetch());       
            cycles += 2; 
        } break;      
        case 0x4C: {                                                          // JMP Absolute
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            pc = (msb << 8) | lsb;
            cycles += 3;
        } break;
        case 0x50: {                                                          // BVC
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_V)) pc += o;
            cycles += 2;
        } break;
        case 0x60: {                                                          // RTS
            uint8_t lsb = pull(); uint8_t msb  = pull();
            pc = ((msb << 8) | lsb) + 1;
            cycles += 6;
        } break;
        case 0x68: { loadRegister(accumulator, pull()); cycles += 4; } break; // PLA
        case 0x69: {                                                          // ADC Immediate
            uint8_t value = fetch();
            uint16_t result = (uint16_t) accumulator + value + getFlag(FLAG_C);
            setFlag(FLAG_C, result > 0xFF);
            setFlag(FLAG_Z, (result & 0xFF) == 0);
            setFlag(FLAG_N, result & FLAG_N);
            setFlag(FLAG_V, (~(accumulator ^ value) & (accumulator ^ result) & 0x80));
            accumulator = (uint8_t)result;
            cycles += 2;
        } break;
        case 0x6C: {                                                          // JMP Indirect
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            uint16_t ptr = (msb << 8) | lsb;
            pc = bus->read(ptr) | (bus->read(ptr + 1) << 8);
            cycles += 5;
        } break;
        case 0x70: {                                                          // BVS
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_V)) pc += o;
            cycles += 2;
        } break;
        case 0x78: {                                                          // SEI
            setFlag(FLAG_I, true);
            cycles += 2;
        } break;
        case 0x84: {                                                          // STY Zero Page
            bus->write(fetch(), y_reg);
            cycles += 3;
        } break;
        case 0x85: {                                                          // STA Zero Page
            bus->write(fetch(), accumulator);
            cycles += 3;
        } break;
        case 0x86: {                                                          // STX Zero Page
            bus->write(fetch(), x_reg);
            cycles += 3;
        } break;
        case 0x8A: {                                                          // TXA
            loadRegister(accumulator, x_reg);
            cycles += 2;
        } break;
        case 0x8C: {                                                          // STY Absolute
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            bus->write((msb << 8) | lsb, y_reg);
            cycles += 4;
        } break;
        case 0x8D: {                                                          // STA Absolute
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            bus->write((msb << 8) | lsb, accumulator);
            cycles += 4;
        } break;
        case 0x8E: {                                                          // STX Absolute
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            bus->write((msb << 8) | lsb, x_reg);
            cycles += 4;
        } break;
        case 0x90: {                                                          // BCC
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_C)) pc += o;
            cycles += 2;
        } break;
        case 0x94: {                                                          // STY Zero Page,X
            uint8_t addr = fetch() + x_reg;
            bus->write(addr, y_reg);
            cycles += 4;
        } break;
        case 0x96: {
            uint8_t addr = fetch() + y_reg;                                   // STX Zero Page,Y 
            bus->write(addr, x_reg);
            cycles += 4;
        } break;
        case 0x98: {                                                          // TYA
            loadRegister(accumulator, y_reg);
            cycles += 2;
        } break;
        case 0x9A: { sp = x_reg; cycles += 2; } break;                        // TXS
        case 0x9D: {                                                          // STA Absolute,X
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            bus->write(((msb << 8) | lsb) + x_reg, accumulator);
            cycles += 5;
        } break;
        case 0xA0: {                                                          // LDY Immediate
            uint8_t val = fetch();
            loadRegister(y_reg, val);
            cycles += 2;
        } break;
        case 0xA1: {                                                          // LDA (Zero Page,X)
            uint8_t ptr = fetch() + x_reg;
            uint8_t lsb  = bus->read(ptr);
            uint8_t msb   = bus->read((uint8_t)(ptr + 1));
            loadRegister(accumulator, bus->read((msb << 8) | lsb));
            cycles += 6;
        } break;
        case 0xA2: { loadRegister(x_reg, fetch()); cycles += 2; } break;      // LDX Immediate
        case 0xA4: {                                                          // LDY Zero Page
            loadRegister(y_reg, bus->read(fetch()));
            cycles += 3;
        } break;
        case 0xA5: { loadRegister(accumulator, bus->read(fetch())); 
            cycles += 3; 
        } break;                                                              // LDA Zero Page
        case 0xA8: {                                                          // TAY
            loadRegister(y_reg, accumulator);
            cycles += 2;
        } break;
        case 0xA9: { loadRegister(accumulator, fetch()); cycles += 2; } break;// LDA Immediate
        case 0xAA: { loadRegister(x_reg, accumulator); cycles += 2; } break;  // TAX
        case 0xAC: {                                                          // LDY Absolute
            uint8_t lsb = fetch();
            uint8_t msb = fetch();
            loadRegister(y_reg, bus->read((msb << 8) | lsb));
            cycles += 4;
        } break;
        case 0xAD: {                                                          // LDA Absolute
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            loadRegister(accumulator, bus->read((msb << 8) | lsb));
            cycles += 4;
        } break;
        case 0xB0: {                                                          // BCS
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_C)) pc += o;
            cycles += 2;
        } break;
        case 0xB1: {                                                          // LDA (Zero Page),Y
            uint8_t ptr = fetch();
            uint8_t lsb  = bus->read(ptr);
            uint8_t msb   = bus->read((uint8_t)(ptr + 1));
            loadRegister(accumulator, bus->read(((msb << 8) | lsb) + y_reg));
            cycles += 5;
        } break;
        case 0xB4: {                                                          // LDY Zero Page,X
            uint8_t addr = fetch() + x_reg;
            loadRegister(y_reg, bus->read(addr));
            cycles += 4;
        } break;
        case 0xB5: {                                                          // LDA Zero Page,X
            uint8_t addr = fetch() + x_reg;
            loadRegister(accumulator, bus->read(addr));
            cycles += 4;
        } break;
        case 0xB8: { setFlag(FLAG_V, false); cycles += 2; } break;            // CLV
        case 0xB9: {                                                          // LDA Absolute,Y
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            loadRegister(accumulator, bus->read(((msb << 8) | lsb) + y_reg));
            cycles += 4;
        } break;
        case 0xBA: {                                                          // TSX
            loadRegister(x_reg, sp);
            cycles += 2;
        } break;
        case 0xBC: {                                                          // LDY Absolute,X
            uint8_t lsb = fetch();
            uint8_t msb = fetch();
            loadRegister(y_reg, bus->read(((msb << 8) | lsb) + x_reg));
            cycles += 4;
        } break;
        case 0xBD: {                                                          // LDA Absolute,X
            uint8_t lsb = fetch(); 
            uint8_t msb  = fetch();
            loadRegister(accumulator, bus->read(((msb << 8) | lsb) + x_reg));
            cycles += 4;
        } break;
        case 0xC0: {                                                          // CPY Immediate
            uint8_t value = fetch();
            uint8_t result = y_reg - value;
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, y_reg >= value);
            setFlag(FLAG_N, result & FLAG_N);
            cycles += 2;
        } break;
        case 0xC4: {                                                          // CPY Zero Page
            uint8_t value = bus->read(fetch());
            uint8_t result = y_reg - value;
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, y_reg >= value);
            setFlag(FLAG_N, result & FLAG_N);
            cycles += 3;
        } break;
        case 0xC6: {                                                          // DEC Zero Page
            uint8_t addr = fetch();
            uint8_t value = bus->read(addr) - 1;
            bus->write(addr, value);
            setFlag(FLAG_Z, value == 0);
            setFlag(FLAG_N, value & FLAG_N);
            cycles += 5;
        } break;
        case 0xC8: {                                                          // INY
            y_reg++;
            setFlag(FLAG_Z, y_reg == 0);
            setFlag(FLAG_N, y_reg & FLAG_N);
            cycles += 2;
        } break;
        case 0xC9: {                                                          // CMP Immediate
            uint8_t value = fetch();
            uint8_t result = accumulator - value;
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, accumulator >= value);
            setFlag(FLAG_N, result & FLAG_N);
            cycles += 2;
        } break;
        case 0xCA: {                                                          // DEX
            x_reg--;
            setFlag(FLAG_Z, x_reg == 0);
            setFlag(FLAG_N, x_reg & FLAG_N);
            cycles += 2;
        } break;
        case 0xCE: {                                                          // DEC Absolute
            uint8_t lsb = fetch();
            uint8_t msb = fetch();
            uint16_t addr = (msb << 8) | lsb;
            uint8_t value = bus->read(addr) - 1;
            bus->write(addr, value);
            setFlag(FLAG_Z, value == 0);
            setFlag(FLAG_N, value & FLAG_N);
            cycles += 6;
        } break;
        case 0xD0: {                                                          // BNE
            int8_t o = (int8_t)fetch();
            if (!getFlag(FLAG_Z)) pc += o;
            cycles += 2;
        } break;
        case 0xD6: {                                                          // DEC Zero Page,X
            uint8_t addr = fetch() + x_reg;
            uint8_t value = bus->read(addr) - 1;
            bus->write(addr, value);
            setFlag(FLAG_Z, value == 0);
            setFlag(FLAG_N, value & FLAG_N);
            cycles += 6;
        } break;
        case 0xDE: {                                                          // DEC Absolute,X
            uint8_t lsb = fetch();
            uint8_t msb = fetch();
            uint16_t addr = (msb << 8) | lsb + x_reg;
            uint8_t value = bus->read(addr) - 1;
            bus->write(addr, value);
            setFlag(FLAG_Z, value == 0);
            setFlag(FLAG_N, value & FLAG_N);
            cycles += 6;
        } break;
        case 0xE0: {                                                          // CPX Immediate
            uint8_t value = fetch();
            uint8_t result = x_reg - value;
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, x_reg >= value);
            setFlag(FLAG_N, result & FLAG_N);
            cycles += 2;
        } break;
        case 0xE5: {                                                          // SBC Zero Page
            cycles += 3;
        } break;
        case 0xE6: {                                                          // INC Zero Page
            uint8_t addr = fetch();
            uint8_t value = bus->read(addr) + 1;
            bus->write(addr, value);
            setFlag(FLAG_Z, value == 0);
            setFlag(FLAG_N, value & FLAG_N);
            cycles += 5;
        } break;
        case 0xE8: {                                                          // INX
            x_reg++;
            setFlag(FLAG_Z, x_reg == 0);
            setFlag(FLAG_N, x_reg & FLAG_N);
            cycles += 2;
        } break;
        case 0xE9: {                                                          // SBC Immediate
            uint8_t value = fetch();
            uint8_t inv = ~value;
            uint16_t result = (uint16_t) accumulator + inv + getFlag(FLAG_C);
            setFlag(FLAG_C, result > 0xFF);
            setFlag(FLAG_Z, (result & 0xFF) == 0);
            setFlag(FLAG_N, result & FLAG_N);
            setFlag(FLAG_V, (~(accumulator ^ inv) & (accumulator ^ result) & 0x80));
            accumulator = (uint8_t)result;
            cycles += 2;
        } break;
        case 0xF0: {                                                          // BEQ
            int8_t o = (int8_t)fetch();
            if (getFlag(FLAG_Z)) pc += o;
            cycles += 2;
        } break;
        case 0xF8: {
            setFlag(FLAG_D, true);
            cycles += 2;
        } break;
        default: {
            halted = true;
        } break;
    }
}