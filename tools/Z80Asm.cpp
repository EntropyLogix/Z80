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

    std::string single_line_code = "LD A, 0x42";
    std::string multi_line_code = R"(
        ; This is an example code snippet
        LD HL, 0x8000   ; Set pointer
        LD A, 10
        ADD A, H        ; Add H to A
        LD B,A          ; No space between operands
        HALT
    )";

    try {
        std::cout << "Single line assembly:" << std::endl;
        auto bytes1 = assembler.assemble(single_line_code);
        std::cout << "'" << single_line_code << "' -> ";
        print_bytes(bytes1); // Expected: 3e 42

        std::cout << "\nMulti-line assembly:" << std::endl;
        auto bytes2 = assembler.assemble(multi_line_code);
        std::cout << "Machine code -> ";
        print_bytes(bytes2); // Expected: 21 00 80 3e 0a 84 47 76

    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
