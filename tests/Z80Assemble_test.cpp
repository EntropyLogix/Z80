//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble_test.cpp
// Verson: 1.0.5
//
// This file contains unit tests for the Z80Assembler class.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Assemble.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <iomanip>

int tests_passed = 0;
int tests_failed = 0;

#define TEST_CASE(name)                                           \
    void test_##name();                                           \
    struct TestRegisterer_##name {                                \
        TestRegisterer_##name() {                                 \
            std::cout << "--- Running test: " #name " ---\n";     \
            try {                                                 \
                test_##name();                                    \
            } catch (const std::exception& e) {                   \
                std::cerr << "ERROR: " << e.what() << std::endl;  \
                tests_failed++;                                   \
            }                                                     \
        }                                                         \
    } test_registerer_##name;                                     \
    void test_##name()

void ASSERT_CODE(const std::string& asm_code, const std::vector<uint8_t>& expected_bytes) {
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);

    bool success = assembler.compile(asm_code, 0x0000);
    if (!success) {
        std::cerr << "Assertion failed: Compilation failed for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    if (blocks.empty() || blocks[0].second != expected_bytes.size()) {
        std::cerr << "Assertion failed: Incorrect compiled size for '" << asm_code << "'.\n";
        std::cerr << "  Expected size: " << expected_bytes.size() << ", Got: " << (blocks.empty() ? 0 : blocks[0].second) << "\n";
        tests_failed++;
        return;
    }

    bool mismatch = false;
    for (size_t i = 0; i < expected_bytes.size(); ++i) {
        if (bus.peek(i) != expected_bytes[i]) {
            mismatch = true;
            break;
        }
    }

    if (mismatch) {
        std::cerr << "Assertion failed: Byte mismatch for '" << asm_code << "'\n";
        std::cerr << "  Expected: ";
        for (uint8_t byte : expected_bytes) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
        std::cerr << "\n  Got:      ";
        for (size_t i = 0; i < expected_bytes.size(); ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)bus.peek(i) << " ";
        std::cerr << std::dec << "\n";
        tests_failed++;
    } else {
        tests_passed++;
    }
}

void ASSERT_COMPILE_FAILS(const std::string& asm_code) {
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);
    try {
        bool success = assembler.compile(asm_code, 0x0000);
        if (success) {
            std::cerr << "Assertion failed: Compilation succeeded for '" << asm_code << "' but was expected to fail.\n";
            tests_failed++;
        } else {
            // This case might occur if compile() returns false without an exception.
            tests_passed++;
        }
    } catch (const std::runtime_error& e) {
        // Exception was expected, so this is a pass.
        tests_passed++;
    } catch (...) {
        std::cerr << "Assertion failed: An unexpected exception was thrown for '" << asm_code << "'.\n";
        tests_failed++;
    }
}

TEST_CASE(NoOperandInstructions) {
    ASSERT_CODE("NOP", {0x00});
    ASSERT_CODE("HALT", {0x76});
    ASSERT_CODE("DI", {0xF3});
    ASSERT_CODE("EI", {0xFB});
    ASSERT_CODE("EXX", {0xD9});
    ASSERT_CODE("RET", {0xC9});
    ASSERT_CODE("RETI", {0xED, 0x4D});
    ASSERT_CODE("RETN", {0xED, 0x45});
    ASSERT_CODE("RLCA", {0x07});
    ASSERT_CODE("RRCA", {0x0F});
    ASSERT_CODE("RLA", {0x17});
    ASSERT_CODE("RRA", {0x1F});
    ASSERT_CODE("DAA", {0x27});
    ASSERT_CODE("CPL", {0x2F});
    ASSERT_CODE("SCF", {0x37});
    ASSERT_CODE("CCF", {0x3F});
    ASSERT_CODE("LDI", {0xED, 0xA0});
    ASSERT_CODE("CPI", {0xED, 0xA1});
    ASSERT_CODE("INI", {0xED, 0xA2});
    ASSERT_CODE("OUTI", {0xED, 0xA3});
    ASSERT_CODE("LDD", {0xED, 0xA8});
    ASSERT_CODE("CPD", {0xED, 0xA9});
    ASSERT_CODE("IND", {0xED, 0xAA});
    ASSERT_CODE("OUTD", {0xED, 0xAB});
    ASSERT_CODE("LDIR", {0xED, 0xB0});
    ASSERT_CODE("CPIR", {0xED, 0xB1});
    ASSERT_CODE("INIR", {0xED, 0xB2});
    ASSERT_CODE("OTIR", {0xED, 0xB3});
    ASSERT_CODE("LDDR", {0xED, 0xB8});
    ASSERT_CODE("CPDR", {0xED, 0xB9});
    ASSERT_CODE("INDR", {0xED, 0xBA});
    ASSERT_CODE("OTDR", {0xED, 0xBB});
    ASSERT_CODE("NEG", {0xED, 0x44});
}

TEST_CASE(OneOperandInstructions) {
    // PUSH/POP
    ASSERT_CODE("PUSH BC", {0xC5});
    ASSERT_CODE("PUSH DE", {0xD5});
    ASSERT_CODE("PUSH HL", {0xE5});
    ASSERT_CODE("PUSH AF", {0xF5});
    ASSERT_CODE("PUSH IX", {0xDD, 0xE5});
    ASSERT_CODE("PUSH IY", {0xFD, 0xE5});
    ASSERT_CODE("POP BC", {0xC1});
    ASSERT_CODE("POP DE", {0xD1});
    ASSERT_CODE("POP HL", {0xE1});
    ASSERT_CODE("POP AF", {0xF1});
    ASSERT_CODE("POP IX", {0xDD, 0xE1});
    ASSERT_CODE("POP IY", {0xFD, 0xE1});

    // INC/DEC 16-bit
    ASSERT_CODE("INC BC", {0x03});
    ASSERT_CODE("INC DE", {0x13});
    ASSERT_CODE("INC HL", {0x23});
    ASSERT_CODE("INC SP", {0x33});
    ASSERT_CODE("INC IX", {0xDD, 0x23});
    ASSERT_CODE("INC IY", {0xFD, 0x23});
    ASSERT_CODE("DEC BC", {0x0B});
    ASSERT_CODE("DEC DE", {0x1B});
    ASSERT_CODE("DEC HL", {0x2B});
    ASSERT_CODE("DEC SP", {0x3B});
    ASSERT_CODE("DEC IX", {0xDD, 0x2B});
    ASSERT_CODE("DEC IY", {0xFD, 0x2B});

    // INC/DEC 8-bit
    ASSERT_CODE("INC A", {0x3C});
    ASSERT_CODE("INC B", {0x04});
    ASSERT_CODE("INC C", {0x0C});
    ASSERT_CODE("INC D", {0x14});
    ASSERT_CODE("INC E", {0x1C});
    ASSERT_CODE("INC H", {0x24});
    ASSERT_CODE("INC L", {0x2C});
    ASSERT_CODE("INC (HL)", {0x34});
    ASSERT_CODE("DEC A", {0x3D});
    ASSERT_CODE("DEC B", {0x05});
    ASSERT_CODE("DEC C", {0x0D});
    ASSERT_CODE("DEC D", {0x15});
    ASSERT_CODE("DEC E", {0x1D});
    ASSERT_CODE("DEC H", {0x25});
    ASSERT_CODE("DEC L", {0x2D});
    ASSERT_CODE("DEC (HL)", {0x35});

    // Jumps
    ASSERT_CODE("JP 0x1234", {0xC3, 0x34, 0x12});
    ASSERT_CODE("JP (HL)", {0xE9});
    ASSERT_CODE("JP (IX)", {0xDD, 0xE9});
    ASSERT_CODE("JP (IY)", {0xFD, 0xE9});
    ASSERT_CODE("JR 0x0005", {0x18, 0x03}); // 5 - (0+2) = 3
    ASSERT_CODE("JR 0x0000", {0x18, 0xFE}); // 0 - (0+2) = -2

    // Calls
    ASSERT_CODE("CALL 0xABCD", {0xCD, 0xCD, 0xAB});

    // RST
    ASSERT_CODE("RST 0x00", {0xC7});
    ASSERT_CODE("RST 0x08", {0xCF});
    ASSERT_CODE("RST 0x10", {0xD7});
    ASSERT_CODE("RST 0x18", {0xDF});
    ASSERT_CODE("RST 0x20", {0xE7});
    ASSERT_CODE("RST 0x28", {0xEF});
    ASSERT_CODE("RST 0x30", {0xF7});
    ASSERT_CODE("RST 0x38", {0xFF});

    // Arithmetic/Logic with immediate
    ASSERT_CODE("ADD A, 0x42", {0xC6, 0x42});
    ASSERT_CODE("ADC A, 0x42", {0xCE, 0x42});
    ASSERT_CODE("SUB 0x42", {0xD6, 0x42});
    ASSERT_CODE("SBC A, 0x42", {0xDE, 0x42});
    ASSERT_CODE("AND 0x42", {0xE6, 0x42});
    ASSERT_CODE("XOR 0x42", {0xEE, 0x42});
    ASSERT_CODE("OR 0x42", {0xF6, 0x42});
    ASSERT_CODE("CP 0x42", {0xFE, 0x42});

    // Arithmetic/Logic with register
    ASSERT_CODE("ADD A, B", {0x80});
    ASSERT_CODE("ADD A, C", {0x81});
    ASSERT_CODE("ADD A, D", {0x82});
    ASSERT_CODE("ADD A, E", {0x83});
    ASSERT_CODE("ADD A, H", {0x84});
    ASSERT_CODE("ADD A, L", {0x85});
    ASSERT_CODE("ADD A, (HL)", {0x86});
    ASSERT_CODE("ADD A, A", {0x87});
    ASSERT_CODE("SUB B", {0x90});
    ASSERT_CODE("AND C", {0xA1});
    ASSERT_CODE("OR D", {0xB2});
    ASSERT_CODE("XOR E", {0xAB});
    ASSERT_CODE("CP H", {0xBC});

    // Conditional RET
    ASSERT_CODE("RET NZ", {0xC0});
    ASSERT_CODE("RET Z", {0xC8});
    ASSERT_CODE("RET NC", {0xD0});
    ASSERT_CODE("RET C", {0xD8});
    ASSERT_CODE("RET PO", {0xE0});
    ASSERT_CODE("RET PE", {0xE8});
    ASSERT_CODE("RET P", {0xF0});
    ASSERT_CODE("RET M", {0xF8});

    // IM
    ASSERT_CODE("IM 0", {0xED, 0x46});
    ASSERT_CODE("IM 1", {0xED, 0x56});
    ASSERT_CODE("IM 2", {0xED, 0x5E});
}

TEST_CASE(OneOperandInstructions_Indexed) {
    // INC (IX+d)
    ASSERT_CODE("INC (IX+5)", {0xDD, 0x34, 0x05});
    ASSERT_CODE("INC (IX-10)", {0xDD, 0x34, 0xF6});
    // DEC (IX+d)
    ASSERT_CODE("DEC (IX+127)", {0xDD, 0x35, 0x7F});
    ASSERT_CODE("DEC (IX-128)", {0xDD, 0x35, 0x80});
    // INC (IY+d)
    ASSERT_CODE("INC (IY+0)", {0xFD, 0x34, 0x00});
    ASSERT_CODE("DEC (IY-30)", {0xFD, 0x35, 0xE2});
}

TEST_CASE(TwoOperandInstructions_LD) {
    // LD r, r'
    ASSERT_CODE("LD A, B", {0x78});
    ASSERT_CODE("LD H, L", {0x65});
    ASSERT_CODE("LD B, B", {0x40});

    // LD r, n
    ASSERT_CODE("LD A, 0x12", {0x3E, 0x12});
    ASSERT_CODE("LD B, 0x34", {0x06, 0x34});
    ASSERT_CODE("LD C, 0x56", {0x0E, 0x56});
    ASSERT_CODE("LD D, 0x78", {0x16, 0x78});
    ASSERT_CODE("LD E, 0x9A", {0x1E, 0x9A});
    ASSERT_CODE("LD H, 0xBC", {0x26, 0xBC});
    ASSERT_CODE("LD L, 0xDE", {0x2E, 0xDE});

    // LD r, (HL)
    ASSERT_CODE("LD A, (HL)", {0x7E});
    ASSERT_CODE("LD B, (HL)", {0x46});
    ASSERT_CODE("LD C, (HL)", {0x4E});
    ASSERT_CODE("LD D, (HL)", {0x56});
    ASSERT_CODE("LD E, (HL)", {0x5E});
    ASSERT_CODE("LD H, (HL)", {0x66});
    ASSERT_CODE("LD L, (HL)", {0x6E});

    // LD (HL), r
    ASSERT_CODE("LD (HL), A", {0x77});
    ASSERT_CODE("LD (HL), B", {0x70});
    ASSERT_CODE("LD (HL), C", {0x71});
    ASSERT_CODE("LD (HL), D", {0x72});
    ASSERT_CODE("LD (HL), E", {0x73});
    ASSERT_CODE("LD (HL), H", {0x74});
    ASSERT_CODE("LD (HL), L", {0x75});

    // LD (HL), n
    ASSERT_CODE("LD (HL), 0xAB", {0x36, 0xAB});

    // LD A, (rr)
    ASSERT_CODE("LD A, (BC)", {0x0A});
    ASSERT_CODE("LD A, (DE)", {0x1A});

    // LD (rr), A
    ASSERT_CODE("LD (BC), A", {0x02});
    ASSERT_CODE("LD (DE), A", {0x12});

    // LD A, (nn)
    ASSERT_CODE("LD A, (0x1234)", {0x3A, 0x34, 0x12});

    // LD (nn), A
    ASSERT_CODE("LD (0x1234), A", {0x32, 0x34, 0x12});

    // LD rr, nn
    ASSERT_CODE("LD BC, 0x1234", {0x01, 0x34, 0x12});
    ASSERT_CODE("LD DE, 0x5678", {0x11, 0x78, 0x56});
    ASSERT_CODE("LD HL, 0x9ABC", {0x21, 0xBC, 0x9A});
    ASSERT_CODE("LD SP, 0xDEF0", {0x31, 0xF0, 0xDE});

    // LD rr, (nn)
    ASSERT_CODE("LD HL, (0x1234)", {0x2A, 0x34, 0x12});
    ASSERT_CODE("LD BC, (0x1234)", {0xED, 0x4B, 0x34, 0x12});
    ASSERT_CODE("LD DE, (0x1234)", {0xED, 0x5B, 0x34, 0x12});
    ASSERT_CODE("LD SP, (0x1234)", {0xED, 0x7B, 0x34, 0x12});

    // LD (nn), rr
    ASSERT_CODE("LD (0x1234), HL", {0x22, 0x34, 0x12});
    ASSERT_CODE("LD (0x1234), BC", {0xED, 0x43, 0x34, 0x12});
    ASSERT_CODE("LD (0x1234), DE", {0xED, 0x53, 0x34, 0x12});
    ASSERT_CODE("LD (0x1234), SP", {0xED, 0x73, 0x34, 0x12});

    // LD SP, HL/IX/IY
    ASSERT_CODE("LD SP, HL", {0xF9});
    ASSERT_CODE("LD SP, IX", {0xDD, 0xF9});
    ASSERT_CODE("LD SP, IY", {0xFD, 0xF9});

    // LD I, A and LD R, A
    ASSERT_CODE("LD I, A", {0xED, 0x47});
    ASSERT_CODE("LD R, A", {0xED, 0x4F});

    // LD A, I and LD A, R
    ASSERT_CODE("LD A, I", {0xED, 0x57});
    ASSERT_CODE("LD A, R", {0xED, 0x5F});
}

TEST_CASE(TwoOperandInstructions_LD_Indexed) {
    // LD IX/IY, nn
    ASSERT_CODE("LD IX, 0x1234", {0xDD, 0x21, 0x34, 0x12});
    ASSERT_CODE("LD IY, 0x5678", {0xFD, 0x21, 0x78, 0x56});

    // LD IX/IY, (nn)
    ASSERT_CODE("LD IX, (0x1234)", {0xDD, 0x2A, 0x34, 0x12});
    ASSERT_CODE("LD IY, (0x5678)", {0xFD, 0x2A, 0x78, 0x56});

    // LD (nn), IX/IY
    ASSERT_CODE("LD (0x1234), IX", {0xDD, 0x22, 0x34, 0x12});
    ASSERT_CODE("LD (0x5678), IY", {0xFD, 0x22, 0x78, 0x56});

    // LD r, (IX/IY+d)
    ASSERT_CODE("LD A, (IX+10)", {0xDD, 0x7E, 0x0A});
    ASSERT_CODE("LD B, (IX-20)", {0xDD, 0x46, 0xEC}); // -20 = 0xEC
    ASSERT_CODE("LD C, (IY+0)", {0xFD, 0x4E, 0x00});
    ASSERT_CODE("LD D, (IY+127)", {0xFD, 0x56, 0x7F});

    // LD (IX/IY+d), r
    ASSERT_CODE("LD (IX+5), A", {0xDD, 0x77, 0x05});
    ASSERT_CODE("LD (IX-8), B", {0xDD, 0x70, 0xF8});
    ASSERT_CODE("LD (IY+0), C", {0xFD, 0x71, 0x00});
    ASSERT_CODE("LD (IY+127), D", {0xFD, 0x72, 0x7F});

    // LD (IX/IY+d), n
    ASSERT_CODE("LD (IX+1), 0xAB", {0xDD, 0x36, 0x01, 0xAB});
    ASSERT_CODE("LD (IY-2), 0xCD", {0xFD, 0x36, 0xFE, 0xCD});

    // LD r, IXH/IXL/IYH/IYL
    ASSERT_CODE("LD A, IXH", {0xDD, 0x7C});
    ASSERT_CODE("LD B, IXL", {0xDD, 0x45});
    ASSERT_CODE("LD C, IYH", {0xFD, 0x4C});
    ASSERT_CODE("LD D, IYL", {0xFD, 0x55});

    // LD IXH/IXL/IYH/IYL, r
    ASSERT_CODE("LD IXH, A", {0xDD, 0x67});
    ASSERT_CODE("LD IXL, B", {0xDD, 0x68});
    ASSERT_CODE("LD IYH, C", {0xFD, 0x61});
    ASSERT_CODE("LD IYL, D", {0xFD, 0x6A});

    // LD IXH, IXL etc.
    ASSERT_CODE("LD IXH, IXL", {0xDD, 0x65});
    ASSERT_CODE("LD IYH, IYL", {0xFD, 0x65});
}

TEST_CASE(TwoOperandInstructions_Arithmetic) {
    // ADD HL, rr
    ASSERT_CODE("ADD HL, BC", {0x09});
    ASSERT_CODE("ADD HL, DE", {0x19});
    ASSERT_CODE("ADD HL, HL", {0x29});
    ASSERT_CODE("ADD HL, SP", {0x39});

    // ADC HL, rr
    ASSERT_CODE("ADC HL, BC", {0xED, 0x4A});
    ASSERT_CODE("ADC HL, DE", {0xED, 0x5A});
    ASSERT_CODE("ADC HL, HL", {0xED, 0x6A});
    ASSERT_CODE("ADC HL, SP", {0xED, 0x7A});

    // SBC HL, rr
    ASSERT_CODE("SBC HL, BC", {0xED, 0x42});
    ASSERT_CODE("SBC HL, DE", {0xED, 0x52});
    ASSERT_CODE("SBC HL, HL", {0xED, 0x62});
    ASSERT_CODE("SBC HL, SP", {0xED, 0x72});

    // ADD IX/IY, rr
    ASSERT_CODE("ADD IX, BC", {0xDD, 0x09});
    ASSERT_CODE("ADD IX, DE", {0xDD, 0x19});
    ASSERT_CODE("ADD IX, IX", {0xDD, 0x29});
    ASSERT_CODE("ADD IX, SP", {0xDD, 0x39});
    ASSERT_CODE("ADD IY, BC", {0xFD, 0x09});
    ASSERT_CODE("ADD IY, DE", {0xFD, 0x19});
    ASSERT_CODE("ADD IY, IY", {0xFD, 0x29});
    ASSERT_CODE("ADD IY, SP", {0xFD, 0x39});

    // EX DE, HL
    ASSERT_CODE("EX DE, HL", {0xEB});

    // EX AF, AF'
    ASSERT_CODE("EX AF, AF'", {0x08});

    // EX (SP), HL/IX/IY
    ASSERT_CODE("EX (SP), HL", {0xE3});
    ASSERT_CODE("EX (SP), IX", {0xDD, 0xE3});
    ASSERT_CODE("EX (SP), IY", {0xFD, 0xE3});
}

TEST_CASE(TwoOperandInstructions_Arithmetic_Indexed) {
    // ADD A, (IX/IY+d)
    ASSERT_CODE("ADD A, (IX+10)", {0xDD, 0x86, 0x0A});
    ASSERT_CODE("ADD A, (IY-5)", {0xFD, 0x86, 0xFB});
    // ADC A, (IX/IY+d)
    ASSERT_CODE("ADC A, (IX+1)", {0xDD, 0x8E, 0x01});
    ASSERT_CODE("ADC A, (IY-2)", {0xFD, 0x8E, 0xFE});
    // SUB (IX/IY+d)
    ASSERT_CODE("SUB (IX+15)", {0xDD, 0x96, 0x0F});
    ASSERT_CODE("SUB (IY-128)", {0xFD, 0x96, 0x80});
    // SBC A, (IX/IY+d)
    ASSERT_CODE("SBC A, (IX+0)", {0xDD, 0x9E, 0x00});
    ASSERT_CODE("SBC A, (IY+127)", {0xFD, 0x9E, 0x7F});
    // AND/XOR/OR/CP (IX/IY+d)
    ASSERT_CODE("AND (IX+20)", {0xDD, 0xA6, 0x14});
    ASSERT_CODE("XOR (IY-30)", {0xFD, 0xAE, 0xE2});
    ASSERT_CODE("OR (IX+7)", {0xDD, 0xB6, 0x07});
    ASSERT_CODE("CP (IY-1)", {0xFD, 0xBE, 0xFF});
}

TEST_CASE(TwoOperandInstructions_JumpsAndCalls) {
    // JP cc, nn
    ASSERT_CODE("JP NZ, 0x1234", {0xC2, 0x34, 0x12});
    ASSERT_CODE("JP Z, 0x1234", {0xCA, 0x34, 0x12});
    ASSERT_CODE("JP NC, 0x1234", {0xD2, 0x34, 0x12});
    ASSERT_CODE("JP C, 0x1234", {0xDA, 0x34, 0x12});
    ASSERT_CODE("JP PO, 0x1234", {0xE2, 0x34, 0x12});
    ASSERT_CODE("JP PE, 0x1234", {0xEA, 0x34, 0x12});
    ASSERT_CODE("JP P, 0x1234", {0xF2, 0x34, 0x12});
    ASSERT_CODE("JP M, 0x1234", {0xFA, 0x34, 0x12});

    // JR cc, d
    ASSERT_CODE("JR NZ, 0x0010", {0x20, 0x0E}); // 16 - (0+2) = 14
    ASSERT_CODE("JR Z, 0x0010", {0x28, 0x0E});
    ASSERT_CODE("JR NC, 0x0010", {0x30, 0x0E});
    ASSERT_CODE("JR C, 0x0010", {0x38, 0x0E});

    // CALL cc, nn
    ASSERT_CODE("CALL NZ, 0x1234", {0xC4, 0x34, 0x12});
    ASSERT_CODE("CALL Z, 0x1234", {0xCC, 0x34, 0x12});
    ASSERT_CODE("CALL NC, 0x1234", {0xD4, 0x34, 0x12});
    ASSERT_CODE("CALL C, 0x1234", {0xDC, 0x34, 0x12});
    ASSERT_CODE("CALL PO, 0x1234", {0xE4, 0x34, 0x12});
    ASSERT_CODE("CALL PE, 0x1234", {0xEC, 0x34, 0x12});
    ASSERT_CODE("CALL P, 0x1234", {0xF4, 0x34, 0x12});
    ASSERT_CODE("CALL M, 0x1234", {0xFC, 0x34, 0x12});
}

TEST_CASE(TwoOperandInstructions_IO) {
    // IN A, (n)
    ASSERT_CODE("IN A, (0x12)", {0xDB, 0x12});

    // OUT (n), A
    ASSERT_CODE("OUT (0x34), A", {0xD3, 0x34});

    // IN r, (C)
    ASSERT_CODE("IN A, (C)", {0xED, 0x78});
    ASSERT_CODE("IN B, (C)", {0xED, 0x40});
    ASSERT_CODE("IN C, (C)", {0xED, 0x48});
    ASSERT_CODE("IN D, (C)", {0xED, 0x50});
    ASSERT_CODE("IN E, (C)", {0xED, 0x58});
    ASSERT_CODE("IN H, (C)", {0xED, 0x60});
    ASSERT_CODE("IN L, (C)", {0xED, 0x68});
    ASSERT_CODE("IN (C)", {0xED, 0x70});

    // OUT (C), r
    ASSERT_CODE("OUT (C), A", {0xED, 0x79});
    ASSERT_CODE("OUT (C), B", {0xED, 0x41});
    ASSERT_CODE("OUT (C), C", {0xED, 0x49});
    ASSERT_CODE("OUT (C), D", {0xED, 0x51});
    ASSERT_CODE("OUT (C), E", {0xED, 0x59});
    ASSERT_CODE("OUT (C), H", {0xED, 0x61});
    ASSERT_CODE("OUT (C), L", {0xED, 0x69});
}

TEST_CASE(BitInstructions) {
    // BIT b, r
    ASSERT_CODE("BIT 0, A", {0xCB, 0x47});
    ASSERT_CODE("BIT 7, B", {0xCB, 0x78});
    ASSERT_CODE("BIT 3, (HL)", {0xCB, 0x5E});

    // SET b, r
    ASSERT_CODE("SET 1, C", {0xCB, 0xC9});
    ASSERT_CODE("SET 6, D", {0xCB, 0xF2});
    ASSERT_CODE("SET 2, (HL)", {0xCB, 0xD6});

    // RES b, r
    ASSERT_CODE("RES 2, E", {0xCB, 0x93});
    ASSERT_CODE("RES 5, H", {0xCB, 0xAC});
    ASSERT_CODE("RES 0, (HL)", {0xCB, 0x86});

    // BIT b, (IX/IY+d)
    ASSERT_CODE("BIT 0, (IX+3)", {0xDD, 0xCB, 0x03, 0x46});
    ASSERT_CODE("BIT 7, (IY-1)", {0xFD, 0xCB, 0xFF, 0x7E});

    // SET b, (IX/IY+d)
    ASSERT_CODE("SET 1, (IX+4)", {0xDD, 0xCB, 0x04, 0xCE});
    ASSERT_CODE("SET 6, (IY-5)", {0xFD, 0xCB, 0xFB, 0xF6});

    // RES b, (IX/IY+d)
    ASSERT_CODE("RES 2, (IX+6)", {0xDD, 0xCB, 0x06, 0x96});
    ASSERT_CODE("RES 5, (IY-7)", {0xFD, 0xCB, 0xF9, 0xAE});
}

TEST_CASE(RotateAndShiftInstructions) {
    // RLC r
    ASSERT_CODE("RLC A", {0xCB, 0x07});
    ASSERT_CODE("RLC B", {0xCB, 0x00});
    ASSERT_CODE("RLC (HL)", {0xCB, 0x06});

    // RRC r
    ASSERT_CODE("RRC C", {0xCB, 0x09});
    ASSERT_CODE("RRC (HL)", {0xCB, 0x0E});

    // RL r
    ASSERT_CODE("RL D", {0xCB, 0x12});
    ASSERT_CODE("RL (HL)", {0xCB, 0x16});

    // RR r
    ASSERT_CODE("RR E", {0xCB, 0x1B});
    ASSERT_CODE("RR (HL)", {0xCB, 0x1E});

    // SLA r
    ASSERT_CODE("SLA H", {0xCB, 0x24});
    ASSERT_CODE("SLA (HL)", {0xCB, 0x26});

    // SRA r
    ASSERT_CODE("SRA L", {0xCB, 0x2D});
    ASSERT_CODE("SRA (HL)", {0xCB, 0x2E});

    // SLL/SLI r
    ASSERT_CODE("SLL A", {0xCB, 0x37});
    ASSERT_CODE("SLI A", {0xCB, 0x37}); // SLI is an alias for SLL
    ASSERT_CODE("SLL (HL)", {0xCB, 0x36});

    // SRL r
    ASSERT_CODE("SRL B", {0xCB, 0x38});
    ASSERT_CODE("SRL (HL)", {0xCB, 0x3E});
}

TEST_CASE(Directives) {
    // DB
    ASSERT_CODE("DB 0x12", {0x12});
    ASSERT_CODE("DB 0x12, 0x34, 0x56", {0x12, 0x34, 0x56});
    ASSERT_CODE("DB 'A'", {0x41});
    ASSERT_CODE("DB \"Hello\"", {0x48, 0x65, 0x6C, 0x6C, 0x6F});
    ASSERT_CODE("DB \"Hi\", 0, '!'", {0x48, 0x69, 0x00, 0x21});

    // DW
    ASSERT_CODE("DW 0x1234", {0x34, 0x12});
    ASSERT_CODE("DW 0x1234, 0x5678", {0x34, 0x12, 0x78, 0x56});
    ASSERT_CODE("DW 'a'", {0x61, 0x00});

    // DS
    ASSERT_CODE("DS 5", {0x00, 0x00, 0x00, 0x00, 0x00});
    ASSERT_CODE("DS 3, 0xFF", {0xFF, 0xFF, 0xFF});
}

TEST_CASE(LabelsAndExpressions) {
    std::string code = R"(
        ORG 0x100
    START:
        LD A, 5
        LD B, A
        ADD A, B
        LD (VALUE), A ; VALUE is at 0x10A
        JP END        ; END is at 0x10B
    VALUE:
        DB 0
    END:
        HALT
    )";
    std::vector<uint8_t> expected = {
        0x3E, 0x05,       // LD A, 5
        0x47,             // LD B, A
        0x80,             // ADD A, B
        0x32, 0x0A, 0x01, // LD (VALUE), A
        0xC3, 0x0B, 0x01, // JP END
        0x00,             // DB 0
        0x76              // HALT
    };

    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);
    bool success = assembler.compile(code);
    assert(success && "Compilation with labels failed");

    auto blocks = assembler.get_blocks();
    assert(blocks.size() == 1 && "Expected one code block");
    assert(blocks[0].first == 0x100 && "Block should start at 0x100");
    assert(blocks[0].second == expected.size() && "Incorrect compiled size");

    bool mismatch = false;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (bus.peek(0x100 + i) != expected[i]) {
            mismatch = true;
            break;
        }
    }

    if (mismatch) {
        std::cerr << "Assertion failed: Byte mismatch for 'LabelsAndExpressions' test\n";
        std::cerr << "  Expected: ";
        for (uint8_t byte : expected) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
        std::cerr << "\n  Got:      ";
        for (size_t i = 0; i < expected.size(); ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)bus.peek(0x100 + i) << " ";
        std::cerr << std::dec << "\n";
        tests_failed++;
    } else {
        tests_passed++;
    }
}

TEST_CASE(EQUAndSETDirectives) {
    std::string code = R"(
        PORTA EQU 0x10
        VAL EQU 5
        LD A, VAL
        OUT (PORTA), A
    )";
    std::vector<uint8_t> expected = {
        0x3E, 0x05, // LD A, 5
        0xD3, 0x10  // OUT (0x10), A
    };
    ASSERT_CODE(code, expected);
}

TEST_CASE(ExpressionEvaluation) {
    std::string code = R"(
        VAL1 EQU 10
        VAL2 EQU 2
        LD A, VAL1 * VAL2 + 5 ; 25
        LD B, (VAL1 + VAL2) / 3 ; 4
        LD C, VAL1 & 0x0C ; 8
    )";
    std::vector<uint8_t> expected = {
        0x3E, 25, // LD A, 25
        0x06, 4,  // LD B, 4
        0x0E, 8   // LD C, 8
    };
    ASSERT_CODE(code, expected);
}

TEST_CASE(ComprehensiveExpressionEvaluation) {
    // Test basic arithmetic operators
    ASSERT_CODE("VAL EQU 10 - 5\nLD A, VAL", {0x3E, 5});
    ASSERT_CODE("VAL EQU 10 * 2\nLD A, VAL", {0x3E, 20});
    ASSERT_CODE("VAL EQU 20 / 4\nLD A, VAL", {0x3E, 5});
    ASSERT_CODE("VAL EQU 21 % 5\nLD A, VAL", {0x3E, 1});

    // Test bitwise operators
    ASSERT_CODE("VAL EQU 0b1100 | 0b0101\nLD A, VAL", {0x3E, 0b1101}); // 13
    ASSERT_CODE("VAL EQU 0b1100 & 0b0101\nLD A, VAL", {0x3E, 0b0100}); // 4
    ASSERT_CODE("VAL EQU 0b1100 ^ 0b0101\nLD A, VAL", {0x3E, 0b1001}); // 9
    ASSERT_CODE("VAL EQU 5 << 2\nLD A, VAL", {0x3E, 20});
    ASSERT_CODE("VAL EQU 20 >> 1\nLD A, VAL", {0x3E, 10});

    // Test operator precedence
    ASSERT_CODE("VAL EQU 2 + 3 * 4\nLD A, VAL", {0x3E, 14}); // 2 + 12
    ASSERT_CODE("VAL EQU 10 | 1 & 12\nLD A, VAL", {0x3E, 10}); // 10 | (1 & 12) = 10 | 0 = 10

    // Test parentheses
    ASSERT_CODE("VAL EQU (2 + 3) * 4\nLD A, VAL", {0x3E, 20});
    ASSERT_CODE("VAL EQU (10 | 1) & 12\nLD A, VAL", {0x3E, 8}); // 11 & 12 = 8

    // Test complex expression
    ASSERT_CODE(R"(
        VAL1 EQU 10
        VAL2 EQU 2
        VAL3 EQU (VAL1 + 5) * VAL2 / (10 - 5) ; (15 * 2) / 5 = 30 / 5 = 6
        LD A, VAL3
    )", {0x3E, 6});

    // Test negative numbers (as 0 - n)
    ASSERT_CODE("LD A, 0-5", {0x3E, (uint8_t)-5}); // 0xFB

    // Test a very complex expression
    ASSERT_CODE(R"(
        V1 EQU 5
        V2 EQU 10
        V3 EQU 0x40
        ; Expression: (((5 << 2) + (10 * 3)) & 0x7F) | (0x40 - (20 / 2))
        ;             ((( 20 )   + (  30  )) & 0x7F) | (0x40 - (  10  ))
        ;             ((      50          ) & 0x7F) | (     0x36      )
        ;             (      0x32          & 0x7F) | (     0x36      ) -> 0x32 | 0x36 = 0x36
        COMPLEX_VAL EQU (((V1 << 2) + (V2 * 3)) & 0x7F) | (V3 - (20 / 2))
        LD A, COMPLEX_VAL
    )", {0x3E, 0x36});
}

TEST_CASE(ForwardReferences) {
    std::string code = R"(
        JP TARGET
        NOP
        NOP
    TARGET:
        LD A, 1
    )";
    std::vector<uint8_t> expected = {
        0xC3, 0x05, 0x00, // JP 0x0005
        0x00,
        0x00,
        0x3E, 0x01
    };
    ASSERT_CODE(code, expected);
}

TEST_CASE(CyclicDependency) {
    ASSERT_COMPILE_FAILS(R"(
        VAL1 EQU VAL2
        VAL2 EQU VAL1
        LD A, VAL1
    )");
    ASSERT_COMPILE_FAILS(R"(
        VAL1 EQU VAL2 + 1
        VAL2 EQU VAL1 - 1
        LD A, VAL1
    )");
}

int main() {
    std::cout << "=============================\n";
    std::cout << "  Running Z80Assembler Tests \n";
    std::cout << "=============================\n";

    // The test cases are automatically registered and run here.

    std::cout << "\n=============================\n";
    std::cout << "Test summary:\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";
    std::cout << "=============================\n";

    return (tests_failed > 0) ? 1 : 0;
}
