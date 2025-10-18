//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄   
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██  
//      ██▀   ██▄  ▄██  ██    ██ 
//    ▄██▀     ██████   ██ ██ ██ 
//   ▄██      ██▀  ▀██  ██    ██ 
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██  
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm.cpp
// Verson: 1.0.4
// 
// This file contains a command-line utility for assembling Z80 code.
// It serves as an example of how to use the Z80Assembler class.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Assemble.h"
#include <iostream>
#include <iomanip>

void print_bytes(const std::vector<uint8_t>& bytes) {
    for (uint8_t byte : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::endl;
}

int main() {
    Z80Assembler assembler;

    std::string source_code = R"(
        ; Example code with labels
        ORG 0x8000      ; Set the origin address

START:
        LD A, 10        ; Load A with a value
LOOP:
        DEC A           ; Decrement A
        JP NZ, LOOP     ; Jump back to LOOP if A is not zero
        JP START        ; Jump back to the start
        HALT            ; This will never be reached
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code << std::endl;
        auto machine_code = assembler.assemble(source_code, 0x8000);
        std::cout << "Machine code -> ";
        print_bytes(machine_code); // Expected: 3e 0a 3d c2 03 80 c3 00 80 76

    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
