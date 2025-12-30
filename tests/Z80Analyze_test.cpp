//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Analyze_test.cpp
// Verson: 1.1.6
//
// This file contains unit tests for the Z80Analyzer class.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Analyze.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

// Mock Memory implementation
class TestMemory {
public:
    std::vector<uint8_t> data;

    TestMemory() : data(65536, 0) {}

    uint8_t peek(uint16_t address) {
        return data[address];
    }

    void poke(uint16_t address, uint8_t value) {
        data[address] = value;
    }

    void set_data(uint16_t address, const std::vector<uint8_t>& bytes) {
        for (size_t i = 0; i < bytes.size(); ++i) {
            data[(address + i) & 0xFFFF] = bytes[i];
        }
    }
};

// Mock Labels implementation
class TestLabels : public ILabels {
public:
    std::map<uint16_t, std::string> labels;

    std::string get_label(uint16_t address) const override {
        auto it = labels.find(address);
        if (it != labels.end()) {
            return it->second;
        }
        return "";
    }

    void add_label(uint16_t address, const std::string& label) override {
        labels[address] = label;
    }
};

class TestableZ80Analyzer : public Z80Analyzer<TestMemory> {
public:
    using Z80Analyzer<TestMemory>::Z80Analyzer;

    std::vector<CodeLine> generate_listing_public(CodeMap& map, uint16_t& start_address, size_t instruction_limit, bool use_map, size_t max_data_group = 16) {
        return generate_listing(map, start_address, instruction_limit, use_map, max_data_group);
    }
};

int tests_passed = 0;
int tests_failed = 0;

void check_inst(TestableZ80Analyzer& analyzer, TestMemory& memory, 
                const std::vector<uint8_t>& bytes, 
                const std::string& expected_mnemonic, 
                const std::vector<std::string>& expected_ops) {
    uint16_t addr = 0x1000;
    memory.set_data(addr, bytes);
    auto line = analyzer.parse_instruction(addr);
    
    bool match = true;
    if (line.mnemonic != expected_mnemonic) match = false;
    if (line.operands.size() != expected_ops.size()) match = false;
    
    if (match) {
        for (size_t i = 0; i < expected_ops.size(); ++i) {
            std::string op_str;
            const auto& op = line.operands[i];
            switch (op.type) {
                case Z80Analyzer<TestMemory>::CodeLine::Operand::REG8:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::REG16:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::CONDITION:
                    op_str = op.s_val;
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::IMM8:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::PORT_IMM8:
                    { std::stringstream ss; ss << "0x" << std::hex << std::uppercase << op.num_val; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::IMM16:
                    { std::stringstream ss; ss << "0x" << std::hex << std::uppercase << op.num_val; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_REG16:
                    op_str = "(" + op.s_val + ")";
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_IMM16:
                    { std::stringstream ss; ss << "(0x" << std::hex << std::uppercase << op.num_val << ")"; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_INDEXED:
                    { 
                        std::stringstream ss; 
                        ss << "(" << op.base_reg << (op.offset >= 0 ? "+" : "") << std::dec << (int)op.offset << ")"; 
                        op_str = ss.str(); 
                    }
                    break;
                default:
                    op_str = "???";
            }
            
            // Simple check to handle potential formatting differences (e.g. 0x0 vs 0x00)
            if (op_str != expected_ops[i]) {
                 // If simple string comparison fails, we could try more robust parsing, 
                 // but for this test suite we will rely on consistent formatting in test cases.
                 match = false; 
                 break;
            }
        }
    }

    if (match) {
        tests_passed++;
    } else {
        tests_failed++;
        std::cout << "FAIL: Expected " << expected_mnemonic << " ";
        for(auto& s : expected_ops) std::cout << s << " ";
        std::cout << "\n      Got      " << line.mnemonic << " ";
        for(size_t i=0; i<line.operands.size(); ++i) {
            std::string op_str;
            const auto& op = line.operands[i];
            switch (op.type) {
                case Z80Analyzer<TestMemory>::CodeLine::Operand::REG8:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::REG16:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::CONDITION:
                    op_str = op.s_val;
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::IMM8:
                case Z80Analyzer<TestMemory>::CodeLine::Operand::PORT_IMM8:
                    { std::stringstream ss; ss << "0x" << std::hex << std::uppercase << op.num_val; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::IMM16:
                    { std::stringstream ss; ss << "0x" << std::hex << std::uppercase << op.num_val; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_REG16:
                    op_str = "(" + op.s_val + ")";
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_IMM16:
                    { std::stringstream ss; ss << "(0x" << std::hex << std::uppercase << op.num_val << ")"; op_str = ss.str(); }
                    break;
                case Z80Analyzer<TestMemory>::CodeLine::Operand::MEM_INDEXED:
                    { 
                        std::stringstream ss; 
                        ss << "(" << op.base_reg << (op.offset >= 0 ? "+" : "") << std::dec << (int)op.offset << ")"; 
                        op_str = ss.str(); 
                    }
                    break;
                default:
                    op_str = "???";
            }
            std::cout << op_str << " ";
        }
        std::cout << "\n";
    }
}

void run_tests() {
    TestMemory memory;
    TestLabels labels;
    TestableZ80Analyzer analyzer(&memory, &labels);

    std::cout << "Running Z80Analyzer tests...\n";

    // --- 8-bit Loads ---
    check_inst(analyzer, memory, {0x78}, "LD", {"A", "B"});
    check_inst(analyzer, memory, {0x06, 0x55}, "LD", {"B", "0x55"});
    check_inst(analyzer, memory, {0x0A}, "LD", {"A", "(BC)"});
    check_inst(analyzer, memory, {0x32, 0x00, 0x20}, "LD", {"(0x2000)", "A"});
    check_inst(analyzer, memory, {0xED, 0x57}, "LD A, I", {}); // Mnemonic includes operands for this one in analyzer
    check_inst(analyzer, memory, {0xED, 0x5F}, "LD A, R", {});

    // --- 16-bit Loads ---
    check_inst(analyzer, memory, {0x01, 0x34, 0x12}, "LD", {"BC", "0x1234"});
    check_inst(analyzer, memory, {0xC5}, "PUSH", {"BC"});
    check_inst(analyzer, memory, {0xF1}, "POP", {"AF"});
    check_inst(analyzer, memory, {0xED, 0x4B, 0x00, 0x30}, "LD", {"BC", "(0x3000)"});
    
    // --- ALU 8-bit ---
    check_inst(analyzer, memory, {0x80}, "ADD", {"A", "B"});
    check_inst(analyzer, memory, {0xC6, 0x10}, "ADD", {"A", "0x10"});
    check_inst(analyzer, memory, {0x90}, "SUB", {"B"});
    check_inst(analyzer, memory, {0xA0}, "AND", {"B"});
    check_inst(analyzer, memory, {0xB0}, "OR", {"B"});
    check_inst(analyzer, memory, {0xA8}, "XOR", {"B"});
    check_inst(analyzer, memory, {0xB8}, "CP", {"B"});
    check_inst(analyzer, memory, {0x3C}, "INC", {"A"});
    check_inst(analyzer, memory, {0x3D}, "DEC", {"A"});
    check_inst(analyzer, memory, {0x2F}, "CPL", {});
    check_inst(analyzer, memory, {0x27}, "DAA", {});
    check_inst(analyzer, memory, {0x37}, "SCF", {});
    check_inst(analyzer, memory, {0x3F}, "CCF", {});

    // --- ALU 16-bit ---
    check_inst(analyzer, memory, {0x09}, "ADD", {"HL", "BC"});
    check_inst(analyzer, memory, {0xED, 0x4A}, "ADC", {"HL", "BC"});
    check_inst(analyzer, memory, {0xED, 0x42}, "SBC", {"HL", "BC"});
    check_inst(analyzer, memory, {0x03}, "INC", {"BC"});
    check_inst(analyzer, memory, {0x0B}, "DEC", {"BC"});
    
    // --- Control / Branching ---
    check_inst(analyzer, memory, {0x00}, "NOP", {});
    check_inst(analyzer, memory, {0x76}, "HALT", {});
    check_inst(analyzer, memory, {0xF3}, "DI", {});
    check_inst(analyzer, memory, {0xFB}, "EI", {});
    check_inst(analyzer, memory, {0xC3, 0x00, 0x00}, "JP", {"0x0"});
    check_inst(analyzer, memory, {0xC2, 0x00, 0x00}, "JP", {"NZ", "0x0"});
    check_inst(analyzer, memory, {0x18, 0xFE}, "JR", {"0x1000"}); // 0x1000 + 2 - 2 = 0x1000
    check_inst(analyzer, memory, {0x20, 0xFE}, "JR", {"NZ", "0x1000"});
    check_inst(analyzer, memory, {0x10, 0xFE}, "DJNZ", {"0x1000"});
    check_inst(analyzer, memory, {0xCD, 0x00, 0x00}, "CALL", {"0x0"});
    check_inst(analyzer, memory, {0xC9}, "RET", {});
    check_inst(analyzer, memory, {0xC0}, "RET", {"NZ"});
    check_inst(analyzer, memory, {0xC7}, "RST", {"0x0"});
    
    // --- IO ---
    check_inst(analyzer, memory, {0xD3, 0x10}, "OUT", {"0x10", "A"});
    check_inst(analyzer, memory, {0xDB, 0x10}, "IN", {"A", "0x10"});
    check_inst(analyzer, memory, {0xED, 0x78}, "IN", {"A", "(C)"});
    check_inst(analyzer, memory, {0xED, 0x79}, "OUT", {"(C)", "A"});
    
    // --- Extended (ED) ---
    check_inst(analyzer, memory, {0xED, 0xB0}, "LDIR", {});
    check_inst(analyzer, memory, {0xED, 0x45}, "RETN", {});
    check_inst(analyzer, memory, {0xED, 0x46}, "IM", {"0x0"});
    check_inst(analyzer, memory, {0xED, 0x44}, "NEG", {});
    
    // --- Bit/Shift (CB) ---
    check_inst(analyzer, memory, {0xCB, 0x07}, "RLC", {"A"});
    check_inst(analyzer, memory, {0xCB, 0x40}, "BIT", {"0x0", "B"});
    check_inst(analyzer, memory, {0xCB, 0xC7}, "SET", {"0x0", "A"});
    check_inst(analyzer, memory, {0xCB, 0x87}, "RES", {"0x0", "A"});
    
    // --- Index (IX/IY) ---
    check_inst(analyzer, memory, {0xDD, 0x21, 0x00, 0x10}, "LD", {"IX", "0x1000"});
    check_inst(analyzer, memory, {0xFD, 0x21, 0x00, 0x10}, "LD", {"IY", "0x1000"});
    check_inst(analyzer, memory, {0xDD, 0x7E, 0x05}, "LD", {"A", "(IX+5)"});
    check_inst(analyzer, memory, {0xFD, 0x7E, 0xFB}, "LD", {"A", "(IY-5)"});
    check_inst(analyzer, memory, {0xDD, 0x86, 0x00}, "ADD", {"A", "(IX+0)"});
    check_inst(analyzer, memory, {0xDD, 0xE9}, "JP", {"(IX)"});
    
    // --- Index Bit (DDCB/FDCB) ---
    check_inst(analyzer, memory, {0xDD, 0xCB, 0x05, 0x46}, "BIT", {"0x0", "(IX+5)"});
    check_inst(analyzer, memory, {0xFD, 0xCB, 0x10, 0xCE}, "SET", {"0x1", "(IY+16)"});

    // --- Additional Control Flow Tests ---
    {
        // Jumps with conditions
        check_inst(analyzer, memory, {0xC2, 0x00, 0x10}, "JP", {"NZ", "0x1000"});
        check_inst(analyzer, memory, {0xCA, 0x00, 0x10}, "JP", {"Z", "0x1000"});
        check_inst(analyzer, memory, {0xD2, 0x00, 0x10}, "JP", {"NC", "0x1000"});
        check_inst(analyzer, memory, {0xDA, 0x00, 0x10}, "JP", {"C", "0x1000"});
        check_inst(analyzer, memory, {0xE2, 0x00, 0x10}, "JP", {"PO", "0x1000"});
        check_inst(analyzer, memory, {0xEA, 0x00, 0x10}, "JP", {"PE", "0x1000"});
        check_inst(analyzer, memory, {0xF2, 0x00, 0x10}, "JP", {"P", "0x1000"});
        check_inst(analyzer, memory, {0xFA, 0x00, 0x10}, "JP", {"M", "0x1000"});
        check_inst(analyzer, memory, {0xE9}, "JP", {"(HL)"});
        check_inst(analyzer, memory, {0xDD, 0xE9}, "JP", {"(IX)"});
        check_inst(analyzer, memory, {0xFD, 0xE9}, "JP", {"(IY)"});

        // Relative Jumps
        check_inst(analyzer, memory, {0x20, 0xFE}, "JR", {"NZ", "0x1000"});
        check_inst(analyzer, memory, {0x28, 0xFE}, "JR", {"Z", "0x1000"});
        check_inst(analyzer, memory, {0x30, 0xFE}, "JR", {"NC", "0x1000"});
        check_inst(analyzer, memory, {0x38, 0xFE}, "JR", {"C", "0x1000"});

        // Calls with conditions
        check_inst(analyzer, memory, {0xC4, 0x00, 0x10}, "CALL", {"NZ", "0x1000"});
        check_inst(analyzer, memory, {0xCC, 0x00, 0x10}, "CALL", {"Z", "0x1000"});
        check_inst(analyzer, memory, {0xD4, 0x00, 0x10}, "CALL", {"NC", "0x1000"});
        check_inst(analyzer, memory, {0xDC, 0x00, 0x10}, "CALL", {"C", "0x1000"});
        check_inst(analyzer, memory, {0xE4, 0x00, 0x10}, "CALL", {"PO", "0x1000"});
        check_inst(analyzer, memory, {0xEC, 0x00, 0x10}, "CALL", {"PE", "0x1000"});
        check_inst(analyzer, memory, {0xF4, 0x00, 0x10}, "CALL", {"P", "0x1000"});
        check_inst(analyzer, memory, {0xFC, 0x00, 0x10}, "CALL", {"M", "0x1000"});

        // Returns with conditions
        check_inst(analyzer, memory, {0xC0}, "RET", {"NZ"});
        check_inst(analyzer, memory, {0xC8}, "RET", {"Z"});
        check_inst(analyzer, memory, {0xD0}, "RET", {"NC"});
        check_inst(analyzer, memory, {0xD8}, "RET", {"C"});
        check_inst(analyzer, memory, {0xE0}, "RET", {"PO"});
        check_inst(analyzer, memory, {0xE8}, "RET", {"PE"});
        check_inst(analyzer, memory, {0xF0}, "RET", {"P"});
        check_inst(analyzer, memory, {0xF8}, "RET", {"M"});

        // Restarts
        check_inst(analyzer, memory, {0xC7}, "RST", {"0x0"});
        check_inst(analyzer, memory, {0xCF}, "RST", {"0x8"});
        check_inst(analyzer, memory, {0xD7}, "RST", {"0x10"});
        check_inst(analyzer, memory, {0xDF}, "RST", {"0x18"});
        check_inst(analyzer, memory, {0xE7}, "RST", {"0x20"});
        check_inst(analyzer, memory, {0xEF}, "RST", {"0x28"});
        check_inst(analyzer, memory, {0xF7}, "RST", {"0x30"});
        check_inst(analyzer, memory, {0xFF}, "RST", {"0x38"});
    }

    // --- Stack & 16-bit Arithmetic Tests ---
    {
        // PUSH/POP
        check_inst(analyzer, memory, {0xC5}, "PUSH", {"BC"});
        check_inst(analyzer, memory, {0xD5}, "PUSH", {"DE"});
        check_inst(analyzer, memory, {0xE5}, "PUSH", {"HL"});
        check_inst(analyzer, memory, {0xF5}, "PUSH", {"AF"});
        check_inst(analyzer, memory, {0xDD, 0xE5}, "PUSH", {"IX"});
        check_inst(analyzer, memory, {0xFD, 0xE5}, "PUSH", {"IY"});

        check_inst(analyzer, memory, {0xC1}, "POP", {"BC"});
        check_inst(analyzer, memory, {0xD1}, "POP", {"DE"});
        check_inst(analyzer, memory, {0xE1}, "POP", {"HL"});
        check_inst(analyzer, memory, {0xF1}, "POP", {"AF"});
        check_inst(analyzer, memory, {0xDD, 0xE1}, "POP", {"IX"});
        check_inst(analyzer, memory, {0xFD, 0xE1}, "POP", {"IY"});

        // 16-bit Arithmetic (IX/IY)
        check_inst(analyzer, memory, {0xDD, 0x09}, "ADD", {"IX", "BC"});
        check_inst(analyzer, memory, {0xFD, 0x19}, "ADD", {"IY", "DE"});
        check_inst(analyzer, memory, {0xDD, 0x29}, "ADD", {"IX", "IX"});
        check_inst(analyzer, memory, {0xFD, 0x39}, "ADD", {"IY", "SP"});
        
        // LD SP, HL/IX/IY
        check_inst(analyzer, memory, {0xF9}, "LD", {"SP", "HL"});
        check_inst(analyzer, memory, {0xDD, 0xF9}, "LD", {"SP", "IX"});
        check_inst(analyzer, memory, {0xFD, 0xF9}, "LD", {"SP", "IY"});
    }

    // --- Parse Instruction Edge Cases & Missing Groups ---
    {
        // Exchange
        check_inst(analyzer, memory, {0xEB}, "EX", {"DE", "HL"});
        check_inst(analyzer, memory, {0xE3}, "EX", {"(SP)", "HL"});
        check_inst(analyzer, memory, {0xDD, 0xE3}, "EX", {"(SP)", "IX"});
        check_inst(analyzer, memory, {0xFD, 0xE3}, "EX", {"(SP)", "IY"});

        // Block instructions (Group 2)
        check_inst(analyzer, memory, {0xED, 0xA0}, "LDI", {});
        check_inst(analyzer, memory, {0xED, 0xA1}, "CPI", {});
        check_inst(analyzer, memory, {0xED, 0xA2}, "INI", {});
        check_inst(analyzer, memory, {0xED, 0xA3}, "OUTI", {});
        
        // Rotate Digit
        check_inst(analyzer, memory, {0xED, 0x67}, "RRD", {});
        check_inst(analyzer, memory, {0xED, 0x6F}, "RLD", {});

        // Interrupt Modes
        check_inst(analyzer, memory, {0xED, 0x56}, "IM", {"0x1"});
        check_inst(analyzer, memory, {0xED, 0x5E}, "IM", {"0x2"});

        // Unknown ED opcode (fallback to NOP with operands)
        check_inst(analyzer, memory, {0xED, 0xFF}, "NOP", {"0xED", "0xFF"});

        // Prefix handling
        check_inst(analyzer, memory, {0xDD, 0x00}, "NOP", {}); // IX prefix + NOP -> NOP
        check_inst(analyzer, memory, {0xDD, 0xFD, 0x21, 0x00, 0x00}, "LD", {"IY", "0x0"}); // Double prefix

        // More ED instructions
        check_inst(analyzer, memory, {0xED, 0x47}, "LD I, A", {});
        check_inst(analyzer, memory, {0xED, 0x4F}, "LD R, A", {});
        check_inst(analyzer, memory, {0xED, 0x57}, "LD A, I", {});
        check_inst(analyzer, memory, {0xED, 0x5F}, "LD A, R", {});
        check_inst(analyzer, memory, {0xED, 0x4D}, "RETI", {});

        // Block instructions (Repeating & Decrementing)
        check_inst(analyzer, memory, {0xED, 0xA8}, "LDD", {});
        check_inst(analyzer, memory, {0xED, 0xB8}, "LDDR", {});
        check_inst(analyzer, memory, {0xED, 0xB1}, "CPIR", {});
        check_inst(analyzer, memory, {0xED, 0xA9}, "CPD", {});
        check_inst(analyzer, memory, {0xED, 0xB9}, "CPDR", {});
        check_inst(analyzer, memory, {0xED, 0xB2}, "INIR", {});
        check_inst(analyzer, memory, {0xED, 0xAA}, "IND", {});
        check_inst(analyzer, memory, {0xED, 0xBA}, "INDR", {});
        check_inst(analyzer, memory, {0xED, 0xB3}, "OTIR", {});
        check_inst(analyzer, memory, {0xED, 0xAB}, "OUTD", {});
        check_inst(analyzer, memory, {0xED, 0xBB}, "OTDR", {});
    }

    // --- Undocumented Instructions ---
    {
        // SLL (Shift Left Logical) - CB 30-37
        check_inst(analyzer, memory, {0xCB, 0x37}, "SLL", {"A"});
        check_inst(analyzer, memory, {0xCB, 0x30}, "SLL", {"B"});
        
        // SLL (IX+d)
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x05, 0x36}, "SLL", {"(IX+5)"});

        // IXH/IXL/IYH/IYL Access
        // DD 44 -> LD B, IXH
        check_inst(analyzer, memory, {0xDD, 0x44}, "LD", {"B", "IXH"});
        // DD 45 -> LD B, IXL
        check_inst(analyzer, memory, {0xDD, 0x45}, "LD", {"B", "IXL"});
        // FD 44 -> LD B, IYH
        check_inst(analyzer, memory, {0xFD, 0x44}, "LD", {"B", "IYH"});
        
        // Arithmetic on IXH/IXL
        // DD 84 -> ADD A, IXH
        check_inst(analyzer, memory, {0xDD, 0x84}, "ADD", {"A", "IXH"});
        // DD 24 -> INC IXH
        check_inst(analyzer, memory, {0xDD, 0x24}, "INC", {"IXH"});
    }

    // --- Accumulator & Flags ---
    {
        check_inst(analyzer, memory, {0x07}, "RLCA", {});
        check_inst(analyzer, memory, {0x0F}, "RRCA", {});
        check_inst(analyzer, memory, {0x17}, "RLA", {});
        check_inst(analyzer, memory, {0x1F}, "RRA", {});
        check_inst(analyzer, memory, {0xD9}, "EXX", {});
    }

    // --- Direct Addressing (16-bit & 8-bit) ---
    {
        check_inst(analyzer, memory, {0x3A, 0x34, 0x12}, "LD", {"A", "(0x1234)"});
        check_inst(analyzer, memory, {0x32, 0x34, 0x12}, "LD", {"(0x1234)", "A"});

        // LD HL, (nn) / LD (nn), HL
        check_inst(analyzer, memory, {0x2A, 0x34, 0x12}, "LD", {"HL", "(0x1234)"});
        check_inst(analyzer, memory, {0x22, 0x34, 0x12}, "LD", {"(0x1234)", "HL"});
        
        // LD dd, (nn) / LD (nn), dd (ED prefix)
        check_inst(analyzer, memory, {0xED, 0x4B, 0x34, 0x12}, "LD", {"BC", "(0x1234)"});
        check_inst(analyzer, memory, {0xED, 0x5B, 0x34, 0x12}, "LD", {"DE", "(0x1234)"});
        check_inst(analyzer, memory, {0xED, 0x7B, 0x34, 0x12}, "LD", {"SP", "(0x1234)"});
        
        check_inst(analyzer, memory, {0xED, 0x43, 0x34, 0x12}, "LD", {"(0x1234)", "BC"});
        check_inst(analyzer, memory, {0xED, 0x53, 0x34, 0x12}, "LD", {"(0x1234)", "DE"});
        check_inst(analyzer, memory, {0xED, 0x73, 0x34, 0x12}, "LD", {"(0x1234)", "SP"});

        // IX/IY Memory Access
        check_inst(analyzer, memory, {0xDD, 0x2A, 0x34, 0x12}, "LD", {"IX", "(0x1234)"});
        check_inst(analyzer, memory, {0xFD, 0x2A, 0x34, 0x12}, "LD", {"IY", "(0x1234)"});
        check_inst(analyzer, memory, {0xDD, 0x22, 0x34, 0x12}, "LD", {"(0x1234)", "IX"});
        check_inst(analyzer, memory, {0xFD, 0x22, 0x34, 0x12}, "LD", {"(0x1234)", "IY"});
    }

    // --- Extended I/O (ED prefix) ---
    {
        check_inst(analyzer, memory, {0xED, 0x40}, "IN", {"B", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x48}, "IN", {"C", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x50}, "IN", {"D", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x58}, "IN", {"E", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x60}, "IN", {"H", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x68}, "IN", {"L", "(C)"});
        check_inst(analyzer, memory, {0xED, 0x70}, "IN", {"(C)"});

        check_inst(analyzer, memory, {0xED, 0x41}, "OUT", {"(C)", "B"});
        check_inst(analyzer, memory, {0xED, 0x49}, "OUT", {"(C)", "C"});
        check_inst(analyzer, memory, {0xED, 0x51}, "OUT", {"(C)", "D"});
        check_inst(analyzer, memory, {0xED, 0x59}, "OUT", {"(C)", "E"});
        check_inst(analyzer, memory, {0xED, 0x61}, "OUT", {"(C)", "H"});
        check_inst(analyzer, memory, {0xED, 0x69}, "OUT", {"(C)", "L"});
        check_inst(analyzer, memory, {0xED, 0x71}, "OUT", {"(C)", "0x0"});
    }

    // --- Extended 16-bit Arithmetic (ED prefix) ---
    {
        check_inst(analyzer, memory, {0xED, 0x42}, "SBC", {"HL", "BC"});
        check_inst(analyzer, memory, {0xED, 0x52}, "SBC", {"HL", "DE"});
        check_inst(analyzer, memory, {0xED, 0x62}, "SBC", {"HL", "HL"});
        check_inst(analyzer, memory, {0xED, 0x72}, "SBC", {"HL", "SP"});

        check_inst(analyzer, memory, {0xED, 0x4A}, "ADC", {"HL", "BC"});
        check_inst(analyzer, memory, {0xED, 0x5A}, "ADC", {"HL", "DE"});
        check_inst(analyzer, memory, {0xED, 0x6A}, "ADC", {"HL", "HL"});
        check_inst(analyzer, memory, {0xED, 0x7A}, "ADC", {"HL", "SP"});
    }

    // --- Indirect HL Operations ---
    {
        // LD (HL), n
        check_inst(analyzer, memory, {0x36, 0x55}, "LD", {"(HL)", "0x55"});
        
        // LD r, (HL)
        check_inst(analyzer, memory, {0x7E}, "LD", {"A", "(HL)"});
        check_inst(analyzer, memory, {0x46}, "LD", {"B", "(HL)"});
        
        // LD (HL), r
        check_inst(analyzer, memory, {0x77}, "LD", {"(HL)", "A"});
        check_inst(analyzer, memory, {0x70}, "LD", {"(HL)", "B"});

        // INC/DEC (HL)
        check_inst(analyzer, memory, {0x34}, "INC", {"(HL)"});
        check_inst(analyzer, memory, {0x35}, "DEC", {"(HL)"});

        // ALU (HL)
        check_inst(analyzer, memory, {0x86}, "ADD", {"A", "(HL)"});
        check_inst(analyzer, memory, {0x8E}, "ADC", {"A", "(HL)"});
        check_inst(analyzer, memory, {0x96}, "SUB", {"(HL)"});
        check_inst(analyzer, memory, {0x9E}, "SBC", {"A", "(HL)"});
        check_inst(analyzer, memory, {0xA6}, "AND", {"(HL)"});
        check_inst(analyzer, memory, {0xAE}, "XOR", {"(HL)"});
        check_inst(analyzer, memory, {0xB6}, "OR", {"(HL)"});
        check_inst(analyzer, memory, {0xBE}, "CP", {"(HL)"});
    }

    // --- Indirect HL Bit/Shift ---
    {
        // RLC (HL)
        check_inst(analyzer, memory, {0xCB, 0x06}, "RLC", {"(HL)"});
        // RRC (HL)
        check_inst(analyzer, memory, {0xCB, 0x0E}, "RRC", {"(HL)"});
        // RL (HL)
        check_inst(analyzer, memory, {0xCB, 0x16}, "RL", {"(HL)"});
        // RR (HL)
        check_inst(analyzer, memory, {0xCB, 0x1E}, "RR", {"(HL)"});
        // SLA (HL)
        check_inst(analyzer, memory, {0xCB, 0x26}, "SLA", {"(HL)"});
        // SRA (HL)
        check_inst(analyzer, memory, {0xCB, 0x2E}, "SRA", {"(HL)"});
        // SLL (HL) - Undocumented but supported
        check_inst(analyzer, memory, {0xCB, 0x36}, "SLL", {"(HL)"});
        // SRL (HL)
        check_inst(analyzer, memory, {0xCB, 0x3E}, "SRL", {"(HL)"});

        // BIT b, (HL)
        check_inst(analyzer, memory, {0xCB, 0x46}, "BIT", {"0x0", "(HL)"});
        check_inst(analyzer, memory, {0xCB, 0x7E}, "BIT", {"0x7", "(HL)"});
        
        // RES b, (HL)
        check_inst(analyzer, memory, {0xCB, 0x86}, "RES", {"0x0", "(HL)"});
        check_inst(analyzer, memory, {0xCB, 0xBE}, "RES", {"0x7", "(HL)"});

        // SET b, (HL)
        check_inst(analyzer, memory, {0xCB, 0xC6}, "SET", {"0x0", "(HL)"});
        check_inst(analyzer, memory, {0xCB, 0xFE}, "SET", {"0x7", "(HL)"});
    }

    // --- Misc Instructions ---
    {
        check_inst(analyzer, memory, {0x08}, "EX AF, AF'", {});
    }

    // --- Parse Instruction Backwards ---
    {
        // Setup a sequence of instructions
        // 0x5000: LD A, 10     (3E 0A) - 2 bytes
        // 0x5002: INC A        (3C)    - 1 byte
        // 0x5003: LD B, A      (47)    - 1 byte
        // 0x5004: NOP          (00)    - 1 byte
        
        memory.set_data(0x5000, {0x3E, 0x0A, 0x3C, 0x47, 0x00});
        
        // Test finding start of 0x5002 (INC A) -> should be 0x5000
        uint16_t prev = analyzer.parse_instruction_backwards(0x5002);
        if (prev != 0x5000) {
             std::cout << "FAIL: parse_instruction_backwards(0x5002) returned 0x" << std::hex << prev << " expected 0x5000\n";
             tests_failed++;
        } else {
             tests_passed++;
        }

        // Target 0x5003 (LD B, A), prev should be 0x5002
        prev = analyzer.parse_instruction_backwards(0x5003);
        if (prev != 0x5002) {
             std::cout << "FAIL: parse_instruction_backwards(0x5003) returned 0x" << std::hex << prev << " expected 0x5002\n";
             tests_failed++;
        } else {
             tests_passed++;
        }
        
        // Test with CodeMap hint
        Z80Analyzer<TestMemory>::CodeMap map(0x10000, 0);
        map.mark_code(0x5000, 2, true); // Mark LD A, 10
        
        prev = analyzer.parse_instruction_backwards(0x5002, &map);
        if (prev != 0x5000) {
             std::cout << "FAIL: parse_instruction_backwards(0x5002, map) returned 0x" << std::hex << prev << " expected 0x5000\n";
             tests_failed++;
        } else {
             tests_passed++;
        }
    }

    // --- Data Grouping Test ---
    {
        std::vector<uint8_t> data_block;
        for(int i=0; i<20; ++i) data_block.push_back(i);
        memory.set_data(0x2000, data_block);

        Z80Analyzer<TestMemory>::CodeMap map(0x10000, 0); 
        uint16_t start_addr = 0x2000;
        
        auto lines = analyzer.generate_listing_public(map, start_addr, 100, true, 8);
        
        if (lines.empty()) {
            std::cout << "FAIL: Data grouping returned no lines\n";
            tests_failed++;
        } else {
            if (lines[0].mnemonic == "DB" && lines[0].operands.size() == 8) tests_passed++;
            else {
                std::cout << "FAIL: Data grouping failed (expected DB with 8 operands)\n";
                std::cout << "      Got: " << lines[0].mnemonic << " with " << lines[0].operands.size() << " operands\n";
                tests_failed++;
            }
        }
    }

    // --- CodeMap Tests ---
    {
        using CodeMap = Z80Analyzer<TestMemory>::CodeMap;
        CodeMap map(0x10000, 0);
        bool codemap_pass = true;

        // Test 1: Mark Code
        map.mark_code(0x1000, 3, true);
        if (map[0x1000] != CodeMap::FLAG_CODE_START) {
            std::cout << "FAIL: CodeMap mark_code start flag missing\n";
            codemap_pass = false;
        }
        if (map[0x1001] != CodeMap::FLAG_CODE_INTERIOR) {
            std::cout << "FAIL: CodeMap mark_code interior flag missing (1)\n";
            codemap_pass = false;
        }
        if (map[0x1002] != CodeMap::FLAG_CODE_INTERIOR) {
            std::cout << "FAIL: CodeMap mark_code interior flag missing (2)\n";
            codemap_pass = false;
        }

        // Test 2: Mark Data
        map.mark_data(0x2000, 1, false, true); // Read
        if (map[0x2000] != CodeMap::FLAG_DATA_READ) {
             std::cout << "FAIL: CodeMap mark_data read flag missing\n";
             codemap_pass = false;
        }
        map.mark_data(0x2000, 1, true, true); // Write
        if (map[0x2000] != (CodeMap::FLAG_DATA_READ | CodeMap::FLAG_DATA_WRITE)) {
             std::cout << "FAIL: CodeMap mark_data write flag missing or read flag lost\n";
             codemap_pass = false;
        }

        // Test 3: Unmark Code
        map.mark_code(0x1000, 3, false);
        if (map[0x1000] != 0 || map[0x1001] != 0 || map[0x1002] != 0) {
            std::cout << "FAIL: CodeMap unmark_code failed\n";
            codemap_pass = false;
        }

        // Test 4: Orphan cleanup
        // Setup: Instruction at 0x3000 len 4
        map.mark_code(0x3000, 4, true);
        // Overwrite with instruction at 0x3000 len 2
        map.mark_code(0x3000, 2, true);
        
        if (map[0x3000] != CodeMap::FLAG_CODE_START) { std::cout << "FAIL: Orphan cleanup check 0\n"; codemap_pass = false; }
        if (map[0x3001] != CodeMap::FLAG_CODE_INTERIOR) { std::cout << "FAIL: Orphan cleanup check 1\n"; codemap_pass = false; }
        if (map[0x3002] != 0) {
             std::cout << "FAIL: CodeMap orphan cleanup failed at 0x3002 (got " << (int)map[0x3002] << ")\n";
             codemap_pass = false;
        }
        if (map[0x3003] != 0) {
             std::cout << "FAIL: CodeMap orphan cleanup failed at 0x3003 (got " << (int)map[0x3003] << ")\n";
             codemap_pass = false;
        }

        if (codemap_pass) tests_passed++;
        else tests_failed++;
    }

    // --- Parse DS Tests ---
    {
        bool ds_pass = true;
        // Test 1: DS count only
        auto line = analyzer.parse_ds(0x4000, 100);
        if (line.mnemonic != "DS") {
            std::cout << "FAIL: Parse DS mnemonic mismatch\n";
            ds_pass = false;
        } else if (line.operands.size() != 1 || line.operands[0].num_val != 100) {
            std::cout << "FAIL: Parse DS operand mismatch (count)\n";
            ds_pass = false;
        }

        // Test 2: DS count and fill byte
        auto line2 = analyzer.parse_ds(0x4100, 50, 0xAA);
        if (line2.mnemonic != "DS") {
            std::cout << "FAIL: Parse DS (fill) mnemonic mismatch\n";
            ds_pass = false;
        } else if (line2.operands.size() != 2 || line2.operands[0].num_val != 50 || line2.operands[1].num_val != 0xAA) {
            std::cout << "FAIL: Parse DS (fill) operand mismatch\n";
            ds_pass = false;
        }
        
        if (ds_pass) tests_passed++;
        else tests_failed++;
    }

    // --- Register Bit/Shift/Rotate ---
    {
        check_inst(analyzer, memory, {0xCB, 0x00}, "RLC", {"B"});
        check_inst(analyzer, memory, {0xCB, 0x09}, "RRC", {"C"});
        check_inst(analyzer, memory, {0xCB, 0x12}, "RL", {"D"});
        check_inst(analyzer, memory, {0xCB, 0x1B}, "RR", {"E"});
        check_inst(analyzer, memory, {0xCB, 0x24}, "SLA", {"H"});
        check_inst(analyzer, memory, {0xCB, 0x2D}, "SRA", {"L"});
        check_inst(analyzer, memory, {0xCB, 0x3F}, "SRL", {"A"});
    }

    // --- Indexed Bit/Shift/Rotate ---
    {
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x06}, "RLC", {"(IX+0)"});
        check_inst(analyzer, memory, {0xFD, 0xCB, 0x00, 0x0E}, "RRC", {"(IY+0)"});
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x16}, "RL", {"(IX+0)"});
        check_inst(analyzer, memory, {0xFD, 0xCB, 0x00, 0x1E}, "RR", {"(IY+0)"});
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x26}, "SLA", {"(IX+0)"});
        check_inst(analyzer, memory, {0xFD, 0xCB, 0x00, 0x2E}, "SRA", {"(IY+0)"});
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x3E}, "SRL", {"(IX+0)"});
    }

    // --- Undocumented Indexed Bit/Shift/Rotate (Copy to Register) ---
    {
        // RLC (IX+0), B
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x00}, "RLC", {"(IX+0)", "B"});
        // SET 1, (IY+5), C
        check_inst(analyzer, memory, {0xFD, 0xCB, 0x05, 0xC9}, "SET", {"0x1", "(IY+5)", "C"});
        // SLA (IX+0), IXH
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x24}, "SLA", {"(IX+0)", "IXH"});
    }

    // --- Missing Basic Instructions ---
    {
        // LD A, (DE)
        check_inst(analyzer, memory, {0x1A}, "LD", {"A", "(DE)"});
        // LD (BC), A
        check_inst(analyzer, memory, {0x02}, "LD", {"(BC)", "A"});
        // LD (DE), A
        check_inst(analyzer, memory, {0x12}, "LD", {"(DE)", "A"});
        
        // INC/DEC Indexed
        check_inst(analyzer, memory, {0xDD, 0x34, 0x05}, "INC", {"(IX+5)"});
        check_inst(analyzer, memory, {0xFD, 0x35, 0xFA}, "DEC", {"(IY-6)"});
        
        // LD (Indexed), n
        check_inst(analyzer, memory, {0xDD, 0x36, 0x00, 0x55}, "LD", {"(IX+0)", "0x55"});
        
        // ADD HL, SP
        check_inst(analyzer, memory, {0x39}, "ADD", {"HL", "SP"});
    }

    // --- Prefix Quirks ---
    {
        // Multiple prefixes - last one wins
        // FD DD 21 00 00 -> LD IX, 0000
        check_inst(analyzer, memory, {0xFD, 0xDD, 0x21, 0x00, 0x00}, "LD", {"IX", "0x0"});
        
        // Redundant prefixes
        // DD DD 21 00 00 -> LD IX, 0000
        check_inst(analyzer, memory, {0xDD, 0xDD, 0x21, 0x00, 0x00}, "LD", {"IX", "0x0"});

        // Prefix before ED instruction (should be ignored/reset)
        // DD ED 4A -> ADC HL, BC (not ADC IX, BC)
        check_inst(analyzer, memory, {0xDD, 0xED, 0x4A}, "ADC", {"HL", "BC"});
    }

    // --- Undocumented IO ---
    {
        // IN (C) - ED 70
        check_inst(analyzer, memory, {0xED, 0x70}, "IN", {"(C)"});
        
        // OUT (C), 0 - ED 71
        check_inst(analyzer, memory, {0xED, 0x71}, "OUT", {"(C)", "0x0"});
    }

    // --- ED Instruction Aliases ---
    {
        // IM aliases
        check_inst(analyzer, memory, {0xED, 0x4E}, "IM", {"0x0"});
        check_inst(analyzer, memory, {0xED, 0x76}, "IM", {"0x1"});
        check_inst(analyzer, memory, {0xED, 0x7E}, "IM", {"0x2"});

        // NEG aliases
        check_inst(analyzer, memory, {0xED, 0x4C}, "NEG", {});
        
        // RETN aliases
        check_inst(analyzer, memory, {0xED, 0x55}, "RETN", {});
    }

    // --- Ignored Prefixes ---
    {
        // RST 00 with IX prefix -> RST 00
        check_inst(analyzer, memory, {0xDD, 0xC7}, "RST", {"0x0"});
        
        // DI with IY prefix -> DI
        check_inst(analyzer, memory, {0xFD, 0xF3}, "DI", {});
        
        // EI with IX prefix -> EI
        check_inst(analyzer, memory, {0xDD, 0xFB}, "EI", {});
        
        // HALT with IY prefix -> HALT
        check_inst(analyzer, memory, {0xFD, 0x76}, "HALT", {});
        
        // EX AF, AF' with prefix -> EX AF, AF'
        check_inst(analyzer, memory, {0xDD, 0x08}, "EX AF, AF'", {});
        
        // EXX with prefix -> EXX
        check_inst(analyzer, memory, {0xFD, 0xD9}, "EXX", {});
        
        // ALU ops that don't use HL/IX/IY (e.g. ADD A, B) with prefix
        // DD 80 -> ADD A, B (prefix ignored)
        check_inst(analyzer, memory, {0xDD, 0x80}, "ADD", {"A", "B"});

        // EX DE, HL with prefix -> EX DE, HL (not EX DE, IX)
        check_inst(analyzer, memory, {0xDD, 0xEB}, "EX", {"DE", "HL"});
    }

    // --- Indexed Load/Store ---
    {
        // LD r, (IX+d)
        check_inst(analyzer, memory, {0xDD, 0x46, 0x01}, "LD", {"B", "(IX+1)"});
        check_inst(analyzer, memory, {0xDD, 0x4E, 0x02}, "LD", {"C", "(IX+2)"});
        check_inst(analyzer, memory, {0xDD, 0x56, 0x03}, "LD", {"D", "(IX+3)"});
        check_inst(analyzer, memory, {0xDD, 0x5E, 0x04}, "LD", {"E", "(IX+4)"});
        check_inst(analyzer, memory, {0xDD, 0x66, 0x05}, "LD", {"H", "(IX+5)"});
        check_inst(analyzer, memory, {0xDD, 0x6E, 0x06}, "LD", {"L", "(IX+6)"});
        // LD A, (IX+d) is already tested

        // LD (IX+d), r
        check_inst(analyzer, memory, {0xDD, 0x70, 0x01}, "LD", {"(IX+1)", "B"});
        check_inst(analyzer, memory, {0xDD, 0x71, 0x02}, "LD", {"(IX+2)", "C"});
        check_inst(analyzer, memory, {0xDD, 0x72, 0x03}, "LD", {"(IX+3)", "D"});
        check_inst(analyzer, memory, {0xDD, 0x73, 0x04}, "LD", {"(IX+4)", "E"});
        check_inst(analyzer, memory, {0xDD, 0x74, 0x05}, "LD", {"(IX+5)", "H"});
        check_inst(analyzer, memory, {0xDD, 0x75, 0x06}, "LD", {"(IX+6)", "L"});
        check_inst(analyzer, memory, {0xDD, 0x77, 0x07}, "LD", {"(IX+7)", "A"});

        // IY examples
        check_inst(analyzer, memory, {0xFD, 0x46, 0x10}, "LD", {"B", "(IY+16)"});
        check_inst(analyzer, memory, {0xFD, 0x70, 0x20}, "LD", {"(IY+32)", "B"});
    }

    // --- More Undocumented 8-bit Index Operations ---
    {
        // LD IXH/IXL, n
        check_inst(analyzer, memory, {0xDD, 0x26, 0x10}, "LD", {"IXH", "0x10"});
        check_inst(analyzer, memory, {0xDD, 0x2E, 0x20}, "LD", {"IXL", "0x20"});
        check_inst(analyzer, memory, {0xFD, 0x26, 0x30}, "LD", {"IYH", "0x30"});
        check_inst(analyzer, memory, {0xFD, 0x2E, 0x40}, "LD", {"IYL", "0x40"});

        // LD r, IXH/IXL (more combinations)
        check_inst(analyzer, memory, {0xDD, 0x4D}, "LD", {"C", "IXL"});
        check_inst(analyzer, memory, {0xFD, 0x54}, "LD", {"D", "IYH"});
        check_inst(analyzer, memory, {0xFD, 0x5D}, "LD", {"E", "IYL"});
        check_inst(analyzer, memory, {0xDD, 0x7C}, "LD", {"A", "IXH"});

        // LD IXH/IXL, r
        check_inst(analyzer, memory, {0xDD, 0x60}, "LD", {"IXH", "B"});
        check_inst(analyzer, memory, {0xDD, 0x69}, "LD", {"IXL", "C"});
        check_inst(analyzer, memory, {0xFD, 0x62}, "LD", {"IYH", "D"});
        check_inst(analyzer, memory, {0xFD, 0x6B}, "LD", {"IYL", "E"});
        check_inst(analyzer, memory, {0xDD, 0x67}, "LD", {"IXH", "A"});

        // LD IXH, IXL etc.
        check_inst(analyzer, memory, {0xDD, 0x65}, "LD", {"IXH", "IXL"});
        check_inst(analyzer, memory, {0xDD, 0x6C}, "LD", {"IXL", "IXH"});

        // ALU with IXH/IXL
        check_inst(analyzer, memory, {0xDD, 0x8D}, "ADC", {"A", "IXL"});
        check_inst(analyzer, memory, {0xFD, 0x94}, "SUB", {"IYH"});
        check_inst(analyzer, memory, {0xFD, 0x9D}, "SBC", {"A", "IYL"});
        check_inst(analyzer, memory, {0xDD, 0xA4}, "AND", {"IXH"});
        check_inst(analyzer, memory, {0xDD, 0xAD}, "XOR", {"IXL"});
        check_inst(analyzer, memory, {0xFD, 0xB4}, "OR", {"IYH"});
        check_inst(analyzer, memory, {0xFD, 0xBD}, "CP", {"IYL"});
        
        // INC/DEC IXH/IXL (rest of them)
        check_inst(analyzer, memory, {0xDD, 0x2C}, "INC", {"IXL"});
        check_inst(analyzer, memory, {0xFD, 0x25}, "DEC", {"IYH"});
        check_inst(analyzer, memory, {0xFD, 0x2D}, "DEC", {"IYL"});
    }

    // --- Parse Data Directives (DB, DW, DZ) ---
    {
        // DB
        memory.set_data(0x6000, {0x10, 0x20, 0x30});
        auto line_db = analyzer.parse_db(0x6000, 3);
        
        if (line_db.mnemonic != "DB") { std::cout << "FAIL: DB mnemonic\n"; tests_failed++; }
        else if (line_db.operands.size() != 3) { std::cout << "FAIL: DB operands count\n"; tests_failed++; }
        else if (line_db.operands[0].num_val != 0x10) { std::cout << "FAIL: DB op0\n"; tests_failed++; }
        else tests_passed++;

        // DW
        memory.set_data(0x6100, {0x34, 0x12}); // 0x1234
        auto line_dw = analyzer.parse_dw(0x6100, 1);
        if (line_dw.mnemonic != "DW") { std::cout << "FAIL: DW mnemonic\n"; tests_failed++; }
        else if (line_dw.operands.size() != 1) { std::cout << "FAIL: DW operands count\n"; tests_failed++; }
        else if (line_dw.operands[0].num_val != 0x1234) { std::cout << "FAIL: DW op0 value\n"; tests_failed++; }
        else tests_passed++;

        // DZ
        memory.set_data(0x6200, {'H', 'e', 'l', 'l', 'o', 0x00});
        auto line_dz = analyzer.parse_dz(0x6200);
        if (line_dz.mnemonic != "DZ") { std::cout << "FAIL: DZ mnemonic\n"; tests_failed++; }
        else if (line_dz.operands.size() != 1) { std::cout << "FAIL: DZ operands count\n"; tests_failed++; }
        else if (line_dz.operands[0].s_val != "Hello") { std::cout << "FAIL: DZ string value\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- 16-bit INC/DEC ---
    {
        check_inst(analyzer, memory, {0x03}, "INC", {"BC"});
        check_inst(analyzer, memory, {0x13}, "INC", {"DE"});
        check_inst(analyzer, memory, {0x23}, "INC", {"HL"});
        check_inst(analyzer, memory, {0x33}, "INC", {"SP"});
        check_inst(analyzer, memory, {0xDD, 0x23}, "INC", {"IX"});
        check_inst(analyzer, memory, {0xFD, 0x23}, "INC", {"IY"});

        check_inst(analyzer, memory, {0x0B}, "DEC", {"BC"});
        check_inst(analyzer, memory, {0x1B}, "DEC", {"DE"});
        check_inst(analyzer, memory, {0x2B}, "DEC", {"HL"});
        check_inst(analyzer, memory, {0x3B}, "DEC", {"SP"});
        check_inst(analyzer, memory, {0xDD, 0x2B}, "DEC", {"IX"});
        check_inst(analyzer, memory, {0xFD, 0x2B}, "DEC", {"IY"});
    }

    // --- ALU Immediate ---
    {
        check_inst(analyzer, memory, {0xCE, 0x10}, "ADC", {"A", "0x10"});
        check_inst(analyzer, memory, {0xD6, 0x20}, "SUB", {"0x20"});
        check_inst(analyzer, memory, {0xDE, 0x30}, "SBC", {"A", "0x30"});
        check_inst(analyzer, memory, {0xE6, 0x40}, "AND", {"0x40"});
        check_inst(analyzer, memory, {0xEE, 0x50}, "XOR", {"0x50"});
        check_inst(analyzer, memory, {0xF6, 0x60}, "OR", {"0x60"});
        check_inst(analyzer, memory, {0xFE, 0x70}, "CP", {"0x70"});
    }

    // --- 8-bit INC/DEC Registers ---
    {
        check_inst(analyzer, memory, {0x04}, "INC", {"B"});
        check_inst(analyzer, memory, {0x05}, "DEC", {"B"});
        check_inst(analyzer, memory, {0x0C}, "INC", {"C"});
        check_inst(analyzer, memory, {0x0D}, "DEC", {"C"});
        check_inst(analyzer, memory, {0x14}, "INC", {"D"});
        check_inst(analyzer, memory, {0x15}, "DEC", {"D"});
        check_inst(analyzer, memory, {0x1C}, "INC", {"E"});
        check_inst(analyzer, memory, {0x1D}, "DEC", {"E"});
        check_inst(analyzer, memory, {0x24}, "INC", {"H"});
        check_inst(analyzer, memory, {0x25}, "DEC", {"H"});
        check_inst(analyzer, memory, {0x2C}, "INC", {"L"});
        check_inst(analyzer, memory, {0x2D}, "DEC", {"L"});
    }

    // --- 8-bit Load Immediate ---
    {
        check_inst(analyzer, memory, {0x0E, 0x11}, "LD", {"C", "0x11"});
        check_inst(analyzer, memory, {0x16, 0x22}, "LD", {"D", "0x22"});
        check_inst(analyzer, memory, {0x1E, 0x33}, "LD", {"E", "0x33"});
        check_inst(analyzer, memory, {0x26, 0x44}, "LD", {"H", "0x44"});
        check_inst(analyzer, memory, {0x2E, 0x55}, "LD", {"L", "0x55"});
    }

    // --- 8-bit Register-to-Register Loads ---
    {
        check_inst(analyzer, memory, {0x41}, "LD", {"B", "C"});
        check_inst(analyzer, memory, {0x48}, "LD", {"C", "B"});
        check_inst(analyzer, memory, {0x53}, "LD", {"D", "E"});
        check_inst(analyzer, memory, {0x5A}, "LD", {"E", "D"});
        check_inst(analyzer, memory, {0x65}, "LD", {"H", "L"});
        check_inst(analyzer, memory, {0x6C}, "LD", {"L", "H"});
        check_inst(analyzer, memory, {0x7C}, "LD", {"A", "H"});
        check_inst(analyzer, memory, {0x67}, "LD", {"H", "A"});
        
        // LD r, r (NOP equivalent but valid LD)
        check_inst(analyzer, memory, {0x7F}, "LD", {"A", "A"});
        check_inst(analyzer, memory, {0x40}, "LD", {"B", "B"});
        check_inst(analyzer, memory, {0x49}, "LD", {"C", "C"});
    }

    // --- Additional IY and Immediate Tests ---
    {
        // LD A, n
        check_inst(analyzer, memory, {0x3E, 0x42}, "LD", {"A", "0x42"});

        // LD (IY+d), n
        check_inst(analyzer, memory, {0xFD, 0x36, 0x05, 0x99}, "LD", {"(IY+5)", "0x99"});
        
        // More IY Load/Store
        check_inst(analyzer, memory, {0xFD, 0x4E, 0x01}, "LD", {"C", "(IY+1)"});
        check_inst(analyzer, memory, {0xFD, 0x56, 0x02}, "LD", {"D", "(IY+2)"});
        check_inst(analyzer, memory, {0xFD, 0x5E, 0x03}, "LD", {"E", "(IY+3)"});
        check_inst(analyzer, memory, {0xFD, 0x66, 0x04}, "LD", {"H", "(IY+4)"});
        check_inst(analyzer, memory, {0xFD, 0x6E, 0x05}, "LD", {"L", "(IY+5)"});
        check_inst(analyzer, memory, {0xFD, 0x7E, 0x06}, "LD", {"A", "(IY+6)"});

        check_inst(analyzer, memory, {0xFD, 0x71, 0x01}, "LD", {"(IY+1)", "C"});
        check_inst(analyzer, memory, {0xFD, 0x72, 0x02}, "LD", {"(IY+2)", "D"});
        check_inst(analyzer, memory, {0xFD, 0x73, 0x03}, "LD", {"(IY+3)", "E"});
        check_inst(analyzer, memory, {0xFD, 0x74, 0x04}, "LD", {"(IY+4)", "H"});
        check_inst(analyzer, memory, {0xFD, 0x75, 0x05}, "LD", {"(IY+5)", "L"});
        check_inst(analyzer, memory, {0xFD, 0x77, 0x06}, "LD", {"(IY+6)", "A"});
    }

    // --- ED 16-bit Load (Slow HL) & Aliases ---
    {
        // LD HL, (nn) - ED 6B
        check_inst(analyzer, memory, {0xED, 0x6B, 0x34, 0x12}, "LD", {"HL", "(0x1234)"});
        // LD (nn), HL - ED 63
        check_inst(analyzer, memory, {0xED, 0x63, 0x34, 0x12}, "LD", {"(0x1234)", "HL"});

        // More IM Aliases
        check_inst(analyzer, memory, {0xED, 0x66}, "IM", {"0x0"});
        check_inst(analyzer, memory, {0xED, 0x6E}, "IM", {"0x0"});

        // More NEG Aliases
        check_inst(analyzer, memory, {0xED, 0x54}, "NEG", {});
        check_inst(analyzer, memory, {0xED, 0x7C}, "NEG", {});

        // More RETN Aliases
        check_inst(analyzer, memory, {0xED, 0x6D}, "RETN", {});
        check_inst(analyzer, memory, {0xED, 0x75}, "RETN", {});
    }

    // --- Labels Integration ---
    {
        labels.add_label(0x8000, "ENTRY_POINT");
        memory.set_data(0x8000, {0x3E, 0x01}); // LD A, 1
        auto line = analyzer.parse_instruction(0x8000);
        if (line.label != "ENTRY_POINT") { std::cout << "FAIL: Label integration (instruction label)\n"; tests_failed++; }
        else tests_passed++;

        labels.add_label(0x9000, "JUMP_TARGET");
        memory.set_data(0x8005, {0xC3, 0x00, 0x90}); // JP 0x9000
        auto line3 = analyzer.parse_instruction(0x8005);
        if (line3.operands.size() > 0 && line3.operands[0].label != "JUMP_TARGET") {
             std::cout << "FAIL: Label integration (jump target label)\n";
             tests_failed++;
        } else tests_passed++;

        labels.add_label(0x800A, "LOOP_START");
        memory.set_data(0x8008, {0x10, 0x00}); // DJNZ +0 (to 0x800A)
        auto line4 = analyzer.parse_instruction(0x8008);
        if (line4.operands.size() > 0 && line4.operands[0].label != "LOOP_START") {
             std::cout << "FAIL: Label integration (DJNZ target label)\n";
             tests_failed++;
        } else tests_passed++;
    }

    // --- More Labels Integration ---
    {
        labels.add_label(0xA000, "SUBROUTINE");
        memory.set_data(0x8100, {0xCD, 0x00, 0xA0}); // CALL 0xA000
        auto line_call = analyzer.parse_instruction(0x8100);
        if (line_call.operands.size() > 0 && line_call.operands[0].label != "SUBROUTINE") {
             std::cout << "FAIL: Label integration (CALL target label)\n";
             tests_failed++;
        } else tests_passed++;

        labels.add_label(0x8105, "NEAR_TARGET");
        memory.set_data(0x8103, {0x18, 0x00}); // JR +0 (to 0x8105)
        auto line_jr = analyzer.parse_instruction(0x8103);
        if (line_jr.operands.size() > 0 && line_jr.operands[0].label != "NEAR_TARGET") {
             std::cout << "FAIL: Label integration (JR target label)\n";
             tests_failed++;
        } else tests_passed++;
    }

    // --- Wrapping Instruction ---
    {
        // LD A, 0x55 at 0xFFFF
        // 0xFFFF: 3E
        // 0x0000: 55
        memory.poke(0xFFFF, 0x3E);
        memory.poke(0x0000, 0x55);
        
        auto line = analyzer.parse_instruction(0xFFFF);
        if (line.mnemonic != "LD") { std::cout << "FAIL: Wrapping instruction mnemonic\n"; tests_failed++; }
        else if (line.operands.size() != 2) { std::cout << "FAIL: Wrapping instruction operands\n"; tests_failed++; }
        else if (line.operands[1].num_val != 0x55) { std::cout << "FAIL: Wrapping instruction operand value\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Instruction Bytes & Ticks ---
    {
        // NOP: 1 byte, 4 ticks
        memory.set_data(0x7000, {0x00});
        auto line = analyzer.parse_instruction(0x7000);
        if (line.bytes.size() != 1 || line.bytes[0] != 0x00) { std::cout << "FAIL: NOP bytes\n"; tests_failed++; }
        else if (line.ticks != 4) { std::cout << "FAIL: NOP ticks\n"; tests_failed++; }
        else tests_passed++;

        // LD BC, nn: 3 bytes, 10 ticks
        memory.set_data(0x7001, {0x01, 0x34, 0x12});
        line = analyzer.parse_instruction(0x7001);
        if (line.bytes.size() != 3 || line.bytes[0] != 0x01 || line.bytes[1] != 0x34 || line.bytes[2] != 0x12) { 
            std::cout << "FAIL: LD BC, nn bytes\n"; tests_failed++; 
        }
        else if (line.ticks != 10) { std::cout << "FAIL: LD BC, nn ticks\n"; tests_failed++; }
        else tests_passed++;

        // JR NZ, d: 2 bytes, 7/12 ticks
        memory.set_data(0x7004, {0x20, 0xFE});
        line = analyzer.parse_instruction(0x7004);
        if (line.bytes.size() != 2) { std::cout << "FAIL: JR NZ bytes\n"; tests_failed++; }
        else if (line.ticks != 7 || line.ticks_alt != 12) { std::cout << "FAIL: JR NZ ticks\n"; tests_failed++; }
        else tests_passed++;

        // IX instruction: LD A, (IX+d): 3 bytes, 19 ticks
        memory.set_data(0x7006, {0xDD, 0x7E, 0x05});
        line = analyzer.parse_instruction(0x7006);
        if (line.bytes.size() != 3) { std::cout << "FAIL: LD A, (IX+d) bytes\n"; tests_failed++; }
        else if (line.ticks != 19) { std::cout << "FAIL: LD A, (IX+d) ticks\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Instruction Types ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;
        
        // LD -> LOAD
        memory.set_data(0x7010, {0x01, 0x34, 0x12}); // LD BC, 1234
        auto line = analyzer.parse_instruction(0x7010);
        if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LD type\n"; tests_failed++; }
        else tests_passed++;

        // ADD -> ALU
        memory.set_data(0x7013, {0x80}); // ADD A, B
        line = analyzer.parse_instruction(0x7013);
        if (!(line.type & Type::ALU)) { std::cout << "FAIL: ADD type\n"; tests_failed++; }
        else tests_passed++;

        // JP -> JUMP
        memory.set_data(0x7014, {0xC3, 0x00, 0x00});
        line = analyzer.parse_instruction(0x7014);
        if (!(line.type & Type::JUMP)) { std::cout << "FAIL: JP type\n"; tests_failed++; }
        else tests_passed++;
        
        // CALL -> CALL | STACK
        memory.set_data(0x7017, {0xCD, 0x00, 0x00});
        line = analyzer.parse_instruction(0x7017);
        if (!(line.type & Type::CALL)) { std::cout << "FAIL: CALL type (CALL)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: CALL type (STACK)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Variable Timing Instructions ---
    {
        // RET cc: 5/11 ticks
        memory.set_data(0x7100, {0xC0}); // RET NZ
        auto line = analyzer.parse_instruction(0x7100);
        if (line.ticks != 5 || line.ticks_alt != 11) { 
            std::cout << "FAIL: RET NZ ticks (expected 5/11, got " << line.ticks << "/" << line.ticks_alt << ")\n"; 
            tests_failed++; 
        } else tests_passed++;

        // CALL cc: 10/17 ticks
        memory.set_data(0x7101, {0xCC, 0x00, 0x00}); // CALL Z, 0000
        line = analyzer.parse_instruction(0x7101);
        if (line.ticks != 10 || line.ticks_alt != 17) { 
            std::cout << "FAIL: CALL Z ticks (expected 10/17, got " << line.ticks << "/" << line.ticks_alt << ")\n"; 
            tests_failed++; 
        } else tests_passed++;

        // JR cc: 7/12 ticks
        memory.set_data(0x7104, {0x38, 0xFE}); // JR C, -2
        line = analyzer.parse_instruction(0x7104);
        if (line.ticks != 7 || line.ticks_alt != 12) { 
            std::cout << "FAIL: JR C ticks (expected 7/12, got " << line.ticks << "/" << line.ticks_alt << ")\n"; 
            tests_failed++; 
        } else tests_passed++;

        // DJNZ: 8/13 ticks
        memory.set_data(0x7106, {0x10, 0xFE}); // DJNZ -2
        line = analyzer.parse_instruction(0x7106);
        if (line.ticks != 8 || line.ticks_alt != 13) { 
            std::cout << "FAIL: DJNZ ticks (expected 8/13, got " << line.ticks << "/" << line.ticks_alt << ")\n"; 
            tests_failed++; 
        } else tests_passed++;

        // Block instruction (LDIR): 16/21 ticks
        memory.set_data(0x7108, {0xED, 0xB0}); // LDIR
        line = analyzer.parse_instruction(0x7108);
        if (line.ticks != 16 || line.ticks_alt != 21) { 
            std::cout << "FAIL: LDIR ticks (expected 16/21, got " << line.ticks << "/" << line.ticks_alt << ")\n"; 
            tests_failed++; 
        } else tests_passed++;
    }

    // --- Instruction Types (Extended) ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;

        // RST -> CALL | STACK
        memory.set_data(0x7200, {0xC7}); // RST 00
        auto line = analyzer.parse_instruction(0x7200);
        if (!(line.type & Type::CALL)) { std::cout << "FAIL: RST type (CALL)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: RST type (STACK)\n"; tests_failed++; }
        else tests_passed++;

        // RET -> RETURN | STACK
        memory.set_data(0x7201, {0xC9}); // RET
        line = analyzer.parse_instruction(0x7201);
        if (!(line.type & Type::RETURN)) { std::cout << "FAIL: RET type (RETURN)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: RET type (STACK)\n"; tests_failed++; }
        else tests_passed++;

        // PUSH -> STACK | LOAD
        memory.set_data(0x7202, {0xC5}); // PUSH BC
        line = analyzer.parse_instruction(0x7202);
        if (!(line.type & Type::STACK)) { std::cout << "FAIL: PUSH type (STACK)\n"; tests_failed++; }
        else if (!(line.type & Type::LOAD)) { std::cout << "FAIL: PUSH type (LOAD)\n"; tests_failed++; }
        else tests_passed++;

        // IN -> IO
        memory.set_data(0x7203, {0xDB, 0x00}); // IN A, (0)
        line = analyzer.parse_instruction(0x7203);
        if (!(line.type & Type::IO)) { std::cout << "FAIL: IN type (IO)\n"; tests_failed++; }
        else tests_passed++;

        // DI -> CPU_CONTROL
        memory.set_data(0x7205, {0xF3}); // DI
        line = analyzer.parse_instruction(0x7205);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: DI type (CPU_CONTROL)\n"; tests_failed++; }
        else tests_passed++;
        
        // Unknown ED -> CPU_CONTROL
        memory.set_data(0x7206, {0xED, 0xFF}); // Unknown ED
        line = analyzer.parse_instruction(0x7206);
        if (line.mnemonic != "NOP") { std::cout << "FAIL: Unknown ED mnemonic\n"; tests_failed++; }
        else if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: Unknown ED type\n"; tests_failed++; }
        else if (line.ticks != 8) { std::cout << "FAIL: Unknown ED ticks\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Bit Instruction Ticks ---
    {
        // BIT 0, A: 8 ticks
        memory.set_data(0x7400, {0xCB, 0x47});
        auto line = analyzer.parse_instruction(0x7400);
        if (line.ticks != 8) { std::cout << "FAIL: BIT 0, A ticks\n"; tests_failed++; }
        else tests_passed++;

        // BIT 0, (HL): 12 ticks
        memory.set_data(0x7402, {0xCB, 0x46});
        line = analyzer.parse_instruction(0x7402);
        if (line.ticks != 12) { std::cout << "FAIL: BIT 0, (HL) ticks\n"; tests_failed++; }
        else tests_passed++;

        // SET 0, (HL): 15 ticks
        memory.set_data(0x7404, {0xCB, 0xC6});
        line = analyzer.parse_instruction(0x7404);
        if (line.ticks != 15) { std::cout << "FAIL: SET 0, (HL) ticks\n"; tests_failed++; }
        else tests_passed++;
        
        // BIT 0, (IX+d): 20 ticks
        memory.set_data(0x7406, {0xDD, 0xCB, 0x00, 0x46});
        line = analyzer.parse_instruction(0x7406);
        if (line.ticks != 20) { std::cout << "FAIL: BIT 0, (IX+d) ticks\n"; tests_failed++; }
        else tests_passed++;

        // SET 0, (IX+d): 23 ticks
        memory.set_data(0x740A, {0xDD, 0xCB, 0x00, 0xC6});
        line = analyzer.parse_instruction(0x740A);
        if (line.ticks != 23) { std::cout << "FAIL: SET 0, (IX+d) ticks\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Instruction Types (Block & Misc) ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;

        // LDI -> BLOCK | LOAD
        memory.set_data(0x7500, {0xED, 0xA0});
        auto line = analyzer.parse_instruction(0x7500);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: LDI type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LDI type (LOAD)\n"; tests_failed++; }
        else tests_passed++;

        // CPI -> BLOCK | ALU
        memory.set_data(0x7502, {0xED, 0xA1});
        line = analyzer.parse_instruction(0x7502);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: CPI type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: CPI type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // INI -> BLOCK | IO
        memory.set_data(0x7504, {0xED, 0xA2});
        line = analyzer.parse_instruction(0x7504);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: INI type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::IO)) { std::cout << "FAIL: INI type (IO)\n"; tests_failed++; }
        else tests_passed++;

        // IM 1 -> CPU_CONTROL
        memory.set_data(0x7506, {0xED, 0x56});
        line = analyzer.parse_instruction(0x7506);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: IM 1 type (CPU_CONTROL)\n"; tests_failed++; }
        else tests_passed++;

        // EX DE, HL -> EXCHANGE
        memory.set_data(0x7508, {0xEB});
        line = analyzer.parse_instruction(0x7508);
        if (!(line.type & Type::EXCHANGE)) { std::cout << "FAIL: EX DE, HL type (EXCHANGE)\n"; tests_failed++; }
        else tests_passed++;

        // EX (SP), HL -> EXCHANGE | STACK
        memory.set_data(0x7509, {0xE3});
        line = analyzer.parse_instruction(0x7509);
        if (!(line.type & Type::EXCHANGE)) { std::cout << "FAIL: EX (SP), HL type (EXCHANGE)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: EX (SP), HL type (STACK)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Negative Index Displacements ---
    {
        // LD A, (IX-16)
        check_inst(analyzer, memory, {0xDD, 0x7E, 0xF0}, "LD", {"A", "(IX-16)"});
        // LD (IX-2), 0
        check_inst(analyzer, memory, {0xDD, 0x36, 0xFE, 0x00}, "LD", {"(IX-2)", "0x0"});
        // ADD A, (IX-128)
        check_inst(analyzer, memory, {0xDD, 0x86, 0x80}, "ADD", {"A", "(IX-128)"});
        
        // BIT 0, (IX-1)
        check_inst(analyzer, memory, {0xDD, 0xCB, 0xFF, 0x46}, "BIT", {"0x0", "(IX-1)"});
        // RES 0, (IX-2)
        check_inst(analyzer, memory, {0xDD, 0xCB, 0xFE, 0x86}, "RES", {"0x0", "(IX-2)"});
        // RLC (IX-3)
        check_inst(analyzer, memory, {0xDD, 0xCB, 0xFD, 0x06}, "RLC", {"(IX-3)"});
        
        // Undocumented: RLC (IX-3), B
        check_inst(analyzer, memory, {0xDD, 0xCB, 0xFD, 0x00}, "RLC", {"(IX-3)", "B"});
    }

    // --- Invalid ED Opcodes ---
    {
        // ED 00 -> NOP (ED, 00)
        check_inst(analyzer, memory, {0xED, 0x00}, "NOP", {"0xED", "0x0"});
        // ED 01 -> NOP (ED, 01)
        check_inst(analyzer, memory, {0xED, 0x01}, "NOP", {"0xED", "0x1"});
    }

    // --- More Undocumented Index Bit Ops ---
    {
        // BIT 0, (IX+0), B
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x40}, "BIT", {"0x0", "(IX+0)", "B"});
        // RES 0, (IX+0), B
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x80}, "RES", {"0x0", "(IX+0)", "B"});
        // SET 0, (IX+0), B
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0xC0}, "SET", {"0x0", "(IX+0)", "B"});
    }

    // --- Missing ED Aliases ---
    {
        // NEG aliases
        check_inst(analyzer, memory, {0xED, 0x5C}, "NEG", {});
        check_inst(analyzer, memory, {0xED, 0x64}, "NEG", {});
        check_inst(analyzer, memory, {0xED, 0x6C}, "NEG", {});
        check_inst(analyzer, memory, {0xED, 0x74}, "NEG", {});

        // RETN aliases
        check_inst(analyzer, memory, {0xED, 0x5D}, "RETN", {});
        check_inst(analyzer, memory, {0xED, 0x65}, "RETN", {});
        check_inst(analyzer, memory, {0xED, 0x7D}, "RETN", {});
    }

    // --- Instruction Types (Bit, Shift, Control, Misc) ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;

        // BIT 0, A -> BIT | ALU
        memory.set_data(0x7600, {0xCB, 0x47});
        auto line = analyzer.parse_instruction(0x7600);
        if (!(line.type & Type::BIT)) { std::cout << "FAIL: BIT type (BIT)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: BIT type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // RLC A -> SHIFT_ROTATE | ALU
        memory.set_data(0x7602, {0xCB, 0x07});
        line = analyzer.parse_instruction(0x7602);
        if (!(line.type & Type::SHIFT_ROTATE)) { std::cout << "FAIL: RLC type (SHIFT_ROTATE)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: RLC type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // HALT -> CPU_CONTROL
        memory.set_data(0x7604, {0x76});
        line = analyzer.parse_instruction(0x7604);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: HALT type (CPU_CONTROL)\n"; tests_failed++; }
        else tests_passed++;

        // EI -> CPU_CONTROL
        memory.set_data(0x7605, {0xFB});
        line = analyzer.parse_instruction(0x7605);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: EI type (CPU_CONTROL)\n"; tests_failed++; }
        else tests_passed++;

        // RETI -> RETURN | STACK
        memory.set_data(0x7606, {0xED, 0x4D});
        line = analyzer.parse_instruction(0x7606);
        if (!(line.type & Type::RETURN)) { std::cout << "FAIL: RETI type (RETURN)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: RETI type (STACK)\n"; tests_failed++; }
        else tests_passed++;

        // LD A, I -> LOAD
        memory.set_data(0x7608, {0xED, 0x57});
        line = analyzer.parse_instruction(0x7608);
        if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LD A, I type (LOAD)\n"; tests_failed++; }
        else tests_passed++;
        
        // IN (C) -> IO | ALU
        memory.set_data(0x760A, {0xED, 0x70});
        line = analyzer.parse_instruction(0x760A);
        if (!(line.type & Type::IO)) { std::cout << "FAIL: IN (C) type (IO)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: IN (C) type (ALU)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- More Instruction Types ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;

        // DJNZ -> JUMP | ALU
        memory.set_data(0x8100, {0x10, 0xFE});
        auto line = analyzer.parse_instruction(0x8100);
        if (!(line.type & Type::JUMP)) { std::cout << "FAIL: DJNZ type (JUMP)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: DJNZ type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // JP (HL) -> JUMP
        memory.set_data(0x8102, {0xE9});
        line = analyzer.parse_instruction(0x8102);
        if (!(line.type & Type::JUMP)) { std::cout << "FAIL: JP (HL) type (JUMP)\n"; tests_failed++; }
        else tests_passed++;
        
        // JP (IX) -> JUMP
        memory.set_data(0x8103, {0xDD, 0xE9});
        line = analyzer.parse_instruction(0x8103);
        if (!(line.type & Type::JUMP)) { std::cout << "FAIL: JP (IX) type (JUMP)\n"; tests_failed++; }
        else tests_passed++;

        // SLL A -> SHIFT_ROTATE | ALU
        memory.set_data(0x8105, {0xCB, 0x37});
        line = analyzer.parse_instruction(0x8105);
        if (!(line.type & Type::SHIFT_ROTATE)) { std::cout << "FAIL: SLL type (SHIFT_ROTATE)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: SLL type (ALU)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Instruction Types (Comprehensive) ---
    {
        using Type = Z80Analyzer<TestMemory>::CodeLine::Type;

        // IM 0 -> CPU_CONTROL
        memory.set_data(0x8200, {0xED, 0x46});
        auto line = analyzer.parse_instruction(0x8200);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: IM 0 type\n"; tests_failed++; }
        else tests_passed++;

        // IM 2 -> CPU_CONTROL
        memory.set_data(0x8202, {0xED, 0x5E});
        line = analyzer.parse_instruction(0x8202);
        if (!(line.type & Type::CPU_CONTROL)) { std::cout << "FAIL: IM 2 type\n"; tests_failed++; }
        else tests_passed++;

        // IN A, (n) -> IO
        memory.set_data(0x8204, {0xDB, 0x10});
        line = analyzer.parse_instruction(0x8204);
        if (!(line.type & Type::IO)) { std::cout << "FAIL: IN A, (n) type\n"; tests_failed++; }
        else tests_passed++;

        // OUT (n), A -> IO
        memory.set_data(0x8206, {0xD3, 0x20});
        line = analyzer.parse_instruction(0x8206);
        if (!(line.type & Type::IO)) { std::cout << "FAIL: OUT (n), A type\n"; tests_failed++; }
        else tests_passed++;

        // EX (SP), IX -> EXCHANGE | STACK
        memory.set_data(0x8208, {0xDD, 0xE3});
        line = analyzer.parse_instruction(0x8208);
        if (!(line.type & Type::EXCHANGE)) { std::cout << "FAIL: EX (SP), IX type (EXCHANGE)\n"; tests_failed++; }
        else if (!(line.type & Type::STACK)) { std::cout << "FAIL: EX (SP), IX type (STACK)\n"; tests_failed++; }
        else tests_passed++;

        // LD SP, HL -> LOAD
        memory.set_data(0x820A, {0xF9});
        line = analyzer.parse_instruction(0x820A);
        if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LD SP, HL type\n"; tests_failed++; }
        else tests_passed++;

        // ADD IX, BC -> ALU
        memory.set_data(0x820B, {0xDD, 0x09});
        line = analyzer.parse_instruction(0x820B);
        if (!(line.type & Type::ALU)) { std::cout << "FAIL: ADD IX, BC type\n"; tests_failed++; }
        else tests_passed++;

        // NEG -> ALU
        memory.set_data(0x820D, {0xED, 0x44});
        line = analyzer.parse_instruction(0x820D);
        if (!(line.type & Type::ALU)) { std::cout << "FAIL: NEG type\n"; tests_failed++; }
        else tests_passed++;

        // RRD -> SHIFT_ROTATE | ALU
        memory.set_data(0x820F, {0xED, 0x67});
        line = analyzer.parse_instruction(0x820F);
        if (!(line.type & Type::SHIFT_ROTATE)) { std::cout << "FAIL: RRD type (SHIFT_ROTATE)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: RRD type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // LDIR -> BLOCK | LOAD
        memory.set_data(0x8211, {0xED, 0xB0});
        line = analyzer.parse_instruction(0x8211);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: LDIR type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LDIR type (LOAD)\n"; tests_failed++; }
        else tests_passed++;

        // CPIR -> BLOCK | ALU
        memory.set_data(0x8213, {0xED, 0xB1});
        line = analyzer.parse_instruction(0x8213);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: CPIR type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::ALU)) { std::cout << "FAIL: CPIR type (ALU)\n"; tests_failed++; }
        else tests_passed++;

        // INIR -> BLOCK | IO
        memory.set_data(0x8215, {0xED, 0xB2});
        line = analyzer.parse_instruction(0x8215);
        if (!(line.type & Type::BLOCK)) { std::cout << "FAIL: INIR type (BLOCK)\n"; tests_failed++; }
        else if (!(line.type & Type::IO)) { std::cout << "FAIL: INIR type (IO)\n"; tests_failed++; }
        else tests_passed++;

        // JP (IY) -> JUMP
        memory.set_data(0x8217, {0xFD, 0xE9});
        line = analyzer.parse_instruction(0x8217);
        if (!(line.type & Type::JUMP)) { std::cout << "FAIL: JP (IY) type\n"; tests_failed++; }
        else tests_passed++;

        // LD SP, IX -> LOAD
        memory.set_data(0x8219, {0xDD, 0xF9});
        line = analyzer.parse_instruction(0x8219);
        if (!(line.type & Type::LOAD)) { std::cout << "FAIL: LD SP, IX type\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- CB Prefix Edge Cases ---
    {
        // CB FF -> SET 7, A (Highest CB opcode)
        check_inst(analyzer, memory, {0xCB, 0xFF}, "SET", {"0x7", "A"});
        
        // CB 00 -> RLC B (Lowest CB opcode)
        check_inst(analyzer, memory, {0xCB, 0x00}, "RLC", {"B"});
        
        // CB 30 -> SLL B (Undocumented / No standard mnemonic)
        check_inst(analyzer, memory, {0xCB, 0x30}, "SLL", {"B"});
    }

    // --- Non-indexed Instructions with Prefixes ---
    {
        // DD 00 -> NOP
        check_inst(analyzer, memory, {0xDD, 0x00}, "NOP", {});
        // FD 00 -> NOP
        check_inst(analyzer, memory, {0xFD, 0x00}, "NOP", {});
        
        // DD 47 -> LD B, A (Prefix ignored)
        check_inst(analyzer, memory, {0xDD, 0x47}, "LD", {"B", "A"});
        // FD 90 -> SUB B (Prefix ignored)
        check_inst(analyzer, memory, {0xFD, 0x90}, "SUB", {"B"});
        
        // DD 04 -> INC B (Prefix ignored)
        check_inst(analyzer, memory, {0xDD, 0x04}, "INC", {"B"});
    }

    // --- Prefix Bytes Check ---
    {
        // DD 3E 01 -> LD A, 1 (Prefix DD ignored but present in bytes)
        memory.set_data(0x9000, {0xDD, 0x3E, 0x01});
        auto line = analyzer.parse_instruction(0x9000);
        if (line.mnemonic != "LD") { std::cout << "FAIL: Prefix Bytes Check mnemonic\n"; tests_failed++; }
        else if (line.bytes.size() != 3) { std::cout << "FAIL: Prefix Bytes Check size\n"; tests_failed++; }
        else if (line.bytes[0] != 0xDD) { std::cout << "FAIL: Prefix Bytes Check byte 0\n"; tests_failed++; }
        else tests_passed++;

        // FD DD 00 -> NOP (Prefixes FD DD ignored)
        memory.set_data(0x9003, {0xFD, 0xDD, 0x00});
        line = analyzer.parse_instruction(0x9003);
        if (line.mnemonic != "NOP") { std::cout << "FAIL: Multiple Prefixes mnemonic\n"; tests_failed++; }
        else if (line.bytes.size() != 3) { std::cout << "FAIL: Multiple Prefixes size\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Undocumented Index Shift/Rotate Copy ---
    {
        // SLL (IX+5), H
        // SLL is 0x30 base. H is 4. -> 0x34.
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x05, 0x34}, "SLL", {"(IX+5)", "IXH"});
        
        // SRL (IY-2), A
        // SRL is 0x38 base. A is 7. -> 0x3F.
        check_inst(analyzer, memory, {0xFD, 0xCB, 0xFE, 0x3F}, "SRL", {"(IY-2)", "A"});
        
        // RL (IX+0), C
        // RL is 0x10 base. C is 1. -> 0x11.
        check_inst(analyzer, memory, {0xDD, 0xCB, 0x00, 0x11}, "RL", {"(IX+0)", "C"});
    }

    // --- More Indirect HL Loads/Stores ---
    {
        // LD r, (HL)
        check_inst(analyzer, memory, {0x4E}, "LD", {"C", "(HL)"});
        check_inst(analyzer, memory, {0x56}, "LD", {"D", "(HL)"});
        check_inst(analyzer, memory, {0x5E}, "LD", {"E", "(HL)"});
        check_inst(analyzer, memory, {0x66}, "LD", {"H", "(HL)"});
        check_inst(analyzer, memory, {0x6E}, "LD", {"L", "(HL)"});

        // LD (HL), r
        check_inst(analyzer, memory, {0x71}, "LD", {"(HL)", "C"});
        check_inst(analyzer, memory, {0x72}, "LD", {"(HL)", "D"});
        check_inst(analyzer, memory, {0x73}, "LD", {"(HL)", "E"});
        check_inst(analyzer, memory, {0x74}, "LD", {"(HL)", "H"});
        check_inst(analyzer, memory, {0x75}, "LD", {"(HL)", "L"});
    }

    // --- Parse Code Tests ---
    {
        // Test 1: Basic Raw Disassembly
        // 0x1000: 3E 05 (LD A, 5)
        // 0x1002: 06 0A (LD B, 10)
        memory.set_data(0x1000, {0x3E, 0x05, 0x06, 0x0A});
        uint16_t start = 0x1000;
        auto lines = analyzer.parse_code(start, 10);
        
        if (lines.size() < 2) { std::cout << "FAIL: Parse Code Raw size\n"; tests_failed++; }
        else if (lines[0].mnemonic != "LD" || lines[0].operands[0].s_val != "A") { std::cout << "FAIL: Parse Code Raw line 1\n"; tests_failed++; }
        else if (lines[1].mnemonic != "LD" || lines[1].operands[0].s_val != "B") { std::cout << "FAIL: Parse Code Raw line 2\n"; tests_failed++; }
        else tests_passed++;

        // Test 2: Heuristic Analysis (Jump following)
        // 0x2000: C3 05 20 (JP 0x2005)
        // 0x2003: 00 00    (NOPs/Data)
        // 0x2005: 3C       (INC A)
        memory.set_data(0x2000, {0xC3, 0x05, 0x20, 0x00, 0x00, 0x3C});
        start = 0x2000;
        // Use heuristic = true
        lines = analyzer.parse_code(start, 10, nullptr, false, true);
        
        bool found_jp = false;
        bool found_inc = false;
        bool found_db = false;
        for(const auto& l : lines) {
            if (l.address == 0x2000 && l.mnemonic == "JP") found_jp = true;
            if (l.address == 0x2003 && l.mnemonic == "DB") found_db = true;
            if (l.address == 0x2005 && l.mnemonic == "INC") found_inc = true;
        }
        
        if (!found_jp) { std::cout << "FAIL: Parse Code Heuristic JP missing\n"; tests_failed++; }
        else if (!found_db) { std::cout << "FAIL: Parse Code Heuristic DB missing (gap should be data)\n"; tests_failed++; }
        else if (!found_inc) { std::cout << "FAIL: Parse Code Heuristic INC missing\n"; tests_failed++; }
        else tests_passed++;

        // Test 3: Execution Analysis
        // 0x3000: 3E 01    LD A, 1
        // 0x3002: FE 01    CP 1
        // 0x3004: 28 02    JR Z, +2 (to 0x3008)
        // 0x3006: 00       NOP (Skipped)
        // 0x3008: 76       HALT
        memory.set_data(0x3000, {0x3E, 0x01, 0xFE, 0x01, 0x28, 0x02, 0x00, 0x00, 0x76});
        start = 0x3000;
        // Use execution = true
        lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_ld = false;
        bool found_jr = false;
        bool found_halt = false;
        bool found_skipped_nop_as_db = false;

        for(const auto& l : lines) {
            if (l.address == 0x3000 && l.mnemonic == "LD") found_ld = true;
            if (l.address == 0x3004 && l.mnemonic == "JR") found_jr = true;
            if (l.address == 0x3006 && l.mnemonic == "DB") found_skipped_nop_as_db = true;
            if (l.address == 0x3008 && l.mnemonic == "HALT") found_halt = true;
        }

        if (!found_ld) { std::cout << "FAIL: Parse Code Exec LD missing\n"; tests_failed++; }
        else if (!found_jr) { std::cout << "FAIL: Parse Code Exec JR missing\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Parse Code Exec HALT missing\n"; tests_failed++; }
        else if (!found_skipped_nop_as_db) { std::cout << "FAIL: Parse Code Exec skipped NOP should be DB\n"; tests_failed++; }
        else tests_passed++;

        // Test 4: External CodeMap
        // 0x4000: 00 (NOP)
        // 0x4001: 00 (NOP)
        memory.set_data(0x4000, {0x00, 0xFF});
        Z80Analyzer<TestMemory>::CodeMap my_map(0x10000, 0);
        my_map.mark_code(0x4000, 1, true); // Mark first NOP as code
        
        start = 0x4000;
        lines = analyzer.parse_code(start, 2, &my_map, false, false);
        
        if (lines.size() < 2) { std::cout << "FAIL: Parse Code Map size\n"; tests_failed++; }
        else if (lines[0].mnemonic == "NOP" && lines[1].mnemonic == "DB") tests_passed++;
        else { std::cout << "FAIL: Parse Code Map mismatch\n"; tests_failed++; }
    }

    // --- Parse Code Advanced Tests ---
    {
        // Test 5: Instruction Limit
        // 0x5000: NOP
        // 0x5001: NOP
        // 0x5002: NOP
        memory.set_data(0x5000, {0x00, 0x00, 0x00});
        uint16_t start = 0x5000;
        auto lines = analyzer.parse_code(start, 2); // Limit 2
        if (lines.size() != 2) { std::cout << "FAIL: Parse Code Limit size\n"; tests_failed++; }
        else tests_passed++;

        // Test 6: Max Data Grouping
        // 0x6000: 01 02 03 04 05 (Different bytes to avoid DS)
        memory.set_data(0x6000, {0x01, 0x02, 0x03, 0x04, 0x05});
        Z80Analyzer<TestMemory>::CodeMap data_map(0x10000, 0); // All data
        start = 0x6000;
        // max_data_group = 2
        lines = analyzer.parse_code(start, 3, &data_map, false, false, 2);
        
        if (lines.size() != 3) { 
            std::cout << "FAIL: Parse Code Max Data Group size (expected 3, got " << lines.size() << ")\n"; 
            tests_failed++; 
        } else {
            if (lines[0].operands.size() == 2 && lines[1].operands.size() == 2 && lines[2].operands.size() == 1) tests_passed++;
            else { 
                std::cout << "FAIL: Parse Code Max Data Group operands count. Got: " 
                          << lines[0].operands.size() << ", " 
                          << lines[1].operands.size() << ", " 
                          << lines[2].operands.size() << "\n"; 
                tests_failed++; 
            }
        }

        // Test 7: Heuristic Conditional Jump
        // 0x7000: 28 02    JR Z, +2 (to 0x7004)
        // 0x7002: C9       RET (Fallthrough)
        // 0x7003: FF       DB 0xFF (Data gap)
        // 0x7004: 00       NOP (Target)
        memory.set_data(0x7000, {0x28, 0x02, 0xC9, 0xFF, 0x00});
        start = 0x7000;
        lines = analyzer.parse_code(start, 10, nullptr, false, true); // Use heuristic
        
        bool found_jr = false;
        bool found_ret = false;
        bool found_data = false;
        bool found_nop = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x7000 && l.mnemonic == "JR") found_jr = true;
            if (l.address == 0x7002 && l.mnemonic == "RET") found_ret = true;
            if (l.address == 0x7003 && l.mnemonic == "DB") found_data = true;
            if (l.address == 0x7004 && l.mnemonic == "NOP") found_nop = true;
        }
        
        if (!found_jr) { std::cout << "FAIL: Heuristic JR missing\n"; tests_failed++; }
        else if (!found_ret) { std::cout << "FAIL: Heuristic RET (fallthrough) missing\n"; tests_failed++; }
        else if (!found_data) { std::cout << "FAIL: Heuristic Data gap missing\n"; tests_failed++; }
        else if (!found_nop) { std::cout << "FAIL: Heuristic Target NOP missing\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Edge Cases ---
    {
        // Test 8: Code at Memory End
        // 0xFFFE: 00 (NOP)
        // 0xFFFF: 00 (NOP)
        memory.set_data(0xFFFE, {0x00, 0x00});
        uint16_t start = 0xFFFE;
        auto lines = analyzer.parse_code(start, 10);
        
        if (lines.size() != 2) { std::cout << "FAIL: Parse Code Memory End size\n"; tests_failed++; }
        else if (lines[0].address != 0xFFFE || lines[1].address != 0xFFFF) { std::cout << "FAIL: Parse Code Memory End addresses\n"; tests_failed++; }
        else tests_passed++;

        // Test 9: Execution Analysis Loop
        // 0x9000: 3D       DEC A
        // 0x9001: 20 FD    JR NZ, -3 (to 0x9000)
        // 0x9003: 76       HALT
        memory.set_data(0x9000, {0x3D, 0x20, 0xFD, 0x76});
        
        start = 0x9000;
        lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_dec = false;
        bool found_jr = false;
        bool found_halt = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x9000 && l.mnemonic == "DEC") found_dec = true;
            if (l.address == 0x9001 && l.mnemonic == "JR") found_jr = true;
            if (l.address == 0x9003 && l.mnemonic == "HALT") found_halt = true;
        }
        
        if (!found_dec) { std::cout << "FAIL: Parse Code Exec Loop DEC missing\n"; tests_failed++; }
        else if (!found_jr) { std::cout << "FAIL: Parse Code Exec Loop JR missing\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Parse Code Exec Loop HALT missing\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code SMC & Heuristic Stop ---
    {
        // Test 10: Self-Modifying Code (SMC) with Execution
        // 0x9100: 21 05 91    LD HL, 0x9105
        // 0x9103: 36 76       LD (HL), 0x76 (HALT)
        // 0x9105: 00          NOP (Initially) -> becomes HALT
        // 0x9106: 00          NOP
        // Clear memory first to ensure no artifacts
        for(uint16_t i=0x9100; i<0x9110; ++i) memory.poke(i, 0);
        memory.set_data(0x9100, {0x21, 0x05, 0x91, 0x36, 0x76, 0x00, 0x00});
        
        uint16_t start = 0x9100;
        // Run with execution enabled
        auto lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_halt = false;
        for(const auto& l : lines) {
            if (l.address == 0x9105) {
                if (l.mnemonic == "HALT") found_halt = true;
            }
        }
        
        if (!found_halt) { 
            std::cout << "FAIL: Parse Code SMC: Target instruction not modified to HALT\n"; 
            std::cout << "      Byte at 0x9105: 0x" << std::hex << (int)memory.peek(0x9105) << "\n";
            // Debug output
            for(const auto& l : lines) {
                if (l.address == 0x9105) std::cout << "At 0x9105 found: " << l.mnemonic << " (Byte: 0x" << std::hex << (int)memory.peek(l.address) << ")\n";
            }
            tests_failed++; 
        }
        else tests_passed++;

        // Test 11: Heuristic Stop on Unconditional Jump
        // 0x9200: C3 00 00    JP 0x0000
        // 0x9203: 3C          INC A (Unreachable, should be DB)
        memory.set_data(0x9200, {0xC3, 0x00, 0x00, 0x3C});
        start = 0x9200;
        // Use heuristic
        lines = analyzer.parse_code(start, 10, nullptr, false, true);
        
        bool found_jp = false;
        bool found_db = false;
        for(const auto& l : lines) {
            if (l.address == 0x9200 && l.mnemonic == "JP") found_jp = true;
            if (l.address == 0x9203 && l.mnemonic == "DB") found_db = true;
        }
        
        if (!found_jp) { std::cout << "FAIL: Heuristic Stop JP missing\n"; tests_failed++; }
        else if (!found_db) { std::cout << "FAIL: Heuristic Stop Unreachable code should be DB\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Data Grouping Edge Cases ---
    {
        // Test 12: Max Data Grouping = 1 (No grouping)
        // 0x9300: 01 02 03 04
        memory.set_data(0x9300, {0x01, 0x02, 0x03, 0x04});
        Z80Analyzer<TestMemory>::CodeMap data_map(0x10000, 0); // All data
        
        uint16_t start = 0x9300;
        auto lines = analyzer.parse_code(start, 4, &data_map, false, false, 1);
        
        if (lines.size() != 4) { 
            std::cout << "FAIL: Parse Code Data Group 1 size (expected 4, got " << lines.size() << ")\n"; 
            tests_failed++; 
        } else {
            bool all_single = true;
            for(const auto& l : lines) if(l.operands.size() != 1) all_single = false;
            if (all_single) tests_passed++;
            else { std::cout << "FAIL: Parse Code Data Group 1 operands count\n"; tests_failed++; }
        }

        // Test 13: Max Data Grouping = 0 (Unlimited)
        start = 0x9300;
        lines = analyzer.parse_code(start, 1, &data_map, false, false, 0);
        
        if (lines.size() != 1) { 
            std::cout << "FAIL: Parse Code Data Group 0 size (expected 1, got " << lines.size() << ")\n"; 
            tests_failed++; 
        } else {
            if (lines[0].operands.size() == 4) tests_passed++;
            else { std::cout << "FAIL: Parse Code Data Group 0 operands count (expected 4, got " << lines[0].operands.size() << ")\n"; tests_failed++; }
        }
    }

    // --- Parse Code Heuristic JR ---
    {
        // Test 14: Heuristic Stop on Unconditional JR
        // 0x9400: 18 FE    JR -2 (Jump to self)
        // 0x9402: 3C       INC A (Unreachable)
        memory.set_data(0x9400, {0x18, 0xFE, 0x3C});
        uint16_t start = 0x9400;
        auto lines = analyzer.parse_code(start, 10, nullptr, false, true);
        
        bool found_jr = false;
        bool found_db_jr = false;
        for(const auto& l : lines) {
            if (l.address == 0x9400 && l.mnemonic == "JR") found_jr = true;
            if (l.address == 0x9402 && l.mnemonic == "DB") found_db_jr = true;
        }
        if (!found_jr) { std::cout << "FAIL: Heuristic Stop JR missing\n"; tests_failed++; }
        else if (!found_db_jr) { std::cout << "FAIL: Heuristic Stop JR Unreachable code should be DB\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Execution: Indirect Jump ---
    {
        // Test 15: Execution Tracing Indirect Jump (JP (HL))
        // 0x9500: 21 05 95    LD HL, 0x9505
        // 0x9503: E9          JP (HL)
        // 0x9504: FF          DB 0xFF (Skipped)
        // 0x9505: 76          HALT
        memory.set_data(0x9500, {0x21, 0x05, 0x95, 0xE9, 0xFF, 0x76});
        uint16_t start = 0x9500;
        
        auto lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_ld = false;
        bool found_jp_hl = false;
        bool found_halt = false;
        bool found_db = false;

        for(const auto& l : lines) {
            if (l.address == 0x9500 && l.mnemonic == "LD") found_ld = true;
            if (l.address == 0x9503 && l.mnemonic == "JP") found_jp_hl = true;
            if (l.address == 0x9504 && l.mnemonic == "DB") found_db = true;
            if (l.address == 0x9505 && l.mnemonic == "HALT") found_halt = true;
        }

        if (!found_ld) { std::cout << "FAIL: Exec JP(HL) LD missing\n"; tests_failed++; }
        else if (!found_jp_hl) { std::cout << "FAIL: Exec JP(HL) JP missing\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Exec JP(HL) HALT missing (target not found)\n"; tests_failed++; }
        else if (!found_db) { std::cout << "FAIL: Exec JP(HL) DB missing (skipped byte)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Execution: CALL/RET ---
    {
        // Test 16: Execution Tracing CALL/RET
        // 0x9600: CD 04 96    CALL 0x9604
        // 0x9603: 76          HALT (Return address)
        // 0x9604: C9          RET
        memory.set_data(0x9600, {0xCD, 0x04, 0x96, 0x76, 0xC9});
        uint16_t start = 0x9600;
        
        auto lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_call = false;
        bool found_ret = false;
        bool found_halt = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x9600 && l.mnemonic == "CALL") found_call = true;
            if (l.address == 0x9603 && l.mnemonic == "HALT") found_halt = true;
            if (l.address == 0x9604 && l.mnemonic == "RET") found_ret = true;
        }
        
        if (!found_call) { std::cout << "FAIL: Exec CALL missing\n"; tests_failed++; }
        else if (!found_ret) { std::cout << "FAIL: Exec RET missing\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Exec HALT missing (after return)\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Execution: RST & DJNZ ---
    {
        // Test 17: Execution Tracing RST
        // 0x9700: D7          RST 10H
        // 0x9701: 00          NOP (After return)
        // 0x9702: 76          HALT
        // 0x0010: C9          RET
        memory.set_data(0x9700, {0xD7, 0x00, 0x76});
        memory.poke(0x0010, 0xC9); // RET at RST 10H vector
        
        uint16_t start = 0x9700;
        auto lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_rst = false;
        bool found_nop_after = false;
        bool found_halt = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x9700 && l.mnemonic == "RST") found_rst = true;
            if (l.address == 0x9701 && l.mnemonic == "NOP") found_nop_after = true;
            if (l.address == 0x9702 && l.mnemonic == "HALT") found_halt = true;
        }
        
        if (!found_rst) { std::cout << "FAIL: Exec RST missing\n"; tests_failed++; }
        else if (!found_nop_after) { std::cout << "FAIL: Exec RST return path (NOP) missing\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Exec RST HALT missing\n"; tests_failed++; }
        else tests_passed++;

        // Test 18: Execution Tracing DJNZ
        // 0x9800: 06 02       LD B, 2
        // 0x9802: 3C          INC A
        // 0x9803: 10 FD       DJNZ -3 (to 0x9802)
        // 0x9805: 76          HALT
        memory.set_data(0x9800, {0x06, 0x02, 0x3C, 0x10, 0xFD, 0x76});
        start = 0x9800;
        
        lines = analyzer.parse_code(start, 10, nullptr, true, false);
        
        bool found_ld = false;
        bool found_inc = false;
        bool found_djnz = false;
        bool found_halt_djnz = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x9800 && l.mnemonic == "LD") found_ld = true;
            if (l.address == 0x9802 && l.mnemonic == "INC") found_inc = true;
            if (l.address == 0x9803 && l.mnemonic == "DJNZ") found_djnz = true;
            if (l.address == 0x9805 && l.mnemonic == "HALT") found_halt_djnz = true;
        }
        
        if (!found_ld) { std::cout << "FAIL: Exec DJNZ LD missing\n"; tests_failed++; }
        else if (!found_inc) { std::cout << "FAIL: Exec DJNZ INC missing\n"; tests_failed++; }
        else if (!found_djnz) { std::cout << "FAIL: Exec DJNZ DJNZ missing\n"; tests_failed++; }
        else if (!found_halt_djnz) { std::cout << "FAIL: Exec DJNZ HALT (fallthrough) missing\n"; tests_failed++; }
        else tests_passed++;
    }

    // --- Parse Code Execution + CodeMap ---
    {
        // Test 19: Execution + External CodeMap
        // 0x9900: 18 01       JR +1 (to 0x9903)
        // 0x9902: 00          NOP (Skipped by execution, but marked in map)
        // 0x9903: 76          HALT
        memory.set_data(0x9900, {0x18, 0x01, 0x00, 0x76});
        
        Z80Analyzer<TestMemory>::CodeMap my_map(0x10000, 0);
        // Manually mark the skipped NOP as code
        my_map.mark_code(0x9902, 1, true);
        
        uint16_t start = 0x9900;
        // use_execution = true, external_code_map = &my_map
        auto lines = analyzer.parse_code(start, 10, &my_map, true, false);
        
        bool found_jr = false;
        bool found_nop = false;
        bool found_halt = false;
        
        for(const auto& l : lines) {
            if (l.address == 0x9900 && l.mnemonic == "JR") found_jr = true;
            if (l.address == 0x9902 && l.mnemonic == "NOP") found_nop = true;
            if (l.address == 0x9903 && l.mnemonic == "HALT") found_halt = true;
        }
        
        if (!found_jr) { std::cout << "FAIL: Exec+Map JR missing (execution)\n"; tests_failed++; }
        else if (!found_nop) { std::cout << "FAIL: Exec+Map NOP missing (from map)\n"; tests_failed++; }
        else if (!found_halt) { std::cout << "FAIL: Exec+Map HALT missing (execution)\n"; tests_failed++; }
        else {
            // Verify map was updated
            if (!(my_map[0x9900] & Z80Analyzer<TestMemory>::CodeMap::FLAG_CODE_START)) {
                 std::cout << "FAIL: Exec+Map: Map not updated by execution (JR)\n"; tests_failed++;
            }
            else if (!(my_map[0x9903] & Z80Analyzer<TestMemory>::CodeMap::FLAG_CODE_START)) {
                 std::cout << "FAIL: Exec+Map: Map not updated by execution (HALT)\n"; tests_failed++;
            }
            else tests_passed++;
        }
    }

    std::cout << "Tests passed: " << tests_passed << ", Failed: " << tests_failed << "\n";
}

int main() {
    run_tests();
    return (tests_failed > 0) ? 1 : 0;
}