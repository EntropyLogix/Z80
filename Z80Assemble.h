//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble.h
// Verson: 1.0.5
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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <functional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <utility>
#include <vector>

class ISourceProvider {
public:
	virtual ~ISourceProvider() = default;
	virtual bool get_source(const std::string& identifier, std::vector<uint8_t>& data) = 0;
};

template <typename TMemory> class Z80Assembler {
public:
    struct Options {
        struct LabelOptions {
            bool enabled = true;
            bool allow_colon = true;
            bool allow_no_colon = true;
        } labels;
        struct CommentOptions {
            bool enabled = true;
            bool allow_semicolon = true;
            bool allow_block = true;
        } comments;
        struct DirectiveOptions {
            bool enabled = true;
            struct ConstantOptions {
                bool enabled = true;
                bool allow_equ = true;
                bool allow_set = true;
            } constants;
            bool allow_org = true;
            bool allow_align = true;
            bool allow_data_definitions = true;
            bool allow_incbin = true;
            bool allow_includes = true;
            bool allow_conditional_compilation = true;
            bool allow_repeat = true;
            bool allow_phase = true;
        } directives;
        struct ExpressionOptions {
            bool enabled = true;
        } expressions;
        struct CompilationOptions {
            int max_passes = 10;
        } compilation;
    };
    static const Options& get_default_options() {
        static const Options default_options;
        return default_options;
    };
    struct SymbolInfo {
        std::string name;
        int32_t value;
    };
    struct BlockInfo {
        uint16_t start_address;
        uint16_t size;
    };
    Z80Assembler(TMemory* memory, ISourceProvider* source_provider, const Options& options = get_default_options()) : m_context(options)
    {
        m_context.memory = memory;
        m_context.source_provider = source_provider;
    }
    bool compile(const std::string& main_file_path, uint16_t start_addr = 0x0000) {
        Preprocessor preprocessor(m_context);
        std::string flat_source;
        if (!preprocessor.process(main_file_path, flat_source))
            throw std::runtime_error("Could not open main source file: " + main_file_path);
        std::stringstream source_stream(flat_source);
        std::vector<std::string> source_lines;
        std::string line;
        while (std::getline(source_stream, line))
            source_lines.push_back(line);
        SymbolsBuilding symbols_building(m_context, m_context.options.compilation.max_passes);
        CodeGeneration code_generation(m_context, start_addr);
        std::vector<IAssemblyPolicy*> m_phases = {&symbols_building, &code_generation};
        for (auto& phase : m_phases) {
            if (!phase)
                continue;
            phase->on_initialize();
            bool end_phase = false;
            m_context.current_pass = 1;
            do {
                phase->on_pass_begin();
                m_context.current_logical_address = start_addr;
                m_context.current_physical_address = start_addr;
                m_context.current_line_number = 0;
                LineProcessor line_processor(*phase);
                for (size_t i = 0; i < source_lines.size(); ++i) {
                    m_context.current_line_number = i + 1;
                    line_processor.process(source_lines[i]);
                }
                line_processor.finalize();
                if (phase->on_pass_end())
                    end_phase = true;
                else {
                    ++m_context.current_pass;
                    phase->on_pass_next();
                }
            } while (!end_phase);
            phase->on_finalize();
        }
        return true;
    }
    std::map<std::string, SymbolInfo> get_symbols() const { return m_context.symbols; }
    std::vector<BlockInfo> get_blocks() const { return m_context.blocks; }

private:
    struct CompilationContext;
    class StringHelper;
    class IAssemblyPolicy;
    class SymbolsBuilding;
    class Expressions;

    class Preprocessor {
    public:
        Preprocessor(CompilationContext& context) : m_context(context) {}

        bool process(const std::string& main_file_path, std::string& output_source) {
            std::set<std::string> included_files;
            return process_file(main_file_path, output_source, included_files);
        }
    private:
        void remove_block_comments(std::string& source_str, const std::string& identifier) {
            size_t start_pos = source_str.find("/*");
            while (start_pos != std::string::npos) {
                size_t end_pos = source_str.find("*/", start_pos + 2);
                if (end_pos == std::string::npos)
                    throw std::runtime_error("Unterminated block comment in " + identifier);
                source_str.replace(start_pos, end_pos - start_pos + 2, "\n");
                start_pos = source_str.find("/*");
            }
        }
        bool process_file(const std::string& identifier, std::string& output_source, std::set<std::string>& included_files) {
            if (included_files.count(identifier))
                throw std::runtime_error("Circular or duplicate include detected: " + identifier);
            included_files.insert(identifier);
            std::vector<uint8_t> source_data;
            if (!m_context.source_provider->get_source(identifier, source_data))
                return false;
            std::string source_content(source_data.begin(), source_data.end());
            if (m_context.options.comments.enabled) {
                if (m_context.options.comments.allow_block)
                    remove_block_comments(source_content, identifier);
            }
            std::stringstream source_stream(source_content);
            std::string line;
            size_t line_number = 0;
            while (std::getline(source_stream, line)) {
                line_number++;
                std::string trimmed_line = line;
                if (m_context.options.directives.allow_includes) {
                    StringHelper::trim_whitespace(trimmed_line);
                    std::string upper_line = line;
                    StringHelper::to_upper(upper_line);
                    if (upper_line.find("INCLUDE ") != std::string::npos) {
                        size_t first_quote = trimmed_line.find('"');
                        size_t last_quote = trimmed_line.find('"', first_quote + 1);
                        if (first_quote == std::string::npos || last_quote == std::string::npos)
                            throw std::runtime_error("Malformed INCLUDE directive in " + identifier + " at line " + std::to_string(line_number));
                        std::string include_filename = trimmed_line.substr(first_quote + 1, last_quote - first_quote - 1);
                        process_file(include_filename, output_source, included_files);
                    } else
                        output_source.append(line).append("\n");
                } else output_source.append(line).append("\n");
            }
            return true;
        }
        CompilationContext& m_context;
    };

    class IAssemblyPolicy;
    //Operands
    class OperandParser {
    public:
        OperandParser(IAssemblyPolicy& policy) : m_policy(policy) {}

        enum class OperandType { REG8, REG16, IMMEDIATE, MEM_IMMEDIATE, MEM_REG16, MEM_INDEXED, CONDITION, CHAR_LITERAL, STRING_LITERAL, UNKNOWN };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            int32_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };
        Operand parse(const std::string& operand_string, const std::string& mnemonic = "") {
            Operand operand;
            operand.str_val = operand_string;
            std::string upper_operand_string = operand_string;
            StringHelper::to_upper(upper_operand_string);
            if (is_char_literal(operand_string)) {
                operand.num_val = (uint8_t)(operand_string[1]);
                operand.type = OperandType::CHAR_LITERAL;
                return operand;
            } else if (is_string_literal(operand_string)) {
                operand.type = OperandType::STRING_LITERAL;
                return operand;
            }
            if (upper_operand_string == "(C)") {
                 operand.type = OperandType::MEM_REG16;
                 operand.str_val = "C";
                 return operand;
            }
            if ((mnemonic == "RET" || mnemonic == "JP" || mnemonic == "CALL" || mnemonic == "JR") && is_condition(upper_operand_string)) {
                operand.type = OperandType::CONDITION;
                return operand;
            }
            if (is_reg8(upper_operand_string)) {
                operand.type = OperandType::REG8;
                return operand;
            }
            if (is_reg16(upper_operand_string)) {
                operand.type = OperandType::REG16;
                return operand;
            }
            if (is_condition(upper_operand_string)) {
                operand.type = OperandType::CONDITION;
                return operand;
            }
            if (is_mem_ptr(operand_string)) {
                std::string inner = operand_string.substr(1, operand_string.length() - 2);
                inner.erase(0, inner.find_first_not_of(" \t"));
                inner.erase(inner.find_last_not_of(" \t") + 1);
                std::string upper_inner = inner;
                StringHelper::to_upper(upper_inner);
                if (is_reg16(upper_inner)) {
                    // Handle (REG16)
                    operand.type = OperandType::MEM_REG16;
                    operand.str_val = upper_inner;
                    return operand;
                }
                size_t plus_pos = upper_inner.find('+');
                size_t minus_pos = upper_inner.find('-');
                size_t operator_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;
                if (operator_pos != std::string::npos) {
                    std::string base_reg_str = upper_inner.substr(0, operator_pos);
                    base_reg_str.erase(base_reg_str.find_last_not_of(" \t") + 1);
                    if (base_reg_str == "IX" || base_reg_str == "IY") {
                        std::string offset_str = inner.substr(operator_pos);
                        int32_t offset_val;
                        if (StringHelper::is_number(offset_str, offset_val)) {
                            // Handle (IX/IY +/- d)
                            operand.type = OperandType::MEM_INDEXED;
                            operand.base_reg = base_reg_str;
                            operand.offset = (int16_t)(offset_val);
                            return operand;
                        }
                    }
                }
                Expressions expression(m_policy);
                int32_t inner_num_val = 0;
                if (expression.evaluate(inner, inner_num_val)) {
                    // Handle (number) or (LABEL)
                    operand.type = OperandType::MEM_IMMEDIATE;
                    operand.num_val = inner_num_val;
                    return operand;
                }
            }
            Expressions expression(m_policy);
            int32_t num_val = 0;
            if (expression.evaluate(operand_string, num_val)) {
                operand.num_val = num_val;
                operand.type = OperandType::IMMEDIATE;
                return operand;
            }
            m_policy.on_unknown_operand(operand_string);
            return operand;
        }

    private:
        inline bool is_reg8(const std::string& s) const { return reg8_names().count(s);}
        inline bool is_reg16(const std::string& s) const { return reg16_names().count(s); }
        inline bool is_mem_ptr(const std::string& s) const { return !s.empty() && s.front() == '(' && s.back() == ')'; }
        inline bool is_char_literal(const std::string& s) const { return s.length() == 3 && s.front() == '\'' && s.back() == '\''; }
        inline bool is_string_literal(const std::string& s) const { return s.length() > 1 && s.front() == '"' && s.back() == '"'; }
        inline bool is_condition(const std::string& s) const { return condition_names().count(s); }

        static const std::set<std::string>& reg8_names() {
            static const std::set<std::string> s_reg8_names = {"B", "C", "D", "E", "H", "L", "A", "I", "R", "IXH", "IXL", "IYH", "IYL"};
            return s_reg8_names;
        }
        static const std::set<std::string>& reg16_names() {
            static const std::set<std::string> s_reg16_names = {"BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
            return s_reg16_names;
        }
        static const std::set<std::string>& condition_names() {
            static const std::set<std::string> s_condition_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
            return s_condition_names;
        }

        IAssemblyPolicy& m_policy;
    };
    //Expressions
    class Expressions {
    public:
        Expressions(IAssemblyPolicy& policy) : m_policy(policy){}
        bool evaluate(const std::string& s, int32_t& out_value) const {
            if (!m_policy.get_compilation_context().options.expressions.enabled) {
                std::string trimmed_s = s;
                StringHelper::trim_whitespace(trimmed_s);
                if (StringHelper::is_number(trimmed_s, out_value))
                    return true;
                return false;
            }
            auto tokens = tokenize_expression(s);
            auto rpn = shunting_yard(tokens);
            return evaluate_rpn(rpn, out_value);
        }
    private:
        enum class OperatorType {
            UNARY_MINUS, UNARY_PLUS, LOGICAL_NOT, BITWISE_NOT,
            ADD, SUB, MUL, DIV, MOD,
            BITWISE_AND, BITWISE_OR, BITWISE_XOR,
            SHL, SHR,
            GT, LT, GTE, LTE, EQ, NE,
            LOGICAL_AND, LOGICAL_OR
        };
        struct OperatorInfo {
            OperatorType type;
            int precedence;
            bool is_unary;
            bool left_assoc;
            std::function<double(double, double)> apply;
        };
        struct FunctionInfo {
            int num_args; //if negative, it's a variadic function with at least -num_args arguments.
            std::function<double(const std::vector<double> &)> apply;
        };
        struct Token {
            enum class Type { UNKNOWN, NUMBER, SYMBOL, OPERATOR, FUNCTION, LPAREN, RPAREN, CHAR_LITERAL, STRING_LITERAL, COMMA };
            Type type = Type::FUNCTION;
            std::string s_val;
            double n_val = 0.0;
            int precedence = 0;
            bool left_assoc = true;
            const OperatorInfo* op_info = nullptr;
        };
    private:
        static const std::map<std::string, OperatorInfo>& get_operator_map() {
            static const std::map<std::string, OperatorInfo> op_map = {
                // unary
                {"_",  {OperatorType::UNARY_MINUS, 10, true, false, [](int32_t a, int32_t) { return -a; }}},
                {"~",  {OperatorType::BITWISE_NOT, 10, true, false, [](int32_t a, int32_t) { return ~a; }}},
                {"!",  {OperatorType::LOGICAL_NOT, 10, true, false, [](int32_t a, int32_t) { return !a; }}},
                {"NOT", {OperatorType::BITWISE_NOT, 10, true, false, [](int32_t a, int32_t) { return ~a; }}},
                // binary
                {"*",  {OperatorType::MUL,         9, false, true,  [](double a, double b) { if (b==0) throw std::runtime_error("Division by zero."); return a * b; }}},
                {"/",  {OperatorType::DIV,         9, false, true,  [](double a, double b) { if (b==0) throw std::runtime_error("Division by zero."); return a / b; }}},
                {"%",  {OperatorType::MOD,         9, false, true,  [](double a, double b) { if ((int32_t)b==0) throw std::runtime_error("Division by zero."); return (int32_t)a % (int32_t)b; }}},
                {"MOD",{OperatorType::MOD,         9, false, true,  [](double a, double b) { if ((int32_t)b==0) throw std::runtime_error("Division by zero."); return (int32_t)a % (int32_t)b; }}},
                {"+",  {OperatorType::ADD,         8, false, true,  [](double a, double b) { return a + b; }}},
                {"-",  {OperatorType::SUB,         8, false, true,  [](double a, double b) { return a - b; }}},
                {"<<", {OperatorType::SHL,         7, false, true,  [](double a, double b) { return (int32_t)a << (int32_t)b; }}},
                {">>", {OperatorType::SHR,         7, false, true,  [](double a, double b) { return (int32_t)a >> (int32_t)b; }}},
                {"SHL",{OperatorType::SHL,         7, false, true,  [](double a, double b) { return (int32_t)a << (int32_t)b; }}},
                {"SHR",{OperatorType::SHR,         7, false, true,  [](double a, double b) { return (int32_t)a >> (int32_t)b; }}},
                {">",  {OperatorType::GT,          6, false, true,  [](double a, double b) { return a > b; }}},
                {"GT", {OperatorType::GT,          6, false, true,  [](double a, double b) { return a > b; }}},
                {"<",  {OperatorType::LT,          6, false, true,  [](double a, double b) { return a < b; }}},
                {"LT", {OperatorType::LT,          6, false, true,  [](double a, double b) { return a < b; }}},
                {">=", {OperatorType::GTE,         6, false, true,  [](double a, double b) { return a >= b; }}},
                {"GE", {OperatorType::GTE,         6, false, true,  [](double a, double b) { return a >= b; }}},
                {"<=", {OperatorType::LTE,         6, false, true,  [](double a, double b) { return a <= b; }}},
                {"LE", {OperatorType::LTE,         6, false, true,  [](double a, double b) { return a <= b; }}},
                {"==", {OperatorType::EQ,          5, false, true,  [](double a, double b) { return a == b; }}},
                {"EQ", {OperatorType::EQ,          5, false, true,  [](double a, double b) { return a == b; }}},
                {"!=", {OperatorType::NE,          5, false, true,  [](double a, double b) { return a != b; }}},
                {"NE", {OperatorType::NE,          5, false, true,  [](double a, double b) { return a != b; }}},
                {"&",  {OperatorType::BITWISE_AND, 4, false, true,  [](double a, double b) { return (int32_t)a & (int32_t)b; }}},
                {"AND",{OperatorType::BITWISE_AND, 4, false, true,  [](double a, double b) { return (int32_t)a & (int32_t)b; }}},
                {"^",  {OperatorType::BITWISE_XOR, 3, false, true,  [](double a, double b) { return (int32_t)a ^ (int32_t)b; }}},
                {"XOR",{OperatorType::BITWISE_XOR, 3, false, true,  [](double a, double b) { return (int32_t)a ^ (int32_t)b; }}},
                {"|",  {OperatorType::BITWISE_OR,  2, false, true,  [](double a, double b) { return (int32_t)a | (int32_t)b; }}},
                {"OR", {OperatorType::BITWISE_OR,  2, false, true,  [](double a, double b) { return (int32_t)a | (int32_t)b; }}},
                {"&&", {OperatorType::LOGICAL_AND, 1, false, true,  [](double a, double b) { return a && b; }}},
                {"||", {OperatorType::LOGICAL_OR,  0, false, true,  [](double a, double b) { return a || b; }}}
            };
            return op_map;
        }
        static const std::map<std::string, FunctionInfo>& get_function_map() {
            static const std::map<std::string, FunctionInfo> func_map = {
                {"HIGH", {1, [](const std::vector<double>& args) { return ((int32_t)args[0] >> 8) & 0xFF; }}},
                {"LOW",  {1, [](const std::vector<double>& args) { return (int32_t)args[0] & 0xFF; }}},
                {"MIN",  {-2, [](const std::vector<double>& args) {
                    if (args.size() < 2) throw std::runtime_error("MIN requires at least two arguments.");
                    double result = args[0];
                    for (size_t i = 1; i < args.size(); ++i)
                        result = std::min(result, args[i]);
                    return result;
                }}},
                {"MAX",  {-2, [](const std::vector<double>& args) {
                    if (args.size() < 2) throw std::runtime_error("MAX requires at least two arguments.");
                    double result = args[0];
                    for (size_t i = 1; i < args.size(); ++i)
                        result = std::max(result, args[i]);
                    return result;
                }}},
                {"SIN",   {1, [](const std::vector<double>& args) { return sin(args[0]); }}},
                {"COS",   {1, [](const std::vector<double>& args) { return cos(args[0]); }}},
                {"TAN",   {1, [](const std::vector<double>& args) { return tan(args[0]); }}},
                {"ASIN",  {1, [](const std::vector<double>& args) { return asin(args[0]); }}},
                {"ACOS",  {1, [](const std::vector<double>& args) { return acos(args[0]); }}},
                {"ATAN",  {1, [](const std::vector<double>& args) { return atan(args[0]); }}},
                {"ATAN2", {2, [](const std::vector<double>& args) { return atan2(args[0], args[1]); }}},
                {"ABS",   {1, [](const std::vector<double>& args) { return fabs(args[0]); }}},
                {"POW",   {2, [](const std::vector<double>& args) { return pow(args[0], args[1]); }}},
                {"SQRT",  {1, [](const std::vector<double>& args) { return sqrt(args[0]); }}},
                {"LOG",   {1, [](const std::vector<double>& args) { return log(args[0]); }}},
                {"LOG10", {1, [](const std::vector<double>& args) { return log10(args[0]); }}},
                {"LOG2",  {1, [](const std::vector<double>& args) { return log2(args[0]); }}},
                {"EXP",   {1, [](const std::vector<double>& args) { return exp(args[0]); }}},
                {"RAND",  {2, [](const std::vector<double>& args) {
                    static std::mt19937 gen(0); // Seed with 0 for deterministic results
                    std::uniform_int_distribution<> distrib((int)args[0], (int)args[1]);
                    return (double)distrib(gen);
                }}},
                {"FLOOR", {1, [](const std::vector<double>& args) { return floor(args[0]); }}},
                {"CEIL",  {1, [](const std::vector<double>& args) { return ceil(args[0]); }}},
                {"ROUND", {1, [](const std::vector<double>& args) { return round(args[0]); }}}
            };
            return func_map;
        }
        static const std::map<std::string, double>& get_constant_map() {
            static const std::map<std::string, double> const_map = {
                {"MATH_PI",    3.14159265358979323846},
                {"MATH_E",     2.71828182845904523536},
                {"TRUE",  1.0},
                {"FALSE", 0.0}
            };
            return const_map;
        }
        bool parse_char_literal(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (expr[i] == '\'' && i + 2 < expr.length() && expr[i+2] == '\'') {
                tokens.push_back({Token::Type::CHAR_LITERAL, "", (double)(expr[i+1])});
                i += 2;
                return true;
            }
            return false;
        }
        bool parse_string_literal(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (expr[i] == '"') {
                size_t end_pos = expr.find('"', i + 1);
                if (end_pos != std::string::npos) {
                    tokens.push_back({Token::Type::STRING_LITERAL, expr.substr(i, end_pos - i + 1)});
                    i = end_pos;
                    return true;
                }
            }
            return false;
        }
        bool parse_symbol(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (expr[i] == '$') {
                tokens.push_back({Token::Type::SYMBOL, "$"});
                return true;
            }
            if (!isalpha(expr[i]) && expr[i] != '_' && !(expr[i] == '.' && i + 1 < expr.length() && (isalpha(expr[i+1]) || expr[i+1] == '_')))
                return false;
            size_t j = i;
            while (j < expr.length() && (isalnum(expr[j]) || expr[j] == '_' || expr[j] == '.')) {
                if (expr[j] == '.' && j == i && (j + 1 >= expr.length() || !isalnum(expr[j+1]))) break;
                j++;
            }
            std::string symbol_str = expr.substr(i, j - i);
            std::string upper_symbol = symbol_str;
            StringHelper::to_upper(upper_symbol);

            auto const_it = get_constant_map().find(upper_symbol);
            if (const_it != get_constant_map().end()) {
                tokens.push_back({Token::Type::NUMBER, "", const_it->second});
            } else if (get_function_map().count(upper_symbol)) {
                tokens.push_back({Token::Type::FUNCTION, upper_symbol, 0.0, 12, false});
            } else {
                auto op_it = get_operator_map().find(upper_symbol);
                if (op_it != get_operator_map().end())
                    tokens.push_back({Token::Type::OPERATOR, upper_symbol, 0, op_it->second.precedence, op_it->second.left_assoc, &op_it->second});
                else
                    tokens.push_back({Token::Type::SYMBOL, symbol_str});
            }
            i = j - 1;
            return true;
        }
        bool parse_number(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (isdigit(expr[i]) || (expr[i] == '.' && i + 1 < expr.length() && isdigit(expr[i + 1]))) {
                size_t j = i;
                bool has_dot = false;
                while (j < expr.length() && (isdigit(expr[j]) || (!has_dot && expr[j] == '.'))) {
                    if (expr[j] == '.') has_dot = true;
                    j++;
                }
                if (!has_dot) {
                    j = i;
                    if (expr.substr(i, 2) == "0x" || expr.substr(i, 2) == "0X") j += 2;
                    while (j < expr.length() && isalnum(expr[j])) j++;
                    if (j < expr.length() && (expr[j] == 'h' || expr[j] == 'H' || expr[j] == 'b' || expr[j] == 'B')) {
                        char last_char = toupper(expr[j - 1]);
                        if (last_char != 'B' && last_char != 'H') j++;
                    }
                    int32_t val;
                    if (StringHelper::is_number(expr.substr(i, j - i), val)) {
                        tokens.push_back({Token::Type::NUMBER, "", (double)(val)});
                        i = j - 1;
                        return true;
                    }
                    j = i;
                    while (j < expr.length() && isdigit(expr[j]))
                        j++;
                }
                std::string num_str = expr.substr(i, j - i);
                try {
                    tokens.push_back({Token::Type::NUMBER, "", std::stod(num_str)});
                } catch (const std::invalid_argument&) {
                    throw std::runtime_error("Invalid number in expression: " + num_str);
                }
                i = j - 1;
                return true;
            }
            return false;
        }
        bool parse_operator(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            std::string op_str;
            if (i + 1 < expr.length()) {
                op_str = expr.substr(i, 2);
                if (get_operator_map().find(op_str) == get_operator_map().end())
                    op_str = expr.substr(i, 1);
            } else 
                op_str = expr.substr(i, 1);
            bool is_unary = (tokens.empty() || tokens.back().type == Token::Type::OPERATOR || tokens.back().type == Token::Type::LPAREN);
            std::string op_key = op_str;
            if (is_unary && (op_str == "-" || op_str == "~" || op_str == "!"))
                op_key = (op_str == "-") ? "_" : op_str;
            else if (is_unary && op_str == "+") {
                i += op_str.length() - 1;
                return true;
            }
            auto op_it = get_operator_map().find(op_key);
            if (op_it == get_operator_map().end())
                return false;
            tokens.push_back({Token::Type::OPERATOR, op_key, 0.0, op_it->second.precedence, op_it->second.left_assoc, &op_it->second});
            i += op_str.length() - 1;
            return true;
        }
        bool parse_parens(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (expr[i] == '(') {
                tokens.push_back({Token::Type::LPAREN, "("});
                return true;
            }
            if (expr[i] == ')') {
                tokens.push_back({Token::Type::RPAREN, ")"});
                return true;
            }
            return false;
        }
        bool parse_comma(const std::string& expr, int& i, std::vector<Token>& tokens) const {
            if (expr[i] == ',') {
                tokens.push_back({Token::Type::COMMA, ","});
                return true;
            }
            return false;
        }
        std::vector<Token> tokenize_expression(const std::string& expr) const {
            std::vector<Token> tokens;
            for (int i = 0; i < expr.length(); ++i) {
                char c = expr[i];
                if (isspace(c))
                    continue;
                if (parse_string_literal(expr, i, tokens))
                    continue;
                if (parse_number(expr, i, tokens))
                    continue;
                if (parse_char_literal(expr, i, tokens))
                    continue;
                if (parse_symbol(expr, i, tokens))
                    continue;
                if (parse_operator(expr, i, tokens))
                    continue;
                if (parse_comma(expr, i, tokens))
                    continue;
                if (parse_parens(expr, i, tokens))
                    continue;
                else
                    throw std::runtime_error("Invalid character in expression: " + std::string(1, c));
            }
            return tokens;
        }
        std::vector<Token> shunting_yard(const std::vector<Token>& infix) const {
            std::vector<Token> postfix;
            std::vector<Token> op_stack;
            std::vector<int> arg_counts;
            for (const auto& token : infix) {
                switch (token.type) {
                    case Token::Type::NUMBER:
                    case Token::Type::CHAR_LITERAL:
                    case Token::Type::SYMBOL:
                        postfix.push_back(token);
                        break; 
                    case Token::Type::FUNCTION:
                        arg_counts.push_back(0);
                        op_stack.push_back(token);
                        break;
                    case Token::Type::OPERATOR:
                        while (!op_stack.empty() && op_stack.back().type == Token::Type::OPERATOR &&
                              ((op_stack.back().precedence > token.precedence) || (op_stack.back().precedence == token.precedence && token.left_assoc))) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        op_stack.push_back(token);
                        break;
                    case Token::Type::LPAREN:
                        if (!op_stack.empty() && op_stack.back().type == Token::Type::FUNCTION)
                            arg_counts.back() = 1;
                        op_stack.push_back(token);
                        break;
                    case Token::Type::RPAREN:
                        while (!op_stack.empty() && op_stack.back().type != Token::Type::LPAREN) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        if (op_stack.empty())
                            throw std::runtime_error("Mismatched parentheses in expression.");
                        op_stack.pop_back();
                        if (!op_stack.empty() && op_stack.back().type == Token::Type::FUNCTION) {
                            Token func_token = op_stack.back();
                            if (arg_counts.back() > 0)
                                func_token.n_val = arg_counts.back();
                            postfix.push_back(func_token);
                            arg_counts.pop_back();
                            op_stack.pop_back();
                        }
                        break;
                    case Token::Type::COMMA:
                        while (!op_stack.empty() && op_stack.back().type != Token::Type::LPAREN) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        if (op_stack.empty()) throw std::runtime_error("Comma outside of function arguments or mismatched parentheses.");
                        if (!arg_counts.empty())
                            arg_counts.back()++;
                        break;
                    default: break;
                }
            }
            while (!op_stack.empty()) {
                if (op_stack.back().type == Token::Type::LPAREN || op_stack.back().type == Token::Type::RPAREN)
                    throw std::runtime_error("Mismatched parentheses in expression.");
                postfix.push_back(op_stack.back());
                op_stack.pop_back();
            }
            return postfix;
        }
        bool evaluate_rpn(const std::vector<Token>& rpn, int32_t& out_value) const {
            std::vector<double> val_stack;
            for (const auto& token : rpn) {
                if (token.type == Token::Type::NUMBER || token.type == Token::Type::CHAR_LITERAL) {
                    val_stack.push_back(token.n_val);
                } else if (token.type == Token::Type::SYMBOL) {
                    int32_t sum_val;
                    if (!m_policy.on_symbol_resolve(token.s_val, sum_val))
                        return false;
                    val_stack.push_back((double)(sum_val));
                } else if (token.type == Token::Type::FUNCTION) {
                    auto it = get_function_map().find(token.s_val);
                    if (it == get_function_map().end())
                        throw std::runtime_error("Unknown function in RPN evaluation: " + token.s_val);
                    const auto& func_info = it->second;
                    int num_args_provided = token.n_val > 0 ? (int)(token.n_val) : 0;
                    if (func_info.num_args > 0) {
                        if (num_args_provided != func_info.num_args)
                            throw std::runtime_error("Function " + token.s_val + " expects " + std::to_string(func_info.num_args) + " arguments, but got " + std::to_string(num_args_provided));
                    } else { // variadic
                        int min_args = -func_info.num_args;
                        if (num_args_provided < min_args)
                            throw std::runtime_error("Function " + token.s_val + " expects at least " + std::to_string(min_args) + " arguments, but got " + std::to_string(num_args_provided));
                    }
                    if (val_stack.size() < (size_t)num_args_provided)
                        throw std::runtime_error("Not enough values on stack for function " + token.s_val);
                    std::vector<double> args;
                    if (num_args_provided > 0) {
                        args.resize(num_args_provided);
                        for (int i = num_args_provided - 1; i >= 0; --i) {
                            args[i] = val_stack.back();
                            val_stack.pop_back();
                        }
                    }
                    val_stack.push_back(func_info.apply(args));
                } else if (token.type == Token::Type::OPERATOR) {
                    auto it = get_operator_map().find(token.s_val);
                    if (it == get_operator_map().end())
                        throw std::runtime_error("Unknown operator in RPN evaluation: " + token.s_val);
                    
                    const auto& op_info = it->second;
                    if (op_info.is_unary) {
                        if (val_stack.size() < 1)
                            throw std::runtime_error("Invalid expression syntax for unary operator.");
                        double v = val_stack.back();
                        val_stack.back() = op_info.apply(v, 0.0);
                        continue;
                    }
                    // Binary operators
                    if (val_stack.size() < 2)
                        throw std::runtime_error("Invalid expression syntax for binary operator.");
                    double v2 = val_stack.back();
                    val_stack.pop_back();
                    double v1 = val_stack.back();
                    val_stack.pop_back();
                    val_stack.push_back(op_info.apply(v1, v2));
                }
            }
            if (val_stack.size() != 1)
                throw std::runtime_error("Invalid expression syntax.");
            out_value = (int32_t)(val_stack.back());
            return true;
        }
        IAssemblyPolicy& m_policy;
    };
    //Policy
    struct CompilationContext {
        CompilationContext(const Options& opts) : options(opts) {}

        CompilationContext(const CompilationContext& other) = delete;
        CompilationContext& operator=(const CompilationContext& other) = delete;

        TMemory* memory = nullptr;
        ISourceProvider* source_provider = nullptr;
        uint16_t current_logical_address = 0;
        uint16_t current_physical_address = 0;
        size_t current_line_number = 0;
        size_t current_pass = 0;
        std::map<std::string, SymbolInfo> symbols;
        std::string last_global_label;
        std::vector<BlockInfo> blocks;
        const Options& options;
    };
    class IAssemblyPolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        virtual ~IAssemblyPolicy() = default;

        virtual void on_initialize() = 0;
        virtual void on_finalize() = 0;
        virtual void on_pass_begin() = 0;
        virtual bool on_pass_end() = 0;
        virtual void on_pass_next() = 0;

        virtual CompilationContext& get_compilation_context() = 0;
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) = 0;
        virtual void on_label_definition(const std::string& label) = 0;
        virtual void on_equ_directive(const std::string& label, const std::string& value) = 0;
        virtual void on_set_directive(const std::string& label, const std::string& value) = 0;
        virtual void on_org_directive(const std::string& label) = 0;
        virtual void on_phase_directive(const std::string& address_str) = 0;
        virtual void on_dephase_directive() = 0;
        virtual void on_align_directive(const std::string& boundary) = 0;
        virtual void on_incbin_directive(const std::string& filename) = 0;
        virtual void on_unknown_operand(const std::string& operand) = 0;
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) = 0;
        virtual void on_proc_begin(const std::string& name) = 0;
        virtual void on_proc_end() = 0;
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) = 0;
        virtual void on_assemble(std::vector<uint8_t> bytes) = 0;
    };

    class DefaultAssemblyPolicy : public IAssemblyPolicy {
    public:
        using Operand = typename IAssemblyPolicy::Operand;
        using OperandType = typename IAssemblyPolicy::OperandType;

        DefaultAssemblyPolicy(CompilationContext& context) : m_context(context) {}

        virtual CompilationContext& get_compilation_context() override { return m_context; }

        void on_initialize() override {}
        void on_finalize() override {}
        void on_pass_begin() override {}
        bool on_pass_end() override { return true; }
        void on_pass_next() override {}
        bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {
            if (symbol == "$") {
                out_value = this->m_context.current_logical_address;
                return true;
            }
            return false;
        }
        void on_label_definition(const std::string& label) override {
            if (!label.empty() && label[0] != '.')
                this->m_last_global_label = label;
        }
        void on_equ_directive(const std::string& label, const std::string& value) override {}
        void on_set_directive(const std::string& label, const std::string& value) override {}
        void on_org_directive(const std::string& label) override {}
        void on_phase_directive(const std::string& address_str) override {}
        void on_dephase_directive() override {}
        void on_align_directive(const std::string& boundary) override {
            if (!this->m_context.options.directives.allow_align)
                return;
            Expressions expression(*this);
            int32_t align_val;
            if (expression.evaluate(boundary, align_val) && align_val > 0) {
                uint16_t current_addr = this->m_context.current_logical_address;
                uint16_t new_addr = (current_addr + align_val - 1) & ~(align_val - 1);
                for (uint16_t i = current_addr; i < new_addr; ++i)
                    on_assemble({0x00});
            }
        }
        void on_incbin_directive(const std::string& filename) override {
            if (!this->m_context.options.directives.allow_incbin) return;
            std::vector<uint8_t> data;
            if (this->m_context.source_provider->get_source(filename, data)) on_assemble(data);
            else throw std::runtime_error("Could not open file for INCBIN: " + filename);
        }
        void on_unknown_operand(const std::string& operand) override {}
        void on_proc_begin(const std::string& name) override {
            m_proc_stack.push_back(name);
            on_label_definition(name);
        }
        void on_proc_end() override {
            if (m_proc_stack.empty())
                throw std::runtime_error("ENDP without PROC.");
            m_proc_stack.pop_back();
        }
        bool on_operand_not_matching(const Operand& operand, OperandType expected) override { return false; } // Now Operand and OperandType are known
        void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) override {}
        void on_assemble(std::vector<uint8_t> bytes) override {}
    protected:
        std::string get_absolute_symbol_name(const std::string& name) const {
            if (!name.empty() && name[0] == '.') {
                if (this->m_last_global_label.empty())
                    throw std::runtime_error("Local label '" + name + "' used without a preceding global label.");
                return this->m_last_global_label + name;
            }
            return name;
        }    
        std::string m_last_global_label;
        std::vector<std::string> m_proc_stack;
        CompilationContext& m_context;
    };
    // Helpers
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
        static bool is_number(const std::string& s, int32_t& out_value) {
            std::string str = s;
            trim_whitespace(str);
            if (str.empty())
                return false;
            const char* start = str.data();
            const char* end = str.data() + str.size();
            bool is_negative = false;
            if (start < end && *start == '-') {
                is_negative = true;
                start++;
            } else if (start < end && *start == '+')
                start++;
            int base = 10;
            if ((end - start) > 2 && (*start == '0' && (*(start + 1) == 'x' || *(start + 1) == 'X'))) {
                start += 2;
                base = 16;
            } else if ((end - start) > 2 && (*start == '0' && (*(start + 1) == 'b' || *(start + 1) == 'B'))) {
                start += 2;
                base = 2;
            } else if ((end - start) > 0) {
                char last_char = *(end - 1);
                if (last_char == 'H' || last_char == 'h') {
                    end -= 1;
                    base = 16;
                } else if (last_char == 'B' || last_char == 'b') {
                    end -= 1;
                    base = 2;
                }
            }
            if (start == end)
                return false;
            auto result = std::from_chars(start, end, out_value, base);
            bool success = (result.ec == std::errc() && result.ptr == end);
            if (success && is_negative)
                out_value = -out_value;
            return success;
        }
    };
    class SymbolsBuilding : public DefaultAssemblyPolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        SymbolsBuilding(CompilationContext& context, int max_pass) : DefaultAssemblyPolicy(context), m_max_pass(max_pass) {}
        virtual ~SymbolsBuilding() {
            clear_symbols();
        }
        void on_finalize() override {
            if (!this->m_proc_stack.empty())
                throw std::runtime_error("Unterminated procedure block (missing ENDP).");
        }

        virtual void on_initialize() override {
            clear_symbols();
        }
        virtual void on_pass_begin() override {
            m_symbols_stable = true;
        }
        virtual bool on_pass_end() override {
            if (!all_used_symbols_defined())
                m_symbols_stable = false;
            if (m_final_pass_scheduled) {
                if (m_symbols_stable) {
                    this->m_context.symbols.clear();
                    for (const auto& symbol_pair : m_symbols) {
                        Symbol* symbol = symbol_pair.second;
                        if (symbol) {
                            int index = symbol->index;
                            if (!symbol->undefined[index])
                                this->m_context.symbols[symbol_pair.first] = {symbol_pair.first, symbol->value[index]};
                        }
                    }
                    return true;
                }
                else {
                    m_final_pass_scheduled = false;
                    return false;
                }
            }
            if (m_symbols_stable)
                m_final_pass_scheduled = true;
            return false;
        }
        virtual void on_pass_next() override {
            if (this->m_context.current_pass > m_max_pass) {
                std::string error_msg = "Failed to resolve all symbols after " + std::to_string(m_max_pass) + " passes.";
                if (all_used_symbols_defined())
                    error_msg += " Symbols are defined but their values did not stabilize. Need more passes.";
                else {
                    error_msg += " Undefined symbol(s): ";
                    bool first = true;
                    for (const auto& symbol_pair : m_symbols) {
                        Symbol* symbol = symbol_pair.second;
                        if (symbol) {
                            int index = symbol->index;
                            if (symbol->undefined[index]) {
                                error_msg += (first ? "" : ", ") + symbol_pair.first;
                                first = false;
                            }
                        }
                    }
                    error_msg += ". This may be due to circular dependencies or not enough passes.";
                }
                throw std::runtime_error(error_msg);
            }
            for (auto& symbol_pair : m_symbols) {
                Symbol* symbol = symbol_pair.second;
                if (symbol)
                    symbol->index = -1;
            }
        }
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {
            if (DefaultAssemblyPolicy::on_symbol_resolve(symbol, out_value))
                return true;
            bool resolved = false;
            std::string actual_symbol_name = this->get_absolute_symbol_name(symbol);
            auto it = m_symbols.find(actual_symbol_name);
            if (it != m_symbols.end()) {
                Symbol *symbol = it->second;
                if (symbol) {
                    symbol->in_use = true;
                    int index = symbol->index;
                    if (index == -1)
                        index = symbol->value.size() - 1;
                    out_value = symbol->value[index];
                    resolved = !symbol->undefined[index];
                }
            }
            return resolved;
        }
        virtual void on_label_definition(const std::string& label) override {
            DefaultAssemblyPolicy::on_label_definition(label);
            std::string actual_label_name = this->get_absolute_symbol_name(label);
            update_symbol(actual_label_name, this->m_context.current_logical_address, false, false);
        };
        virtual void on_equ_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, false);
        };
        virtual void on_set_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, true);
        };
        virtual void on_org_directive(const std::string& label) override {
            int32_t num_val;
            if (StringHelper::is_number(label, num_val))
                this->m_context.current_logical_address = this->m_context.current_physical_address = num_val;
            else if (m_symbols_stable) {
                Expressions expression(*this);
                if (expression.evaluate(label, num_val)) {
                    this->m_context.current_logical_address = num_val;
                    this->m_context.current_physical_address = num_val;
                }
            }
        };
        virtual void on_phase_directive(const std::string& label) override {
            int32_t num_val;
            if (StringHelper::is_number(label, num_val)) {
                this->m_context.current_logical_address = num_val;
            }
            else if (m_symbols_stable) {
                Expressions expression(*this);
                if (expression.evaluate(label, num_val))
                    this->m_context.current_logical_address = num_val;
            }
        }
        virtual void on_dephase_directive() override {
            this->m_context.current_logical_address = this->m_context.current_physical_address;
        }

        virtual bool on_operand_not_matching(const Operand& operand, typename OperandParser::OperandType expected) override {
            if (operand.type == OperandType::UNKNOWN)
                return expected == OperandType::IMMEDIATE || expected == OperandType::MEM_IMMEDIATE;
            return false;
        }
        virtual void on_assemble(std::vector<uint8_t> bytes) override {
            size_t size = bytes.size();
            this->m_context.current_logical_address += size;
            this->m_context.current_physical_address += size;
        }
    private:
        struct Symbol {
            bool redefinable;
            int index;
            std::vector<int32_t> value;
            std::vector<bool> undefined;
            bool in_use;
        };
        void clear_symbols() {
            for (auto& symbol_pair : m_symbols) {
                delete symbol_pair.second;
                symbol_pair.second = nullptr;
            }
            m_symbols.clear();
        }
        void on_const(const std::string& label, const std::string& value, bool redefinable) {
            int32_t num_val = 0;
            Expressions expression(*this);
            bool evaluated = expression.evaluate(value, num_val);
            update_symbol(label, num_val, !evaluated, redefinable);
        };
        bool all_used_symbols_defined() const {
            bool all_used_defined = true;
            for (const auto& symbol_pair : m_symbols) {
                Symbol* symbol = symbol_pair.second;
                if (symbol) {
                    if (symbol->in_use && symbol->undefined[symbol->index]) {
                        all_used_defined = false;
                        break;
                    }
                }
            }
            return all_used_defined;
        }
        void update_symbol(const std::string& name, int32_t value, bool is_undefined, bool redefinable) {
            auto it = m_symbols.find(name);
            if (it == m_symbols.end()) {
                Symbol* new_symbol = new Symbol{redefinable, 0, {value}, {is_undefined}};
                m_symbols[name] = new_symbol;
                m_symbols_stable = false;
            } else {
                Symbol *symbol = it->second;
                if (symbol) {
                    if (!symbol->redefinable && redefinable)
                        throw std::runtime_error("Cannot redefine constant symbol: " + name);
                    int& index = symbol->index;
                    index++;
                    if (index >= symbol->value.size()) {
                        if (!redefinable)
                            throw std::runtime_error("Duplicate symbol definition: " + name);
                        symbol->value.push_back(value);
                        symbol->undefined.push_back(is_undefined);
                        m_symbols_stable = false;
                        return;
                    }
                    if (symbol->value[index] != value || symbol->undefined[index] != is_undefined) {
                        symbol->value[index] = value;
                        symbol->undefined[index] = is_undefined;
                        m_symbols_stable = false;
                    }
                }
            }
        }
        std::map<std::string, Symbol*> m_symbols; 
        bool m_symbols_stable = false;
        bool m_final_pass_scheduled = false;
        int m_max_pass = 0;
    };
    class CodeGeneration : public DefaultAssemblyPolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;
        
        CodeGeneration(CompilationContext& context, uint16_t start_addr) : DefaultAssemblyPolicy(context), m_start_addr(start_addr) {}
        virtual ~CodeGeneration() = default;

        virtual void on_pass_begin() override {
            this->m_last_global_label.clear();
            m_blocks.push_back({m_start_addr, 0});
        }
        virtual bool on_pass_end() override {
            for (auto& block : m_blocks) {
                if (block.second != 0)
                    this->m_context.blocks.push_back({block.first, block.second});
            }
            return true;
        }
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {
            if (DefaultAssemblyPolicy::on_symbol_resolve(symbol, out_value))
                return true;            
            std::string actual_symbol_name = this->get_absolute_symbol_name(symbol);
            auto it = this->m_context.symbols.find(actual_symbol_name);
            if (it == this->m_context.symbols.end())
                return false; // For IFDEF/IFNDEF
            out_value = it->second.value;
                return true;
        }
        virtual void on_label_definition(const std::string& label) override {
            DefaultAssemblyPolicy::on_label_definition(label);
        };
        virtual void on_org_directive(const std::string& label) override {
            int32_t addr;
            Expressions expression(*this);
            if (expression.evaluate(label, addr)) {
                this->m_context.current_logical_address = addr;
                this->m_context.current_physical_address = addr;
                this->m_blocks.push_back({addr, 0});
            }
            else
                throw std::runtime_error("Invalid code block label: " + label);
        }
        virtual void on_phase_directive(const std::string& address_str) override {
            int32_t new_logical_addr;
            Expressions expression(*this);
            if (expression.evaluate(address_str, new_logical_addr))
                this->m_context.current_logical_address = new_logical_addr;
        }
        virtual void on_dephase_directive() override {
            this->m_context.current_logical_address = this->m_context.current_physical_address;
        }
        virtual void on_unknown_operand(const std::string& operand) override { throw std::runtime_error("Unknown operand or undefined symbol: " + operand); }
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) override {
            throw std::runtime_error(mnemonic + " jump target out of range. Offset: " + std::to_string(offset));
        }
        virtual void on_assemble(std::vector<uint8_t> bytes) override {
            for (auto& byte : bytes) {
                this->m_context.memory->poke(this->m_context.current_physical_address++, byte);
            }
            this->m_context.current_logical_address += bytes.size();
            if (this->m_blocks.empty())
                throw std::runtime_error("Invalid code block.");
            this->m_blocks.back().second += bytes.size();
        }
    private:
        uint16_t m_start_addr;
        std::vector<std::pair<uint16_t, uint16_t>> m_blocks;
    };
    class Keywords {
    public:
        static bool is_mnemonic(const std::string& s) { return is_in_set(s, mnemonics()); }
        static bool is_directive(const std::string& s) { return is_in_set(s, directives()); }
        static bool is_register(const std::string& s) { return is_in_set(s, registers()); }
        static bool is_reserved(const std::string& s) { return is_mnemonic(s) || is_directive(s) || is_register(s); }

        static bool is_valid_label_name(const std::string& s) {
            if (s.empty() || is_reserved(s))
                return false;
            if (!std::isalpha(s[0]) && s[0] != '_' && s[0] != '.')
                return false;
            for (char c : s) {
                if (!std::isalnum(c) && c != '_' && c != '.')
                    return false;
            }
            return true;
        }
    private:
        static bool is_in_set(const std::string& s, const std::set<std::string>& set) {
            std::string upper_s = s;
            StringHelper::to_upper(upper_s);
            return set.count(upper_s);
        }
        static const std::set<std::string>& mnemonics() {
            static const std::set<std::string> mnemonics = {
                "ADC", "ADD", "AND", "BIT", "CALL", "CCF", "CP", "CPD", "CPDR", "CPI", "CPIR", "CPL", "DAA", "DEC", "DI",
                "DJNZ", "EI", "EX", "EXX", "HALT", "IM", "IN", "INC", "IND", "INDR", "INI", "INIR", "JP", "JR", "LD",
                "LDD", "LDDR", "LDI", "LDIR", "NEG", "NOP", "OR", "OTDR", "OTIR", "OUT", "OUTD", "OUTI", "POP", "PUSH",
                "RES", "RET", "RETI", "RETN", "RL", "RLA", "RLC", "RLCA", "RLD", "RR", "RRA", "RRC", "RRCA", "RRD",
                "RST", "SBC", "SCF", "SET", "SLA", "SLL", "SLI", "SRA", "SRL", "SUB", "XOR"
            };
            return mnemonics;
        }
        static const std::set<std::string>& directives() {
            static const std::set<std::string> directives = {
                "DB", "DEFB", "DEFS", "DEFW", "DW", "DS", "EQU", "SET", "DEFL", "ORG", 
                "INCLUDE", "ALIGN", "INCBIN", "PHASE", "DEPHASE",
                "PROC", "ENDP"
            };
            return directives;
        }
        static const std::set<std::string>& registers() {
            static const std::set<std::string> registers = {"B", "C", "D", "E", "H", "L", "A", "I", "R", "IXH", "IXL", "IYH", "IYL", "BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
            return registers;
        }
    };
    class InstructionEncoder {
    public:
        InstructionEncoder(IAssemblyPolicy& policy) : m_policy(policy) {}

        bool encode(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& operands) {
            if (Keywords::is_directive(mnemonic)) {
                if (encode_data_block(mnemonic, operands))
                    return true;
            } else if (!Keywords::is_mnemonic(mnemonic)) {
                throw std::runtime_error("Unknown mnemonic: " + mnemonic);
            }
            switch (operands.size()) {
            case 0:
                if (encode_no_operand(mnemonic))
                    return true;
                break;
            case 1:
                if (encode_one_operand(mnemonic, operands[0]))
                    return true;
                break;
            case 2:
                if (encode_two_operands(mnemonic, operands[0], operands[1]))
                    return true;
                break;
            }
            throw std::runtime_error("Invalid instruction or operands for mnemonic: " + mnemonic);
        }
    private:
        static const std::map<std::string, uint8_t>& reg8_map() {
            static const std::map<std::string, uint8_t> map = { {"B", 0},   {"C", 1},   {"D", 2},    {"E", 3}, {"H", 4},   {"L", 5},   {"(HL)", 6}, {"A", 7}, {"IXH", 4}, {"IXL", 5}, {"IYH", 4},  {"IYL", 5} };
            return map;
        }
        static const std::map<std::string, uint8_t>& reg16_map() {
            static const std::map<std::string, uint8_t> map = { {"BC", 0}, {"DE", 1}, {"HL", 2}, {"SP", 3} };
            return map;
        }
        static const std::map<std::string, uint8_t>& reg16_af_map() {
            static const std::map<std::string, uint8_t> map = { {"BC", 0}, {"DE", 1}, {"HL", 2}, {"AF", 3} };
            return map;
        }
        static const std::map<std::string, uint8_t>& condition_map() {
            static const std::map<std::string, uint8_t> map = { {"NZ", 0}, {"Z", 1},  {"NC", 2}, {"C", 3}, {"PO", 4}, {"PE", 5}, {"P", 6},  {"M", 7}};
            return map;
        }
        static const std::map<std::string, uint8_t>& relative_jump_condition_map() {
            static const std::map<std::string, uint8_t> map = { {"NZ", 0x20}, {"Z", 0x28}, {"NC", 0x30}, {"C", 0x38}};
            return map;
        }
        
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        bool match(const Operand& operand, OperandType expected) const {
            bool match = operand.type == expected;
            if (!match)
                match = m_policy.on_operand_not_matching(operand, expected);
            return match;
        }
        bool match_reg8(const Operand& operand) const { return match(operand, OperandType::REG8);}
        bool match_reg16(const Operand& operand) const { return match(operand, OperandType::REG16); }
        bool match_imm8(const Operand& operand) const { 
            return (match(operand, OperandType::IMMEDIATE) || match(operand, OperandType::CHAR_LITERAL)) && operand.num_val >= -128 && operand.num_val <= 255; 
        }
        bool match_imm16(const Operand& operand) const { 
            return match(operand, OperandType::IMMEDIATE) && operand.num_val >= -32768 && operand.num_val <= 65535; 
        }
        bool match_mem_imm16(const Operand& operand) const { return match(operand, OperandType::MEM_IMMEDIATE); }
        bool match_mem_reg16(const Operand& operand) const { return match(operand, OperandType::MEM_REG16); }
        bool match_mem_indexed(const Operand& operand) const { return match(operand, OperandType::MEM_INDEXED); }
        bool match_condition(const Operand& operand) const { return match(operand, OperandType::CONDITION); }
        bool match_char(const Operand& operand) const { return match(operand, OperandType::CHAR_LITERAL ); }
        bool match_string(const Operand& operand) const { return match(operand, OperandType::STRING_LITERAL ); }

        void assemble(std::vector<uint8_t> bytes) { m_policy.on_assemble(bytes);}

        bool encode_data_block(const std::string& mnemonic, const std::vector<Operand>& ops) {
            const auto& directive_options = m_policy.get_compilation_context().options.directives;
            if (!directive_options.enabled || !directive_options.allow_data_definitions)
                return false;
            if (mnemonic == "DB" || mnemonic == "DEFB") {
                for (const auto& op : ops) {
                    if (match_imm16(op) || match_char(op)) {
                        if (op.num_val > 0xFF)
                            throw std::runtime_error("Value in DB statement exceeds 1 byte: " + op.str_val);
                        assemble({(uint8_t)(op.num_val)});
                    } else if (match_string(op)) {
                        std::string str_content = op.str_val.substr(1, op.str_val.length() - 2);
                        for (char c : str_content)
                            assemble({(uint8_t)(c)});
                    } else
                        throw std::runtime_error("Unsupported operand for DB: " + op.str_val);
                }
                return true;
            }
            if (mnemonic == "DW" || mnemonic == "DEFW") {
                for (const auto& op : ops) {
                    if (match_imm16(op) || match_char(op)) {
                        assemble({(uint8_t)(op.num_val & 0xFF), (uint8_t)(op.num_val >> 8)});
                    } else 
                        throw std::runtime_error("Unsupported operand for DW: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                return true;
            }
            if (mnemonic == "DS" || mnemonic == "DEFS") {
                if (ops.empty() || ops.size() > 2) {
                    throw std::runtime_error("DS/DEFS requires 1 or 2 operands.");
                }
                if (!match_imm16(ops[0])) {
                    throw std::runtime_error("DS/DEFS size must be a number.");
                }
                size_t count = ops[0].num_val;
                uint8_t fill_value = (ops.size() == 2) ? (uint8_t)(ops[1].num_val) : 0;
                for (size_t i = 0; i < count; ++i) {
                    assemble({fill_value});
                }
                return true;
            }
            return false;
        }
        bool encode_no_operand(const std::string& mnemonic) {
            if (mnemonic == "NOP") {
                assemble({0x00});
                return true;
            }
            if (mnemonic == "HALT") {
                assemble({0x76});
                return true;
            }
            if (mnemonic == "DI") {
                assemble({0xF3});
                return true;
            }
            if (mnemonic == "EI") {
                assemble({0xFB});
                return true;
            }
            if (mnemonic == "EXX") {
                assemble({0xD9});
                return true;
            }
            if (mnemonic == "RET") {
                assemble({0xC9});
                return true;
            }
            if (mnemonic == "RETI") {
                assemble({0xED, 0x4D});
                return true;
            }
            if (mnemonic == "RETN") {
                assemble({0xED, 0x45});
                return true;
            }
            if (mnemonic == "RLCA") {
                assemble({0x07});
                return true;
            }
            if (mnemonic == "RRCA") {
                assemble({0x0F});
                return true;
            }
            if (mnemonic == "RLA") {
                assemble({0x17});
                return true;
            }
            if (mnemonic == "RRA") {
                assemble({0x1F});
                return true;
            }
            if (mnemonic == "DAA") {
                assemble({0x27});
                return true;
            }
            if (mnemonic == "CPL") {
                assemble({0x2F});
                return true;
            }
            if (mnemonic == "SCF") {
                assemble({0x37});
                return true;
            }
            if (mnemonic == "CCF") {
                assemble({0x3F});
                return true;
            }
            if (mnemonic == "LDI") {
                assemble({0xED, 0xA0});
                return true;
            }
            if (mnemonic == "CPI") {
                assemble({0xED, 0xA1});
                return true;
            }
            if (mnemonic == "INI") {
                assemble({0xED, 0xA2});
                return true;
            }
            if (mnemonic == "OUTI") {
                assemble({0xED, 0xA3});
                return true;
            }
            if (mnemonic == "LDD") {
                assemble({0xED, 0xA8});
                return true;
            }
            if (mnemonic == "CPD") {
                assemble({0xED, 0xA9});
                return true;
            }
            if (mnemonic == "IND") {
                assemble({0xED, 0xAA});
                return true;
            }
            if (mnemonic == "OUTD") {
                assemble({0xED, 0xAB});
                return true;
            }
            if (mnemonic == "LDIR") {
                assemble({0xED, 0xB0});
                return true;
            }
            if (mnemonic == "NEG") {
                assemble({0xED, 0x44});
                return true;
            }
            if (mnemonic == "CPIR") {
                assemble({0xED, 0xB1});
                return true;
            }
            if (mnemonic == "INIR") {
                assemble({0xED, 0xB2});
                return true;
            }
            if (mnemonic == "OTIR") {
                assemble({0xED, 0xB3});
                return true;
            }
            if (mnemonic == "LDDR") {
                assemble({0xED, 0xB8});
                return true;
            }
            if (mnemonic == "CPDR") {
                assemble({0xED, 0xB9});
                return true;
            }
            if (mnemonic == "INDR") {
                assemble({0xED, 0xBA});
                return true;
            }
            if (mnemonic == "OTDR") {
                assemble({0xED, 0xBB});
                return true;
            }
            return false;
        }
        bool encode_one_operand(const std::string& mnemonic, const Operand& op) {
            if (mnemonic == "PUSH" && match_reg16(op)) {
                if (reg16_af_map().count(op.str_val)) {
                    assemble({(uint8_t)(0xC5 | (reg16_af_map().at(op.str_val) << 4))});
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble({0xDD, 0xE5});
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble({0xFD, 0xE5});
                    return true;
                }
            }
            if (mnemonic == "POP" && match_reg16(op)) {
                if (reg16_af_map().count(op.str_val)) {
                    assemble({(uint8_t)(0xC1 | (reg16_af_map().at(op.str_val) << 4))});
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble({0xDD, 0xE1});
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble({0xFD, 0xE1});
                    return true;
                }
            }
            if (mnemonic == "INC" && match_reg16(op)) {
                if (reg16_map().count(op.str_val)) {
                    assemble({(uint8_t)(0x03 | (reg16_map().at(op.str_val) << 4))});
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble({0xDD, 0x23});
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble({0xFD, 0x23});
                    return true;
                }
            }
            if (mnemonic == "DEC" && match_reg16(op)) {
                if (reg16_map().count(op.str_val)) {
                    assemble({(uint8_t)(0x0B | (reg16_map().at(op.str_val) << 4))});
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble({0xDD, 0x2B});
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble({0xFD, 0x2B});
                    return true;
                }
            }
            if (mnemonic == "INC" && match_mem_reg16(op) && op.str_val == "HL") {
                assemble({0x34});
                return true;
            }
            if (mnemonic == "SUB" && match_imm8(op)) {
                assemble({0xD6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "DEC" && match_mem_reg16(op) && op.str_val == "HL") {
                assemble({0x35});
                return true;
            }
            if ((mnemonic == "INC" || mnemonic == "DEC") && match_mem_indexed(op)) {
                uint8_t prefix = 0;
                if (op.base_reg == "IX")
                    prefix = 0xDD;
                else if (op.base_reg == "IY")
                    prefix = 0xFD;
                else
                    return false;
                uint8_t opcode = (mnemonic == "INC") ? 0x34 : 0x35;
                assemble({prefix, opcode, (uint8_t)((int8_t)op.offset)});
                return true;
            }
            if (mnemonic == "INC" && match_reg8(op)) {
                if (op.str_val.find("IX") != std::string::npos || op.str_val.find("IY") != std::string::npos) {
                    uint8_t prefix = (op.str_val.find("IX") != std::string::npos) ? 0xDD : 0xFD;
                    uint8_t opcode = (op.str_val.back() == 'H') ? 0x24 : 0x2C; // INC H or INC L
                    assemble({prefix, opcode});
                    return true;
                }
                assemble({(uint8_t)(0x04 | (reg8_map().at(op.str_val) << 3))});
                return true;
            }
            if (mnemonic == "DEC" && match_reg8(op)) {
                if (op.str_val.find("IX") != std::string::npos || op.str_val.find("IY") != std::string::npos) {
                    uint8_t prefix = (op.str_val.find("IX") != std::string::npos) ? 0xDD : 0xFD;
                    uint8_t opcode = (op.str_val.back() == 'H') ? 0x25 : 0x2D; // DEC H or DEC L
                    assemble({prefix, opcode});
                    return true;
                }
                assemble({(uint8_t)(0x05 | (reg8_map().at(op.str_val) << 3))});
                return true;
            }
            if (mnemonic == "JP" && match_imm16(op)) {
                assemble({0xC3, (uint8_t)(op.num_val & 0xFF), (uint8_t)(op.num_val >> 8)});
                return true;
            }
            if (mnemonic == "JP" && match(op, OperandType::MEM_REG16)) {
                if (op.str_val == "HL") {
                    assemble({0xE9});
                    return true;
                }
                if (op.str_val == "IX") {
                    assemble({0xDD, 0xE9});
                    return true;
                }
                if (op.str_val == "IY") {
                    assemble({0xFD, 0xE9});
                    return true;
                }
            }
            if (mnemonic == "JR" && match_imm16(op)) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_policy.get_compilation_context().current_logical_address + instruction_size);
                if (offset < -128 || offset > 127)
                    m_policy.on_jump_out_of_range(mnemonic, offset);
                assemble({0x18, (uint8_t)(offset)});
                return true;
            }
            if (mnemonic == "ADD" && match_imm8(op)) {
                assemble({0xC6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "ADC" && match_imm8(op)) {
                assemble({0xCE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "SBC" && match_imm8(op)) {
                assemble({0xDE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "AND" && match_imm8(op)) {
                assemble({0xE6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "XOR" && match_imm8(op)) {
                assemble({0xEE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "OR" && match_imm8(op)) {
                assemble({0xF6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "CP" && match_imm8(op)) {
                assemble({0xFE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "DJNZ" && match_imm16(op)) {
                int32_t target_addr = op.num_val;
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_policy.get_compilation_context().current_logical_address + instruction_size);
                if (offset < -128 || offset > 127)
                    m_policy.on_jump_out_of_range(mnemonic, offset);
                assemble({0x10, (uint8_t)(offset)});
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
                    assemble({0xDD, base_opcode, (uint8_t)((int8_t)op.offset)});
                else if (op.base_reg == "IY")
                    assemble({0xFD, base_opcode, (uint8_t)((int8_t)op.offset)});
                return true;
            }
            if (mnemonic == "CALL" && match_imm16(op)) {
                assemble({0xCD, (uint8_t)(op.num_val & 0xFF), (uint8_t)(op.num_val >> 8)});
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") && (match_reg8(op) || (match_mem_reg16(op) && op.str_val == "HL"))) {
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
                uint8_t reg_code;
                if (op.str_val == "HL")
                    reg_code = reg8_map().at("(HL)");
                else
                    reg_code = reg8_map().at(op.str_val);
                uint8_t prefix = 0;
                if (op.str_val.find("IX") != std::string::npos)
                    prefix = 0xDD;
                else if (op.str_val.find("IY") != std::string::npos)
                    prefix = 0xFD;
                if (prefix)
                    assemble({prefix, (uint8_t)(base_opcode | reg_code)});
                else
                    assemble({(uint8_t)(base_opcode | reg_code)});
                return true;
            }
            if (mnemonic == "RET" && match_condition(op)) {
                if (condition_map().count(op.str_val)) {
                    uint8_t cond_code = condition_map().at(op.str_val);
                    assemble({(uint8_t)(0xC0 | (cond_code << 3))});
                    return true;
                }
            }
            if (mnemonic == "IM" && match_imm8(op)) {
                switch (op.num_val) {
                case 0:
                    assemble({0xED, 0x46});
                    return true;
                case 1:
                    assemble({0xED, 0x56});
                    return true;
                case 2:
                    assemble({0xED, 0x5E});
                    return true;
                }
            }
            if (mnemonic == "RST" && match_imm8(op)) {
                switch (op.num_val) {
                    case 0x00:
                        assemble({0xC7});
                        return true;
                    case 0x08:
                        assemble({0xCF});
                        return true;
                    case 0x10:
                        assemble({0xD7});
                        return true;
                    case 0x18:
                        assemble({0xDF});
                        return true;
                    case 0x20:
                        assemble({0xE7});
                        return true;
                    case 0x28:
                        assemble({0xEF});
                        return true;
                    case 0x30:
                        assemble({0xF7});
                        return true;
                    case 0x38:
                        assemble({0xFF});
                        return true;
                }
            }
            const std::map<std::string, uint8_t> rotate_shift_map = {
                {"RLC", 0x00}, {"RRC", 0x08}, {"RL", 0x10}, {"RR", 0x18},
                {"SLA", 0x20}, {"SRA", 0x28}, {"SLL", 0x30}, {"SLI", 0x30}, {"SRL", 0x38}
            };
            if (rotate_shift_map.count(mnemonic)) {
                if (match_reg8(op) || (match_mem_reg16(op) && op.str_val == "HL")) {
                    uint8_t base_opcode = rotate_shift_map.at(mnemonic);
                    uint8_t reg_code;
                    if (op.type == OperandType::MEM_REG16)
                        reg_code = reg8_map().at("(HL)");
                    else
                        reg_code = reg8_map().at(op.str_val);
                    assemble({0xCB, (uint8_t)(base_opcode | reg_code)});
                    return true;
                }
            }
            if (mnemonic == "IN" && op.type == OperandType::MEM_REG16 && op.str_val == "C") {
                assemble({0xED, 0x70});
                return true;
            }
            return false;
        }
        bool encode_two_operands(const std::string& mnemonic, const Operand& op1, const Operand& op2) {
            if (mnemonic == "EX" && op1.str_val == "AF" && op2.str_val == "AF'") {
                assemble({0x08});
                return true;
            }
            if (mnemonic == "EX" && op1.str_val == "DE" && op2.str_val == "HL") {
                assemble({0xEB});
                return true;
            }
            if (mnemonic == "EX" && match_mem_reg16(op1) && op1.str_val == "SP" && match_reg16(op2)) {
                if (op2.str_val == "HL") {
                    assemble({0xE3});
                    return true;
                }
                if (op2.str_val == "IX") {
                    assemble({0xDD, 0xE3});
                    return true;
                }
                if (op2.str_val == "IY") {
                    assemble({0xFD, 0xE3});
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.str_val == "I" && op2.str_val == "A") {
                assemble({0xED, 0x47});
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "R" && op2.str_val == "A") {
                assemble({0xED, 0x4F});
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && op2.str_val == "I") {
                assemble({0xED, 0x57});
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && op2.str_val == "R") {
                assemble({0xED, 0x5F});
                return true;
            }
            if (mnemonic == "ADD" && match_reg16(op1) && match_reg16(op2)) {
                uint8_t prefix = 0;
                std::string target_reg_str = op1.str_val;
                std::string source_reg_str = op2.str_val;
                if (target_reg_str == "IX")
                    prefix = 0xDD;
                else if (target_reg_str == "IY")
                    prefix = 0xFD;
                else if (target_reg_str != "HL")
                    return false;
                if (source_reg_str != "BC" && source_reg_str != "DE" && source_reg_str != "HL" && source_reg_str != "SP" &&
                    (prefix == 0 || (source_reg_str != (prefix == 0xDD ? "IX" : "IY")))) {
                    return false;
                }
                std::string effective_source_reg_for_opcode = source_reg_str;
                if (source_reg_str == "IX" || source_reg_str == "IY")
                    effective_source_reg_for_opcode = "HL";
                if (reg16_map().count(effective_source_reg_for_opcode)) {
                    uint8_t opcode_suffix = (uint8_t)(0x09 | (reg16_map().at(effective_source_reg_for_opcode) << 4));
                    if (prefix)
                        assemble({prefix, opcode_suffix});
                    else
                        assemble({opcode_suffix});
                    return true;
                }
            }
            if ((mnemonic == "ADC" || mnemonic == "SBC") && op1.str_val == "HL" && match_reg16(op2)) {
                uint8_t base_opcode = (mnemonic == "ADC") ? 0x4A : 0x42;
                if (reg16_map().count(op2.str_val)) {
                    assemble({0xED, (uint8_t)(base_opcode | (reg16_map().at(op2.str_val) << 4))});
                    return true;
                }
            }
            uint8_t prefix = 0;
            if (op1.base_reg == "IX" || op2.base_reg == "IX" || op1.str_val.find("IX") != std::string::npos || op2.str_val.find("IX") != std::string::npos)
                prefix = 0xDD;
            else if (op1.base_reg == "IY" || op2.base_reg == "IY" || op1.str_val.find("IY") != std::string::npos || op2.str_val.find("IY") != std::string::npos)
                prefix = 0xFD;
            if (mnemonic == "LD" && match_reg8(op1) && match_reg8(op2)) {
                uint8_t dest_code = reg8_map().at(op1.str_val);
                uint8_t src_code = reg8_map().at(op2.str_val);
                if (prefix) {
                    if ((op1.str_val.find("IX") != std::string::npos && op2.str_val.find("IY") != std::string::npos) ||
                        (op1.str_val.find("IY") != std::string::npos && op2.str_val.find("IX") != std::string::npos))
                        throw std::runtime_error("Cannot mix IX and IY register parts");
                    assemble({prefix, (uint8_t)(0x40 | (dest_code << 3) | src_code)});
                    return true;
                }
                assemble({(uint8_t)(0x40 | (dest_code << 3) | src_code)});
                return true;
            }
            if (mnemonic == "LD" &&
                (op1.str_val == "IXH" || op1.str_val == "IXL" || op1.str_val == "IYH" || op1.str_val == "IYL") && match_imm8(op2)) 
            {
                uint8_t opcode = 0;
                if (op1.str_val == "IXH" || op1.str_val == "IYH")
                    opcode = 0x26; // LD H, n
                else // IXL or IYL
                    opcode = 0x2E; // LD L, n
                assemble({prefix, opcode, (uint8_t)op2.num_val});
                return true;    
            }
            if (mnemonic == "LD" && match_reg8(op1) && match_imm8(op2)) {
                uint8_t dest_code = reg8_map().at(op1.str_val);
                assemble({(uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "LD" && match_reg16(op1) && match_imm16(op2)) {
                if (reg16_map().count(op1.str_val)) {
                    assemble({(uint8_t)(0x01 | (reg16_map().at(op1.str_val) << 4)), (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "IX") {
                    assemble({0xDD, 0x21, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "IY") {
                    assemble({0xFD, 0x21, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
            }
            if (mnemonic == "LD" && match_reg16(op1) && match_mem_imm16(op2)) {
                if (op1.str_val == "HL") {
                    assemble({0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "BC") {
                    assemble({0xED, 0x4B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "DE") {
                    assemble({0xED, 0x5B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "SP") {
                    assemble({0xED, 0x7B, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "IX") {
                    assemble({0xDD, 0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
                if (op1.str_val == "IY") {
                    assemble({0xFD, 0x2A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                    return true;
                }
            }
            if (mnemonic == "LD" && match_mem_reg16(op1) && op2.str_val == "A") {
                if (op1.str_val == "BC") {
                    assemble({0x02});
                    return true;
                }
                if (op1.str_val == "DE") {
                    assemble({0x12});
                    return true;
                }
                if (op1.str_val == "SP")
                    return false;
            }
            if (mnemonic == "LD" && match_reg8(op1) && match_mem_reg16(op2) && op2.str_val == "HL") {
                uint8_t dest_code = reg8_map().at(op1.str_val);
                assemble({(uint8_t)(0x40 | (dest_code << 3) | 6)}); // 6 is code for (HL)
                return true;
            }
            if (mnemonic == "LD" && match_mem_reg16(op1) && op1.str_val == "HL" && match_reg8(op2)) {
                uint8_t src_code = reg8_map().at(op2.str_val);
                assemble({(uint8_t)(0x70 | src_code)});
                return true;
            }
            if (mnemonic == "LD" && match_mem_reg16(op1) && op1.str_val == "HL" && match_imm8(op2)) {
                assemble({0x36, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match_mem_reg16(op2)) {
                if (op2.str_val == "BC") {
                    assemble({0x0A});
                    return true;
                }
                if (op2.str_val == "DE") {
                    assemble({0x1A});
                    return true;
                }
                if (op2.str_val == "SP")
                    return false;
            }
            if (mnemonic == "LD" && match_mem_imm16(op1) && op2.str_val == "A") {
                assemble({0x32, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                return true;
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match_mem_imm16(op2)) {
                assemble({0x3A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (mnemonic == "LD" && match_mem_imm16(op1) && match_reg16(op2)) {
                if (op2.str_val == "IX") {
                    assemble({0xDD, 0x22, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
                if (op2.str_val == "IY") {
                    assemble({0xFD, 0x22, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
                if (op2.str_val == "HL") {
                    assemble({0x22, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
                if (op2.str_val == "BC") {
                    assemble({0xED, 0x43, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
                if (op2.str_val == "DE") {
                    assemble({0xED, 0x53, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
                if (op2.str_val == "SP") {
                    assemble({0xED, 0x73, (uint8_t)(op1.num_val & 0xFF), (uint8_t)(op1.num_val >> 8)});
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.str_val == "SP" && match_reg16(op2)) {
                if (op2.str_val == "HL") {
                    assemble({0xF9});
                    return true;
                }
                if (op2.str_val == "IX") {
                    assemble({0xDD, 0xF9});
                    return true;
                }
                if (op2.str_val == "IY") {
                    assemble({0xFD, 0xF9});
                    return true;
                }
            }
            if (mnemonic == "LD" && op1.str_val == "A" && match_mem_imm16(op2)) {
                assemble({0x3A, (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (mnemonic == "IN" && op1.str_val == "A" && match_mem_imm16(op2)) {
                if (op2.num_val > 0xFF)
                    throw std::runtime_error("Port for IN instruction must be 8-bit");
                assemble({0xDB, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "OUT" && match_mem_imm16(op1) && op2.str_val == "A" && op1.num_val <= 0xFF) {
                if (op1.num_val > 0xFF)
                    throw std::runtime_error("Port for OUT instruction must be 8-bit");
                assemble({0xD3, (uint8_t)op1.num_val});
                return true;
            }
            if (mnemonic == "LD" && match_mem_reg16(op1) && match_imm8(op2)) {
                if (op1.str_val == "HL") {
                    uint8_t reg_code = reg8_map().at("(HL)");
                    assemble({(uint8_t)(0x06 | (reg_code << 3)), (uint8_t)op2.num_val});
                    return true;
                }
            }
            if (mnemonic == "LD" && match_mem_indexed(op1) && match_imm8(op2)) {
                assemble({prefix, 0x36, (uint8_t)((int8_t)op1.offset), (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "LD" && match_reg8(op1) && match_mem_indexed(op2)) {
                uint8_t reg_code = reg8_map().at(op1.str_val);
                assemble({prefix, (uint8_t)(0x46 | (reg_code << 3)), (uint8_t)((int8_t)op2.offset)});
                return true;
            }
            if (mnemonic == "LD" && match_mem_indexed(op1) && match_reg8(op2)) {
                uint8_t reg_code = reg8_map().at(op2.str_val);
                assemble({prefix, (uint8_t)(0x70 | reg_code), (uint8_t)((int8_t)op1.offset)});
                return true;
            }
            if (mnemonic == "ADD" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xC6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "ADC" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xCE, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "SBC" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xDE, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "SUB" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xD6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "AND" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xE6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "XOR" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xEE, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "OR" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xF6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "CP" && op1.str_val == "A" && match_imm8(op2)) {
                assemble({0xFE, (uint8_t)op2.num_val});
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") &&
                 op1.str_val == "A" && ((match_reg8(op2) || (match_mem_reg16(op2) && op2.str_val == "HL")))) {
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
                uint8_t reg_code;
                if (op2.str_val == "HL")
                    reg_code = reg8_map().at("(HL)");
                else 
                    reg_code = reg8_map().at(op2.str_val);
                if (prefix)
                    assemble({prefix, (uint8_t)(base_opcode | reg_code)});
                else
                    assemble({(uint8_t)(base_opcode | reg_code)});
                return true;
            }
            if ((mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBC" ||
                 mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "CP") &&
                op1.str_val == "A" && match_mem_indexed(op2)) {
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
                if (op2.base_reg == "IX") {
                    assemble({0xDD, base_opcode, (uint8_t)((int8_t)op2.offset)});
                } else if (op2.base_reg == "IY")
                    assemble({0xFD, base_opcode, (uint8_t)((int8_t)op2.offset)});
                return true;
            }
            if (mnemonic == "JP" && match_condition(op1) && match_imm16(op2)) {
                uint8_t cond_code = condition_map().at(op1.str_val);
                assemble({(uint8_t)(0xC2 | (cond_code << 3)), (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (mnemonic == "JR" && match_condition(op1) && match_imm16(op2)) {
                if (relative_jump_condition_map().count(op1.str_val)) {
                    int32_t target_addr = op2.num_val;
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_policy.get_compilation_context().current_logical_address + instruction_size);
                    if (offset < -128 || offset > 127)
                        m_policy.on_jump_out_of_range(mnemonic + " " + op1.str_val, offset);
                    assemble({relative_jump_condition_map().at(op1.str_val), (uint8_t)(offset)});
                    return true;
                }
            }
            if (mnemonic == "CALL" && match_condition(op1) && match_imm16(op2)) {
                uint8_t cond_code = condition_map().at(op1.str_val);
                assemble({(uint8_t)(0xC4 | (cond_code << 3)), (uint8_t)(op2.num_val & 0xFF), (uint8_t)(op2.num_val >> 8)});
                return true;
            }
            if (mnemonic == "IN" && match_reg8(op1) && match_mem_reg16(op2) && op2.str_val == "C") {
                if (op1.str_val == "F") {
                    assemble({0xED, 0x70});
                    return true;
                }
                uint8_t reg_code = reg8_map().at(op1.str_val);
                assemble({0xED, (uint8_t)(0x40 | (reg_code << 3))});
                return true;
            }
            if (mnemonic == "OUT" && match_mem_reg16(op1) && op1.str_val == "C" && (match_reg8(op2) || (op2.type == OperandType::IMMEDIATE && op2.num_val == 0))) {
                if (op2.type == OperandType::IMMEDIATE && op2.num_val == 0) {
                    assemble({0xED, 0x71});
                    return true;
                }
                uint8_t reg_code = reg8_map().at(op2.str_val);
                if (op2.str_val == "(HL)")
                    throw std::runtime_error("OUT (C), (HL) is not a valid instruction");
                assemble({0xED, (uint8_t)(0x41 | (reg_code << 3))});
                return true;
            }
            if (mnemonic == "BIT" && match_imm8(op1) && (match_reg8(op2) || (match_mem_reg16(op2) && op2.str_val == "HL"))) {
                if (op1.num_val > 7)
                    throw std::runtime_error("BIT index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code;
                if (match_mem_reg16(op2))
                    reg_code = reg8_map().at("(HL)");
                else
                    reg_code = reg8_map().at(op2.str_val);
                assemble({0xCB, (uint8_t)(0x40 | (bit << 3) | reg_code)});
                return true;
            }
            if (mnemonic == "SET" && match_imm8(op1) && (match_reg8(op2) || (match_mem_reg16(op2) && op2.str_val == "HL"))) {
                if (op1.num_val > 7)
                    throw std::runtime_error("SET index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code;
                if (match_mem_reg16(op2))
                    reg_code = reg8_map().at("(HL)");
                else
                    reg_code = reg8_map().at(op2.str_val);
                assemble({0xCB, (uint8_t)(0xC0 | (bit << 3) | reg_code)});
                return true;
            }
            if (mnemonic == "RES" && match_imm8(op1) && (match_reg8(op2) || (match_mem_reg16(op2) && op2.str_val == "HL"))) {
                if (op1.num_val > 7)
                    throw std::runtime_error("RES index must be 0-7");
                uint8_t bit = op1.num_val;
                uint8_t reg_code;
                if (match_mem_reg16(op2))
                    reg_code = reg8_map().at("(HL)");
                else
                    reg_code = reg8_map().at(op2.str_val);
                assemble({0xCB, (uint8_t)(0x80 | (bit << 3) | reg_code)});
                return true;
            }
            if ((mnemonic == "SLL" || mnemonic == "SLI") && match_reg8(op1)) {
                if (op1.num_val > 7) {
                    throw std::runtime_error("SLL bit index must be 0-7");
                }
                uint8_t reg_code = reg8_map().at(op1.str_val);
                assemble({0xCB, (uint8_t)(0x30 | reg_code)});
                return true;
            }
            if ((mnemonic == "BIT" || mnemonic == "SET" || mnemonic == "RES") && match_imm8(op1) && match_mem_indexed(op2)) {
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
                    assemble({0xDD, 0xCB, (uint8_t)((int8_t)op2.offset), final_opcode});
                } else if (op2.base_reg == "IY") {
                    assemble({0xFD, 0xCB, (uint8_t)((int8_t)op2.offset), final_opcode});
                } else
                    return false;
                return true;
            }
            return false;
        }

        IAssemblyPolicy& m_policy;
    };
    class LineProcessor {
    public:
        LineProcessor(IAssemblyPolicy& policy) : m_policy(policy) {}

        void finalize() const {
            if (!m_conditional_stack.empty())
                throw std::runtime_error("Unterminated conditional compilation block (missing ENDIF).");
            if (!m_rept_stack.empty())
                throw std::runtime_error("Unterminated REPT block (missing ENDR).");
        }
        bool process_rept_block() {
            if (m_rept_stack.empty() || m_rept_stack.back().recording)
                return false;
            ReptState rept_block = m_rept_stack.back();
            m_rept_stack.pop_back();
            for (int i = 0; i < rept_block.count; ++i) {
                for (const auto& body_line : rept_block.body)
                    process(body_line);
            }
            return true;
        }
        bool process(const std::string& source_line) {
            std::string line = source_line;
            if (m_policy.get_compilation_context().options.comments.enabled)
                process_comments(line);
            if (process_directives(line))
                 return true;
            if (is_skipping())
                return true;
            if (m_policy.get_compilation_context().options.labels.enabled)
                process_label(line);
            if (!line.empty()) 
                return process_instruction(line);
            return true;
        }

    private:
        bool process_comments(std::string& line) {
            if (m_policy.get_compilation_context().options.comments.allow_semicolon) {
                size_t comment_pos = line.find(';');
                if (comment_pos != std::string::npos) {
                    line.erase(comment_pos);
                    return true;
                }
            }
            return false;
        }
        bool process_directives(const std::string& line) {
            if (!m_policy.get_compilation_context().options.directives.enabled)
                return false;
            std::string line_upper = line;
            StringHelper::to_upper(line_upper);
            std::string trimmed_line = line;
            StringHelper::trim_whitespace(trimmed_line);
            std::string upper_trimmed_line = trimmed_line;
            StringHelper::to_upper(upper_trimmed_line);

            if (upper_trimmed_line.rfind("IF ", 0) == 0) {
                if (!m_policy.get_compilation_context().options.directives.allow_conditional_compilation)
                    return false;
                std::string expr_str = trimmed_line.substr(3);
                bool condition_result = false;
                if (!is_skipping()) {
                    Expressions expression(m_policy);
                    int32_t value;
                    if (expression.evaluate(expr_str, value))
                        condition_result = (value != 0);
                }
                m_control_flow_stack.push_back(ControlBlockType::CONDITIONAL);
                m_conditional_stack.push_back({!is_skipping() && condition_result, false});
                return true;
            } else if (upper_trimmed_line.rfind("IFDEF ", 0) == 0) {
                if (!m_policy.get_compilation_context().options.directives.allow_conditional_compilation)
                    return false;
                std::string symbol = trimmed_line.substr(6);
                StringHelper::trim_whitespace(symbol);
                int32_t dummy;
                bool condition_result = !is_skipping() && m_policy.on_symbol_resolve(symbol, dummy);
                m_control_flow_stack.push_back(ControlBlockType::CONDITIONAL);
                m_conditional_stack.push_back({condition_result, false});
                return true;
            } else if (upper_trimmed_line.rfind("IFNDEF ", 0) == 0) {
                if (!m_policy.get_compilation_context().options.directives.allow_conditional_compilation)
                    return false;
                std::string symbol = trimmed_line.substr(7);
                StringHelper::trim_whitespace(symbol);
                int32_t dummy;
                bool condition_result = !is_skipping() && !m_policy.on_symbol_resolve(symbol, dummy);
                m_control_flow_stack.push_back(ControlBlockType::CONDITIONAL);
                m_conditional_stack.push_back({condition_result, false});
                return true;
            } else if (upper_trimmed_line == "ELSE") {
                if (!m_policy.get_compilation_context().options.directives.allow_conditional_compilation)
                    return false;
                if (m_conditional_stack.empty())
                    throw std::runtime_error("ELSE without IF");
                if (m_conditional_stack.back().else_seen)
                    throw std::runtime_error("Multiple ELSE directives for the same IF");
                m_conditional_stack.back().else_seen = true;
                bool parent_is_skipping = m_conditional_stack.size() > 1 && !m_conditional_stack[m_conditional_stack.size() - 2].is_active;
                if (!parent_is_skipping)
                    m_conditional_stack.back().is_active = !m_conditional_stack.back().is_active;
                return true;
            } else if (upper_trimmed_line == "ENDIF") {
                if (!m_policy.get_compilation_context().options.directives.allow_conditional_compilation)
                    return false;
                if (m_conditional_stack.empty())
                    throw std::runtime_error("ENDIF without IF");
                if (m_control_flow_stack.empty() || m_control_flow_stack.back() != ControlBlockType::CONDITIONAL)
                    throw std::runtime_error("Mismatched ENDIF. An ENDR might be missing.");
                m_control_flow_stack.pop_back();
                m_conditional_stack.pop_back();
                return true;
            }
            if (is_skipping())
                return true;
            size_t rept_pos = line_upper.find("REPT ");
            if (rept_pos != std::string::npos && line_upper.substr(0, rept_pos).find_first_not_of(" \t") == std::string::npos) {
                if (!m_policy.get_compilation_context().options.directives.allow_repeat)
                    return false;
                std::string expr_str = line.substr(rept_pos + 5);
                StringHelper::trim_whitespace(expr_str);
                Expressions expression(m_policy);
                m_control_flow_stack.push_back(ControlBlockType::REPT);
                int32_t count;
                if (expression.evaluate(expr_str, count)) {
                    if (count < 0)
                        throw std::runtime_error("REPT count cannot be negative.");
                    m_rept_stack.push_back({count, 0, {}, true});
                } else
                    m_rept_stack.push_back({0, 0, {}, true});
                return true;
            }
            if (!m_rept_stack.empty() && m_rept_stack.back().recording) {
                std::string trimmed_line = line;
                StringHelper::trim_whitespace(trimmed_line);
                std::string upper_trimmed_line = trimmed_line;
                StringHelper::to_upper(upper_trimmed_line);
                if (upper_trimmed_line == "ENDR") {
                    m_rept_stack.back().recording = false;
                    if (m_control_flow_stack.empty() || m_control_flow_stack.back() != ControlBlockType::REPT)
                        throw std::runtime_error("Mismatched ENDR. An ENDIF might be missing.");
                    m_control_flow_stack.pop_back();
                    process_rept_block();
                } else
                    m_rept_stack.back().body.push_back(line);
                return true;
            }
            std::stringstream line_stream(line);
            std::string label, directive, value;
            line_stream >> label >> directive;
            if (!line_stream.fail()) {
                StringHelper::to_upper(directive);
                const auto& constants_options = m_policy.get_compilation_context().options.directives.constants;
                if (constants_options.enabled && (directive == "EQU" || directive == "SET" || directive == "DEFL")) {
                    if (!Keywords::is_valid_label_name(label))
                        throw std::runtime_error("Invalid label name for directive: '" + label + "'");
                    std::getline(line_stream, value);
                    StringHelper::trim_whitespace(value);
                    if ((directive == "SET" || directive == "DEFL") && constants_options.allow_set)
                        m_policy.on_set_directive(label, value);
                    else if (directive == "EQU" && constants_options.allow_equ) 
                        m_policy.on_equ_directive(label, value);
                    else
                        throw std::runtime_error("Directive '" + directive + "' is unknown.");
                    return true;
                }
            }
            size_t org_pos = line_upper.find("ORG ");
            if (org_pos != std::string::npos && line_upper.substr(0, org_pos).find_first_not_of(" \t") == std::string::npos) {
                if (m_policy.get_compilation_context().options.directives.allow_org) {
                    std::string org_value_str = line.substr(org_pos + 4);
                    StringHelper::trim_whitespace(org_value_str);
                    m_policy.on_org_directive(org_value_str);
                } else
                    return false;
                return true;
            }
            size_t align_pos = line_upper.find("ALIGN ");
            if (align_pos != std::string::npos && line_upper.substr(0, align_pos).find_first_not_of(" \t") == std::string::npos) {
                if (m_policy.get_compilation_context().options.directives.allow_align) {
                    std::string align_value_str = line.substr(align_pos + 6);
                    StringHelper::trim_whitespace(align_value_str);
                    m_policy.on_align_directive(align_value_str);
                } else
                    return false;
                return true;
            }        
            size_t incbin_pos = line_upper.find("INCBIN ");
            if (incbin_pos != std::string::npos && line_upper.substr(0, incbin_pos).find_first_not_of(" \t") == std::string::npos) {
                std::string filename_str = line.substr(incbin_pos + 7);
                if (!m_policy.get_compilation_context().options.directives.allow_incbin)
                    return false;
                StringHelper::trim_whitespace(filename_str);
                if (filename_str.length() > 1 && filename_str.front() == '"' && filename_str.back() == '"')
                    m_policy.on_incbin_directive(filename_str.substr(1, filename_str.length() - 2));
                else
                    throw std::runtime_error("INCBIN filename must be in double quotes.");
                return true;
            }
            size_t phase_pos = line_upper.find("PHASE ");
            if (phase_pos != std::string::npos && line_upper.substr(0, phase_pos).find_first_not_of(" \t") == std::string::npos) {
                if (!m_policy.get_compilation_context().options.directives.allow_phase)
                    return false;
                std::string address_str = line.substr(phase_pos + 6);
                StringHelper::trim_whitespace(address_str);
                m_policy.on_phase_directive(address_str);
                return true;
            }
            std::string trimmed_upper = line_upper;
            StringHelper::trim_whitespace(trimmed_upper);
            if (trimmed_upper == "DEPHASE") {
                if (!m_policy.get_compilation_context().options.directives.allow_phase)
                    return false;
                m_policy.on_dephase_directive();
                return true;
            }
            size_t proc_pos = line_upper.find(" PROC");
            if (proc_pos != std::string::npos) {
                std::string proc_name = line.substr(0, proc_pos);
                StringHelper::trim_whitespace(proc_name);
                if (!Keywords::is_valid_label_name(proc_name))
                     throw std::runtime_error("Invalid procedure name: '" + proc_name + "'");
                m_policy.on_proc_begin(proc_name);
                m_control_flow_stack.push_back(ControlBlockType::PROCEDURE);
                return true;
            }
            if (trimmed_upper == "ENDP") {
                if (m_control_flow_stack.empty() || m_control_flow_stack.back() != ControlBlockType::PROCEDURE)
                     throw std::runtime_error("Mismatched ENDP. An ENDIF or ENDR might be missing.");

                m_policy.on_proc_end();
                m_control_flow_stack.pop_back();
                return true;
            }
            return false;
        }
        bool process_label(std::string& line) {
            const auto& label_options = m_policy.get_compilation_context().options.labels;
            if (label_options.allow_colon) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string label = line.substr(0, colon_pos);
                    StringHelper::trim_whitespace(label);
                    if (!label.empty() && label.find_first_of(" \t") == std::string::npos) {
                        if (!Keywords::is_valid_label_name(label))
                            throw std::runtime_error("Invalid label name: '" + label + "'");
                        line.erase(0, colon_pos + 1);
                        m_policy.on_label_definition(label);
                        return true;
                    }
                }
            }
            if (label_options.allow_no_colon) {
                std::string trimmed_line = line;
                StringHelper::trim_whitespace(trimmed_line);
                if (trimmed_line.empty() || (!isalpha(trimmed_line[0]) && trimmed_line[0] != '_' && trimmed_line[0] != '.'))
                    return false;
                size_t first_space = trimmed_line.find_first_of(" \t");
                std::string potential_label = (first_space == std::string::npos) ? trimmed_line : trimmed_line.substr(0, first_space);
                if (Keywords::is_reserved(potential_label))
                    return false;
                if (!Keywords::is_valid_label_name(potential_label))
                    throw std::runtime_error("Invalid label name: '" + potential_label + "'");
                m_policy.on_label_definition(potential_label);
                if (first_space == std::string::npos)
                    line.clear();
                else
                    line = line.substr(line.find(potential_label) + potential_label.length());
                return true;
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

                OperandParser operand_parser(m_policy);
                std::vector<typename OperandParser::Operand> operands;

                std::string ops;
                if (std::getline(instruction_stream, ops)) {
                    StringHelper::trim_whitespace(ops);
                    if (!ops.empty()) {
                        std::string current_operand;
                        bool in_string = false;
                        int paren_level = 0;
                        for (char c : ops) {
                            if (c == '"')
                                in_string = !in_string;
                            else if (c == '(' && !in_string)
                                paren_level++;
                            else if (c == ')' && !in_string)
                                paren_level--;
                            if (c == ',' && !in_string && paren_level == 0) {
                                StringHelper::trim_whitespace(current_operand);
                                if (!current_operand.empty())
                                    operands.push_back(operand_parser.parse(current_operand, mnemonic));
                                current_operand.clear();
                            } else 
                                current_operand += c;
                        }
                        StringHelper::trim_whitespace(current_operand);
                        if (!current_operand.empty()) {
                            operands.push_back(operand_parser.parse(current_operand, mnemonic));
                        }
                    }
                }
                InstructionEncoder encoder(m_policy);
                return encoder.encode(mnemonic, operands);
            }
            return true;
        }

        bool is_skipping() const {
            return !m_conditional_stack.empty() && !m_conditional_stack.back().is_active;
        }

        IAssemblyPolicy& m_policy;
        struct ConditionalState {
            bool is_active;
            bool else_seen;
        };
        enum class ControlBlockType {
            NONE,
            CONDITIONAL,
            REPT,
            PROCEDURE
        };

        struct ReptState {
            int count;
            int current_iteration;
            std::vector<std::string> body;
            bool recording;
        };
        std::vector<ConditionalState> m_conditional_stack;
        std::vector<ReptState> m_rept_stack;
        std::vector<ControlBlockType> m_control_flow_stack;
    };

    CompilationContext m_context;
};

#endif //__Z80ASSEMBLE_H__
