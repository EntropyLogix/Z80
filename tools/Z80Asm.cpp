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
    Z80Assembler<> assembler;

    std::string source_code = R"(
        ; Example with labels, data directives, and EQU
        ORG 0x8000

MAX_COUNT EQU 10

START:
        LD HL, MESSAGE  ; Load address of the message
        LD A, MAX_COUNT ; Use the constant
LOOP:
        DEC A
        JP NZ, LOOP
        JR $            ; Jump to current address (infinite loop)

        ; Data section
MESSAGE:
        DB "Loop finished!", 0
POINTER:
        DW START        ; Define a 16-bit word with the address of START
BUFFER:
        DS 16, 0xFF     ; Define a 16-byte buffer filled with 0xFF
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code << std::endl;
        auto machine_code = assembler.assemble(source_code); // ORG in code is used
        std::cout << "Machine code -> ";
        print_bytes(machine_code);

    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
