//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble.h
// Verson: 1.1.0
//
// This header provides a single-header Z80 assembler class, `Z80Assembler`, capable of
// compiling Z80 assembly source code into machine code. It supports standard Z80
// mnemonics, advanced expressions, macros, and a rich set of directives.
//
// Copyright (c) 2025 Adam Szulc
// MIT License
//
// Supported Z80 assembler syntax
// ------------------------------
// Each line of code can contain a label, an instruction (mnemonic with operands), and a comment.
//
//   LABEL: MNEMONIC OPERAND1, OPERAND2 ; This is a comment
//
// Labels
//   Labels are used to mark addresses in memory, making them easy to reference.
//   - Global Labels: Start with a letter or _. They can optionally end with a colon (:).
//     Their scope is global.
//   - Local Labels: Start with a dot (.). Their scope is limited to the last defined
//     global label or the current procedure (PROC block).
//   - A label must start with a letter, underscore (_), dot (.), at-sign (@), or question mark (?).
//   - Subsequent characters can also include numbers.
//   - Labels cannot be the same as reserved keywords (mnemonics, directives, or register names).
//
//   Example:
//     GlobalLabel:
//         NOP
//         JR .local_jump // Jumps to the address of GlobalLabel.local_jump
//
//     .local_jump:
//         HALT
//
//     AnotherGlobal:
//     .local_jump: // A different local label with the same name
//         RET
//
// Instructions
//   An instruction consists of a mnemonic (e.g., LD, ADD) and zero or more operands
//   (e.g., A, 5). Operands are separated by commas.
//
// Comments
//   The assembler supports three types of comments:
//   - Single-line: Starts with a semicolon (;).
//   - Single-line (C++ style): Starts with //.
//   - Block (C style): Starts with /* and ends with */.
//
//   Example:
//     LD A, 5     ; This is a comment until the end of the line.
//     // This is also a comment.
//     /*
//       This is a
//       multi-line block comment.
//     */
//
// Registers
// ---------
// The assembler supports all standard Z80 registers and their sub-parts.
//
//   Type                      | Registers
//   --------------------------|---------------------------------
//   8-bit                     | A, B, C, D, E, H, L, I, R
//   16-bit                    | AF, BC, DE, HL, SP, IX, IY
//   Index Register Parts      | IXH, IXL, IYH, IYL
//   Register Pairs (PUSH/POP) | AF, BC, DE, HL, IX, IY
//   Special                   | AF' (alternate register)
//
// Expressions
// -----------
// The assembler features an advanced expression evaluator that calculates values at
// compile time. Expressions can be used anywhere a numeric value is expected
// (e.g., LD A, <expression>).
//
// Operators:
//   Arithmetic, bitwise, and logical operators are supported, respecting standard
//   operator precedence. Both symbols and keywords can be used.
//
//   Category     | Operators (Symbol)      | Operators (Keyword)         | Example
//   -------------|-------------------------|-----------------------------|------------------
//   Arithmetic   | +, -, *, /, %           | MOD                         | (5 + 3) * 2
//   Bitwise      | &, |, ^, ~, <<, >>      | AND, OR, XOR, NOT, SHL, SHR | MY_CONST & 0x0F
//   Logical      | !, &&, ||               |                             | (A > 5) && (B < 3)
//   Comparison   | ==, !=, >, <, >=, <=    | EQ, NE, GT, LT, GE, LE      | MY_CONST == 10
//   Unary        | +, - (sign)             |                             | LD A, -5
//
// Functions:
//   Function                       | Description                                    | Example
//   -------------------------------|------------------------------------------------|-----------------------------
//   HIGH(val), LOW(val)            | Returns the high/low byte of a 16-bit value.   | LD A, HIGH(0x1234)
//   SIN(n), COS(n), TAN(n)         | Trigonometric functions (argument in radians). | DB ROUND(SIN(MATH_PI / 2))
//   ASIN(n), ACOS(n), ATAN(n)      | Inverse trigonometric functions.               | DB ROUND(ACOS(1))
//   SINH(n), COSH(n), TANH(n)      | Hyperbolic functions.                          | DB ROUND(COSH(0))
//   ASINH(n), ACOSH(n), ATANH(n)   | Inverse hyperbolic functions.                  | DB ROUND(ASINH(0))
//   POW(b, e)                      | Power (b to the power of e).                   | DB POW(2, 7)
//   SQRT(x)                        | Square root.                                   | DB SQRT(64)
//   LOG(x), LOG10(x), LOG2(x)      | Logarithms (natural, base 10, base 2).         | DB LOG10(100)
//   ABS(x)                         | Absolute value.                                | DB ABS(-10)
//   ROUND(n), FLOOR(n), CEIL(n)    | Rounding (nearest, down, up).                  | DB ROUND(3.14)
//   TRUNC(n)                       | Truncates the fractional part (towards zero).  | DB TRUNC(3.9)
//   RAND(min, max), RRND(min, max) | Returns a random integer within the range.     | DB RAND(1, 100)
//   RND()                          | Random floating-point number [0.0, 1.0).       | DB RND()
//   MEM(addr)                      | Reads a byte from memory at the given address. | DB MEM(0x8000)
//   SGN(n)                         | Returns the sign of a number (-1, 0, or 1).    | DB SGN(-5)
//   MIN(...), MAX(...)             | Returns the minimum/maximum of a list.         | DB MIN(10, 5, 20)
//
// Special Variables:
//   Variable | Description
//   ---------|------------------------------------------------------------------
//   $, @     | Current logical address (synonyms).
//   $PASS    | The current assembly pass number (starting from 1).
//   $$       | Current physical address (useful in PHASE/DEPHASE blocks).
//
// Constants:
//   Constant   | Description
//   -----------|------------------------------
//   TRUE       | The value 1.
//   FALSE      | The value 0.
//   MATH_PI    | The constant Pi (≈3.14159).
//   MATH_E     | Euler's number (≈2.71828).
//
// Assembler Directives
// --------------------
// Directives are commands for the assembler that control the compilation process.
//
// Data Definition:
//   Directive | Aliases        | Syntax                       | Example
//   ----------|----------------|------------------------------|-------------------------------
//   DB        | DEFB, BYTE, DM | DB <expr>, <string>, ...     | DB 10, 0xFF, "Hello", 'A'
//   DW        | DEFW, WORD     | DW <expr>, <label>, ...      | DW 0x1234, MyLabel
//   DS        | DEFS, BLOCK    | DS <count> [, <fill_byte>]   | DS 10, 0xFF
//   DZ        | ASCIZ          | DZ <string>, <expr>, ...     | DZ "Game Over"
//   DH        | HEX, DEFH      | DH <hex_string>, ...         | DH "DEADBEEF"
//   DG        | DEFG           | DG <bit_string>, ...         | DG "11110000", "XXXX...."
//
// Symbol Definition:
//   Directive | Syntax              | Description                                                        | Example
//   ----------|---------------------|--------------------------------------------------------------------|-----------------
//   EQU       | <label> EQU <expr>  | Assigns a constant value. Redefinition causes an error.            | PORTA EQU 0x80
//   SET       | <label> SET <expr>  | Assigns a numeric value. The symbol can be redefined later.        | Counter SET 0
//   DEFINE    | DEFINE <lbl> <expr> | An alias for SET.                                                  | DEBUG DEFINE 1
//   =         | <label> = <expr>    | By default, acts as EQU. Can be configured to act as SET.          | PORTA = 0x80
//
// Address & Structure Control:
//   Directive | Syntax                  | Description
//   ----------|-------------------------|------------------------------------------------------------------
//   ORG       | ORG <address>           | Sets the origin address for subsequent code.
//   ALIGN     | ALIGN <boundary>        | Aligns the current address to a boundary, filling gaps with zeros.
//   PHASE     | PHASE <address>         | Sets a logical address without changing the physical address.
//   DEPHASE   | DEPHASE                 | Ends a PHASE block, syncing logical address back to physical.
//   PROC      | <name> PROC             | Begins a procedure, creating a new namespace for local labels.
//   ENDP      | ENDP                    | Ends a procedure block.
//   LOCAL     | LOCAL <sym1>, ...       | Declares symbols as local within a macro or procedure.
//
// Conditional Compilation:
//   Directive | Syntax                  | Description
//   ----------|-------------------------|------------------------------------------------------------------
//   IF        | IF <expression>         | Starts a conditional block if the expression is non-zero.
//   ELSE      | ELSE                    | Executes code if the IF condition was false.
//   ENDIF     | ENDIF                   | Ends a conditional block.
//   IFDEF     | IFDEF <symbol>          | Executes code if the symbol is defined.
//   IFNDEF    | IFNDEF <symbol>         | Executes code if the symbol is not defined.
//   IFNB      | IFNB <argument>         | Executes code if a macro argument is not blank.
//   IFIDN     | IFIDN <arg1>, <arg2>    | Executes code if the two text arguments are identical.
//
// Macros:
//   Macros allow you to define reusable code templates.
//   - `MACRO`/`ENDM`: Defines a macro.
//   - `SHIFT`: Shifts positional parameters (\2 becomes \1, etc.).
//   - `EXITM`: Exits the current macro expansion.
//   - Parameters: `{name}` (named), `\1` (positional), `\0` (arg count).
//
//   Example:
//     // A macro that defines a series of bytes from its arguments
//     WRITE_BYTES MACRO
//         REPT \0      // Repeat for the number of arguments
//             DB \1    // Define the CURRENT first argument
//             SHIFT    // Shift the argument queue: \2 becomes \1, etc.
//         ENDR
//     ENDM
//
//     WRITE_BYTES 10, 20, 30 // Generates: DB 10, DB 20, DB 30
//
// Repetition (Loops):
//   Directive | Aliases | Syntax       | Description
//   ----------|---------|--------------|------------------------------------------------------------------
//   REPT      | DUP     | REPT <count> | Repeats a block of code a specified number of times.
//   ENDR      | EDUP    | ENDR         | Ends a REPT block.
//   EXITR     |         | EXITR        | Exits the current REPT loop.
//
//   Inside a REPT loop, the special symbol \@ represents the current iteration (from 1).
//
//   Example:
//     REPT 4
//         DB \@ * 2 // Generates: DB 2, DB 4, DB 6, DB 8
//     ENDR
//
// File Inclusion:
//   Directive | Aliases | Syntax               | Description
//   ----------|---------|----------------------|--------------------------------------------
//   INCLUDE   |         | INCLUDE "<filename>" | Includes the content of another source file.
//   INCBIN    | BINARY  | INCBIN "<filename>"  | Includes a binary file into the output.
//
// Other Directives:
//   Directive | Syntax                     | Description
//   ----------|----------------------------|------------------------------------------------------------------
//   DISPLAY   | DISPLAY <msg>, <expr>...   | Prints a message or value to the console during compilation.
//   ERROR     | ERROR "<message>"          | Halts compilation and prints an error message.
//   ASSERT    | ASSERT <expression>        | Halts compilation if the expression evaluates to false (zero).
//   END       | END                        | Terminates the assembly process.
//
// Supported Instructions (Mnemonics)
// ----------------------------------
// The assembler supports the full standard and most of the undocumented Z80 instruction set.
//
// - 8-Bit Load: LD
// - 16-Bit Load: LD, PUSH, POP
// - Exchange, Block Transfer, and Search: EX, EXX, LDI, LDD, LDIR, LDDR, CPI, CPD, CPIR, CPDR
// - 8-Bit Arithmetic: ADD, ADC, SUB, SBC, AND, OR, XOR, CP, INC, DEC
// - General-Purpose Arithmetic and CPU Control: DAA, CPL, NEG, CCF, SCF, NOP, HALT, DI, EI, IM
// - 16-Bit Arithmetic: ADD, ADC, SBC, INC, DEC
// - Rotate and Shift: RLCA, RLA, RRCA, RRA, RLC, RL, RRC, RR, SLA, SRA, SRL, RLD, RRD
// - Bit Set, Reset, and Test: BIT, SET, RES
// - Jump: JP, JR, DJNZ
// - Call and Return: CALL, RET, RETI, RETN, RST
// - Input and Output: IN, INI, INIR, IND, INDR, OUT, OUTI, OTIR, OUTD, OTDR
// - Undocumented: SLL (alias SLI), OUT (C), etc.

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
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <utility>
#include <optional>
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
            bool allow_cpp_style = true;
        } comments;
        struct DirectiveOptions {
            bool enabled = true;
            struct ConstantOptions {
                bool enabled = true;
                bool allow_equ = true;
                bool allow_set = true;
                bool allow_define = true;
                bool allow_undefine = true;
                bool assignments_as_eqs = true;
            } constants;
            bool allow_org = true;
            bool allow_align = true;
            bool allow_data_definitions = true;
            bool allow_incbin = true;
            bool allow_includes = true;
            bool allow_conditionals = true;
            bool allow_repeat = true;
            bool allow_phase = true;
            bool allow_proc = true;
            bool allow_macros = true;
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
    Z80Assembler(TMemory* memory, ISourceProvider* source_provider, const Options& options = get_default_options()) : m_options(options), m_context(*this) {
        m_context.memory = memory;
        m_context.source_provider = source_provider;
    }
    struct SourceLine {
        std::string file_path;
        size_t line_number;
        std::string content;
    };
    bool compile(const std::string& main_file_path, uint16_t start_addr = 0x0000) {
        Preprocessor preprocessor(m_context);
        std::vector<SourceLine> source_lines;
        try {
            if (!preprocessor.process(main_file_path, source_lines))
                throw std::runtime_error("Could not open main source file: " + main_file_path);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(e.what());
        }
        SymbolsPhase symbols_building(m_context, m_options.compilation.max_passes);
        m_context.address.start = start_addr;
        AssemblyPhase code_generation(m_context);
        std::vector<IPhasePolicy*> m_phases = {&symbols_building, &code_generation};
        for (auto& phase : m_phases) {
            if (!phase)
                continue;
            phase->on_initialize();
            bool end_phase = false;
            m_context.source.current_pass = 1;
            do {
                phase->on_pass_begin();
                Source source(*phase);
                for (size_t i = 0; i < source_lines.size(); ++i) {
                    m_context.source.source_location = &source_lines[i];
                    if (!source.process_line(source_lines[i].content))
                        break;
                }
                if (phase->on_pass_end())
                    end_phase = true;
                else {
                    ++m_context.source.current_pass;
                    phase->on_pass_next();
                }
            } while (!end_phase);
            phase->on_finalize();
        }
        return true;
    }
    const std::map<std::string, SymbolInfo>& get_symbols() const { return m_context.results.symbols_table; }
    const std::vector<BlockInfo>& get_blocks() const { return m_context.results.blocks_table; }
private:
    [[noreturn]]void report_error(const std::string& message) const {
        std::stringstream error_stream;
        if (m_context.source.source_location)
            error_stream << m_context.source.source_location->file_path << ":" << m_context.source.source_location->line_number << ": ";
        error_stream << "error: " << message;
        if (!m_context.macros.stack.empty())
            error_stream << "\n    (in macro '" << m_context.macros.stack.back().name << "')";
        if (!m_context.repeat.stack.empty())
            error_stream << "\n    (in REPT block, iteration " << m_context.repeat.stack.back().current_iteration << ")";
        if (m_context.source.source_location)
             error_stream << "\n    " << m_context.source.source_location->content;
        throw std::runtime_error(error_stream.str());
    }
    const Options m_options;
class Strings {
    public:
        static void trim_whitespace(std::string& s) {
            const char* whitespace = " \t";
            s.erase(0, s.find_first_not_of(whitespace));
            s.erase(s.find_last_not_of(whitespace) + 1);
        }
        static void to_upper(std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        }
        static void replace_words(std::string& str, const std::string& from, const std::string& to) {
            if (from.empty())
                return;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                bool prefix_ok = (start_pos == 0) || std::isspace(str[start_pos - 1]);
                bool suffix_ok = (start_pos + from.length() == str.length()) || std::isspace(str[start_pos + from.length()]);
                if (prefix_ok && suffix_ok) {
                    str.replace(start_pos, from.length(), to);
                    start_pos += to.length();
                } else {
                    start_pos += 1;
                }
            }
        }
        static void replace_labels(std::string& str, const std::string& label, const std::string& replacement) {
            if (label.empty())
                return;
            size_t start_pos = 0;
            while ((start_pos = str.find(label, start_pos)) != std::string::npos) {
                bool prefix_ok = (start_pos == 0) || !std::isalnum(str[start_pos - 1]);
                size_t suffix_pos = start_pos + label.length();
                bool suffix_ok = (suffix_pos == str.length()) || !std::isalnum(str[suffix_pos]);
                if (prefix_ok && suffix_ok) {
                    str.replace(start_pos, label.length(), replacement);
                    start_pos += replacement.length();
                } else {
                    start_pos += 1;
                }
            }
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
        class Tokens {
            public:
                class Token {
                public:
                    Token(const std::string& text) : m_original(text) {}
                    Token(std::string&& text) : m_original(std::move(text)) {}
                    const std::string& original() const { return m_original; }
                    const std::string& upper() const {
                        if (m_upper.empty()) {
                            m_upper.reserve(m_original.length());
                            std::transform(m_original.begin(), m_original.end(), std::back_inserter(m_upper), [](unsigned char c){ return std::toupper(c); });
                        }
                        return m_upper;
                    }
                    bool matches(const std::function<bool(char)>& predicate) const {
                        return std::all_of(m_original.begin(), m_original.end(), predicate);
                    }
                    bool matches_regex(const std::regex& re) const {
                        return std::regex_match(m_original, re);
                    }
                    bool to_number(int32_t& out_value) const {
                        if (!m_number_val.has_value()) {
                            int32_t val;
                            if (Strings::is_number(m_original, val))
                                m_number_val = val;
                            else 
                                m_number_val = std::nullopt;
                        }
                        if (m_number_val.has_value()) {
                            out_value = *m_number_val;
                            return true;
                        }
                        return false;
                    }
                    std::vector<Token> to_arguments(char delimiter = ',') const {
                        if (!m_arguments.has_value()) {
                            std::vector<Token> args;
                            bool in_string = false;
                            int paren_level = 0;
                            size_t start = 0;
                            for (size_t i = 0; i <= m_original.length(); ++i) {
                                if (i < m_original.length()) {
                                    char c = m_original[i];
                                    if (c == '"') 
                                        in_string = !in_string;
                                    else if (!in_string) {
                                        if (c == '(')
                                            paren_level++;
                                        else if (c == ')') 
                                            paren_level--;
                                    }
                                    if (c != delimiter || in_string || paren_level != 0)
                                        continue;
                                }
                                std::string arg_str = m_original.substr(start, i - start);
                                size_t first = arg_str.find_first_not_of(" \t");
                                if (first != std::string::npos) {
                                    size_t last = arg_str.find_last_not_of(" \t");
                                    args.emplace_back(arg_str.substr(first, last - first + 1));
                                }
                                start = i + 1;
                            }
                            m_arguments = std::move(args);
                        }
                        return *m_arguments;
                    }
                private:
                    std::string m_original;
                    mutable std::string m_upper;
                    mutable std::optional<int32_t> m_number_val;
                    mutable std::optional<std::vector<Token>> m_arguments;
                };
                const std::string& get_original_string() const { return m_original_string; }
                void process(const std::string& string) {
                    m_original_string = string;
                    m_tokens.clear();
                    std::stringstream ss(string);
                    std::string token_str;
                    while (ss >> token_str)
                        m_tokens.emplace_back(std::move(token_str));
                }
                size_t count() const { return m_tokens.size(); }
                const Token& operator[](size_t index) const {
                    if (index >= m_tokens.size())
                        throw std::out_of_range("Tokens: index out of range.");
                    return m_tokens[index];
                }
                void merge(size_t start_index, size_t end_index) {
                    if (start_index >= m_tokens.size() || end_index >= m_tokens.size() || start_index > end_index)
                        return;
                    std::string merged;
                    for (size_t i = start_index; i <= end_index; ++i) {
                        if (i > start_index)
                            merged += " ";
                        merged += m_tokens[i].original();
                    }
                    Token merged_token(std::move(merged));
                    m_tokens.erase(m_tokens.begin() + start_index, m_tokens.begin() + end_index + 1);
                    m_tokens.insert(m_tokens.begin() + start_index, merged_token);
                }
                void remove(size_t index) {
                    if (index >= m_tokens.size())
                        return;
                    m_tokens.erase(m_tokens.begin() + index);
                }
            private:
                std::string m_original_string;
                std::vector<Token> m_tokens;
            };
    };
    struct Context {
        Context(Z80Assembler<TMemory>& assembler) : assembler(assembler) {}

        Context(const Context& other) = delete;
        Context& operator=(const Context& other) = delete;

        Z80Assembler<TMemory>& assembler;
        TMemory* memory = nullptr;
        ISourceProvider* source_provider = nullptr;
        struct Address {
            uint16_t start = 0;
            uint16_t current_logical = 0;
            uint16_t current_physical = 0;
        } address;
        struct Symbols {
            struct Symbol {
                bool redefinable;
                int index;
                std::vector<int32_t> value;
                std::vector<bool> undefined;
                bool used;
            };
            struct Scope {
                std::string full_name;
                std::set<std::string> local_symbols;
            };
            std::map<std::string, Symbol*> map;
            std::string last_global_label;
            std::vector<Scope> scope_stack;
        } symbols;
        struct Results {
            std::map<std::string, SymbolInfo> symbols_table;
            std::vector<BlockInfo> blocks_table;
        } results;
        struct Macros {
            struct Macro {
                std::vector<std::string> arg_names;
                std::vector<std::string> body;
                std::vector<std::string> local_labels;
            };
            struct ExpansionState {
                Macro macro;
                std::string name;
                std::vector<std::string> parameters;
                size_t next_line_index;
            };
            std::vector<ExpansionState> stack;
            std::map<std::string, Macro> definitions;
            int unique_id_counter = 0;
            bool in_expansion = false;
            bool is_exiting = false;
        } macros;
        struct Source {
            enum class ControlType {
                NONE,
                CONDITIONAL,
                REPT,
                PROCEDURE
            };
            struct ConditionalState {
                bool is_active;
                bool else_seen;
            };
            size_t current_pass;
            std::vector<ControlType> control_stack;
            const SourceLine* source_location = nullptr;
            std::vector<std::string> lines_stack;
            std::vector<ConditionalState> conditional_stack;
        } source;
        struct Repeat {
            struct State {
                size_t count;
                size_t current_iteration;
                std::vector<std::string> body;
            };
            std::vector<State> stack;
        } repeat;
        struct Defines {
            std::map<std::string, std::string> map;
        } defines;
    };
    class Preprocessor {
    public:
        Preprocessor(Context& context) : m_context(context) {}
        bool process(const std::string& main_file_path, std::vector<SourceLine>& output_source) {
            std::set<std::string> included_files;
            return process_file(main_file_path, output_source, included_files, 0);
        }
    private:
        std::string remove_comments(const std::string& line, bool& in_block_comment) { 
            const auto& comment_options = m_context.assembler.m_options.comments;
            std::string processed_line;
            bool in_string = false;
            bool in_char = false;
            for (size_t i = 0; i < line.length(); ++i) {
                char c = line[i];
                if (in_block_comment) {
                    if (comment_options.allow_block && i + 1 < line.length() && c == '*' && line[i + 1] == '/') {
                        in_block_comment = false;
                        i++;
                        processed_line += ' ';
                    }
                    continue;
                }
                if (c == '\'' && !in_string)
                    in_char = !in_char;
                else if (c == '"' && !in_char)
                    in_string = !in_string;
                if (!in_string && !in_char) {
                    if (comment_options.allow_semicolon && c == ';')
                        break;
                    if (comment_options.allow_cpp_style && i + 1 < line.length() && c == '/' && line[i + 1] == '/')
                        break;
                    if (comment_options.allow_block && i + 1 < line.length() && c == '/' && line[i + 1] == '*') {
                        in_block_comment = true;
                        i++;
                        continue;
                    }
                }
                processed_line += c;
            }
            return processed_line;
        }
        bool process_file(const std::string& identifier, std::vector<SourceLine>& output_source, std::set<std::string>& included_files, size_t include_line) {
            if (included_files.count(identifier))
                m_context.assembler.report_error("Circular or duplicate include detected: " + identifier);
            included_files.insert(identifier);

            std::vector<uint8_t> source_data;
            if (!m_context.source_provider->get_source(identifier, source_data))
                return false;

            std::string source_content(source_data.begin(), source_data.end());
            std::stringstream source_stream(source_content);
            std::string line;
            size_t line_number = 0;
            bool in_macro_def = false;
            bool in_block_comment = false;
            std::string current_macro_name;
            typename Context::Macros::Macro current_macro;
            while (std::getline(source_stream, line)) {
                m_context.source.source_location = new SourceLine{identifier, line_number, ""};
                line_number++;
                if (m_context.assembler.m_options.comments.enabled)
                    line = remove_comments(line, in_block_comment);
                typename Strings::Tokens tokens;
                tokens.process(line);
                if (in_macro_def) {
                    if (tokens.count() == 1 && (tokens[0].upper() == "ENDM" || tokens[0].upper() == "MEND")) {
                        in_macro_def = false;
                        m_context.macros.definitions[current_macro_name] = current_macro;
                    } else {
                        if (tokens.count() > 1 && tokens[0].upper() == "LOCAL") {
                            auto args = tokens[1].to_arguments();
                            for(const auto& arg : args)
                                current_macro.local_labels.push_back(arg.original());
                        } else
                            current_macro.body.push_back(line);
                    }
                    continue;
                }
                if (tokens.count() >= 2 && tokens[1].upper() == "MACRO") {
                    if (!m_context.assembler.m_options.directives.allow_macros)
                        continue;
                    current_macro_name = tokens[0].original();
                    if (!Keywords::is_valid_label_name(current_macro_name))
                        m_context.assembler.report_error("Invalid macro name: '" + current_macro_name + "'");
                    in_macro_def = true;
                    current_macro = {};
                    if (tokens.count() > 2) {
                        tokens.merge(2, tokens.count() - 1);
                        auto arg_tokens = tokens[2].to_arguments();
                        for (const auto& arg_token : arg_tokens)
                            current_macro.arg_names.push_back(arg_token.original());
                    }
                    continue;
                }
                if (m_context.assembler.m_options.directives.allow_includes) {
                    if (tokens.count() == 2 && tokens[0].upper() == "INCLUDE") {
                        const auto& filename_token = tokens[1];
                        if (filename_token.original().length() > 1 && filename_token.original().front() == '"' && filename_token.original().back() == '"') {
                            std::string include_filename = filename_token.original().substr(1, filename_token.original().length() - 2);
                            process_file(include_filename, output_source, included_files, line_number);
                        } else
                            m_context.assembler.report_error("Malformed INCLUDE directive");
                        continue;
                    }
                }
                output_source.push_back({identifier, line_number, line});
            }
            if (in_block_comment) {
                if (m_context.assembler.m_options.comments.allow_block)
                    m_context.assembler.report_error("Unterminated block comment");
            }
            return true;
        }
        Context& m_context;
    };
    class IPhasePolicy;
    class OperandParser {
    public:
        OperandParser(IPhasePolicy& policy) : m_policy(policy) {}
        enum class OperandType { REG8, REG16, IMMEDIATE, MEM_IMMEDIATE, MEM_REG16, MEM_INDEXED, CONDITION, CHAR_LITERAL, STRING_LITERAL, UNKNOWN };
        struct Operand {
            OperandType type = OperandType::UNKNOWN;
            std::string str_val;
            int32_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };
        Operand parse(const std::string& operand_string, const std::string& mnemonic) {
            Operand operand;
            operand.str_val = operand_string;
            std::string upper_operand_string = operand_string;
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
                Strings::to_upper(upper_inner);
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
                        if (Strings::is_number(offset_str, offset_val)) {
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
            typename Expressions::Value value;
            if (expression.evaluate(operand_string, value)) {
                if (value.type == Expressions::Value::Type::STRING) {
                    operand.str_val = value.s_val;
                    operand.type = OperandType::STRING_LITERAL;
                } else {
                    operand.num_val = (int32_t)value.n_val;
                    operand.type = OperandType::IMMEDIATE;
                }
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
        IPhasePolicy& m_policy;
    };
    class Expressions {
    public:
        struct Value {
            enum class Type { NUMBER, STRING };
            Type type = Type::NUMBER;
            double n_val = 0.0;
            std::string s_val;
        };
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
            std::function<Value(Context&, const Value&, const Value&)> apply;
        };
        struct FunctionInfo {
            int num_args; //if negative, it's a variadic function with at least -num_args arguments.
            std::function<Value(Context&, const std::vector<Value>&)> apply;
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
        Expressions(IPhasePolicy& policy) : m_policy(policy){}
        bool evaluate(const std::string& s, int32_t& out_value) const {
            if (!m_policy.context().assembler.m_options.expressions.enabled) {
                if (Strings::is_number(s, out_value))
                    return true;
                return false;
            }
            auto tokens = tokenize_expression(s);
            auto rpn = shunting_yard(tokens);
            return evaluate_rpn(rpn, out_value);
        }
        bool evaluate(const std::string& s, Value& out_value) const {
            if (!m_policy.context().assembler.m_options.expressions.enabled) {
                int32_t num_val;
                if (Strings::is_number(s, num_val)) {
                    out_value = { Value::Type::NUMBER, (double)num_val };
                    return true;
                }
                return false;
            }
            auto tokens = tokenize_expression(s);
            auto rpn = shunting_yard(tokens);
            return evaluate_rpn(rpn, out_value);
        }
    private:
        static const std::map<std::string, OperatorInfo>& get_operator_map() {
            static const std::map<std::string, OperatorInfo> op_map = {
                // unary
                {"_",  {OperatorType::UNARY_MINUS, 10, true, false, [](Context& ctx, const Value& a, const Value&) { if(a.type == Value::Type::STRING) ctx.assembler.report_error("Unary minus not supported for strings."); return Value{Value::Type::NUMBER, -a.n_val}; }}},
                {"~",  {OperatorType::BITWISE_NOT, 10, true, false, [](Context& ctx, const Value& a, const Value&) { if(a.type == Value::Type::STRING) ctx.assembler.report_error("Bitwise NOT not supported for strings."); return Value{Value::Type::NUMBER, (double)(~(int32_t)a.n_val)}; }}},
                {"!",  {OperatorType::LOGICAL_NOT, 10, true, false, [](Context& ctx, const Value& a, const Value&) { if(a.type == Value::Type::STRING) ctx.assembler.report_error("Logical NOT not supported for strings."); return Value{Value::Type::NUMBER, (double)(!a.n_val)}; }}},
                {"NOT", {OperatorType::BITWISE_NOT, 10, true, false, [](Context& ctx, const Value& a, const Value&) { if(a.type == Value::Type::STRING) ctx.assembler.report_error("Bitwise NOT not supported for strings."); return Value{Value::Type::NUMBER, (double)(~(int32_t)a.n_val)}; }}},
                // binary
                {"*",  {OperatorType::MUL,         9, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator * not supported for strings."); if (b.n_val==0) throw std::runtime_error("Division by zero."); return Value{Value::Type::NUMBER, a.n_val * b.n_val}; }}},
                {"/",  {OperatorType::DIV,         9, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator / not supported for strings."); if (b.n_val==0) throw std::runtime_error("Division by zero."); return Value{Value::Type::NUMBER, a.n_val / b.n_val}; }}},
                {"%",  {OperatorType::MOD,         9, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator % not supported for strings."); if ((int32_t)b.n_val==0) throw std::runtime_error("Division by zero."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val % (int32_t)b.n_val)}; }}},
                {"MOD",{OperatorType::MOD,         9, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator MOD not supported for strings."); if ((int32_t)b.n_val==0) throw std::runtime_error("Division by zero."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val % (int32_t)b.n_val)}; }}},
                {"+",  {OperatorType::ADD,         8, false, true,  [](Context& ctx, const Value& a, const Value& b) {
                    if (a.type == Value::Type::STRING || b.type == Value::Type::STRING) {
                        std::string s1 = (a.type == Value::Type::STRING) ? a.s_val : std::to_string((int32_t)a.n_val);
                        std::string s2 = (b.type == Value::Type::STRING) ? b.s_val : std::to_string((int32_t)b.n_val);
                        return Value{Value::Type::STRING, 0.0, s1 + s2};
                    }
                    return Value{Value::Type::NUMBER, a.n_val + b.n_val};
                }}},
                {"-",  {OperatorType::SUB,         8, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator - not supported for strings."); return Value{Value::Type::NUMBER, a.n_val - b.n_val}; }}},
                {"<<", {OperatorType::SHL,         7, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator << not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val << (int32_t)b.n_val)}; }}},
                {">>", {OperatorType::SHR,         7, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator >> not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val >> (int32_t)b.n_val)}; }}},
                {"SHL",{OperatorType::SHL,         7, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator SHL not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val << (int32_t)b.n_val)}; }}},
                {"SHR",{OperatorType::SHR,         7, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator SHR not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val >> (int32_t)b.n_val)}; }}},
                {">",  {OperatorType::GT,          6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator > not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val > b.n_val)}; }}},
                {"GT", {OperatorType::GT,          6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator GT not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val > b.n_val)}; }}},
                {"<",  {OperatorType::LT,          6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator < not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val < b.n_val)}; }}},
                {"LT", {OperatorType::LT,          6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator LT not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val < b.n_val)}; }}},
                {">=", {OperatorType::GTE,         6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator >= not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val >= b.n_val)}; }}},
                {"GE", {OperatorType::GTE,         6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator GE not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val >= b.n_val)}; }}},
                {"<=", {OperatorType::LTE,         6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator <= not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val <= b.n_val)}; }}},
                {"LE", {OperatorType::LTE,         6, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator LE not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val <= b.n_val)}; }}},
                {"==", {OperatorType::EQ,          5, false, true,  [](Context& ctx, const Value& a, const Value& b) {
                    if (a.type != b.type) return Value{Value::Type::NUMBER, 0.0}; // false
                    if (a.type == Value::Type::STRING) return Value{Value::Type::NUMBER, (double)(a.s_val == b.s_val)};
                    return Value{Value::Type::NUMBER, (double)(a.n_val == b.n_val)};
                }}},
                {"EQ", {OperatorType::EQ,          5, false, true,  [](Context& ctx, const Value& a, const Value& b) {
                    if (a.type != b.type) return Value{Value::Type::NUMBER, 0.0}; // false
                    if (a.type == Value::Type::STRING) return Value{Value::Type::NUMBER, (double)(a.s_val == b.s_val)};
                    return Value{Value::Type::NUMBER, (double)(a.n_val == b.n_val)};
                }}},
                {"!=", {OperatorType::NE,          5, false, true,  [](Context& ctx, const Value& a, const Value& b) {
                    if (a.type != b.type) return Value{Value::Type::NUMBER, 1.0}; // true
                    if (a.type == Value::Type::STRING) return Value{Value::Type::NUMBER, (double)(a.s_val != b.s_val)};
                    return Value{Value::Type::NUMBER, (double)(a.n_val != b.n_val)};
                }}},
                {"NE", {OperatorType::NE,          5, false, true,  [](Context& ctx, const Value& a, const Value& b) {
                    if (a.type != b.type) return Value{Value::Type::NUMBER, 1.0}; // true
                    if (a.type == Value::Type::STRING) return Value{Value::Type::NUMBER, (double)(a.s_val != b.s_val)};
                    return Value{Value::Type::NUMBER, (double)(a.n_val != b.n_val)};
                }}},
                {"&",  {OperatorType::BITWISE_AND, 4, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator & not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val & (int32_t)b.n_val)}; }}},
                {"AND",{OperatorType::BITWISE_AND, 4, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator AND not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val & (int32_t)b.n_val)}; }}},
                {"^",  {OperatorType::BITWISE_XOR, 3, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator ^ not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val ^ (int32_t)b.n_val)}; }}},
                {"XOR",{OperatorType::BITWISE_XOR, 3, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator XOR not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val ^ (int32_t)b.n_val)}; }}},
                {"|",  {OperatorType::BITWISE_OR,  2, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator | not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val | (int32_t)b.n_val)}; }}},
                {"OR", {OperatorType::BITWISE_OR,  2, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator OR not supported for strings."); return Value{Value::Type::NUMBER, (double)((int32_t)a.n_val | (int32_t)b.n_val)}; }}},
                {"&&", {OperatorType::LOGICAL_AND, 1, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator && not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val && b.n_val)}; }}},
                {"||", {OperatorType::LOGICAL_OR,  0, false, true,  [](Context& ctx, const Value& a, const Value& b) { if(a.type == Value::Type::STRING || b.type == Value::Type::STRING) ctx.assembler.report_error("Operator || not supported for strings."); return Value{Value::Type::NUMBER, (double)(a.n_val || b.n_val)}; }}}
            };
            return op_map;
        }
        static const std::map<std::string, FunctionInfo>& get_function_map() {
            static const std::map<std::string, FunctionInfo> func_map = {
                {"MEM", {1, [](Context& context, const std::vector<Value>& args) { 
                    uint16_t addr = (uint16_t)((int32_t)args[0].n_val);
                    return Value{Value::Type::NUMBER, (double)context.memory->peek(addr)};
                }}},
                {"HIGH", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, (double)(((int32_t)args[0].n_val >> 8) & 0xFF)}; }}},
                {"LOW",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, (double)((int32_t)args[0].n_val & 0xFF)}; }}},
                {"MIN",  {-2, [](Context&, const std::vector<Value>& args) {
                    if (args.size() < 2) throw std::runtime_error("MIN requires at least two arguments.");
                    double result = args[0].n_val;
                    for (size_t i = 1; i < args.size(); ++i)
                        result = std::min(result, args[i].n_val);
                    return Value{Value::Type::NUMBER, result};
                }}},
                {"MAX",  {-2, [](Context&, const std::vector<Value>& args) {
                    if (args.size() < 2) throw std::runtime_error("MAX requires at least two arguments.");
                    double result = args[0].n_val;
                    for (size_t i = 1; i < args.size(); ++i)
                        result = std::max(result, args[i].n_val);
                    return Value{Value::Type::NUMBER, result};
                }}},
                {"SIN",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, sin(args[0].n_val)}; }}},
                {"COS",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, cos(args[0].n_val)}; }}},
                {"TAN",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, tan(args[0].n_val)}; }}},
                {"ASIN",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, asin(args[0].n_val)}; }}},
                {"ACOS",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, acos(args[0].n_val)}; }}},
                {"ATAN",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, atan(args[0].n_val)}; }}},
                {"ATAN2", {2, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, atan2(args[0].n_val, args[1].n_val)}; }}},
                {"SINH",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, sinh(args[0].n_val)}; }}},
                {"COSH",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, cosh(args[0].n_val)}; }}},
                {"TANH",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, tanh(args[0].n_val)}; }}},
                {"ASINH", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, asinh(args[0].n_val)}; }}},
                {"ACOSH", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, acosh(args[0].n_val)}; }}},
                {"ATANH", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, atanh(args[0].n_val)}; }}},
                {"ABS",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, fabs(args[0].n_val)}; }}},
                {"POW",   {2, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, pow(args[0].n_val, args[1].n_val)}; }}},
                {"SQRT",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, sqrt(args[0].n_val)}; }}},
                {"LOG",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, log(args[0].n_val)}; }}},
                {"LOG10", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, log10(args[0].n_val)}; }}},
                {"LOG2",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, log2(args[0].n_val)}; }}},
                {"EXP",   {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, exp(args[0].n_val)}; }}},
                {"RAND",  {2, [](Context&, const std::vector<Value>& args) {
                    static std::mt19937 gen(0); // Seed with 0 for deterministic results
                    std::uniform_int_distribution<> distrib((int)args[0].n_val, (int)args[1].n_val);
                    return Value{Value::Type::NUMBER, (double)distrib(gen)};
                }}},
                {"RND",   {0, [](Context&, const std::vector<Value>& args) {
                    static std::mt19937 gen(1); // Seed with 1 for deterministic results
                    std::uniform_real_distribution<> distrib(0.0, 1.0);
                    return Value{Value::Type::NUMBER, distrib(gen)};
                }}},
                {"RRND",  {2, [](Context&, const std::vector<Value>& args) {
                    static std::mt19937 gen(0); // Same seed as RAND for same sequence
                    std::uniform_int_distribution<> distrib((int)args[0].n_val, (int)args[1].n_val);
                    return Value{Value::Type::NUMBER, (double)distrib(gen)};
                }}},
                {"FLOOR", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, floor(args[0].n_val)}; }}},
                {"CEIL",  {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, ceil(args[0].n_val)}; }}},
                {"ROUND", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, round(args[0].n_val)}; }}},
                {"TRUNC", {1, [](Context&, const std::vector<Value>& args) { return Value{Value::Type::NUMBER, trunc(args[0].n_val)}; }}},
                {"SGN",   {1, [](Context&, const std::vector<Value>& args) {
                    return Value{Value::Type::NUMBER, (double)((args[0].n_val > 0) - (args[0].n_val < 0))};
                }}}
            };
            return func_map;
        }
        static const std::map<std::string, double>& get_constant_map() {
            static const std::map<std::string, double> const_map = {
                {"MATH_PI",    3.14159265358979323846},
                {"MATH_E",     2.71828182845904523536},
                {"MATH_PI_2",  1.57079632679489661923},
                {"MATH_PI_4",  0.78539816339744830962},
                {"MATH_LN2",   0.69314718055994530942},
                {"MATH_LN10",  2.30258509299404568402},
                {"MATH_LOG2E", 1.44269504088896340736},
                {"MATH_LOG10E",0.43429448190325182765},
                {"MATH_SQRT2", 1.41421356237309504880},
                {"MATH_SQRT1_2",0.70710678118654752440},
                {"TRUE",  1.0},
                {"FALSE", 0.0}
            };
            return const_map;
        }
        bool parse_char_literal(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            if (expr[i] == '\'' && i + 2 < expr.length() && expr[i+2] == '\'') {
                tokens.push_back({Token::Type::CHAR_LITERAL, "", (double)(expr[i+1])});
                i += 2;
                return true;
            }
            return false;
        }
        bool parse_string_literal(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
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
        bool parse_symbol(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            if (!isalpha(expr[i]) && expr[i] != '_' && expr[i] != '@' && expr[i] != '?' && expr[i] != '$' && !(expr[i] == '.' && i + 1 < expr.length() && (isalpha(expr[i+1]) || expr[i+1] == '_')))
                return false;
            size_t j = i;
            if (expr[j] == '$' && j + 1 < expr.length() && isalpha(expr[j+1])) j++; // $PASS
            while (j < expr.length() && (isalnum(expr[j]) || expr[j] == '_' || expr[j] == '.' || expr[j] == '@' || expr[j] == '?' || expr[j] == '$')) {
                if (expr[j] == '.' && j == i && (j + 1 >= expr.length() || !isalnum(expr[j+1]))) break; // Don't treat a single dot as a symbol
                j++;
            }
            std::string symbol_str = expr.substr(i, j - i);
            std::string upper_symbol = symbol_str;
            Strings::to_upper(upper_symbol);

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
        bool parse_number(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
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
                    if (Strings::is_number(expr.substr(i, j - i), val)) {
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
                    m_policy.context().assembler.report_error("Invalid number in expression: " + num_str);
                }
                i = j - 1;
                return true;
            }
            return false;
        }
        bool parse_operator(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
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
        bool parse_parens(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
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
        bool parse_comma(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            if (expr[i] == ',') {
                tokens.push_back({Token::Type::COMMA, ","});
                return true;
            }
            return false;
        }
        std::vector<Token> tokenize_expression(const std::string& expr) const {
            std::vector<Token> tokens;
            for (size_t i = 0; i < expr.length(); ++i) {
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
                    m_policy.context().assembler.report_error("Invalid character in expression: " + std::string(1, c));
            }
            return tokens;
        }
        std::vector<Token> shunting_yard(const std::vector<Token>& infix) const {
            std::vector<Token> postfix;
            std::vector<Token> op_stack;
            std::vector<int> arg_counts;
            for (size_t i = 0; i < infix.size(); ++i) {
                const auto& token = infix[i];
                switch (token.type) {
                    case Token::Type::NUMBER:
                    case Token::Type::CHAR_LITERAL:
                    case Token::Type::STRING_LITERAL:
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
                        if (!op_stack.empty() && op_stack.back().type == Token::Type::FUNCTION) {
                            if (i + 1 < infix.size() && infix[i + 1].type == Token::Type::RPAREN)
                                arg_counts.back() = 0;
                            else
                                arg_counts.back() = 1;
                        }
                        op_stack.push_back(token);
                        break;
                    case Token::Type::RPAREN:
                        while (!op_stack.empty() && op_stack.back().type != Token::Type::LPAREN) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        if (op_stack.empty())
                            m_policy.context().assembler.report_error("Mismatched parentheses in expression.");
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
                        if (op_stack.empty())
                            m_policy.context().assembler.report_error("Comma outside of function arguments or mismatched parentheses.");
                        if (!arg_counts.empty())
                            arg_counts.back()++;
                        break;
                    default: break;
                }
            }
            while (!op_stack.empty()) {
                if (op_stack.back().type == Token::Type::LPAREN || op_stack.back().type == Token::Type::RPAREN)
                    m_policy.context().assembler.report_error("Mismatched parentheses in expression.");
                postfix.push_back(op_stack.back());
                op_stack.pop_back();
            }
            return postfix;
        }
        bool evaluate_rpn(const std::vector<Token>& rpn, Value& out_value) const {
            std::vector<Value> val_stack;
            for (const auto& token : rpn) {
                if (token.type == Token::Type::NUMBER || token.type == Token::Type::CHAR_LITERAL) {
                    val_stack.push_back({Value::Type::NUMBER, token.n_val});
                } else if (token.type == Token::Type::STRING_LITERAL) {
                    std::string s = token.s_val;
                    if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
                        s = s.substr(1, s.length() - 2);
                    val_stack.push_back({Value::Type::STRING, 0.0, s});
                } else if (token.type == Token::Type::SYMBOL) {
                    int32_t sum_val;
                    if (!m_policy.on_symbol_resolve(token.s_val, sum_val))
                        return false;
                    val_stack.push_back({Value::Type::NUMBER, (double)sum_val});
                } else if (token.type == Token::Type::FUNCTION) {
                    auto it = get_function_map().find(token.s_val);
                    if (it == get_function_map().end())
                        m_policy.context().assembler.report_error("Unknown function in RPN evaluation: " + token.s_val);
                    const auto& func_info = it->second;
                    int num_args_provided = token.n_val > 0 ? (int)(token.n_val) : 0;
                    if (func_info.num_args >= 0) {
                        if (num_args_provided != func_info.num_args)
                            m_policy.context().assembler.report_error("Function " + token.s_val + " expects " + std::to_string(func_info.num_args) + " arguments, but got " + std::to_string(num_args_provided));
                    } else { // variadic
                        int min_args = -func_info.num_args;
                        if (num_args_provided < min_args)
                            m_policy.context().assembler.report_error("Function " + token.s_val + " expects at least " + std::to_string(min_args) + " arguments, but got " + std::to_string(num_args_provided));
                    }
                    if (val_stack.size() < (size_t)num_args_provided)
                        m_policy.context().assembler.report_error("Not enough values on stack for function " + token.s_val);
                    std::vector<Value> args;
                    if (num_args_provided > 0) {
                        args.resize(num_args_provided);
                        for (int i = num_args_provided - 1; i >= 0; --i) {
                            args[i] = val_stack.back();
                            val_stack.pop_back();
                        }
                    }
                    val_stack.push_back(it->second.apply(m_policy.context(), args));
                } else if (token.type == Token::Type::OPERATOR) {
                    auto it = get_operator_map().find(token.s_val);
                    if (it == get_operator_map().end())
                        m_policy.context().assembler.report_error("Unknown operator in RPN evaluation: " + token.s_val);
                    const auto& op_info = it->second;
                    if (op_info.is_unary) {
                        if (val_stack.size() < 1)
                            m_policy.context().assembler.report_error("Invalid expression syntax for unary operator.");
                        Value v1 = val_stack.back();
                        val_stack.pop_back();
                        val_stack.push_back(op_info.apply(m_policy.context(), v1, {}));
                        continue;
                    }
                    if (val_stack.size() < 2)
                        m_policy.context().assembler.report_error("Invalid expression syntax for binary operator.");
                    Value v2 = val_stack.back(); 
                    val_stack.pop_back();
                    Value v1 = val_stack.back();
                    val_stack.pop_back();
                    val_stack.push_back(op_info.apply(m_policy.context(), v1, v2));
                }
            }
            if (val_stack.size() != 1) {
                m_policy.context().assembler.report_error("Invalid expression syntax.");
                return false;
            }
            out_value = val_stack.back();
            return true;
        }
        bool evaluate_rpn(const std::vector<Token>& rpn, int32_t& out_value) const {
            Value result_val;
            if (!evaluate_rpn(rpn, result_val)) {
                return false;
            }
            if (result_val.type == Value::Type::STRING) {
                m_policy.context().assembler.report_error("Expression resulted in a string, but a numeric value was expected.");
                return false;
            }
            out_value = (int32_t)result_val.n_val;
            return true;
        }
        IPhasePolicy& m_policy;
    };
    class IPhasePolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        virtual ~IPhasePolicy() = default;
        virtual Context& context() = 0;

        virtual void on_initialize() = 0;
        virtual void on_finalize() = 0;
        
        virtual void on_pass_begin() = 0;
        virtual bool on_pass_end() = 0;
        virtual void on_pass_next() = 0;

        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) = 0;
        virtual void on_label_definition(const std::string& label) = 0;
        virtual void on_equ_directive(const std::string& label, const std::string& value) = 0;
        virtual void on_set_directive(const std::string& label, const std::string& value) = 0;
        virtual void on_org_directive(const std::string& label) = 0;
        virtual void on_phase_directive(const std::string& address_str) = 0;
        virtual void on_dephase_directive() = 0;
        virtual void on_align_directive(const std::string& boundary) = 0;
        virtual void on_incbin_directive(const std::string& filename) = 0;
        virtual void on_proc_begin(const std::string& name) = 0;
        virtual void on_proc_end() = 0;
        virtual void on_local_directive(const std::vector<std::string>& symbols) = 0;
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) = 0;
        virtual void on_if_directive(const std::string& expression) = 0;
        virtual void on_ifdef_directive(const std::string& symbol) = 0;
        virtual void on_ifndef_directive(const std::string& symbol) = 0;
        virtual void on_ifnb_directive(const std::string& arg) = 0;
        virtual void on_ifidn_directive(const std::string& arg1, const std::string& arg2) = 0;
        virtual void on_else_directive() = 0;
        virtual void on_define_directive(const std::string& key, const std::string& value) = 0;
        virtual void on_display_directive(const std::vector<typename Strings::Tokens::Token>& tokens) = 0;
        virtual void on_endif_directive() = 0;
        virtual void on_error_directive(const std::string& message) = 0;
        virtual void on_assert_directive(const std::string& expression) = 0;
        virtual bool is_conditional_block_active() const = 0;
        virtual void on_rept_directive(const std::string& expression) = 0;
        virtual bool on_repeat_recording(const std::string& line) = 0;
        virtual void on_endr_directive() = 0;
        virtual void on_macro(const std::string& name, const std::vector<std::string>& parameters) = 0;
        virtual void on_undefine_directive(const std::string& key) = 0;
        virtual void on_macro_line() = 0;
        virtual void on_unknown_operand(const std::string& operand) = 0;
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) = 0;
        virtual void on_assemble(std::vector<uint8_t> bytes) = 0;
    };
    class BasePolicy : public IPhasePolicy {
    public:
        using Operand = typename IPhasePolicy::Operand;
        using OperandType = typename IPhasePolicy::OperandType;

        BasePolicy(Context& context) : m_context(context) {}
        virtual ~BasePolicy() {}

        virtual Context& context() override {
            return m_context;
        }
        virtual void on_initialize() override {
            m_context.source.conditional_stack.clear();
            m_context.macros.stack.clear();
            m_context.repeat.stack.clear();
            m_context.defines.map.clear();
        }
        virtual void on_finalize() override {
            if (m_context.macros.in_expansion)
                m_context.assembler.report_error("Unterminated macro expansion at end of file.");
            if (!m_context.source.control_stack.empty()) {
                switch (m_context.source.control_stack.back()) {
                    case Context::Source::ControlType::CONDITIONAL:
                        m_context.assembler.report_error("Unterminated conditional compilation block (missing ENDIF).");
                    case Context::Source::ControlType::REPT:
                        m_context.assembler.report_error("Unterminated REPT block (missing ENDR).");
                    case Context::Source::ControlType::PROCEDURE:
                        m_context.assembler.report_error("Unterminated PROC block (missing ENDP).");
                }
            }
        }
        virtual void on_pass_begin() override {
            m_context.address.current_logical = m_context.address.start;
            m_context.address.current_physical = m_context.address.start;
            m_context.macros.unique_id_counter = 0;
            m_context.source.conditional_stack.clear();
            m_context.source.control_stack.clear();
            m_context.defines.map.clear();
        }
        virtual bool on_pass_end() override { return true; }
        virtual void on_pass_next() override {}
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {            
            if (symbol == "$" || symbol == "@") {
                out_value = this->m_context.address.current_logical;
                return true;
            }
            else if (symbol == "$$") {
                out_value = this->m_context.address.current_physical;
                return true;
            }
            std::string upper_symbol = symbol;
            Strings::to_upper(upper_symbol);
            if (upper_symbol == "$PASS") {
                out_value = this->m_context.source.current_pass;
                return true;
            }
            return false;
        }
        virtual void on_label_definition(const std::string& label) override {
            if (!label.empty() && label[0] != '.')
                m_context.symbols.last_global_label = label;
        }
        virtual void on_equ_directive(const std::string& label, const std::string& value) override {}
        virtual void on_set_directive(const std::string& label, const std::string& value) override {}
        virtual void on_org_directive(const std::string& label) override {}
        virtual void on_phase_directive(const std::string& address_str) override {}
        virtual void on_dephase_directive() override {}
        virtual void on_incbin_directive(const std::string& filename) override {
            if (!this->m_context.assembler.m_options.directives.allow_incbin) return;
            std::vector<uint8_t> data;
            if (this->m_context.source_provider->get_source(filename, data))
                on_assemble(data);
            else
                m_context.assembler.report_error("Could not open file for INCBIN: " + filename);
        }
        virtual void on_unknown_operand(const std::string& operand) override {}
        virtual void on_proc_begin(const std::string& name) override {
            if (m_context.symbols.scope_stack.empty())
                m_context.symbols.scope_stack.push_back({name, {}});
            else {
                auto& parent_scope = m_context.symbols.scope_stack.back();
                if (parent_scope.local_symbols.count(name)) {
                    std::string full_name = parent_scope.full_name + "." + name;
                    m_context.symbols.scope_stack.push_back({full_name, {}});
                } else
                    m_context.symbols.scope_stack.push_back({name, {}});
            }
            on_label_definition(name);
        }
        virtual void on_proc_end() override {
            if (m_context.symbols.scope_stack.empty())
                m_context.assembler.report_error("ENDP without PROC.");
            m_context.symbols.scope_stack.pop_back();
        }
        virtual void on_local_directive(const std::vector<std::string>& symbols) override {
            if (m_context.symbols.scope_stack.empty())
                m_context.assembler.report_error("LOCAL directive used outside of a PROC block.");
            auto& current_scope = m_context.symbols.scope_stack.back();
            for (const auto& symbol : symbols) {
                if (!Keywords::is_valid_label_name(symbol) || symbol.find('.') != std::string::npos)
                    m_context.assembler.report_error("Invalid symbol name in LOCAL directive: '" + symbol + "'");
                current_scope.local_symbols.insert(symbol);
            }
        }
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) override { return false; }
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) override {}
        virtual void on_ifdef_directive(const std::string& symbol) override {
            bool is_defined_in_symbols = false;
            int32_t dummy;
            is_defined_in_symbols = on_symbol_resolve(symbol, dummy);
            bool is_defined_in_defines = m_context.defines.map.count(symbol) > 0;
            bool condition_result = is_conditional_block_active() && (is_defined_in_symbols || is_defined_in_defines);
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifndef_directive(const std::string& symbol) override {
            bool is_defined_in_symbols = false;
            int32_t dummy;
            is_defined_in_symbols = on_symbol_resolve(symbol, dummy);
            bool is_defined_in_defines = m_context.defines.map.count(symbol) > 0;
            bool condition_result = is_conditional_block_active() && !is_defined_in_symbols && !is_defined_in_defines;
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifnb_directive(const std::string& arg) override {
            bool condition_result = is_conditional_block_active() && !arg.empty();
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifidn_directive(const std::string& arg1, const std::string& arg2) override {
            std::string s1 = arg1;
            std::string s2 = arg2;
            if (s1.length() >= 2 && s1.front() == '<' && s1.back() == '>')
                s1 = s1.substr(1, s1.length() - 2);
            if (s2.length() >= 2 && s2.front() == '<' && s2.back() == '>')
                s2 = s2.substr(1, s2.length() - 2);
            bool condition_result = is_conditional_block_active() && (s1 == s2);
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_define_directive(const std::string& key, const std::string& value) override {
            m_context.defines.map[key] = value;
        }
        virtual void on_undefine_directive(const std::string& key) override {
            m_context.defines.map.erase(key);
        }
        virtual void on_display_directive(const std::vector<typename Strings::Tokens::Token>& tokens) override {
            enum class DisplayFormat { DEC, BIN, CHR, HEX, HEX_DEC };
            DisplayFormat format = DisplayFormat::DEC;
            std::stringstream ss;
            for (const auto& token : tokens) {
                const std::string& s = token.original();
                const std::string& s_upper = token.upper();
                if (s_upper == "/D")
                    format = DisplayFormat::DEC;
                else if (s_upper == "/B")
                    format = DisplayFormat::BIN;
                else if (s_upper == "/C")
                    format = DisplayFormat::CHR;
                else if (s_upper == "/H")
                    format = DisplayFormat::HEX;
                else if (s_upper == "/A")
                    format = DisplayFormat::HEX_DEC;
                else if (s.length() > 1 && s.front() == '"' && s.back() == '"') {
                    ss << s.substr(1, s.length() - 2);
                } else {
                    typename Expressions::Value value;
                    Expressions expr_eval(*this);
                    if (expr_eval.evaluate(s, value)) {
                        if (value.type == Expressions::Value::Type::STRING) {
                            ss << value.s_val;
                        } else {
                            int32_t num_val = (int32_t)value.n_val;
                            switch (format) {
                                case DisplayFormat::DEC:
                                    ss << num_val; break;
                                case DisplayFormat::BIN: {
                                    uint8_t val8 = num_val;
                                    std::string bin_str;
                                    for(int i = 7; i >= 0; --i)
                                        bin_str += ((val8 >> i) & 1) ? '1' : '0';
                                    ss << bin_str;
                                    break;
                                }
                                case DisplayFormat::CHR:
                                    ss << "'" << (char)(num_val & 0xFF) << "'";
                                    break;
                                case DisplayFormat::HEX:
                                    ss << "0x" << std::hex << num_val << std::dec;
                                    break;
                                case DisplayFormat::HEX_DEC:
                                    ss << "0x" << std::hex << num_val << std::dec << ", " << num_val;
                                    break;
                            }
                        }
                    } else
                        ss << s;
                }
            }
            std::cout << "> " << ss.str() << std::endl;
        }
        virtual void on_error_directive(const std::string& message) override {
            m_context.assembler.report_error("ERROR: " + message);
        }
        virtual void on_assert_directive(const std::string& expression) override {
            Expressions expr_eval(*this);
            int32_t value;
            if (expr_eval.evaluate(expression, value)) {
                if (value == 0)
                    m_context.assembler.report_error("ASSERT failed: " + expression);
            }
        }
        virtual void on_else_directive() override {
            if (m_context.source.conditional_stack.empty())
                m_context.assembler.report_error("ELSE without IF");
            if (m_context.source.conditional_stack.back().else_seen)
                m_context.assembler.report_error("Multiple ELSE directives for the same IF");
            m_context.source.conditional_stack.back().else_seen = true;
            bool parent_is_skipping = m_context.source.conditional_stack.size() > 1 && !m_context.source.conditional_stack[m_context.source.conditional_stack.size() - 2].is_active;
            if (!parent_is_skipping)
                m_context.source.conditional_stack.back().is_active = !m_context.source.conditional_stack.back().is_active;
        }
        virtual void on_endif_directive() override {
            if (m_context.source.conditional_stack.empty())
                m_context.assembler.report_error("ENDIF without IF");
            if (m_context.source.control_stack.empty() || m_context.source.control_stack.back() != Context::Source::ControlType::CONDITIONAL)
                m_context.assembler.report_error("Mismatched ENDIF. An ENDR or ENDP might be missing.");
            m_context.source.control_stack.pop_back();
            m_context.source.conditional_stack.pop_back();
        }
        virtual bool is_conditional_block_active() const override {
            return m_context.source.conditional_stack.empty() || m_context.source.conditional_stack.back().is_active;
        }
        virtual void on_assemble(std::vector<uint8_t> bytes) override {}
        virtual bool on_repeat_recording(const std::string& line) override {
            if (!m_context.repeat.stack.empty()) {
                m_context.repeat.stack.back().body.push_back(line);
                return true;
            }
            return false;
        }
        virtual void on_endr_directive() override {
            if (m_context.source.control_stack.empty() || m_context.source.control_stack.back() != Context::Source::ControlType::REPT) {
                m_context.assembler.report_error("Mismatched ENDR. An ENDIF might be missing.");
            }
            m_context.source.control_stack.pop_back();
            typename Context::Repeat::State& rept_block = m_context.repeat.stack.back();

            std::vector<std::string> expanded_lines;
            for (size_t i = 0; i < rept_block.count; ++i) {
                rept_block.current_iteration = i + 1;
                std::string iteration_str = std::to_string(rept_block.current_iteration);
                for (const auto& line_template : rept_block.body) {
                    std::string line = line_template;
                    typename Strings::Tokens tokens;
                    tokens.process(line);
                    if (tokens.count() > 0 && tokens[0].upper() == "EXITR")
                        break;
                    Strings::replace_words(line, "\\@", iteration_str);
                    expanded_lines.push_back(line);
                }
            }

            if (m_context.macros.in_expansion && !m_context.macros.stack.empty()) {
                typename Context::Macros::ExpansionState& current_macro_state = m_context.macros.stack.back();
                current_macro_state.macro.body.insert(current_macro_state.macro.body.begin() + current_macro_state.next_line_index, expanded_lines.begin(), expanded_lines.end());
            } else {
                m_context.source.lines_stack.insert(m_context.source.lines_stack.end(), expanded_lines.rbegin(), expanded_lines.rend());
            }

            m_context.repeat.stack.pop_back();
        }
        virtual void on_macro(const std::string& name, const std::vector<std::string>& parameters) {
            typename Context::Macros::Macro macro = m_context.macros.definitions.at(name);
            if (!macro.local_labels.empty()) {
                std::string unique_id_str = std::to_string(m_context.macros.unique_id_counter++);
                for (auto& line : macro.body) {
                    for (const auto& label : macro.local_labels) {
                        std::string replacement = "??" + label + "_" + unique_id_str;
                        Strings::replace_labels(line, label, replacement);
                    }
                }
            }                
            m_context.macros.stack.push_back({macro, name, parameters, 0});
            m_context.macros.in_expansion = true;
            m_context.macros.is_exiting = false;
        }
        virtual void on_macro_line() override {
            if (m_context.macros.stack.empty())
                return;
            typename Context::Macros::ExpansionState& current_macro_state = m_context.macros.stack.back();
            if (current_macro_state.next_line_index < current_macro_state.macro.body.size()) {
                std::string line = current_macro_state.macro.body[current_macro_state.next_line_index++];
                if (m_context.repeat.stack.empty()) {                
                    typename Strings::Tokens tokens;
                    tokens.process(line);
                    if (tokens.count() == 1) {
                        if (tokens[0].upper() == "SHIFT") {
                            if (!current_macro_state.parameters.empty())
                                current_macro_state.parameters.erase(current_macro_state.parameters.begin());
                            return;
                        }
                        else if (tokens[0].upper() == "EXITM") {
                            m_context.macros.is_exiting = true;
                            return;
                        }
                    }
                    expand_macro_parameters(line);
                }
                if (!m_context.macros.is_exiting)
                    m_context.source.lines_stack.push_back(line);
            } else {
                m_context.macros.stack.pop_back();
                m_context.macros.in_expansion = !m_context.macros.stack.empty();
            }
        }
    protected:
        void clear_symbols() {
            for (auto& symbol_pair : this->m_context.symbols.map) {
                delete symbol_pair.second;
                symbol_pair.second = nullptr;
            }
            this->m_context.symbols.map.clear();
        }
        void reset_symbols_index() {
            for (auto& symbol_pair : this->m_context.symbols.map) {
                typename Context::Symbols::Symbol* symbol = symbol_pair.second;
                if (symbol)
                    symbol->index = -1;
            }
        }
        std::string get_absolute_symbol_name(const std::string& name) {
            for (auto it = m_context.symbols.scope_stack.rbegin(); it != m_context.symbols.scope_stack.rend(); ++it) {
                if (it->local_symbols.count(name))
                    return it->full_name + "." + name;
            }
            if (name[0] == '.') {
                if (m_context.symbols.last_global_label.empty())
                    m_context.assembler.report_error("Local label '" + name + "' used without a preceding global label.");
                return m_context.symbols.last_global_label + name;
            }
            return name;
        }
        void expand_macro_parameters(std::string& line) {
            typename Context::Macros::ExpansionState& current_macro_state = m_context.macros.stack.back();
            std::string final_line;
            final_line.reserve(line.length());
            for (size_t i = 0; i < line.length(); ++i) {
                if (line[i] == '\\' && i + 1 < line.length()) {
                    char next_char = line[i + 1];
                    if (isdigit(next_char)) {
                        size_t j = i + 1;
                        int param_num = 0;
                        while (j < line.length() && isdigit(line[j])) {
                            param_num = param_num * 10 + (line[j] - '0');
                            j++;
                        }
                        if (param_num == 0)
                            final_line += std::to_string(current_macro_state.parameters.size());
                        else if (param_num > 0 && (size_t)param_num <= current_macro_state.parameters.size())
                            final_line += current_macro_state.parameters[param_num - 1];
                        i = j - 1;
                        continue;
                    } else if (next_char == '{') {
                        size_t start_num = i + 2;
                        size_t end_brace = line.find('}', start_num);
                        if (end_brace != std::string::npos) {
                            std::string_view num_sv(line.data() + start_num, end_brace - start_num);
                            int param_num;
                            auto result = std::from_chars(num_sv.data(), num_sv.data() + num_sv.size(), param_num);
                            if (result.ec == std::errc() && result.ptr == num_sv.data() + num_sv.size()) {
                                if (param_num > 0 && (size_t)param_num <= current_macro_state.parameters.size()) {
                                    final_line += current_macro_state.parameters[param_num - 1];
                                    i = end_brace;
                                    continue;
                                }
                            }
                        }
                    }
                }
                if (line[i] == '{') {
                    size_t end_brace = line.find('}', i + 1);
                    if (end_brace != std::string::npos) {
                        std::string arg_name = line.substr(i + 1, end_brace - i - 1);
                        auto it = std::find(current_macro_state.macro.arg_names.begin(), current_macro_state.macro.arg_names.end(), arg_name);
                        if (it != current_macro_state.macro.arg_names.end()) {
                            size_t arg_index = std::distance(current_macro_state.macro.arg_names.begin(), it);
                            if (arg_index < current_macro_state.parameters.size()) {
                                final_line += current_macro_state.parameters[arg_index];
                                i = end_brace;
                                continue;
                            }
                        }
                    }
                }
                final_line += line[i];
            }
            line = final_line;
        }
        void on_if_directive(const std::string& expression, bool stop_on_evaluate_error) {
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            bool condition_result = false;
            if (is_conditional_block_active()) {
                Expressions expr_eval(*this);
                int32_t value;
                if (expr_eval.evaluate(expression, value))
                    condition_result = (value != 0);
                else {
                    if (stop_on_evaluate_error)
                        throw std::runtime_error("Invalid IF expression: " + expression);
                }
            }
            m_context.source.conditional_stack.push_back({is_conditional_block_active() && condition_result, false});
        }
        void on_rept_directive(const std::string& counter_expr, bool stop_on_evaluate_error) {
            m_context.source.control_stack.push_back(Context::Source::ControlType::REPT);
            Expressions expression(*this);
            int32_t count;
            if (expression.evaluate(counter_expr, count)) {
                if (count < 0)
                    m_context.assembler.report_error("REPT count cannot be negative.");
                m_context.repeat.stack.push_back({(size_t)count, 0, {}});
            } else {
                if (stop_on_evaluate_error)
                    m_context.assembler.report_error("Invalid REPT expression: " + counter_expr);
                m_context.repeat.stack.push_back({0, {}});
            }
        }
        void on_align_directive(const std::string& boundary, bool stop_on_evaluate_error) {
            if (!this->m_context.assembler.m_options.directives.allow_align)
                return;
            Expressions expression(*this);
            int32_t align_val;
            if (expression.evaluate(boundary, align_val) && align_val > 0) {
                uint16_t current_addr = this->m_context.address.current_logical;
                uint16_t new_addr = (current_addr + align_val - 1) & ~(align_val - 1);
                for (uint16_t i = current_addr; i < new_addr; ++i)
                    on_assemble({0x00});
            } else {
                if (stop_on_evaluate_error)
                    m_context.assembler.report_error("Invalid ALIGN expression: " + boundary);
            }
        }
        Context& m_context;
    };
    class SymbolsPhase : public BasePolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;

        SymbolsPhase(Context& context, int max_pass) : BasePolicy(context), m_max_pass(max_pass) {}
        virtual ~SymbolsPhase() {}

        virtual void on_initialize() override {
            this->clear_symbols();
        }
        virtual void on_pass_begin() override {
            BasePolicy::on_pass_begin();
            m_symbols_stable = true;
        }
        virtual bool on_pass_end() override {
            if (!all_used_symbols_defined())
                m_symbols_stable = false;
            if (m_final_pass_scheduled) { 
                if (m_symbols_stable) {
                    this->m_context.results.symbols_table.clear();
                    for (const auto& symbol_pair : this->m_context.symbols.map) {
                        typename Context::Symbols::Symbol* symbol = symbol_pair.second;
                        if (symbol) {
                            int index = symbol->index;
                            if (!symbol->undefined[index])
                                this->m_context.results.symbols_table[symbol_pair.first] = {symbol_pair.first, symbol->value[index]};
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
            if (this->m_context.source.current_pass > m_max_pass) {
                std::string error_msg = "Failed to resolve all symbols after " + std::to_string(m_max_pass) + " passes.";
                if (all_used_symbols_defined())
                    error_msg += " Symbols are defined but their values did not stabilize. Need more passes.";
                else {
                    error_msg += " Undefined symbol(s): ";
                    bool first = true;
                    for (const auto& symbol_pair : this->m_context.symbols.map) {
                        typename Context::Symbols::Symbol* symbol = symbol_pair.second;
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
                this->m_context.assembler.report_error(error_msg);
            }
            this->reset_symbols_index();
        }
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {
            if (BasePolicy::on_symbol_resolve(symbol, out_value))
                return true;
            bool resolved = false;
            std::string actual_symbol_name = this->get_absolute_symbol_name(symbol);
            auto it = this->m_context.symbols.map.find(actual_symbol_name);
            if (it != this->m_context.symbols.map.end()) {
                typename Context::Symbols::Symbol *symbol = it->second;
                if (symbol) {
                    symbol->used = true;
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
            BasePolicy::on_label_definition(label);
            update_symbol(label, this->m_context.address.current_logical, false, false);
        };
        virtual void on_equ_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, false);
        };
        virtual void on_set_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, true);
        };
        virtual void on_org_directive(const std::string& label) override {
            int32_t num_val;
            if (Strings::is_number(label, num_val))
                this->m_context.address.current_logical = this->m_context.address.current_physical = num_val;
            else if (m_symbols_stable) {
                Expressions expression(*this);
                if (expression.evaluate(label, num_val)) {
                    this->m_context.address.current_logical = num_val;
                    this->m_context.address.current_physical = num_val;
                }
            }
        };
        virtual void on_phase_directive(const std::string& label) override {
            int32_t num_val;
            if (Strings::is_number(label, num_val)) {
                this->m_context.address.current_logical = num_val;
            }
            else if (m_symbols_stable) {
                Expressions expression(*this);
                if (expression.evaluate(label, num_val))
                    this->m_context.address.current_logical = num_val;
            }
        }
        virtual void on_dephase_directive() override {
            this->m_context.address.current_logical = this->m_context.address.current_physical;
        }
        virtual bool on_operand_not_matching(const Operand& operand, typename OperandParser::OperandType expected) override {
            if (operand.type == OperandType::UNKNOWN)
                return expected == OperandType::IMMEDIATE || expected == OperandType::MEM_IMMEDIATE;
            return false;
        }
        virtual void on_if_directive(const std::string& expression) override {
            BasePolicy::on_if_directive(expression, false);
        }
        virtual void on_rept_directive(const std::string& counter_expr) override {
            BasePolicy::on_rept_directive(counter_expr, false);
        }
        virtual void on_align_directive(const std::string& boundary) override {
            BasePolicy::on_align_directive(boundary, false);
        }
        virtual void on_assemble(std::vector<uint8_t> bytes) override {
            size_t size = bytes.size();
            this->m_context.address.current_logical += size;
            this->m_context.address.current_physical += size;
        }
    private:
        void on_const(const std::string& label, const std::string& value, bool redefinable) {
            int32_t num_val = 0;
            Expressions expression(*this);
            bool evaluated = expression.evaluate(value, num_val);
            update_symbol(label, num_val, !evaluated, redefinable);
        };
        bool all_used_symbols_defined() const {
            bool all_used_defined = true;
            for (const auto& symbol_pair : this->m_context.symbols.map) {
                typename Context::Symbols::Symbol* symbol = symbol_pair.second;
                if (symbol) {
                    if (symbol->used && symbol->undefined[symbol->index]) {
                        all_used_defined = false;
                        break;
                    }
                }
            }
            return all_used_defined;
        }
        void update_symbol(const std::string& name, int32_t value, bool is_undefined, bool redefinable) {
            std::string actual_name = this->get_absolute_symbol_name(name);
            auto it = this->m_context.symbols.map.find(actual_name);
            if (it == this->m_context.symbols.map.end()) {
                typename Context::Symbols::Symbol* new_symbol = new typename Context::Symbols::Symbol{redefinable, 0, {value}, {is_undefined}};
                this->m_context.symbols.map[actual_name] = new_symbol;
                m_symbols_stable = false;
            } else {
                typename Context::Symbols::Symbol *symbol = it->second;
                if (symbol) {
                    if (!symbol->redefinable && redefinable)
                        this->m_context.assembler.report_error("Cannot redefine constant symbol: " + actual_name);
                    int& index = symbol->index;
                    index++;
                    if (index >= symbol->value.size()) {
                        if (!redefinable)
                            this->m_context.assembler.report_error("Duplicate symbol definition: " + actual_name);
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
        bool m_symbols_stable = false;
        bool m_final_pass_scheduled = false;
        int m_max_pass = 0;
    };
    class AssemblyPhase : public BasePolicy {
    public:
        using Operand = typename OperandParser::Operand;
        using OperandType = typename OperandParser::OperandType;
        
        AssemblyPhase(Context& context) : BasePolicy(context) {}
        virtual ~AssemblyPhase() = default;

        virtual void on_initialize() override {
            this->reset_symbols_index();
        }
        virtual void on_finalize() override {
            this->clear_symbols();
        }
        virtual void on_pass_begin() override {
            BasePolicy::on_pass_begin();
            this->m_context.symbols.last_global_label.clear();
            m_blocks.push_back({this->m_context.address.start, 0});
        }
        virtual bool on_pass_end() override {
            for (auto& block : m_blocks) {
                if (block.second != 0)
                    this->m_context.results.blocks_table.push_back({block.first, block.second});
            }
            return true;
        }
        virtual bool on_symbol_resolve(const std::string& symbol, int32_t& out_value) override {
            if (BasePolicy::on_symbol_resolve(symbol, out_value))
                return true;
            std::string actual_symbol_name = this->get_absolute_symbol_name(symbol);
            auto it = this->m_context.symbols.map.find(actual_symbol_name);
            if (it != this->m_context.symbols.map.end()) {
                typename Context::Symbols::Symbol *symbol = it->second;
                if (symbol) {
                    int index = symbol->index;
                    if (index == -1)
                        index = symbol->value.size() - 1;
                    out_value = symbol->value[index];
                    return true;
                }
            }
            return false;
        }
        virtual void on_label_definition(const std::string& label) override {
            BasePolicy::on_label_definition(label);
            update_symbol_index(label);
        };
        virtual void on_equ_directive(const std::string& label, const std::string& value) override {
            update_symbol_index(label);
        };
        virtual void on_set_directive(const std::string& label, const std::string& value) override {
            update_symbol_index(label);
        };
        virtual void on_org_directive(const std::string& label) override {
            int32_t addr;
            Expressions expression(*this);
            if (expression.evaluate(label, addr)) {
                this->m_context.address.current_logical = addr;
                this->m_context.address.current_physical = addr;
                this->m_blocks.push_back({addr, 0});
            }
            else
                this->m_context.assembler.report_error("Invalid ORG expression: " + label);
        }
        virtual void on_phase_directive(const std::string& address_str) override {
            int32_t new_logical_addr;
            Expressions expression(*this);
            if (expression.evaluate(address_str, new_logical_addr))
                this->m_context.address.current_logical = new_logical_addr;
            else
                this->m_context.assembler.report_error("Invalid PHASE expression: " + address_str);
        }
        virtual void on_dephase_directive() override {
            this->m_context.address.current_logical = this->m_context.address.current_physical;
        }
        virtual void on_unknown_operand(const std::string& operand) override {
            std::string actual_symbol_name = this->get_absolute_symbol_name(operand);
            std::string resolved;
            if (actual_symbol_name != operand)
                resolved = " (resolved to '" + actual_symbol_name + "')";
            this->m_context.assembler.report_error("Invalid expression or unknown operand: '" + operand + "'" + resolved);
        }
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) override {
            this->m_context.assembler.report_error(mnemonic + " jump target out of range. Offset: " + std::to_string(offset));
        }
        virtual void on_if_directive(const std::string& expression) override {
            BasePolicy::on_if_directive(expression, true);
        }
        virtual void on_rept_directive(const std::string& counter_expr) override {
            BasePolicy::on_rept_directive(counter_expr, true);
        }
        virtual void on_align_directive(const std::string& boundary) override {
            BasePolicy::on_align_directive(boundary, true);
        }
        virtual void on_assemble(std::vector<uint8_t> bytes) override {
            for (auto& byte : bytes)
                this->m_context.memory->poke(this->m_context.address.current_physical++, byte);
            this->m_context.address.current_logical += bytes.size();
            if (this->m_blocks.empty())
                this->m_context.assembler.report_error("Invalid code block.");
            this->m_blocks.back().second += bytes.size();
        }
    private:
        void update_symbol_index(const std::string& label) {
            std::string actual_name = this->get_absolute_symbol_name(label);
            auto it = this->m_context.symbols.map.find(actual_name);
            if (it != this->m_context.symbols.map.end()) {
                typename Context::Symbols::Symbol *symbol = it->second;
                if (symbol)
                    symbol->index++;
            }
        };
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
            if (!std::isalpha(s[0]) && s[0] != '_' && s[0] != '.' && s[0] != '@' && s[0] != '?')
                return false;
            for (char c : s) {
                if (!std::isalnum(c) && c != '_' && c != '.' && c != '@' && c != '?')
                    return false;
            }
            return true;
        }
    private:
        static bool is_in_set(const std::string& s, const std::set<std::string>& set) {
            return set.count(s);
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
                "DB", "DEFB", "BYTE", "DM", "DEFS", "DEFW", "DW", "WORD", "DWORD", "DD", "DQ", "DS", "BLOCK", "ORG", "DH", "HEX", "DEFH", "DZ", "ASCIZ", "DG", "DEFG",
                "EQU", "SET", "DEFL", "DEFINE", "UNDEFINE",
                "INCLUDE", "ALIGN", "INCBIN", "BINARY", "PHASE", "DEPHASE", "UNPHASE", "ERROR", "ASSERT", "EXITR",
                "IF", "ELSE", "ENDIF", "IFDEF", "IFNDEF", "IFNB", "IFIDN", "DISPLAY", "END",
                "REPT", "ENDR", "DUP", "EDUP", "MACRO", "ENDM", "EXITM", "LOCAL", "PROC", "ENDP", "SHIFT"
            };
            return directives;
        }
        static const std::set<std::string>& registers() {
            static const std::set<std::string> registers = {"B", "C", "D", "E", "H", "L", "A", "I", "R", "IXH", "IXL", "IYH", "IYL", "BC", "DE", "HL", "SP", "IX", "IY", "AF", "AF'"};
            return registers;
        }
    };
    class Instructions{
    public:
        Instructions(IPhasePolicy& policy) : m_policy(policy) {}
        bool encode(const std::string& mnemonic, const std::vector<typename OperandParser::Operand>& operands) {
            if (Keywords::is_directive(mnemonic)) {
                if (encode_data_block(mnemonic, operands))
                    return true;
            } else if (!Keywords::is_mnemonic(mnemonic))
                m_policy.context().assembler.report_error("Unknown mnemonic: " + mnemonic);
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
            m_policy.context().assembler.report_error("Invalid instruction or operands for mnemonic: " + mnemonic);
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
            const auto& directive_options = m_policy.context().assembler.m_options.directives;
            if (!directive_options.enabled || !directive_options.allow_data_definitions)
                return false;
            if (mnemonic == "DB" || mnemonic == "DEFB" || mnemonic == "BYTE" || mnemonic == "DM") {
                for (const auto& op : ops) {
                    if (op.type == OperandType::STRING_LITERAL) {
                        for (char c : op.str_val)
                            assemble({(uint8_t)c});
                    } else if (match_imm8(op)) 
                        assemble({(uint8_t)op.num_val});
                    else
                        m_policy.context().assembler.report_error("Unsupported or out-of-range operand for DB: " + op.str_val);
                }
                return true;
            } else if (mnemonic == "DW" || mnemonic == "DEFW" || mnemonic == "WORD") {
                for (const auto& op : ops) {
                    if (match_imm16(op) || match_char(op)) {
                        assemble({(uint8_t)(op.num_val & 0xFF), (uint8_t)(op.num_val >> 8)});
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DW: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                return true;
            } else if (mnemonic == "DWORD" || mnemonic == "DD") {
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        assemble({(uint8_t)(op.num_val & 0xFF), (uint8_t)((op.num_val >> 8) & 0xFF), (uint8_t)((op.num_val >> 16) & 0xFF), (uint8_t)((op.num_val >> 24) & 0xFF)});
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DWORD/DD: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                return true;
            } else if (mnemonic == "DQ") {
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        uint64_t val = static_cast<uint64_t>(op.num_val);
                        assemble({(uint8_t)(val & 0xFF), (uint8_t)((val >> 8) & 0xFF), (uint8_t)((val >> 16) & 0xFF), (uint8_t)((val >> 24) & 0xFF), (uint8_t)((val >> 32) & 0xFF), (uint8_t)((val >> 40) & 0xFF), (uint8_t)((val >> 48) & 0xFF), (uint8_t)((val >> 56) & 0xFF)});
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DQ: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                return true;
            } else if (mnemonic == "DH" || mnemonic == "HEX" || mnemonic == "DEFH") {
                if (ops.empty())
                    m_policy.context().assembler.report_error(mnemonic + " requires at least one string argument.");
                for (const auto& op : ops) {
                    if (!match_string(op))
                        m_policy.context().assembler.report_error(mnemonic + " arguments must be string literals. Found: '" + op.str_val + "'");
                    std::string hex_str = op.str_val;
                    std::string continuous_hex;
                    for (char c : hex_str) {
                        if (!isspace(c))
                            continuous_hex += tolower(c);
                    }
                    if (continuous_hex.length() % 2 != 0)
                        m_policy.context().assembler.report_error("Hex string in " + mnemonic + " must have an even number of characters: \"" + hex_str + "\"");
                    for (size_t i = 0; i < continuous_hex.length(); i += 2) {
                        std::string byte_str = continuous_hex.substr(i, 2);
                        uint8_t byte_val;
                        auto result = std::from_chars(byte_str.data(), byte_str.data() + byte_str.size(), byte_val, 16);
                        if (result.ec != std::errc())
                            m_policy.context().assembler.report_error("Invalid hex character in " + mnemonic + ": \"" + byte_str + "\"");
                        assemble({ byte_val });
                    }
                }
                return true;
            } else if (mnemonic == "DZ" || mnemonic == "ASCIZ") {
                if (ops.empty())
                    m_policy.context().assembler.report_error(mnemonic + " requires at least one argument.");
                for (const auto& op : ops) {
                    if (match_string(op)) {
                        for (char c : op.str_val)
                            assemble({(uint8_t)c});
                    } else if (match_imm8(op))
                        assemble({(uint8_t)op.num_val});
                    else
                        m_policy.context().assembler.report_error("Unsupported operand for " + mnemonic + ": " + op.str_val);
                }
                assemble({0x00});
                return true;
            } else if (mnemonic == "DS" || mnemonic == "DEFS" || mnemonic == "BLOCK") {
                if (ops.empty() || ops.size() > 2)
                    m_policy.context().assembler.report_error(mnemonic + " requires 1 or 2 operands.");
                if (!match_imm16(ops[0]))
                    m_policy.context().assembler.report_error(mnemonic + " size must be a number.");
                size_t count = ops[0].num_val;
                uint8_t fill_value = (ops.size() == 2) ? (uint8_t)(ops[1].num_val) : 0;
                for (size_t i = 0; i < count; ++i)
                    assemble({fill_value});
                return true;
            } else if (mnemonic == "DG" || mnemonic == "DEFG") {
                for (const auto& op : ops) {
                    if (!match_string(op))
                        m_policy.context().assembler.report_error("DG directive requires a string literal operand.");
                    std::string content = op.str_val;
                    std::string all_bits;
                    for (char c : content) {
                        if (isspace(c)) continue;
                        if (c == '-' || c == '.' || c == '_' || c == '0')
                            all_bits += '0';
                        else
                            all_bits += '1';
                    }
                    if (all_bits.length() % 8 != 0)
                        m_policy.context().assembler.report_error("Bit stream data for DG must be in multiples of 8. Total bits: " + std::to_string(all_bits.length()));
                    for (size_t i = 0; i < all_bits.length(); i += 8) {
                        std::string byte_str = all_bits.substr(i, 8);
                        assemble({(uint8_t)std::stoul(byte_str, nullptr, 2)});
                    }
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
                int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
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
                int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
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
                    m_policy.context().assembler.report_error("Port for IN instruction must be 8-bit");
                assemble({0xDB, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "OUT" && match_mem_imm16(op1) && op2.str_val == "A" && op1.num_val <= 0xFF) {
                if (op1.num_val > 0xFF)
                    m_policy.context().assembler.report_error("Port for OUT instruction must be 8-bit");
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
                    int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
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
                    m_policy.context().assembler.report_error("OUT (C), (HL) is not a valid instruction");
                assemble({0xED, (uint8_t)(0x41 | (reg_code << 3))});
                return true;
            }
            if (mnemonic == "BIT" && match_imm8(op1) && (match_reg8(op2) || (match_mem_reg16(op2) && op2.str_val == "HL"))) {
                if (op1.num_val > 7)
                    m_policy.context().assembler.report_error("BIT index must be 0-7");
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
                    m_policy.context().assembler.report_error("SET index must be 0-7");
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
                    m_policy.context().assembler.report_error("RES index must be 0-7");
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
                if (op1.num_val > 7)
                    m_policy.context().assembler.report_error("SLL bit index must be 0-7");
                uint8_t reg_code = reg8_map().at(op1.str_val);
                assemble({0xCB, (uint8_t)(0x30 | reg_code)});
                return true;
            }
            if ((mnemonic == "BIT" || mnemonic == "SET" || mnemonic == "RES") && match_imm8(op1) && match_mem_indexed(op2)) {
                if (op1.num_val > 7)
                    m_policy.context().assembler.report_error(mnemonic + " bit index must be 0-7");
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
        IPhasePolicy& m_policy;
    };
    class Source {
    public:
        Source(IPhasePolicy& policy) : m_policy(policy) {}
        bool process_line(const std::string& initial_line) {
            m_policy.context().source.lines_stack.clear();
            m_policy.context().source.lines_stack.push_back(initial_line);
            while (!m_policy.context().source.lines_stack.empty() || m_policy.context().macros.in_expansion) {
                if (m_policy.context().macros.in_expansion) {
                    m_policy.on_macro_line();
                    if (m_policy.context().source.lines_stack.empty())
                        continue;
                }
                std::string current_line = m_policy.context().source.lines_stack.back();
                m_policy.context().source.lines_stack.pop_back();
                m_tokens.process(current_line);
                if (m_tokens.count() == 0)
                    continue;
                if (process_defines())
                    continue;
                if (process_repeat(current_line))
                    continue;
                if (process_conditional_directives()) {
                    if (m_end_of_source)
                        return false;
                    continue;
                }
                if (!m_policy.is_conditional_block_active())
                    continue;
                if (process_macro())
                    continue;
                if (process_label())
                    continue;
                if (process_non_conditional_directives())
                    continue;
                process_instruction();
            }
            return true;
        }
    private:
        bool process_defines() {
            const auto& const_opts = m_policy.context().assembler.m_options.directives.constants;
            if (const_opts.enabled && const_opts.allow_define && m_tokens.count() >= 2) {
                size_t define_idx = 0;
                if (m_tokens.count() > 1 && Keywords::is_valid_label_name(m_tokens[0].original()) && !Keywords::is_reserved(m_tokens[0].upper())) {
                    define_idx = 1;
                }
                if (m_tokens.count() > define_idx && m_tokens[define_idx].upper() == "DEFINE") {
                    if (m_tokens.count() < define_idx + 2)
                        m_policy.context().assembler.report_error("DEFINE directive requires a key.");
                    const std::string& key = m_tokens[define_idx + 1].original();
                    if (!Keywords::is_valid_label_name(key))
                        m_policy.context().assembler.report_error("Invalid key name for DEFINE directive: '" + key + "'");
                    std::string value;
                    if (m_tokens.count() > define_idx + 2) {
                        m_tokens.merge(define_idx + 2, m_tokens.count() - 1);
                        value = m_tokens[define_idx + 2].original();
                    }
                    m_policy.on_define_directive(key, value);
                    return true;
                }
            }
            const auto& defines_map = m_policy.context().defines.map;
            if (defines_map.empty())
                return false;
            std::string rebuilt_line;
            for (size_t i = 0; i < m_tokens.count(); ++i) {
                std::string token_str = m_tokens[i].original();
                if (!(token_str.length() > 1 && token_str.front() == '"' && token_str.back() == '"')) {
                    std::set<std::string> visited;
                    while (defines_map.count(token_str)) {
                        if (visited.count(token_str))
                            m_policy.context().assembler.report_error("Circular DEFINE reference detected for '" + token_str + "'");
                        visited.insert(token_str);
                        token_str = defines_map.at(token_str);
                    }
                }
                if (!rebuilt_line.empty())
                    rebuilt_line += " ";
                rebuilt_line += token_str;
            }
            m_tokens.process(rebuilt_line);
            return false;
        }
        bool process_macro() {
            if (!m_policy.context().assembler.m_options.directives.enabled || !m_policy.context().assembler.m_options.directives.allow_macros)
                return false;
            const auto& potential_macro_name = m_tokens[0].original();
            if (m_policy.context().macros.definitions.count(potential_macro_name)) {
                std::vector<std::string> params;
                if (m_tokens.count() > 1) {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    auto arg_tokens = m_tokens[1].to_arguments();
                    params.reserve(arg_tokens.size());
                    for (const auto& token : arg_tokens)
                        params.push_back(token.original());
                }
                m_policy.on_macro(potential_macro_name, params);
                return true;
            }
            return false;            
        }
        bool process_repeat(const std::string& line) {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (m_policy.context().assembler.m_options.directives.allow_repeat) {
                if (m_tokens.count() >= 2 && (m_tokens[0].upper() == "REPT" || m_tokens[0].upper() == "DUP")) {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    const std::string& expr_str = m_tokens[1].original();
                    m_policy.on_rept_directive(expr_str);
                    return true;
                }
                if (m_tokens.count() == 1 && (m_tokens[0].upper() == "ENDR" || m_tokens[0].upper() == "EDUP")) {
                    m_policy.on_endr_directive();
                    return true;
                }
                return m_policy.on_repeat_recording(line);
            }
            return false;
        }
        bool process_non_conditional_directives() {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (process_constant_directives())
                return true;
            if (process_procedures())
                return true;
            if (process_memory_directives())
                return true;
            if (process_error_directives())
                return true;
            return false;
        }
        bool process_label() {
            if (!m_policy.context().assembler.m_options.labels.enabled)
                return false;
            if (m_tokens.count() == 0)
                return false;
            const auto& label_options = m_policy.context().assembler.m_options.labels;
            const auto& first_token = m_tokens[0];
            std::string label_str = first_token.original();
            bool is_label = false;
            if (label_options.allow_colon) {
                if (label_str.length() > 1 && label_str.back() == ':') {
                    label_str.pop_back();
                    is_label = true;
                }
            }
            if (!is_label && label_options.allow_no_colon) {
                if (!Keywords::is_reserved(first_token.upper())) {
                    if (m_tokens.count() > 1) {
                        const std::string& next_token_upper = m_tokens[1].upper();
                        if (next_token_upper != "EQU" && next_token_upper != "SET" && next_token_upper != "DEFL" && next_token_upper != "=" && next_token_upper != "PROC")
                            is_label = true;
                    } else
                        is_label = true;
                }
            }
            if (is_label) {
                if (!Keywords::is_valid_label_name(label_str))
                    m_policy.context().assembler.report_error("Invalid label name: '" + label_str + "'");
                m_policy.on_label_definition(label_str);
                m_tokens.remove(0);
                return m_tokens.count() == 0;
            }
            return false;
        }
        bool process_instruction() {
            if (m_tokens.count() > 0) {
                std::string mnemonic = m_tokens[0].upper();
                OperandParser operand_parser(m_policy);
                std::vector<typename OperandParser::Operand> operands;
                if (m_tokens.count() > 1) {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    auto arg_tokens = m_tokens[1].to_arguments();
                    for (const auto& arg_token : arg_tokens)
                        operands.push_back(operand_parser.parse(arg_token.original(), mnemonic));
                }
                Instructions instructions(m_policy);
                instructions.encode(mnemonic, operands);
            }
            return true;
        }
        bool process_conditional_directives() {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (!m_policy.context().assembler.m_options.directives.allow_conditionals)
                return false;
            if (m_tokens.count() == 0)
                return false;
            const std::string& directive = m_tokens[0].upper();
            if (directive == "END") {
                m_end_of_source = true;
                return true;
            } else if (directive == "IF") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("IF directive requires an expression.");
                m_tokens.merge(1, m_tokens.count() - 1);
                m_policy.on_if_directive(m_tokens[1].original());
                return true;
            } else if (directive == "IFDEF") {
                if (m_tokens.count() != 2)
                    m_policy.context().assembler.report_error("IFDEF requires a single symbol.");
                m_policy.on_ifdef_directive(m_tokens[1].original());
                return true;
            } else if (directive == "IFNDEF") {
                if (m_tokens.count() != 2)
                    m_policy.context().assembler.report_error("IFNDEF requires a single symbol.");
                m_policy.on_ifndef_directive(m_tokens[1].original());
                return true;
            } else if (directive == "IFNB") {
                if (m_tokens.count() > 1) {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    m_policy.on_ifnb_directive(m_tokens[1].original());
                } else
                    m_policy.on_ifnb_directive("");
                return true;
            } else if (directive == "IFIDN") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("IFIDN directive requires two arguments.");
                m_tokens.merge(1, m_tokens.count() - 1);
                auto args = m_tokens[1].to_arguments();
                if (args.size() != 2)
                    throw std::runtime_error("IFIDN requires exactly two arguments, separated by a comma.");
                m_policy.on_ifidn_directive(args[0].original(), args[1].original());
                return true;
            } else if (directive == "ELSE") {
                m_policy.on_else_directive();
                return true;
            } else if (directive == "ENDIF") {
                m_policy.on_endif_directive();
                return true;
            }
            return false;
        }
        bool process_constant_directives() {
            const auto& const_opts = m_policy.context().assembler.m_options.directives.constants;
            if (!const_opts.enabled || m_tokens.count() < 2)
                return false;            
            if (const_opts.allow_undefine && m_tokens[0].upper() == "UNDEFINE") {
                m_policy.on_undefine_directive(m_tokens[1].original());
                return true;
            }
            if (m_tokens.count() >= 3 && m_tokens[1].original() == "=") {
                const std::string& label = m_tokens[0].original();
                if (Keywords::is_valid_label_name(label) && !Keywords::is_reserved(m_tokens[0].upper())) {
                    m_tokens.merge(2, m_tokens.count() - 1);
                    const std::string& value = m_tokens[2].original();
                    if (const_opts.assignments_as_eqs && const_opts.allow_equ)
                        m_policy.on_equ_directive(label, value);
                    else if (const_opts.allow_set)
                        m_policy.on_set_directive(label, value);
                    return true;
                }
            }
            if (m_tokens.count() >= 3) {
                const std::string& directive = m_tokens[1].upper();
                if (directive == "EQU" || directive == "SET" || directive == "DEFL") {
                    const std::string& label = m_tokens[0].original();
                    if (!Keywords::is_valid_label_name(label))
                        m_policy.context().assembler.report_error("Invalid label name for directive: '" + label + "'");
                    m_tokens.merge(2, m_tokens.count() - 1);
                    const std::string& value = m_tokens[2].original();
                    if ((directive == "SET" || directive == "DEFL") && const_opts.allow_set)
                        m_policy.on_set_directive(label, value);
                    else if (directive == "EQU" && const_opts.allow_equ)
                        m_policy.on_equ_directive(label, value);
                    else
                        return false;
                    return true;
                }
            }
            return false;
        }
        bool process_procedures() {
            if (m_policy.context().assembler.m_options.directives.allow_proc) {
                if (m_tokens.count() == 2 && m_tokens[1].upper() == "PROC") {
                    const std::string& proc_name = m_tokens[0].original();
                    if (Keywords::is_valid_label_name(proc_name)) {
                        m_policy.on_proc_begin(proc_name);
                        m_policy.context().source.control_stack.push_back(Context::Source::ControlType::PROCEDURE);
                        return true;
                    }
                }
                if (m_tokens.count() == 1 && m_tokens[0].upper() == "ENDP") {
                    if (m_policy.context().source.control_stack.empty() || m_policy.context().source.control_stack.back() != Context::Source::ControlType::PROCEDURE)
                        m_policy.context().assembler.report_error("Mismatched ENDP. An ENDIF or ENDR might be missing.");
                    m_policy.on_proc_end();
                    m_policy.context().source.control_stack.pop_back();
                    return true;
                }
                if (m_tokens.count() >= 2 && m_tokens[0].upper() == "LOCAL") {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    const std::string& symbols_str = m_tokens[1].original();
                    std::vector<std::string> symbols;
                    std::stringstream ss(symbols_str);
                    std::string symbol;
                    while (std::getline(ss, symbol, ',')) {
                        Strings::trim_whitespace(symbol);
                        if (!symbol.empty())
                            symbols.push_back(symbol);
                    }
                    m_policy.on_local_directive(symbols);
                    return true;
                }
            }
            return false;
        }
        bool process_error_directives() {
            if (m_tokens.count() == 0)
                return false;
            const auto& directive_token = m_tokens[0];
            const std::string& directive_upper = directive_token.upper();
            if (directive_upper == "ERROR") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("ERROR directive requires a message.");
                m_tokens.merge(1, m_tokens.count() - 1);
                m_policy.on_error_directive(m_tokens[1].original());
                return true;
            } else if (directive_upper == "ASSERT") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("ASSERT directive requires an expression.");
                m_tokens.merge(1, m_tokens.count() - 1);
                m_policy.on_assert_directive(m_tokens[1].original());
                return true;
            }
            if (directive_upper == "DISPLAY") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("DISPLAY directive requires arguments.");
                m_tokens.merge(1, m_tokens.count() - 1);
                auto args = m_tokens[1].to_arguments();
                m_policy.on_display_directive(args);
                return true;
            }
            return false;
        }
        bool process_memory_directives() {
            if (m_tokens.count() == 0)
                return false;
            const auto& directive_token = m_tokens[0];
            const std::string& directive_upper = directive_token.upper();
            if (m_policy.context().assembler.m_options.directives.allow_org && directive_upper == "ORG") {
                if (m_tokens.count() <= 1)
                    m_policy.context().assembler.report_error("ORG directive requires an address argument.");
                m_tokens.merge(1, m_tokens.count() - 1);
                m_policy.on_org_directive(m_tokens[1].original());
                return true;
            }
            if (m_policy.context().assembler.m_options.directives.allow_align && directive_upper == "ALIGN") {
                if (m_tokens.count() <= 1)
                    m_policy.context().assembler.report_error("ALIGN directive requires a boundary argument.");
                m_tokens.merge(1, m_tokens.count() - 1);
                m_policy.on_align_directive(m_tokens[1].original());
                return true;
            }
            if (m_policy.context().assembler.m_options.directives.allow_incbin && (directive_upper == "INCBIN" || directive_upper == "BINARY")) {
                if (m_tokens.count() != 2)
                    m_policy.context().assembler.report_error(directive_upper + " directive requires exactly one argument.");
                const auto& filename_token = m_tokens[1];
                const std::string& filename_str = filename_token.original();
                if (filename_str.length() > 1 && filename_str.front() == '"' && filename_str.back() == '"')
                    m_policy.on_incbin_directive(filename_str.substr(1, filename_str.length() - 2));
                else
                    m_policy.context().assembler.report_error(directive_upper + " filename must be in double quotes.");
                return true;
            }
            if (m_policy.context().assembler.m_options.directives.allow_phase) {
                if (directive_upper == "PHASE") {
                    if (m_tokens.count() <= 1)
                        m_policy.context().assembler.report_error("PHASE directive requires an address argument.");
                    m_tokens.merge(1, m_tokens.count() - 1);
                    m_policy.on_phase_directive(m_tokens[1].original());
                    return true;
                } else if (directive_upper == "DEPHASE" || directive_upper == "UNPHASE") {
                    if (m_tokens.count() > 1)
                        m_policy.context().assembler.report_error("DEPHASE directive does not take any arguments.");
                    m_policy.on_dephase_directive();
                    m_tokens.remove(0);
                    return true;
                }
            }
            return false;
        }
        IPhasePolicy& m_policy;
        typename Strings::Tokens m_tokens;
        bool m_end_of_source = false;
    };
    Context m_context;
};

#endif //__Z80ASSEMBLE_H__