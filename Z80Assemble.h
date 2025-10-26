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
#include <iterator>
#include <map>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <utility>
#include <vector>

template <typename TMemory> class Z80Assembler {
public:
    enum class ParsePhase { SymbolsGathering, CodeGeneration };

    Z80Assembler(TMemory* memory) {
        m_context.m_memory = memory;
    }

    bool compile(const std::string& source_code, uint16_t default_org = 0x0000) {
        std::stringstream source_stream(source_code);
        std::vector<std::string> source_lines;
        std::string line;
        while (std::getline(source_stream, line))
            source_lines.push_back(line);

        m_context.m_pass_count = 0;
        const int max_passes = 10;
        bool stabilize_pass = false;
        while (m_context.m_pass_count < max_passes) {
            m_context.m_pass_count++;
            std::cout << "Pass " << m_context.m_pass_count << "..." << std::endl;
            try {
                m_context.m_phase = ParsePhase::SymbolsGathering;
                m_context.m_current_address = default_org;
                m_context.m_blocks.start_block();
                m_context.m_current_line_number = 0;
                LineProcessor line_processor(m_context);
                for (size_t i = 0; i < source_lines.size(); ++i) {
                    m_context.m_current_line_number = i + 1;
                    line_processor.process(source_lines[i]);
                }
                bool just_stabilized = m_context.m_symbols.resolve();
                if (stabilize_pass && just_stabilized)
                    break;
                stabilize_pass = just_stabilized;
            } catch (const std::runtime_error& e) {
                throw std::runtime_error(std::string(e.what()) + " on line " + std::to_string(m_context.m_current_line_number));
            }
        }
        if (!stabilize_pass) {
            throw std::runtime_error("Failed to resolve all symbols after " + std::to_string(max_passes) + " passes.");
        }
        m_context.m_phase = ParsePhase::CodeGeneration;
        m_context.m_current_address = default_org;
        m_context.m_blocks.start_block();
        m_context.m_current_line_number = 0;

        for (size_t i = 0; i < source_lines.size(); ++i) {
            m_context.m_current_line_number = i + 1;
            LineProcessor(m_context).process(source_lines[i]);
        }
        return true;
    }

    struct CompilationContext;
    class CodeBlocks {
        public:
            CodeBlocks(CompilationContext& context) : m_context(context) {}
            struct Block {
                uint16_t m_start_address = 0;
                uint16_t m_length = 0;
                std::string m_name;
            };
            Block* start_block(std::string label = "") {
                Block* block = nullptr;
                if (m_context.m_phase == ParsePhase::CodeGeneration) {
                    auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                                    [this](const Block& b) { return b.m_start_address == m_context.m_current_address; });
                    if (it == m_blocks.end())
                        throw std::runtime_error("Internal error: code blocks mismatch.");
                    else 
                        block = &(*it);
                } else {
                    auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                                            [this, label](const Block& b) { return (!label.empty() && b.m_name == label); });
                    if (it == m_blocks.end())
                        it = std::find_if(m_blocks.begin(), m_blocks.end(),
                                            [this](const Block& b) { return b.m_start_address == m_context.m_current_address; });
                    if (it == m_blocks.end()) {
                        m_blocks.push_back({m_context.m_current_address, 0, label});
                        block = &m_blocks.back();
                    } else 
                        block = &(*it);
                    if (block)
                        block->m_start_address = m_context.m_current_address;
                }
                m_context.m_current_block = block;
                m_context.m_current_block->m_length = 0;
                return block;
            }
            void update_block(const std::string& name, uint16_t new_address) {
                auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                                       [&name](const Block& b) { return b.m_name == name; });
                if (it != m_blocks.end())
                    it->m_start_address = new_address;
            }
            const std::vector<Block>& get_blocks() const {
                return m_blocks;
            }
            void clear() {
                m_blocks.clear();
            }

        private:
            CompilationContext& m_context;
            std::vector<Block> m_blocks;
    };
    class Symbols {
    public:
        struct Symbol {
            enum class Type {LABEL, EQU};
            Type m_type;
            typename CodeBlocks::Block* m_block;
            uint16_t m_value;

            std::string m_expression = "";
            bool m_unresolved = false;
        };

        Symbols(CompilationContext& context) : m_context(context) {}

        void add(typename Symbol::Type type, const std::string& name, uint16_t value) {
            if (m_symbols.count(name)) {
                if (m_context.m_phase == ParsePhase::SymbolsGathering && m_context.m_pass_count == 1)
                    throw std::runtime_error("Duplicate symbol definition: " + name);
            }
            typename CodeBlocks::Block* block = m_context.m_current_block;
            if (type == Symbol::Type::LABEL && block)
                value -= block->m_start_address;
            m_symbols[name] = { type, block, value };
            resolve();
        }
        bool evaluated(const std::string& name) const {
            if (name == "$" || name == "_")
                return true;
            if (m_symbols.count(name))
                return !m_symbols.at(name).m_unresolved;
            return false;
        }
        uint16_t get_value(const std::string& name) const {
            if (name == "$" || name == "_")
                return m_context.m_current_address;
            if (m_symbols.count(name)) {
                Symbol symbol = m_symbols.at(name);
                uint16_t address = symbol.m_value;
                if (symbol.m_type == Symbol::Type::LABEL && symbol.m_block)
                    address += symbol.m_block->m_start_address;
                return address;
            }
            if (m_context.m_phase == ParsePhase::CodeGeneration) {
                throw std::runtime_error("Undefined symbol: " + name);
            }
            return 0;
        }
        void clear() {
            m_symbols.clear();
        }
        void add_unresolved(typename Symbol::Type type, const std::string& name, const std::string& expression) {
            if (m_symbols.count(name)) {
                if (m_context.m_phase == ParsePhase::SymbolsGathering && m_context.m_pass_count == 1)
                    throw std::runtime_error("Duplicate symbol definition: " + name);
                return;
            }
            m_symbols[name] = { type, m_context.m_current_block, 0, expression, true };
            resolve();
        }
        bool resolve() {
            bool still_unresolved = false;
            auto it = m_symbols.begin();
            while (it != m_symbols.end()) {
                uint16_t value;
                if (it->second.m_unresolved) {
                    ExpressionEvaluator evaluator(m_context);
                    if (evaluator.evaluate(it->second.m_expression, value)) {
                        m_context.m_blocks.update_block(it->first, value);
                        typename Symbols::Symbol::Type type = it->second.m_type;
                        std::string name = it->first;
                        it = m_symbols.erase(it);
                        add(type, name, value);    
                    } else
                        still_unresolved = true;
                }
                ++it;
            }
            return !still_unresolved;
        }
    private:
        CompilationContext& m_context;
        std::map<std::string, Symbol> m_symbols;
    };
    struct CompilationContext {
        CompilationContext() : m_symbols(*this), m_blocks(*this) {}
        CompilationContext(const CompilationContext& other) = delete;
        CompilationContext& operator=(const CompilationContext& other) = delete;

        TMemory* m_memory = nullptr;
        Symbols m_symbols;
        CodeBlocks m_blocks;
        uint16_t m_current_address = 0;
        size_t m_current_line_number = 0;
        ParsePhase m_phase = ParsePhase::CodeGeneration;
        int m_pass_count = 0;
        typename CodeBlocks::Block* m_current_block = nullptr;
    };

    const CodeBlocks& get_code_blocks() const {
        return m_context.m_blocks;
    }

private:
    class StringHelper {
    public:
        static void trim_whitespace(std::string& s) {
            const char* whitespace = " \t";
            s.erase(0, s.find_first_not_of(whitespace));
            s.erase(s.find_last_not_of(whitespace) + 1);
        }

        static void to_upper(std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        }

        static bool is_number(const std::string& s, uint16_t& out_value) {
            std::string str = s;
            trim_whitespace(str);
            if (str.empty())
                return false;
            const char* start = str.data();
            const char* end = str.data() + str.size();
            if (start < end && *start == '+')
                start += 1;
            bool has_h_suffix = false;
            bool has_b_suffix = false;
            bool has_0x_prefix = false;
            int base = 10;
            if ((end - start) > 0) {
                char last_char = *(end - 1);
                if (last_char == 'H' || last_char == 'h') {
                    end -= 1;
                    has_h_suffix = true;
                } else if (last_char == 'B' || last_char == 'b') {
                    end -= 1;
                    has_b_suffix = true;
                }
            }
            if ((end - start) > 2 && *start == '0' && (*(start + 1) == 'x' || *(start + 1) == 'X')) {
                start += 2;
                has_0x_prefix = true;
            }
            if (has_0x_prefix) {
                if (has_h_suffix)
                    return false;
                if (has_b_suffix)
                    return false;
                base = 16;
            } else if (has_h_suffix)
                base = 16;
            else if (has_b_suffix)
                base = 2;
            else
                base = 10;
            if (start == end)
                return false;
            auto result = std::from_chars(start, end, out_value, base);
            return result.ec == std::errc() && result.ptr == end;
        }

    };
    class ExpressionEvaluator {
    public:
        ExpressionEvaluator(CompilationContext& context) : m_context(context) {}

        bool evaluate(const std::string& s, uint16_t& out_value) const {
            auto tokens = tokenize_expression(s);
            auto rpn = shunting_yard(tokens);
            return evaluate_rpn(rpn, out_value);
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
                    while (j < expr.length() && isalnum(expr[j]))
                        j++;
                    char last_char = toupper(expr[j-1]);
                    if (j < expr.length() && (last_char != 'B' && last_char != 'H') && (expr[j] == 'h' || expr[j] == 'H' || expr[j] == 'b' || expr[j] == 'B'))
                        j++;
                    uint16_t val;
                    if (StringHelper::is_number(expr.substr(i, j - i), val)) {
                        tokens.push_back({ExpressionToken::Type::NUMBER, "", val});
                    } else
                         throw std::runtime_error("Invalid number in expression: " + expr.substr(i, j - i));
                    i = j - 1;
                } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^' || c == '<' || c == '>') {
                    std::string op_str(1, c);
                    int precedence = 0;
                    if (c == '<' && i + 1 < expr.length() && expr[i+1] == '<') {
                        op_str = "<<";
                        i++;
                    } else if (c == '>' && i + 1 < expr.length() && expr[i+1] == '>') {
                        op_str = ">>";
                        i++;
                    }

                    if (op_str == "*" || op_str == "/" || op_str == "%")
                        precedence = 5;
                    else if (op_str == "+" || op_str == "-")
                        precedence = 4;
                    else if (op_str == "<<" || op_str == ">>")
                        precedence = 3;
                    else if (op_str == "&")
                        precedence = 2;
                    else if (op_str == "^")
                        precedence = 1;
                    else if (op_str == "|")
                        precedence = 0;

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

        bool evaluate_rpn(const std::vector<ExpressionToken>& rpn, uint16_t& out_value) const {
            std::vector<uint16_t> val_stack;
            for (const auto& token : rpn) {
                if (token.type == ExpressionToken::Type::NUMBER) {
                    val_stack.push_back(token.n_val);
                } else if (token.type == ExpressionToken::Type::SYMBOL) {
                    if (!m_context.m_symbols.evaluated(token.s_val))
                        return false;
                    val_stack.push_back(m_context.m_symbols.get_value(token.s_val));
                } else if (token.type == ExpressionToken::Type::OPERATOR) {
                    if (val_stack.size() < 2)
                        throw std::runtime_error("Invalid expression syntax.");
                    uint16_t v1 = val_stack.back();
                    val_stack.pop_back();
                    uint16_t v2 = val_stack.back();
                    val_stack.pop_back();
                    if (token.s_val == "+")
                        val_stack.push_back(v1 + v2);
                    else if (token.s_val == "-")
                        val_stack.push_back(v2 - v1);
                    else if (token.s_val == "*")
                        val_stack.push_back(v1 * v2);
                    else if (token.s_val == "/") {
                        if (v1 == 0) throw std::runtime_error("Division by zero in expression.");
                        val_stack.push_back(v2 / v1); // v2 is dividend, v1 is divisor
                    } else if (token.s_val == "%") {
                        if (v1 == 0) throw std::runtime_error("Division by zero in expression.");
                        val_stack.push_back(v2 % v1);
                    } else if (token.s_val == "&") {
                        val_stack.push_back(v2 & v1);
                    } else if (token.s_val == "|") {
                        val_stack.push_back(v2 | v1);
                    } else if (token.s_val == "^") {
                        val_stack.push_back(v2 ^ v1);
                    } else if (token.s_val == "<<") {
                        val_stack.push_back(v2 << v1);
                    } else if (token.s_val == ">>") {
                        val_stack.push_back(v2 >> v1);
                    }
                }
            }
            if (val_stack.size() != 1)
                throw std::runtime_error("Invalid expression syntax.");
            out_value = val_stack.back();
            return true;
        }
        CompilationContext& m_context;
    };
    class OperandParser {
    public:
        OperandParser(CompilationContext& context) : m_context(context) {}
        enum class OperandType { REG8, REG16, IMMEDIATE, MEM_IMMEDIATE, MEM_REG16, MEM_INDEXED, CONDITION, STRING_LITERAL, CHAR_LITERAL, EXPRESSION, UNKNOWN };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            uint16_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };

        Operand parse(const std::string& operand_string) {
            Operand operand;
            operand.str_val = operand_string;
            if (is_string_literal(operand_string)) {
                if (operand_string.length() == 3) {
                    operand.num_val = static_cast<uint16_t>(operand_string[1]);
                    operand.type = OperandType::CHAR_LITERAL;
                    return operand;
                } else {
                    operand.type = OperandType::STRING_LITERAL;
                    return operand;
                }
            }
            if (is_char_literal(operand_string)) {
                operand.num_val = static_cast<uint16_t>(operand_string[1]);
                operand.type = OperandType::CHAR_LITERAL;
                return operand;
            }

            std::string upper_opperand_string = operand_string;
            StringHelper::to_upper(upper_opperand_string);
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
            ExpressionEvaluator evaluator(m_context);
            if (evaluator.evaluate(operand_string, num_val)) {
                operand.num_val = num_val;
                operand.type = OperandType::IMMEDIATE;
                return operand;
            }
            if (is_mem_ptr(operand_string)) { // (HL), (IX+d), (nn)
                std::string inner = operand_string.substr(1, operand_string.length() - 2);
                inner.erase(0, inner.find_first_not_of(" \t"));
                inner.erase(inner.find_last_not_of(" \t") + 1);
                return parse_mem_ptr(inner);
            }
            if (m_context.m_phase == ParsePhase::SymbolsGathering && !m_context.m_symbols.evaluated(operand_string)) {
                operand.type = OperandType::EXPRESSION;
                operand.str_val = operand_string;
                return operand;
            }
            if (m_context.m_symbols.evaluated(operand_string)) {
                operand.num_val = m_context.m_symbols.get_value(operand_string);
                operand.type = OperandType::IMMEDIATE;
                return operand;
            }
            throw std::runtime_error("Unknown operand or undefined symbol: " + operand_string);
        }

    private:
        inline bool is_mem_ptr(const std::string& s) const { return !s.empty() && s.front() == '(' && s.back() == ')'; }
        inline bool is_char_literal(const std::string& s) const { return s.length() == 3 && s.front() == '\'' && s.back() == '\''; }
        inline bool is_string_literal(const std::string& s) const { return s.length() > 1 && s.front() == '"' && s.back() == '"'; }
        inline bool is_reg8(const std::string& s) const { return s_reg8_names.count(s);}
        inline bool is_reg16(const std::string& s) const { return s_reg16_names.count(s); }
        inline bool is_condition(const std::string& s) const { return s_condition_names.count(s); }
        inline static const std::set<std::string> s_reg8_names = {"B",    "C", "D",   "E",   "H",   "L", "(HL)", "A", "IXH", "IXL", "IYH", "IYL"};
        inline static const std::set<std::string> s_reg16_names = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
        inline static const std::set<std::string> s_condition_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};

        Operand parse_mem_ptr(const std::string& inner) {
            Operand op;
            std::string upper_inner = inner;
            StringHelper::to_upper(upper_inner);

            // Handle (IX/IY +/- d)
            size_t plus_pos = upper_inner.find('+');
            size_t minus_pos = upper_inner.find('-');
            size_t operator_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;

            if (operator_pos != std::string::npos) {
                std::string base_reg_str = upper_inner.substr(0, operator_pos);
                base_reg_str.erase(base_reg_str.find_last_not_of(" \t") + 1);
                if (base_reg_str == "IX" || base_reg_str == "IY") {
                    std::string offset_str = inner.substr(operator_pos); uint16_t offset_val;
                    if (StringHelper::is_number(offset_str, offset_val)) {
                        op.type = OperandType::MEM_INDEXED;
                        op.base_reg = base_reg_str;
                        op.offset = static_cast<int16_t>(offset_val);
                        return op;
                    }
                }
            }
            // Handle (REG16)
            if (is_reg16(upper_inner)) {
                op.type = OperandType::MEM_REG16;
                op.str_val = upper_inner;
                return op;
            }
            // Handle (number) or (LABEL)
            uint16_t inner_num_val;
            ExpressionEvaluator evaluator(m_context);
            if (evaluator.evaluate(inner, inner_num_val)) {
                op.type = OperandType::MEM_IMMEDIATE;
                op.num_val = inner_num_val;
            } else {
                op.type = OperandType::MEM_IMMEDIATE;
                op.num_val = 0; // Ensure num_val is zeroed for unresolved expressions
                op.str_val = inner;
            }
            return op;
        }
        CompilationContext& m_context;
    };
    class InstructionEncoder {
    public:
        InstructionEncoder(CompilationContext& context) : m_context(context) {}

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
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        bool encode_pseudo_instruction(const std::string& mnemonic, const std::vector<Operand>& ops) {
            if (mnemonic == "ORG") {
                if (ops.size() == 1 && match(ops[0], OperandType::IMMEDIATE)) {
                    typename CodeBlocks::Block* block = m_context.m_blocks.start_block(ops[0].str_val);
                    if (block->m_name.empty())
                        m_context.m_current_address = ops[0].num_val;
                    return true;
                } else
                    throw std::runtime_error("Invalid operand for ORG directive");
            }
            if (mnemonic == "DB" || mnemonic == "DEFB") {
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE) || match(op, OperandType::CHAR_LITERAL)) {
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
                    if (match(op, OperandType::IMMEDIATE) || match(op, OperandType::CHAR_LITERAL)) {
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
        bool match(const Operand& op, OperandType type) const {
            if (op.type == type)
                return true;
            if (m_context.m_phase == ParsePhase::SymbolsGathering && op.type == OperandType::EXPRESSION) {
                return type == OperandType::IMMEDIATE || type == OperandType::MEM_IMMEDIATE;
            }
            return false;
        }
        bool match_imm8(const Operand& op) const {
            return (match(op, OperandType::IMMEDIATE) || match(op, OperandType::CHAR_LITERAL)) && op.num_val <= 0xFF;
        }
        bool match_imm16(const Operand& op) const {
            return match(op, OperandType::IMMEDIATE);
        }
        template <typename... Args>
        void assemble(Args... args) {
            if (m_context.m_phase == ParsePhase::CodeGeneration) {
                (m_context.m_memory->poke(m_context.m_current_address++, static_cast<uint8_t>(args)), ...);
            } else {
                m_context.m_current_address += sizeof...(args);
            }
            if (m_context.m_current_block)
                m_context.m_current_block->m_length += sizeof...(args);
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
                int32_t offset = target_addr - (m_context.m_current_address + instruction_size);
                if (m_context.m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
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
                int32_t offset = target_addr - (m_context.m_current_address + instruction_size);
                if (m_context.m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
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
            if (mnemonic == "LD" && match(op1, OperandType::REG16) && match(op2, OperandType::MEM_IMMEDIATE)) {
                if (op1.str_val == "HL") {
                    assemble(0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "BC") {
                    assemble(0xED, 0x4B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "DE") {
                    assemble(0xED, 0x5B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "SP") {
                    assemble(0xED, 0x7B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "IX") {
                    assemble(0xDD, 0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
                    return true;
                }
                if (op1.str_val == "IY") {
                    assemble(0xFD, 0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8));
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
                    int32_t offset = target_addr - (m_context.m_current_address + instruction_size);
                    if (m_context.m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
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
                    int32_t offset = target_addr - (m_context.m_current_address + instruction_size);
                    if (m_context.m_phase == ParsePhase::CodeGeneration && (offset < -128 || offset > 127))
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
        CompilationContext& m_context;
    };
    class LineProcessor {
    public:
        LineProcessor(CompilationContext& context) : m_context(context) {}
    
        bool process(const std::string& source_line) {
            std::string line = source_line;
            bool add_symbols = m_context.m_phase == ParsePhase::SymbolsGathering;
            strip_comments(line);
            if (process_equ(line, add_symbols))
                return true;
            process_label(line, add_symbols);
            if (!line.empty()) {
                return process_instruction(line);
            }
            return true;
        }

    private:
        bool strip_comments(std::string& line) {
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line.erase(comment_pos);
                return true;
            }
            return false;
        }

        bool process_equ(const std::string& line, bool add_symbol) {
            std::string temp_upper = line;
            StringHelper::to_upper(temp_upper);
            size_t equ_pos = temp_upper.find(" EQU ");
            if (equ_pos != std::string::npos) {
                std::string equ_label = line.substr(0, equ_pos);
                StringHelper::trim_whitespace(equ_label);
                std::string equ_value = line.substr(equ_pos + 5);
                StringHelper::trim_whitespace(equ_value);
                if (add_symbol)
                    m_context.m_symbols.add_unresolved(Symbols::Symbol::Type::EQU, equ_label, equ_value);
                return true;
            }
            return false;
        }

        bool process_label(std::string& line, bool add_symbol) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string potential_label = line.substr(0, colon_pos);
                if (potential_label.find_first_of(" \t") == std::string::npos) {
                    std::string label = potential_label;
                    StringHelper::trim_whitespace(label);
                    line.erase(0, colon_pos + 1);
                    if (add_symbol)
                        m_context.m_symbols.add(Symbols::Symbol::Type::LABEL, label, m_context.m_current_address);
                    return true;
                }
            }
            return false;
        }

        bool process_instruction(std::string& line) {
            StringHelper::trim_whitespace(line);
            if (!line.empty()) {
                std::string mnemonic;

                std::stringstream instruction_stream(line);
                instruction_stream >> mnemonic;
                StringHelper::to_upper(mnemonic);

                OperandParser operand_parser(m_context);
                std::vector<typename OperandParser::Operand> operands;

                std::string ops;
                if (std::getline(instruction_stream, ops)) {
                    StringHelper::trim_whitespace(ops);
                    if (!ops.empty()) {
                        std::string current_operand;
                        bool in_string = false;
                        for (char c : ops) {
                            if (c == '"') {
                                in_string = !in_string;
                            }
                            if (c == ',' && !in_string) {
                                StringHelper::trim_whitespace(current_operand);
                                if (!current_operand.empty()) {
                                    operands.push_back(operand_parser.parse(current_operand));
                                }
                                current_operand.clear();
                            } else {
                                current_operand += c;
                            }
                        }
                        StringHelper::trim_whitespace(current_operand);
                        if (!current_operand.empty()) {
                            operands.push_back(operand_parser.parse(current_operand));
                        }
                    }
                }
                InstructionEncoder encoder(m_context);
                return encoder.encode(mnemonic, operands);
            }
            return true;
        }
        CompilationContext& m_context;
    };

    CompilationContext m_context;
};

#endif //__Z80ASSEMBLE_H__
