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

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <map>

class Z80Assembler {
public:
    std::vector<uint8_t> assemble(const std::string& source_code, uint16_t default_org = 0x0000) {
        m_symbol_table.clear();
        std::vector<std::string> lines;
        std::stringstream ss(source_code);
        std::string line;
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        // --- First Pass: Build symbol table ---
        m_current_address = default_org;
        m_pass = 1;
        for (const auto& l : lines) {
            // In the first pass, we only care about calculating instruction sizes to find label addresses.
            // The actual byte generation is irrelevant, but calling assemble_line is the easiest way
            // to parse and get the size.
            auto bytes = assemble_line(l);
            m_current_address += bytes.size();
        }

        // --- Second Pass: Generate machine code ---
        m_current_address = default_org;
        m_pass = 2;
        std::vector<uint8_t> machine_code;
        for (const auto& l : lines) {
            auto bytes = assemble_line(l);
            machine_code.insert(machine_code.end(), bytes.begin(), bytes.end());
            m_current_address += bytes.size();
        }

        return machine_code;
    }

private:
    int m_pass = 1;
    uint16_t m_current_address = 0;
    std::map<std::string, uint16_t> m_symbol_table;

    std::vector<uint8_t> assemble_line(const std::string& instruction) {
        std::string processed_instr = instruction;

        // Remove comments
        size_t comment_pos = processed_instr.find(';');
        if (comment_pos != std::string::npos) {
            processed_instr.erase(comment_pos);
        }

        // Handle EQU directive (only in pass 1)
        if (m_pass == 1) {
            std::string temp_upper = processed_instr;
            std::transform(temp_upper.begin(), temp_upper.end(), temp_upper.begin(), ::toupper);
            size_t equ_pos = temp_upper.find(" EQU ");
            if (equ_pos != std::string::npos) {
                std::string symbol = processed_instr.substr(0, equ_pos);
                symbol.erase(0, symbol.find_first_not_of(" \t"));
                symbol.erase(symbol.find_last_not_of(" \t") + 1);
                std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

                std::string value_str = processed_instr.substr(equ_pos + 5);
                value_str.erase(0, value_str.find_first_not_of(" \t"));
                value_str.erase(value_str.find_last_not_of(" \t") + 1);

                uint16_t value;
                if (is_number(value_str, value)) {
                    if (m_symbol_table.count(symbol)) {
                        throw std::runtime_error("Duplicate symbol definition: " + symbol);
                    }
                    m_symbol_table[symbol] = value;
                } else {
                    // For now, we only support numeric values for EQU.
                    // Could be extended to support expressions.
                    throw std::runtime_error("Invalid value for EQU: " + value_str);
                }
                return {}; // EQU does not generate code
            }
        }

        // Handle labels
        size_t colon_pos = processed_instr.find(':');
        if (colon_pos != std::string::npos) {
            std::string label = processed_instr.substr(0, colon_pos);
            label.erase(0, label.find_first_not_of(" \t"));
            label.erase(label.find_last_not_of(" \t") + 1);
            std::transform(label.begin(), label.end(), label.begin(), ::toupper);

            if (m_pass == 1) {
                if (m_symbol_table.count(label)) {
                    throw std::runtime_error("Duplicate label definition: " + label);
                }
                m_symbol_table[label] = m_current_address;
            }
            processed_instr.erase(0, colon_pos + 1);
        }

        // Trim leading/trailing whitespace
        processed_instr.erase(0, processed_instr.find_first_not_of(" \t\n\r"));
        processed_instr.erase(processed_instr.find_last_not_of(" \t\n\r") + 1);

        if (processed_instr.empty()) {
            return {}; // Ignore empty or comment-only lines
        }

        // Convert to uppercase for case-insensitive matching
        std::transform(processed_instr.begin(), processed_instr.end(), processed_instr.begin(), ::toupper);

        std::string mnemonic;
        std::vector<std::string> operands;

        std::stringstream instr_stream(processed_instr);
        instr_stream >> mnemonic;

        std::string ops_str;
        if (std::getline(instr_stream, ops_str)) {
            // Trim leading whitespace from the operand string
            ops_str.erase(0, ops_str.find_first_not_of(" \t"));

            if (!ops_str.empty()) {
                std::stringstream ss(ops_str);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    // Trim whitespace from operand
                    item.erase(0, item.find_first_not_of(" \t"));
                    item.erase(item.find_last_not_of(" \t") + 1);
                    operands.push_back(item);
                }
            }
        }

        return assemble_instruction(mnemonic, operands);
    }

    enum class OperandType { REG8, REG16, IMM8, IMM16, MEM_IMM16, MEM_REG16, MEM_INDEXED, CONDITION, UNKNOWN };

    struct Operand {
        OperandType type = OperandType::UNKNOWN;
        std::string str_val;
        uint16_t num_val = 0;
        int8_t offset = 0;
        std::string base_reg; // For indexed addressing
    };

    // --- Register Maps ---
    const std::map<std::string, uint8_t> reg8_map = {
        {"B", 0}, {"C", 1}, {"D", 2}, {"E", 3}, {"H", 4}, {"L", 5}, {"(HL)", 6}, {"A", 7},
        {"IXH", 4}, {"IXL", 5}, {"IYH", 4}, {"IYL", 5}
    };
    const std::map<std::string, uint8_t> reg16_map = {
        {"BC", 0}, {"DE", 1}, {"HL", 2}, {"SP", 3}
    };
    const std::map<std::string, uint8_t> reg16_af_map = {
        {"BC", 0}, {"DE", 1}, {"HL", 2}, {"AF", 3}
    };
    const std::map<std::string, uint8_t> condition_map = {
        {"NZ", 0}, {"Z", 1}, {"NC", 2}, {"C", 3}, {"PO", 4}, {"PE", 5}, {"P", 6}, {"M", 7}
    };

    // --- Parsing Helpers ---

    bool is_number(const std::string& s, uint16_t& value) {
        std::string str = s;
        // Trim whitespace before attempting conversion
        str.erase(0, str.find_first_not_of(" \t\n\r"));
        str.erase(str.find_last_not_of(" \t\n\r") + 1);
        if (str.empty()) return false;

        try {
            size_t pos;
            if (str.size() > 2 && str.substr(0, 2) == "0X") {
                value = std::stoul(str.substr(2), &pos, 16);
                pos += 2; // Adjust for "0X" prefix
            } else if (str.back() == 'H') {
                value = std::stoul(str.substr(0, str.length() - 1), &pos, 16);
                pos += 1; // Adjust for 'H' suffix
            } else {
                value = std::stoul(str, &pos, 10);
            }
            return pos == str.length();
        } catch (...) {
            return false;
        }
    }

    Operand parse_operand(const std::string& op_str) {
        Operand op;
        op.str_val = op_str;
        uint16_t num_val;

        // Handle '$' for current address
        if (op_str == "$") {
            op.num_val = m_current_address;
            op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
            return op;
        }

        if (reg8_map.count(op_str)) {
            op.type = OperandType::REG8;
            return op;
        }
        if (reg16_map.count(op_str) || op_str == "IX" || op_str == "IY" || op_str == "AF" || op_str == "AF'") {
            op.type = OperandType::REG16;
            return op;
        }
        if (condition_map.count(op_str)) {
            op.type = OperandType::CONDITION;
            return op;
        }
        if (is_number(op_str, num_val)) {
            op.num_val = num_val;
            op.type = (num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
            return op;
        }
        // Check if it's a known label
        if (m_symbol_table.count(op_str)) {
            op.num_val = m_symbol_table.at(op_str);
            op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
        }
        if (op_str.front() == '(' && op_str.back() == ')') {
            std::string inner = op_str.substr(1, op_str.length() - 2);
            if (reg16_map.count(inner) || inner == "IX" || inner == "IY") {
                op.type = OperandType::MEM_REG16;
                op.str_val = inner;
                return op;
            }
            if (is_number(inner, num_val)) {
                op.type = OperandType::MEM_IMM16;
                op.num_val = num_val;
                return op;
            }
            if (m_symbol_table.count(inner)) {
                op.type = OperandType::MEM_IMM16;
                op.num_val = m_symbol_table.at(inner);
                return op;
            }
            // Indexed addressing (IX+d), (IY+d)
            size_t plus_pos = inner.find('+');
            size_t minus_pos = inner.find('-');
            size_t sign_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;

            if (sign_pos != std::string::npos) {
                op.base_reg = inner.substr(0, sign_pos);
                if (op.base_reg == "IX" || op.base_reg == "IY") {
                    std::string offset_str = inner.substr(sign_pos);
                    try {
                        op.offset = std::stoi(offset_str, nullptr, 0);
                        op.type = OperandType::MEM_INDEXED;
                        return op;
                    } catch (...) { /* fall through */ }
                }
            }
        }

        // If after all checks the type is still unknown, it might be a forward-referenced label.
        if (op.type == OperandType::UNKNOWN) {
            if (m_pass == 1) {
                // In pass 1, assume it's a 16-bit value (address) and create a placeholder.
                op.type = OperandType::IMM16;
            }
            // In pass 2, an UNKNOWN type will cause an error later, which is correct.
        }
        return op;
    }

    // --- Assembly Logic ---

    std::vector<uint8_t> assemble_instruction(const std::string& mnemonic, const std::vector<std::string>& operands_str) {
        std::vector<Operand> ops;
        for (const auto& s : operands_str) {
            ops.push_back(parse_operand(s));
        }

        // Handle ORG directive
        if (mnemonic == "ORG") {
            if (ops.size() == 1 && (ops[0].type == OperandType::IMM8 || ops[0].type == OperandType::IMM16)) {
                m_current_address = ops[0].num_val;
                return {}; // ORG doesn't generate bytes
            } else {
                // In pass 1, a label might not be resolved yet, but we can still calculate size.
                if (m_pass == 1 && ops.size() == 1) return {};
                throw std::runtime_error("Invalid operand for ORG directive");
            }
        }
        // --- Pseudo-instructions ---
        if (mnemonic == "DB" || mnemonic == "DEFB") {
            std::vector<uint8_t> bytes;
            for (const auto& op : ops) {
                if (op.type == OperandType::IMM8 || op.type == OperandType::IMM16) {
                    if (op.num_val > 0xFF) {
                        throw std::runtime_error("Value in DB statement exceeds 1 byte: " + op.str_val);
                    }
                    bytes.push_back(static_cast<uint8_t>(op.num_val));
                } else if (op.str_val.front() == '"' && op.str_val.back() == '"') {
                    // It's a string literal
                    std::string str_content = op.str_val.substr(1, op.str_val.length() - 2);
                    for (char c : str_content) {
                        bytes.push_back(static_cast<uint8_t>(c));
                    }
                } else {
                    throw std::runtime_error("Unsupported operand for DB: " + op.str_val);
                }
            }
            return bytes;
        }

        if (mnemonic == "DW" || mnemonic == "DEFW") {
            std::vector<uint8_t> bytes;
            for (const auto& op : ops) {
                if (op.type == OperandType::IMM8 || op.type == OperandType::IMM16) {
                    bytes.push_back(static_cast<uint8_t>(op.num_val & 0xFF));
                    bytes.push_back(static_cast<uint8_t>(op.num_val >> 8));
                } else {
                    throw std::runtime_error("Unsupported operand for DW: " + op.str_val);
                }
            }
            return bytes;
        }

        if (mnemonic == "DS" || mnemonic == "DEFS") {
            if (ops.empty() || ops.size() > 2) {
                throw std::runtime_error("DS/DEFS requires 1 or 2 operands.");
            }
            if (ops[0].type != OperandType::IMM8 && ops[0].type != OperandType::IMM16) {
                throw std::runtime_error("DS/DEFS size must be a number.");
            }
            size_t count = ops[0].num_val;
            uint8_t fill_value = 0;
            if (ops.size() == 2) {
                if (ops[1].type != OperandType::IMM8) {
                    throw std::runtime_error("DS/DEFS fill value must be an 8-bit number.");
                }
                fill_value = static_cast<uint8_t>(ops[1].num_val);
            }
            return std::vector<uint8_t>(count, fill_value);
        }

        // --- Simple no-operand instructions ---
        if (ops.empty()) {
            if (mnemonic == "NOP") return {0x00};
            if (mnemonic == "HALT") return {0x76};
            if (mnemonic == "DI") return {0xF3};
            if (mnemonic == "EI") return {0xFB};
            if (mnemonic == "EXX") return {0xD9};
            if (mnemonic == "RET") return {0xC9};
            if (mnemonic == "RETI") return {0xED, 0x4D};
            if (mnemonic == "RETN") return {0xED, 0x45};
            if (mnemonic == "RLCA") return {0x07};
            if (mnemonic == "RRCA") return {0x0F};
            if (mnemonic == "RLA") return {0x17};
            if (mnemonic == "RRA") return {0x1F};
            if (mnemonic == "DAA") return {0x27};
            if (mnemonic == "CPL") return {0x2F};
            if (mnemonic == "SCF") return {0x37};
            if (mnemonic == "CCF") return {0x3F};
            if (mnemonic == "LDI") return {0xED, 0xA0};
            if (mnemonic == "CPI") return {0xED, 0xA1};
            if (mnemonic == "INI") return {0xED, 0xA2};
            if (mnemonic == "OUTI") return {0xED, 0xA3};
            if (mnemonic == "LDD") return {0xED, 0xA8};
            if (mnemonic == "CPD") return {0xED, 0xA9};
            if (mnemonic == "IND") return {0xED, 0xAA};
            if (mnemonic == "OUTD") return {0xED, 0xAB};
            if (mnemonic == "LDIR") return {0xED, 0xB0};
            if (mnemonic == "CPIR") return {0xED, 0xB1};
            if (mnemonic == "INIR") return {0xED, 0xB2};
            if (mnemonic == "OTIR") return {0xED, 0xB3};
            if (mnemonic == "LDDR") return {0xED, 0xB8};
            if (mnemonic == "CPDR") return {0xED, 0xB9};
            if (mnemonic == "INDR") return {0xED, 0xBA};
            if (mnemonic == "OTDR") return {0xED, 0xBB};
        }

        // --- One-operand instructions ---
        if (ops.size() == 1) {
            // PUSH/POP
            if (mnemonic == "PUSH" && ops[0].type == OperandType::REG16) {
                if (reg16_af_map.count(ops[0].str_val)) {
                    return { (uint8_t)(0xC5 | (reg16_af_map.at(ops[0].str_val) << 4)) };
                }
                if (ops[0].str_val == "IX") return {0xDD, 0xE5};
                if (ops[0].str_val == "IY") return {0xFD, 0xE5};
            }
            if (mnemonic == "POP" && ops[0].type == OperandType::REG16) {
                if (reg16_af_map.count(ops[0].str_val)) {
                    return { (uint8_t)(0xC1 | (reg16_af_map.at(ops[0].str_val) << 4)) };
                }
                if (ops[0].str_val == "IX") return {0xDD, 0xE1};
                if (ops[0].str_val == "IY") return {0xFD, 0xE1};
            }
            // INC/DEC r16
            if (mnemonic == "INC" && ops[0].type == OperandType::REG16) {
                if (reg16_map.count(ops[0].str_val)) {
                    return { (uint8_t)(0x03 | (reg16_map.at(ops[0].str_val) << 4)) };
                }
                if (ops[0].str_val == "IX") return {0xDD, 0x23};
                if (ops[0].str_val == "IY") return {0xFD, 0x23};
            }
            if (mnemonic == "DEC" && ops[0].type == OperandType::REG16) {
                if (reg16_map.count(ops[0].str_val)) {
                    return { (uint8_t)(0x0B | (reg16_map.at(ops[0].str_val) << 4)) };
                }
                if (ops[0].str_val == "IX") return {0xDD, 0x2B};
                if (ops[0].str_val == "IY") return {0xFD, 0x2B};
            }
            // INC/DEC r8
            if (mnemonic == "INC" && ops[0].type == OperandType::REG8) {
                return { (uint8_t)(0x04 | (reg8_map.at(ops[0].str_val) << 3)) };
            }
            if (mnemonic == "DEC" && ops[0].type == OperandType::REG8) {
                return { (uint8_t)(0x05 | (reg8_map.at(ops[0].str_val) << 3)) };
            }
            // JP nn
            if (mnemonic == "JP" && (ops[0].type == OperandType::IMM8 || ops[0].type == OperandType::IMM16)) {
                return { 0xC3, (uint8_t)(ops[0].num_val & 0xFF), (uint8_t)(ops[0].num_val >> 8) };
            }
            // JP (rr)
            if (mnemonic == "JP" && ops[0].type == OperandType::MEM_REG16) {
                if (ops[0].str_val == "HL") return {0xE9};
                if (ops[0].str_val == "IX") return {0xDD, 0xE9};
                if (ops[0].str_val == "IY") return {0xFD, 0xE9};
            }
            // JR d
            if (mnemonic == "JR" && (ops[0].type == OperandType::IMM8 || ops[0].type == OperandType::IMM16)) {
                int32_t target_addr = ops[0].num_val;
                int32_t offset = target_addr - (m_current_address + 2);
                if (m_pass == 2 && (offset < -128 || offset > 127)) {
                    throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                }
                return { 0x18, static_cast<uint8_t>(offset) };
            }
            // SUB n
            if (mnemonic == "SUB" && ops[0].type == OperandType::IMM8) {
                return { 0xD6, (uint8_t)ops[0].num_val };
            }
            // ADD A, r (this is an alias for ADD r)
            if (mnemonic == "ADD" && ops[0].type == OperandType::REG8) {
                 return { (uint8_t)(0x80 | reg8_map.at(ops[0].str_val)) };
            }
            // DJNZ d
            if (mnemonic == "DJNZ" && (ops[0].type == OperandType::IMM8 || ops[0].type == OperandType::IMM16)) {
                int32_t target_addr = ops[0].num_val;
                int32_t offset = target_addr - (m_current_address + 2);
                if (m_pass == 2 && (offset < -128 || offset > 127)) {
                    throw std::runtime_error("DJNZ jump target out of range. Offset: " + std::to_string(offset));
                }
                return { 0x10, static_cast<uint8_t>(offset) };
            }

        }

        // --- Two-operand instructions ---
        if (ops.size() == 2) {
            // EX AF, AF'
            if (mnemonic == "EX" && ops[0].str_val == "AF" && ops[1].str_val == "AF'") {
                return {0x08};
            }

            // EX DE, HL
            if (mnemonic == "EX" && ops[0].str_val == "DE" && ops[1].str_val == "HL") {
                return {0xEB};
            }

            // Handle IX/IY parts as 8-bit registers
            uint8_t prefix = 0;
            if (ops[0].str_val.find("IX") != std::string::npos || ops[1].str_val.find("IX") != std::string::npos) {
                prefix = 0xDD;
            } else if (ops[0].str_val.find("IY") != std::string::npos || ops[1].str_val.find("IY") != std::string::npos) {
                prefix = 0xFD;
            }

            // LD r, r'
            if (mnemonic == "LD" && ops[0].type == OperandType::REG8 && ops[1].type == OperandType::REG8) {
                uint8_t dest_code = reg8_map.at(ops[0].str_val);
                uint8_t src_code = reg8_map.at(ops[1].str_val);
                if (prefix) {
                    // Disallow LD IXH, IYL etc.
                    if ((ops[0].str_val.find("IX") != std::string::npos && ops[1].str_val.find("IY") != std::string::npos) ||
                        (ops[0].str_val.find("IY") != std::string::npos && ops[1].str_val.find("IX") != std::string::npos))
                        throw std::runtime_error("Cannot mix IX and IY register parts");
                    return { prefix, (uint8_t)(0x40 | (dest_code << 3) | src_code) };
                }
                return { (uint8_t)(0x40 | (dest_code << 3) | src_code) };
            }

            // LD r, n
            if (mnemonic == "LD" && ops[0].type == OperandType::REG8 && ops[1].type == OperandType::IMM8) {
                uint8_t dest_code = reg8_map.at(ops[0].str_val);
                return { (uint8_t)(0x06 | (dest_code << 3)), (uint8_t)ops[1].num_val };
            }
            if (mnemonic == "LD" && (ops[0].str_val == "IXH" || ops[0].str_val == "IXL" || ops[0].str_val == "IYH" || ops[0].str_val == "IYL") && ops[1].type == OperandType::IMM8) {
                return { prefix, (uint8_t)(0x06 | (reg8_map.at(ops[0].str_val) << 3)), (uint8_t)ops[1].num_val };
            }

            // LD rr, nn
            if (mnemonic == "LD" && ops[0].type == OperandType::REG16 && (ops[1].type == OperandType::IMM8 || ops[1].type == OperandType::IMM16)) {
                if (reg16_map.count(ops[0].str_val)) {
                    return { (uint8_t)(0x01 | (reg16_map.at(ops[0].str_val) << 4)), (uint8_t)(ops[1].num_val & 0xFF), (uint8_t)(ops[1].num_val >> 8) };
                }
                if (ops[0].str_val == "IX") {
                    return { 0xDD, 0x21, (uint8_t)(ops[1].num_val & 0xFF), (uint8_t)(ops[1].num_val >> 8) };
                }
                if (ops[0].str_val == "IY") {
                    return { 0xFD, 0x21, (uint8_t)(ops[1].num_val & 0xFF), (uint8_t)(ops[1].num_val >> 8) };
                }
            }

            // LD (rr), A
            if (mnemonic == "LD" && ops[0].type == OperandType::MEM_REG16 && ops[1].str_val == "A") {
                if (ops[0].str_val == "BC") return {0x02};
                if (ops[0].str_val == "DE") return {0x12};
            }

            // LD A, (rr)
            if (mnemonic == "LD" && ops[0].str_val == "A" && ops[1].type == OperandType::MEM_REG16) {
                if (ops[1].str_val == "BC") return {0x0A};
                if (ops[1].str_val == "DE") return {0x1A};
            }

            // LD (nn), A
            if (mnemonic == "LD" && ops[0].type == OperandType::MEM_IMM16 && ops[1].str_val == "A") {
                return { 0x32, (uint8_t)(ops[0].num_val & 0xFF), (uint8_t)(ops[0].num_val >> 8) };
            }

            // LD A, (nn)
            if (mnemonic == "LD" && ops[0].str_val == "A" && ops[1].type == OperandType::MEM_IMM16) {
                return { 0x3A, (uint8_t)(ops[1].num_val & 0xFF), (uint8_t)(ops[1].num_val >> 8) };
            }

            // LD (IX+d), n
            if (mnemonic == "LD" && ops[0].type == OperandType::MEM_INDEXED && ops[1].type == OperandType::IMM8) {
                uint8_t prefix = (ops[0].base_reg == "IX") ? 0xDD : 0xFD;
                return { prefix, 0x36, (uint8_t)ops[0].offset, (uint8_t)ops[1].num_val };
            }

            // LD r, (IX+d)
            if (mnemonic == "LD" && ops[0].type == OperandType::REG8 && ops[1].type == OperandType::MEM_INDEXED) {
                uint8_t prefix = (ops[1].base_reg == "IX") ? 0xDD : 0xFD;
                uint8_t reg_code = reg8_map.at(ops[0].str_val);
                return { prefix, (uint8_t)(0x46 | (reg_code << 3)), (uint8_t)ops[1].offset };
            }

            // LD (IX+d), r
            if (mnemonic == "LD" && ops[0].type == OperandType::MEM_INDEXED && ops[1].type == OperandType::REG8) {
                uint8_t prefix = (ops[0].base_reg == "IX") ? 0xDD : 0xFD;
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                return { prefix, (uint8_t)(0x70 | reg_code), (uint8_t)ops[0].offset };
            }

            // ADD A, n
            if (mnemonic == "ADD" && ops[0].str_val == "A" && ops[1].type == OperandType::IMM8) {
                return { 0xC6, (uint8_t)ops[1].num_val };
            }

            // ADD/ADC/SUB/SBC/AND/XOR/OR/CP A, r
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") &&
                ops[0].str_val == "A" && ops[1].type == OperandType::REG8) {
                
                uint8_t base_opcode = 0;
                if (mnemonic == "ADD") base_opcode = 0x80; else if (mnemonic == "ADC") base_opcode = 0x88;
                else if (mnemonic == "SUB") base_opcode = 0x90; else if (mnemonic == "SBC") base_opcode = 0x98;
                else if (mnemonic == "AND") base_opcode = 0xA0; else if (mnemonic == "XOR") base_opcode = 0xA8;
                else if (mnemonic == "OR")  base_opcode = 0xB0; else if (mnemonic == "CP")  base_opcode = 0xB8;
                
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                if (prefix) return { prefix, (uint8_t)(base_opcode | reg_code) };
                return { (uint8_t)(base_opcode | reg_code) };
            }

            // JP cc, nn
            if (mnemonic == "JP" && ops[0].type == OperandType::CONDITION && (ops[1].type == OperandType::IMM8 || ops[1].type == OperandType::IMM16)) {
                uint8_t cond_code = condition_map.at(ops[0].str_val);
                return { (uint8_t)(0xC2 | (cond_code << 3)), (uint8_t)(ops[1].num_val & 0xFF), (uint8_t)(ops[1].num_val >> 8) };
            }

            // JR cc, d
            if (mnemonic == "JR" && ops[0].type == OperandType::CONDITION && (ops[1].type == OperandType::IMM8 || ops[1].type == OperandType::IMM16)) {
                // JR uses a different condition map
                const std::map<std::string, uint8_t> jr_condition_map = {
                    {"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}
                };
                if (jr_condition_map.count(ops[0].str_val)) {
                    int32_t target_addr = ops[1].num_val;
                    int32_t offset = target_addr - (m_current_address + 2);
                    if (m_pass == 2 && (offset < -128 || offset > 127)) {
                        throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                    }
                    return { jr_condition_map.at(ops[0].str_val), static_cast<uint8_t>(offset) };
                }
            }

            // IN r, (C)
            if (mnemonic == "IN" && ops[0].type == OperandType::REG8 && ops[1].str_val == "(C)") {
                uint8_t reg_code = reg8_map.at(ops[0].str_val);
                if (reg_code == 6) return {0xED, 0x70}; // IN (C) or IN F, (C)
                return {0xED, (uint8_t)(0x40 | (reg_code << 3))};
            }

            // OUT (C), r
            if (mnemonic == "OUT" && ops[0].str_val == "(C)" && ops[1].type == OperandType::REG8) {
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                if (reg_code == 6) { // OUT (C), (HL) is not a valid instruction
                    throw std::runtime_error("OUT (C), (HL) is not a valid instruction");
                }
                return {0xED, (uint8_t)(0x41 | (reg_code << 3))};
            }

            // BIT b, r
            if (mnemonic == "BIT" && ops[0].type == OperandType::IMM8 && ops[1].type == OperandType::REG8) {
                if (ops[0].num_val > 7) {
                    throw std::runtime_error("BIT index must be 0-7");
                }
                uint8_t bit = ops[0].num_val;
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                return { 0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code) };
            }

            // SET b, r
            if (mnemonic == "SET" && ops[0].type == OperandType::IMM8 && ops[1].type == OperandType::REG8) {
                if (ops[0].num_val > 7) {
                    throw std::runtime_error("SET index must be 0-7");
                }
                uint8_t bit = ops[0].num_val;
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                return { 0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code) };
            }

            // RES b, r
            if (mnemonic == "RES" && ops[0].type == OperandType::IMM8 && ops[1].type == OperandType::REG8) {
                if (ops[0].num_val > 7) {
                    throw std::runtime_error("RES index must be 0-7");
                }
                uint8_t bit = ops[0].num_val;
                uint8_t reg_code = reg8_map.at(ops[1].str_val);
                return { 0xCB, (uint8_t)(0x80 | (bit << 3) | reg_code) };
            }

            // SLL b, r
            if ((mnemonic == "SLL" || mnemonic == "SLI") && ops[0].type == OperandType::REG8) {
                if (ops[0].num_val > 7) {
                    throw std::runtime_error("SLL bit index must be 0-7");
                }
                uint8_t reg_code = reg8_map.at(ops[0].str_val);
                return { 0xCB, (uint8_t)(0x30 | reg_code) };
            }

        }

        throw std::runtime_error("Unsupported or invalid instruction: " + mnemonic + " " + (operands_str.empty() ? "" : operands_str[0]));
    }
};

#endif //__Z80ASSEMBLE_H__