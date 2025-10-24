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
    struct OrgBlock {
        uint16_t start_address;
        size_t size;
    };

    enum class ParsePhase { SymbolTableBuild, CodeGeneration };

    Z80Assembler(TMemory* memory)
        : m_memory(memory), m_operand_parser(m_symbol_table, m_current_address, m_phase), m_encoder(m_memory, m_current_address, m_phase) {
    }

    bool compile(const std::string& source_code, uint16_t default_org = 0x0000) {
        std::stringstream ss(source_code);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ss, line))
            lines.push_back(line);
        
        try {
            m_phase = ParsePhase::SymbolTableBuild;
            m_symbol_table.clear();
            m_org_blocks.clear();
            m_org_blocks.push_back({default_org, 0});
            m_current_address = default_org;
            m_current_line_number = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                m_current_line_number = i + 1;
                process_line(lines[i]);
            }
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(m_current_line_number));
        }
        m_symbol_table.resolve_expressions(m_current_address);
        try {
            m_phase = ParsePhase::CodeGeneration;
            m_org_blocks.clear();
            m_org_blocks.push_back({default_org, 0});
            m_current_address = default_org;
            m_current_line_number = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                m_current_line_number = i + 1;
                process_line(lines[i]);
            }
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(m_current_line_number));
        }
        return true;
    }

    const std::vector<OrgBlock>& get_org_blocks() const {
        return m_org_blocks;
    }

    class SymbolTable {
    public:
        void add(const std::string& name, uint16_t value) {
            if (is_symbol(name))
                throw std::runtime_error("Duplicate symbol definition: " + name);
            m_symbols[name] = value;
        }
        void add_expression(const std::string& name, const std::string& expression) {
            if (is_symbol(name) || m_expressions.count(name))
                throw std::runtime_error("Duplicate symbol definition: " + name);
            m_expressions[name] = expression;
        }
        bool is_symbol(const std::string& name) const {
            if (name == "$" || name == "_")
                return true;
            return m_symbols.count(name);
        }
        uint16_t get_value(const std::string& name, uint16_t current_address) const {
            if (name == "$" || name == "_")
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

    class ExpressionEvaluator {
    public:
        ExpressionEvaluator(const SymbolTable& symbol_table, const uint16_t& current_address, const ParsePhase& phase)
            : m_symbol_table(symbol_table), m_current_address(current_address) {}

        bool evaluate(const std::string& s, uint16_t& out_value) const {
            try {
                auto tokens = tokenize_expression(s);
                auto rpn = shunting_yard(tokens);
                out_value = evaluate_rpn(rpn);
                return true;
            } catch (const std::exception&) {
                if (m_symbol_table.is_symbol(s)) {
                    out_value = m_symbol_table.get_value(s, m_current_address);
                    return true;
                }
                return false;
            }
        }

    private:
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
                if (isspace(c))
                    continue;
                if (isalpha(c) || c == '_' || c == '$') { // Symbol or register
                    size_t j = i;
                    while (j < expr.length() && (isalnum(expr[j]) || expr[j] == '_')) {
                        j++;
                    }
                    tokens.push_back({ExpressionToken::Type::SYMBOL, expr.substr(i, j - i)});
                    i = j - 1;
                } else if (isdigit(c) || (c == '0' && (expr[i+1] == 'x' || expr[i+1] == 'X'))) { // Number
                    size_t j = i;
                    if (expr.substr(i, 2) == "0x" || expr.substr(i, 2) == "0X")
                        j += 2;
                    while (j < expr.length() && isxdigit(expr[j]))
                        j++;
                    if (j < expr.length() && (expr[j] == 'h' || expr[j] == 'H'))
                        j++;
                    uint16_t val;
                    if (is_number(expr.substr(i, j - i), val)) {
                        tokens.push_back({ExpressionToken::Type::NUMBER, "", val});
                    } else
                         throw std::runtime_error("Invalid number in expression: " + expr.substr(i, j - i));
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
                        if (op_stack.empty())
                            throw std::runtime_error("Mismatched parentheses in expression.");
                        op_stack.pop_back(); // Pop the LPAREN
                        break;
                    default: break;
                }
            }
            while (!op_stack.empty()) {
                if (op_stack.back().type == ExpressionToken::Type::LPAREN)
                    throw std::runtime_error("Mismatched parentheses in expression.");
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
                    if (val_stack.size() < 2)
                        throw std::runtime_error("Invalid expression syntax.");
                    uint16_t v2 = val_stack.back();
                    val_stack.pop_back();
                    uint16_t v1 = val_stack.back();
                    val_stack.pop_back();
                    if (token.s_val == "+")
                        val_stack.push_back(v1 + v2);
                    else if (token.s_val == "-")
                        val_stack.push_back(v1 - v2);
                    else if (token.s_val == "*")
                        val_stack.push_back(v1 * v2);
                    else if (token.s_val == "/") {
                        if (v2 == 0) throw std::runtime_error("Division by zero in expression.");
                        val_stack.push_back(v1 / v2);
                    }
                }
            }
            if (val_stack.size() != 1)
                throw std::runtime_error("Invalid expression syntax.");
            return val_stack.back();
        }
        const SymbolTable& m_symbol_table;
        const uint16_t& m_current_address;
    };

    const SymbolTable& get_symbol_table() const {
        return m_symbol_table;
    }

private:
    class LineParser {
    public:
        struct ParsedLine {
            std::string label;
            std::string mnemonic;
            std::string original_mnemonic;
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
            result.original_mnemonic = result.mnemonic;
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

        enum class OperandType { REG8, REG16, IMMEDIATE, MEM_IMMEDIATE, MEM_REG16, MEM_INDEXED, CONDITION, STRING_LITERAL, EXPRESSION, UNKNOWN };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            uint16_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };

        Operand parse(const std::string& operand_string) {
            std::string upper_opperand_string = operand_string;
            std::transform(upper_opperand_string.begin(), upper_opperand_string.end(), upper_opperand_string.begin(), ::toupper);

            Operand operand;
            operand.str_val = operand_string;

            if (is_string_literal(operand_string)) {
                operand.type = OperandType::STRING_LITERAL;
                return operand;
            }
            if (is_reg8(upper_opperand_string)) {
                operand.type = OperandType::REG8;
                return operand;
            }
            if (is_reg16(upper_opperand_string)) {
                operand.type = OperandType::REG16;
                return operand;
            }
            if (is_condition(upper_opperand_string)) {
                operand.type = OperandType::CONDITION;
                return operand;
            }
            uint16_t num_val;
            if (is_number(operand_string, num_val)) {
                operand.num_val = num_val;
                operand.type = OperandType::IMMEDIATE;
                return operand;
            }
            if (is_mem_ptr(operand_string)) { // (HL), (IX+d), (nn)
                std::string inner = operand_string.substr(1, operand_string.length() - 2);
                inner.erase(0, inner.find_first_not_of(" \t"));
                inner.erase(inner.find_last_not_of(" \t") + 1);
                return parse_ptr(inner);
            }
            if (m_phase == ParsePhase::SymbolTableBuild && !m_symbol_table.is_symbol(upper_opperand_string)) {
                operand.type = OperandType::EXPRESSION;
                operand.str_val = operand_string;
                return operand;
            }
            if (m_symbol_table.is_symbol(upper_opperand_string)) {
                operand.num_val = m_symbol_table.get_value(upper_opperand_string, m_current_address); // Value might be 0 in pass 1, that's ok
                operand.type = OperandType::IMMEDIATE;
                return operand;
            }
            throw std::runtime_error("Unknown operand or undefined symbol: " + operand_string);
        }

    private:
        inline bool is_mem_ptr(const std::string& s) const {
            return !s.empty() && s.front() == '(' && s.back() == ')';
        }

        inline bool is_string_literal(const std::string& s) const {
            return s.length() > 1 && s.front() == '"' && s.back() == '"';
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
                op.type = OperandType::MEM_IMMEDIATE;
                op.num_val = inner_num_val;
            } else { // Assume it's a symbol
                op.type = OperandType::MEM_IMMEDIATE;
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

    public:
        bool encode_pseudo_instruction(const std::string& mnemonic, const std::vector<Operand>& ops) {
            if (mnemonic == "ORG") {
                if (ops.size() == 1 && match(ops[0], OperandType::IMMEDIATE)) {
                    m_current_address = ops[0].num_val;
                    return true;
                } else
                    throw std::runtime_error("Invalid operand for ORG directive");
            }
            if (mnemonic == "DB" || mnemonic == "DEFB") {
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        if (op.num_val > 0xFF)
                            throw std::runtime_error("Value in DB statement exceeds 1 byte: " + op.str_val);
                        assemble(static_cast<uint8_t>(op.num_val));
                    } else if (match(op, OperandType::STRING_LITERAL)) {
                        std::string str_content = op.str_val.substr(1, op.str_val.length() - 2);
                        for (char c : str_content) {
                            assemble(static_cast<uint8_t>(c));
                        }
                    } else
                        throw std::runtime_error("Unsupported operand for DB: " + op.str_val);
                }
                return true;
            }
            if (mnemonic == "DW" || mnemonic == "DEFW") {
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        assemble(static_cast<uint8_t>(op.num_val & 0xFF), static_cast<uint8_t>(op.num_val >> 8));
                    } else 
                        throw std::runtime_error("Unsupported operand for DW: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                return true;
            }
            if (mnemonic == "DS" || mnemonic == "DEFS") {
                if (ops.empty() || ops.size() > 2) {
                    throw std::runtime_error("DS/DEFS requires 1 or 2 operands.");
                }
                if (!match(ops[0], OperandType::IMMEDIATE)) {
                    throw std::runtime_error("DS/DEFS size must be a number.");
                }
                size_t count = ops[0].num_val;
                uint8_t fill_value = (ops.size() == 2) ? static_cast<uint8_t>(ops[1].num_val) : 0;
                for (size_t i = 0; i < count; ++i) {
                    assemble(fill_value);
                }
                return true;
            }
            return false;
        }

        InstructionEncoder(TMemory* memory, uint16_t& current_address, const ParsePhase& phase)
            : m_memory(memory), m_current_address(current_address), m_phase(phase) {}

        bool encode(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& operands) {
            if (encode_pseudo_instruction(mnemonic, operands))
                return true;
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
            if (op.type == type)
                return true;
            // During pass 1, an EXPRESSION can stand in for any value-based operand type
            if (m_phase == ParsePhase::SymbolTableBuild && op.type == OperandType::EXPRESSION) {
                return type == OperandType::IMMEDIATE || type == OperandType::MEM_IMMEDIATE;
            }
            return false;
        }

        // Check if an operand is an immediate value that fits in 8 bits
        bool match_imm8(const Operand& op) const {
            return match(op, OperandType::IMMEDIATE) && op.num_val <= 0xFF;
        }

        // Check if an operand is an immediate value (16-bit is the general case)
        bool match_imm16(const Operand& op) const {
            return match(op, OperandType::IMMEDIATE);
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
            if (mnemonic == "JP" && match_imm16(op)) {
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
            if (mnemonic == "JR" && match_imm16(op)) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_current_address + instruction_size);
                if (m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
                    throw std::runtime_error("JR jump target out of range. Offset: " + std::to_string(offset));
                assemble(0x18, static_cast<uint8_t>(offset));
                return true;
            }
            if (mnemonic == "SUB" && match_imm8(op)) {
                assemble(0xD6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "ADD" && match(op, OperandType::REG8)) {
                assemble((uint8_t)(0x80 | reg8_map.at(op.str_val)));
                return true;
            }
            if (mnemonic == "DJNZ" && match_imm16(op)) {
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
            if (mnemonic == "CP" && match_imm8(op)) {
                assemble(0xFE, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "AND" && match_imm8(op)) {
                assemble(0xE6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "OR" && match_imm8(op)) {
                assemble(0xF6, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "XOR" && match_imm8(op)) {
                assemble(0xEE, (uint8_t)op.num_val);
                return true;
            }
            if (mnemonic == "SUB" && match_imm8(op)) {
                assemble(0xD6, (uint8_t)op.num_val); // Note: This is SUB A, n - SUB n is ambiguous
                return true;
            }
            if (mnemonic == "CALL" && match_imm16(op)) {
                assemble(0xCD, (uint8_t)(op.num_val & 0xFF),
                         (uint8_t)(op.num_val >> 8));
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") && match(op, OperandType::REG8)) {
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

                uint8_t reg_code = reg8_map.at(op.str_val);
                assemble((uint8_t)(base_opcode | reg_code));
                return true;
            }

            if (mnemonic == "RET" && match(op, OperandType::CONDITION)) {
                if (condition_map.count(op.str_val)) {
                    uint8_t cond_code = condition_map.at(op.str_val);
                    assemble((uint8_t)(0xC0 | (cond_code << 3)));
                    return true;
                }
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
            if (mnemonic == "LD" && match(op1, OperandType::REG8) && match_imm8(op2)) {
                uint8_t dest_code = reg8_map.at(op1.str_val);
                assemble((uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" &&
                (op1.str_val == "IXH" || op1.str_val == "IXL" || op1.str_val == "IYH" || op1.str_val == "IYL") && match_imm8(op2)) {
                assemble(prefix, (uint8_t)(0x06 | (reg8_map.at(op1.str_val) << 3)),
                         (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::REG16) && match_imm16(op2)) {
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
            if (mnemonic == "LD" && match(op1, OperandType::MEM_REG16) && match_imm8(op2)) {
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
            if (mnemonic == "LD" && match(op1, OperandType::MEM_IMMEDIATE) && op2.str_val == "A") {
                assemble(0x32, (uint8_t)(op1.num_val & 0xFF),
                         (uint8_t)(op1.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match(op2, OperandType::MEM_IMMEDIATE)) {
                assemble(0x3A, (uint8_t)(op2.num_val & 0xFF),
                         (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match(op2, OperandType::MEM_IMMEDIATE)) {
                assemble(0x3A, (uint8_t)(op2.num_val & 0xFF),
                         (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "IN" && op1.str_val == "A" && match(op2, OperandType::MEM_IMMEDIATE)) {
                if (op2.num_val > 0xFF) throw std::runtime_error("Port for IN instruction must be 8-bit");
                assemble(0xDB, (uint8_t)op2.num_val);
                return true;
            }
            if (mnemonic == "OUT" && match(op1, OperandType::MEM_IMMEDIATE) && op2.str_val == "A" && op1.num_val <= 0xFF) {
                if (op1.num_val > 0xFF) throw std::runtime_error("Port for OUT instruction must be 8-bit");
                assemble(0xD3, (uint8_t)op1.num_val);
                return true;
            }
            if (mnemonic == "LD" && match(op1, OperandType::MEM_INDEXED) && match_imm8(op2)) {
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
            if (mnemonic == "ADD" && op1.str_val == "A" && match_imm8(op2)) {
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
            if (mnemonic == "JP" && match(op1, OperandType::CONDITION) && match_imm16(op2)) {
                uint8_t cond_code = condition_map.at(op1.str_val);
                assemble((uint8_t)(0xC2 | (cond_code << 3)),
                         (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "JR" && match(op1, OperandType::CONDITION) && match_imm16(op2)) {
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
            if (mnemonic == "CALL" && match(op1, OperandType::CONDITION) && match_imm16(op2)) {
                uint8_t cond_code = condition_map.at(op1.str_val);
                assemble((uint8_t)(0xC4 | (cond_code << 3)),
                         (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                return true;
            }
            if (mnemonic == "JR" && match(op1, OperandType::CONDITION) && match_imm16(op2)) {
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
            if (mnemonic == "BIT" && match_imm8(op1) && match(op2, OperandType::REG8)) {
                if (op1.num_val > 7)
                    throw std::runtime_error("BIT index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "SET" && match_imm8(op1) && match(op2, OperandType::REG8)) {
                if (op1.num_val > 7)
                    throw std::runtime_error("SET index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code = reg8_map.at(op2.str_val);
                assemble(0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code));
                return true;
            }
            if (mnemonic == "RES" && match_imm8(op1) && match(op2, OperandType::REG8)) {
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
                match_imm8(op1) && match(op2, OperandType::MEM_INDEXED)) {
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
        uint16_t address_before = m_current_address;


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
            std::string error_line = parsed.original_mnemonic;
            if (!parsed.operands.empty()) {
                error_line += " ";
                for (size_t i = 0; i < parsed.operands.size(); ++i)
                    error_line += parsed.operands[i] + (i < parsed.operands.size() - 1 ? "," : "");
            }
            throw std::runtime_error("Unsupported or invalid instruction: " + error_line);
        }
        if (!parsed.mnemonic.empty() && parsed.mnemonic == "ORG") {
            uint16_t org_addr = ops[0].num_val;
            m_org_blocks.push_back({org_addr, 0});
            return;
        }
        uint16_t bytes_generated = m_current_address - address_before;
        if (bytes_generated > 0 && !m_org_blocks.empty()) {
            m_org_blocks.back().size += bytes_generated;
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
        if (str.size() > 1 && str.front() == '+') {
            start += 1;
        }

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
        if (m_encoder.encode_pseudo_instruction(mnemonic, ops)) {
            return true;
        }
        return m_encoder.encode(mnemonic, ops);
    }

    ParsePhase m_phase;
    uint16_t m_current_address = 0;
    size_t m_current_line_number = 0;
    std::vector<OrgBlock> m_org_blocks;
    TMemory* m_memory = nullptr;
    typename Z80Assembler<TMemory>::SymbolTable m_symbol_table;
    typename Z80Assembler<TMemory>::LineParser m_line_parser;
    OperandParser m_operand_parser;
    InstructionEncoder m_encoder;
};

#endif //__Z80ASSEMBLE_H__
