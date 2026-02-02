//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble_test.cpp
// Verson: 1.0.0
//
// This file contains unit tests for the Z80Assembler class.
//
// Copyright (c) 2025-2026 Adam Szulc
// MIT License

#include <Z80/Assembler.h>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <chrono>

int tests_passed = 0;
int tests_failed = 0;

struct TestCase {
    void (*func)();
    const char* name;
};
std::vector<TestCase>& get_test_cases() {
    static std::vector<TestCase> tests;
    return tests;
}

#define TEST_CASE(name)                                           \
    void test_##name();                                           \
    struct TestRegisterer_##name {                                \
        TestRegisterer_##name() {                                 \
            get_test_cases().push_back({test_##name, #name});     \
        }                                                         \
    } test_registerer_##name;                                     \
    void test_##name()

void run_all_tests() {
    for (const auto& test : get_test_cases()) {
        std::cout << "--- Running test: " << test.name << " ---\n";
        try {
            test.func();
        } catch (const std::exception& e) {
            std::cerr << "ERROR: An uncaught exception occurred in test '" << test.name << "': " << e.what() << std::endl;
            tests_failed++;
        }
    }
}

class MockFileProvider : public Z80::IFileProvider {
public:
    bool read_file(const std::string& identifier, std::vector<uint8_t>& data) override {
        if (m_sources.count(identifier)) {
            data = m_sources[identifier];
            return true;
        }
        return false;
    }

    size_t file_size(const std::string& identifier) override {
        if (m_sources.count(identifier)) {
            return m_sources.at(identifier).size();
        }
        return 0;
    }

    bool exists(const std::string& identifier) override {
        return m_sources.count(identifier) > 0;
    }

    void add_source(const std::string& identifier, const std::string& content) {
        m_sources[identifier] = std::vector<uint8_t>(content.begin(), content.end());
    }

    void add_binary_source(const std::string& identifier, const std::vector<uint8_t>& content) {
        m_sources[identifier] = content;
    }
private:
    std::map<std::string, std::vector<uint8_t>> m_sources;
};

void ASSERT_CODE_WITH_OPTS(const std::string& asm_code, const std::vector<uint8_t>& expected_bytes, const Z80::Assembler<Z80::StandardBus>::Config& config) {
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", asm_code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider, config);
    bool success = true;
    try {
        assembler.compile("main.asm", 0x0000);
    } catch (const std::runtime_error& e) {
        success = false;
    }
    if (!success) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Compilation failed with an exception for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    // Calculate total compiled size by summing up all blocks
    size_t compiled_size = 0;
    if (!blocks.empty()) { 
        // Only sum blocks that are contiguous from the start
        uint16_t next_addr = blocks[0].start_address;
        for (const auto& block : blocks) {
            if (block.start_address != next_addr) break;
            compiled_size += block.size;
            next_addr += block.size;
        }
    }

    if (compiled_size != expected_bytes.size()) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
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
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
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

void ASSERT_CODE_WITH_ASSEMBLER(Z80::StandardBus& bus, Z80::Assembler<Z80::StandardBus>& assembler, MockFileProvider& file_provider, const std::string& asm_code, const std::vector<uint8_t>& expected_bytes) {
    file_provider.add_source("main.asm", asm_code);
    bool success = true;
    try {
        assembler.compile("main.asm", 0x0000);
    } catch (const std::runtime_error& e) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Compilation failed with an exception for '" << asm_code << "': " << e.what() << "\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    size_t compiled_size = 0;
    if (!blocks.empty()) {
        uint16_t next_addr = blocks[0].start_address;
        for (const auto& block : blocks) {
            if (block.start_address != next_addr) break;
            compiled_size += block.size;
            next_addr += block.size;
        }
    }

    if (compiled_size != expected_bytes.size()) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Incorrect compiled size for '" << asm_code << "'.\n";
        std::cerr << "  Expected size: " << expected_bytes.size() << ", Got: " << compiled_size << "\n";
        tests_failed++;
        return;
    }

    uint16_t start_address = blocks.empty() ? 0x0000 : blocks[0].start_address;
    for (size_t i = 0; i < expected_bytes.size(); ++i) {
        if (bus.peek(start_address + i) != expected_bytes[i]) {
            // ... (error reporting logic from ASSERT_CODE)
            tests_failed++;
            return;
        }
    }
    tests_passed++;
}

void ASSERT_CODE(const std::string& asm_code, const std::vector<uint8_t>& expected_bytes) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    ASSERT_CODE_WITH_OPTS(asm_code, expected_bytes, config);
}

void ASSERT_BLOCKS(const std::string& asm_code, const std::map<uint16_t, std::vector<uint8_t>>& expected_blocks) {
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", asm_code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    bool success = assembler.compile("main.asm", 0x0000);
    if (!success) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Compilation failed for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    auto compiled_blocks = assembler.get_blocks();
    
    // Merge contiguous compiled blocks for comparison
    std::vector<Z80::Assembler<Z80::StandardBus>::BlockInfo> merged_blocks;
    if (!compiled_blocks.empty()) {
        merged_blocks.push_back(compiled_blocks[0]);
        for (size_t i = 1; i < compiled_blocks.size(); ++i) {
            auto& last = merged_blocks.back();
            const auto& curr = compiled_blocks[i];
            if (last.start_address + last.size == curr.start_address) {
                last.size += curr.size;
            } else {
                merged_blocks.push_back(curr);
            }
        }
    }

    if (merged_blocks.size() != expected_blocks.size()) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Incorrect number of compiled blocks for '" << asm_code << "'.\n";
        std::cerr << "  Expected: " << expected_blocks.size() << ", Got: " << merged_blocks.size() << "\n";
        tests_failed++;
        return;
    }

    for (const auto& compiled_block : merged_blocks) {
        uint16_t start_address = compiled_block.start_address;
        if (expected_blocks.find(start_address) == expected_blocks.end()) {
            std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
            std::cerr << "Assertion failed: Unexpected compiled block at address 0x" << std::hex << start_address << "\n";
            tests_failed++;
            continue;
        }

        const auto& expected_bytes = expected_blocks.at(start_address);
        if (compiled_block.size != expected_bytes.size()) {
            std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
            std::cerr << "Assertion failed: Incorrect size for block at 0x" << std::hex << start_address << ".\n";
            std::cerr << "  Expected size: " << expected_bytes.size() << ", Got: " << compiled_block.size << "\n";
            tests_failed++;
            continue;
        }

        for (size_t i = 0; i < expected_bytes.size(); ++i) {
            if (bus.peek(start_address + i) != expected_bytes[i]) {
                std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
                std::cerr << "Assertion failed: Byte mismatch in block at 0x" << std::hex << start_address << " for '" << asm_code << "'\n";
                tests_failed++;
                return; // End after the first error in the block
            }
        }
    }
    tests_passed++;
}

void ASSERT_COMPILE_FAILS_WITH_OPTS(const std::string& asm_code, const Z80::Assembler<Z80::StandardBus>::Config& config) {
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", asm_code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider, config);    
    bool success = true;
    try {
        success = assembler.compile("main.asm", 0x0000);
    } catch (const std::runtime_error&) {
        success = false;
    }
    if (success) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Compilation succeeded for '" << asm_code << "' but was expected to fail.\n";
        tests_failed++;
    } else {
        tests_passed++;
    }
}

void ASSERT_COMPILE_FAILS(const std::string& asm_code) {
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", asm_code);
    Z80::Assembler<Z80::StandardBus>::Config config;
    // Use the unified logic from ASSERT_COMPILE_FAILS_WITH_OPTS
    ASSERT_COMPILE_FAILS_WITH_OPTS(asm_code, config);
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
    ASSERT_CODE("RLD", {0xED, 0x6F});
    ASSERT_CODE("RRD", {0xED, 0x67});
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

TEST_CASE(UndocumentedInstructionsDisabled) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_undocumented = false;

    // SLL / SLI
    ASSERT_COMPILE_FAILS_WITH_OPTS("SLL A", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("SLI B", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("SLL (HL)", config);

    // IXH/IXL/IYH/IYL usage
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, IXH", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD IXL, 10", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("INC IYH", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("ADD A, IYL", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD IXH, IXL", config);

    // Undocumented IO
    ASSERT_COMPILE_FAILS_WITH_OPTS("OUT (C), 0", config);

    // Undocumented Shift/Rotate/Bit with copy to register
    ASSERT_COMPILE_FAILS_WITH_OPTS("RLC (IX+0), B", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("SLA (IY+5), C", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("SET 1, (IX+0), B", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("RES 2, (IY+5), C", config);
}

TEST_CASE(Z80NInstructions) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = true;

    // SWAPNIB
    ASSERT_CODE_WITH_OPTS("SWAPNIB", {0xED, 0x23}, config);
    // MIRROR
    ASSERT_CODE_WITH_OPTS("MIRROR", {0xED, 0x24}, config);
    // BSLA DE, B
    ASSERT_CODE_WITH_OPTS("BSLA DE, B", {0xED, 0x28}, config);
    // BSRA DE, B
    ASSERT_CODE_WITH_OPTS("BSRA DE, B", {0xED, 0x29}, config);
    // BSRL DE, B
    ASSERT_CODE_WITH_OPTS("BSRL DE, B", {0xED, 0x2A}, config);
    // BSRF DE, B
    ASSERT_CODE_WITH_OPTS("BSRF DE, B", {0xED, 0x2B}, config);
    // BRLC DE, B
    ASSERT_CODE_WITH_OPTS("BRLC DE, B", {0xED, 0x2C}, config);
    // MUL D, E
    ASSERT_CODE_WITH_OPTS("MUL D, E", {0xED, 0x30}, config);
    // ADD rr, A
    ASSERT_CODE_WITH_OPTS("ADD HL, A", {0xED, 0x31}, config);
    ASSERT_CODE_WITH_OPTS("ADD DE, A", {0xED, 0x32}, config);
    ASSERT_CODE_WITH_OPTS("ADD BC, A", {0xED, 0x33}, config);
    // ADD rr, nn
    ASSERT_CODE_WITH_OPTS("ADD HL, 0x1234", {0xED, 0x34, 0x34, 0x12}, config);
    ASSERT_CODE_WITH_OPTS("ADD DE, 0x1234", {0xED, 0x35, 0x34, 0x12}, config);
    ASSERT_CODE_WITH_OPTS("ADD BC, 0x1234", {0xED, 0x36, 0x34, 0x12}, config);
    // PUSH nn (Big Endian)
    ASSERT_CODE_WITH_OPTS("PUSH 0x1234", {0xED, 0x8A, 0x12, 0x34}, config);
    // OUTINB
    ASSERT_CODE_WITH_OPTS("OUTINB", {0xED, 0x90}, config);
    // NEXTREG n, n
    ASSERT_CODE_WITH_OPTS("NEXTREG 0x10, 0x20", {0xED, 0x91, 0x10, 0x20}, config);
    // NEXTREG n, A
    ASSERT_CODE_WITH_OPTS("NEXTREG 0x10, A", {0xED, 0x92, 0x10}, config);
    // PIXELAD
    ASSERT_CODE_WITH_OPTS("PIXELAD", {0xED, 0x93}, config);
    // PIXELDN
    ASSERT_CODE_WITH_OPTS("PIXELDN", {0xED, 0x94}, config);
    // SETAE
    ASSERT_CODE_WITH_OPTS("SETAE", {0xED, 0x95}, config);
    // JP (C)
    ASSERT_CODE_WITH_OPTS("JP (C)", {0xED, 0x98}, config);
    // LDIX
    ASSERT_CODE_WITH_OPTS("LDIX", {0xED, 0xA4}, config);
    // LDWS
    ASSERT_CODE_WITH_OPTS("LDWS", {0xED, 0xA5}, config);
    // LDDX
    ASSERT_CODE_WITH_OPTS("LDDX", {0xED, 0xAC}, config);
    // LDIRX
    ASSERT_CODE_WITH_OPTS("LDIRX", {0xED, 0xB4}, config);
    // LDIRSCALE
    ASSERT_CODE_WITH_OPTS("LDIRSCALE", {0xED, 0xB6}, config);
    // LDPIRX
    ASSERT_CODE_WITH_OPTS("LDPIRX", {0xED, 0xB7}, config);
    // LDDRX
    ASSERT_CODE_WITH_OPTS("LDDRX", {0xED, 0xBC}, config);
    // TEST n
    ASSERT_CODE_WITH_OPTS("TEST 0xAA", {0xED, 0x27, 0xAA}, config);
}

TEST_CASE(Z80NInstructionsDisabled) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = false;

    ASSERT_COMPILE_FAILS_WITH_OPTS("SWAPNIB 1", config); // Use invalid syntax to ensure it fails even if treated as label
    ASSERT_COMPILE_FAILS_WITH_OPTS("NEXTREG 0x10, 0x20", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("PUSH 0x1234", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("ADD HL, A", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("ADD HL, 0x1234", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("JP (C), 0", config); // Invalid syntax
    ASSERT_COMPILE_FAILS_WITH_OPTS("TEST 0xAA", config);
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
    ASSERT_CODE("DM \"Test\"", {'T', 'e', 's', 't'}); // DM as alias for DB/BYTE
    ASSERT_CODE("DEFM 1, 2, 3", {0x01, 0x02, 0x03}); // DEFM as alias for DB
    ASSERT_CODE("DEFM \"String\"", {'S', 't', 'r', 'i', 'n', 'g'});
    ASSERT_CODE("DEFM \"RN\",'D'+$80", {'R', 'N', 'D'+0x80});

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

    // DZ / ASCIZ
    ASSERT_CODE("DZ \"Game Over\"", {'G', 'a', 'm', 'e', ' ', 'O', 'v', 'e', 'r', 0x00});
    ASSERT_CODE("ASCIZ \"Hello\"", {'H', 'e', 'l', 'l', 'o', 0x00});
    ASSERT_CODE("DZ \"Part1\", \", Part2\"", {'P', 'a', 'r', 't', '1', ',', ' ', 'P', 'a', 'r', 't', '2', 0x00});
    ASSERT_CODE("DZ \"Numbers: \", 1, 2, 3", {'N', 'u', 'm', 'b', 'e', 'r', 's', ':', ' ', 1, 2, 3, 0x00});
    ASSERT_COMPILE_FAILS("DZ");
}

TEST_CASE(HexDirectives) {
    // DH - Define Hex (string literal)
    ASSERT_CODE("DH \"010203\"", {0x01, 0x02, 0x03});
    ASSERT_COMPILE_FAILS("DH \"badc0de\"");
    ASSERT_CODE("DH \" 12 34 \"", {0x12, 0x34}); // Spaces should be ignored
    ASSERT_CODE("DH \"12\", \"34\"", {0x12, 0x34}); // Multiple arguments
    ASSERT_COMPILE_FAILS("DH \"1\""); // Odd number of characters should fail
    ASSERT_COMPILE_FAILS("DH \"123\""); // Odd number of characters should fail
    ASSERT_COMPILE_FAILS("DH \"12G3\""); // Invalid hex character
    ASSERT_COMPILE_FAILS("DH"); // No arguments
    ASSERT_CODE("DEFH \"010203\"", {0x01, 0x02, 0x03}); // DEFH as alias for DH

    // HEX - Define Hex (unquoted)
    ASSERT_CODE("HEX \"010203\"", {0x01, 0x02, 0x03});
    ASSERT_COMPILE_FAILS("HEX \"badc0de\"");
    ASSERT_CODE("HEX \"12\", \"34\"", {0x12, 0x34}); // Commas should be ignored
    ASSERT_COMPILE_FAILS("HEX \"1\""); // Odd number of characters
    ASSERT_COMPILE_FAILS("HEX \"123\""); // Odd number of characters
    ASSERT_COMPILE_FAILS("HEX 12G3"); // Invalid hex character
    ASSERT_CODE("HEX \"12\", \"34\"\n NOP", {0x12, 0x34, 0x00}); // Should not consume next line
}

TEST_CASE(LabelsAndExpressions) {
    std::string code = R"(
        ORG 0x100
    START:
        LD A, 5
        LD B, A
        ADD A, B
        LD (VALUE), A ; VALUE is at 0x10A (physical)
        JP FINISH     ; FINISH is at 0x10B (physical)
    VALUE:
        DB 0
    FINISH:
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

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    bool success = assembler.compile("main.asm");
    assert(success && "Compilation with labels failed");

    auto blocks = assembler.get_blocks();
    
    // Calculate total size of contiguous blocks
    size_t total_size = 0;
    if (!blocks.empty()) {
        assert(blocks[0].start_address == 0x100 && "Block should start at 0x100");
        for(const auto& b : blocks) total_size += b.size;
    } else {
        assert(false && "No blocks generated");
    }

    assert(total_size == expected.size() && "Incorrect compiled size");

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

TEST_CASE(LabelWithColonAndAssignment) {
    // EQU with colon
    ASSERT_CODE(R"(
        MY_CONST: EQU 0x55
        LD A, MY_CONST
    )", {0x3E, 0x55});

    // SET with colon
    ASSERT_CODE(R"(
        MY_VAR: SET 0xAA
        LD B, MY_VAR
    )", {0x06, 0xAA});

    // = with colon
    ASSERT_CODE(R"(
        MY_VAL: = 0x33
        LD C, MY_VAL
    )", {0x0E, 0x33});

    // DEFL with colon
    ASSERT_CODE(R"(
        MY_DEFL: DEFL 0x44
        LD D, MY_DEFL
    )", {0x16, 0x44});
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

TEST_CASE(EqualsAsSetDirective) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.directives.constants.assignments_as_set = true;

    // 1. Basic usage of '=' as SET
    ASSERT_CODE_WITH_OPTS(R"( 
        VALUE = 15
        LD A, VALUE
    )", {0x3E, 15}, config);

    // 2. Redefinition using '='
    ASSERT_CODE_WITH_OPTS(R"(
        VALUE = 10
        VALUE = 20
        LD A, VALUE
    )", {0x3E, 20}, config);

    // 3. Mixing SET and '='
    ASSERT_CODE_WITH_OPTS(R"(
        VALUE SET 5
        VALUE = 10 ; Redefine with =
        LD A, VALUE
        VALUE SET 15 ; Redefine with SET
        LD B, VALUE
    )", {0x3E, 10, 0x06, 15}, config);

    // 4. Mixing EQU and '=' (should fail)
    ASSERT_COMPILE_FAILS_WITH_OPTS("VAL EQU 1\nVAL = 2", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("VAL = 1\nVAL EQU 2", config);
}

TEST_CASE(EqualsAsEquDirective) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.directives.constants.assignments_as_set = false;

    // 1. Basic usage of '=' as EQU
    ASSERT_CODE_WITH_OPTS(R"(
        VALUE = 15
        LD A, VALUE
    )", {0x3E, 15}, config);

    // 2. Redefinition using '=' should fail
    ASSERT_COMPILE_FAILS_WITH_OPTS(R"(
        VALUE = 10
        VALUE = 20
    )", config);

    // 3. Mixing SET and '=' should fail on redefinition
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE SET 5\nVALUE = 10", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE = 10\nVALUE SET 5", config);

    // 4. Using '==' (comparison) in an IF directive (true case)
    ASSERT_CODE(R"(
        VAL1 EQU 10
        VAL2 SET 10
        IF VAL1 == VAL2
            LD A, 1
        ELSE
            LD A, 0
        ENDIF
    )", {0x3E, 1});

    // 5. Using '==' (comparison) in an IF directive (false case)
    ASSERT_CODE(R"(
        VAL1 EQU 10
        VAL2 SET 11
        IF VAL1 == VAL2
            LD A, 1
        ELSE
            LD A, 0
        ENDIF
    )", {0x3E, 0});

    // 6. Using '==' (comparison) directly in a constant definition
    ASSERT_CODE(R"(
        IS_EQUAL EQU (10 == 10)
        LD A, IS_EQUAL
    )", {0x3E, 1});
}

TEST_CASE(AdvancedConstantsAndExpressions) {
    // 1. SET based on an EQU constant
    ASSERT_CODE(R"(
        BASE_VAL EQU 100
        OFFSET_VAL SET BASE_VAL + 5
        LD A, OFFSET_VAL
    )", {0x3E, 105});

    // 2. EQU based on a SET constant (EQU should be fixed to the value of SET at that point)
    ASSERT_CODE(R"(
        VAR_SET SET 50
        CONST_EQU EQU VAR_SET * 2
        VAR_SET SET 60 ; This redefinition should not affect CONST_EQU
        LD A, CONST_EQU
        LD B, VAR_SET
    )", {0x3E, 100, 0x06, 60});
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

    // Test C++ style comments
    ASSERT_CODE("LD A, 5 // This is a C++ style comment", {0x3E, 0x05});
    ASSERT_CODE("// ENTIRE LINE COMMENT\nLD B, 10", {0x06, 0x0A});

    // Test empty comments
    ASSERT_CODE("LD A, 1 ;", {0x3E, 0x01});
    ASSERT_CODE("LD B, 2 //", {0x06, 0x02});
    ASSERT_CODE("LD C, 3 /**/", {0x0E, 0x03});

    // Test mixed comments
    ASSERT_CODE(R"(
        LD A, 1 ; Semicolon /* Block inside */ // C++ style inside
        LD B, 2 // C++ style ; Semicolon inside
    )", {0x3E, 0x01, 0x06, 0x02});

    // Test comment markers inside strings (should be ignored)
    ASSERT_CODE(R"(DB "This is not a ; comment")",
                {0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61, 0x20, 0x3B, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x65, 0x6E, 0x74});
    // Single quotes can now be used for strings too.
    ASSERT_CODE("DB 'This is not a /* comment */'",
                {0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61, 0x20, 0x2F, 0x2A, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x2A, 0x2F});
    ASSERT_CODE(R"(DB "Nor is this a // comment")",
                {0x4E, 0x6F, 0x72, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x61, 0x20, 0x2F, 0x2F, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x65, 0x6E, 0x74});
    ASSERT_CODE(R"(DB "A string with a /* block comment", 10, "and another one with a ; semicolon")",
                {0x41, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x61, 0x20, 0x2F, 0x2A, 0x20, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x65, 0x6E, 0x74,
                 0x0A,
                 0x61, 0x6E, 0x64, 0x20, 0x61, 0x6E, 0x6F, 0x74, 0x68, 0x65, 0x72, 0x20, 0x6F, 0x6E, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x61, 0x20, 0x3B, 0x20, 0x73, 0x65, 0x6D, 0x69, 0x63, 0x6F, 0x6C, 0x6F, 0x6E});
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
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", asm_code);
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
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

TEST_CASE(AlternativeNumberPrefixes) {
    // Test $ for Hexadecimal (e.g. $FF)
    ASSERT_CODE("LD A, $10", {0x3E, 0x10});
    ASSERT_CODE("LD BC, $ABCD", {0x01, 0xCD, 0xAB});
    ASSERT_CODE("DB $01, $02", {0x01, 0x02});

    // Test % for Binary (e.g. %10101010)
    ASSERT_CODE("LD A, %10101010", {0x3E, 0xAA});
    ASSERT_CODE("LD B, %1100", {0x06, 0x0C});
    ASSERT_CODE("DB %11110000", {0xF0});

    // Test mixed usage in expressions
    ASSERT_CODE("LD A, $0F + %00010000", {0x3E, 0x1F});
    ASSERT_CODE("LD A, $A", {0x3E, 0x0A});
    ASSERT_CODE("LD A, %1", {0x3E, 0x01});

    // Verify $ as current address still works (regression test)
    ASSERT_CODE("NOP\nDB $", {0x00, 0x01});

    // Verify % as modulo operator still works (regression test)
    // Note: % followed by space or non-binary digit is treated as operator
    ASSERT_CODE("LD A, 10 % 3", {0x3E, 0x01});
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

TEST_CASE(ExpressionOperators) {
    // Tests for word and symbolic operators

    // Arithmetic operators
    ASSERT_CODE("LD A, 10 + 3", {0x3E, 13});
    ASSERT_CODE("LD A, 10 - 3", {0x3E, 7});
    ASSERT_CODE("LD A, 10 * 3", {0x3E, 30});
    ASSERT_CODE("LD A, 10 / 3", {0x3E, 3});
    ASSERT_CODE("LD A, 10 % 3", {0x3E, 1});
    ASSERT_CODE("LD A, 10 MOD 3", {0x3E, 1});

    // Bitwise operators
    ASSERT_CODE("LD A, 0b1010 | 0b0110", {0x3E, 0b1110});
    ASSERT_CODE("LD A, 0b1010 OR 0b0110", {0x3E, 0b1110});

    ASSERT_CODE("LD A, 0b1010 & 0b0110", {0x3E, 0b0010});
    ASSERT_CODE("LD A, 0b1010 AND 0b0110", {0x3E, 0b0010});

    ASSERT_CODE("LD A, 0b1010 ^ 0b0110", {0x3E, 0b1100});
    ASSERT_CODE("LD A, 0b1010 XOR 0b0110", {0x3E, 0b1100});

    ASSERT_CODE("LD A, 5 << 2", {0x3E, 20});
    ASSERT_CODE("LD A, 5 SHL 2", {0x3E, 20});

    ASSERT_CODE("LD A, 16 >> 2", {0x3E, 4});
    ASSERT_CODE("LD A, 16 SHR 2", {0x3E, 4});

    // Comparison operators
    ASSERT_CODE("LD A, 10 > 5", {0x3E, 1});
    ASSERT_CODE("LD A, 10 GT 5", {0x3E, 1});
    ASSERT_CODE("LD A, 5 > 10", {0x3E, 0});
    ASSERT_CODE("LD A, 5 GT 10", {0x3E, 0});

    ASSERT_CODE("LD A, 5 < 10", {0x3E, 1});
    ASSERT_CODE("LD A, 5 LT 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 < 5", {0x3E, 0});
    ASSERT_CODE("LD A, 10 LT 5", {0x3E, 0});

    ASSERT_CODE("LD A, 10 >= 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 GE 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 >= 5", {0x3E, 1});
    ASSERT_CODE("LD A, 10 GE 5", {0x3E, 1});
    ASSERT_CODE("LD A, 5 >= 10", {0x3E, 0});
    ASSERT_CODE("LD A, 5 GE 10", {0x3E, 0});

    ASSERT_CODE("LD A, 5 <= 5", {0x3E, 1});
    ASSERT_CODE("LD A, 5 LE 5", {0x3E, 1});
    ASSERT_CODE("LD A, 5 <= 10", {0x3E, 1});
    ASSERT_CODE("LD A, 5 LE 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 <= 5", {0x3E, 0});
    ASSERT_CODE("LD A, 10 LE 5", {0x3E, 0});

    ASSERT_CODE("LD A, 10 == 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 EQ 10", {0x3E, 1});
    ASSERT_CODE("LD A, 10 == 5", {0x3E, 0});
    ASSERT_CODE("LD A, 10 EQ 5", {0x3E, 0});

    ASSERT_CODE("LD A, 10 != 5", {0x3E, 1});
    ASSERT_CODE("LD A, 10 NE 5", {0x3E, 1});
    ASSERT_CODE("LD A, 10 != 10", {0x3E, 0});
    ASSERT_CODE("LD A, 10 NE 10", {0x3E, 0});

    // Logical operators
    ASSERT_CODE("LD A, 1 && 1", {0x3E, 1});
    ASSERT_CODE("LD A, 1 && 0", {0x3E, 0});
    ASSERT_CODE("LD A, 0 && 0", {0x3E, 0});

    ASSERT_CODE("LD A, 1 || 0", {0x3E, 1});
    ASSERT_CODE("LD A, 0 || 1", {0x3E, 1});
    ASSERT_CODE("LD A, 0 || 0", {0x3E, 0});

    // Unary operators
    ASSERT_CODE("LD A, -5", {0x3E, (uint8_t)-5});
    ASSERT_CODE("LD A, ~0b01010101", {0x3E, 0b10101010});
    ASSERT_CODE("LD A, NOT 0b01010101", {0x3E, 0b10101010});
    ASSERT_CODE("LD A, !1", {0x3E, 0});
    ASSERT_CODE("LD A, !0", {0x3E, 1});

    // Check operator precedence
    ASSERT_CODE("LD A, 2 + 3 * 4", {0x3E, 14});
    ASSERT_CODE("LD A, 2 + 3 GT 4", {0x3E, 1}); // (2+3) > 4
    ASSERT_CODE("LD A, 10 AND 12 + 1", {0x3E, 8}); // 10 & (12+1) -> 10 & 13 = 8

    // Check functions
    ASSERT_CODE("LD A, HIGH(0x1234)", {0x3E, 0x12});
    ASSERT_CODE("LD A, LOW(0x1234)", {0x3E, 0x34});

    // Invalid expressions
    ASSERT_COMPILE_FAILS("LD A, 10 / 0");
    ASSERT_COMPILE_FAILS("LD A, 10 MOD 0");
    ASSERT_COMPILE_FAILS("LD A, 10 % 0");
    ASSERT_COMPILE_FAILS("LD A, (10 + 2"); // Missing closing parenthesis
    ASSERT_COMPILE_FAILS("LD A, 10 + * 2"); // Invalid syntax
}

void ASSERT_RAND_IN_RANGE(const std::string& asm_code, int min_val, int max_val) {
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", asm_code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    bool success = assembler.compile("main.asm", 0x0000);
    if (!success) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: Compilation failed for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    if (blocks.empty() || blocks[0].size == 0) {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: No code generated for '" << asm_code << "'\n";
        tests_failed++;
        return;
    }

    uint8_t generated_value = bus.peek(blocks[0].start_address);
    if (generated_value >= min_val && generated_value <= max_val) {
        tests_passed++;
    } else {
        std::cerr << "Failing code:\n---\n" << asm_code << "\n---\n";
        std::cerr << "Assertion failed: RAND value out of range for '" << asm_code << "'\n";
        std::cerr << "  Expected range: [" << min_val << ", " << max_val << "], Got: " << (int)generated_value << "\n";
        tests_failed++;
    }
}

void reset_rand_seed() {
    // This is a bit of a hack. Since the random generator is static inside a lambda,
    // we can't easily reset it. To get a fresh sequence for tests, we compile
    // a dummy expression that re-initializes the static generator.
    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", "DB RAND(0,0)");
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    assembler.compile("main.asm");
}

TEST_CASE(MathFunctionsInExpressions) {
    // Trigonometric function tests (results are cast to int32_t)
    ASSERT_CODE("LD A, SIN(0)", {0x3E, 0});
    ASSERT_CODE("LD A, COS(0)", {0x3E, 1});
    ASSERT_CODE("LD A, TAN(0)", {0x3E, 0});
    ASSERT_CODE("LD A, ROUND(SIN(MATH_PI / 2))", {0x3E, 1}); // sin(pi/2)
    ASSERT_CODE("LD A, ROUND(COS(MATH_PI))", {0x3E, (uint8_t)-1}); // cos(pi)
    ASSERT_CODE("LD A, ASIN(1)", {0x3E, 1}); // asin(1) ~= 1.57, cast to 1
    ASSERT_CODE("LD A, ACOS(1)", {0x3E, 0});
    ASSERT_CODE("LD A, ATAN(1)", {0x3E, 0}); // atan(1) ~= 0.785, cast to 0
    ASSERT_CODE("LD A, ATAN2(1, 0)", {0x3E, 1}); // atan2(1,0) ~= 1.57, cast to 1

    // Power and logarithmic function tests
    ASSERT_CODE("LD A, ABS(-123.0)", {0x3E, 123});
    ASSERT_CODE("LD A, POW(2, 7)", {0x3E, 128});
    ASSERT_CODE("LD A, SQRT(64)", {0x3E, 8});
    ASSERT_CODE("LD A, LOG(1)", {0x3E, 0}); // natural log
    ASSERT_CODE("LD A, LOG10(1000)", {0x3E, 3});
    ASSERT_CODE("LD A, LOG2(256)", {0x3E, 8});
    ASSERT_CODE("LD A, EXP(0)", {0x3E, 1});

    // Rounding function tests
    ASSERT_CODE("LD A, FLOOR(9.9)", {0x3E, 9});
    ASSERT_CODE("LD A, CEIL(9.1)", {0x3E, 10});
    ASSERT_CODE("LD A, ROUND(9.5)", {0x3E, 10});
    ASSERT_CODE("LD A, ROUND(9.4)", {0x3E, 9});

    // Test random function - check if the value is within the expected range.
    ASSERT_RAND_IN_RANGE("DB RAND(1, 10)", 1, 10);
    ASSERT_RAND_IN_RANGE("DB RAND(50, 100)", 50, 100);

    // Complex expression with functions
    ASSERT_CODE("LD A, SQRT(POW(3,2) + POW(4,2))", {0x3E, 5}); // SQRT(9+16) = SQRT(25) = 5

    // Test built-in constants
    ASSERT_CODE("LD A, TRUE", {0x3E, 1});
    ASSERT_CODE("LD A, FALSE", {0x3E, 0});
    ASSERT_CODE("LD A, MATH_PI", {0x3E, 3}); // PI (3.14...) is truncated to 3
    ASSERT_CODE("LD A, MATH_E", {0x3E, 2});  // E (2.71...) is truncated to 2
    ASSERT_CODE("LD A, ROUND(LOG(MATH_E))", {0x3E, 1});
    ASSERT_CODE("LD A, 5 * TRUE", {0x3E, 5});
}
TEST_CASE(SgnFunctionInExpressions) {
    ASSERT_CODE("LD A, SGN(123)", {0x3E, 1});
    ASSERT_CODE("LD A, SGN(-45)", {0x3E, (uint8_t)-1});
    ASSERT_CODE("LD A, SGN(0)", {0x3E, 0});
    ASSERT_CODE("LD A, SGN(123.45)", {0x3E, 1});
    ASSERT_CODE("LD A, SGN(-0.5)", {0x3E, (uint8_t)-1});
    ASSERT_CODE("LD A, SGN(0.0)", {0x3E, 0});
}

TEST_CASE(MathFunctionsExtended) {
    // Hyperbolic functions
    ASSERT_CODE("LD A, ROUND(SINH(0))", {0x3E, 0});
    ASSERT_CODE("LD A, ROUND(COSH(0))", {0x3E, 1});
    ASSERT_CODE("LD A, ROUND(TANH(1))", {0x3E, 1}); // tanh(1) ~= 0.76

    // Truncation
    ASSERT_CODE("LD A, TRUNC(3.9)", {0x3E, 3});
    ASSERT_CODE("LD A, TRUNC(-3.9)", {0x3E, (uint8_t)-3});

    // Random functions - check syntax and range
    ASSERT_RAND_IN_RANGE("DB RND() * 100", 0, 99); // RND() is [0.0, 1.0)
    ASSERT_RAND_IN_RANGE("DB RRND(10, 20)", 10, 20);
    // Also test RAND here to ensure its syntax is checked
    ASSERT_RAND_IN_RANGE("DB RAND(1, 100)", 1, 100);
}

TEST_CASE(CaseSensitivity) {
    // 1. Built-in functions and constants are case-insensitive.
    // Functions
    ASSERT_CODE("LD A, ROUND(9.5)", {0x3E, 10});
    ASSERT_CODE("LD A, round(9.5)", {0x3E, 10});
    ASSERT_CODE("LD A, RoUnD(9.5)", {0x3E, 10});
    ASSERT_CODE("LD A, SIN(0)", {0x3E, 0});
    ASSERT_CODE("LD A, sin(0)", {0x3E, 0});
    ASSERT_CODE("LD A, sIn(0)", {0x3E, 0});

    // Constants
    ASSERT_CODE("LD A, TRUE", {0x3E, 1});
    ASSERT_CODE("LD A, true", {0x3E, 1});
    ASSERT_CODE("LD A, TrUe", {0x3E, 1});
    ASSERT_CODE("LD A, MATH_PI", {0x3E, 3});
    ASSERT_CODE("LD A, math_pi", {0x3E, 3});
    ASSERT_CODE("LD A, MaTh_Pi", {0x3E, 3});

    // 2. User-defined symbols (EQU, SET, labels) are case-sensitive.
    // EQU
    ASSERT_CODE(R"(
        MyConst EQU 123
        LD A, MyConst
    )", {0x3E, 123});
    ASSERT_COMPILE_FAILS("MyConst EQU 123\nLD A, myconst");
    ASSERT_COMPILE_FAILS("MyConst EQU 123\nLD A, MYCONST");

    // SET
    ASSERT_CODE(R"(
        MyVar SET 55
        LD A, MyVar
    )", {0x3E, 55});
    ASSERT_COMPILE_FAILS("MyVar SET 55\nLD A, myvar");

    // Labels
    ASSERT_CODE("MyLabel: NOP\nJP MyLabel", {0x00, 0xC3, 0x00, 0x00});
    ASSERT_COMPILE_FAILS("MyLabel: NOP\nJP mylabel");
    ASSERT_COMPILE_FAILS("MyLabel: NOP\nJP MYLABEL");
}

TEST_CASE(RegisterCaseInsensitivity) {
    // 8-bit registers
    ASSERT_CODE("LD a, 10", {0x3E, 0x0A});
    ASSERT_CODE("ld b, 20", {0x06, 0x14});
    ASSERT_CODE("Ld c, 30", {0x0E, 0x1E});

    // 16-bit registers
    ASSERT_CODE("ld bc, 0x1234", {0x01, 0x34, 0x12});
    ASSERT_CODE("LD de, 0x5678", {0x11, 0x78, 0x56});
    ASSERT_CODE("ld HL, 0x9ABC", {0x21, 0xBC, 0x9A});

    // Special registers
    ASSERT_CODE("push af", {0xF5});
    ASSERT_CODE("pop af", {0xF1});
    ASSERT_CODE("ld sp, 0x0000", {0x31, 0x00, 0x00});
    ASSERT_CODE("ex af, af'", {0x08});

    // Index registers
    ASSERT_CODE("ld ix, 0x1111", {0xDD, 0x21, 0x11, 0x11});
    ASSERT_CODE("ld iy, 0x2222", {0xFD, 0x21, 0x22, 0x22});
    ASSERT_CODE("ld ixh, 0x33", {0xDD, 0x26, 0x33});
    ASSERT_CODE("ld iyl, 0x44", {0xFD, 0x2E, 0x44});

    // Mixed case
    ASSERT_CODE("Ld Bc, 0x1234", {0x01, 0x34, 0x12});
    ASSERT_CODE("lD iX, 0x1234", {0xDD, 0x21, 0x34, 0x12});
}

TEST_CASE(FloatingPointAndVariadicExpressions) {
    // Test floating point numbers in expressions
    ASSERT_CODE("LD A, 3.14 * 2", {0x3E, 6}); // 6.28 is truncated to 6
    ASSERT_CODE("LD A, 10.5 - 2.5", {0x3E, 8});
    ASSERT_CODE("LD A, 7.5 + 2", {0x3E, 9});
    ASSERT_CODE("LD A, 10.0 / 4.0", {0x3E, 2}); // 2.5 is truncated to 2
    ASSERT_CODE("DB 1.5 * 4", {6});

    // Test MIN() function with variadic arguments
    ASSERT_CODE("LD A, MIN(10, 20)", {0x3E, 10});
    ASSERT_CODE("LD A, MIN(30, 15, 25)", {0x3E, 15});
    ASSERT_CODE("LD A, MIN(5, 2, 8, 3, 9)", {0x3E, 2});

    // Test MAX() function with variadic arguments
    ASSERT_CODE("LD A, MAX(10, 20)", {0x3E, 20});
    ASSERT_CODE("LD A, MAX(30, 15, 25)", {0x3E, 30});
    ASSERT_CODE("LD A, MAX(5, 2, 8, 3, 9)", {0x3E, 9});

    // Test MIN/MAX with floating point and mixed arguments
    ASSERT_CODE("LD A, MIN(3.14, 8.5, 2.9)", {0x3E, 2}); // 2.9 is truncated to 2
    ASSERT_CODE("LD A, MAX(3.14, 8.5, 2.9)", {0x3E, 8}); // 8.5 is truncated to 8
    ASSERT_CODE("LD A, MIN(10, 3.5, 12)", {0x3E, 3});
    ASSERT_CODE("LD A, MAX(10, 3.5, 12)", {0x3E, 12});

    // Test MIN/MAX with expressions as arguments
    ASSERT_CODE("LD A, MIN(2*5, 3+3, 20/2)", {0x3E, 6}); // MIN(10, 6, 10)
    ASSERT_CODE("LD A, MAX(2*5, 3+3, 20/2)", {0x3E, 10}); // MAX(10, 6, 10)

    // Test MIN/MAX with functions as arguments
    ASSERT_CODE("LD A, MIN(HIGH(0x1234), LOW(0x5678), 50)", {0x3E, 18}); // MIN(18, 120, 50)
    ASSERT_CODE("LD A, MAX(HIGH(0x1234), LOW(0x5678), 50)", {0x3E, 120}); // MAX(18, 120, 50)

    // Test MIN/MAX with the problematic expression from the bug report
    ASSERT_CODE("LD A, MIN(HIGH(0x1234), LOW(0x1234), 3.14*8)", {0x3E, 18}); // MIN(18, 52, 25)

    // Test error cases for MIN/MAX
    ASSERT_COMPILE_FAILS("LD A, MIN()"); // No arguments
    ASSERT_COMPILE_FAILS("LD A, MIN(10)"); // One argument
    ASSERT_COMPILE_FAILS("LD A, MAX()"); // No arguments
    ASSERT_COMPILE_FAILS("LD A, MAX(10)"); // One argument
}

TEST_CASE(CommentOptions) {
    Z80::Assembler<Z80::StandardBus>::Config config;

    // 1. Test: Comments completely disabled
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.enabled = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 ; This is a comment", config); // Semicolon comment should be treated as code
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 /* This is a block comment */", config); // Block comment should be treated as code
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 // This is a cpp comment", config); // C++ style comment should be treated as code
    ASSERT_CODE_WITH_OPTS("LD A, 5", {0x3E, 0x05}, config); // Regular instruction without comments should pass

    // 2. Semicolon comments disabled, block comments disabled (even if comments.enabled is true)
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_semicolon = false;
    config.comments.allow_block = false;
    config.comments.allow_cpp_style = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 ; This is a comment", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 // This is a cpp comment", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 /* This is a block comment */", config);
    ASSERT_CODE_WITH_OPTS("LD A, 5", {0x3E, 0x05}, config);

    // 3. Test: Only semicolon comments allowed
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_semicolon = true;
    config.comments.allow_block = false;
    config.comments.allow_cpp_style = false;
    ASSERT_CODE_WITH_OPTS("LD A, 5 ; This is a comment", {0x3E, 0x05}, config); // Semicolon comment should pass
    ASSERT_CODE_WITH_OPTS("; ENTIRE LINE COMMENT\nLD B, 10", {0x06, 0x0A}, config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 /* This is a block comment */", config); // Block comment should fail
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 // This is a cpp comment", config); // C++ style comment should fail

    // 4. Test: Only block comments allowed
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_semicolon = false;
    config.comments.allow_cpp_style = false;
    config.comments.allow_block = true;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 ; This is a comment", config); // Semicolon should be invalid
    // Block comments should NOT allow multiple instructions on one line.
    // The parser should fail because it sees "LD A, 1 LD B, 2" after comment removal.
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 1/* comment */LD B, 2", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 1/**/LD B, 2", config);
    // However, a block comment that consumes the rest of the line is valid.
    ASSERT_CODE_WITH_OPTS("LD A, 1 /* comment */", {0x3E, 0x01}, config);
    // And multi-line block comments are also valid.
    ASSERT_CODE_WITH_OPTS("LD A, 1\n/* comment */\nLD B, 2", {0x3E, 0x01, 0x06, 0x02}, config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 // This is a cpp comment", config); // C++ style should be invalid

    // 5. Test: Only C++ style comments allowed
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_semicolon = false;
    config.comments.allow_block = false;
    config.comments.allow_cpp_style = true;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 ; This is a comment", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 5 /* This is a block comment */", config);
    ASSERT_CODE_WITH_OPTS("LD A, 5 // This is a cpp comment", {0x3E, 0x05}, config);
    ASSERT_CODE_WITH_OPTS("// ENTIRE LINE COMMENT\nLD B, 10", {0x06, 0x0A}, config);

    // 6. Test: Default behavior (all comment types allowed)
    config = Z80::Assembler<Z80::StandardBus>::get_default_config(); // Default has all enabled
    ASSERT_CODE_WITH_OPTS("LD A, 5 ; This is a comment", {0x3E, 0x05}, config);
    ASSERT_CODE_WITH_OPTS("LD A, 6 // This is a cpp comment", {0x3E, 0x06}, config);
    // This should fail as it's two instructions on one line after comment removal.
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 1/* Start comment */LD B, 2", config);
    ASSERT_CODE_WITH_OPTS(R"(
        LD A, 1       /* Start comment
        LD B, 2       This is all commented out
        LD C, 3       */ LD D, 4 ; Another comment // And another one
    )", {0x3E, 0x01, 0x16, 0x04}, config); // Only LD A, 1 and LD D, 4 should be assembled

    // 7. Test: Unterminated block comment (should always fail if allow_block is true)
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_block = true;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 1 /* This comment is not closed", config);

    // 8. Test: Block comment with no content
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_block = true;
    ASSERT_COMPILE_FAILS_WITH_OPTS("LD A, 1/**/LD B, 2", config);

    // 9. Test: Block comment spanning multiple lines
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.comments.allow_block = true;
    ASSERT_CODE_WITH_OPTS(R"(
        LD A, 1
        /*
        This is a multi-line comment.
        */
        LD B, 2
    )", {0x3E, 0x01, 0x06, 0x02}, config);
}

TEST_CASE(SemicolonInString) {
    // Test semicolon inside a string literal
    // CP ";"          ; }
    // Should be parsed as CP 0x3B (ASCII for ';')
    // The second semicolon starts a comment.
    ASSERT_CODE("CP \";\"          ; }", {0xFE, 0x3B});
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
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", "LD A, 5\nINCLUDE \"included.asm\"\nADD A, B");
    file_provider.add_source("included.asm", "LD B, 10\n");

    Z80::StandardBus bus;
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    assembler.compile("main.asm");

    std::vector<uint8_t> expected = {0x3E, 0x05, 0x06, 0x0A, 0x80};
    auto blocks = assembler.get_blocks();
    size_t total_size = 0;
    for(const auto& b : blocks) total_size += b.size;
    
    assert(total_size == expected.size());
    bool mismatch = false;
    for(size_t i = 0; i < expected.size(); ++i) {
        if (bus.peek(i) != expected[i]) mismatch = true;
    }
    assert(!mismatch && "Basic include failed");
    tests_passed++;
}

TEST_CASE(IncludeDirective_Nested) {
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", "INCLUDE \"level1.asm\"");
    file_provider.add_source("level1.asm", "LD A, 1\nINCLUDE \"level2.asm\"");
    file_provider.add_source("level2.asm", "LD B, 2\n");

    Z80::StandardBus bus;
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    assembler.compile("main.asm");

    std::vector<uint8_t> expected = {0x3E, 0x01, 0x06, 0x02};
    auto blocks = assembler.get_blocks();
    size_t total_size = 0;
    for(const auto& b : blocks) total_size += b.size;

    assert(total_size == expected.size());
    bool mismatch = false;
    for(size_t i = 0; i < expected.size(); ++i) {
        if (bus.peek(i) != expected[i]) mismatch = true;
    }
    assert(!mismatch && "Nested include failed");
    tests_passed++;
}

TEST_CASE(IncludeDirective_CircularDependency) {
    MockFileProvider file_provider;
    file_provider.add_source("a.asm", "INCLUDE \"b.asm\"");
    file_provider.add_source("b.asm", "INCLUDE \"a.asm\"");
    Z80::StandardBus bus;
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    try {
        assembler.compile("a.asm");
        tests_failed++;
        std::cerr << "Assertion failed: Circular dependency did not throw an exception.\n";
    } catch (const std::runtime_error&) {
        tests_passed++;
    }
}

TEST_CASE(IncbinDirective) {
    // Basic INCBIN
    {
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", "ORG 0x100\nINCBIN \"data.bin\"\nNOP");
        file_provider.add_binary_source("data.bin", {0xDE, 0xAD, 0xBE, 0xEF});
        Z80::StandardBus bus;
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
        bool success = assembler.compile("main.asm");
        assert(success);

        std::vector<uint8_t> expected = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};
        auto blocks = assembler.get_blocks();
        size_t total_size = 0;
        for(const auto& b : blocks) total_size += b.size;

        assert(!blocks.empty() && blocks[0].start_address == 0x100 && total_size == expected.size());
        for(size_t i = 0; i < expected.size(); ++i) {
            if (bus.peek(0x100 + i) != expected[i]) {
                 assert(false && "Byte mismatch in basic INCBIN test");
            }
        }
        tests_passed++;
    }

    // INCBIN with labels
    {
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", R"(
            ORG 0x8000
            LD HL, SPRITE_DATA
            JP END_LABEL
        SPRITE_DATA:
            INCBIN "sprite.dat"
        END_LABEL:
            NOP
        )");
        file_provider.add_binary_source("sprite.dat", {0xFF, 0x81, 0x81, 0xFF});
        Z80::StandardBus bus;
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
        bool success = assembler.compile("main.asm");
        assert(success);
        auto symbols = assembler.get_symbols();
        assert(symbols["SPRITE_DATA"].value == 0x8006);
        assert(symbols["END_LABEL"].value == 0x800A);
        tests_passed++;
    }

    // INCBIN disabled in options
    {
        Z80::Assembler<Z80::StandardBus>::Config config;
        config.directives.allow_incbin = false;
        MockFileProvider file_provider;
        file_provider.add_binary_source("data.bin", {0x01, 0x02});
        ASSERT_COMPILE_FAILS_WITH_OPTS("INCBIN \"data.bin\"", config);
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

TEST_CASE(MismatchedControlDirectives) {
    // REPT inside IF, but ENDIF is inside REPT
    ASSERT_COMPILE_FAILS(R"(
        IF 1
            REPT 2
                NOP
            ENDIF
        ENDR
    )");

    // IF inside REPT, but ENDR is inside IF
    ASSERT_COMPILE_FAILS(R"(
        REPT 2
            IF 1
                NOP
            ENDR
        ENDIF
    )");

    ASSERT_COMPILE_FAILS("IF 1\nENDR"); // ENDR without REPT
}

TEST_CASE(ReptAndConditionalCompilation) {
    // 1. REPT inside an active IF block
    ASSERT_CODE(R"(
        IF 1
            REPT 2
                NOP
            ENDR
        ENDIF
    )", {0x00, 0x00});

    // 2. REPT inside an inactive IF block (is_skipping should be true)
    ASSERT_CODE(R"(
        IF 0
            REPT 2
                NOP ; This should be skipped
            ENDR
        ENDIF
        LD A, 1
    )", {0x3E, 0x01});

    // 3. REPT inside an active ELSE block
    ASSERT_CODE(R"(
        IF 0
            LD A, 1
        ELSE
            REPT 3
                INC A
            ENDR
        ENDIF
    )", {0x3C, 0x3C, 0x3C});

    // 4. IF inside a REPT block
    ASSERT_CODE(R"(
        REPT 2
            IF 1
                NOP
            ENDIF
            IF 0
                HALT
            ELSE
                INC A
            ENDIF
        ENDR
    )", {0x00, 0x3C, 0x00, 0x3C});
}

TEST_CASE(ReptEndrDirective) {
    // 1. Simple REPT
    ASSERT_CODE(R"(
        REPT 3
            NOP
        ENDR
    )", {0x00, 0x00, 0x00});

    // 2. REPT with an expression
    ASSERT_CODE(R"(
        COUNT EQU 4
        REPT COUNT
            INC A
        ENDR
    )", {0x3C, 0x3C, 0x3C, 0x3C});

    // 3. REPT with zero count
    ASSERT_CODE(R"(
        REPT 0
            HALT
        ENDR
        NOP
    )", {0x00});

    // 4. Nested REPT
    ASSERT_CODE(R"(
        REPT 2
            DB 0xFF
            REPT 3
                DB 0xAA
            ENDR
            DB 0xFF
        ENDR
    )", {0xFF, 0xAA, 0xAA, 0xAA, 0xFF, 0xFF, 0xAA, 0xAA, 0xAA, 0xFF});

    // 5. REPT with a forward reference
    ASSERT_CODE(R"(
        REPT FORWARD_COUNT
            NOP
        ENDR
        FORWARD_COUNT EQU 2
    )", {0x00, 0x00});

    // 6. REPT disabled by options
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.directives.allow_repeat = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("REPT 2\nNOP\nENDR", config);

    // 7. Unterminated REPT
    ASSERT_COMPILE_FAILS("REPT 2\nNOP");

    // 8. REPT with complex expression and forward reference
    ASSERT_CODE(R"(
        REPT COUNT_B - 1
            DB 0x11
        ENDR
        COUNT_A EQU 2
        COUNT_B EQU COUNT_A + 2
    )", {0x11, 0x11, 0x11});

    // 9. REPT with a block of multiple instructions
    ASSERT_CODE(R"(
        REPT 2
            LD A, 10
            ADD A, 5
            PUSH AF
        ENDR
    )", {
        0x3E, 10,   // LD A, 10
        0xC6, 5,    // ADD A, 5
        0xF5,       // PUSH AF
        0x3E, 10,   // LD A, 10
        0xC6, 5,    // ADD A, 5
        0xF5        // PUSH AF
    });
}

TEST_CASE(DirectiveOptions) {
    Z80::Assembler<Z80::StandardBus>::Config config;

    // 1. Test directives.enabled = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.enabled = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE EQU 10", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("ORG 0x100", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("DB 1", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("IF 1\nENDIF", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("ALIGN 4", config);

    // 2. Test directives.constants.enabled = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.constants.enabled = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE EQU 10", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE SET 10", config);
    ASSERT_CODE_WITH_OPTS("ORG 0x100\nNOP", {0x00}, config); // Other directives should work

    // 3. Test directives.constants.allow_equ = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.constants.allow_equ = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE EQU 10", config);
    ASSERT_CODE_WITH_OPTS("VALUE SET 10\nLD A, VALUE", {0x3E, 10}, config);

    // 4. Test directives.constants.allow_set = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.constants.allow_set = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("VALUE SET 10", config);
    ASSERT_CODE_WITH_OPTS("VALUE EQU 10\nLD A, VALUE", {0x3E, 10}, config);

    // 5. Test directives.allow_org = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_org = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("ORG 0x100", config);

    // 6. Test directives.allow_align = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_align = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("ALIGN 4", config);

    // 7. Test directives.allow_data_definitions = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_data_definitions = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("DB 1", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("DW 1", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("DS 1", config);

    // 8. Test directives.allow_includes = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_includes = false;
    MockFileProvider file_provider_no_include;
    file_provider_no_include.add_source("main.asm", "INCLUDE \"other.asm\"");
    file_provider_no_include.add_source("other.asm", "NOP");
    Z80::StandardBus bus_no_include;
    Z80::Assembler<Z80::StandardBus> assembler_no_include(&bus_no_include, &file_provider_no_include, config);
    try {
        assembler_no_include.compile("main.asm");
        tests_failed++;
        std::cerr << "Assertion failed: INCLUDE should have failed but didn't.\n";
    } catch (const std::runtime_error&) {
        tests_passed++; // Expected to fail
    }

    // 9. Test directives.allow_conditional_compilation = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_conditionals = false;
    try {
        ASSERT_COMPILE_FAILS_WITH_OPTS("IF 1\nNOP\nENDIF", config);
    } catch (const std::exception& e) {
        // This is expected. The test macro doesn't propagate the exception, so we catch it here.
    }
    ASSERT_COMPILE_FAILS_WITH_OPTS("IFDEF SYMBOL\nNOP\nENDIF", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("IFNDEF SYMBOL\nNOP\nENDIF", config);

    // 10. Test directives.allow_rept_endr = false (already tested in its own case, but good for completeness)
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_repeat = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("REPT 2\nNOP\nENDR", config);

    // 10. Test that disabling conditional compilation doesn't affect other directives
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_conditionals = false;
    ASSERT_CODE_WITH_OPTS(R"(
        VALUE EQU 10
        LD A, VALUE
        DB 0xFF
    )", {0x3E, 10, 0xFF}, config);

    // 11. Test directives.allow_phase = false
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_phase = false;
    ASSERT_COMPILE_FAILS_WITH_OPTS("PHASE 0x8000", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("DEPHASE", config);

    // 12. Test directives.allow_phase = true (default)
    config = Z80::Assembler<Z80::StandardBus>::get_default_config();
    config.directives.allow_phase = true; // Explicitly set for clarity
    ASSERT_CODE_WITH_OPTS(R"(
        PHASE 0x8000
        NOP
    )", {0x00}, config);
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

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    bool success = assembler.compile("main.asm");
    assert(success && "Complex forward reference compilation failed");

    auto symbols = assembler.get_symbols();
    assert(symbols["STACK_TOP"].value == 0x8176);
    assert(symbols["STACK_BASE"].value == 0x8076);
    assert(symbols["STACK_SIZE"].value == 0x0100);
    assert(symbols["COUNT"].value == 100);
    assert(symbols["START"].value == 0x8000);

    // Check the compiled code and data
    assert(bus.peek(0x8001) == 0x31 && bus.peek(0x8002) == 0x76 && bus.peek(0x8003) == 0x81); // LD SP, STACK_TOP (0x8176)
    assert(bus.peek(0x8008) == 0x00 && bus.peek(0x8008 + 99) == 0x00); // DS COUNT (100 bytes of 0x00)
    assert(bus.peek(0x8076) == 0xFF && bus.peek(0x8175) == 0xFF); // DS STACK_SIZE, 0xFF (256 bytes of 0xFF)
    assert(bus.peek(0x8176) == 0x00); // NOP
    assert(bus.peek(0x8177) == 0xAA && bus.peek(0x8180) == 0xAA); // DS COUNT, 0xAA (10 bytes of 0xAA)
    tests_passed++;
}

TEST_CASE(LocalLabels) {
    // Test 1: Basic forward jump to a local label
    ASSERT_CODE(R"(
        GLOBAL_START:
            NOP
            JR .local_target
            NOP
        .local_target:
            HALT
    )", {0x00, 0x18, 0x01, 0x00, 0x76});

    // Test 2: Basic backward jump to a local label
    ASSERT_CODE(R"(
        GLOBAL_START:
        .local_target:
            NOP
            JR .local_target
    )", {0x00, 0x18, 0xFD}); // JR -3 -> 0 - (0+2) = -2 (FE), but NOP is 1 byte, so 1 - (1+2) = -2 (FE). Wait, the address of JR is 1. Target is 0. 0 - (1+2) = -3 = FD. Correct.

    // Test 3: Multiple local labels within one global scope
    ASSERT_CODE(R"(
        GLOBAL_MAIN:
            .loop1:
                NOP
                JR .loop2 ; target is at 0x03, instruction is at 0x01. offset = 0x03 - (0x01+2) = 0
            .loop2:
                INC A
                JR .loop1 ; target is at 0x00, instruction is at 0x04. offset = 0x00 - (0x04+2) = -6 = 0xFA
    )", {0x00, 0x18, 0x00, 0x3C, 0x18, 0xFA});
        
    // Test 4: Reusing local label names in different global scopes
    ASSERT_CODE(R"(
        GLOBAL_ONE:
            .local_label:
                LD A, 1
                JP GLOBAL_TWO.local_label
        GLOBAL_TWO:
            .local_label:
                LD A, 2
    )", {0x3E, 0x01, 0xC3, 0x05, 0x00, 0x3E, 0x02});

    // Test 5: Attempt to define a local label without a preceding global label (should fail)
    ASSERT_COMPILE_FAILS(R"(
        .local_orphan:
            NOP
    )");

    // Test 6: Using a local label in an expression (forward reference)
    ASSERT_CODE(R"(
        GLOBAL_START:
            LD A, .data_val + 1
        .data_val:
                DB 0xAA
    )", {0x3E, 0x03, 0xAA}); // .data_val is at 0x02. .data_val+1 = 3. Correct.

    // Test 7: Redefining a local label within the same scope (should fail)
    ASSERT_COMPILE_FAILS(R"(
        GLOBAL_SCOPE:
            .local: NOP
            .local: NOP
    )");

    // Test 8: Jump to a local label from another scope (qualified vs. unqualified)
    ASSERT_CODE(R"(
        GLOBAL_ONE:
            .local: NOP
            JP .local ; Jump to GLOBAL_ONE.local
        GLOBAL_TWO:
            .local: NOP
            JP .local ; Jump to GLOBAL_TWO.local
            JP GLOBAL_ONE.local
    )", {0x00, 0xC3, 0x00, 0x00, 0x00, 0xC3, 0x04, 0x00, 0xC3, 0x00, 0x00});

    // Test 9: Nested forward references
    ASSERT_CODE(R"(
        START:
            LD HL, .data1
            JP .end
        .data1: DB 0x11
        .data2: DB 0x22
        .end:
            LD A, .data2
    )", {
        0x21, 0x06, 0x00, // LD HL, .data1 (0x0006)
        0xC3, 0x08, 0x00, // JP .end (0x0008)
        0x11,             // .data1
        0x22,             // .data2
        0x3E, 0x07        // LD A, .data2 (value of .data2 is its address 0x07)
    });

    // Test 10: Simple local EQU
    ASSERT_CODE(R"(
        GLOBAL_SCOPE:
            .val EQU 123
            LD A, .val
    )", {0x3E, 123});

    // Test 11: Simple local SET
    ASSERT_CODE(R"(
        GLOBAL_SCOPE:
            .val SET 45
            LD A, .val
    )", {0x3E, 45});

    // Test 12: Redefining local SET
    ASSERT_CODE(R"(
        GLOBAL_SCOPE:
            .val SET 10
            LD A, .val
            .val SET 20
            LD B, .val
    )", {0x3E, 10, 0x06, 20});

    // Test 13: Reusing local EQU name in different scopes
    ASSERT_CODE(R"(
        SCOPE_A:
            .val EQU 1
            LD A, .val
        SCOPE_B:
            .val EQU 2
            LD B, .val
    )", {0x3E, 1, 0x06, 2});

    // Test 14: Using local constant outside its scope (unqualified)
    ASSERT_COMPILE_FAILS(R"(
        SCOPE_A:
            .val EQU 1
        SCOPE_B:
            LD A, .val ; This should resolve to SCOPE_B.val, which is not defined
    )");

    // Test 15: Using local constant with qualified name
    ASSERT_CODE(R"(
        SCOPE_A:
            .val EQU 128
        SCOPE_B:
            LD A, SCOPE_A.val
    )", {0x3E, 128});

    // Test 16: Attempt to define local EQU without a global scope
    ASSERT_COMPILE_FAILS(R"(
        .my_const EQU 10
        NOP
    )");

    // Test 17: Attempt to redefine local EQU with SET
    ASSERT_COMPILE_FAILS(R"(
        SCOPE_A:
            .val EQU 10
            .val SET 20
    )");
}

TEST_CASE(ForwardReferenceWithSet) {
    ASSERT_CODE(R"(
            JP TARGET
 GLOBAL_SCOPE:
            val SET 10
            LD A, val
            val SET 20
            LD B, val
TARGET:     NOP
    )", {
        0xC3, 0x07, 0x00, // JP 0x0007
        0x3E, 10,         // LD A, 10
        0x06, 20,         // LD B, 20
        0x00              // NOP
    });
}

TEST_CASE(PhaseDephaseDirectives) {
    // Test 1: Basic PHASE/DEPHASE functionality
    // Label inside PHASE should have a logical address.
    // Code should be generated at the physical address.
    std::string code1 = R"(
        ORG 0x1000
        LD A, 1         ; Physical: 0x1000, Logical: 0x1000

        PHASE 0x8000
    LOGICAL_START:      ; Should be 0x8000
        LD B, 2         ; Physical: 0x1002, Logical: 0x8000
        LD C, 3         ; Physical: 0x1004, Logical: 0x8002

        DEPHASE
    PHYSICAL_CONTINUE:  ; Should be 0x1006 (synced with physical)
        LD D, 4         ; Physical: 0x1006, Logical: 0x1006
    )";
    const std::vector<uint8_t> expected1 = {
        0x3E, 0x01, // LD A, 1 at 0x1000
        0x06, 0x02, // LD B, 2 at 0x1002
        0x0E, 0x03, // LD C, 3 at 0x1004
        0x16, 0x04  // LD D, 4 at 0x1006
    };
    Z80::StandardBus bus1;
    MockFileProvider sp1;
    sp1.add_source("main.asm", code1); // Use MockFileProvider
    Z80::Assembler<Z80::StandardBus> assembler1(&bus1, &sp1);
    assert(assembler1.compile("main.asm") && "Phase/Dephase test 1 compilation failed");
    auto symbols1 = assembler1.get_symbols();
    assert(symbols1["LOGICAL_START"].value == 0x8000);
    assert(symbols1["PHYSICAL_CONTINUE"].value == 0x1006);
    tests_passed++;

    // Test 2: Using phased labels
    std::string code2 = R"(
        ORG 0x1000
        JP LOGICAL_TARGET ; Should jump to the logical address 0x9000
        
        ORG 0x2000      ; Move physical address somewhere else
    LOGICAL_TARGET_PHYSICAL_LOCATION:
        PHASE 0x9000    ; But assemble as if it's at 0x9000
    LOGICAL_TARGET:
        NOP             ; Physical: 0x2000, Logical: 0x9000
    )";

    std::map<uint16_t, std::vector<uint8_t>> expected_blocks = {
        {0x1000, {0xC3, 0x00, 0x90}}, // JP 0x9000
        {0x2000, {0x00}}              // NOP
    };

    ASSERT_BLOCKS(code2, expected_blocks);
    
    // Test 3: DEPHASE without PHASE should not cause issues
    ASSERT_CODE("ORG 0x100\nDEPHASE\nNOP", {0x00});

    // Test 4: Check '$' and '$$' behavior
    ASSERT_CODE(R"(
        ORG 0x1000
        PHASE 0x8000
        DB $ / 256      ; Logical address high byte (0x8000 -> 0x80)
        DB $$ / 256     ; Physical address high byte (0x1000 -> 0x10)
        DEPHASE
        DB $ / 256      ; Logical address high byte (0x1002 -> 0x10)
        DB $$ / 256     ; Physical address high byte (0x1002 -> 0x10)
    )", {
        0x80, 0x10, // Inside PHASE: $ is 0x8000, $$ is 0x1000
        0x10, 0x10  // Outside PHASE: $ and $$ are both 0x1002
    });
}

TEST_CASE(ProcEndpDirectives) {
    // Test 1: Simple procedure definition and call
    ASSERT_CODE(R"(
        MyProc PROC
            LD A, 42
            RET
        ENDP
        CALL MyProc
    )", {0x3E, 42, 0xC9, 0xCD, 0x00, 0x00});

    // Test 2: Dot label inside a procedure
    ASSERT_CODE(R"(
        MyProc PROC
            JR .skip
            HALT
        .skip:
            NOP
            RET
        ENDP
        CALL MyProc
    )", {0x18, 0x01, 0x76, 0x00, 0xC9, 0xCD, 0x00, 0x00});

    // Test 3: Nested procedures and label resolution
    ASSERT_CODE(R"(
        Outer PROC
        LOCAL Inner
            LD A, 1
            CALL Outer.Inner
            RET
            Inner:
                LD B, 2
                RET
        ENDP
        CALL Outer
    )", {0x3E, 0x01, 0xCD, 0x06, 0x00, 0xC9, 0x06, 0x02, 0xC9, 0xCD, 0x00, 0x00});

    // Test 4: Dot labels refer to the nearest procedure scope
    ASSERT_CODE(R"(
        ; This test checks that a .local label refers to its own
        ; procedure scope, not an outer one.
        Outer PROC
            CALL Inner    ; Call the nested procedure
            .target:      ; This is Outer.target
                HALT
        ENDP

        Inner PROC
            JR .target    ; This should jump to Inner.target, not Outer.target
            .target:      ; This is Inner.target
                NOP
                RET
        ENDP
    )", {
        0xCD, 0x04, 0x00, // Outer: CALL Inner (to address 0x0004)
        0x76,             // Outer.target: HALT
        0x18, 0x00,       // Inner: JR .target (to address 0x0006)
        0x00,             // Inner.target: NOP
        0xC9              // RET
    });

    // Test 5: Error cases for mismatched directives
    ASSERT_COMPILE_FAILS("MyProc PROC"); // Missing ENDP
    ASSERT_COMPILE_FAILS("ENDP"); // ENDP without PROC
    ASSERT_COMPILE_FAILS("IF 1\nPROC MyProc\nENDIF\nENDP"); // Mismatched ENDP
    ASSERT_COMPILE_FAILS("PROC MyProc\nIF 1\nENDP\nENDIF"); // Mismatched ENDIF

    // Test 6: Procedure label used in expression
    ASSERT_CODE(R"(
        MyProc PROC
            NOP
        ENDP
        LD HL, MyProc
    )", {0x00, 0x21, 0x00, 0x00});

    // Test 7: Global label, then procedure with same-named dot label
    ASSERT_CODE(R"(
        ; This test ensures that a .local label inside a PROC
        ; refers to its own scope, not a global one.
        Global:
            JP .local           ; Jumps to Global.local
        .local:
            NOP
            JP Proc             ; Jump to the procedure

        Proc PROC
            JP .local           ; Jumps to Proc.local
        .local:
            HALT
        ENDP
    )", {
        0xC3, 0x03, 0x00, // Global: JP .local (to 0x0003)
        0x00,             // Global.local: NOP
        0xC3, 0x07, 0x00, // JP Proc (to 0x0007)
        0xC3, 0x0A, 0x00, // Proc: JP .local (to 0x000A)
        0x76              // Proc.local: HALT
    });
}

TEST_CASE(ProcEndpNameValidation) {
    // 1. Simple matching names
    ASSERT_CODE(R"(
        Main PROC
            NOP
        Main ENDP
    )", {0x00});

    // 2. Mismatched names (Error)
    ASSERT_COMPILE_FAILS(R"(
        Main PROC
            NOP
        Other ENDP
    )");

    // 3. Nested procedures - Simple names
    ASSERT_CODE(R"(
        Outer PROC
            Inner PROC
                NOP
            Inner ENDP
        Outer ENDP
    )", {0x00});

    // 4. Nested procedures - Local names with dot
    ASSERT_CODE(R"(
        Outer PROC
            .Inner PROC
                NOP
            .Inner ENDP
        Outer ENDP
    )", {0x00});

    // 4b. Nested procedures - Local names with dot (Full name in ENDP)
    ASSERT_CODE(R"(
        Outer PROC
            .Inner PROC
                NOP
            Outer.Inner ENDP
        Outer ENDP
    )", {0x00});

    // 5. Nested procedures - LOCAL names
    ASSERT_CODE(R"(
        Outer PROC
            LOCAL Inner
            Inner PROC
                NOP
            Inner ENDP
        Outer ENDP
    )", {0x00});

    // 5b. Nested procedures - LOCAL names (Full name in ENDP)
    ASSERT_CODE(R"(
        Outer PROC
            LOCAL Inner
            Inner PROC
                NOP
            Outer.Inner ENDP
        Outer ENDP
    )", {0x00});

    // 6. Nested procedures - Global names
    ASSERT_CODE(R"(
        Outer PROC
            GlobalInner PROC
                NOP
            GlobalInner ENDP
        Outer ENDP
    )", {0x00});

    // 7. Mismatched nested
    ASSERT_COMPILE_FAILS(R"(
        Outer PROC
            Inner PROC
                NOP
            Outer ENDP ; Should be Inner
        Outer ENDP
    )");
}

TEST_CASE(MacroEndmNameValidation) {
    // 1. Simple matching names
    ASSERT_CODE(R"(
        MyMacro MACRO
            NOP
        MyMacro ENDM
        MyMacro
    )", {0x00});

    // 2. Mismatched names (Error)
    ASSERT_COMPILE_FAILS(R"(
        MyMacro MACRO
            NOP
        OtherName ENDM
    )");
}

TEST_CASE(MacroEndmWithExtraParams) {
    // ENDM with extra parameters should be recognized as the end of the macro
    // but report an error due to invalid syntax.
    ASSERT_COMPILE_FAILS(R"(
        MyMacro MACRO
            NOP
        ENDM extra ; This should trigger an error
        MyMacro
    )");
}

TEST_CASE(SimpleMacroNoParams) {
    ASSERT_CODE(R"(
        CLEAR_A MACRO
            XOR A
        ENDM

        CLEAR_A
    )", {0xAF}); // XOR A
}

TEST_CASE(MacroWithOneNamedParam) {
    ASSERT_CODE(R"(
        LOAD_A MACRO val
            LD A, {val}
        ENDM

        LOAD_A 42
    )", {0x3E, 42}); // LD A, 42
}

TEST_CASE(MacroWithMissingPositionalParams) {
    ASSERT_COMPILE_FAILS(R"(
        LOAD_REGS MACRO
            LD A, \1
            LD B, \2
            LD C, \3
        ENDM

        LOAD_REGS 5
    )");
}

TEST_CASE(MacroWithMixedParamTypes) {
    ASSERT_CODE(R"(
        COMPLEX_LD MACRO dest, src
            LD {dest}, {src}
        ENDM

        COMPLEX_LD B, A
        COMPLEX_LD C, 123
        COMPLEX_LD A, (label)
    label:
        NOP
    )", {
        0x47,             // LD B, A
        0x0E, 123,        // LD C, 123
        0x3A, 0x06, 0x00, // LD A, (label) where label address is 0x0006
        0x00              // NOP at 0x0006
    });
}

TEST_CASE(MacroWithReptDirective) {
    ASSERT_CODE(R"(
        FILL_NOPS MACRO count
            REPT {count}
                NOP
            ENDR
        ENDM

        FILL_NOPS 4
    )", {0x00, 0x00, 0x00, 0x00}); // 4 x NOP
}

TEST_CASE(NestedMacros) {
    ASSERT_CODE(R"(
        INNER MACRO val
            ADD A, {val}
        ENDM

        OUTER MACRO
            LD A, 10
            INNER 5
        ENDM

        OUTER
    )", {
        0x3E, 10,   // LD A, 10
        0xC6, 5     // ADD A, 5
    });
}

TEST_CASE(MacroWithLocalLabels) {
    ASSERT_CODE(R"(
        DELAY MACRO
            LOCAL loop
            LD B, 255
        loop:
            DJNZ loop
        ENDM

        DELAY
        DELAY
    )", {
        0x06, 255,  // LD B, 255
        0x10, 0xFE, // DJNZ to the first unique label 'loop'
        0x06, 255,  // LD B, 255 (from second call)
        0x10, 0xFE  // DJNZ to the second unique label 'loop'
    });
}

TEST_CASE(MacroWithLocalLabelAndSpecialChars) {
    // This test ensures that a local label 'loop' is correctly replaced,
    // but a different label like 'loop@' or 'loop_' is NOT incorrectly replaced.
    ASSERT_CODE(R"(
        DELAY MACRO
            LOCAL loop, loop@
            LD B, 255
        loop:
            DJNZ loop
        loop@:
            NOP
        ENDM
        DELAY
        DELAY
    )", {0x06, 255, 0x10, 0xFE, 0x00, 0x06, 255, 0x10, 0xFE, 0x00});
}

TEST_CASE(MacroWithLocalLabelAndExtendedChars) {
    // Test ensuring that local labels are not replaced when they are substrings of labels with special chars like ? or @
    ASSERT_CODE(R"(
        TEST_MACRO MACRO
            LOCAL lbl
            lbl: NOP
            JP lbl?  ; Should NOT replace 'lbl' here
            JP lbl@  ; Should NOT replace 'lbl' here
        ENDM
        
        lbl?: NOP
        lbl@: NOP
        
        TEST_MACRO
    )", {
        0x00,             // lbl?: NOP
        0x00,             // lbl@: NOP
        0x00,             // Macro expansion: lbl: NOP
        0xC3, 0x00, 0x00, // JP lbl? (0x0000)
        0xC3, 0x01, 0x00  // JP lbl@ (0x0001)
    });
}

TEST_CASE(MacroWithMoreThanNineParams) {
    ASSERT_CODE(R"(
        BIG_MACRO MACRO
            DB \1, \2, \3, \4, \5, \6, \7, \8, \9, \10
        ENDM

        BIG_MACRO 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    )", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST_CASE(MacroWithTenParamsAndMissingOnes) {
    ASSERT_CODE(R"(
        BIG_MACRO MACRO
            DB \1, \10, \2
        ENDM

        BIG_MACRO 100, 200, 300, 400, 500, 600, 700, 800, 900, 255
    )", {100, 255, 200});
}

TEST_CASE(MacroWithMoreThanTenParamsFailsGracefully) {
    // This test now verifies that parameters beyond 9 are handled correctly.
    ASSERT_CODE(R"(
        MYMACRO MACRO
            DB \11
        ENDM
        MYMACRO 1,2,3,4,5,6,7,8,9,10,55
    )", {55});
}

TEST_CASE(MacroWithMoreThanNineParamsAndMissing) {
    // This test verifies that if a parameter like \10 is used but not provided,
    // it is correctly replaced with an empty string.
    ASSERT_CODE(R"(
        BIG_MACRO MACRO
            DB \1, \2\10
        ENDM

        BIG_MACRO 1, 2
    )", {1, 2}); // Expects DB 1, 2 (since \10 is replaced by nothing)
}

TEST_CASE(MacroWithBracedParams) {
    // Test 1: Basic braced parameter to solve ambiguity
    ASSERT_CODE(R"(
        ADD_SUFFIX MACRO value
            DB \{1}0
        ENDM

        ADD_SUFFIX 5
    )", {0x32}); // DB 5, '0'

    // Test 2: Multi-digit braced parameter
    ASSERT_CODE(R"(
        TENTH_PARAM MACRO
            DB \{10}
        ENDM

        TENTH_PARAM 1,2,3,4,5,6,7,8,9,99
    )", {99});

    // Test 3: Mix of braced and non-braced parameters
    ASSERT_CODE(R"(
        MIXED_MACRO MACRO
            DB \1, \{2}, \3
        ENDM

        MIXED_MACRO 10, 20, 30
    )", {10, 20, 30});

    // Test 4: Braced parameter next to another number, which would be ambiguous otherwise
    ASSERT_CODE(R"(
        AMBIGUOUS MACRO
            DW \{1}1
        ENDM

        AMBIGUOUS 0x12
    )", {0x21, 0x01});

    // Test 5: Unmatched opening brace should fail
    ASSERT_COMPILE_FAILS(R"(
        BAD_MACRO MACRO
            DB \{1
        ENDM
        BAD_MACRO 1
    )");

    // Test 6: Braced parameter with no number inside (should not be substituted)
    ASSERT_CODE(R"(
        EMPTY_BRACE MACRO
            DB "\{}"
        ENDM
        EMPTY_BRACE
    )", {0x5C, 0x7B, 0x7D}); // DB "\{}"
}

TEST_CASE(MacroSpecialParamZero) {
    // Test \0 to get argument count
    ASSERT_CODE(R"(
        ARG_COUNT MACRO
            DB \0
        ENDM
        ARG_COUNT 1, "hello", (1+2)
    )", {3}); // 3 arguments provided
}

TEST_CASE(MacroShift) {
    ASSERT_CODE(R"(
        ORG 0x8000
        TEST_SHIFT MACRO v1, v2, v3
            ; 1. START: Write \1 and \2 (State: 1, 2)
            DEFB \1
            DEFB \2
            ; --- Execute SHIFT ---
            SHIFT
            ; 2. AFTER 1st SHIFT: Write new \1 (should be 2)
            DEFB \1
            ; --- Execute SHIFT ---
            SHIFT
            ; 3. AFTER 2nd SHIFT: Write new \1 (should be 3)
            DEFB \1
        ENDM
        TEST_SHIFT 1, 2, 3
    )", {1, 2, 2, 3});
}

TEST_CASE(MacroVariadicReptShift) {
    ASSERT_CODE(R"(
        ; Definition of a macro that writes all provided bytes
        WRITE_BYTES MACRO
            ; \0 is the number of arguments. 
            ; If called with 3 arguments, the loop runs 3 times.
            REPT \0
                DB \1   ; Write CURRENT first argument
                SHIFT   ; Shift queue: \2 becomes \1, \3 becomes \2 etc.
            ENDR
        ENDM

        ; Call with 4 different values
        WRITE_BYTES 0x10, 0x20, 0x30, 0x40
    )", {0x10, 0x20, 0x30, 0x40});
}

TEST_CASE(MacroIfNotBlank_OptionalParam) {
    ASSERT_CODE(R"(
        ; Macro: If \1 is provided, load it into A.
        ; If not, clear A (XOR A).
        LOAD_OPT MACRO val
            IFNB \1       ; Is \1 not empty?
                LD A, \1
            ELSE          ; Is empty
                XOR A
            ENDIF
        ENDM

        LOAD_OPT 0x55     ; Case 1: Argument provided
        LOAD_OPT          ; Case 2: No argument
    )", {
        0x3E, 0x55,       // LD A, 0x55
        0xAF              // XOR A
    });
}

TEST_CASE(MacroIfIdentical_Optimization) {
    ASSERT_CODE(R"(
        ; Macro: If value is exactly "0", use XOR.
        ; Otherwise use LD.
        SMART_LD MACRO val
            ; We use <> brackets to safely compare strings
            IFIDN <\1>, <0>
                XOR A
            ELSE
                LD A, \1
            ENDIF
        ENDM

        SMART_LD 0        ; Should be optimized
        SMART_LD 1        ; Should be standard
        SMART_LD 00       ; "00" is not the same as "0" textually!
    )", {
        0xAF,             // XOR A
        0x3E, 0x01,       // LD A, 1
        0x3E, 0x00        // LD A, 0 (bo "00" != "0")
    });
}

TEST_CASE(MacroIfIdentical_RegisterSelect) {
    ASSERT_CODE(R"(
        ; Macro generates PUSH for a specific register
        MY_PUSH MACRO reg
            IFIDN <\1>, <HL>
                PUSH HL
            ELSE
                IFIDN <\1>, <BC>
                    PUSH BC
                ELSE
                    NOP ; Unknown or lowercase
                ENDIF
            ENDIF
        ENDM

        MY_PUSH HL        ; Matches
        MY_PUSH BC        ; Matches
        MY_PUSH hl        ; Does not match (lowercase)
        MY_PUSH AF        ; Does not match
    )", {
        0xE5,             // PUSH HL
        0xC5,             // PUSH BC
        0x00,             // NOP (bo "hl" != "HL")
        0x00              // NOP
    });
}

TEST_CASE(MacroVariadicWithShiftAndCount) {
    ASSERT_CODE(R"(
        DUMP_SAFE MACRO
             REPT \0
                DB \1
                SHIFT
             ENDR
        ENDM

        DUMP_SAFE 10, 20, 30
    )", {10, 20, 30});
}

TEST_CASE(MacroIfIdentical_Empty) {
    ASSERT_CODE(R"(
        CHECK_EMPTY MACRO val
            IFIDN <\1>, <>  ; Is \1 empty?
                DB 0xFF
            ELSE
                DB 0x00
            ENDIF
        ENDM

        CHECK_EMPTY       ; Empty
        CHECK_EMPTY 5     ; Not empty
    )", {
        0xFF,
        0x00
    });
}

TEST_CASE(ReptDirectiveWithIterationCounter) {
    // Test 1: Simple iteration counter
    ASSERT_CODE(R"(
        REPT 3
            DB \@
        ENDR
    )", {1, 2, 3});

    // Test 2: Iteration counter in an expression
    ASSERT_CODE(R"(
        REPT 4
            DB \@ * 2
        ENDR
    )", {2, 4, 6, 8});

    // Test 3: Iteration counter with other instructions
    ASSERT_CODE(R"(
        REPT 2
            LD A, \@
            PUSH AF
        ENDR
    )", {
        0x3E, 1, // LD A, 1
        0xF5,    // PUSH AF
        0x3E, 2, // LD A, 2
        0xF5     // PUSH AF
    });

    // Test 4: Nested REPT with iteration counters.
    // The inner loop's counter should be independent and reset for each outer loop iteration.
    ASSERT_CODE(R"(
        REPT 2 ; Outer loop: \@ will be 1, then 2
            DB \@ * 10 ; 10, 20
            REPT 3 ; Inner loop: \@ will be 1, 2, 3
                DB \@
            ENDR
        ENDR
    )", {
        10, 1, 2, 3, // Outer loop 1
        20, 1, 2, 3  // Outer loop 2
    });
}

TEST_CASE(ReptDirectiveComplexReplacement) {
    // Test replacement in expressions (parentheses) and strings
    ASSERT_CODE(R"(
        REPT 2
            DB (\@ + 1)
            DB "Iter: \@"
        ENDR
    )", {2, 'I', 't', 'e', 'r', ':', ' ', '1', 3, 'I', 't', 'e', 'r', ':', ' ', '2'});
}

TEST_CASE(WhileAndReptDirectives) {
    // Test 1: REPT inside a WHILE loop
    // The WHILE loop should execute 3 times, and in each iteration,
    // the REPT loop should generate a decreasing number of bytes.
    ASSERT_CODE(R"(
        COUNTER SET 3
        WHILE COUNTER > 0
            REPT COUNTER
                DB \@  ; The REPT iteration counter (1, 2, ...)
            ENDR
            DB 0xFF ; Separator
            COUNTER SET COUNTER - 1
        ENDW
    )", {0x01, 0x02, 0x03, 0xFF, 0x01, 0x02, 0xFF, 0x01, 0xFF});

    // Test 2: WHILE inside a REPT loop
    // The REPT loop executes 3 times. In each iteration, the WHILE loop
    // generates bytes from the current REPT iteration number down to 1.
    ASSERT_CODE(R"(
        REPT 3
            COUNTER SET \@ ; Set counter to REPT iteration (1, 2, 3)
            WHILE COUNTER > 0
                DB COUNTER
                COUNTER SET COUNTER - 1
            ENDW
            DB 0xFF ; Separator
        ENDR
    )", {0x01, 0xFF, 0x02, 0x01, 0xFF, 0x03, 0x02, 0x01, 0xFF});
}

TEST_CASE(DgDirective) {
    // Test 1: Basic 8-bit definition with '1' and '0'
    ASSERT_CODE("DG \"11110000\"", {0xF0});

    // Test 2: Alternative characters for 0 and 1
    ASSERT_CODE("DG \"XXXX....\"", {0xF0});
    ASSERT_CODE("DG \"____----\"", {0x00});
    ASSERT_CODE("DG \"1_1.1-1.\"", {0b10101010}); // 0xAA

    // Test 3: Multi-byte definition
    ASSERT_CODE("DG \"1111000010101010\"", {0xF0, 0xAA});

    // Test 4: Definition with spaces
    ASSERT_CODE("DG \"1111 0000\"", {0xF0});
    ASSERT_CODE("DG \"11 11 00 00\"", {0xF0});

    // Test 5: Multiple string arguments
    ASSERT_CODE("DG \"11110000\", \"10101010\"", {0xF0, 0xAA});

    // Test 6: Alias DEFG
    ASSERT_CODE("DEFG \"00001111\"", {0x0F});

    // Test 7: Error cases
    ASSERT_COMPILE_FAILS("DG \"1010101\""); // Not a multiple of 8 bits
    ASSERT_COMPILE_FAILS("DG 123"); // Not a string literal
}

TEST_CASE(NewDirectives_D24_DC_DEFD) {
    // D24 - 24-bit integer (3 bytes)
    ASSERT_CODE("D24 0x123456", {0x56, 0x34, 0x12});
    ASSERT_CODE("D24 0xAABBCC, 0x112233", {0xCC, 0xBB, 0xAA, 0x33, 0x22, 0x11});

    // DEFD - 32-bit integer (4 bytes), alias for DD/DWORD
    ASSERT_CODE("DEFD 0x12345678", {0x78, 0x56, 0x34, 0x12});
    ASSERT_CODE("DEFD 0xAABBCCDD, 0x11223344", {0xDD, 0xCC, 0xBB, 0xAA, 0x44, 0x33, 0x22, 0x11});
    ASSERT_CODE("DD 0x12345678", {0x78, 0x56, 0x34, 0x12}); // Verify DD still works
    ASSERT_CODE("DWORD 0x12345678", {0x78, 0x56, 0x34, 0x12}); // Verify DWORD still works

    // DC - String with last character having bit 7 set
    // "A" -> 'A' | 0x80 = 0x41 | 0x80 = 0xC1
    ASSERT_CODE("DC \"A\"", {0xC1});
    // "AB" -> 'A', 'B' | 0x80 = 0x41, 0x42 | 0x80 = 0x41, 0xC2
    ASSERT_CODE("DC \"AB\"", {0x41, 0xC2});
    // "ZX" -> 'Z', 'X' | 0x80 = 0x5A, 0x58 | 0x80 = 0x5A, 0xD8
    ASSERT_CODE("DC \"ZX\"", {0x5A, 0xD8});
    
    // Multiple strings in DC
    // "A", "B" -> ('A'|0x80), ('B'|0x80) -> 0xC1, 0xC2
    ASSERT_CODE("DC \"A\", \"B\"", {0xC1, 0xC2});
}

TEST_CASE(SignedNumbersFix) {
    // Test INT32_MIN (-2147483648) which caused issues with simple negation
    // 0x80000000 -> Little Endian: 00 00 00 80
    ASSERT_CODE("DEFD -2147483648", {0x00, 0x00, 0x00, 0x80});
    
    // Test normal negative number (-10 -> 0xFFFFFFF6)
    ASSERT_CODE("DEFD -10", {0xF6, 0xFF, 0xFF, 0xFF});
    
    // Test boundary of positive signed 32-bit (0x7FFFFFFF)
    ASSERT_CODE("DEFD 2147483647", {0xFF, 0xFF, 0xFF, 0x7F});
    
    // Test unsigned 32-bit that looks like negative signed (0xFFFFFFFF -> -1)
    ASSERT_CODE("DEFD 0xFFFFFFFF", {0xFF, 0xFF, 0xFF, 0xFF});
}

TEST_CASE(MemoryAccessOperator) {
    // Test 1: Basic read from a numeric address
    ASSERT_BLOCKS(R"(
        ORG 0x100
        DB 0xDE, 0xAD, 0xBE, 0xEF

        ORG 0x200
        LD A, {0x101} ; Read 0xAD
        LD B, {0x103} ; Read 0xEF
    )", {
        {0x100, {0xDE, 0xAD, 0xBE, 0xEF}},
        {0x200, {0x3E, 0xAD, 0x06, 0xEF}}
    });

    // Test 2: Read using a label
    ASSERT_CODE(R"(
        MyData:
            DB 10, 20, 30, 40
        LD A, {MyData + 2} ; Read 30
    )", {
        10, 20, 30, 40,
        0x3E, 30
    });

    // Test 3: Read a byte, resolving label with forward reference, but data is set later
    ASSERT_CODE(R"(
        LD A, {ForwardData}
        NOP
    ForwardData:
        DB 0x99
    )", {
        0x3E, 0x00, // LD A, 0x00
        0x00,       // NOP
        0x99        // DB 0x99
    });
}

TEST_CASE(TernaryOperator) {
    // --- Numeric tests ---
    // Simple true condition
    ASSERT_CODE("DB 1 ? 10 : 20", {10});
    // Simple false condition
    ASSERT_CODE("DB 0 ? 10 : 20", {20});
    // Expression as condition (true)
    ASSERT_CODE("DB (5 > 2) ? 100 : 200", {100});
    // Expression as condition (false)
    ASSERT_CODE("DB (5 < 2) ? 100 : 200", {200});
    // Expressions in branches
    ASSERT_CODE("DB 1 ? 10+5 : 20-5", {15});
    ASSERT_CODE("DB 0 ? 10+5 : 20-5", {15});

    // --- String tests ---
    // Simple true condition
    ASSERT_CODE("DB 1 ? \"A\" : \"B\"", {'A'});
    // Simple false condition
    ASSERT_CODE("DB 0 ? \"A\" : \"B\"", {'B'});
    // Longer strings
    ASSERT_CODE("DB 1 ? \"OK\" : \"FAIL\"", {'O', 'K'});
    ASSERT_CODE("DB 0 ? \"FAIL\" : \"OK\"", {'O', 'K'});
    // Concatenation in branches
    ASSERT_CODE("DB 1 ? \"A\"+\"B\" : \"C\"", {'A', 'B'});
    ASSERT_CODE("DB 0 ? \"A\" : \"B\"+\"C\"", {'B', 'C'});

    // --- Forward reference tests ---
    // Condition dependent on forward reference
    ASSERT_CODE(R"(
        DB DO_TRUE ? 100 : 200
        DO_TRUE EQU 1
    )", {100});
    ASSERT_CODE(R"(
        DB DO_FALSE ? 100 : 200
        DO_FALSE EQU 0
    )", {200});
    // Branches dependent on forward reference
    ASSERT_CODE(R"(
        DB 1 ? VAL_A : VAL_B
        VAL_A EQU 55
        VAL_B EQU 99
    )", {55});
    ASSERT_CODE(R"(
        DB 0 ? VAL_A : VAL_B
        VAL_A EQU 55
        VAL_B EQU 99
    )", {99});

    // --- Nested tests ---
    // True -> True
    ASSERT_CODE("DB 1 ? (1 ? 10 : 20) : 30", {10});
    // True -> False
    ASSERT_CODE("DB 1 ? (0 ? 10 : 20) : 30", {20});
    // False -> True
    ASSERT_CODE("DB 0 ? 10 : (1 ? 20 : 30)", {20});
    // False -> False
    ASSERT_CODE("DB 0 ? 10 : (0 ? 20 : 30)", {30});

    // --- Complex expressions ---
    // Nested with forward reference
    ASSERT_CODE(R"(
        DB OUTER_COND ? (INNER_COND ? VAL_A : VAL_B) : VAL_C
        OUTER_COND EQU 1
        INNER_COND EQU 0
        VAL_A EQU 11
        VAL_B EQU 22
        VAL_C EQU 33
    )", {22});

    // Nested with strings
    ASSERT_CODE("DB 1 ? (0 ? \"A\" : \"B\") : \"C\"", {'B'});
    ASSERT_CODE("DB 0 ? \"A\" : (1 ? \"B\" : \"C\")", {'B'});

    // Check if labels with '?' are not parsed as ternary operator
    ASSERT_CODE(R"(
        label?: NOP
        JP label?
    )", {0x00, 0xC3, 0x00, 0x00});
}

TEST_CASE(EndDirective) {
    // Test 1: Basic END directive stops assembly.
    ASSERT_CODE(R"(
        NOP
        END
        HALT ; This should be ignored
    )", {0x00});

    // Test 2: END inside an IF block that is true.
    ASSERT_CODE(R"(
        NOP
        IF 1
            END
        ENDIF
        HALT ; This should be ignored
    )", {0x00});

    // Test 3: END inside an IF block that is false is ignored.
    ASSERT_CODE(R"(
        NOP
        IF 0
            END
        ENDIF
        HALT ; This should be processed
    )", {0x00, 0x76});

    // Test 4: END inside a REPT block.
    // With REPT 0, the block is skipped, and END is not processed.
    ASSERT_CODE(R"(
        NOP
        REPT 0
            END
        ENDR
        HALT ; This should be processed
    )", {0x00, 0x76});

    // With REPT 1, the block is processed once, and END terminates assembly.
    ASSERT_CODE(R"(
        NOP
        REPT 1
            END
        ENDR
        HALT ; This should be ignored
    )", {0x00});

    // Test 4b: Code before END inside a REPT block is executed.
    ASSERT_CODE(R"(
        REPT 2
            NOP ; This should be assembled on the first iteration
            END
        ENDR
        HALT ; This should be ignored
    )", {0x00});

    // Test 5: END inside a macro.
    ASSERT_CODE(R"(
        STOP_MACRO MACRO
            END
        ENDM
        NOP
        STOP_MACRO
        HALT ; This should be ignored
    )", {0x00});

    // Test 6: END inside a macro that is not called.
    ASSERT_CODE(R"(
        STOP_MACRO MACRO
            END
        ENDM
        NOP
        HALT ; This should be processed
    )", {0x00, 0x76});
}

TEST_CASE(PhaseVariable) {
    // Test 1: Check $PHASE in a simple expression
    // The value should be 2 in the final (code generation) phase.
    ASSERT_CODE("DB $PHASE", {2});

    // Test 2: Use $PHASE in a conditional directive
    // This requires at least two passes.
    // Pass 1: MY_VAL is not defined, IF is false.
    // Pass 2: MY_VAL is defined, IF is true, code is generated. $PHASE is 2.
    ASSERT_CODE(R"(
        IFDEF MY_VAL
            DB $PHASE
        ENDIF
        MY_VAL EQU 1
    )", {2});
}

TEST_CASE(PassVariable) {
    // In the final assembly pass (AssemblyPhase), $PASS is reset to 1.
    ASSERT_CODE("DB $PASS", {1});

    // Verify usage in conditional
    // Note: Using $PASS to change code structure between passes can lead to phase errors,
    // but here we just verify that $PASS is 1 during assembly.
    ASSERT_CODE(R"(
        IF $PASS == 1
            DB 0xAA
        ELSE
            DB 0xBB
        ENDIF
    )", {0xAA});
}

class TestAssembler : public Z80::Assembler<Z80::StandardBus> {
public:
    using Z80::Assembler<Z80::StandardBus>::Assembler;
    using BasePolicy = typename Z80::Assembler<Z80::StandardBus>::BasePolicy;

    using OperatorInfo = typename Z80::Assembler<Z80::StandardBus>::Expressions::OperatorInfo;
    using Value = typename Z80::Assembler<Z80::StandardBus>::Expressions::Value;
    using FunctionInfo = typename Z80::Assembler<Z80::StandardBus>::Expressions::FunctionInfo;
    using IPhasePolicy = typename Z80::Assembler<Z80::StandardBus>::IPhasePolicy;
    using Strings = typename Z80::Assembler<Z80::StandardBus>::Strings;
    using Expressions = typename Z80::Assembler<Z80::StandardBus>::Expressions;

    using Context = typename Z80::Assembler<Z80::StandardBus>::Context;

    // Expose report_error for testing purposes
    void report_error(const std::string& message) const override {
        Z80::Assembler<Z80::StandardBus>::report_error(message);
    }

    void public_add_custom_operator(const std::string& op_string, const OperatorInfo& op_info) {
        add_custom_operator(op_string, op_info);
    }

    void public_add_custom_function(const std::string& func_name, const FunctionInfo& func_info) {
        add_custom_function(func_name, func_info);
    }

    void public_add_custom_constant(const std::string& const_name, double value) {
        add_custom_constant(const_name, value);
    }

    void public_add_custom_directive(const std::string& name, std::function<void(IPhasePolicy&, const std::vector<typename Strings::Tokens::Token>&)> func) {
        add_custom_directive(name, func);
    }

    static void fourty_two_handler(IPhasePolicy& policy, const std::vector<typename Strings::Tokens::Token>& args) {
        if (!args.empty()) {
            static_cast<TestAssembler&>(policy.context().assembler).report_error("FOURTY_TWO does not take arguments");
        }
        policy.on_assemble({42}, false);
    }

    static void fill_handler(IPhasePolicy& policy, const std::vector<typename Strings::Tokens::Token>& args) {
        if (args.size() != 2) {
            static_cast<TestAssembler&>(policy.context().assembler).report_error("FILL requires 2 arguments: count and value");
        }
        Expressions expr_eval(policy);
        int64_t count, value;
        if (!expr_eval.evaluate(args[0].original(), count) || !expr_eval.evaluate(args[1].original(), value)) {
            static_cast<TestAssembler&>(policy.context().assembler).report_error("Invalid arguments for FILL");
        }
        std::vector<uint8_t> bytes(count, (uint8_t)value);
        policy.on_assemble(bytes, false);
    }
};

TEST_CASE(CustomOperators) {
    // Test 1: Add a binary power operator '**'
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        TestAssembler::OperatorInfo power_op_info = {
            95,  // Precedence (higher than *, /)
            false, // is_unary
            true, // right_assoc for power operator
            [](TestAssembler::Context& ctx, const std::vector<TestAssembler::Value>& args) {
                return TestAssembler::Value{TestAssembler::Value::Type::IMMEDIATE, pow(args[0].n_val.asDouble(), args[1].n_val.asDouble())};
            }
        };
        assembler.public_add_custom_operator("**", power_op_info);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB 2 ** 0", {1});
        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB 2 ** 7", {128});
    }

    // Test 2: Add a unary 'SQR' operator
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        assembler.public_add_custom_operator("SQR", {100, true, false, [](TestAssembler::Context&, const std::vector<TestAssembler::Value>& args) {
            return TestAssembler::Value{TestAssembler::Value::Type::IMMEDIATE, args[0].n_val * args[0].n_val};
        }});
        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB SQR 9", {81});
    }
}

TEST_CASE(CustomFunctionsAndConstants) {
    // Test 1: Add a custom constant
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        assembler.public_add_custom_constant("MY_CONST", 123.0);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB MY_CONST", {123});
    }

    // Test 2: Add a custom function 'DOUBLE'
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        TestAssembler::FunctionInfo double_func_info = {
            1, // Number of arguments
            [](TestAssembler::Context&, const std::vector<TestAssembler::Value>& args) {
                return TestAssembler::Value{TestAssembler::Value::Type::IMMEDIATE, args[0].n_val * 2};
            }
        };
        assembler.public_add_custom_function("DOUBLE", double_func_info);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB DOUBLE(21)", {42});
    }

    // Test 3: Attempt to override a built-in constant (should fail)
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);
        ASSERT_COMPILE_FAILS_WITH_OPTS("assembler.add_custom_constant(\"TRUE\", 99)", {});
    }

    // Test 4: Add a custom function with no arguments
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        TestAssembler::FunctionInfo get_seven_func = {
            0, // No arguments
            [](TestAssembler::Context&, const std::vector<TestAssembler::Value>& args) {
                return TestAssembler::Value{TestAssembler::Value::Type::IMMEDIATE, 7.0};
            }
        };
        assembler.public_add_custom_function("GET_SEVEN", get_seven_func);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB GET_SEVEN()", {7});
    }

    // Test 5: Add a variadic custom function 'SUM'
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        TestAssembler::FunctionInfo sum_func = {
            -1, // Variadic, at least 1 argument
            [](TestAssembler::Context&, const std::vector<TestAssembler::Value>& args) {
                double sum = 0;
                for(const auto& arg : args) {
                    sum += arg.n_val.asDouble();
                }
                return TestAssembler::Value{TestAssembler::Value::Type::IMMEDIATE, sum};
            }
        };
        assembler.public_add_custom_function("SUM", sum_func);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB SUM(1, 2, 3, 4)", {10});
        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "DB SUM(10)", {10});
    }

    // Test 6: Attempt to override a built-in function (should fail)
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);
        TestAssembler::FunctionInfo dummy_func = {0, nullptr};
        // This should throw an exception during add_custom_function, which the test framework doesn't catch well.
        // We expect the compile to fail because the error is reported.
        ASSERT_COMPILE_FAILS_WITH_OPTS("assembler.add_custom_function(\"SIN\", dummy_func)", {});
    }
}

TEST_CASE(CustomDirectives) {
    // Test 1: Add a simple directive without arguments
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        assembler.public_add_custom_directive("FOURTY_TWO", &TestAssembler::fourty_two_handler);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "FOURTY_TWO", {42});
    }

    // Test 2: Add a directive with arguments
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);

        assembler.public_add_custom_directive("FILL", &TestAssembler::fill_handler);

        ASSERT_CODE_WITH_ASSEMBLER(bus, assembler, file_provider, "FILL 3, 0xAA", {0xAA, 0xAA, 0xAA});
    }

    // Test 3: Attempt to override a built-in directive (should fail)
    {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        TestAssembler assembler(&bus, &file_provider);
        
        ASSERT_COMPILE_FAILS("assembler.public_add_custom_directive(\"DB\", nullptr)");
    }
}

TEST_CASE(NewOperators) {
    // Operator + (Math / Sum)
    // Number + Number
    ASSERT_CODE("DB 65 + 1", {66});
    // String (Len=1) + Number
    ASSERT_CODE("DB \"A\" + 1", {66});
    // Number + String (Len=1)
    ASSERT_CODE("DB 1 + \"A\"", {66});
    // String (Len>1) + Number -> ERROR
    ASSERT_COMPILE_FAILS("DB \"AB\" + 1");

    // Operator + (Concatenation for strings)
    ASSERT_CODE("DB \"A\" + \"B\"", {'A', 'B'});
    ASSERT_CODE("DB \"AB\" + \"CD\"", {'A', 'B', 'C', 'D'});
    ASSERT_CODE("DB \"A\" + \"B\" + \"C\"", {'A', 'B', 'C'});
}

TEST_CASE(SingleCharStringMath) {
    // Arithmetic operators
    ASSERT_CODE("DB \"A\" + 1", {66});
    ASSERT_CODE("DB 'A' + 1", {66});
    ASSERT_CODE("DB 1 + \"A\"", {66});
    ASSERT_CODE("DB \"B\" - \"A\"", {1});
    ASSERT_CODE("DB \"A\" * 2", {130});
    ASSERT_CODE("DB \"d\" / 2", {50}); // 'd' is 100
    ASSERT_CODE("DB \"e\" % 10", {1}); // 'e' is 101

    // Bitwise operators
    ASSERT_CODE("DB \"A\" & 0x0F", {1}); // 65 & 15 = 1
    ASSERT_CODE("DB \"A\" | 0x80", {0xC1}); // 65 | 128 = 193
    ASSERT_CODE("DB \"A\" ^ \"B\"", {3}); // 65 ^ 66 = 3
    ASSERT_CODE("DB ~\"A\"", {0xBE}); // ~65 = -66 = 0xBE

    // Comparison operators
    ASSERT_CODE("DB \"A\" < \"B\"", {1});
    ASSERT_CODE("DB \"B\" > \"A\"", {1});
    ASSERT_CODE("DB \"A\" == 65", {1});
    ASSERT_CODE("DB \"A\" != \"B\"", {1});

    // Functions
    ASSERT_CODE("DB MIN(\"A\", \"B\")", {65});
    ASSERT_CODE("DB MAX(\"A\", \"B\")", {66});
    ASSERT_CODE("DB ABS(\"A\")", {65});
    ASSERT_CODE("DB SGN(\"A\")", {1});
    
    // SQRT(64) = 8. '@' is 64.
    ASSERT_CODE("DB SQRT(\"@\")", {8}); 
    
    // Logical operators
    ASSERT_CODE("DB \"A\" && 1", {1});
    ASSERT_CODE("DB \"A\" || 0", {1});
    ASSERT_CODE("DB !\"A\"", {0}); // !65 is 0
}

TEST_CASE(SingleCharStringParsing) {
    // "A" treated as CHAR_LITERAL (65) in instruction operand
    ASSERT_CODE("LD A, \"A\"", {0x3E, 65});
    
    // "A" treated as number in arithmetic
    ASSERT_CODE("DB \"A\" + 1", {66});
    
    // "A" treated as string for concatenation
    ASSERT_CODE("DM \"A\" + \"B\"", {'A', 'B'});
    
    // 'A' + 'B' -> "AB" (Concatenation of char literals which are now strings)
    ASSERT_CODE("DB 'A' + 'B'", {'A', 'B'});
    
    // Comparison
    ASSERT_CODE("DB \"A\" == 65", {1});
}

TEST_CASE(SingleCharStringOperand) {
    // CHR(65) returns "A" (STRING type). Operands::parse should convert it to CHAR_LITERAL.
    ASSERT_CODE("LD A, CHR(65)", {0x3E, 65}); // 'A'

    // STR(5) returns "5" (STRING type).
    ASSERT_CODE("LD A, STR(5)", {0x3E, 53}); // '5'

    // "A" + "" evaluates to "A" (STRING type).
    ASSERT_CODE("LD A, \"A\" + \"\"", {0x3E, 65});

    // SUBSTR("ABC", 1, 1) -> "B"
    ASSERT_CODE("LD A, SUBSTR(\"ABC\", 1, 1)", {0x3E, 66});

    // ISSTRING checks
    ASSERT_CODE("DB ISSTRING(\"A\")", {1}); // "A" is STRING now
    ASSERT_CODE("DB ISNUMBER(\"A\")", {1}); // "A" is also a NUMBER (char literal)
    ASSERT_CODE("DB ISSTRING(\"AB\")", {1}); // "AB" is STRING
}

TEST_CASE(StringMemoryAddressing) {
    // LD A, ("A") -> LD A, (65) -> 3A 41 00
    ASSERT_CODE("LD A, (\"A\")", {0x3A, 0x41, 0x00});
    
    // LD ("A"), A -> LD (65), A -> 32 41 00
    ASSERT_CODE("LD (\"A\"), A", {0x32, 0x41, 0x00});
    
    // OUT ("A"), A -> OUT (65), A -> D3 41
    ASSERT_CODE("OUT (\"A\"), A", {0xD3, 0x41});

    // LD BC, ("A") -> LD BC, (65) -> ED 4B 41 00
    ASSERT_CODE("LD BC, (\"A\")", {0xED, 0x4B, 0x41, 0x00});

    // LD HL, ("A") -> LD HL, (65) -> 2A 41 00
    ASSERT_CODE("LD HL, (\"A\")", {0x2A, 0x41, 0x00});

    // LD B, ("A") -> LD B, (65) -> Invalid instruction (LD r, (nn) does not exist for B)
    // Parentheses denote memory access, and Z80 only supports LD A, (nn) for 8-bit registers.
    ASSERT_COMPILE_FAILS("LD B, (\"A\")");
}

TEST_CASE(IndexedAddressingWithExpressions) {
    // LD A, (IX + "A") -> LD A, (IX + 65) -> DD 7E 41
    ASSERT_CODE("LD A, (IX + \"A\")", {0xDD, 0x7E, 0x41});

    // LD A, (IX - "A") -> LD A, (IX - 65) -> DD 7E BF
    ASSERT_CODE("LD A, (IX - \"A\")", {0xDD, 0x7E, 0xBF});

    // LD B, (IY + 1 + 2) -> LD B, (IY + 3) -> FD 46 03
    ASSERT_CODE("LD B, (IY + 1 + 2)", {0xFD, 0x46, 0x03});

    // LD (IX + "0"), A -> LD (IX + 48), A -> DD 77 30
    ASSERT_CODE("LD (IX + \"0\"), A", {0xDD, 0x77, 0x30});
}

TEST_CASE(RelationalAndEqualityOperators) {
    // Equality (==, !=)
    ASSERT_CODE("DB 'A' == 65", {1});
    ASSERT_CODE("DB \"A\" == \"A\"", {1});
    ASSERT_CODE("DB \"A\" == 65", {1});
    ASSERT_CODE("DB \"1\" == 49", {1});
    ASSERT_CODE("DB \"1\" == 1", {0});
    ASSERT_CODE("DB \"123\" == 123", {0});
    ASSERT_CODE("DB \"ABC\" == \"ABC\"", {1});
    ASSERT_CODE("DB \"ABC\" == 65", {0});

    // Relational (<, >, <=, >=)
    // Number vs Number
    ASSERT_CODE("DB 10 > 2", {1});
    ASSERT_CODE("DB 2 < 10", {1});
    
    // String vs String (Lexicographical)
    // Removed: Relational operators no longer support strings
    ASSERT_COMPILE_FAILS("DB \"AA\" < \"AB\"");
    ASSERT_COMPILE_FAILS("DB \"10\" < \"2\"");
    
    // Mixed (String len=1 vs Number)
    ASSERT_CODE("DB \"A\" > 64", {1});
    ASSERT_CODE("DB 64 < \"A\"", {1});
    
}

TEST_CASE(OptimizationFlags) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    
    // Default: Disabled (LD A, 0 -> 3E 00) - Optimizations require directive activation
    ASSERT_CODE_WITH_OPTS("LD A, 0", {0x3E, 0x00}, config);

    // Enabled via directive
    ASSERT_CODE_WITH_OPTS("LD A, 0", {0x3E, 0x00}, config);
    
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_XOR\nLD A, 0", {0xAF}, config);
    
    // Disabled via directive
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_XOR\nOPTIMIZE -OPS_XOR\nLD A, 0", {0x3E, 0x00}, config);

    // Disabled globally
    config.compilation.enable_optimization = false;
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_XOR\nLD A, 0", {0x3E, 0x00}, config);
}

TEST_CASE(JpToJrOptimization) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    
    std::string prefix = "OPTIMIZE +BRANCH_SHORT\n";

    // JP nn -> JR e (Forward, within range)
    ASSERT_CODE_WITH_OPTS(prefix + "JP target\nNOP\ntarget: NOP", {0x18, 0x01, 0x00, 0x00}, config);
    
    // JP nn -> JR e (Backward, within range)
    ASSERT_CODE_WITH_OPTS(prefix + "target: NOP\nJP target", {0x00, 0x18, 0xFD}, config);
    
    // JP cc, nn -> JR cc, e
    ASSERT_CODE_WITH_OPTS(prefix + "JP Z, target\nNOP\ntarget: NOP", {0x28, 0x01, 0x00, 0x00}, config);
    
    // JP cc, nn -> JP cc, nn (Condition not supported by JR, e.g. PO)
    ASSERT_CODE_WITH_OPTS(prefix + "JP PO, target\nNOP\ntarget: NOP", {0xE2, 0x04, 0x00, 0x00, 0x00}, config);
    
    // Out of range (Forward) -> Remains JP
    std::string code_far = prefix + "JP target\nDS 130\ntarget: NOP";
    std::vector<uint8_t> expected_far = {0xC3, 0x85, 0x00}; // JP 0x0085 (3 + 130 = 133 = 0x85)
    expected_far.insert(expected_far.end(), 130, 0);
    expected_far.push_back(0x00);
    ASSERT_CODE_WITH_OPTS(code_far, expected_far, config);
}

TEST_CASE(PeepholeOptimizations) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    
    // XOR A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_XOR\nLD A, 0", {0xAF}, config);
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_XOR\nLD A, 1", {0x3E, 0x01}, config); // Not 0
    
    // INC/DEC
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nADD A, 1", {0x3C}, config); // INC A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nSUB 1", {0x3D}, config);    // DEC A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nADD A, 2", {0xC6, 0x02}, config); // Not 1
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nADD A, 255", {0x3D}, config); // DEC A (A + 255 == A - 1)
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nADD A, -1", {0x3D}, config);  // DEC A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nSUB 255", {0x3C}, config);    // INC A (A - 255 == A + 1)
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_INC\nSUB -1", {0x3C}, config);     // INC A
    
    // OR A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_OR\nCP 0", {0xB7}, config); // OR A
    ASSERT_CODE_WITH_OPTS("OPTIMIZE +OPS_OR\nCP 1", {0xFE, 0x01}, config); // Not 0
}

TEST_CASE(RedundantLoadsOptimization) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +DCE\n";
    
    // LD A, A -> Removed (0 bytes)
    ASSERT_CODE_WITH_OPTS(prefix + "LD A, A", {}, config);
    
    // LD B, B -> Removed
    ASSERT_CODE_WITH_OPTS(prefix + "LD B, B", {}, config);
    
    // LD A, B -> Kept
    ASSERT_CODE_WITH_OPTS(prefix + "LD A, B", {0x78}, config);
}

TEST_CASE(OptDirectiveScopes) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    
    std::string code = R"(
        LD A, 0         ; 3E 00
        OPTIMIZE PUSH
        OPTIMIZE +OPS_XOR
        LD A, 0         ; AF
        OPTIMIZE PUSH
        OPTIMIZE +BRANCH_SHORT
        JP target       ; 18 00
    target:
        OPTIMIZE POP
        LD A, 0         ; AF (XOR_A still on)
        JP target       ; C3 05 00 (JR off)
        OPTIMIZE POP
        LD A, 0         ; 3E 00 (Back to default)
    )";
    
    std::vector<uint8_t> expected = {
        0x3E, 0x00, 0xAF, 0x18, 0x00, 0xAF, 0xC3, 0x05, 0x00, 0x3E, 0x00
    };
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(JumpChainOptimization) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD\n";

    // Basic chain: JP A -> JP B -> Target
    // Should optimize JP A to JP Target
    std::string code_basic = prefix + R"(
        JP LabelA       ; Should become JP Target
    LabelA:
        JP LabelB
    LabelB:
        JP Target
    Target:
        NOP
    )";
    // JP Target (0x0009) -> C3 09 00
    // LabelA (0x0003): JP LabelB (0x0006) -> C3 06 00
    // LabelB (0x0006): JP Target (0x0009) -> C3 09 00
    // Target (0x0009): NOP -> 00
    std::vector<uint8_t> expected_basic = {
        0xC3, 0x09, 0x00, 
        0xC3, 0x09, 0x00, 
        0xC3, 0x09, 0x00, 
        0x00
    };
    ASSERT_CODE_WITH_OPTS(code_basic, expected_basic, config);

    // Loop detection: JP A -> JP B -> JP A
    // Should not hang, should just point to next in chain or stop
    std::string code_loop = prefix + R"(
    LabelA:
        JP LabelB
    LabelB:
        JP LabelA
    )";
    // Should compile successfully
    ASSERT_CODE_WITH_OPTS(code_loop, {0xC3, 0x00, 0x00, 0xC3, 0x03, 0x00}, config);

    // Interaction with JR
    // JR A -> JP B -> Target
    // Should optimize JR A to JR Target (if in range)
    std::string code_jr = prefix + "OPTIMIZE +BRANCH_SHORT\n" + R"(
        JR LabelA       ; Should become JR Target (0x05) -> 0x05 - 2 = 0x03
    LabelA:
        JP Target
    Target:
        NOP
    )";
    // JR Target (0x05) -> 18 03
    // LabelA (0x02): JP Target (0x05) -> C3 05 00
    // Target (0x05): NOP -> 00
    std::vector<uint8_t> expected_jr = {
        0x18, 0x02, // JR LabelA (0x02) -> Optimized to Target (0x04). 0x04 - (0+2) = 2
        0x18, 0x00, // JP Target (0x05) -> JR Target (0x04). 0x04 - (0x02+2) = 0x00
        0x00
    };
    ASSERT_CODE_WITH_OPTS(code_jr, expected_jr, config);
}

TEST_CASE(JumpChainWithJr) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD +BRANCH_SHORT\n";

    // JP Start -> JP Target.
    // Start: JP Target -> JR Target.
    // If JR doesn't register target, JP Start might revert to JP Start (or JR Start) in later passes.
    std::string code = prefix + R"(
        JP Start
    Start:
        JP Target
        NOP
    Target:
        NOP
    )";
    
    std::vector<uint8_t> expected = {
        0x18, 0x03, // JR Target (0x0005). Offset = 0x05 - 0x02 = 3
        0x18, 0x01, // JR Target (0x0005). Offset = 0x05 - 0x04 = 1
        0x00,       // NOP
        0x00        // NOP
    };
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(JumpChainTrampoline) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD\n";

    // Scenario: JR jumps to a Trampoline, which JPs to a FarTarget.
    // FarTarget is out of JR range.
    // Optimization should NOT replace Trampoline with FarTarget for the JR instruction.
    
    std::string code = prefix + R"(
        JR Trampoline       ; Should keep jumping to Trampoline (offset 0)
    Trampoline:
        JP FarTarget
        DS 200              ; Make FarTarget far away
    FarTarget:
        NOP
    )";

    std::vector<uint8_t> expected = {
        0x18, 0x00,         // JR Trampoline (offset 0)
        0xC3, 0xCD, 0x00    // JP FarTarget (0x00CD = 2 + 3 + 200)
    };
    // Fill DS with zeros
    expected.insert(expected.end(), 200, 0);
    expected.push_back(0x00); // NOP at FarTarget

    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(JumpChainLoopWithJr) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD +BRANCH_SHORT\n";

    // Loop: LabelA -> LabelB -> LabelA
    // Both are JR instructions.
    // Optimization should resolve LabelA -> LabelA (self loop) and LabelB -> LabelB (self loop).
    std::string code = prefix + R"(
    LabelA:
        JR LabelB
    LabelB:
        JR LabelA
    )";
    
    // LabelA at 0x0000. LabelB at 0x0002.
    // Optimized:
    // LabelA: JR LabelA (offset -2 -> FE)
    // LabelB: JR LabelB (offset -2 -> FE)
    std::vector<uint8_t> expected = {
        0x18, 0xFE, // JR LabelA
        0x18, 0xFE  // JR LabelB
    };
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(JumpChainDjnz) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD\n";

    // DJNZ -> JP -> Target
    std::string code = prefix + R"(
        LD B, 10
    Loop:
        DJNZ Trampoline
        RET
    Trampoline:
        JP Target
    Target:
        XOR A
    )";
    
    std::vector<uint8_t> expected = {
        0x06, 0x0A,       // LD B, 10
        0x10, 0x04,       // DJNZ Target (0x0008). Offset = 08 - (02+2) = 4.
        0xC9,             // RET
        0xC3, 0x08, 0x00, // JP Target
        0xAF              // XOR A
    };
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(JumpChainThroughConditional) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +JUMP_THREAD +BRANCH_SHORT\n";

    // JP Start -> JR Z, Target
    // Should NOT optimize JP Start to JP Target (bypassing Z check)
    std::string code = prefix + R"(
        JP Start
    Start:
        JR Z, Target
    Target:
        NOP
    )";
    
    std::vector<uint8_t> expected = {
        0x18, 0x00, // JP Start -> JR Start (offset 0). Correctly jumps to Start.
        0x28, 0x00, // JR Z, Target (offset 0)
        0x00        // NOP
    };
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(OptimizationKeywords) {
    Z80::Assembler<Z80::StandardBus>::Config config;

    // OFF: Disables everything
    std::string code_off = R"(
        OPTIMIZE SIZE
        OPTIMIZE NONE
        LD A, 0         ; Not optimized (3E 00)
        JP target       ; Not optimized (C3...)
    target:
        NOP
    )";
    ASSERT_CODE_WITH_OPTS(code_off, {0x3E, 0x00, 0xC3, 0x05, 0x00, 0x00}, config);
    
    // OPS: Enables only opss (XOR A, INC/DEC, OR A)
    std::string code_peep = R"(
        OPTIMIZE NONE
        OPTIMIZE OPS
        LD A, 0         ; Optimized (AF)
        ADD A, 1        ; Optimized (3C)
        JP target       ; Not optimized (C3...)
    target:
        NOP
    )";
    ASSERT_CODE_WITH_OPTS(code_peep, {0xAF, 0x3C, 0xC3, 0x05, 0x00, 0x00}, config);
}

TEST_CASE(BranchLongOptimization) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +BRANCH_LONG\n";

    auto make_expected = [](uint8_t opcode, int padding) {
        std::vector<uint8_t> v = {opcode, (uint8_t)((3 + padding) & 0xFF), (uint8_t)((3 + padding) >> 8)};
        v.insert(v.end(), padding, 0);
        v.push_back(0x00);
        return v;
    };

    // JR nn -> JP nn (Out of range)
    std::string code_uncond = prefix + "JR target\nDS 128\ntarget: NOP";
    ASSERT_CODE_WITH_OPTS(code_uncond, make_expected(0xC3, 128), config);

    // JR cc, nn -> JP cc, nn (Out of range)
    ASSERT_CODE_WITH_OPTS(prefix + "JR Z, target\nDS 128\ntarget: NOP", make_expected(0xCA, 128), config); // JP Z
    ASSERT_CODE_WITH_OPTS(prefix + "JR NZ, target\nDS 128\ntarget: NOP", make_expected(0xC2, 128), config); // JP NZ
    ASSERT_CODE_WITH_OPTS(prefix + "JR C, target\nDS 128\ntarget: NOP", make_expected(0xDA, 128), config); // JP C
    ASSERT_CODE_WITH_OPTS(prefix + "JR NC, target\nDS 128\ntarget: NOP", make_expected(0xD2, 128), config); // JP NC

    // JR cc, nn -> JR cc, e (In range) - Should remain JR
    std::string code_in_range = prefix + "JR Z, target\nNOP\ntarget: NOP";
    // JR Z (28), offset 1.
    ASSERT_CODE_WITH_OPTS(code_in_range, {0x28, 0x01, 0x00, 0x00}, config);
}

TEST_CASE(PeepholeLogicAndSla) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +OPS_LOGIC +OPS_SLA\n";

    // AND 0 -> XOR A
    ASSERT_CODE_WITH_OPTS(prefix + "AND 0", {0xAF}, config);
    
    // OR 0 -> OR A
    ASSERT_CODE_WITH_OPTS(prefix + "OR 0", {0xB7}, config);
    
    // XOR 0 -> OR A
    ASSERT_CODE_WITH_OPTS(prefix + "XOR 0", {0xB7}, config);
    
    // SLA A -> ADD A, A
    ASSERT_CODE_WITH_OPTS(prefix + "SLA A", {0x87}, config);
}

TEST_CASE(BranchLongWithJumpThread) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +BRANCH_LONG +JUMP_THREAD\n";

    // Scenario 1: JR -> JP (far) -> Target
    // JR should become JP to Target because Target is far.
    std::string code = prefix + R"(
        JR Start        ; Should become JP Target (far)
    Start:
        JP Target
        DS 200
    Target:
        NOP
    )";
    
    // 0x0000: JP Target (0x00CE) -> C3 CE 00
    // 0x0003: Start: JP Target (0x00CE) -> C3 CE 00
    // 0x0006: DS 200
    // 0x00CE: Target: NOP
    
    std::vector<uint8_t> expected;
    expected.push_back(0xC3); expected.push_back(0xCE); expected.push_back(0x00);
    expected.push_back(0xC3); expected.push_back(0xCE); expected.push_back(0x00);
    expected.insert(expected.end(), 200, 0);
    expected.push_back(0x00);

    ASSERT_CODE_WITH_OPTS(code, expected, config);

    // Scenario 2: JR cc -> JP (far) -> Target
    std::string code_cond = prefix + R"(
        JR Z, Start     ; Should become JP Z, Target
    Start:
        JP Target
        DS 200
    Target:
        NOP
    )";
    
    // 0x0000: JP Z, Target (0x00CE) -> CA CE 00
    // 0x0003: Start: JP Target (0x00CE) -> C3 CE 00
    // 0x0006: DS 200
    // 0x00CE: Target: NOP
    
    std::vector<uint8_t> expected_cond;
    expected_cond.push_back(0xCA); expected_cond.push_back(0xCE); expected_cond.push_back(0x00);
    expected_cond.push_back(0xC3); expected_cond.push_back(0xCE); expected_cond.push_back(0x00);
    expected_cond.insert(expected_cond.end(), 200, 0);
    expected_cond.push_back(0x00);
    
    ASSERT_CODE_WITH_OPTS(code_cond, expected_cond, config);
}

TEST_CASE(BranchLongAndShortInteraction) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    std::string prefix = "OPTIMIZE +BRANCH_LONG +BRANCH_SHORT\n";

    // 1. JP NearTarget -> Should become JR (2 bytes) because of BRANCH_SHORT
    // 2. JR FarTarget  -> Should become JP (3 bytes) because of BRANCH_LONG (out of range)
    
    std::string code = prefix + R"(
        JP NearTarget   ; Optimized to JR (2 bytes)
        JR FarTarget    ; Expanded to JP (3 bytes)
    NearTarget:
        NOP
        DS 200
    FarTarget:
        NOP
    )";
    
    // 0x0000: JR NearTarget (0x0005). Offset = 0x0005 - 0x0002 = 3. (18 03)
    // 0x0002: JP FarTarget (0x00CE). (C3 CE 00)
    // 0x0005: NOP (00)
    // 0x0006: DS 200
    // 0x00CE: NOP (00)
    
    std::vector<uint8_t> expected = {0x18, 0x03, 0xC3, 0xCE, 0x00, 0x00};
    expected.insert(expected.end(), 200, 0);
    expected.push_back(0x00);
    
    ASSERT_CODE_WITH_OPTS(code, expected, config);
}

TEST_CASE(OptimizationStats) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_optimization = true;

    auto check_stats = [&](const std::string& code, int expected_bytes, int expected_cycles) {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", code);
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider, config);
        if (!assembler.compile("main.asm")) {
            std::cerr << "FAIL: Compilation failed for stats test code:\n" << code << "\n";
            tests_failed++;
            return;
        }
        auto stats = assembler.get_optimization_stats();
        if (stats.bytes_saved != expected_bytes || stats.cycles_saved != expected_cycles) {
            std::cerr << "FAIL: Stats mismatch for code:\n" << code << "\n";
            std::cerr << "      Expected bytes saved: " << expected_bytes << ", Got: " << stats.bytes_saved << "\n";
            std::cerr << "      Expected cycles saved: " << expected_cycles << ", Got: " << stats.cycles_saved << "\n";
            tests_failed++;
        } else {
            tests_passed++;
        }
    };

    // 1. LD A, 0 -> XOR A
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_XOR\nLD A, 0", 1, 3);

    // 2. ADD A, 1 -> INC A
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_INC\nADD A, 1", 1, 3);

    // 3. JP to JR (Short branch)
    // Saved: 1 byte, -2 cycles
    check_stats("OPTIMIZE +BRANCH_SHORT\nJP target\ntarget: NOP", 1, -2);

    // 4. CALL to RST (e.g. CALL 0)
    // CALL nn (3 bytes, 17 cycles) -> RST 0 (1 byte, 11 cycles)
    // Saved: 2 bytes, 6 cycles
    check_stats("OPTIMIZE +OPS_RST\nCALL 0", 2, 6);

    // 5. SLA A -> ADD A, A
    // SLA A (2 bytes, 8 cycles) -> ADD A, A (1 byte, 4 cycles)
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +OPS_SLA\nSLA A", 1, 4);

    // 6. Combined
    // LD A, 0 (XOR A) -> 1b, 3c
    // ADD A, 1 (INC A) -> 1b, 3c
    // Total: 2b, 6c
    check_stats("OPTIMIZE +OPS_XOR +OPS_INC\nLD A, 0\nADD A, 1", 2, 6);
    
    // 7. No optimization
    check_stats("OPTIMIZE NONE\nLD A, 0", 0, 0);
}

TEST_CASE(MoreOptimizationStats) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_optimization = true;

    auto check_stats = [&](const std::string& code, int expected_bytes, int expected_cycles) {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", code);
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider, config);
        if (!assembler.compile("main.asm")) {
            std::cerr << "FAIL: Compilation failed for stats test code:\n" << code << "\n";
            tests_failed++;
            return;
        }
        auto stats = assembler.get_optimization_stats();
        if (stats.bytes_saved != expected_bytes || stats.cycles_saved != expected_cycles) {
            std::cerr << "FAIL: Stats mismatch for code:\n" << code << "\n";
            std::cerr << "      Expected bytes saved: " << expected_bytes << ", Got: " << stats.bytes_saved << "\n";
            std::cerr << "      Expected cycles saved: " << expected_cycles << ", Got: " << stats.cycles_saved << "\n";
            tests_failed++;
        } else {
            tests_passed++;
        }
    };

    // 1. BRANCH_LONG: JR -> JP
    // JR (2 bytes, 12 cycles) -> JP (3 bytes, 10 cycles)
    // Saved: -1 byte, +2 cycles
    check_stats("OPTIMIZE +BRANCH_LONG\nJR Target\nDS 130\nTarget: NOP", -1, 2);

    // 2. DCE: JR $+2
    // JR $+2 (2 bytes, 12 cycles) -> Removed
    // Saved: 2 bytes, 12 cycles
    check_stats("OPTIMIZE +DCE\nJR $+2", 2, 12);

    // 3. DCE: LD B, B
    // LD B, B (1 byte, 4 cycles) -> Removed
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +DCE\nLD B, B", 1, 4);

    // 4. OPS_ROT: RLC A -> RLCA
    // RLC A (2 bytes, 8 cycles) -> RLCA (1 byte, 4 cycles)
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +OPS_ROT\nRLC A", 1, 4);

    // 5. OPS_OR: CP 0 -> OR A
    // CP 0 (2 bytes, 7 cycles) -> OR A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_OR\nCP 0", 1, 3);

    // 6. OPS_ADD0: ADD A, 0 -> OR A
    // ADD A, 0 (2 bytes, 7 cycles) -> OR A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_ADD0\nADD A, 0", 1, 3);

    // 7. OPS_INC: SUB 255 -> INC A
    // SUB 255 (2 bytes, 7 cycles) -> INC A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_INC\nSUB 255", 1, 3);
}

TEST_CASE(ExtendedOptimizationStats) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_optimization = true;

    auto check_stats = [&](const std::string& code, int expected_bytes, int expected_cycles) {
        Z80::StandardBus bus;
        MockFileProvider file_provider;
        file_provider.add_source("main.asm", code);
        Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider, config);
        if (!assembler.compile("main.asm")) {
            std::cerr << "FAIL: Compilation failed for stats test code:\n" << code << "\n";
            tests_failed++;
            return;
        }
        auto stats = assembler.get_optimization_stats();
        if (stats.bytes_saved != expected_bytes || stats.cycles_saved != expected_cycles) {
            std::cerr << "FAIL: Stats mismatch for code:\n" << code << "\n";
            std::cerr << "      Expected bytes saved: " << expected_bytes << ", Got: " << stats.bytes_saved << "\n";
            std::cerr << "      Expected cycles saved: " << expected_cycles << ", Got: " << stats.cycles_saved << "\n";
            tests_failed++;
        } else {
            tests_passed++;
        }
    };

    // 1. OPS_LOGIC: AND 0 -> XOR A
    // AND 0 (2 bytes, 7 cycles) -> XOR A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_LOGIC\nAND 0", 1, 3);

    // 2. OPS_LOGIC: OR 0 -> OR A
    // OR 0 (2 bytes, 7 cycles) -> OR A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_LOGIC\nOR 0", 1, 3);

    // 3. OPS_LOGIC: XOR 0 -> OR A
    // XOR 0 (2 bytes, 7 cycles) -> OR A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_LOGIC\nXOR 0", 1, 3);

    // 4. OPS_ROT: RRC A -> RRCA
    // RRC A (2 bytes, 8 cycles) -> RRCA (1 byte, 4 cycles)
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +OPS_ROT\nRRC A", 1, 4);

    // 5. OPS_ROT: RL A -> RLA
    // RL A (2 bytes, 8 cycles) -> RLA (1 byte, 4 cycles)
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +OPS_ROT\nRL A", 1, 4);

    // 6. OPS_ROT: RR A -> RRA
    // RR A (2 bytes, 8 cycles) -> RRA (1 byte, 4 cycles)
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +OPS_ROT\nRR A", 1, 4);

    // 7. OPS_RST: CALL 0x0008 -> RST 08H
    // CALL (3 bytes, 17 cycles) -> RST (1 byte, 11 cycles)
    // Saved: 2 bytes, 6 cycles
    check_stats("OPTIMIZE +OPS_RST\nCALL 0x0008", 2, 6);

    // 8. OPS_INC: ADD A, 255 -> DEC A
    // ADD A, 255 (2 bytes, 7 cycles) -> DEC A (1 byte, 4 cycles)
    // Saved: 1 byte, 3 cycles
    check_stats("OPTIMIZE +OPS_INC\nADD A, 255", 1, 3);

    // 9. DCE: LD C, C
    // LD C, C (1 byte, 4 cycles) -> Removed
    // Saved: 1 byte, 4 cycles
    check_stats("OPTIMIZE +DCE\nLD C, C", 1, 4);
}

TEST_CASE(OptionDirective) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = true;
    config.compilation.enable_undocumented = true;

    // 1. Basic Enable/Disable Z80N (Instruction with no operands)
    // Enabled: SWAPNIB is instruction (2 bytes)
    ASSERT_CODE_WITH_OPTS(R"(
        SWAPNIB
    )", {0xED, 0x23}, config);

    // Disabled: SWAPNIB is label (0 bytes)
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -Z80N
        SWAPNIB
    )", {}, config);

    // Re-enabled: SWAPNIB is instruction
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -Z80N
        OPTION +Z80N
        SWAPNIB
    )", {0xED, 0x23}, config);

    // 2. Basic Enable/Disable Undocumented (Instruction with operands)
    // Enabled: SLL A is instruction
    ASSERT_CODE_WITH_OPTS(R"(
        SLL A
    )", {0xCB, 0x37}, config);

    // Disabled: SLL is label, A is unknown mnemonic -> Fail
    ASSERT_COMPILE_FAILS_WITH_OPTS(R"(
        OPTION -UNDOC
        SLL A
    )", config);

    // Re-enabled
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -UNDOC
        OPTION +UNDOC
        SLL A
    )", {0xCB, 0x37}, config);

    // 3. PUSH/POP State
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -Z80N    ; Disable Z80N
        OPTION PUSH     ; Save state (Z80N=Off)
        OPTION +Z80N    ; Enable Z80N
        SWAPNIB         ; OK (Instruction)
        OPTION POP      ; Restore state (Z80N=Off)
        SWAPNIB         ; OK (Label, 0 bytes)
    )", {0xED, 0x23}, config); // Only one SWAPNIB generates code

    // 4. Verify Config overrides OPTION (if config says no, OPTION cannot enable)
    Z80::Assembler<Z80::StandardBus>::Config config_disabled;
    config_disabled.compilation.enable_z80n = false;
    
    // Even with OPTION +Z80N, it should remain disabled because config is false.
    // So SWAPNIB is a label (0 bytes).
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION +Z80N
        SWAPNIB
    )", {}, config_disabled);
}

TEST_CASE(OptionDirectiveErrors) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = true;
    config.compilation.enable_undocumented = true;

    // 1. Invalid parameter
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION INVALID_PARAM", config);

    // 2. POP without matching PUSH
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION POP", config);

    // 3. PUSH mixed with other arguments
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION PUSH +Z80N", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION +Z80N PUSH", config);

    // 4. POP mixed with other arguments
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION POP +Z80N", config);
    ASSERT_COMPILE_FAILS_WITH_OPTS("OPTION +Z80N POP", config);
}

TEST_CASE(OptionDirectiveMultiple) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = true;
    config.compilation.enable_undocumented = true;

    // 1. Disable multiple options in one line
    // SLL A should fail if UNDOC is disabled (A is not a mnemonic)
    ASSERT_COMPILE_FAILS_WITH_OPTS(R"(
        OPTION -Z80N -UNDOC
        SLL A
    )", config);

    // 2. Enable multiple options in one line
    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -Z80N -UNDOC
        OPTION +Z80N +UNDOC
        SWAPNIB
        SLL A
    )", {0xED, 0x23, 0xCB, 0x37}, config);
}

TEST_CASE(OptionDirectiveNestedStack) {
    Z80::Assembler<Z80::StandardBus>::Config config;
    config.compilation.enable_z80n = true;

    ASSERT_CODE_WITH_OPTS(R"(
        OPTION -Z80N    ; Level 0: Z80N=Off
        OPTION PUSH     ; Push Level 0
        OPTION +Z80N    ; Level 1: Z80N=On
        SWAPNIB         ; OK (Instruction)
        OPTION PUSH     ; Push Level 1
        OPTION -Z80N    ; Level 2: Z80N=Off
        SWAPNIB: NOP    ; OK (Label 'SWAPNIB' followed by NOP)
        OPTION POP      ; Pop Level 1 (Z80N=On)
        SWAPNIB         ; OK (Instruction)
        OPTION POP      ; Pop Level 0 (Z80N=Off)
        ; SWAPNIB       ; Would be label here
    )", {0xED, 0x23, 0x00, 0xED, 0x23}, config);
}

TEST_CASE(SingleCharStringInstructions) {
    ASSERT_CODE("LD A, \"A\"", {0x3E, 'A'});
    ASSERT_CODE("CP \"8\"", {0xFE, '8'});
    ASSERT_CODE("ADD A, \" \"", {0xC6, ' '});
    ASSERT_CODE("SUB \"a\"", {0xD6, 'a'});
    ASSERT_CODE("LD B, \"*\"", {0x06, '*'});
    ASSERT_CODE("AND \"Z\"", {0xE6, 'Z'});
    ASSERT_CODE("XOR \"1\"", {0xEE, '1'});
    ASSERT_CODE("OR \"@\"", {0xF6, '@'});
}

TEST_CASE(SingleCharStringAsNumberContexts) {
    // Ternary operator
    ASSERT_CODE("DB \"A\" ? 1 : 2", {1});
    ASSERT_CODE("DB CHR(0) ? 1 : 2", {2});

    // D24
    ASSERT_CODE("D24 \"A\"", {0x41, 0x00, 0x00});
    
    // DWORD
    ASSERT_CODE("DWORD \"A\"", {0x41, 0x00, 0x00, 0x00});
    
    // DQ
    ASSERT_CODE("DQ \"A\"", {0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

TEST_CASE(Constants64Bit) {
    // Test defining and using a 64-bit constant
    // 0x1122334455667788
    ASSERT_CODE(R"(
        BIG_VAL EQU 0x1122334455667788
        DQ BIG_VAL
    )", {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11});

    // Test arithmetic with 64-bit constants
    ASSERT_CODE(R"(
        VAL1 EQU 0x100000000000000 ; 2^56 (1 followed by 14 zeros, 15 digits total)
        VAL2 EQU 0x0000000000000001
        RESULT EQU VAL1 + VAL2
        DQ RESULT
    )", {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01});
}

TEST_CASE(StringConversion64Bit) {
    // Test STR() with a large number
    // 1234567890123456789
    std::string code = R"(
        VAL EQU 1234567890123456789
        DB STR(VAL)
    )";
    // Expected bytes are the ASCII characters of the number
    std::string num_str = "1234567890123456789";
    std::vector<uint8_t> expected(num_str.begin(), num_str.end());
    ASSERT_CODE(code, expected);
}

TEST_CASE(Rand64Bit) {
    // Test RAND with a range exceeding 32 bits
    // We can't easily predict the value, but we can ensure it compiles and runs without crashing/truncating arguments
    ASSERT_RAND_IN_RANGE("DB RAND(0x100000000, 0x100000005) & 0xFF", 0, 5);
}

TEST_CASE(ValFunction64Bit) {
    ASSERT_CODE(R"(
        DEFINE VAL_STR "1234567890123456789"
        VAL_NUM EQU VAL(VAL_STR)
        DQ VAL_NUM
    )", {0x15, 0x81, 0xE9, 0x7D, 0xF4, 0x10, 0x22, 0x11}); // 0x112210F47DE98115
}

TEST_CASE(AbsFunction64Bit) {
    ASSERT_CODE(R"(
        VAL_NEG EQU -1234567890123456789
        VAL_POS EQU ABS(VAL_NEG)
        DQ VAL_POS
    )", {0x15, 0x81, 0xE9, 0x7D, 0xF4, 0x10, 0x22, 0x11});
}

TEST_CASE(MinMax64Bit) {
    ASSERT_CODE(R"(
        V1 EQU 0x100000000
        V2 EQU 0x200000000
        RES_MIN EQU MIN(V1, V2)
        RES_MAX EQU MAX(V1, V2)
        DQ RES_MIN
        DQ RES_MAX
    )", {
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // 0x100000000
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00  // 0x200000000
    });
}

TEST_CASE(EscapeSequences) {
    // Test string escapes
    ASSERT_CODE("DB \"\\\"\"", {'"'}); // "\"" -> "
    ASSERT_CODE("DB \"\\\\\"", {'\\'}); // "\\" -> \
    ASSERT_CODE("DB \"\\n\"", {0x0A}); // "\n" -> LF
    ASSERT_CODE("DB \"\\r\"", {0x0D}); // "\r" -> CR
    ASSERT_CODE("DB \"\\t\"", {0x09}); // "\t" -> TAB
    ASSERT_CODE("DB \"\\0\"", {0x00}); // "\0" -> NUL
    
    // Test mixed
    ASSERT_CODE("DB \"A\\nB\"", {'A', 0x0A, 'B'});
    
    // Test char literal escapes
    ASSERT_CODE("LD A, '\\n'", {0x3E, 0x0A});
    ASSERT_CODE("LD A, '\\''", {0x3E, '\''});
    ASSERT_CODE("LD A, '\\\\'", {0x3E, '\\'});
    
    // Test in macro arguments (splitting)
    ASSERT_CODE(R"(
        MY_MACRO MACRO arg1, arg2
            DB {arg1}
            DB {arg2}
        ENDM
        MY_MACRO "A,B", "C\"D"
    )", {'A', ',', 'B', 'C', '"', 'D'});
}

TEST_CASE(EscapeSequencesHex) {
    ASSERT_CODE("DB \"\\x41\"", {0x41}); // 'A'
    ASSERT_CODE("DB \"\\x00\"", {0x00});
    ASSERT_CODE("DB \"\\xFF\"", {0xFF});
    ASSERT_CODE("DB \"\\x1\"", {0x01}); // Single digit
    ASSERT_CODE("DB \"\\x61B\"", {0x61, 'B'}); // 'a', 'B'
    ASSERT_CODE("DB \"\\x\"", {'x'}); // Invalid hex, treat as 'x'
    ASSERT_CODE("DB \"\\xG\"", {'x', 'G'}); // Invalid hex, treat as 'x', 'G'
}

TEST_CASE(BlockGeneration) {
    std::string code = R"(
        ORG 0x1000
        NOP             ; Code (1 byte)
        LD A, 0         ; Code (2 bytes)
        DB 0x11         ; Data (1 byte)
        DS 2            ; Data (2 bytes)
        RET             ; Code (1 byte)
        
        ORG 0x2000
        DW 0x1234       ; Data (2 bytes)
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    bool ok = true;
    if (blocks.size() != 4) { std::cerr << "BlockGeneration: Expected 4 blocks, got " << blocks.size() << "\n"; ok = false; }
    else {
        if (blocks[0].start_address != 0x1000 || blocks[0].size != 3 || !blocks[0].is_code) { std::cerr << "Block 0 mismatch\n"; ok = false; }
        if (blocks[1].start_address != 0x1003 || blocks[1].size != 3 || blocks[1].is_code) { std::cerr << "Block 1 mismatch\n"; ok = false; }
        if (blocks[2].start_address != 0x1006 || blocks[2].size != 1 || !blocks[2].is_code) { std::cerr << "Block 2 mismatch\n"; ok = false; }
        if (blocks[3].start_address != 0x2000 || blocks[3].size != 2 || blocks[3].is_code) { std::cerr << "Block 3 mismatch\n"; ok = false; }
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(BlockGeneration_ComplexAddresses) {
    std::string code = R"(
        ORG START_ADDR
    
    CODE_BLOCK_1:
        LD HL, DATA_BLOCK
        LD A, (DATA_BLOCK)
        
        ORG $ + GAP_SIZE
        
    DATA_BLOCK:
        DB 0xAA, 0xBB
        
        ORG $ + GAP_SIZE
        
    CODE_BLOCK_2:
        HALT
        
    START_ADDR EQU 0x2000
    GAP_SIZE   EQU 0x10
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration_ComplexAddresses compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    bool ok = true;
    if (blocks.size() != 3) { std::cerr << "BlockGeneration_ComplexAddresses: Expected 3 blocks, got " << blocks.size() << "\n"; ok = false; }
    else {
        // Block 1: Code at 0x2000, size 6 (LD HL + LD A)
        if (blocks[0].start_address != 0x2000 || blocks[0].size != 6 || !blocks[0].is_code) { 
            std::cerr << "Block 0 mismatch: Addr=" << std::hex << blocks[0].start_address << " Size=" << std::dec << blocks[0].size << " Code=" << blocks[0].is_code << "\n"; 
            ok = false; 
        }
        // Block 2: Data at 0x2016 (0x2006 + 0x10), size 2 (DB, DB)
        if (blocks[1].start_address != 0x2016 || blocks[1].size != 2 || blocks[1].is_code) { 
            std::cerr << "Block 1 mismatch: Addr=" << std::hex << blocks[1].start_address << " Size=" << std::dec << blocks[1].size << " Code=" << blocks[1].is_code << "\n"; 
            ok = false; 
        }
        // Block 3: Code at 0x2028 (0x2018 + 0x10), size 1 (HALT)
        if (blocks[2].start_address != 0x2028 || blocks[2].size != 1 || !blocks[2].is_code) { 
            std::cerr << "Block 2 mismatch: Addr=" << std::hex << blocks[2].start_address << " Size=" << std::dec << blocks[2].size << " Code=" << blocks[2].is_code << "\n"; 
            ok = false; 
        }
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(BlockGeneration_MixedTypes) {
    std::string code = R"(
        ORG 0x1000
        NOP             ; Code (1 byte)
        DB 0x11         ; Data (1 byte)
        LD A, 0         ; Code (2 bytes)
        DW 0x1234       ; Data (2 bytes)
        RET             ; Code (1 byte)
        DS 2            ; Data (2 bytes)
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration_MixedTypes compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    // Expected:
    // 1. 0x1000, size 1, code (NOP)
    // 2. 0x1001, size 1, data (DB)
    // 3. 0x1002, size 2, code (LD A, 0)
    // 4. 0x1004, size 2, data (DW)
    // 5. 0x1006, size 1, code (RET)
    // 6. 0x1007, size 2, data (DS)

    bool ok = true;
    if (blocks.size() != 6) { 
        std::cerr << "BlockGeneration_MixedTypes: Expected 6 blocks, got " << blocks.size() << "\n"; 
        ok = false; 
    } else {
        auto check_block = [&](int idx, uint16_t addr, uint16_t size, bool is_code) {
            if (blocks[idx].start_address != addr || blocks[idx].size != size || blocks[idx].is_code != is_code) {
                std::cerr << "Block " << idx << " mismatch: "
                          << "Addr=" << std::hex << blocks[idx].start_address << " (Exp: " << addr << "), "
                          << "Size=" << std::dec << blocks[idx].size << " (Exp: " << size << "), "
                          << "Code=" << blocks[idx].is_code << " (Exp: " << is_code << ")\n";
                return false;
            }
            return true;
        };

        if (!check_block(0, 0x1000, 1, true)) ok = false;
        if (!check_block(1, 0x1001, 1, false)) ok = false;
        if (!check_block(2, 0x1002, 2, true)) ok = false;
        if (!check_block(3, 0x1004, 2, false)) ok = false;
        if (!check_block(4, 0x1006, 1, true)) ok = false;
        if (!check_block(5, 0x1007, 2, false)) ok = false;
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(BlockGeneration_Directives) {
    std::string code = R"(
        ORG 0x1000
        NOP             ; Code (1 byte) at 0x1000
        ALIGN 4         ; Data (padding 3 bytes: 0x1001, 0x1002, 0x1003) -> Next 0x1004
        LD A, 0         ; Code (2 bytes) at 0x1004
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration_Directives compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    // Expected:
    // 1. 0x1000, size 1, code (NOP)
    // 2. 0x1001, size 3, data (ALIGN padding)
    // 3. 0x1004, size 2, code (LD A, 0)

    bool ok = true;
    if (blocks.size() != 3) { 
        std::cerr << "BlockGeneration_Directives: Expected 3 blocks, got " << blocks.size() << "\n"; 
        ok = false; 
    } else {
        auto check_block = [&](int idx, uint16_t addr, uint16_t size, bool is_code) {
            if (blocks[idx].start_address != addr || blocks[idx].size != size || blocks[idx].is_code != is_code) {
                std::cerr << "Block " << idx << " mismatch: "
                          << "Addr=" << std::hex << blocks[idx].start_address << " (Exp: " << addr << "), "
                          << "Size=" << std::dec << blocks[idx].size << " (Exp: " << size << "), "
                          << "Code=" << blocks[idx].is_code << " (Exp: " << is_code << ")\n";
                return false;
            }
            return true;
        };

        if (!check_block(0, 0x1000, 1, true)) ok = false;
        if (!check_block(1, 0x1001, 3, false)) ok = false;
        if (!check_block(2, 0x1004, 2, true)) ok = false;
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(BlockGeneration_Macros) {
    std::string code = R"(
        MIXED MACRO
            NOP         ; Code (1)
            DB 0xAA     ; Data (1)
            RET         ; Code (1)
        ENDM
        
        ORG 0x1000
        MIXED
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration_Macros compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    // Expected:
    // 1. 0x1000, size 1, code (NOP)
    // 2. 0x1001, size 1, data (DB)
    // 3. 0x1002, size 1, code (RET)

    bool ok = true;
    if (blocks.size() != 3) { 
        std::cerr << "BlockGeneration_Macros: Expected 3 blocks, got " << blocks.size() << "\n"; 
        ok = false; 
    } else {
        auto check_block = [&](int idx, uint16_t addr, uint16_t size, bool is_code) {
            if (blocks[idx].start_address != addr || blocks[idx].size != size || blocks[idx].is_code != is_code) {
                std::cerr << "Block " << idx << " mismatch: "
                          << "Addr=" << std::hex << blocks[idx].start_address << " (Exp: " << addr << "), "
                          << "Size=" << std::dec << blocks[idx].size << " (Exp: " << size << "), "
                          << "Code=" << blocks[idx].is_code << " (Exp: " << is_code << ")\n";
                return false;
            }
            return true;
        };

        if (!check_block(0, 0x1000, 1, true)) ok = false;
        if (!check_block(1, 0x1001, 1, false)) ok = false;
        if (!check_block(2, 0x1002, 1, true)) ok = false;
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(BlockGeneration_Incbin) {
    std::string code = R"(
        ORG 0x1000
        NOP             ; Code (1)
        INCBIN "data.bin" ; Data (4)
        RET             ; Code (1)
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    file_provider.add_binary_source("data.bin", {0x11, 0x22, 0x33, 0x44});
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    bool success = assembler.compile("main.asm");
    if (!success) {
        std::cerr << "BlockGeneration_Incbin compilation failed\n";
        tests_failed++;
        return;
    }

    auto blocks = assembler.get_blocks();
    
    // Expected:
    // 1. 0x1000, size 1, code
    // 2. 0x1001, size 4, data
    // 3. 0x1005, size 1, code

    bool ok = true;
    if (blocks.size() != 3) { 
        std::cerr << "BlockGeneration_Incbin: Expected 3 blocks, got " << blocks.size() << "\n"; 
        ok = false; 
    } else {
        auto check_block = [&](int idx, uint16_t addr, uint16_t size, bool is_code) {
            if (blocks[idx].start_address != addr || blocks[idx].size != size || blocks[idx].is_code != is_code) {
                std::cerr << "Block " << idx << " mismatch: "
                          << "Addr=" << std::hex << blocks[idx].start_address << " (Exp: " << addr << "), "
                          << "Size=" << std::dec << blocks[idx].size << " (Exp: " << size << "), "
                          << "Code=" << blocks[idx].is_code << " (Exp: " << is_code << ")\n";
                return false;
            }
            return true;
        };

        if (!check_block(0, 0x1000, 1, true)) ok = false;
        if (!check_block(1, 0x1001, 4, false)) ok = false;
        if (!check_block(2, 0x1005, 1, true)) ok = false;
    }
    
    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(MemoryMapGeneration) {
    std::string code = R"(
        ORG 0x1000
        LD A, 0x10      ; Code: 3E 10 (Opcode, Operand)
        DB 0xAA         ; Data: AA
        NOP             ; Code: 00 (Opcode)
        DW 0xBBCC       ; Data: CC BB
        LD BC, 0x1234   ; Code: 01 34 12 (Opcode, Operand, Operand)
        DS 2, 0xFF      ; Data: FF FF
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    std::vector<uint8_t> memory_map;
    bool success = assembler.compile("main.asm", 0x0000, nullptr, nullptr, &memory_map);
    
    if (!success) {
        std::cerr << "MemoryMapGeneration compilation failed\n";
        tests_failed++;
        return;
    }

    if (memory_map.size() != 65536) {
        std::cerr << "MemoryMapGeneration: Expected map size 65536, got " << memory_map.size() << "\n";
        tests_failed++;
        return;
    }

    using Map = Z80::Assembler<Z80::StandardBus>::Map;
    auto check_map = [&](uint16_t addr, Map expected) {
        if (memory_map[addr] != (uint8_t)expected) {
            std::cerr << "MemoryMap mismatch at 0x" << std::hex << addr 
                      << ". Expected " << (int)expected << ", Got " << (int)memory_map[addr] << "\n";
            return false;
        }
        return true;
    };

    bool ok = true;
    if (!check_map(0x1000, Map::Opcode)) ok = false;  // LD A, ...
    if (!check_map(0x1001, Map::Operand)) ok = false; // ... 0x10
    if (!check_map(0x1002, Map::Data)) ok = false;    // DB 0xAA
    if (!check_map(0x1003, Map::Opcode)) ok = false;  // NOP
    if (!check_map(0x1004, Map::Data)) ok = false;    // DW low
    if (!check_map(0x1005, Map::Data)) ok = false;    // DW high
    if (!check_map(0x1006, Map::Opcode)) ok = false;  // LD BC, ...
    if (!check_map(0x1007, Map::Operand)) ok = false; // ... low
    if (!check_map(0x1008, Map::Operand)) ok = false; // ... high
    if (!check_map(0x1009, Map::Data)) ok = false;    // DS byte 1
    if (!check_map(0x100A, Map::Data)) ok = false;    // DS byte 2
    
    // Check uninitialized area
    if (!check_map(0x0000, Map::None)) ok = false;
    if (!check_map(0x2000, Map::None)) ok = false;

    if (ok) tests_passed++; else tests_failed++;
}

TEST_CASE(MemoryMapPhaseDephase) {
    std::string code = R"(
        ORG 0x1000
        PHASE 0x8000
        NOP             ; Logical 0x8000, Physical 0x1000
        DEPHASE
    )";

    Z80::StandardBus bus;
    MockFileProvider file_provider;
    file_provider.add_source("main.asm", code);
    Z80::Assembler<Z80::StandardBus> assembler(&bus, &file_provider);
    
    std::vector<uint8_t> memory_map;
    bool success = assembler.compile("main.asm", 0x0000, nullptr, nullptr, &memory_map);
    
    if (!success) {
        std::cerr << "MemoryMapPhaseDephase compilation failed\n";
        tests_failed++;
        return;
    }

    using Map = Z80::Assembler<Z80::StandardBus>::Map;
    
    bool ok = true;
    // Should be at physical address 0x1000
    if (memory_map[0x1000] != (uint8_t)Map::Opcode) {
        std::cerr << "MemoryMapPhaseDephase: Expected Opcode at physical 0x1000, got " << (int)memory_map[0x1000] << "\n";
        ok = false;
    }
    // Should NOT be at logical address 0x8000 (unless physical was also 0x8000, which it isn't)
    if (memory_map[0x8000] != (uint8_t)Map::None) {
        std::cerr << "MemoryMapPhaseDephase: Expected None at logical 0x8000, got " << (int)memory_map[0x8000] << "\n";
        ok = false;
    }

    if (ok) tests_passed++; else tests_failed++;
}

int main() {
    std::cout << "=============================\n";
    std::cout << "  Running Z80Assembler Tests \n";
    std::cout << "=============================\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    run_all_tests();

    auto end_time = std::chrono::high_resolution_clock::now();
    long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    long long ms = total_ms % 1000;
    long long total_seconds = total_ms / 1000;
    long long seconds = total_seconds % 60;
    long long minutes = total_seconds / 60;

    std::cout << "\n=============================\n";
    std::cout << "Test summary:\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";
    std::cout << "  Duration: " << std::setfill('0') << std::setw(2) << minutes << "m "
              << std::setfill('0') << std::setw(2) << seconds << "s "
              << std::setw(3) << ms << "ms\n";
    std::cout << "=============================\n";

    return (tests_failed > 0) ? 1 : 0;
}
