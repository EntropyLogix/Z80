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

template <typename TMemory> class Z80Assembler {
public:
    Z80Assembler(TMemory* memory)
        : m_memory(memory), m_operand_parser(m_symbol_table, m_current_address, m_phase), m_encoder(m_memory, m_current_address, m_phase) {
    }

    bool assemble(const std::string& source_code, uint16_t default_org = 0x0000) {
        m_symbol_table.clear(); // Use the new class method

        std::stringstream ss(source_code);
        std::vector<std::string> lines;
        std::string line;

        while (std::getline(ss, line))
            lines.push_back(line);

        m_current_address = default_org;
        m_phase = ParsePhase::SymbolTableBuild;
        for (const auto& l : lines)
            process_line(l);

        m_current_address = default_org;
        m_phase = ParsePhase::GeneratingFinalCode;
        for (const auto& l : lines)
            process_line(l);

        return true;
    }

private:
    enum class ParsePhase { SymbolTableBuild, GeneratingFinalCode };

    class SymbolTable {
    public:
        void add(const std::string& name, uint16_t value) {
            if (is_symbol(name)) {
                throw std::runtime_error("Duplicate symbol definition: " + name);
            }
            m_symbols[name] = value;
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
        }

    private:
        std::map<std::string, uint16_t> m_symbols;
    };

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

        enum class OperandType { REG8, REG16, IMM8, IMM16, MEM_IMM16, MEM_REG16, MEM_INDEXED, CONDITION, UNKNOWN };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            uint16_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };

        Operand parse(const std::string& op_str) {
            Operand op;
            op.str_val = op_str;
            uint16_t num_val;
            if (is_indexed(op_str)) {
                std::string inner = op_str.substr(1, op_str.length() - 2);
                inner.erase(0, inner.find_first_not_of(" \t"));
                inner.erase(inner.find_last_not_of(" \t") + 1);
                if (is_reg16(inner)) {
                    op.type = OperandType::MEM_REG16;
                    op.str_val = inner;
                    return op;
                }
                if (is_number(inner, num_val)) {
                    op.type = OperandType::MEM_IMM16;
                    op.num_val = num_val;
                    return op;
                }
                if (is_number(inner, num_val)) {
                    op.type = OperandType::MEM_IMM16;
                    op.num_val = num_val;
                    return op;
                }
                if (parse_offset(inner, op.base_reg, op.offset)) {
                    std::transform(op.base_reg.begin(), op.base_reg.end(), op.base_reg.begin(), ::toupper);
                    if (op.base_reg == "IX" || op.base_reg == "IY")
                        op.type = OperandType::MEM_INDEXED;
                    return op;
                }
                if (evaluate_expression(inner, op.num_val)) {
                    op.type = OperandType::MEM_IMM16;
                    return op;
                }

                if (m_phase == ParsePhase::GeneratingFinalCode && m_symbol_table.is_symbol(inner)) {
                    op.type = OperandType::MEM_IMM16;
                    op.num_val = m_symbol_table.get_value(inner, m_current_address);
                    return op;
                }
            }
            if (is_reg8(op_str)) {
                op.type = OperandType::REG8;
                return op;
            }
            if (is_reg16(op_str)) {
                op.type = OperandType::REG16;
                return op;
            }
            if (is_condition(op_str)) {
                op.type = OperandType::CONDITION;
                return op;
            }
            if (is_number(op_str, num_val)) {
                op.num_val = num_val;
                op.type = (num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
                return op;
            }
            if (evaluate_expression(op_str, op.num_val)) {
                op.type = (op.num_val <= 0xFF) ? OperandType::IMM8 : OperandType::IMM16;
                return op;
            }
            if (m_phase == ParsePhase::SymbolTableBuild) {
                op.type = OperandType::IMM16;
            }
            return op;
        }

    private:
        inline bool is_indexed(const std::string& s) const {
            return !s.empty() && s.front() == '(' && s.back() == ')';
        }

        struct ExpressionToken {
            enum class Type { UNKNOWN, NUMBER, SYMBOL, OPERATOR, LPAREN, RPAREN };
            Type type = Type::UNKNOWN;
            std::string s_val;
            uint16_t n_val = 0;
            int precedence = 0;
            bool left_assoc = true;
        };

        std::vector<ExpressionToken> tokenize_expression(const std::string& expr) const {
            std::vector<ExpressionToken> tokens;
            for (size_t i = 0; i < expr.length(); ++i) {
                char c = expr[i];
                if (isspace(c)) continue;

                if (isalpha(c) || c == '_' || c == '$') { // Symbol or register
                    size_t j = i;
                    while (j < expr.length() && (isalnum(expr[j]) || expr[j] == '_')) {
                        j++;
                    }
                    tokens.push_back({ExpressionToken::Type::SYMBOL, expr.substr(i, j - i)});
                    i = j - 1;
                } else if (isdigit(c) || (c == '0' && (expr[i+1] == 'x' || expr[i+1] == 'X'))) { // Number
                    size_t j = i;
                    if (expr.substr(i, 2) == "0x" || expr.substr(i, 2) == "0X") j += 2;
                    while (j < expr.length() && isxdigit(expr[j])) {
                        j++;
                    }
                    if (j < expr.length() && (expr[j] == 'h' || expr[j] == 'H')) j++;
                    
                    uint16_t val;
                    if (is_number(expr.substr(i, j - i), val)) {
                        tokens.push_back({ExpressionToken::Type::NUMBER, "", val});
                    } else {
                         throw std::runtime_error("Invalid number in expression: " + expr.substr(i, j - i));
                    }
                    i = j - 1;
                } else if (c == '+' || c == '-' || c == '*' || c == '/') {
                    int precedence = (c == '+' || c == '-') ? 1 : 2;
                    tokens.push_back({ExpressionToken::Type::OPERATOR, std::string(1, c), 0, precedence, true});
                } else if (c == '(') {
                    tokens.push_back({ExpressionToken::Type::LPAREN, "("});
                } else if (c == ')') {
                    tokens.push_back({ExpressionToken::Type::RPAREN, ")"});
                } else {
                    throw std::runtime_error("Invalid character in expression: " + std::string(1, c));
                }
            }
            return tokens;
        }

        std::vector<ExpressionToken> shunting_yard(const std::vector<ExpressionToken>& infix) const {
            std::vector<ExpressionToken> postfix;
            std::vector<ExpressionToken> op_stack;
            for (const auto& token : infix) {
                switch (token.type) {
                    case ExpressionToken::Type::NUMBER:
                    case ExpressionToken::Type::SYMBOL:
                        postfix.push_back(token);
                        break;
                    case ExpressionToken::Type::OPERATOR:
                        while (!op_stack.empty() && op_stack.back().type == ExpressionToken::Type::OPERATOR &&
                               ((op_stack.back().precedence > token.precedence) ||
                                (op_stack.back().precedence == token.precedence && token.left_assoc))) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        op_stack.push_back(token);
                        break;
                    case ExpressionToken::Type::LPAREN:
                        op_stack.push_back(token);
                        break;
                    case ExpressionToken::Type::RPAREN:
                        while (!op_stack.empty() && op_stack.back().type != ExpressionToken::Type::LPAREN) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        if (op_stack.empty()) throw std::runtime_error("Mismatched parentheses in expression.");
                        op_stack.pop_back(); // Pop the LPAREN
                        break;
                    default: break;
                }
            }
            while (!op_stack.empty()) {
                if (op_stack.back().type == ExpressionToken::Type::LPAREN) throw std::runtime_error("Mismatched parentheses in expression.");
                postfix.push_back(op_stack.back());
                op_stack.pop_back();
            }
            return postfix;
        }

        uint16_t evaluate_rpn(const std::vector<ExpressionToken>& rpn) const {
            std::vector<uint16_t> val_stack;
            for (const auto& token : rpn) {
                if (token.type == ExpressionToken::Type::NUMBER) {
                    val_stack.push_back(token.n_val);
                } else if (token.type == ExpressionToken::Type::SYMBOL) {
                    val_stack.push_back(m_symbol_table.get_value(token.s_val, m_current_address));
                } else if (token.type == ExpressionToken::Type::OPERATOR) {
                    if (val_stack.size() < 2) throw std::runtime_error("Invalid expression syntax.");
                    uint16_t v2 = val_stack.back(); val_stack.pop_back();
                    uint16_t v1 = val_stack.back(); val_stack.pop_back();
                    if (token.s_val == "+") val_stack.push_back(v1 + v2);
                    else if (token.s_val == "-") val_stack.push_back(v1 - v2);
                    else if (token.s_val == "*") val_stack.push_back(v1 * v2);
                    else if (token.s_val == "/") {
                        if (v2 == 0) throw std::runtime_error("Division by zero in expression.");
                        val_stack.push_back(v1 / v2);
                    }
                }
            }
            if (val_stack.size() != 1) throw std::runtime_error("Invalid expression syntax.");
            return val_stack.back();
        }

        bool evaluate_expression(const std::string& s, uint16_t& out_value) const {
            try {
                auto tokens = tokenize_expression(s);
                if (m_phase == ParsePhase::SymbolTableBuild) {
                    for(const auto& token : tokens) {
                        if (token.type == ExpressionToken::Type::SYMBOL && !m_symbol_table.is_symbol(token.s_val)) {
                             out_value = 0;
                             return true;
                        }
                    }
                }
                auto rpn = shunting_yard(tokens);
                out_value = evaluate_rpn(rpn);
                return true;
            } catch (const std::exception&) {
                if (m_symbol_table.is_symbol(s)) {
                    if (m_phase == ParsePhase::GeneratingFinalCode) {
                        out_value = m_symbol_table.get_value(s, m_current_address);
                    } else {
                        out_value = 0;
                    }
                    return true;
                }
                return false;
            }
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

        bool parse_offset(const std::string& s, std::string& out_base, int16_t& out_offset) const {
            size_t plus_pos = s.find('+');
            size_t minus_pos = s.find('-');
            if (plus_pos == 0 || minus_pos == 0)
                return false;
            size_t sign_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;
            if (sign_pos != std::string::npos) {
                out_base = s.substr(0, sign_pos);
                out_base.erase(out_base.find_last_not_of(" \t") + 1);
                std::string offset_str = s.substr(sign_pos);
                offset_str.erase(0, offset_str.find_first_not_of(" \t"));
                std::string offset_val_str = offset_str.substr(1);
                offset_val_str.erase(0, offset_val_str.find_first_not_of(" \t"));

                uint16_t num_val;
                if (is_number(offset_val_str, num_val)) {
                    if (offset_str.front() == '-')
                        out_offset = -static_cast<int16_t>(num_val);
                    else
                        out_offset = static_cast<int16_t>(num_val);
                    return true;
                } else if (m_phase == ParsePhase::GeneratingFinalCode && m_symbol_table.is_symbol(offset_val_str)) {
                    num_val = m_symbol_table.get_value(offset_val_str, m_current_address);
                    if (offset_str.front() == '-')
                        out_offset = -static_cast<int16_t>(num_val);
                    else
                        out_offset = static_cast<int16_t>(num_val);
                    return true;
                } 
            }
            return false;
        }
        inline static const std::set<std::string> s_reg8_names = {"B",    "C", "D",   "E",   "H",   "L",
                                                                  "(HL)", "A", "IXH", "IXL", "IYH", "IYL"};
        inline static const std::set<std::string> s_reg16_names = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
        inline static const std::set<std::string> s_condition_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};

        const SymbolTable& m_symbol_table;
        const uint16_t& m_current_address;
        const ParsePhase& m_phase;
    };
    class InstructionEncoder {
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
        template <typename... Args>
        void assemble(Args... args) {
            if (m_phase == ParsePhase::GeneratingFinalCode)
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

        bool encode_one_operand(const std::string& mnemonic, const typename OperandParser::Operand& op) {
            if (mnemonic == "PUSH" && op.type == OperandParser::OperandType::REG16) {
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
            if (mnemonic == "POP" && op.type == OperandParser::OperandType::REG16) {
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
            if (mnemonic == "INC" && op.type == OperandParser::OperandType::REG16) {
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
            if (mnemonic == "DEC" && op.type == OperandParser::OperandType::REG16) {
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
            if (mnemonic == "INC" && op.type == OperandParser::OperandType::REG8) {
                assemble((uint8_t)(0x04 | (reg8_map.at(op.str_val) << 3)));
                return true;
            }
            if (mnemonic == "DEC" && op.type == OperandParser::OperandType::REG8) {
                assemble((uint8_t)(0x05 | (reg8_map.at(op.str_val) << 3)));
                return true;
            }
            if (mnemonic == "JP" &&
                (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16)) {
                assemble(0xC3, (uint8_t)(op.num_val & 0xFF),
                         (uint8_t)(op.num_val >> 8));
                return true;
            }
            if (mnemonic == "JP" && op.type == OperandParser::OperandType::MEM_REG16) {
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
            if (mnemonic == "JR" &&
                (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16)) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_current_address + instruction_size);
                if (m_phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127))
                    throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                assemble(0x18, static_cast<uint8_t>(offset));
                return true;
            }
            if (mnemonic == "SUB" && op.type == OperandParser::OperandType::IMM8) {
                assemble(0xD6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "ADD" && op.type == OperandParser::OperandType::REG8) {
                assemble((uint8_t)(0x80 | reg8_map.at(op.str_val)));
                return true;
            }
            if (mnemonic == "DJNZ" &&
                (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16)) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_current_address + instruction_size);
                if (m_phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127))
                    throw std::runtime_error("DJNZ jump target out of range. Offset: " + std::to_string(offset));
                assemble(0x10, static_cast<uint8_t>(offset));
                return true;
            }
            return false;
        }

        bool encode_two_operands(const std::string& mnemonic, const typename OperandParser::Operand& op1,
                                 const typename OperandParser::Operand& op2) {
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
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::REG8 &&
                op2.type == OperandParser::OperandType::REG8) {
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
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::REG8 &&
                op2.type == OperandParser::OperandType::IMM8) {
                uint8_t dest_code = reg8_map.at(op1.str_val);
                assemble((uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" &&
                (op1.str_val == "IXH" || op1.str_val == "IXL" || op1.str_val == "IYH" || op1.str_val == "IYL") &&
                op2.type == OperandParser::OperandType::IMM8) {
                assemble(prefix, (uint8_t)(0x06 | (reg8_map.at(op1.str_val) << 3)),
                         (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::REG16 &&
                (op2.type == OperandParser::OperandType::IMM8 || op2.type == OperandParser::OperandType::IMM16)) {
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
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::MEM_REG16 && op2.str_val == "A") {
                if (op1.str_val == "BC") {
                    assemble(0x02);
                    return true;
                }
                if (op1.str_val == "DE") {
                    assemble(0x12);
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.str_val == "A" && op2.type == OperandParser::OperandType::MEM_REG16) {
                if (op2.str_val == "BC") {
                    assemble(0x0A);
                    return true;
                }
                if (op2.str_val == "DE") {
                    assemble(0x1A);
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::MEM_IMM16 && op2.str_val == "A") {
                assemble(0x32, (uint8_t)(op1.num_val & 0xFF),
                         (uint8_t)(op1.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && op2.type == OperandParser::OperandType::MEM_IMM16) {
                assemble(0x3A, (uint8_t)(op2.num_val & 0xFF),
                         (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::MEM_INDEXED &&
                op2.type == OperandParser::OperandType::IMM8) {
                assemble(prefix, 0x36, static_cast<int8_t>(op1.offset), (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::REG8 &&
                op2.type == OperandParser::OperandType::MEM_INDEXED) {
                uint8_t reg_code = reg8_map.at(op1.str_val);
                assemble(prefix, (uint8_t)(0x46 | (reg_code << 3)),
                         static_cast<int8_t>(op2.offset));
                return true;
            }
            if (mnemonic == "LD" && op1.type == OperandParser::OperandType::MEM_INDEXED &&
                op2.type == OperandParser::OperandType::REG8) {
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(prefix, (uint8_t)(0x70 | reg_code), static_cast<int8_t>(op1.offset));
                return true;
            }
            if (mnemonic == "ADD" && op1.str_val == "A" && op2.type == OperandParser::OperandType::IMM8) {
                assemble(0xC6, (uint8_t)op2.num_val);
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") &&
                op1.str_val == "A" && op2.type == OperandParser::OperandType::REG8) {
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
            if (mnemonic == "JP" && op1.type == OperandParser::OperandType::CONDITION &&
                (op2.type == OperandParser::OperandType::IMM8 || op2.type == OperandParser::OperandType::IMM16)) {
                uint8_t cond_code = condition_map.at(op1.str_val);
                assemble((uint8_t)(0xC2 | (cond_code << 3)),
                         (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "JR" &&
                (op1.type == OperandParser::OperandType::CONDITION &&
                 (op2.type == OperandParser::OperandType::IMM8 || op2.type == OperandParser::OperandType::IMM16))) {
                const std::map<std::string, uint8_t> jr_condition_map = {
                    {"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}};
                if (jr_condition_map.count(op1.str_val)) {
                    int32_t target_addr = op2.num_val;
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_current_address + instruction_size);
                    if (m_phase == ParsePhase::GeneratingFinalCode && (offset < -128 || offset > 127))
                        throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                    assemble(jr_condition_map.at(op1.str_val),
                             static_cast<uint8_t>(offset));
                    return true;
                }
            }
            if (mnemonic == "IN" && op1.type == OperandParser::OperandType::REG8 && op2.str_val == "(C)") {
                uint8_t reg_code = reg8_map.at(op1.str_val);
                if (reg_code == 6) {
                    assemble(0xED, 0x70);
                    return true;
                }
                assemble(0xED, (uint8_t)(0x40 | (reg_code << 3)));
                return true;
            }
            if (mnemonic == "OUT" && op1.str_val == "(C)" && op2.type == OperandParser::OperandType::REG8) {
                uint8_t reg_code = reg8_map.at(op2.str_val);
                if (reg_code == 6)
                    throw std::runtime_error("OUT (C), (HL) is not a valid instruction");
                assemble(0xED, (uint8_t)(0x41 | (reg_code << 3)));
                return true;
            }
            if (mnemonic == "BIT" && op1.type == OperandParser::OperandType::IMM8 &&
                op2.type == OperandParser::OperandType::REG8) {
                if (op1.num_val > 7)
                    throw std::runtime_error("BIT index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "SET" && op1.type == OperandParser::OperandType::IMM8 &&
                op2.type == OperandParser::OperandType::REG8) {
                if (op1.num_val > 7)
                    throw std::runtime_error("SET index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "RES" && op1.type == OperandParser::OperandType::IMM8 &&
                op2.type == OperandParser::OperandType::REG8) {
                if (op1.num_val > 7)
                    throw std::runtime_error("RES index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0x80 | (bit << 3) | reg_code));
                return true;
            }
            if ((mnemonic == "SLL" || mnemonic == "SLI") && op1.type == OperandParser::OperandType::REG8) {
                if (op1.num_val > 7) {
                    throw std::runtime_error("SLL bit index must be 0-7");
                }
                uint8_t reg_code = reg8_map.at(op1.str_val);
                assemble(0xCB, (uint8_t)(0x30 | reg_code));
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
        case ParsePhase::SymbolTableBuild:
            if (parsed.is_equ) {
                uint16_t value;
                if (is_number(parsed.equ_value, value)) {
                    m_symbol_table.add(parsed.label, value);
                } else {
                    throw std::runtime_error("Invalid value for EQU: " + parsed.equ_value);
                }
                return;
            }

            if (!parsed.label.empty()) {
                m_symbol_table.add(parsed.label, m_current_address);
            }
            break;

        case ParsePhase::GeneratingFinalCode:
            if (parsed.is_equ)
                return;
            break;
        }

        if (parsed.mnemonic.empty())
            return;

        std::vector<typename OperandParser::Operand> ops;
        for (const auto& s : parsed.operands)
            ops.push_back(m_operand_parser.parse(s));

        assemble_instruction(parsed.mnemonic, ops);
    }

    inline static bool is_number(const std::string& s, uint16_t& out_value) {
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

    void assemble_instruction(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& ops) {
        // Handle ORG directive
        if (mnemonic == "ORG") {
            if (ops.size() == 1 &&
                (ops[0].type == OperandParser::OperandType::IMM8 || ops[0].type == OperandParser::OperandType::IMM16)) {
                m_current_address = ops[0].num_val;
                return; // ORG doesn't generate bytes
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
                    if (m_phase == ParsePhase::GeneratingFinalCode)
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val));
                    else
                        m_current_address++;
                } else if (op.str_val.front() == '"' && op.str_val.back() == '"') {
                    std::string str_content = op.str_val.substr(1, op.str_val.length() - 2);
                    for (char c : str_content) {
                        if (m_phase == ParsePhase::GeneratingFinalCode)
                            m_memory->poke(m_current_address++, static_cast<uint8_t>(c));
                        else
                            m_current_address++;
                    }
                } else {
                    throw std::runtime_error("Unsupported operand for DB: " + op.str_val);
                }
            }
            return;
        }

        if (mnemonic == "DW" || mnemonic == "DEFW") {
            for (const auto& op : ops) {
                if (op.type == OperandParser::OperandType::IMM8 || op.type == OperandParser::OperandType::IMM16) {
                    if (m_phase == ParsePhase::GeneratingFinalCode) {
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val & 0xFF));
                        m_memory->poke(m_current_address++, static_cast<uint8_t>(op.num_val >> 8));
                    } else
                        m_current_address += 2;
                } else {
                    throw std::runtime_error("Unsupported operand for DW: " + op.str_val);
                }
            }
            return;
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
            if (m_phase == ParsePhase::GeneratingFinalCode)
                for (size_t i = 0; i < count; ++i)
                    m_memory->poke(m_current_address++, fill_value);
            else
                m_current_address += count;
            return;
        }

        if (!m_encoder.encode(mnemonic, ops)) {
            std::string ops_str;
            for (const auto& op : ops) {
                ops_str += op.str_val + " ";
            }
            throw std::runtime_error("Unsupported or invalid instruction: " + mnemonic + " " + ops_str);
        }
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
