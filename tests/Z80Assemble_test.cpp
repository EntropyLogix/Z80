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
#include <map>
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

class MockSourceProvider : public ISourceProvider {
public:
    bool get_source(const std::string& identifier, std::string& source) override {
        if (m_sources.count(identifier)) {
            source = m_sources[identifier];
            return true;
        }
        return false;
    }

    void add_source(const std::string& identifier, const std::string& content) {
        m_sources[identifier] = content;
    }

private:
    std::map<std::string, std::string> m_sources;
};

void ASSERT_CODE(const std::string& asm_code, const std::vector<uint8_t>& expected_bytes) {
    Z80DefaultBus bus;
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", asm_code);
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    bool success = assembler.compile("main.asm", 0x0000);
    if (!success) {
        std::cerr << "Assertion failed: Compilation failed for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    size_t compiled_size = 0;
    if (!blocks.empty()) {
        compiled_size = blocks[0].size;
    }

    if (compiled_size != expected_bytes.size()) {
        std::cerr << "Assertion failed: Incorrect compiled size for '" << asm_code << "'.\n";
        std::cerr << "  Expected size: " << expected_bytes.size() << ", Got: " << compiled_size << "\n";
        tests_failed++;
        return;
    }

    uint16_t start_address = 0x0000;
    if (!blocks.empty()) {
        start_address = blocks[0].start_address;
    }

    bool mismatch = false;
    for (size_t i = 0; i < expected_bytes.size(); ++i) {
        if (bus.peek(start_address + i) != expected_bytes[i]) {
            mismatch = true;
            break;
        }
    }

    if (mismatch) {
        std::cerr << "Assertion failed: Byte mismatch for '" << asm_code << "'\n";
        std::cerr << "  Expected: ";
        for (uint8_t byte : expected_bytes) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
        std::cerr << "\n  Got:      ";
        for (size_t i = 0; i < expected_bytes.size(); ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)bus.peek(start_address + i) << " ";
        std::cerr << std::dec << "\n";
        tests_failed++;
    } else {
        tests_passed++;
    }
}

void ASSERT_COMPILE_FAILS_WITH_OPTS(const std::string& asm_code, const Z80Assembler<Z80DefaultBus>::Options& options) {
    Z80DefaultBus bus;
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", asm_code);
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider, options);
    try {
        bool success = assembler.compile("main.asm", 0x0000);
        if (success) {
            std::cerr << "Assertion failed: Compilation succeeded for '" << asm_code << "' but was expected to fail.\n";
            tests_failed++;
        } else {
            tests_passed++;
        }
    } catch (const std::runtime_error& e) {
        tests_passed++;
    } catch (...) {
        std::cerr << "Assertion failed: An unexpected exception was thrown for '" << asm_code << "'.\n";
        tests_failed++;
    }
}

void ASSERT_COMPILE_FAILS(const std::string& asm_code) {
    Z80DefaultBus bus;
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", asm_code);
    Z80Assembler<Z80DefaultBus>::Options options;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider, options);
    try {
        bool success = assembler.compile("main.asm", 0x0000);
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

template<typename T>
std::string to_binary_string(T n) {
    if (n == 0) return "0";
    std::string binary;
    constexpr size_t bits = sizeof(T) * 8;
    for (size_t i = 0; i < bits; ++i) {
        if ((n >> (bits - 1 - i)) & 1) {
            binary += '1';
        } else if (!binary.empty()) {
            binary += '0';
        }
    }
    return binary.empty() ? "0" : binary;
}

void TEST_IMMEDIATE_8BIT(const std::string& instruction_format, const std::vector<uint8_t>& opcode_prefix) {
    auto test_value = [&](int value) {
        std::vector<std::string> formats;
        // Decimal
        formats.push_back(std::to_string(value));
        // Hex
        std::stringstream ss_hex;
        if (value < 0) ss_hex << "-";
        ss_hex << "0x" << std::hex << (value < 0 ? -value : value);
        formats.push_back(ss_hex.str());
        // Binary
        std::string bin_str = "0b" + to_binary_string(value < 0 ? -value : value);
        if (value < 0) bin_str.insert(0, "-");
        formats.push_back(bin_str);

        for (const auto& value_str : formats) {
            std::string code = instruction_format;
            size_t pos = code.find("{}");
            if (pos != std::string::npos) {
                code.replace(pos, 2, value_str);
            }
            std::vector<uint8_t> expected_bytes = opcode_prefix;
            expected_bytes.push_back(static_cast<uint8_t>(value));
            ASSERT_CODE(code, expected_bytes);
        }
    };

    for (int i = 0; i <= 255; ++i) {
        test_value(i);
    }
    for (int i = -128; i < 0; ++i) {
        test_value(i);
    }
}

void TEST_IMMEDIATE_16BIT(const std::string& instruction_format, const std::vector<uint8_t>& opcode_prefix) {
    // WARNING: This is a very long-running test, iterating through all 65536 values.
    auto test_value = [&](long value) {
        std::vector<std::string> formats;
        formats.push_back(std::to_string(value));
        std::stringstream ss_hex;
        if (value < 0) ss_hex << "-";
        ss_hex << "0x" << std::hex << (value < 0 ? -value : value);
        formats.push_back(ss_hex.str());
        std::string bin_str = "0b" + to_binary_string(value < 0 ? -value : value);
        if (value < 0) bin_str.insert(0, "-");
        formats.push_back(bin_str);

        for (const auto& value_str : formats) {
            std::string code = instruction_format;
            size_t pos = code.find("{}");
            if (pos != std::string::npos) code.replace(pos, 2, value_str);
            std::vector<uint8_t> expected_bytes = opcode_prefix;
            expected_bytes.push_back(static_cast<uint8_t>(value & 0xFF));
            expected_bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            ASSERT_CODE(code, expected_bytes);
        }
    };
    for (long i = 0; i <= 65535; ++i) {
        test_value(i);
    }
    for (long i = -32768; i < 0; ++i) {
        test_value(i);
    }
}

void TEST_INDEXED_IMMEDIATE_8BIT(const std::string& instruction_format, uint8_t prefix, uint8_t opcode, bool full_test = false) {
    auto test_displacement = [&](int d) {
        auto test_value_n = [&](int n) {
            std::vector<std::string> d_formats;
            d_formats.push_back((d >= 0 ? "+" : "") + std::to_string(d));
            
            std::stringstream ss_hex_d;
            ss_hex_d << (d >= 0 ? "+" : "-") << "0x" << std::hex << (d < 0 ? -d : d);
            d_formats.push_back(ss_hex_d.str());

            std::vector<std::string> n_formats;
            n_formats.push_back(std::to_string(n));
            std::stringstream ss_hex_n;
            ss_hex_n << "0x" << std::hex << n;
            n_formats.push_back(ss_hex_n.str());
            n_formats.push_back("0b" + to_binary_string(static_cast<uint8_t>(n)));

            for (const auto& d_str : d_formats) {
                for (const auto& n_str : n_formats) {
                    std::string code = instruction_format;
                    size_t pos_d = code.find("{d}");
                    if (pos_d != std::string::npos) code.replace(pos_d, 3, d_str);
                    size_t pos_n = code.find("{n}");
                    if (pos_n != std::string::npos) code.replace(pos_n, 3, n_str);
                    
                    std::vector<uint8_t> expected_bytes;
                    if (prefix) expected_bytes.push_back(prefix);
                    expected_bytes.push_back(opcode);
                    expected_bytes.push_back(static_cast<uint8_t>(d));
                    expected_bytes.push_back(static_cast<uint8_t>(n));
                    ASSERT_CODE(code, expected_bytes);
                }
            }
        };

        // Test all 256 possible values for the immediate operand 'n'
        for (int i = 0; i <= 255; ++i) {
            test_value_n(i);
        }
    };

    if (full_test) {
        // Test all 256 possible displacement values. This is slow.
        for (int d = -128; d <= 127; ++d) {
            test_displacement(d);
        }
    } else {
        // Test a few representative displacement values for a quick check.
        int representative_displacements[] = {0, 1, -1, 10, -20, 127, -128};
        for (int d : representative_displacements) {
            test_displacement(d);
        }
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
    const char* registers[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    for (int i = 0; i < 8; ++i) {
        ASSERT_CODE("INC " + std::string(registers[i]), { (uint8_t)(0x04 | (i << 3)) });
        ASSERT_CODE("DEC " + std::string(registers[i]), { (uint8_t)(0x05 | (i << 3)) });
    }
    ASSERT_CODE("INC (HL)", {0x34});
    ASSERT_CODE("DEC (HL)", {0x35});

    // Jumps
    ASSERT_CODE("JP (HL)", {0xE9});
    ASSERT_CODE("JP (IX)", {0xDD, 0xE9});
    ASSERT_CODE("JP (IY)", {0xFD, 0xE9});
    ASSERT_CODE("JR 0x0005", {0x18, 0x03}); // 5 - (0+2) = 3
    ASSERT_CODE("JR 0x0000", {0x18, 0xFE}); // 0 - (0+2) = -2

    // Calls
    // RST
    ASSERT_CODE("RST 0x00", {0xC7});
    ASSERT_CODE("RST 0x08", {0xCF});
    ASSERT_CODE("RST 0x10", {0xD7});
    ASSERT_CODE("RST 0x18", {0xDF});
    ASSERT_CODE("RST 0x20", {0xE7});
    ASSERT_CODE("RST 0x28", {0xEF});
    ASSERT_CODE("RST 0x30", {0xF7});
    ASSERT_CODE("RST 0x38", {0xFF});

    // Arithmetic/Logic with register
    ASSERT_CODE("ADD A, B", {0x80});
    ASSERT_CODE("ADD A, C", {0x81});
    ASSERT_CODE("ADD A, D", {0x82});
    ASSERT_CODE("ADD A, E", {0x83});
    ASSERT_CODE("ADD A, H", {0x84});
    ASSERT_CODE("ADD A, L", {0x85});
    ASSERT_CODE("ADD A, (HL)", {0x86});
    ASSERT_CODE("ADD A, A", {0x87});    
    ASSERT_CODE("ADD B", {0x80}); // Implicit A
    ASSERT_CODE("SUB A, B", {0x90});
    ASSERT_CODE("SUB C", {0x91});
    ASSERT_CODE("SUB D", {0x92});
    ASSERT_CODE("SUB E", {0x93});
    ASSERT_CODE("SUB H", {0x94});
    ASSERT_CODE("SUB L", {0x95});
    ASSERT_CODE("SUB (HL)", {0x96});    
    ASSERT_CODE("SUB A, A", {0x97});
    ASSERT_CODE("ADC A, B", {0x88});
    ASSERT_CODE("ADC B", {0x88});
    ASSERT_CODE("ADC C", {0x89});
    ASSERT_CODE("ADC D", {0x8A});
    ASSERT_CODE("ADC E", {0x8B});
    ASSERT_CODE("ADC H", {0x8C});
    ASSERT_CODE("ADC L", {0x8D});
    ASSERT_CODE("ADC (HL)", {0x8E});    
    ASSERT_CODE("ADC A, A", {0x8F});
    ASSERT_CODE("SBC A, B", {0x98});
    ASSERT_CODE("SBC B", {0x98});
    ASSERT_CODE("SBC (HL)", {0x9E});    
    ASSERT_CODE("SBC A, A", {0x9F});
    ASSERT_CODE("AND A, B", {0xA0});
    ASSERT_CODE("AND C", {0xA1});
    ASSERT_CODE("AND (HL)", {0xA6});    
    ASSERT_CODE("AND A", {0xA7});
    ASSERT_CODE("OR D", {0xB2});
    ASSERT_CODE("OR (HL)", {0xB6});    
    ASSERT_CODE("OR A", {0xB7});
    ASSERT_CODE("XOR E", {0xAB});
    ASSERT_CODE("XOR (HL)", {0xAE});    
    ASSERT_CODE("XOR A", {0xAF});
    ASSERT_CODE("CP H", {0xBC});
    ASSERT_CODE("CP (HL)", {0xBE});
    ASSERT_CODE("CP A", {0xBF});

    // Arithmetic/Logic with IX/IY parts
    ASSERT_CODE("ADD A, IXH", {0xDD, 0x84});
    ASSERT_CODE("ADD A, IXL", {0xDD, 0x85});
    ASSERT_CODE("ADD A, IYH", {0xFD, 0x84});
    ASSERT_CODE("ADD A, IYL", {0xFD, 0x85});
    ASSERT_CODE("ADC A, IXH", {0xDD, 0x8C});
    ASSERT_CODE("SUB IXL", {0xDD, 0x95});
    ASSERT_CODE("SBC A, IYH", {0xFD, 0x9C});
    ASSERT_CODE("AND IXH", {0xDD, 0xA4});
    ASSERT_CODE("XOR IXL", {0xDD, 0xAD});
    ASSERT_CODE("OR IYH", {0xFD, 0xB4});
    ASSERT_CODE("CP IYL", {0xFD, 0xBD});
    // Test mixed explicit/implicit 'A'
    ASSERT_CODE("SUB A, IXH", {0xDD, 0x94});
    ASSERT_CODE("AND A, IYL", {0xFD, 0xA5});
    ASSERT_CODE("OR A, IXH", {0xDD, 0xB4});
    ASSERT_CODE("CP A, IXL", {0xDD, 0xBD});

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
TEST_CASE(OneOperandInstructions_Immediate) {
    // Arithmetic/Logic with immediate
    TEST_IMMEDIATE_8BIT("ADD A, {}", {0xC6});
    TEST_IMMEDIATE_8BIT("ADD {}", {0xC6}); // Implicit A
    TEST_IMMEDIATE_8BIT("ADC A, {}", {0xCE});
    TEST_IMMEDIATE_8BIT("ADC {}", {0xCE}); // Implicit A
    TEST_IMMEDIATE_8BIT("SUB A, {}", {0xD6});
    TEST_IMMEDIATE_8BIT("SUB {}", {0xD6});
    TEST_IMMEDIATE_8BIT("SBC A, {}", {0xDE});
    TEST_IMMEDIATE_8BIT("SBC {}", {0xDE}); // Implicit A
    TEST_IMMEDIATE_8BIT("AND {}", {0xE6});
    TEST_IMMEDIATE_8BIT("AND A, {}", {0xE6}); // Explicit A
    TEST_IMMEDIATE_8BIT("XOR {}", {0xEE});
    TEST_IMMEDIATE_8BIT("XOR A, {}", {0xEE}); // Explicit A
    TEST_IMMEDIATE_8BIT("OR {}", {0xF6});
    TEST_IMMEDIATE_8BIT("OR A, {}", {0xF6}); // Explicit A
    TEST_IMMEDIATE_8BIT("CP {}", {0xFE});
    TEST_IMMEDIATE_8BIT("CP A, {}", {0xFE}); // Explicit A
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
    TEST_IMMEDIATE_8BIT("LD A, {}", {0x3E});
    TEST_IMMEDIATE_8BIT("LD B, {}", {0x06});
    TEST_IMMEDIATE_8BIT("LD C, {}", {0x0E});
    TEST_IMMEDIATE_8BIT("LD D, {}", {0x16});
    TEST_IMMEDIATE_8BIT("LD E, {}", {0x1E});
    TEST_IMMEDIATE_8BIT("LD H, {}", {0x26});
    TEST_IMMEDIATE_8BIT("LD L, {}", {0x2E});

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
    TEST_IMMEDIATE_8BIT("LD (HL), {}", {0x36});

    // LD A, (rr)
    ASSERT_CODE("LD A, (BC)", {0x0A});
    ASSERT_CODE("LD A, (DE)", {0x1A});

    // LD (rr), A
    ASSERT_CODE("LD (BC), A", {0x02});
    ASSERT_CODE("LD (DE), A", {0x12});

    // LD A, (nn)
    // LD (nn), A

    // LD rr, nn

    // LD rr, (nn)

    // LD (nn), rr

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

TEST_CASE(TwoOperandInstructions_LD_Immediate16) {
    // LD rr, nn
    TEST_IMMEDIATE_16BIT("LD BC, {}", {0x01});
    TEST_IMMEDIATE_16BIT("LD DE, {}", {0x11});
    TEST_IMMEDIATE_16BIT("LD HL, {}", {0x21});
    TEST_IMMEDIATE_16BIT("LD SP, {}", {0x31});

    // LD A, (nn) and LD (nn), A
    TEST_IMMEDIATE_16BIT("LD A, ({})", {0x3A});
    TEST_IMMEDIATE_16BIT("LD ({}), A", {0x32});

    // LD rr, (nn)
    TEST_IMMEDIATE_16BIT("LD HL, ({})", {0x2A});
    TEST_IMMEDIATE_16BIT("LD BC, ({})", {0xED, 0x4B});
    TEST_IMMEDIATE_16BIT("LD DE, ({})", {0xED, 0x5B});
    TEST_IMMEDIATE_16BIT("LD SP, ({})", {0xED, 0x7B});

    // LD (nn), rr
    TEST_IMMEDIATE_16BIT("LD ({}), HL", {0x22});
    TEST_IMMEDIATE_16BIT("LD ({}), BC", {0xED, 0x43});
    TEST_IMMEDIATE_16BIT("LD ({}), DE", {0xED, 0x53});
    TEST_IMMEDIATE_16BIT("LD ({}), SP", {0xED, 0x73});
}

TEST_CASE(TwoOperandInstructions_LD_Indexed) {
    // LD IX/IY, nn
    TEST_IMMEDIATE_16BIT("LD IX, {}", {0xDD, 0x21});
    TEST_IMMEDIATE_16BIT("LD IY, {}", {0xFD, 0x21});

    // LD IX/IY, (nn)
    TEST_IMMEDIATE_16BIT("LD IX, ({})", {0xDD, 0x2A});
    TEST_IMMEDIATE_16BIT("LD IY, ({})", {0xFD, 0x2A});

    // LD (nn), IX/IY
    TEST_IMMEDIATE_16BIT("LD ({}), IX", {0xDD, 0x22});
    TEST_IMMEDIATE_16BIT("LD ({}), IY", {0xFD, 0x22});

    // LD r, (IX/IY+d)
    ASSERT_CODE("LD A, (IX+10)", {0xDD, 0x7E, 0x0A});
    ASSERT_CODE("LD B, (IX-20)", {0xDD, 0x46, 0xEC}); // -20 = 0xEC
    ASSERT_CODE("LD C, (IY+0)", {0xFD, 0x4E, 0x00});
    ASSERT_CODE("LD D, (IY+127)", {0xFD, 0x56, 0x7F});    
    ASSERT_CODE("LD E, (IX+1)", {0xDD, 0x5E, 0x01});
    ASSERT_CODE("LD H, (IY+2)", {0xFD, 0x66, 0x02});
    ASSERT_CODE("LD L, (IX+3)", {0xDD, 0x6E, 0x03});

    // LD (IX/IY+d), r
    ASSERT_CODE("LD (IX+5), A", {0xDD, 0x77, 0x05});
    ASSERT_CODE("LD (IX-8), B", {0xDD, 0x70, 0xF8});
    ASSERT_CODE("LD (IY+0), C", {0xFD, 0x71, 0x00});
    ASSERT_CODE("LD (IY+127), D", {0xFD, 0x72, 0x7F});    
    ASSERT_CODE("LD (IX+1), E", {0xDD, 0x73, 0x01});
    ASSERT_CODE("LD (IY+2), H", {0xFD, 0x74, 0x02});
    ASSERT_CODE("LD (IX+3), L", {0xDD, 0x75, 0x03});    
    TEST_INDEXED_IMMEDIATE_8BIT("LD (IX{d}), {n}", 0xDD, 0x36);
    TEST_INDEXED_IMMEDIATE_8BIT("LD (IY{d}), {n}", 0xFD, 0x36);

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
    TEST_IMMEDIATE_16BIT("JP {}", {0xC3});
    TEST_IMMEDIATE_16BIT("JP NZ, {}", {0xC2});
    TEST_IMMEDIATE_16BIT("JP Z, {}", {0xCA});
    TEST_IMMEDIATE_16BIT("JP NC, {}", {0xD2});
    TEST_IMMEDIATE_16BIT("JP C, {}", {0xDA});
    TEST_IMMEDIATE_16BIT("JP PO, {}", {0xE2});
    TEST_IMMEDIATE_16BIT("JP PE, {}", {0xEA});
    TEST_IMMEDIATE_16BIT("JP P, {}", {0xF2});
    TEST_IMMEDIATE_16BIT("JP M, {}", {0xFA});

    // JR cc, d
    ASSERT_CODE("JR NZ, 0x0010", {0x20, 0x0E}); // 16 - (0+2) = 14
    ASSERT_CODE("JR Z, 0x0010", {0x28, 0x0E});
    ASSERT_CODE("JR NC, 0x0010", {0x30, 0x0E});
    ASSERT_CODE("JR C, 0x0010", {0x38, 0x0E});
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

TEST_CASE(TwoOperandInstructions_Calls) {
    // CALL nn
    TEST_IMMEDIATE_16BIT("CALL {}", {0xCD});

    // CALL cc, nn
    TEST_IMMEDIATE_16BIT("CALL NZ, {}", {0xC4});
    TEST_IMMEDIATE_16BIT("CALL Z, {}", {0xCC});
    TEST_IMMEDIATE_16BIT("CALL NC, {}", {0xD4});
    TEST_IMMEDIATE_16BIT("CALL C, {}", {0xDC});
    TEST_IMMEDIATE_16BIT("CALL PO, {}", {0xE4});
    TEST_IMMEDIATE_16BIT("CALL PE, {}", {0xEC});
    TEST_IMMEDIATE_16BIT("CALL P, {}", {0xF4});
    TEST_IMMEDIATE_16BIT("CALL M, {}", {0xFC});
}

TEST_CASE(BitInstructions) {
    // BIT b, r
    ASSERT_CODE("BIT 0, A", {0xCB, 0x47});
    ASSERT_CODE("BIT 7, A", {0xCB, 0x7F});
    ASSERT_CODE("BIT 7, B", {0xCB, 0x78});
    ASSERT_CODE("BIT 3, (HL)", {0xCB, 0x5E});
    ASSERT_CODE("BIT 0, (HL)", {0xCB, 0x46});

    // SET b, r
    ASSERT_CODE("SET 1, C", {0xCB, 0xC9});
    ASSERT_CODE("SET 0, A", {0xCB, 0xC7});
    ASSERT_CODE("SET 6, D", {0xCB, 0xF2});
    ASSERT_CODE("SET 2, (HL)", {0xCB, 0xD6});
    ASSERT_CODE("SET 7, (HL)", {0xCB, 0xFE});

    // RES b, r
    ASSERT_CODE("RES 2, E", {0xCB, 0x93});
    ASSERT_CODE("RES 7, A", {0xCB, 0xBF});
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

TEST_CASE(UndocumentedInstructions) {
    // SLI is an alias for SLL
    ASSERT_CODE("SLI A", {0xCB, 0x37});
    ASSERT_CODE("SLI (HL)", {0xCB, 0x36});

    // IN F,(C) can be written as IN (C)
    ASSERT_CODE("IN (C)", {0xED, 0x70});
    ASSERT_CODE("OUT (C), 0", {0xED, 0x71});
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

    // Aliases
    ASSERT_CODE("DEFB 0x12, 0x34", {0x12, 0x34});
    ASSERT_CODE("DEFW 0xABCD", {0xCD, 0xAB});
    ASSERT_CODE("DEFS 4", {0x00, 0x00, 0x00, 0x00});

    // More complex cases
    ASSERT_CODE("DB 1+2, 10-3", {0x03, 0x07});
    ASSERT_CODE("DB 'A'+1", {0x42});
    ASSERT_CODE(R"(
        VALUE EQU 0x1234
        DW VALUE, VALUE+1
    )", {0x34, 0x12, 0x35, 0x12});
    ASSERT_CODE(R"(
        ORG 0x100
        DW 0x1122, L1
    L1: DW 0x3344
    )", {0x22, 0x11, 0x04, 0x01, 0x44, 0x33});
    ASSERT_CODE("DS 2+2, 5*5", {0x19, 0x19, 0x19, 0x19});
    ASSERT_CODE(R"(
        COUNT EQU 3
        FILL EQU 0xEE
        DS COUNT, FILL
    )", {0xEE, 0xEE, 0xEE});
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
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", code);
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    bool success = assembler.compile("main.asm");
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
    ASSERT_CODE(R"(
        PORTA EQU 0x10
        VAL EQU 5
        LD A, VAL
        OUT (PORTA), A
    )", {
        0x3E, 0x05, // LD A, 5
        0xD3, 0x10  // OUT (0x10), A
    });

    // Test redefinition with EQU should fail
    ASSERT_COMPILE_FAILS(R"(
        VALUE EQU 10
        VALUE EQU 20
    )");
}

TEST_CASE(SETDirective) {
    // Basic SET
    ASSERT_CODE(R"(
        VALUE SET 10
        LD A, VALUE
    )", {0x3E, 10});

    // Redefinition with SET
    ASSERT_CODE(R"(
        VALUE SET 10
        VALUE SET 20
        LD A, VALUE
    )", {0x3E, 20});

    // SET with forward reference
    ASSERT_CODE(R"(
        VALUE_A SET VALUE_B + 1
        LD A, VALUE_A
        VALUE_B SET 5
    )", {0x3E, 6});

    // Mixing EQU and SET (should fail if EQU is redefined)
    ASSERT_COMPILE_FAILS("VAL EQU 1\nVAL SET 2");
    ASSERT_COMPILE_FAILS("VAL SET 1\nVAL EQU 2");
}

TEST_CASE(Comments) {
    // Test single-line semicolon comments
    ASSERT_CODE("LD A, 5 ; This is a comment", {0x3E, 0x05});
    ASSERT_CODE("; ENTIRE LINE COMMENT\nLD B, 10", {0x06, 0x0A});

    // Test multi-line block comments
    ASSERT_CODE(R"(
        LD A, 1       /* Start comment
        LD B, 2       This is all commented out
        LD C, 3       */ LD D, 4
    )", {0x3E, 0x01, 0x16, 0x04});

    // Test unterminated block comment
    ASSERT_COMPILE_FAILS("LD A, 1 /* This comment is not closed");
}

TEST_CASE(IndexedRegisterParts) {
    const char* regs[] = {"B", "C", "D", "E", "A"}; // H and L are special

    // LD r, IXH/L and LD r, IYH/L
    for (int i = 0; i < 5; ++i) { // B, C, D, E, A
        uint8_t reg_code = (i < 4) ? i : 7; // B=0, C=1, D=2, E=3, A=7
        // LD r, IXH is like LD r, H
        ASSERT_CODE("LD " + std::string(regs[i]) + ", IXH", {0xDD, (uint8_t)(0x40 | (reg_code << 3) | 4)});
        // LD r, IXL is like LD r, L
        ASSERT_CODE("LD " + std::string(regs[i]) + ", IXL", {0xDD, (uint8_t)(0x40 | (reg_code << 3) | 5)});
        // LD r, IYH is like LD r, H
        ASSERT_CODE("LD " + std::string(regs[i]) + ", IYH", {0xFD, (uint8_t)(0x40 | (reg_code << 3) | 4)});
        // LD r, IYL is like LD r, L
        ASSERT_CODE("LD " + std::string(regs[i]) + ", IYL", {0xFD, (uint8_t)(0x40 | (reg_code << 3) | 5)});
    }

    // LD IXH/L, r and LD IYH/L, r
    for (int i = 0; i < 5; ++i) { // B, C, D, E, A
        uint8_t reg_code = (i < 4) ? i : 7; // B=0, C=1, D=2, E=3, A=7
        // LD IXH, r is like LD H, r
        ASSERT_CODE("LD IXH, " + std::string(regs[i]), {0xDD, (uint8_t)(0x60 | reg_code)});
        // LD IXL, r is like LD L, r
        ASSERT_CODE("LD IXL, " + std::string(regs[i]), {0xDD, (uint8_t)(0x68 | reg_code)});
        // LD IYH, r is like LD H, r
        ASSERT_CODE("LD IYH, " + std::string(regs[i]), {0xFD, (uint8_t)(0x60 | reg_code)});
        // LD IYL, r is like LD L, r
        ASSERT_CODE("LD IYL, " + std::string(regs[i]), {0xFD, (uint8_t)(0x68 | reg_code)});
    }

    // LD IXH/L, n and LD IYH/L, n
    TEST_IMMEDIATE_8BIT("LD IXH, {}", {0xDD, 0x26});
    TEST_IMMEDIATE_8BIT("LD IXL, {}", {0xDD, 0x2E});
    TEST_IMMEDIATE_8BIT("LD IYH, {}", {0xFD, 0x26});
    TEST_IMMEDIATE_8BIT("LD IYL, {}", {0xFD, 0x2E});

    // INC/DEC IXH/L/IYH/L
    ASSERT_CODE("INC IXH", {0xDD, 0x24});
    ASSERT_CODE("DEC IXH", {0xDD, 0x25});
    ASSERT_CODE("INC IXL", {0xDD, 0x2C});
    ASSERT_CODE("DEC IXL", {0xDD, 0x2D});
    ASSERT_CODE("INC IYH", {0xFD, 0x24});
    ASSERT_CODE("DEC IYH", {0xFD, 0x25});
    ASSERT_CODE("INC IYL", {0xFD, 0x2C});
    ASSERT_CODE("DEC IYL", {0xFD, 0x2D});

    // Arithmetic and Logic
    const char* alu_mnemonics[] = {"ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP"};
    for (int i = 0; i < 8; ++i) {
        uint8_t base_opcode = 0x80 + (i * 8);
        std::string mnemonic = alu_mnemonics[i];
        // vs IX parts
        ASSERT_CODE(mnemonic + " A, IXH", {0xDD, (uint8_t)(base_opcode + 4)});
        ASSERT_CODE(mnemonic + " A, IXL", {0xDD, (uint8_t)(base_opcode + 5)});
        // vs IY parts
        ASSERT_CODE(mnemonic + " A, IYH", {0xFD, (uint8_t)(base_opcode + 4)});
        ASSERT_CODE(mnemonic + " A, IYL", {0xFD, (uint8_t)(base_opcode + 5)});
    }
}

TEST_CASE(RelativeJumpBoundaries) {
    // JR tests
    // Helper to test code with ORG directive
    auto assert_org_code = [](const std::string& asm_code, uint16_t org_addr, const std::vector<uint8_t>& expected_bytes) {
        Z80DefaultBus bus;
        MockSourceProvider source_provider;
        source_provider.add_source("main.asm", asm_code);
        Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
        assembler.compile("main.asm");
        bool mismatch = false;
        for (size_t i = 0; i < expected_bytes.size(); ++i) {
            if (bus.peek(org_addr + i) != expected_bytes[i]) {
                mismatch = true;
                break;
            }
        }
        assert(!mismatch && "Byte mismatch in assert_org_code");
    };
    assert_org_code("ORG 0x100\nJR 0x181", 0x100, {0x18, 0x7F}); // Max positive jump: 0x181 - (0x100 + 2) = 127
    assert_org_code("ORG 0x100\nJR 0x100", 0x100, {0x18, 0xFE}); // Jump to self: 0x100 - (0x100 + 2) = -2
    assert_org_code("ORG 0x180\nJR 0x102", 0x180, {0x18, 0x80}); // Max negative jump: 0x102 - (0x180 + 2) = -128

    // DJNZ tests
    assert_org_code("ORG 0x100\nDJNZ 0x181", 0x100, {0x10, 0x7F}); // Max positive jump
    assert_org_code("ORG 0x180\nDJNZ 0x102", 0x180, {0x10, 0x80}); // Max negative jump

    // Out of range tests
    ASSERT_COMPILE_FAILS("ORG 0x100\nJR 0x182"); // offset = 128, out of range
    ASSERT_COMPILE_FAILS("ORG 0x180\nJR 0x101"); // offset = -129, out of range
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

    // Test HIGH() and LOW() functions
    ASSERT_CODE("ADDR EQU 0x1234\nLD A, HIGH(ADDR)", {0x3E, 0x12});
    ASSERT_CODE("ADDR EQU 0x1234\nLD A, LOW(ADDR)", {0x3E, 0x34});
    ASSERT_CODE("LD A, HIGH(0xABCD)", {0x3E, 0xAB});
    ASSERT_CODE("LD A, LOW(0xABCD)", {0x3E, 0xCD});
    ASSERT_CODE("ADDR EQU 0x1234\nLD A, HIGH(ADDR+1)", {0x3E, 0x12});
    ASSERT_CODE("ADDR EQU 0x1234\nLD A, LOW(ADDR+1)", {0x3E, 0x35});

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

    // Test unary plus
    ASSERT_CODE("VAL EQU +5\nLD A, VAL", {0x3E, 5});
    ASSERT_CODE("VAL EQU 10 * +2\nLD A, VAL", {0x3E, 20});
    ASSERT_CODE("VAL EQU +(2+3)\nLD A, VAL", {0x3E, 5});
    ASSERT_CODE("VAL EQU -+5\nLD A, VAL", {0x3E, (uint8_t)-5});

    // Test bitwise NOT
    ASSERT_CODE("VAL EQU ~0\nLD A, VAL", {0x3E, (uint8_t)-1});
    ASSERT_CODE("VAL EQU ~0b01010101\nLD A, VAL", {0x3E, 0b10101010});
    ASSERT_CODE("VAL EQU 5 + ~2\nLD A, VAL", {0x3E, (uint8_t)(5 + ~2)});

    // Test comparison and logical operators
    ASSERT_CODE("VAL EQU 10 > 5\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU 5 < 10\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU 10 >= 10\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU 5 <= 5\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU 10 == 10\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU 10 != 5\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU (1 && 1)\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU (1 || 0)\nLD A, VAL", {0x3E, 1});
    ASSERT_CODE("VAL EQU (5 > 2) && (10 < 20)\nLD A, VAL", {0x3E, 1});
}

TEST_CASE(LogicalNOTOperator) {
    ASSERT_CODE("LD A, !1", {0x3E, 0});
    ASSERT_CODE("LD A, !0", {0x3E, 1});
    ASSERT_CODE("LD A, !5", {0x3E, 0});
    ASSERT_CODE("LD A, !-1", {0x3E, 0});
    ASSERT_CODE("LD A, !!1", {0x3E, 1});
    ASSERT_CODE("LD A, !!0", {0x3E, 0});
    ASSERT_CODE("LD A, !(1==1)", {0x3E, 0});
    ASSERT_CODE("LD A, !(1==0)", {0x3E, 1});
    ASSERT_CODE("VAL_A EQU 10\nLD A, !VAL_A", {0x3E, 0});
    ASSERT_CODE("VAL_B EQU 0\nLD A, !VAL_B", {0x3E, 1});
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

TEST_CASE(IncludeDirective_Basic) {
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", "LD A, 5\nINCLUDE \"included.asm\"\nADD A, B");
    source_provider.add_source("included.asm", "LD B, 10\n");

    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    assembler.compile("main.asm");

    std::vector<uint8_t> expected = {0x3E, 0x05, 0x06, 0x0A, 0x80};
    auto blocks = assembler.get_blocks();
    assert(blocks.size() == 1 && blocks[0].second == expected.size());
    bool mismatch = false;
    for(size_t i = 0; i < expected.size(); ++i) {
        if (bus.peek(i) != expected[i]) mismatch = true;
    }
    assert(!mismatch && "Basic include failed");
    tests_passed++;
}

TEST_CASE(IncludeDirective_Nested) {
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", "INCLUDE \"level1.asm\"");
    source_provider.add_source("level1.asm", "LD A, 1\nINCLUDE \"level2.asm\"");
    source_provider.add_source("level2.asm", "LD B, 2\n");

    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    assembler.compile("main.asm");

    std::vector<uint8_t> expected = {0x3E, 0x01, 0x06, 0x02};
    auto blocks = assembler.get_blocks();
    assert(blocks.size() == 1 && blocks[0].second == expected.size());
    bool mismatch = false;
    for(size_t i = 0; i < expected.size(); ++i) {
        if (bus.peek(i) != expected[i]) mismatch = true;
    }
    assert(!mismatch && "Nested include failed");
    tests_passed++;
}

TEST_CASE(IncludeDirective_CircularDependency) {
    MockSourceProvider source_provider;
    source_provider.add_source("a.asm", "INCLUDE \"b.asm\"");
    source_provider.add_source("b.asm", "INCLUDE \"a.asm\"");
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    try {
        assembler.compile("a.asm");
        tests_failed++;
        std::cerr << "Assertion failed: Circular dependency did not throw an exception.\n";
    } catch (const std::runtime_error&) {
        tests_passed++;
    }
}

TEST_CASE(ConditionalCompilation) {
    // Simple IF (true)
    ASSERT_CODE(R"(
        IF 1
            LD A, 1
        ENDIF
    )", {0x3E, 0x01});

    // Simple IF (false)
    ASSERT_CODE(R"(
        IF 0
            LD A, 1
        ENDIF
    )", {});

    // IF with expression
    ASSERT_CODE(R"(
        VALUE EQU 10
        IF VALUE > 5
            LD A, 1
        ENDIF
    )", {0x3E, 0x01});

    // IF with ELSE (IF part taken)
    ASSERT_CODE(R"(
        IF 1
            LD A, 1
        ELSE
            LD A, 2
        ENDIF
    )", {0x3E, 0x01});

    // IF with ELSE (ELSE part taken)
    ASSERT_CODE(R"(
        IF 0
            LD A, 1
        ELSE
            LD A, 2
        ENDIF
    )", {0x3E, 0x02});

    // IFDEF (defined)
    ASSERT_CODE(R"(
        MY_SYMBOL EQU 1
        IFDEF MY_SYMBOL
            LD A, 1
        ENDIF
    )", {0x3E, 0x01});

    // IFDEF (not defined)
    ASSERT_CODE(R"(
        IFDEF MY_UNDEFINED_SYMBOL
            LD A, 1
        ENDIF
    )", {});

    // IFNDEF (not defined)
    ASSERT_CODE(R"(
        IFNDEF MY_UNDEFINED_SYMBOL
            LD A, 1
        ENDIF
    )", {0x3E, 0x01});

    // IFNDEF (defined)
    ASSERT_CODE(R"(
        MY_SYMBOL EQU 1
        IFNDEF MY_SYMBOL
            LD A, 1
        ENDIF
    )", {});

    // Nested IF (all true)
    ASSERT_CODE(R"(
        IF 1
            LD A, 1
            IF 1
                LD B, 2
            ENDIF
            LD C, 3
        ENDIF
    )", {0x3E, 0x01, 0x06, 0x02, 0x0E, 0x03});

    // Nested IF (inner false)
    ASSERT_CODE(R"(
        IF 1
            LD A, 1
            IF 0
                LD B, 2
            ENDIF
            LD C, 3
        ENDIF
    )", {0x3E, 0x01, 0x0E, 0x03});

    // Nested IF (outer false)
    ASSERT_CODE(R"(
        IF 0
            LD A, 1
            IF 1
                LD B, 2
            ENDIF
            LD C, 3
        ENDIF
    )", {});

    // Complex nesting with ELSE
    ASSERT_CODE(R"(
        VERSION EQU 2
        IF VERSION == 1
            LD A, 1
        ELSE
            IF VERSION == 2
                LD A, 2
            ELSE
                LD A, 3
            ENDIF
        ENDIF
    )", {0x3E, 0x02});

    // Error cases
    ASSERT_COMPILE_FAILS("IF 1\nLD A, 1"); // Missing ENDIF
    ASSERT_COMPILE_FAILS("ENDIF"); // ENDIF without IF
    ASSERT_COMPILE_FAILS("ELSE"); // ELSE without IF
    ASSERT_COMPILE_FAILS(R"(
        IF 1
        ELSE
        ELSE
        ENDIF
    )"); // Double ELSE
}

TEST_CASE(ConditionalCompilation_ForwardReference) {
    // Forward reference in IF (true)
    ASSERT_CODE(R"(
        IF FORWARD_VAL == 1
            LD A, 1
        ENDIF
        FORWARD_VAL EQU 1
    )", {0x3E, 0x01});

    // Forward reference in IF (false)
    ASSERT_CODE(R"(
        IF FORWARD_VAL == 1
            LD A, 1
        ENDIF
        FORWARD_VAL EQU 0
    )", {});

    // Forward reference in IF with ELSE
    ASSERT_CODE(R"(
        IF FORWARD_VAL > 10
            LD A, 1
        ELSE
            LD A, 2
        ENDIF
        FORWARD_VAL EQU 5
    )", {0x3E, 0x02});
}

TEST_CASE(ComplexForwardReferences) {
    std::string code = R"(
        ORG 0x8000

STACK_SIZE      SET 256
STACK_BASE      SET STACK_TOP - STACK_SIZE

START:
                DI                      ; F3
                LD SP, STACK_TOP        ; 31 00 90
                LD A, 10101010b         ; 3E AA
                LD A, 2*8+1             ; 3E 11
                DS COUNT                ; DS 100 -> 100 bytes of 00

; --- Stack definition ---
                DS 10                   ; 10 bytes of 00
                ORG STACK_BASE
                DS STACK_SIZE, 0xFF     ; DS 256, 0xFF
STACK_TOP:
COUNT           SET 10
                NOP
                DS COUNT, 0xAA
COUNT           SET 100
    )";

    Z80DefaultBus bus;
    MockSourceProvider source_provider;
    source_provider.add_source("main.asm", code);
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    bool success = assembler.compile("main.asm");
    assert(success && "Complex forward reference compilation failed");

    auto symbols = assembler.get_symbols();
    assert(symbols["STACK_TOP"].value == 0x9000);
    assert(symbols["STACK_BASE"].value == 0x8F00);
    assert(symbols["COUNT"].value == 100);

    // Check the compiled code and data
    assert(bus.peek(0x8001) == 0x31 && bus.peek(0x8002) == 0x00 && bus.peek(0x8003) == 0x90); // LD SP, 0x9000
    assert(bus.peek(0x8008) == 0x00 && bus.peek(0x8008 + 99) == 0x00); // DS 100
    assert(bus.peek(0x8F00) == 0xFF && bus.peek(0x8FFF) == 0xFF); // DS 256, 0xFF
    assert(bus.peek(0x9000) == 0x00); // NOP
    assert(bus.peek(0x9001) == 0xAA && bus.peek(0x900A) == 0xAA); // DS 10, 0xAA
    tests_passed++;
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
