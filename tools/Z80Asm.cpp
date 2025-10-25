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
#include <cstdint>
#include <iomanip>
#include <iostream>

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
SCREEN_WIDTH    EQU 32
SCREEN_HEIGHT   EQU 24
VIDEO_RAM       EQU 0x4000
IO_PORT         EQU 0x38
STACK_SIZE      EQU 256
STACK_BASE      EQU 0xF000

; and various addressing modes.
; ============================================================================


; --- Main program ---
START:
        DI                      ; Disable interrupts
        LD SP, STACK_TOP        ; Set the stack pointer

        ; Loading registers with immediate values and constants
        LD A, 5
        LD BC, 0x1234
        LD HL, MESSAGE          ; Load the address of the label (forward reference)

        CALL CLEAR_SCREEN       ; Call a subroutine

; --- Main loop ---
MAIN_LOOP:
        LD A, (COUNTER)         ; Load value from a variable
        LD HL, (VIDEO_RAM)
        CP 10                   ; Compare with immediate value
        JP Z, FINISH            ; If equal, jump to the end

        INC A
        LD (COUNTER), A         ; Save the new value

        ; Examples of instructions with ED prefix (I/O)
        LD B, IO_PORT
        IN A, (C)               ; Read from I/O port specified by C
        OUT (C), A              ; Write to I/O port

        ; Examples of instructions with CB prefix (bit operations)
        SET 7, B                ; Set bit 7 in register B
        RES 0, C                ; Reset bit 0 in register C
        BIT 4, D                ; Test bit 4 in register D

        JP MAIN_LOOP            ; Repeat the loop

; --- End of the program ---
FINISH:
        LD HL, MSG_FINISH       ; Load the address of the final message
        HALT                    ; Halt the processor
        JR $                    ; Infinite loop (jump to self)

; --- Subroutines ---
CLEAR_SCREEN:
        LD HL, VIDEO_RAM        ; Address of the start of video memory
        LD DE, VIDEO_RAM
        INC DE
        LD BC, 767 ; SCREEN_WIDTH * SCREEN_HEIGHT - 1
        LD (HL), ' '            ; Clear the first byte
        LDIR                    ; Copy the space over the entire screen
        RET                     ; Return from subroutine

; --- More instruction examples ---
OTHER_EXAMPLES:
        NOP                     ; No operation
        EXX                     ; Exchange alternate registers
        PUSH BC                 ; Push BC onto the stack
        PUSH AF
        POP DE                  ; Pop into DE
        POP HL

        LD HL, VAR_TABLE
        ADD A, (HL)             ; Add value from memory
        INC (HL)                ; Increment value in memory
        LD B, A
        OR B                    ; Logical OR
        SUB 10                  ; Subtract immediate value

        RLA                     ; Rotate A left through carry
        DAA                     ; Decimal Adjust Accumulator
        CPL                     ; Complement A
        SCF                     ; Set Carry Flag
        CCF                     ; Complement Carry Flag

        LD A, 0
        CP A                    ; Compare A with A (will set Z flag)
        RET Z                   ; Conditional return

        RETI                    ; Return from interrupt
        RETN                    ; Return from non-maskable interrupt
        EI                      ; Enable interrupts
        RET

; ============================================================================
; Data and variables section
; ============================================================================
MESSAGE:
        DB "Program started!", 0x0D, 0x0A, 0 ; String with control chars

MSG_FINISH:
        DB "Loop finished. End.", 0

; --- Variables in RAM ---
COUNTER:
        DB 0                    ; 8-bit counter

VAR_TABLE:
        DB 1, 2, 4, 8, 16, 32   ; Define bytes

; --- Examples of using DW ---
POINTER_TO_START:
        DW START                ; 16-bit pointer to the START label

; --- Examples of instructions with DD/FD prefixes (IX/IY registers) ---
INDEXED_OPS:
        LD IX, VAR_TABLE        ; Load table address into IX
        LD IY, VIDEO_RAM        ; Load VRAM address into IY

        ; Memory access with offset
        LD A, (IX+3)
        LD (IY+10), A

        ; Arithmetic operations using indexed registers
        ADD (IX+1)
        SUB A, (IX+2)

        ; Using parts of IX/IY registers
        LD IXH, 0x80            ; Load high part of IX
        LD IXL, 0x00            ; Load low part of IX
        LD A, IXH               ; Copy from IXH
        ADD A, IXL              ; Add IXL

        ; Instructions with DDCB/FDCB prefixes
        SET 1, (IX+0)
        RES 2, (IY+0)
        BIT 7, (IX+3)
        RET
        
; --- Stack definition ---
        DS 10                   ; Reserve 10 bytes (filled with 0)
        ;ORG STACK_BASE
        DS STACK_SIZE, 0xFF     ; Reserve stack space, fill with 0xFF
STACK_TOP:                      ; Label indicating the top of the stack
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code << std::endl;
        if (assembler.compile(source_code, 0x8000)) {
            std::cout << "\nAssembly successful. Code written to bus memory." << std::endl;

            std::cout << "\n--- Code Blocks Summary ---" << std::endl;
            std::cout << std::setw(10) << std::left << "Start"
                      << std::setw(10) << std::left << "End"
                      << std::setw(10) << std::left << "Size (bytes)" << std::endl;
            std::cout << "--------------------------------" << std::endl;

            const auto& blocks = assembler.get_code_blocks().get_blocks();
            for (const auto& block : blocks) {
                std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << block.m_start_address << "    "
                          << "0x" << std::hex << std::setw(4) << std::setfill('0') << (block.m_start_address + block.m_length -1) << "    "
                          << std::dec << block.m_length
                          << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
