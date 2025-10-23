//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble.h
// Verson: 1.0.4
//
// This file contains the Z80Assembler class,
// which provides functionality for assembling Z80 assembly instructions
// from strings into machine code.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#ifndef __Z80ASSEMBLE_H__
#define __Z80ASSEMBLE_H__

#include "Z80.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

template <typename TMemory> class Z80Assembler {
public:
    Z80Assembler(TMemory* memory)
        : m_memory(memory), m_operand_parser(m_symbol_table, m_current_address, m_phase), m_encoder(m_memory, m_current_address, m_phase) {
    }

    bool assemble(const std::string& source_code, uint16_t default_org = 0x0000) {
        std::stringstream ss(source_code);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ss, line))
            lines.push_back(line);

        // Pass 1: Collect all labels with their addresses and raw EQU definitions.
        m_symbol_table.clear();
        m_current_address = default_org;
        m_phase = ParsePhase::SymbolTableBuild;
        for (const auto& l : lines)
            process_line(l);

        // Pass 2: Iteratively evaluate all expressions from EQU directives.
        std::cout << "--- Starting Pass 2: Expressions Evaluation ---" << std::endl;
        m_phase = ParsePhase::ExpressionsEvaluation;
        m_symbol_table.resolve_expressions(m_current_address);
        std::cout << "--- Finished Pass 2 ---" << std::endl;

        // Pass 3: Generate the final machine code.
        m_current_address = default_org;
        m_phase = ParsePhase::CodeGeneration;
        for (const auto& l : lines)
            process_line(l);

        return true;
    }
    class SymbolTable {
    public:
        void add(const std::string& name, uint16_t value) {
            if (is_symbol(name)) {
                throw std::runtime_error("Duplicate symbol definition: " + name);
            }
            m_symbols[name] = value;
        }
        void add_expression(const std::string& name, const std::string& expression) {
            if (is_symbol(name) || m_expressions.count(name)) throw std::runtime_error("Duplicate symbol definition: " + name);
            m_expressions[name] = expression;
        }
        bool is_symbol(const std::string& name) const {
            if (name == "$")
                return true;
            return m_symbols.count(name);
        }
        uint16_t get_value(const std::string& name, uint16_t current_address) const {
            if (name == "$")
                return current_address;
            if (m_symbols.count(name))
                return m_symbols.at(name);
            throw std::runtime_error("Undefined symbol: " + name);
        }
        void clear() {
            m_symbols.clear();
            m_expressions.clear();
        }

        bool resolve_expressions(uint16_t& current_address) {
            if (m_expressions.empty()) {
                std::cout << "[PASS 2] No EQU expressions to resolve." << std::endl;
                return true;
            }
            
            for (const auto& pair : m_expressions) {
                uint16_t value;
                if (is_number(pair.second, value)) {
                    add(pair.first, value);
                    std::cout << "[PASS 2] Resolved EQU: " << pair.first << " = " << value << " (0x" << std::hex << value << std::dec << ")" << std::endl;
                } else {
                    throw std::runtime_error("Could not resolve EQU expression for '" + pair.first + "'. Only numeric literals are currently supported.");
                }
            }
            m_expressions.clear();
            return true;
        }

    private:
        std::map<std::string, uint16_t> m_symbols;
        std::map<std::string, std::string> m_expressions;
    };

    const SymbolTable& get_symbol_table() const {
        return m_symbol_table;
    }

private:
    enum class ParsePhase { SymbolTableBuild, ExpressionsEvaluation, CodeGeneration };

    class LineParser {
    public:
        struct ParsedLine {
            std::string label;
            std::string mnemonic;
            std::vector<std::string> operands;
            bool is_equ = false;
            std::string equ_value;
        };

        ParsedLine parse(const std::string& line) {
            ParsedLine result;
            std::string processed_line = line;

            size_t comment_pos = processed_line.find(';');
            if (comment_pos != std::string::npos)
                processed_line.erase(comment_pos);

            // Handle EQU directive
            std::string temp_upper = processed_line;
            std::transform(temp_upper.begin(), temp_upper.end(), temp_upper.begin(), ::toupper);
            size_t equ_pos = temp_upper.find(" EQU ");
            if (equ_pos != std::string::npos) {
                result.is_equ = true;
                result.label = processed_line.substr(0, equ_pos);
                result.label.erase(0, result.label.find_first_not_of(" \t"));
                result.label.erase(result.label.find_last_not_of(" \t") + 1);
                std::transform(result.label.begin(), result.label.end(), result.label.begin(), ::toupper);

                result.equ_value = processed_line.substr(equ_pos + 5);
                result.equ_value.erase(0, result.equ_value.find_first_not_of(" \t"));
                result.equ_value.erase(result.equ_value.find_last_not_of(" \t") + 1);
                return result;
            }

            // Handle labels
            size_t colon_pos = processed_line.find(':');
            if (colon_pos != std::string::npos) {
                result.label = processed_line.substr(0, colon_pos);
                result.label.erase(0, result.label.find_first_not_of(" \t"));
                result.label.erase(result.label.find_last_not_of(" \t") + 1);
                std::transform(result.label.begin(), result.label.end(), result.label.begin(), ::toupper);
                processed_line.erase(0, colon_pos + 1);
            }

            processed_line.erase(0, processed_line.find_first_not_of(" \t\n\r"));
            processed_line.erase(processed_line.find_last_not_of(" \t\n\r") + 1);

            if (processed_line.empty())
                return result;

            std::stringstream instr_stream(processed_line);
            instr_stream >> result.mnemonic;
            std::transform(result.mnemonic.begin(), result.mnemonic.end(), result.mnemonic.begin(), ::toupper);

            std::string ops_str;
            if (std::getline(instr_stream, ops_str)) {
                ops_str.erase(0, ops_str.find_first_not_of(" \t"));
                if (!ops_str.empty()) {
                    std::stringstream ss(ops_str);
                    std::string item;
                    while (std::getline(ss, item, ',')) {
                        item.erase(0, item.find_first_not_of(" \t"));
                        item.erase(item.find_last_not_of(" \t") + 1);
                        result.operands.push_back(item);
                    }
                }
            }
            return result;
        }
    };

    class OperandParser {
    public:
        OperandParser(const SymbolTable& symbol_table, const uint16_t& current_address, const ParsePhase& phase)
            : m_symbol_table(symbol_table), m_current_address(current_address), m_phase(phase) {
        }

        enum class OperandType { REG8, REG16, IMM8, IMM16, MEM_IMM16, MEM_REG16, MEM_INDEXED, CONDITION, UNKNOWN, EXPRESSION };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            uint16_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };

        Operand parse(const std::string& op_str) {
            Operand op;
            std::string upper_op_str = op_str;
            // Keep the original string for case-sensitive values like string literals
            op.str_val = op_str;
            std::transform(upper_op_str.begin(), upper_op_str.end(), upper_op_str.begin(), ::toupper);

            // 1. Check for registers and conditions first to avoid symbol name conflicts
            if (op_str.length() > 1 && op_str.front() == '"' && op_str.back() == '"') {
                // This is a string literal for DB/DEFB. Don't treat it as a symbol.
                // The DB handler will process it based on op.str_val.
                return op;
            }
            if (is_reg8(upper_op_str)) {
                op.type = OperandType::REG8;
                return op;
            }
            if (is_reg16(upper_op_str)) {
                op.type = OperandType::REG16;
                return op;
            }
            if (is_condition(upper_op_str)) {
                op.type = OperandType::CONDITION;
                return op;
            }

            // 2. Check for indexed memory access like (HL), (IX+d), (nn)
            uint16_t num_val;
            if (is_ptr(op_str)) {
                std::string inner = op_str.substr(1, op_str.length() - 2);
                inner.erase(0, inner.find_first_not_of(" \t"));
                inner.erase(inner.find_last_not_of(" \t") + 1);
                return parse_ptr(inner);
            }

            // 3. Check for a number literal
            if (is_number(op_str, num_val)) {
                op.num_val = num_val;
                op.type = (num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
                return op;
            }

            // 4. In Pass 1, anything that is not a number or register is an expression to be resolved later.
            if (m_phase == ParsePhase::SymbolTableBuild) {
                op.type = OperandType::EXPRESSION;
                op.str_val = op_str; // Keep original string for later evaluation
                return op;
            }

            // 5. In Pass 3 (CodeGeneration), it must be a resolvable symbol.
            // Pass 2 (ExpressionsEvaluation) does not use this parser.
            if (m_symbol_table.is_symbol(upper_op_str)) {
                op.num_val = m_symbol_table.get_value(upper_op_str, m_current_address);
                op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
                return op;
            }

            // If we are here in CodeGeneration phase, the symbol is undefined.
            throw std::runtime_error("Unknown operand or undefined symbol: " + op_str);
        }

    private:
        inline bool is_ptr(const std::string& s) const {
            return !s.empty() && s.front() == '(' && s.back() == ')';
        }

        inline bool is_reg8(const std::string& s) const {
            return s_reg8_names.count(s);
        }

        inline bool is_reg16(const std::string& s) const {
            return s_reg16_names.count(s);
        }

        inline bool is_condition(const std::string& s) const {
            return s_condition_names.count(s);
        }
        inline static const std::set<std::string> s_reg8_names = {"B",    "C", "D",   "E",   "H",   "L",
                                                                  "(HL)", "A", "IXH", "IXL", "IYH", "IYL"};
        inline static const std::set<std::string> s_reg16_names = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
        inline static const std::set<std::string> s_condition_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};

        Operand parse_ptr(const std::string& inner) {
            Operand op;
            std::string upper_inner = inner;
            std::transform(upper_inner.begin(), upper_inner.end(), upper_inner.begin(), ::toupper);

            // Case 1: Handle (IX/IY +/- d)
            size_t plus_pos = upper_inner.find('+');
            size_t minus_pos = upper_inner.find('-');
            size_t operator_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;

            if (operator_pos != std::string::npos) {
                std::string base_reg_str = upper_inner.substr(0, operator_pos);
                base_reg_str.erase(base_reg_str.find_last_not_of(" \t") + 1);
                if (base_reg_str == "IX" || base_reg_str == "IY") {
                    std::string offset_str = inner.substr(operator_pos);
                    uint16_t offset_val;
                    if (is_number(offset_str, offset_val)) {
                        op.type = OperandType::MEM_INDEXED;
                        op.base_reg = base_reg_str;
                        op.offset = static_cast<int16_t>(offset_val);
                        return op;
                    }
                }
            }

            // Case 2: Handle (REG16)
            if (is_reg16(upper_inner)) {
                op.type = OperandType::MEM_REG16;
                op.str_val = upper_inner;
                return op;
            }

            // Case 3: Handle (number) or (LABEL)
            uint16_t inner_num_val;
            if (is_number(inner, inner_num_val)) {
                op.type = OperandType::MEM_IMM16;
                op.num_val = inner_num_val;
            } else { // Assume it's a symbol
                op.type = OperandType::MEM_IMM16;
                op.str_val = inner; // Store the original symbol name for later resolution
            }
            return op;
        }

        const SymbolTable& m_symbol_table;
        const uint16_t& m_current_address;
        const ParsePhase& m_phase;
    };
    class InstructionEncoder {
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;
        using InstructionRule = std::function<bool(const std::string&, const std::vector<Operand>&)>;

    public:
        InstructionEncoder(TMemory* memory, uint16_t& current_address, const ParsePhase& phase)
            : m_memory(memory), m_current_address(current_address), m_phase(phase) {}

        bool encode(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& operands) {
            switch (operands.size()) {
            case 0:
                return encode_no_operand(mnemonic);
            case 1:
                return encode_one_operand(mnemonic, operands[0]);
            case 2:
                return encode_two_operands(mnemonic, operands[0], operands[1]);
            }
            return false;
        }

    private:
        bool match(const Operand& op, OperandType type) const {
            if (op.type == type) return true;
            if (m_phase == ParsePhase::SymbolTableBuild && op.type == OperandType::EXPRESSION) {
                // In Pass 1, an EXPRESSION can match any immediate or memory address type
                return type == OperandType::IMM8 || type == OperandType::IMM16 || type == OperandType::MEM_IMM16;
            }
            return false;
        }

        template <typename... Args>
        void assemble(Args... args) {
            if (m_phase == ParsePhase::CodeGeneration)
                (m_memory->poke(m_current_address++, static_cast<uint8_t>(args)), ...);
            else
                m_current_address += sizeof...(args);
        }

        inline static const std::map<std::string, uint8_t> reg8_map = {{"B", 0},   {"C", 1},   {"D", 2},    {"E", 3},
                                                                       {"H", 4},   {"L", 5},   {"(HL)", 6}, {"A", 7},
                                                                       {"IXH", 4}, {"IXL", 5}, {"IYH", 4},  {"IYL", 5}};
        inline static const std::map<std::string, uint8_t> reg16_map = {{"BC", 0}, {"DE", 1}, {"HL", 2}, {"SP", 3}};
        inline static const std::map<std::string, uint8_t> reg16_af_map = {{"BC", 0}, {"DE", 1}, {"HL", 2}, {"AF", 3}};
        inline static const std::map<std::string, uint8_t> condition_map = {{"NZ", 0}, {"Z", 1},  {"NC", 2}, {"C", 3},
                                                                            {"PO", 4}, {"PE", 5}, {"P", 6},  {"M", 7}};

        bool encode_no_operand(const std::string& mnemonic) {
            if (mnemonic == "NOP") {
                assemble(0x00);
                return true;
            }
            if (mnemonic == "HALT") {
                assemble(0x76);
                return true;
            }
            if (mnemonic == "DI") {
                assemble(0xF3);
                return true;
            }
            if (mnemonic == "EI") {
                assemble(0xFB);
                return true;
            }
            if (mnemonic == "EXX") {
                assemble(0xD9);
                return true;
            }
            if (mnemonic == "RET") {
                assemble(0xC9);
                return true;
            }
            if (mnemonic == "RETI") {
                assemble(0xED, 0x4D);
                return true;
            }
            if (mnemonic == "RETN") {
                assemble(0xED, 0x45);
                return true;
            }
            if (mnemonic == "RLCA") {
                assemble(0x07);
                return true;
            }
            if (mnemonic == "RRCA") {
                assemble(0x0F);
                return true;
            }
            if (mnemonic == "RLA") {
                assemble(0x17);
                return true;
            }
            if (mnemonic == "RRA") {
                assemble(0x1F);
                return true;
            }
            if (mnemonic == "DAA") {
                assemble(0x27);
                return true;
            }
            if (mnemonic == "CPL") {
                assemble(0x2F);
                return true;
            }
            if (mnemonic == "SCF") {
                assemble(0x37);
                return true;
            }
            if (mnemonic == "CCF") {
                assemble(0x3F);
                return true;
            }
            if (mnemonic == "LDI") {
                assemble(0xED, 0xA0);
                return true;
            }
            if (mnemonic == "CPI") {
                assemble(0xED, 0xA1);
                return true;
            }
            if (mnemonic == "INI") {
                assemble(0xED, 0xA2);
                return true;
            }
            if (mnemonic == "OUTI") {
                assemble(0xED, 0xA3);
                return true;
            }
            if (mnemonic == "LDD") {
                assemble(0xED, 0xA8);
                return true;
            }
            if (mnemonic == "CPD") {
                assemble(0xED, 0xA9);
                return true;
            }
            if (mnemonic == "IND") {
                assemble(0xED, 0xAA);
                return true;
            }
            if (mnemonic == "OUTD") {
                assemble(0xED, 0xAB);
                return true;
            }
            if (mnemonic == "LDIR") {
                assemble(0xED, 0xB0);
                return true;
            }
            if (mnemonic == "CPIR") {
                assemble(0xED, 0xB1);
                return true;
            }
            if (mnemonic == "INIR") {
                assemble(0xED, 0xB2);
                return true;
            }
            if (mnemonic == "OTIR") {
                assemble(0xED, 0xB3);
                return true;
            }
            if (mnemonic == "LDDR") {
                assemble(0xED, 0xB8);
                return true;
            }
            if (mnemonic == "CPDR") {
                assemble(0xED, 0xB9);
                return true;
            }
            if (mnemonic == "INDR") {
                assemble(0xED, 0xBA);
                return true;
            }
            if (mnemonic == "OTDR") {
                assemble(0xED, 0xBB);
                return true;
            }
            return false;
        }

        bool encode_one_operand(const std::string& mnemonic, const Operand& op) {
            if (mnemonic == "PUSH" && match(op, OperandType::REG16)) {
                if (reg16_af_map.count(op.str_val)) {
                    assemble((uint8_t)(0xC5 | (reg16_af_map.at(op.str_val) << 4)));
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble(0xDD, 0xE5);
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble(0xFD, 0xE5);
                    return true;
                }
            }
            if (mnemonic == "POP" && match(op, OperandType::REG16)) {
                if (reg16_af_map.count(op.str_val)) {
                    assemble((uint8_t)(0xC1 | (reg16_af_map.at(op.str_val) << 4)));
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble(0xDD, 0xE1);
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble(0xFD, 0xE1);
                    return true;
                }
            }
            if (mnemonic == "INC" && match(op, OperandType::REG16)) {
                if (reg16_map.count(op.str_val)) {
                    assemble((uint8_t)(0x03 | (reg16_map.at(op.str_val) << 4)));
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble(0xDD, 0x23);
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble(0xFD, 0x23);
                    return true;
                }
            }
            if (mnemonic == "DEC" && match(op, OperandType::REG16)) {
                if (reg16_map.count(op.str_val)) {
                    assemble((uint8_t)(0x0B | (reg16_map.at(op.str_val) << 4)));
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble(0xDD, 0x2B);
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble(0xFD, 0x2B);
                    return true;
                }
            }
            if (mnemonic == "INC" && match(op, OperandType::REG8)) {
                assemble((uint8_t)(0x04 | (reg8_map.at(op.str_val) << 3)));
                return true;
            }
            if (mnemonic == "DEC" && match(op, OperandType::REG8)) {
                assemble((uint8_t)(0x05 | (reg8_map.at(op.str_val) << 3)));
                return true;
            }
            if (mnemonic == "JP" && (match(op, OperandType::IMM8) || match(op, OperandType::IMM16))) {
                assemble(0xC3, (uint8_t)(op.num_val & 0xFF),
                         (uint8_t)(op.num_val >> 8));
                return true;
            }
            if (mnemonic == "JP" && match(op, OperandType::MEM_REG16)) {
                if (op.str_val == "HL") {
                    assemble(0xE9);
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble(0xDD, 0xE9);
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble(0xFD, 0xE9);
                    return true;
                }
            }
            if (mnemonic == "JR" && (match(op, OperandType::IMM8) || match(op, OperandType::IMM16))) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_current_address + instruction_size);
                if (m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
                    throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                assemble(0x18, static_cast<uint8_t>(offset));
                return true;
            }
            if (mnemonic == "SUB" && match(op, OperandType::IMM8)) {
                assemble(0xD6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "ADD" && match(op, OperandType::REG8)) {
                assemble((uint8_t)(0x80 | reg8_map.at(op.str_val)));
                return true;
            }
            if (mnemonic == "DJNZ" && (match(op, OperandType::IMM8) || match(op, OperandType::IMM16))) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_current_address + instruction_size);
                if (m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
                    throw std::runtime_error("DJNZ jump target out of range. Offset: " + std::to_string(offset));
                assemble(0x10, static_cast<uint8_t>(offset));
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") && match(op, OperandType::MEM_INDEXED)) {
                uint8_t base_opcode = 0;
                if (mnemonic == "ADD")
                    base_opcode = 0x86;
                else if (mnemonic == "ADC")
                    base_opcode = 0x8E;
                else if (mnemonic == "SUB")
                    base_opcode = 0x96;
                else if (mnemonic == "SBC")
                    base_opcode = 0x9E;
                else if (mnemonic == "AND")
                    base_opcode = 0xA6;
                else if (mnemonic == "XOR")
                    base_opcode = 0xAE;
                else if (mnemonic == "OR")
                    base_opcode = 0xB6;
                else if (mnemonic == "CP")
                    base_opcode = 0xBE;

                if (op.base_reg == "IX")
                    assemble(0xDD, base_opcode, static_cast<int8_t>(op.offset));
                else if (op.base_reg == "IY")
                    assemble(0xFD, base_opcode, static_cast<int8_t>(op.offset));

                return true;
            }
            if (mnemonic == "CP" && match(op, OperandType::IMM8)) {
                assemble(0xFE, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "AND" && match(op, OperandType::IMM8)) {
                assemble(0xE6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "OR" && match(op, OperandType::IMM8)) {
                assemble(0xF6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "XOR" && match(op, OperandType::IMM8)) {
                assemble(0xEE, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "SUB" && match(op, OperandType::IMM8)) {
                assemble(0xD6, (uint8_t)op.num_val); // Note: This is SUB A, n - SUB n is ambiguous
                return true;
            }
            if (mnemonic == "CALL" && (match(op, OperandType::IMM8) || match(op, OperandType::IMM16))) {
                assemble(0xCD, (uint8_t)(op.num_val & 0xFF),
                         (uint8_t)(op.num_val >> 8));
                return true;
            }

            return false;
        }

        bool encode_two_operands(const std::string& mnemonic, const Operand& op1, const Operand& op2) {
            if (mnemonic == "EX" && op1.str_val == "AF" && op2.str_val == "AF'") {
                assemble(0x08);
                return true;
            }
            if (mnemonic == "EX" && op1.str_val == "DE" && op2.str_val == "HL") {
                assemble(0xEB);
                return true;
            }
            uint8_t prefix = 0;
            if (op1.base_reg == "IX" || op2.base_reg == "IX" || op1.str_val.find("IX") != std::string::npos ||
                op2.str_val.find("IX") != std::string::npos)
                prefix = 0xDD;
            else if (op1.base_reg == "IY" || op2.base_reg == "IY" || op1.str_val.find("IY") != std::string::npos ||
                     op2.str_val.find("IY") != std::string::npos) {
                prefix = 0xFD;
            }
            if (mnemonic == "LD" && match(op1, OperandType::REG8) && match(op2, OperandType::REG8)) {
                uint8_t dest_code = reg8_map.at(op1.str_val);
                uint8_t src_code = reg8_map.at(op2.str_val);
                if (prefix) {
                    if ((op1.str_val.find("IX") != std::string::npos && op2.str_val.find("IY") != std::string::npos) ||
                        (op1.str_val.find("IY") != std::string::npos && op2.str_val.find("IX") != std::string::npos))
                        throw std::runtime_error("Cannot mix IX and IY register parts");
                    assemble(prefix, (uint8_t)(0x40 | (dest_code << 3) | src_code));
                    return true;
                }
                assemble((uint8_t)(0x40 | (dest_code << 3) | src_code));
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::REG8) && match(op2, OperandType::IMM8)) {
                uint8_t dest_code = reg8_map.at(op1.str_val);
                assemble((uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" &&
                (op1.str_val == "IXH" || op1.str_val == "IXL" || op1.str_val == "IYH" || op1.str_val == "IYL") &&
                match(op2, OperandType::IMM8)) {
                assemble(prefix, (uint8_t)(0x06 | (reg8_map.at(op1.str_val) << 3)),
                         (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::REG16) && (match(op2, OperandType::IMM8) || match(op2, OperandType::IMM16))) {
                if (reg16_map.count(op1.str_val)) {
                    assemble((uint8_t)(0x01 | (reg16_map.at(op1.str_val) << 4)),
                             (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "IX") {
                    assemble(0xDD, 0x21, (uint8_t)(op2.num_val & 0xFF),
                             (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "IY") {
                    assemble(0xFD, 0x21, (uint8_t)(op2.num_val & 0xFF),
                             (uint8_t)(op2.num_val >> 8));
                    return true;
                }
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_REG16) && op2.str_val == "A") {
                if (op1.str_val == "BC") {
                    assemble(0x02);
                    return true;
                }
                if (op1.str_val == "DE") {
                    assemble(0x12);
                    return true;
                }
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_REG16) && match(op2, OperandType::IMM8)) {
                if (op1.str_val == "HL") {
                    uint8_t reg_code = reg8_map.at("(HL)");
                    assemble((uint8_t)(0x06 | (reg_code << 3)), (uint8_t)op2.num_val);
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match(op2, OperandType::MEM_REG16)) {
                if (op2.str_val == "BC") {
                    assemble(0x0A);
                    return true;
                }
                if (op2.str_val == "DE") {
                    assemble(0x1A);
                    return true;
                }
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_IMM16) && op2.str_val == "A") {
                assemble(0x32, (uint8_t)(op1.num_val & 0xFF),
                         (uint8_t)(op1.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match(op2, OperandType::MEM_IMM16)) {
                assemble(0x3A, (uint8_t)(op2.num_val & 0xFF),
                         (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match(op2, OperandType::MEM_IMM16)) {
                assemble(0x3A, (uint8_t)(op2.num_val & 0xFF),
                         (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "IN" && op1.str_val == "A" && match(op2, OperandType::MEM_IMM16)) {
                if (op2.num_val > 0xFF) throw std::runtime_error("Port for IN instruction must be 8-bit");
                assemble(0xDB, (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "OUT" && match(op1, OperandType::MEM_IMM16) && op2.str_val == "A") {
                if (op1.num_val > 0xFF) throw std::runtime_error("Port for OUT instruction must be 8-bit");
                assemble(0xD3, (uint8_t)op1.num_val);
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_INDEXED) && match(op2, OperandType::IMM8)) {
                assemble(prefix, 0x36, static_cast<int8_t>(op1.offset), (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::REG8) && match(op2, OperandType::MEM_INDEXED)) {
                uint8_t reg_code = reg8_map.at(op1.str_val);
                assemble(prefix, (uint8_t)(0x46 | (reg_code << 3)),
                         static_cast<int8_t>(op2.offset));
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_INDEXED) && match(op2, OperandType::REG8)) {
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(prefix, (uint8_t)(0x70 | reg_code), static_cast<int8_t>(op1.offset));
                return true;
            }
            if (mnemonic == "ADD" && op1.str_val == "A" && match(op2, OperandType::IMM8)) {
                assemble(0xC6, (uint8_t)op2.num_val); // Note: This is ADD A, n
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") && op1.str_val == "A" && match(op2, OperandType::REG8)) {
                uint8_t base_opcode = 0;
                if (mnemonic == "ADD")
                    base_opcode = 0x80;
                else if (mnemonic == "ADC")
                    base_opcode = 0x88;
                else if (mnemonic == "SUB")
                    base_opcode = 0x90;
                else if (mnemonic == "SBC")
                    base_opcode = 0x98;
                else if (mnemonic == "AND")
                    base_opcode = 0xA0;
                else if (mnemonic == "XOR")
                    base_opcode = 0xA8;
                else if (mnemonic == "OR")
                    base_opcode = 0xB0;
                else if (mnemonic == "CP")
                    base_opcode = 0xB8;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                if (prefix)
                    assemble(prefix, (uint8_t)(base_opcode | reg_code));
                else
                    assemble((uint8_t)(base_opcode | reg_code));
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") &&
                op1.str_val == "A" && match(op2, OperandType::MEM_INDEXED)) {
                uint8_t base_opcode = 0;
                if (mnemonic == "ADD")
                    base_opcode = 0x86;
                else if (mnemonic == "ADC")
                    base_opcode = 0x8E;
                else if (mnemonic == "SUB")
                    base_opcode = 0x96;
                else if (mnemonic == "SBC")
                    base_opcode = 0x9E;
                else if (mnemonic == "AND")
                    base_opcode = 0xA6;
                else if (mnemonic == "XOR")
                    base_opcode = 0xAE;
                else if (mnemonic == "OR")
                    base_opcode = 0xB6;
                else if (mnemonic == "CP")
                    base_opcode = 0xBE;

                if (op2.base_reg == "IX")
                    assemble(0xDD, base_opcode, static_cast<int8_t>(op2.offset));
                else if (op2.base_reg == "IY")
                    assemble(0xFD, base_opcode, static_cast<int8_t>(op2.offset));

                return true;
            }
            if (mnemonic == "JP" && match(op1, OperandType::CONDITION) && (match(op2, OperandType::IMM8) || match(op2, OperandType::IMM16))) {
                uint8_t cond_code = condition_map.at(op1.str_val);
                assemble((uint8_t)(0xC2 | (cond_code << 3)),
                         (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "JR" && match(op1, OperandType::CONDITION) && (match(op2, OperandType::IMM8) || match(op2, OperandType::IMM16))) {
                const std::map<std::string, uint8_t> jr_condition_map = {
                    {"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}};
                if (jr_condition_map.count(op1.str_val)) {
                    int32_t target_addr = op2.num_val;
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_current_address + instruction_size);
                    if (m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
                        throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                    assemble(jr_condition_map.at(op1.str_val),
                             static_cast<uint8_t>(offset));
                    return true;
                }
            }
            if (mnemonic == "CALL" && match(op1, OperandType::CONDITION) && (match(op2, OperandType::IMM8) || match(op2, OperandType::IMM16))) {
                uint8_t cond_code = condition_map.at(op1.str_val);
                assemble((uint8_t)(0xC4 | (cond_code << 3)),
                         (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "JR" && match(op1, OperandType::CONDITION) && (match(op2, OperandType::IMM8) || match(op2, OperandType::IMM16))) {
                const std::map<std::string, uint8_t> jr_condition_map = {
                    {"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}};
                if (jr_condition_map.count(op1.str_val)) {
                    int32_t target_addr = op2.num_val;
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_current_address + instruction_size);
                    if (m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
                        throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                    assemble(jr_condition_map.at(op1.str_val),
                             static_cast<uint8_t>(offset));
                    return true;
                }
            }
            if (mnemonic == "IN" && match(op1, OperandType::REG8) && op2.str_val == "(C)") {
                uint8_t reg_code = reg8_map.at(op1.str_val);
                if (reg_code == 6) {
                    assemble(0xED, 0x70);
                    return true;
                }
                assemble(0xED, (uint8_t)(0x40 | (reg_code << 3)));
                return true;
            }
            if (mnemonic == "OUT" && op1.str_val == "(C)" && match(op2, OperandType::REG8)) {
                uint8_t reg_code = reg8_map.at(op2.str_val);
                if (reg_code == 6)
                    throw std::runtime_error("OUT (C), (HL) is not a valid instruction");
                assemble(0xED, (uint8_t)(0x41 | (reg_code << 3)));
                return true;
            }
            if (mnemonic == "BIT" && match(op1, OperandType::IMM8) && match(op2, OperandType::REG8)) {
                if (op1.num_val > 7)
                    throw std::runtime_error("BIT index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "SET" && match(op1, OperandType::IMM8) && match(op2, OperandType::REG8)) {
                if (op1.num_val > 7)
                    throw std::runtime_error("SET index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "RES" && match(op1, OperandType::IMM8) && match(op2, OperandType::REG8)) {
                if (op1.num_val > 7)
                    throw std::runtime_error("RES index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0x80 | (bit << 3) | reg_code));
                return true;
            }
            if ((mnemonic == "SLL" || mnemonic == "SLI") && match(op1, OperandType::REG8)) {
                if (op1.num_val > 7) {
                    throw std::runtime_error("SLL bit index must be 0-7");
                }
                uint8_t reg_code = reg8_map.at(op1.str_val);
                assemble(0xCB, (uint8_t)(0x30 | reg_code));
                return true;
            }
            if ((mnemonic == "BIT" || mnemonic == "SET" || mnemonic == "RES") &&
                match(op1, OperandType::IMM8) && match(op2, OperandType::MEM_INDEXED)) {
                if (op1.num_val > 7)
                    throw std::runtime_error(mnemonic + " bit index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t base_opcode = 0;
                if (mnemonic == "BIT")
                    base_opcode = 0x40;
                else if (mnemonic == "RES")
                    base_opcode = 0x80;
                else // SET
                    base_opcode = 0xC0;

                uint8_t final_opcode = base_opcode | (bit << 3) | 6; // 6 is the code for (HL)

                if (op2.base_reg == "IX") {
                    assemble(0xDD, 0xCB, static_cast<uint8_t>(op2.offset), final_opcode);
                } else if (op2.base_reg == "IY") {
                    assemble(0xFD, 0xCB, static_cast<uint8_t>(op2.offset), final_opcode);
                } else {
                    return false;
                }
                return true;
            }
            return false;
        }

        TMemory* m_memory;
        uint16_t& m_current_address;
        const ParsePhase& m_phase;
    };

    void process_line(const std::string& line) {
        typename Z80Assembler<TMemory>::LineParser::ParsedLine parsed = m_line_parser.parse(line);

        switch (m_phase) {
        case ParsePhase::SymbolTableBuild: {
            if (parsed.is_equ) {
                m_symbol_table.add_expression(parsed.label, parsed.equ_value);
                std::cout << "[PASS 1] Found EQU: " << parsed.label << " = " << parsed.equ_value << std::endl;
                return;
            }

            if (!parsed.label.empty()) {
                m_symbol_table.add(parsed.label, m_current_address);
                std::cout << "[PASS 1] Found Label: " << parsed.label << " at 0x" << std::hex << m_current_address << std::dec << std::endl;
            }
            break;
        }
        case ParsePhase::ExpressionsEvaluation:
            // This phase is handled outside process_line
            return;

        case ParsePhase::CodeGeneration:
            if (parsed.is_equ)
                return;
            break;
        }

        if (parsed.mnemonic.empty())
            return;

        std::vector<typename OperandParser::Operand> ops;
        for (const auto& s : parsed.operands)
            ops.push_back(m_operand_parser.parse(s));

        if (!assemble_instruction(parsed.mnemonic, ops)) {
            // Reconstruct the problematic part of the line for a better error message.
            std::string error_line = parsed.mnemonic;
            if (!parsed.operands.empty()) {
                error_line += " ";
                for (size_t i = 0; i < parsed.operands.size(); ++i) {
                    error_line += parsed.operands[i] + (i < parsed.operands.size() - 1 ? "," : "");
                }
            }
            throw std::runtime_error("Unsupported or invalid instruction: " + error_line);
        }
    }

    inline static bool is_number(const std::string& s, uint16_t& out_value) {
        std::string str = s;
        str.erase(0, str.find_first_not_of(" \t\n\r"));
        str.erase(str.find_last_not_of(" \t\n\r") + 1);
        if (str.empty())
            return false;
        if (str.length() == 3 && str.front() == '\'' && str.back() == '\'') {
            out_value = static_cast<uint8_t>(str[1]);
            return true;
        }

        const char* start = str.data();
        const char* end = str.data() + str.size();
        int base = 10;
        if (str.size() > 2 && (str.substr(0, 2) == "0X" || str.substr(0, 2) == "0x")) {
            start += 2;
            base = 16;
        } else if (str.back() == 'H' || str.back() == 'h') {
            end -= 1;
            base = 16;
        }
        auto result = std::from_chars(start, end, out_value, base);
        return result.ec == std::errc() && result.ptr == end;
    }

    bool assemble_instruction(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& ops) {
        // Handle ORG directive
        if (mnemonic == "ORG") {
            if (ops.size() == 1 &&
                (ops[0].type == OperandParser::OperandType::IMM8 || ops[0].type == OperandParser::OperandType::IMM16)) {
                m_current_address = ops[0].num_val;
                return true; // ORG doesn't generate bytes
            } else {
                throw std::runtime_error("Invalid operand for ORG directive");
            }
        }
        // --- Pseudo-instructions ---
        if (mnemonic == "DB" || mnemonic == "DEFB") {
            std::vector<uint8_t> bytes;
            bytes.reserve(ops.size());
            for (const auto& op : ops) {
                if (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16) {
                    if (op.num_val > 0xFF) {
                        throw std::runtime_error("Value in DB statement exceeds 1 byte: " + op.str_val);
                    }
                    if (m_phase == ParsePhase::CodeGeneration)
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val));
                    else
                        m_current_address++;
                } else if (op.str_val.front() == '"' && op.str_val.back() == '"') {
                    std::string str_content = op.str_val.substr(1, op.str_val.length() - 2);
                    for (char c : str_content) {
                        if (m_phase == ParsePhase::CodeGeneration)
                            m_memory->poke(m_current_address++, static_cast<uint8_t>(c));
                        else
                            m_current_address++;
                    }
                } else {
                    throw std::runtime_error("Unsupported operand for DB: " + op.str_val);
                }
            }
            return true;
        }

        if (mnemonic == "DW" || mnemonic == "DEFW") {
            for (const auto& op : ops) {
                if (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16 ||
                    (m_phase == ParsePhase::SymbolTableBuild && op.type == OperandParser::OperandType::EXPRESSION)) {
                    if (m_phase == ParsePhase::CodeGeneration) {
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val & 0xFF));
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val >> 8));
                    } else
                        m_current_address += 2;
                } else {
                    throw std::runtime_error("Unsupported operand for DW: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
            }
            return true;
        }

        if (mnemonic == "DS" || mnemonic == "DEFS") {
            if (ops.empty() || ops.size() > 2) {
                throw std::runtime_error("DS/DEFS requires 1 or 2 operands.");
            }
            if (ops[0].type != OperandParser::OperandType::IMM8 && ops[0].type != OperandParser::OperandType::IMM16) {
                throw std::runtime_error("DS/DEFS size must be a number.");
            }
            size_t count = ops[0].num_val;
            uint8_t fill_value = 0;
            if (ops.size() == 2) {
                if (ops[1].type != OperandParser::OperandType::IMM8) {
                    throw std::runtime_error("DS/DEFS fill value must be an 8-bit number.");
                }
                fill_value = static_cast<uint8_t>(ops[1].num_val);
            }
            if (m_phase == ParsePhase::CodeGeneration)
                for (size_t i = 0; i < count; ++i)
                    m_memory->poke(m_current_address++, fill_value);
            else
                m_current_address += count;
            return true;
        }

        return m_encoder.encode(mnemonic, ops);
    }

    ParsePhase m_phase;
    uint16_t m_current_address = 0;
    TMemory* m_memory = nullptr;
    typename Z80Assembler<TMemory>::SymbolTable m_symbol_table;
    typename Z80Assembler<TMemory>::LineParser m_line_parser;
    OperandParser m_operand_parser;
    InstructionEncoder m_encoder;
};

#endif //__Z80ASSEMBLE_H__
