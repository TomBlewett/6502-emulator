#include <cstdint>
#include <iostream>
#include <vector>
#include "cpu.cpp"

int main() {
    // Initialise 6502, setting the program counter to point to the address 0x0200
    CPU cpu = CPU();
    cpu.bus->ram[0xFFFC] = 0x00;
    cpu.bus->ram[0xFFFD] = 0x02;

    // CPU starts up
    cpu.restart();

    // Multiplies 3 x 5 via repeated addition, storing each partial product
    // into a zero-page array ($0040-$0044) using absolute,X addressing.
    // Fans the final result out to X and Y, then calls a subroutine that
    // preserves A across the call (PHA/PLA) while exercising AND/ORA/EOR.
    // Halts on STP (0x02).
    std::vector<uint8_t> program = {
        0xA9, 0x00,             // $0200 LDA #$00
        0xA2, 0x00,             // $0202 LDX #$00
        0x18,                   // $0204 CLC
        0x69, 0x03,             // $0205 ADC #$03   (L1)
        0x9D, 0x40, 0x00,       // $0207 STA $0040,X
        0xE8,                   // $020A INX
        0xE0, 0x05,             // $020B CPX #$05
        0xD0, 0xF6,             // $020D BNE L1
        0x85, 0x30,             // $020F STA $30
        0xAA,                   // $0211 TAX
        0xA8,                   // $0212 TAY
        0x20, 0x25, 0x02,       // $0213 JSR $0225
        0x02,                   // $0216 STP
        0x00, 0x00, 0x00, 0x00, // $0217 -> $0224 unused padding
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x48,                   // $0225 PHA        (subroutine)
        0xA9, 0xFF,             // $0226 LDA #$FF
        0x29, 0x0F,             // $0228 AND #$0F
        0x09, 0xF0,             // $022A ORA #$F0
        0x49, 0xFF,             // $022C EOR #$FF
        0x68,                   // $022E PLA
        0x60                    // $022F RTS
    };

    const uint16_t loadAddr = 0x0200;
    for (size_t i = 0; i < program.size(); i++) {
        cpu.bus->ram[loadAddr + i] = program[i];
    }

    // Bool to check if its the first time the CPU is halted, so that the display message is printed once
    bool firstTime = true;

    // 40 is a random arbitrary number of steps to run the CPU for
    for (int i = 0; i < 40; i++) {
        std::cout << "Step " << i + 1 << std::endl;
        std::cout << "PC value: ";
        cpu.printBytes(cpu.returnProgramCounter());
        std::cout << "Stack Pointer value: ";
        cpu.printByte(cpu.returnRegister('sp'));
        cpu.step();
        std::cout << "Accumulator value: " << std::dec << (int) cpu.returnRegister('a') << std::endl;
        std::cout << "X Register value: " << std::dec << (int) cpu.returnRegister('x') << std::endl;
        std::cout << "Y Register value: " << std::dec << (int) cpu.returnRegister('y') << std::endl;
        if (cpu.isHalted() && firstTime) {
            int haltedOpcode = (int) cpu.bus->ram[cpu.returnProgramCounter() - 1];
            if (haltedOpcode < 10) {
                std::cout << "----------" << std::endl << "CPU process ended here due to opcode 0x0" << std::hex << haltedOpcode << std::dec << std::endl << "----------" << std::endl;
            } else {
                std::cout << "----------" << std::endl << "CPU process ended here due to opcode 0x" << std::hex << haltedOpcode << std::dec << std::endl << "----------" << std::endl;
            }
            firstTime = false;
        }
    }

    std::cout << "Final memory[$0030] (stored product): "
              << std::dec << (int) cpu.bus->ram[0x0030] << std::endl;
    std::cout << "Total active cycle count: " << cpu.returnCycleCount() << std::endl;
}