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
#include "Z80Analyze.h"
#include <cstdint>
#include <iomanip>
#include <iostream>

int main() {
    Z80<> cpu;
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);


    std::string source_code_complex_resolve = R"(
; This example demonstrates forward reference resolution,
; where symbols like STACK_TOP and COUNT are used before they are defined.
; The assembler must perform multiple passes to resolve these.
                ORG 0x9000

STACK_SIZE      EQU 256
STACK_BASE      EQU STACK_TOP - STACK_SIZE

START:
        DI                      ; F3
        LD SP, STACK_TOP        ; 31 00 91
        LD A, 10101010b         ; 3E AA
        LD A, 2*8+1             ; 3E 11
        DS COUNT                ; DS 100 -> 100 bytes of 00

; --- Stack definition ---
        DS 10                   ; 10 bytes of 00
        ORG STACK_BASE
        DS STACK_SIZE, 0xFF      ; 256 bytes of FF
STACK_TOP:                      
COUNT           EQU 100
    )";

    std::string source_code_instructions = R"(
; Z80 Assembler Test Suite
; This code demonstrates various features of the Z80 assembler.

        ORG 0x8010
; --- Start of the program ---
START:
        DI                      ; 0xF3
        LD SP, 0xFFFF           ; 0x31, 0xFF, 0xFF
        LD HL, MSG_HELLO        ; 0x21, 0x02, 0x80
        LD A, VALUE1 * VALUE2   ; 0x3E, 20 (10 * 2)
        LD B, 'Z'               ; 0x06, 'Z'

; --- Control Flow ---
        CALL SUBROUTINE         ; 0xCD, 0x30, 0x80
        JP MAIN_LOOP            ; 0xC3, 0x1A, 0x80

; --- Constants and Data ---
VALUE1          EQU 10
VALUE2          EQU 2
MSG_HELLO:      ; Should be at 0x8010 after the JP instruction
        DB "Hello, Z80!", 0

; --- Data Block ---
        ORG 0x8010
DATA_BLOCK:
        DB 0xDE, 0xAD, 0xBE, 0xEF ; 0xDE, 0xAD, 0xBE, 0xEF
        DW 0x1234, 0x5678       ; 0x34, 0x12, 0x78, 0x56
        DS 4, 0xAA              ; 0xAA, 0xAA, 0xAA, 0xAA

; --- Main Program Loop ---
MAIN_LOOP:      ; 0x801A
        LD A, (DATA_BLOCK)      ; 0x3A, 0x10, 0x80
        INC A                   ; 0x3C
        DEC B                   ; 0x05
        JR NZ, MAIN_LOOP        ; 0x20, 0xFB (-5)
        HALT                    ; 0x76

; --- Subroutine Example ---
SUBROUTINE:     ; 0x8030
        PUSH AF                 ; 0xF5
        PUSH HL                 ; 0xE5
        LD A, 0                 ; 3E 00
LOOP:
        ADD HL, DE              ; 0x19
        DJNZ LOOP               ; 0x10, 0xFC (-4)
        POP HL                  ; 0xE1
        POP AF                  ; 0xF1
        RET                     ; 0xC9

; --- Indexed Addressing (IX) ---
        ORG 0x8040
IX_TEST:
        LD IX, 0x9000           ; 0xDD, 0x21, 0x00, 0x90
        LD (IX+5), 123          ; 0xDD, 0x36, 0x05, 123
        LD B, (IX+5)            ; 0xDD, 0x46, 0x05
        INC (IX-10)             ; 0xDD, 0x34, 0xF6

; --- Indexed Addressing (IY) ---
        ORG 0x8050
IY_TEST:
        LD IY, 0xA000           ; 0xFD, 0x21, 0x00, 0xA0
        LD A, (IY+0)            ; 0xFD, 0x7E, 0x00
        SET 7, (IY+1)           ; 0xFD, 0xCB, 0x01, 0xFE
        RES 0, (IY+2)           ; 0xFD, 0xCB, 0x02, 0x86

; --- ED-prefixed instructions ---
        ORG 0x8060
ED_TEST:
        IM 1                    ; 0xED, 0x56
        LD I, A                 ; 0xED, 0x47
        LD R, A                 ; 0xED, 0x4F
        LD A, I                 ; 0xED, 0x57
        LD A, R                 ; 0xED, 0x5F
        LDI                     ; 0xED, 0xA0
        CPI                     ; 0xED, 0xA1
        INI                     ; 0xED, 0xA2
        OUTI                    ; 0xED, 0xA3
        NEG                     ; 0xED, 0x44

; --- CB-prefixed instructions ---
        ORG 0x8070
CB_TEST:
        RLC B                   ; 0xCB, 0x00
        RRC C                   ; 0xCB, 0x09
        RL D                    ; 0xCB, 0x12
        RR E                    ; 0xCB, 0x1B
        SLA H                   ; 0xCB, 0x24
        SRA L                   ; 0xCB, 0x2D
        SLL A                   ; 0xCB, 0x37 (undocumented)
        SRL (HL)                ; 0xCB, 0x3E
        BIT 7, A                ; 0xCB, 0x7F
        SET 0, B                ; 0xCB, 0xC0
        RES 4, C                ; 0xCB, 0xA1
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code_complex_resolve << std::endl;
        if (assembler.compile(source_code_complex_resolve, 0x8000)) {
            std::cout << "\n--- Assembly Successful ---\n" << std::endl;

            // Print symbols
            auto symbols = assembler.get_symbols();
            std::cout << "--- Calculated Symbols ---" << std::endl;
            for (const auto& symbol : symbols) {
                std::stringstream hex_val;
                hex_val << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                        << static_cast<uint16_t>(symbol.second);

                std::cout << std::setw(20) << std::left << std::setfill(' ') << symbol.first
                          << " = " << hex_val.str()
                          << " (" << std::dec << symbol.second << ")" << std::endl;
            }
            std::cout << std::endl;

            // Print memory map of code blocks
            Z80Analyzer<Z80DefaultBus, Z80<>, void> analyzer(&bus, &cpu, nullptr);
            auto blocks = assembler.get_blocks();
            std::cout << "--- Code Blocks ---" << std::endl;
            for (size_t i = 0; i < blocks.size(); ++i) {
                const auto& block = blocks[i];
                uint16_t start_addr = block.first;
                uint16_t len = block.second;

                std::cout << "--- Block #" << i 
                          << ": Address=0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << start_addr
                          << ", Size=" << std::dec << len << " bytes ---\n";

                if (len > 0) {
                    uint16_t dump_addr = start_addr;
                    auto dump = analyzer.dump_memory(dump_addr, (len + 15) / 16, 16);
                    for (const auto& line : dump) {
                        std::cout << line << std::endl;
                    }
                    std::cout << "\n--- Disassembly for Block #" << i << " ---\n";
                    uint16_t disasm_addr = start_addr;
                    while (disasm_addr < start_addr + len) {
                        std::cout << analyzer.disassemble(disasm_addr, "%a: %-12b %-15m") << std::endl;
                    }
                }
                std::cout << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
