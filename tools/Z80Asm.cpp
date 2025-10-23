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
#include <iomanip>
#include <iostream>

void print_bytes(const std::vector<uint8_t>& bytes) {
    for (uint8_t byte : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::endl;
}

int main() {
    // The assembler needs a bus to interact with memory, even if it's just for calculating sizes.
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);

    // This source code demonstrates a wide range of assembler features,
    // including directives, labels, various addressing modes,
    // and instructions with CB, ED, DD, FD prefixes.
    std::string source_code = R"(
; ============================================================================
; Comprehensive example for Z80Asm
; Demonstrates labels, directives, prefixed instructions,
; and various addressing modes.
; ============================================================================

        ORG 0x8000          ; Set the program's starting address

; --- Constants defined with EQU ---
MAX_RETRIES     EQU 5
VIDEO_RAM      EQU 0x4000
IO_PORT        EQU 0x38

; --- Main program ---
START:
        DI                  ; Disable interrupts
        LD SP, STACK_TOP    ; Set the stack pointer

        ; Loading registers with immediate values and constants
        LD A, MAX_RETRIES
        LD BC, 0x1234
        LD HL, MESSAGE      ; Load the address of the label (forward reference)

        CALL CLEAR_SCREEN   ; Call a subroutine

; --- Main loop ---
MAIN_LOOP:
        LD A, (COUNTER)     ; Load value from a variable
        CP MAX_RETRIES      ; Compare with the maximum value
        JP Z, FINISH        ; If equal, jump to the end

        INC A
        LD (COUNTER), A     ; Save the new value

        ; Examples of instructions with ED prefix (I/O)
        IN A, (IO_PORT)     ; Read from I/O port
        OUT (IO_PORT), A    ; Write to I/O port

        ; Examples of instructions with CB prefix (bit operations)
        SET 7, B            ; Set bit 7 in register B
        RES 0, C            ; Reset bit 0 in register C
        BIT 4, D            ; Test bit 4 in register D

        JP MAIN_LOOP        ; Repeat the loop

; --- End of the program ---
FINISH:
        LD HL, MSG_FINISH   ; Load the address of the final message
        HALT                ; Halt the processor
        JR $                 ; Infinite loop (jump to self)

; --- Subroutines ---
CLEAR_SCREEN:
        LD HL, VIDEO_RAM    ; Address of the start of video memory
        LD DE, VIDEO_RAM
        INC DE
        LD BC, 767          ; 32*24 - 1
        LD (HL), ' '        ; Clear the first byte
        LDIR                ; Copy the space over the entire screen
        RET                 ; Return from subroutine

; ============================================================================
; Data and variables section
; ============================================================================
MESSAGE:
        DB "Program started!", 0x0D, 0x0A, 0

MSG_FINISH:
        DB "Loop finished. End.", 0

; --- Variables in RAM ---
COUNTER:
        DB 0                ; 8-bit counter

; --- Examples of using DW ---
POINTER_TO_START:
        DW START            ; 16-bit pointer to the START label

; --- Examples of instructions with DD/FD prefixes (IX/IY registers) ---
INDEXED_OPS:
        LD IX, TABLE_DATA   ; Load table address into IX
        LD IY, VIDEO_RAM    ; Load VRAM address into IY

        ; Memory access with offset (note: these are not supported by the current assembler)
        ; LD A, (IX+3)
        ; LD (IY+10), A

        ; Arithmetic operations using indexed registers (note: not supported)
        ; ADD A, (IX+1)
        ; SUB (IX+2)

        ; Using parts of IX/IY registers
        LD IXH, 0x80
        LD IXL, 0x00
        LD A, IXH
        ADD A, IXL

        ; Instructions with DDCB/FDCB prefixes (note: not supported)
        ; SET 1, (IX+0)
        ; RES 2, (IY+0)
        ; BIT 7, (IX+3)
        RET

TABLE_DATA:
        DB 0xAA, 0xBB, 0xCC, 0xDD, 0xEE

; --- Stack definition ---
        DS 255, 0           ; Reserve 255 bytes for the stack
STACK_TOP:                  ; Label indicating the top of the stack
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code << std::endl;
        if (assembler.assemble(source_code)) {
            std::cout << "Machine code -> ";
            // Read back from the bus to display the assembled code
            const auto& symbol_table = assembler.get_symbol_table();
            std::vector<uint8_t> machine_code;
            uint16_t start_addr = 0x8000; 
            uint16_t end_addr = symbol_table.get_value("STACK_TOP", 0);
            for (uint16_t addr = start_addr; addr < end_addr; ++addr) {
                machine_code.push_back(bus.peek(addr));
            }
            print_bytes(machine_code);
            std::cout << "Assembly successful. Code written to bus memory." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
