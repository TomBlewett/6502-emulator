#include <cstdint>
#include <iostream>
#include "cpu.cpp"

int main() {
    // Initialise 6502, setting PC to point to 0x0200 on startup
    CPU cpu = CPU();
    cpu.bus->ram[0xFFFC] = 0x00;
    cpu.bus->ram[0xFFFD] = 0x02;
    
    // CPU begins running
    cpu.restart();
    
    // Setting a simple additive program, 50 + 10
    cpu.bus->ram[0x0200] = 0x18;
    cpu.bus->ram[0x0201] = 0xA9;
    cpu.bus->ram[0x0202] = 0x32;
    cpu.bus->ram[0x0203] = 0x69;
    cpu.bus->ram[0x0204] = 0x0A;
    cpu.bus->ram[0x0205] = 0x02;

    // Run through instruction sets until STP opcode (0x02)
    bool firstTime = true;
    for (int i = 0; i < 7; i++) {
        std::cout << "Step " << i+1 << std::endl;
        std::cout << "PC value: ";
        cpu.printBytes(cpu.returnProgramCounter());
        std::cout << "Stack Pointer value: ";
        cpu.printByte(cpu.returnRegister('sp'));
        cpu.step();
        std::cout << "Accumulator value: " << std::dec << (int) cpu.returnRegister('a') << std::endl;
        if (cpu.isHalted() && firstTime) {
            int haltedOpcode = (int) cpu.bus->ram[cpu.returnProgramCounter()-1];
            if (haltedOpcode < 10) {
                std::cout << "----------" << std:: endl << "CPU process ended here due to opcode 0x0" << std::hex << haltedOpcode << std::endl << "----------" << std::endl;
            } else {
                std::cout << "----------" << std:: endl << "CPU process ended here due to opcode 0x" << std::hex << haltedOpcode << std::endl << "----------" << std::endl;                
            }
            firstTime = false;
        }
    }
    std::cout << "Total active cycle count: " << cpu.returnCycleCount() << std::endl;
}