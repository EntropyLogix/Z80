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
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

// --- Forward declarations and Helper Structs ---

struct ParsedLine {
    std::string label;
    std::string mnemonic;
    std::vector<std::string> operands;
    bool is_equ = false;
    std::string equ_value;
};

enum class OperandType { REG8, REG16, IMM8, IMM16, MEM_IMM16, MEM_REG16, MEM_INDEXED, CONDITION, LABEL, UNKNOWN };

struct Operand {
    OperandType type = OperandType::UNKNOWN;
    std::string str_val;
    uint16_t num_val = 0;
    int8_t offset = 0;
    std::string base_reg; // For indexed addressing
};

enum class ParsePhase { SymbolTableBuild, GeneratingFinalCode };

inline bool is_number(const std::string& s, uint16_t& out_value) {
    std::string str = s;
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
    if (str.empty())
        return false;
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

class SourceLineParser {
public:
    ParsedLine parse(const std::string& line) {
        ParsedLine result;
        std::string processed_line = line;

        // Remove comments
        size_t comment_pos = processed_line.find(';');
        if (comment_pos != std::string::npos) {
            processed_line.erase(comment_pos);
        }

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

        if (processed_line.empty()) {
            return result;
        }

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
    Operand parse(const std::string& op_str, const std::map<std::string, uint16_t>& symbol_table, uint16_t current_address, ParsePhase phase) {
        Operand op;
        op.str_val = op_str;
        uint16_t num_val;

        if (op_str == "$") {
            op.num_val = current_address;
            op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
            return op;
        }
        if (s_reg8_names.count(op_str)) {
            op.type = OperandType::REG8;
            return op;
        }
        if (s_reg16_names.count(op_str)) {
            op.type = OperandType::REG16;
            return op;
        }
        if (s_condition_names.count(op_str)) {
            op.type = OperandType::CONDITION;
            return op;
        }
        if (is_number(op_str, num_val)) {
            op.num_val = num_val;
            op.type = (num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
            return op;
        }
        if (symbol_table.count(op_str)) {
            op.num_val = symbol_table.at(op_str);
            op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
            return op;
        }
        if (op_str.front() == '(' && op_str.back() == ')') {
            std::string inner = op_str.substr(1, op_str.length() - 2);
            if (s_reg16_names.count(inner)) {
                op.type = OperandType::MEM_REG16;
                op.str_val = inner;
                return op;
            }
            if (is_number(inner, num_val)) {
                op.type = OperandType::MEM_IMM16;
                op.num_val = num_val;
                return op;
            }
            if (symbol_table.count(inner)) {
                op.type = OperandType::MEM_IMM16;
                op.num_val = symbol_table.at(inner);
                return op;
            }
            size_t plus_pos = inner.find('+');
            size_t minus_pos = inner.find('-');
            size_t sign_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;
            if (sign_pos != std::string::npos) {
                op.base_reg = inner.substr(0, sign_pos);
                std::transform(op.base_reg.begin(), op.base_reg.end(), op.base_reg.begin(), ::toupper);
                if (op.base_reg == "IX" || op.base_reg == "IY") {
                    std::string offset_str = inner.substr(sign_pos);
                    auto result = std::from_chars(offset_str.data(), offset_str.data() + offset_str.size(), op.offset);
                    if (result.ec == std::errc()) {
                        op.type = OperandType::MEM_INDEXED;
                        return op;
                    }
                }
            }
        }
        if (op.type == OperandType::UNKNOWN) {
            if (phase == ParsePhase::SymbolTableBuild) {
                op.type = OperandType::LABEL;
            }
        }
        return op;
    }

private:
    inline static const std::set<std::string> s_reg8_names = {"B", "C", "D", "E", "H", "L", "(HL)", "A", "IXH", "IXL", "IYH", "IYL"};
    inline static const std::set<std::string> s_reg16_names = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
    inline static const std::set<std::string> s_condition_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
};
class InstructionEncoder {
public:
    bool encode(const std::string& mnemonic, const std::vector<Operand>& operands, const OperandParser& operand_parser, uint16_t current_address, ParsePhase phase, std::vector<uint8_t>& result) {
        if (operands.size() == 0) {
            if (encode_no_operand(mnemonic, result))
                return true;
        } else if (operands.size() == 1) {
            if (encode_one_operand(mnemonic, operands[0], operand_parser, current_address, phase, result))
                return true;
        } else if (operands.size() == 2) {
            if (encode_two_operands(mnemonic, operands[0], operands[1], operand_parser, current_address, phase, result))
                return true;
        }
        return false;
    }

private:
    inline static const std::map<std::string, uint8_t> reg8_map = {{"B", 0}, {"C", 1}, {"D", 2}, {"E", 3}, {"H", 4}, {"L", 5}, {"(HL)", 6}, {"A", 7}, {"IXH", 4}, {"IXL", 5}, {"IYH", 4}, {"IYL", 5}};
    inline static const std::map<std::string, uint8_t> reg16_map = {{"BC", 0}, {"DE", 1}, {"HL", 2}, {"SP", 3}};
    inline static const std::map<std::string, uint8_t> reg16_af_map = {{"BC", 0}, {"DE", 1}, {"HL", 2}, {"AF", 3}};
    inline static const std::map<std::string, uint8_t> condition_map = {{"NZ", 0}, {"Z", 1}, {"NC", 2}, {"C", 3}, {"PO", 4}, {"PE", 5}, {"P", 6}, {"M", 7}};

    bool encode_no_operand(const std::string& mnemonic, std::vector<uint8_t>& result) {
        {
            if (mnemonic == "NOP") {
                result.push_back(0x00);
                return true;
            }
            if (mnemonic == "HALT") {
                result.push_back(0x76);
                return true;
            }
            if (mnemonic == "DI") {
                result.push_back(0xF3);
                return true;
            }
            if (mnemonic == "EI") {
                result.push_back(0xFB);
                return true;
            }
            if (mnemonic == "EXX") {
                result.push_back(0xD9);
                return true;
            }
            if (mnemonic == "RET") {
                result.push_back(0xC9);
                return true;
            }
            if (mnemonic == "RETI") {
                result.insert(result.end(), {0xED, 0x4D});
                return true;
            }
            if (mnemonic == "RETN") {
                result.insert(result.end(), {0xED, 0x45});
                return true;
            }
            if (mnemonic == "RLCA") {
                result.push_back(0x07);
                return true;
            }
            if (mnemonic == "RRCA") {
                result.push_back(0x0F);
                return true;
            }
            if (mnemonic == "RLA") {
                result.push_back(0x17);
                return true;
            }
            if (mnemonic == "RRA") {
                result.push_back(0x1F);
                return true;
            }
            if (mnemonic == "DAA") {
                result.push_back(0x27);
                return true;
            }
            if (mnemonic == "CPL") {
                result.push_back(0x2F);
                return true;
            }
            if (mnemonic == "SCF") {
                result.push_back(0x37);
                return true;
            }
            if (mnemonic == "CCF") {
                result.push_back(0x3F);
                return true;
            }
            if (mnemonic == "LDI") {
                result.insert(result.end(), {0xED, 0xA0});
                return true;
            }
            if (mnemonic == "CPI") {
                result.insert(result.end(), {0xED, 0xA1});
                return true;
            }
            if (mnemonic == "INI") {
                result.insert(result.end(), {0xED, 0xA2});
                return true;
            }
            if (mnemonic == "OUTI") {
                result.insert(result.end(), {0xED, 0xA3});
                return true;
            }
            if (mnemonic == "LDD") {
                result.insert(result.end(), {0xED, 0xA8});
                return true;
            }
            if (mnemonic == "CPD") {
                result.insert(result.end(), {0xED, 0xA9});
                return true;
            }
            if (mnemonic == "IND") {
                result.insert(result.end(), {0xED, 0xAA});
                return true;
            }
            if (mnemonic == "OUTD") {
                result.insert(result.end(), {0xED, 0xAB});
                return true;
            }
            if (mnemonic == "LDIR") {
                result.insert(result.end(), {0xED, 0xB0});
                return true;
            }
            if (mnemonic == "CPIR") {
                result.insert(result.end(), {0xED, 0xB1});
                return true;
            }
            if (mnemonic == "INIR") {
                result.insert(result.end(), {0xED, 0xB2});
                return true;
            }
            if (mnemonic == "OTIR") {
                result.insert(result.end(), {0xED, 0xB3});
                return true;
            }
            if (mnemonic == "LDDR") {
                result.insert(result.end(), {0xED, 0xB8});
                return true;
            }
            if (mnemonic == "CPDR") {
                result.insert(result.end(), {0xED, 0xB9});
                return true;
            }
            if (mnemonic == "INDR") {
                result.insert(result.end(), {0xED, 0xBA});
                return true;
            }
            if (mnemonic == "OTDR") {
                result.insert(result.end(), {0xED, 0xBB});
                return true;
            }
            return false;
        }
    }

    bool encode_one_operand(const std::string& mnemonic, const Operand& op, const OperandParser& operand_parser, uint16_t current_address, ParsePhase phase, std::vector<uint8_t>& result) {
        if (mnemonic == "PUSH" && op.type == OperandType::REG16) {
            if (reg16_af_map.count(op.str_val)) {
                result.push_back((uint8_t)(0xC5 | (reg16_af_map.at(op.str_val) << 4)));
                return true;
            }
            if (op.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0xE5});
                return true;
            }
            if (op.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0xE5});
                return true;
            }
        }
        if (mnemonic == "POP" && op.type == OperandType::REG16) {
            if (reg16_af_map.count(op.str_val)) {
                result.push_back((uint8_t)(0xC1 | (reg16_af_map.at(op.str_val) << 4)));
                return true;
            }
            if (op.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0xE1});
                return true;
            }
            if (op.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0xE1});
                return true;
            }
        }
        if (mnemonic == "INC" && op.type == OperandType::REG16) {
            if (reg16_map.count(op.str_val)) {
                result.push_back((uint8_t)(0x03 | (reg16_map.at(op.str_val) << 4)));
                return true;
            }
            if (op.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0x23});
                return true;
            }
            if (op.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0x23});
                return true;
            }
        }
        if (mnemonic == "DEC" && op.type == OperandType::REG16) {
            if (reg16_map.count(op.str_val)) {
                result.push_back((uint8_t)(0x0B | (reg16_map.at(op.str_val) << 4)));
                return true;
            }
            if (op.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0x2B});
                return true;
            }
            if (op.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0x2B});
                return true;
            }
        }
        if (mnemonic == "INC" && op.type == OperandType::REG8) {
            result.push_back((uint8_t)(0x04 | (reg8_map.at(op.str_val) << 3)));
            return true;
        }
        if (mnemonic == "DEC" && op.type == OperandType::REG8) {
            result.push_back((uint8_t)(0x05 | (reg8_map.at(op.str_val) << 3)));
            return true;
        }
        if (mnemonic == "JP" && (op.type == OperandType::IMM8 || op.type == OperandType::IMM16 || op.type == OperandType::LABEL)) {
            result.insert(result.end(), {0xC3, (uint8_t)(op.num_val & 0xFF), (uint8_t)(op.num_val >> 8)});
            return true;
        }
        if (mnemonic == "JP" && op.type == OperandType::MEM_REG16) {
            if (op.str_val == "HL") {
                result.push_back(0xE9);
                return true;
            }
            if (op.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0xE9});
                return true;
            }
            if (op.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0xE9});
                return true;
            }
        }
        if (mnemonic == "JR" && (op.type == OperandType::IMM8 || op.type == OperandType::IMM16)) {
            int32_t target_addr = op.num_val;
            int32_t offset = (op.type == OperandType::LABEL) ? 0 : target_addr - (current_address + 2);
            if (phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127)) {
                throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
            }
            result.insert(result.end(), {0x18, static_cast<uint8_t>(offset)});
            return true;
        }
        if (mnemonic == "SUB" && op.type == OperandType::IMM8) {
            result.insert(result.end(), {0xD6, (uint8_t)op.num_val});
            return true;
        }
        if (mnemonic == "ADD" && op.type == OperandType::REG8) {
            result.push_back((uint8_t)(0x80 | reg8_map.at(op.str_val)));
            return true;
        }
        if (mnemonic == "DJNZ" && (op.type == OperandType::IMM8 || op.type == OperandType::IMM16)) {
            int32_t target_addr = op.num_val;
            int32_t offset = (op.type == OperandType::LABEL) ? 0 : target_addr - (current_address + 2);
            if (phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127)) {
                throw std::runtime_error("DJNZ jump target out of range. Offset: " + std::to_string(offset));
            }
            result.insert(result.end(), {0x10, static_cast<uint8_t>(offset)});
            return true;
        }
        return false;
    }

    bool encode_two_operands(const std::string& mnemonic, const Operand& op1, const Operand& op2, const OperandParser& operand_parser, uint16_t current_address, ParsePhase phase, std::vector<uint8_t>& result) {
        if (mnemonic == "EX" && op1.str_val == "AF" && op2.str_val == "AF'") {
            result.push_back(0x08);
            return true;
        }
        if (mnemonic == "EX" && op1.str_val == "DE" && op2.str_val == "HL") {
            result.push_back(0xEB);
            return true;
        }
        uint8_t prefix = 0;
        if (op1.base_reg == "IX" || op2.base_reg == "IX" || op1.str_val.find("IX") != std::string::npos || op2.str_val.find("IX") != std::string::npos)
            prefix = 0xDD;
        else if (op1.base_reg == "IY" || op2.base_reg == "IY" || op1.str_val.find("IY") != std::string::npos || op2.str_val.find("IY") != std::string::npos) {
            prefix = 0xFD;
        }
        if (mnemonic == "LD" && op1.type == OperandType::REG8 && op2.type == OperandType::REG8) {
            uint8_t dest_code = reg8_map.at(op1.str_val);
            uint8_t src_code = reg8_map.at(op2.str_val);
            if (prefix) {
                if ((op1.str_val.find("IX") != std::string::npos && op2.str_val.find("IY") != std::string::npos) || (op1.str_val.find("IY") != std::string::npos && op2.str_val.find("IX") != std::string::npos))
                    throw std::runtime_error("Cannot mix IX and IY register parts");
                result.insert(result.end(), {prefix, (uint8_t)(0x40 | (dest_code << 3) | src_code)});
                return true;
            }
            result.push_back((uint8_t)(0x40 | (dest_code << 3) | src_code));
            return true;
        }
        if (mnemonic == "LD" && op1.type == OperandType::REG8 && op2.type == OperandType::IMM8) {
            uint8_t dest_code = reg8_map.at(op1.str_val);
            result.insert(result.end(), {(uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val});
            return true;
        }
        if (mnemonic == "LD" && (op1.str_val == "IXH" || op1.str_val == "IXL" || op1.str_val == "IYH" || op1.str_val == "IYL") && op2.type == OperandType::IMM8) {
            result.insert(result.end(), {prefix, (uint8_t)(0x06 | (reg8_map.at(op1.str_val) << 3)), (uint8_t)op2.num_val});
            return true;
        }
        if (mnemonic == "LD" && op1.type == OperandType::REG16 && (op2.type == OperandType::IMM8 || op2.type == OperandType::IMM16 || op2.type == OperandType::LABEL)) {
            if (reg16_map.count(op1.str_val)) {
                result.insert(result.end(), {(uint8_t)(0x01 | (reg16_map.at(op1.str_val) << 4)), (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (op1.str_val == "IX") {
                result.insert(result.end(), {0xDD, 0x21, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (op1.str_val == "IY") {
                result.insert(result.end(), {0xFD, 0x21, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
        }
        if (mnemonic == "LD" && op1.type == OperandType::MEM_REG16 && op2.str_val == "A") {
            if (op1.str_val == "BC") {
                result.push_back(0x02);
                return true;
            }
            if (op1.str_val == "DE") {
                result.push_back(0x12);
                return true;
            }
        }
        if (mnemonic == "LD" && op1.str_val == "A" && op2.type == OperandType::MEM_REG16) {
            if (op2.str_val == "BC") {
                result.push_back(0x0A);
                return true;
            }
            if (op2.str_val == "DE") {
                result.push_back(0x1A);
                return true;
            }
        }
        if (mnemonic == "LD" && op1.type == OperandType::MEM_IMM16 && op2.str_val == "A") {
            result.insert(result.end(), {0x32, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
            return true;
        }
        if (mnemonic == "LD" && op1.str_val == "A" && op2.type == OperandType::MEM_IMM16) {
            result.insert(result.end(), {0x3A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
            return true;
        }
        if (mnemonic == "LD" && op1.type == OperandType::MEM_INDEXED && op2.type == OperandType::IMM8) {
            result.insert(result.end(), {prefix, 0x36, (uint8_t)op1.offset, (uint8_t)op2.num_val});
            return true;
        }
        if (mnemonic == "LD" && op1.type == OperandType::REG8 && op2.type == OperandType::MEM_INDEXED) {
            uint8_t reg_code = reg8_map.at(op1.str_val);
            result.insert(result.end(), {prefix, (uint8_t)(0x46 | (reg_code << 3)), (uint8_t)op2.offset});
            return true;
        }
        if (mnemonic == "LD" && op1.type == OperandType::MEM_INDEXED && op2.type == OperandType::REG8) {
            uint8_t reg_code = reg8_map.at(op2.str_val);
            result.insert(result.end(), {prefix, (uint8_t)(0x70 | reg_code), (uint8_t)op1.offset});
            return true;
        }
        if (mnemonic == "ADD" && op1.str_val == "A" && op2.type == OperandType::IMM8) {
            result.insert(result.end(), {0xC6, (uint8_t)op2.num_val});
            return true;
        }
        if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" || mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") && op1.str_val == "A" && op2.type == OperandType::REG8) {
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
                result.insert(result.end(), {prefix, (uint8_t)(base_opcode | reg_code)});
            else
                result.push_back((uint8_t)(base_opcode | reg_code));
            return true;
        }
        if (mnemonic == "JP" && op1.type == OperandType::CONDITION && (op2.type == OperandType::IMM8 || op2.type == OperandType::IMM16 || op2.type == OperandType::LABEL)) {
            uint8_t cond_code = condition_map.at(op1.str_val);
            result.insert(result.end(), {(uint8_t)(0xC2 | (cond_code << 3)), (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
            return true;
        }
        if (mnemonic == "JR" && (op1.type == OperandType::CONDITION && (op2.type == OperandType::IMM8 || op2.type == OperandType::IMM16))) {
            const std::map<std::string, uint8_t> jr_condition_map = {{"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}};
            if (jr_condition_map.count(op1.str_val)) {
                int32_t target_addr = op2.num_val;
                int32_t offset = target_addr - (current_address + 2);
                if (phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127)) {
                    throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                }
                result.insert(result.end(), {jr_condition_map.at(op1.str_val), static_cast<uint8_t>(offset)});
                return true;
            }
        }
        if (mnemonic == "IN" && op1.type == OperandType::REG8 && op2.str_val == "(C)") {
            uint8_t reg_code = reg8_map.at(op1.str_val);
            if (reg_code == 6) {
                result.insert(result.end(), {0xED, 0x70});
                return true;
            }
            result.insert(result.end(), {0xED, (uint8_t)(0x40 | (reg_code << 3))});
            return true;
        }
        if (mnemonic == "OUT" && op1.str_val == "(C)" && op2.type == OperandType::REG8) {
            uint8_t reg_code = reg8_map.at(op2.str_val);
            if (reg_code == 6) {
                throw std::runtime_error("OUT (C), (HL) is not a valid instruction");
            }
            result.insert(result.end(), {0xED, (uint8_t)(0x41 | (reg_code << 3))});
            return true;
        }
        if (mnemonic == "BIT" && op1.type == OperandType::IMM8 && op2.type == OperandType::REG8) {
            if (op1.num_val > 7) {
                throw std::runtime_error("BIT index must be 0-7");
            }
            uint8_t bit = op1.num_val;
            uint8_t reg_code = reg8_map.at(op2.str_val);
            result.insert(result.end(), {0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code)});
            return true;
        }
        if (mnemonic == "SET" && op1.type == OperandType::IMM8 && op2.type == OperandType::REG8) {
            if (op1.num_val > 7) {
                throw std::runtime_error("SET index must be 0-7");
            }
            uint8_t bit = op1.num_val;
            uint8_t reg_code = reg8_map.at(op2.str_val);
            result.insert(result.end(), {0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code)});
            return true;
        }
        if (mnemonic == "RES" && op1.type == OperandType::IMM8 && op2.type == OperandType::REG8) {
            if (op1.num_val > 7) {
                throw std::runtime_error("RES index must be 0-7");
            }
            uint8_t bit = op1.num_val;
            uint8_t reg_code = reg8_map.at(op2.str_val);
            result.insert(result.end(), {0xCB, (uint8_t)(0x80 | (bit << 3) | reg_code)});
            return true;
        }
        if ((mnemonic == "SLL" || mnemonic == "SLI") && op1.type == OperandType::REG8) {
            if (op1.num_val > 7) {
                throw std::runtime_error("SLL bit index must be 0-7");
            }
            uint8_t reg_code = reg8_map.at(op1.str_val);
            result.insert(result.end(), {0xCB, (uint8_t)(0x30 | reg_code)});
            return true;
        }
        return false;
    }
};
template <typename TMemory> class Z80Assembler {
public:
    Z80Assembler(TMemory* memory) : m_memory(memory) {
    }

    bool assemble(const std::string& source_code, std::vector<uint8_t>& machine_code, uint16_t default_org = 0x0000) {
        m_symbol_table.clear();
        machine_code.clear();

        std::stringstream ss(source_code);
        std::vector<std::string> lines;
        std::string line;

        while (std::getline(ss, line))
            lines.push_back(line);

        m_current_address = default_org;
        m_phase = ParsePhase::SymbolTableBuild;
        for (const auto& l : lines) {
            process_line(l);
        }

        m_current_address = default_org;
        m_phase = ParsePhase::GeneratingFinalCode;
        for (const auto& l : lines) {
            auto bytes = process_line(l);
            machine_code.insert(machine_code.end(), bytes.begin(), bytes.end());
        }

        return true;
    }

private:
    ParsePhase m_phase;
    uint16_t m_current_address = 0;
    TMemory* m_memory = nullptr;
    std::map<std::string, uint16_t> m_symbol_table;

    SourceLineParser m_line_parser;
    OperandParser m_operand_parser;
    InstructionEncoder m_encoder;

    std::vector<uint8_t> process_line(const std::string& line) {
        ParsedLine parsed = m_line_parser.parse(line);
        std::vector<uint8_t> generated_bytes;

        switch (m_phase) {
            case ParsePhase::SymbolTableBuild:
                if (parsed.is_equ) {
                    uint16_t value;
                    if (is_number(parsed.equ_value, value)) {
                        if (m_symbol_table.count(parsed.label)) {
                            throw std::runtime_error("Duplicate symbol definition: " + parsed.label);
                        }
                        m_symbol_table[parsed.label] = value;
                    } else {
                        throw std::runtime_error("Invalid value for EQU: " + parsed.equ_value);
                    }
                    return {};
                }

                if (!parsed.label.empty()) {
                    if (m_symbol_table.count(parsed.label)) {
                        throw std::runtime_error("Duplicate label definition: " + parsed.label);
                    }
                    m_symbol_table[parsed.label] = m_current_address;
                }
                break;

            case ParsePhase::GeneratingFinalCode:
                if (parsed.is_equ) {
                    return {};
                }
                break;
        }

        if (parsed.mnemonic.empty()) {
            return {};
        }

        std::vector<Operand> ops;
        for (const auto& s : parsed.operands) {
            ops.push_back(m_operand_parser.parse(s, m_symbol_table, m_current_address, m_phase));
        }

        generated_bytes = assemble_instruction(parsed.mnemonic, ops);
        m_current_address += generated_bytes.size();

        if (m_phase == ParsePhase::SymbolTableBuild) {
            return {};
        }

        return generated_bytes;
    }

    std::vector<uint8_t> assemble_instruction(const std::string& mnemonic, const std::vector<Operand>& ops) {
        // Handle ORG directive
        if (mnemonic == "ORG") {
            if (ops.size() == 1 && (ops[0].type == OperandType::IMM8 || ops[0].type == OperandType::IMM16 || ops[0].type == OperandType::LABEL)) {
                m_current_address = ops[0].num_val;
                return {}; // ORG doesn't generate bytes
            } else {
                throw std::runtime_error("Invalid operand for ORG directive");
            }
        }
        // --- Pseudo-instructions ---
        if (mnemonic == "DB" || mnemonic == "DEFB") {
            std::vector<uint8_t> bytes;
            for (const auto& op : ops) {
                if (op.type == OperandType::IMM8 || op.type == OperandType::IMM16 || op.type == OperandType::LABEL) {
                    if (op.num_val > 0xFF) {
                        throw std::runtime_error("Value in DB statement exceeds 1 byte: " + op.str_val);
                    }
                    bytes.push_back(static_cast<uint8_t>(op.num_val));
                } else if (op.str_val.front() == '"' && op.str_val.back() == '"') {
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
                if (op.type == OperandType::IMM8 || op.type == OperandType::IMM16 || op.type == OperandType::LABEL) {
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
            if (ops[0].type != OperandType::IMM8 && ops[0].type != OperandType::IMM16 && ops[0].type != OperandType::LABEL) {
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

        std::vector<uint8_t> encoded_bytes;
        if (!m_encoder.encode(mnemonic, ops, m_operand_parser, m_current_address, m_phase, encoded_bytes)) {
            std::string ops_str;
            for (const auto& op : ops) {
                ops_str += op.str_val + " ";
            }
            throw std::runtime_error("Unsupported or invalid instruction: " + mnemonic + " " + ops_str);
        }
        return encoded_bytes;
    }
};

#endif //__Z80ASSEMBLE_H__
