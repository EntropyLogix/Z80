//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Assemble.h
// Version: 1.1.7a
//
// This header provides a single-header Z80 assembler class, `Z80Assembler`, capable of
// compiling Z80 assembly source code into machine code. It supports standard Z80
// mnemonics, advanced expressions, macros, and a rich set of directives.
//
// Copyright (c) 2025-2026 Adam Szulc
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
//   Category     | Operators (Symbol)      | Operators (Keyword)         | Description
//   -------------|-------------------------|-----------------------------|------------------------------------
//   Arithmetic   | +, -, *, /, %           | MOD                         | Addition, subtraction, etc.
//   Bitwise      | &, |, ^, ~, <<, >>      | AND, OR, XOR, NOT, SHL, SHR | Bitwise operations.
//   Logical      | !, &&, ||               |                             | Logical NOT, AND, OR.
//   Comparison   | ==, !=, >, <, >=, <=    | EQ, NE, GT, LT, GE, LE      | Comparison operators.
//   Unary        | +, - (sign)             | DEFINED                     | Sign operators and symbol check.
//   Conditional  | ? :                     |                             | Ternary operator (e.g., `cond ? val1 : val2`).
//   String       | ##                      |                             | String concatenation.
//
//   Operator Details:
//   - +  : Adds numbers. Single-char strings are treated as ASCII codes. Multi-char strings cause error.
//   - ## : Concatenates operands as strings (e.g., "R" ## 2 -> "R2").
//
// Functions:
//   The assembler supports a wide range of built-in functions for compile-time calculations.
//
//   String & Type Conversion:
//   Function                       | Description
//   -------------------------------|--------------------------------------------------------------------------------
//   ISSTRING(val)                  | Returns TRUE if the argument is a string.
//   ISNUMBER(val)                  | Returns TRUE if the argument is a number or a string that can be converted to a number.
//   STR(num)                       | Converts a number to its string representation.
//   VAL(str)                       | Converts a string representation of a number into a numeric value.
//   CHR(num)                       | Returns a single-character string from an ASCII code.
//   ASC(str)                       | Returns the ASCII code of the first character of a string.
//   CHARS(str)                     | Converts a string of up to 4 characters into a little-endian integer value.
//   STRLEN(str)                    | Returns the length of a string.
//   SUBSTR(str, pos, len)          | Extracts a substring of a given length starting from a specified position (0-based).
//   STRIN(str, sub)                | Finds the starting position (1-based) of a substring within a string. Returns 0 if not found.
//   REPLACE(str, old, new)         | Replaces all occurrences of a substring with a new string.
//   LCASE(str)                     | Converts a string to lowercase.
//   UCASE(str)                     | Converts a string to uppercase.
//
//   Bit, Byte & Memory:
//   Function                       | Description
//   -------------------------------|--------------------------------------------------------------------------------
//   {addr}                         | Reads a byte from memory at the specified address during compilation.
//   HIGH(val)                      | Returns the high byte of a 16-bit value.
//   LOW(val)                       | Returns the low byte of a 16-bit value.
//   MEM(addr)                      | Reads a byte from memory at the specified address during the final assembly pass.
//   FILESIZE("file")               | Returns the size of a file in bytes. Reports an error if the file does not exist.
//
//   Mathematical Functions:
//   Function                       | Description
//   -------------------------------|--------------------------------------------------------------------------------
//   MIN(n1, n2,...), MAX(n1, n2,...) | Returns the minimum/maximum value from a list of numbers.
//   ABS(x)                         | Returns the absolute value of a number.
//   SGN(n)                         | Returns the sign of a number (-1 for negative, 0 for zero, 1 for positive).
//   POW(base, exp)                 | Calculates base raised to the power of exp.
//   SQRT(x)                        | Calculates the square root of a number.
//   HYPOT(x, y)                    | Calculates the hypotenuse of a right-angled triangle (sqrt(x^2 + y^2)).
//   FMOD(x, y)                     | Returns the floating-point remainder of x/y.
//   LOG(x), LOG10(x), LOG2(x)      | Calculates the natural, base-10, and base-2 logarithm.
//   ROUND(n), FLOOR(n), CEIL(n)    | Rounds a number to the nearest integer, down, or up.
//   TRUNC(n)                       | Truncates the fractional part of a number (rounds towards zero).
//   SIN(n), COS(n), TAN(n)         | Trigonometric functions (angle in radians).
//   ASIN(n), ACOS(n), ATAN(n)      | Inverse trigonometric functions.
//   ATAN2(y, x)                    | Arc tangent of y/x, using the signs of arguments to determine the quadrant.
//   SINH(n), COSH(n), TANH(n)      | Hyperbolic functions.
//   ASINH(n), ACOSH(n), ATANH(n)   | Inverse hyperbolic functions.
//   RAND(min, max)                 | Returns a pseudo-random integer within the specified range [min, max].
//   RRND(min, max)                 | Returns a pseudo-random integer from a generator seeded to 0.
//   RND()                          | Returns a pseudo-random float between 0.0 and 1.0 from a generator seeded to 1.
//
// Special Variables:
//   Variable | Description
//   ---------|------------------------------------------------------------------
//   $, @     | Current logical address.
//   $PASS    | The current assembly pass number (starting from 1).
//   $PHASE   | The current phase number (1 for symbols, 2 for assembly).
//   $$       | Current physical address (useful in PHASE/DEPHASE blocks).
//
// Constants:
//   Constant     | Description
//   -------------|-------------------------------------------------
//   TRUE         | The value 1.
//   FALSE        | The value 0.
//   MATH_PI      | The constant Pi (≈3.14159).
//   MATH_E       | Euler's number (≈2.71828).
//   MATH_PI_2    | Pi / 2.
//   MATH_PI_4    | Pi / 4.
//   MATH_LN2     | Natural logarithm of 2.
//   MATH_LN10    | Natural logarithm of 10.
//   MATH_LOG2E   | Base-2 logarithm of E.
//   MATH_LOG10E  | Base-10 logarithm of E.
//   MATH_SQRT2   | Square root of 2.
//   MATH_SQRT1_2 | Square root of 1/2.
//
// Assembler Directives
// --------------------
// Directives are commands for the assembler that control the compilation process.
//
// Data Definition:
//   Directive | Aliases              | Syntax                       | Example
//   ----------|----------------------|------------------------------|-------------------------------
//   DB        | DEFB, BYTE, DM, DEFM | DB <expr>, <string>, ...     | DB 10, 0xFF, "Hello", 'A'
//   DW        | DEFW, WORD           | DW <expr>, <label>, ...      | DW 0x1234, MyLabel
//   DS        | DEFS, BLOCK          | DS <count> [, <fill_byte>]   | DS 10, 0xFF
//   DZ        | ASCIZ                | DZ <string>, <expr>, ...     | DZ "Game Over"
//   DH        | HEX, DEFH            | DH <hex_string>, ...         | DH "DEADBEEF"
//   DG        | DEFG                 | DG <bit_string>, ...         | DG "11110000", "XXXX...."
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
//   ENDP      | [<name>] ENDP           | Ends a procedure block. If <name> is provided, it must match the PROC name.
//   LOCAL     | LOCAL <sym1>, ...       | Declares symbols as local within a macro or procedure.
//
// Conditional Compilation:
//   Directive | Syntax                  | Description
//   ----------|-------------------------|------------------------------------------------------------------
//   IF        | IF <expression>         | Starts a conditional block if the expression is non-zero.
//   ELSE      | ELSE                    | Executes code if the IF condition was false.
//   ENDIF     | ENDIF                   | Ends a conditional block.
//   IFEXIST   | IFEXIST <filename>      | Executes code if the specified file exists.
//   IFDEF     | IFDEF <symbol>          | Executes code if the symbol is defined.
//   IFNDEF    | IFNDEF <symbol>         | Executes code if the symbol is not defined.
//   IFNB      | IFNB <argument>         | Executes code if a macro argument is not blank.
//   IFIDN     | IFIDN <arg1>, <arg2>    | Executes code if the two text arguments are identical.
//
// Macros:
//   Macros allow you to define reusable code templates.
//
//   Directive | Syntax                  | Description
//   ----------|-------------------------|------------------------------------------------------------------
//   MACRO     | <name> MACRO [args...]  | Defines a macro.
//   ENDM      | ENDM                    | Ends a macro definition.
//   SHIFT     | SHIFT                   | Shifts positional parameters (\2 becomes \1, etc.).
//   EXITM     | EXITM                   | Exits the current macro expansion.
//   LOCAL     | LOCAL <sym1>, ...       | Declares symbols as local to the macro instance.
//
//   Parameters: `{name}` (named), `\1`...\`\9` (positional), `\0` (arg count).
//
//   Example:
//     // A macro that defines a series of bytes from its arguments
//     WRITE_BYTES MACRO
//         LOCAL loop   // Declare a local label
//         REPT \0      // Repeat for the number of arguments
//             DB \1    // Define the CURRENT first argument
//             SHIFT    // Shift the argument queue: \2 becomes \1, etc.
//         ENDR
//     ENDM
//
//     WRITE_BYTES 10, 20, 30 // Generates: DB 10, DB 20, DB 30
//
// Optimizations
// -------------
// The assembler includes an optimization pass that can be controlled via the `OPTIMIZE` directive.
//
//   Directive | Syntax                                   | Description
//   ----------|------------------------------------------|------------------------------------------------------------------
//   OPTIMIZE  | OPTIMIZE [PUSH|POP] [+/-FLAG] [KEYWORD]  | Controls optimization settings.
//
//   Flags:
//   - BRANCH_SHORT: Optimizes `JP` to `JR` if the target is within range (-128 to +127).
//   - BRANCH_LONG:  Automatically converts `JR` to `JP` if target is out of range.
//   - JUMP_THREAD:  Optimizes jump chains (e.g., `JP A` where `A` contains `JP B` becomes `JP B`).
//   - DCE:          Dead Code Elimination (e.g., removes `LD A, A`).
//   - OPS_RST:      Optimizes `CALL n` to `RST n` if n is a restart vector. (Safe)
//   Unsafe:
//   - OPS_XOR:      Optimizes `LD A, 0` to `XOR A`. (Unsafe: modifies flags)
//   - OPS_INC:      Optimizes `ADD A, 1` to `INC A`, `SUB 1` to `DEC A`, `ADD A, -1` to `DEC A`, `SUB -1` to `INC A`. (Unsafe: modifies flags)
//   - OPS_OR:       Optimizes `CP 0` to `OR A`. (Unsafe: modifies flags)
//   - OPS_LOGIC:    Optimizes `OR 0`/`XOR 0` to `OR A` and `AND 0` to `XOR A`. (Unsafe: modifies flags)
//   - OPS_SLA:      Optimizes `SLA A` to `ADD A, A`. (Unsafe: modifies flags)
//   - OPS_ROT:      Optimizes `RLC A` (etc.) to `RLCA` (etc.). (Unsafe: modifies flags)
//   - OPS_ADD0:     Optimizes `ADD A, 0` to `OR A`. (Unsafe: modifies P/V flag behavior)
//
//   Keywords:
//   - NONE:         Disable all optimizations.
//   - SPEED:        Enable optimizations for speed (OPS_*, DCE, JUMP_THREAD). Disables BRANCH_SHORT.
//   - SIZE:         Enable optimizations for size (enables all, including BRANCH_SHORT).
//   - ALL:          Enable all optimizations.
//   - OPS:          Enable all OPS_* optimizations.
//   - UNSAFE:       Enable all unsafe optimizations.
//
//   Stack Control:
//   - PUSH:         Saves the current optimization state.
//   - POP:          Restores the previously saved optimization state.
//
//   Example:
//     OPTIMIZE PUSH           ; Save current state
//     OPTIMIZE +OPS_XOR  ; Enable XOR optimization locally
//     LD A, 0                 ; Optimized to XOR A
//     OPTIMIZE POP            ; Restore previous state
//
// Repetition (Loops):
//   Directive | Aliases | Syntax       | Description
//   ----------|---------|--------------|------------------------------------------------------------------
//   REPT      | DUP     | REPT <count> | Repeats a block of code a specified number of times.
//   ENDR      | EDUP    | ENDR         | Ends a REPT block.
//   WHILE     |         | WHILE <expr> | Repeats a block of code as long as the expression is true.
//   ENDW      |         | ENDW         | Ends a WHILE block.
//   EXITR     |         | EXITR        | Exits the current REPT loop.
//   BREAK     |         | BREAK        | Exits the current loop (REPT or WHILE).
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
//   DISPLAY   | DISPLAY <msg>, <expr>...   | Prints a message or value to the console during compilation. (Alias: ECHO)
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

class IFileProvider {
public:
	virtual ~IFileProvider() = default;
	virtual bool read_file(const std::string& identifier, std::vector<uint8_t>& data) = 0;
	virtual size_t file_size(const std::string& identifier) = 0;
	virtual bool exists(const std::string& identifier) = 0;
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
                bool assignments_as_set = true;
            } constants;
            bool allow_org = true;
            bool allow_align = true;
            bool allow_data_definitions = true;
            bool allow_incbin = true;
            bool allow_includes = true;
            bool allow_conditionals = true;
            bool allow_repeat = true;
            bool allow_phase = true;
            bool allow_while = true;
            bool allow_proc = true;
            bool allow_macros = true;
            bool allow_optimize = true;
        } directives;
        struct ExpressionOptions {
            bool enabled = true;
        } expressions;
        struct NumberOptions {
            bool allow_hex_prefix_0x = true;
            bool allow_hex_prefix_dollar = true;
            bool allow_hex_suffix_h = true;
            bool allow_bin_suffix_b = true;
            bool allow_bin_prefix_percent = true;
        } numbers;
        struct CompilationOptions {
            int max_passes = 10;
            int max_while_iterations = 10000;
            bool enable_optimization = true;
        } compilation;
    };
    static const Options& get_default_options() {
        static const Options default_options;
        return default_options;
    };
    struct SymbolInfo {
        std::string name;
        int32_t value;
        bool label;
    };
    struct BlockInfo {
        uint16_t start_address;
        uint16_t size;
    };
    struct SourceLine {
        std::string file_path;
        size_t line_number;
        std::string content;
    };
    struct ListingLine {
        SourceLine source_line;
        uint16_t address;
        std::vector<uint8_t> bytes;
    };
    Z80Assembler(TMemory* memory, IFileProvider* source_provider, const Options& options = get_default_options()) : m_options(options), m_context(*this), m_keywords(m_context) {
        m_context.memory = memory;
        m_context.source_provider = source_provider;
        const auto& op_map = Expressions::get_operator_map();
        max_operator_len = 0;
        for (const auto& pair : op_map) {
            if (pair.first.length() > max_operator_len)
                max_operator_len = pair.first.length();
        }
    }
    virtual ~Z80Assembler() {}
    virtual bool compile(const std::string& main_file_path, uint16_t start_addr = 0x0000) {
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
        std::vector<IPhasePolicy*> phases = {&symbols_building, &code_generation};
        m_context.phase_index = 1;
        for (auto& phase : phases) {
            if (!phase)
                continue;
            m_context.current_phase = phase;
            phase->on_initialize();
            bool end_phase = false;
            m_context.source.current_pass = 1;
            do {
                phase->on_pass_begin();
                Source source(*phase);
                m_context.source.parser = &source;
                for (const auto& line : source_lines) {
                    m_context.source.source_location = &line;
                    if (!source.process_line(line.content))
                        break;
                }
                if (phase->on_pass_end())
                    end_phase = true;
                else {
                    ++m_context.source.current_pass;
                    phase->on_pass_next();
                }
            } while (!end_phase);
            m_context.phase_index++;
            phase->on_finalize();
        }
        return true;
    }
    virtual const std::map<std::string, SymbolInfo>& get_symbols() const { return m_context.results.symbols_table; }
    virtual const std::vector<BlockInfo>& get_blocks() const { return m_context.results.blocks_table; }
    virtual const std::vector<ListingLine>& get_listing() const { return m_context.results.listing; }
protected:
    [[noreturn]] virtual void report_error(const std::string& message) const {
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
    class IPhasePolicy;
    class Expressions;
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
        static bool is_number(const std::string& s, int32_t& out_value, const typename Options::NumberOptions& options = get_default_options().numbers) {
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
                if (!options.allow_hex_prefix_0x)
                    return false;
                start += 2;
                base = 16;
            } else if ((end - start) > 2 && (*start == '0' && (*(start + 1) == 'b' || *(start + 1) == 'B'))) {
                start += 2;
                base = 2;
            } else if ((end - start) > 1 && *start == '$') {
                if (!options.allow_hex_prefix_dollar)
                    return false;
                start += 1;
                base = 16;
            } else if ((end - start) > 1 && *start == '%') {
                if (!options.allow_bin_prefix_percent)
                    return false;
                start += 1;
                base = 2;
            } else if ((end - start) > 0) {
                char last_char = *(end - 1);
                if (last_char == 'H' || last_char == 'h') {
                    if (!options.allow_hex_suffix_h)
                        return false;
                    end -= 1;
                    base = 16;
                } else if (last_char == 'B' || last_char == 'b') {
                    if (!options.allow_bin_suffix_b)
                        return false;
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
                    bool to_number(int32_t& out_value, const typename Options::NumberOptions& options = get_default_options().numbers) const {
                        if (!m_number_val.has_value()) {
                            int32_t val;
                            if (Strings::is_number(m_original, val, options))
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
        IFileProvider* source_provider = nullptr;
        IPhasePolicy* current_phase = nullptr;
        int phase_index = 0;
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
                bool label;
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
            std::vector<ListingLine> listing;
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
                REPEAT,
                WHILE,
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
            typename Z80Assembler<TMemory>::Source* parser = nullptr;
        } source;
        struct Repeat {
            struct State {
                size_t count;
                size_t current_iteration;
                std::vector<std::string> body;
                std::string expression;
            };
            std::vector<State> stack;
        } repeat;
        struct While {
            struct State {
                std::string expression;
                std::vector<std::string> body;
                bool active;
                size_t skip_lines;
                bool is_exiting;
            };
            std::vector<State> stack;
            std::vector<size_t> iteration_counters;
        } while_loop;
        struct Defines {
            std::map<std::string, std::string> map;
        } defines;
        struct OptimizationState {
            bool branch_short = false;
            bool ops_xor = false;
            bool ops_inc = false;
            bool ops_or = false;
            bool dce = false;
            bool jump_thread = false;
            bool branch_long = false;
            bool ops_logic = false;
            bool ops_sla = false;
            bool ops_rot = false;
            bool ops_rst = false;
            bool ops_add0 = false;
        } optimization;
        std::vector<OptimizationState> optimization_stack;
        std::map<int32_t, int32_t> jump_targets;
        std::map<int32_t, int32_t> prev_jump_targets;
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
            if (!m_context.source_provider->read_file(identifier, source_data))
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
                    auto is_macro_end = [](const std::string& s) { return s == "ENDM" || s == "MEND"; };
                    bool is_end = false;
                    size_t endm_idx = 0;
                    if (tokens.count() > 0) {
                        if (is_macro_end(tokens[0].upper())) {
                            is_end = true;
                            endm_idx = 0;
                        } else if (tokens.count() > 1 && is_macro_end(tokens[1].upper())) {
                            is_end = true;
                            endm_idx = 1;
                        }
                    }
                    if (is_end) {
                        if (endm_idx == 1) {
                            if (tokens[0].original() != current_macro_name)
                                m_context.assembler.report_error("ENDM name '" + tokens[0].original() + "' does not match current macro '" + current_macro_name + "'.");
                        }
                        if (tokens.count() > endm_idx + 1)
                             m_context.assembler.report_error("Unexpected text following ENDM directive.");
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
                    if (!m_context.assembler.m_keywords.is_valid_label_name(current_macro_name))
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
    class Operands {
    public:
        Operands(IPhasePolicy& policy) : m_policy(policy) {}
        struct Operand {
            enum class Type { REG8, REG16, IMMEDIATE, MEM_IMMEDIATE, MEM_REG16, MEM_INDEXED, CONDITION, CHAR_LITERAL, STRING_LITERAL, UNKNOWN };
            Type type = Type::UNKNOWN;
            std::string str_val;
            int32_t num_val = 0;
            int16_t offset = 0;
            std::string base_reg;
        };
        Operand parse(const std::string& operand_string, const std::string& mnemonic) {
            Operand operand;
            operand.str_val = operand_string;
            std::string upper_operand_string = operand_string;
            Strings::to_upper(upper_operand_string);
            if (upper_operand_string == "(C)") {
                 operand.type = Operand::Type::MEM_REG16;
                 operand.str_val = "C";
                 return operand;
            }
            if ((mnemonic == "RET" || mnemonic == "JP" || mnemonic == "CALL" || mnemonic == "JR") && is_condition(upper_operand_string)) {
                operand.type = Operand::Type::CONDITION;
                operand.str_val = upper_operand_string;
                return operand;
            }
            if (is_reg8(upper_operand_string)) {
                operand.type = Operand::Type::REG8;
                operand.str_val = upper_operand_string;
                return operand;
            }
            if (is_reg16(upper_operand_string)) {
                operand.type = Operand::Type::REG16;
                operand.str_val = upper_operand_string;
                return operand;
            }
            if (is_condition(upper_operand_string)) {
                operand.type = Operand::Type::CONDITION;
                operand.str_val = upper_operand_string;
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
                    operand.type = Operand::Type::MEM_REG16;
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
                        if (Strings::is_number(offset_str, offset_val, m_policy.context().assembler.m_options.numbers)) {
                            // Handle (IX/IY +/- d)
                            operand.type = Operand::Type::MEM_INDEXED;
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
                    operand.type = Operand::Type::MEM_IMMEDIATE;
                    operand.num_val = inner_num_val;
                    return operand;
                }
            }
            Expressions expression(m_policy);
            typename Expressions::Value value;
            if (expression.evaluate(operand_string, value)) {
                if (value.type == Expressions::Value::Type::STRING) {
                    operand.str_val = value.s_val;
                    operand.type = Operand::Type::STRING_LITERAL;
                    if (value.s_val.length() == 1)
                        operand.num_val = (int32_t)(unsigned char)value.s_val[0];
                } else {
                    operand.num_val = (int32_t)value.n_val;
                    operand.type = Operand::Type::IMMEDIATE;
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
        enum class Type { NUMBER, STRING, TERNARY_SKIP };
            Type type = Type::NUMBER;
            double n_val = 0.0;
            std::string s_val;
        };
        struct OperatorInfo {
            int precedence;
            bool is_unary;
            bool left_assoc;
            std::function<Value(Context&, const std::vector<Value>&)> apply;
        };
        struct FunctionInfo {
            int num_args; //if negative, it's a variadic function with at least -num_args arguments.
            std::function<Value(Context&, const std::vector<Value>&)> apply;
        };
        struct Token {
            enum class Type { UNKNOWN, NUMBER, SYMBOL, OPERATOR, FUNCTION, LPAREN, RPAREN, MEM_LBRACE, MEM_RBRACE, CHAR_LITERAL, STRING_LITERAL, COMMA };
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
                if (Strings::is_number(s, out_value, m_policy.context().assembler.m_options.numbers))
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
                if (Strings::is_number(s, num_val, m_policy.context().assembler.m_options.numbers)) {
                    out_value = { Value::Type::NUMBER, (double)num_val };
                    return true;
                }
                return false;
            }
            auto tokens = tokenize_expression(s);
            auto rpn = shunting_yard(tokens);
            return evaluate_rpn(rpn, out_value);
        }
        static const std::map<std::string, OperatorInfo>& get_operator_map() {
            static const std::map<std::string, OperatorInfo> op_map = {
                // unary
                {"_",  {100, true, false, op_unary_minus}},
                {"~",  {100, true, false, op_bitwise_not}},
                {"DEFINED", {100, true, false, op_defined}},
                {"!",  {100, true, false, op_logical_not}},
                {"NOT", {100, true, false, op_bitwise_not}},
                // binary
                {"*",  {90, false, true,  op_mul}},
                {"/",  {90, false, true,  op_div}},
                {"%",  {90, false, true,  op_mod}},
                {"MOD",{90, false, true,  op_mod}},
                {"+",  {80, false, true,  op_add}},
                {"##", {75, false, true,  op_concat}},
                {"-",  {80, false, true,  op_sub}},
                {"<<", {70, false, true,  op_shl}},
                {">>", {70, false, true,  op_shr}},
                {"SHL",{70, false, true,  op_shl}},
                {"SHR",{70, false, true,  op_shr}},
                {">",  {60, false, true,  op_gt}},
                {"GT", {60, false, true,  op_gt}},
                {"<",  {60, false, true,  op_lt}},
                {"LT", {60, false, true,  op_lt}},
                {">=", {60, false, true,  op_ge}},
                {"GE", {60, false, true,  op_ge}},
                {"<=", {60, false, true,  op_le}},
                {"LE", {60, false, true,  op_le}},
                {"==", {50, false, true,  op_eq}},
                {"EQ", {50, false, true,  op_eq}},
                {"!=", {50, false, true,  op_ne}},
                {"NE", {50, false, true,  op_ne}},
                {"&",  {40, false, true,  op_and}},
                {"AND",{40, false, true,  op_and}},
                {"^",  {30, false, true,  op_xor}},
                {"XOR",{30, false, true,  op_xor}},
                {"|",  {20, false, true,  op_or}},
                {"OR", {20, false, true,  op_or}},
                {"&&", {10, false, true,  op_land}},
                {"||", {0, false, true,  op_lor}},
                {"?",  {-10, false, false, op_ternary}},
                {":",  {-20, false, false, op_colon}}
            };
            return op_map;
        }
        static const std::map<std::string, FunctionInfo>& get_function_map() {
            static const std::map<std::string, FunctionInfo> func_map = {
                {"ISSTRING", {1, func_isstring}},
                {"ISNUMBER", {1, func_isnumber}},
                {"STR", {1, func_str}},
                {"VAL", {1, func_val}},
                {"CHR", {1, func_chr}},
                {"ASC", {1, func_asc}},
                {"CHARS", {1, func_chars}},
                {"INT", {1, func_int}},
                {"STRLEN", {1, func_strlen}},
                {"SUBSTR", {3, func_substr}},
                {"STRIN", {2, func_strin}},
                {"REPLACE", {3, func_replace}},
                {"LCASE", {1, func_lcase}},
                {"UCASE", {1, func_ucase}},
                {"MEM", {1, func_mem}},
                {"FILESIZE", {1, func_filesize}},
                {"HIGH", {1, func_high}},
                {"LOW",  {1, func_low}},
                {"MIN",  {-2, func_min}},
                {"MAX",  {-2, func_max}},
                {"SIN",   {1, func_sin}},
                {"COS",   {1, func_cos}},
                {"TAN",   {1, func_tan}},
                {"ASIN",  {1, func_asin}},
                {"ACOS",  {1, func_acos}},
                {"ATAN",  {1, func_atan}},
                {"ATAN2", {2, func_atan2}},
                {"SINH",  {1, func_sinh}},
                {"COSH",  {1, func_cosh}},
                {"TANH",  {1, func_tanh}},
                {"ASINH", {1, func_asinh}},
                {"ACOSH", {1, func_acosh}},
                {"ATANH", {1, func_atanh}},
                {"ABS",   {1, func_abs}},
                {"POW",   {2, func_pow}},
                {"HYPOT", {2, func_hypot}},
                {"FMOD",  {2, func_fmod}},
                {"SQRT",  {1, func_sqrt}},
                {"LOG",   {1, func_log}},
                {"LOG10", {1, func_log10}},
                {"LOG2",  {1, func_log2}},
                {"EXP",   {1, func_exp}},
                {"RAND",  {2, func_rand}},
                {"RND",   {0, func_rnd}},
                {"RRND",  {2, func_rrnd}},
                {"FLOOR", {1, func_floor}},
                {"CEIL",  {1, func_ceil}},
                {"ROUND", {1, func_round}},
                {"TRUNC", {1, func_trunc}},
                {"SGN",   {1, func_sgn}}
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
    private:
        static double get_numeric_value(Context& ctx, const Value& v, const std::string& error_context = "") {
            if (v.type == Value::Type::NUMBER)
                return v.n_val;
            if (v.type == Value::Type::STRING && v.s_val.length() == 1)
                return (double)(unsigned char)v.s_val[0];
            std::string msg = "Type Mismatch";
            if (!error_context.empty())
                msg += " in " + error_context;
            msg += ": Expected number or single-character string.";
            ctx.assembler.report_error(msg);
            return 0.0;
        }
        static Value op_unary_minus(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, -get_numeric_value(ctx, args[0], "unary -")};
        }
        static Value op_bitwise_not(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)(~(int32_t)get_numeric_value(ctx, args[0], "bitwise NOT"))};
        }
        static Value op_logical_not(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)(!get_numeric_value(ctx, args[0], "logical NOT"))};
        }
        static Value op_defined(Context& ctx, const std::vector<Value>& args) {
             if (args[0].type != Value::Type::STRING)
                ctx.assembler.report_error("Argument to DEFINED must be a symbol name.");
            const std::string& symbol_name = args[0].s_val;
            int32_t dummy;
            if (ctx.defines.map.count(symbol_name) || (ctx.current_phase && ctx.current_phase->on_symbol_resolve(symbol_name, dummy)))
                return Value{Value::Type::NUMBER, 1.0};
            return Value{Value::Type::NUMBER, 0.0};
        }
        static Value op_mul(Context& ctx, const std::vector<Value>& args) {
            double v2 = get_numeric_value(ctx, args[1], "*");
            return Value{Value::Type::NUMBER, get_numeric_value(ctx, args[0], "*") * v2};
        }
        static Value op_div(Context& ctx, const std::vector<Value>& args) {
            double v2 = get_numeric_value(ctx, args[1], "/");
            if (std::abs(v2) < 1e-12) throw std::runtime_error("Division by zero.");
            return Value{Value::Type::NUMBER, get_numeric_value(ctx, args[0], "/") / v2};
        }
        static Value op_mod(Context& ctx, const std::vector<Value>& args) {
            int32_t v2 = (int32_t)get_numeric_value(ctx, args[1], "%");
            if (v2 == 0) throw std::runtime_error("Division by zero.");
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "%") % v2)};
        }
        static Value op_add(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, get_numeric_value(ctx, args[0], "+") + get_numeric_value(ctx, args[1], "+")};
        }
        static Value op_sub(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, get_numeric_value(ctx, args[0], "-") - get_numeric_value(ctx, args[1], "-")};
        }
        static Value op_concat(Context& ctx, const std::vector<Value>& args) {
            auto to_str = [](const Value& v) {
                return (v.type == Value::Type::STRING) ? v.s_val : std::to_string((int32_t)v.n_val);
            };
            return Value{Value::Type::STRING, 0.0, to_str(args[0]) + to_str(args[1])};
        }
        static Value op_shl(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "<<") << (int32_t)get_numeric_value(ctx, args[1], "<<"))};
        }
        static Value op_shr(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], ">>") >> (int32_t)get_numeric_value(ctx, args[1], ">>"))};
        }
        static Value op_gt(Context& ctx, const std::vector<Value>& args) { 
            if (args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING)
                return Value{Value::Type::NUMBER, (double)(args[0].s_val > args[1].s_val)};
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], ">") > get_numeric_value(ctx, args[1], ">"))}; 
        }
        static Value op_lt(Context& ctx, const std::vector<Value>& args) { 
            if (args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING)
                return Value{Value::Type::NUMBER, (double)(args[0].s_val < args[1].s_val)};
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], "<") < get_numeric_value(ctx, args[1], "<"))}; 
        }
        static Value op_ge(Context& ctx, const std::vector<Value>& args) { 
            if (args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING)
                return Value{Value::Type::NUMBER, (double)(args[0].s_val >= args[1].s_val)};
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], ">=") >= get_numeric_value(ctx, args[1], ">="))}; 
        }
        static Value op_le(Context& ctx, const std::vector<Value>& args) { 
            if (args[0].type == Value::Type::STRING && args[1].type == Value::Type::STRING)
                return Value{Value::Type::NUMBER, (double)(args[0].s_val <= args[1].s_val)};
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], "<=") <= get_numeric_value(ctx, args[1], "<="))}; 
        }
        static Value op_eq(Context& ctx, const std::vector<Value>& args) {
            if (args[0].type == args[1].type) {
                if (args[0].type == Value::Type::STRING)
                    return Value{Value::Type::NUMBER, (double)(args[0].s_val == args[1].s_val)};
                return Value{Value::Type::NUMBER, (double)(args[0].n_val == args[1].n_val)};
            }
            auto try_get_val = [](const Value& v) -> std::optional<double> {
                if (v.type == Value::Type::NUMBER)
                    return v.n_val;
                if (v.type == Value::Type::STRING && v.s_val.length() == 1)
                    return (double)(unsigned char)v.s_val[0];
                return std::nullopt;
            };
            auto v1 = try_get_val(args[0]);
            auto v2 = try_get_val(args[1]);
            if (v1 && v2)
                return Value{Value::Type::NUMBER, (double)(*v1 == *v2)};
            return Value{Value::Type::NUMBER, 0.0};
        }
        static Value op_ne(Context& ctx, const std::vector<Value>& args) {
            if (args[0].type == args[1].type) {
                if (args[0].type == Value::Type::STRING)
                    return Value{Value::Type::NUMBER, (double)(args[0].s_val != args[1].s_val)};
                return Value{Value::Type::NUMBER, (double)(args[0].n_val != args[1].n_val)};
            }
            auto try_get_val = [](const Value& v) -> std::optional<double> {
                if (v.type == Value::Type::NUMBER)
                    return v.n_val;
                if (v.type == Value::Type::STRING && v.s_val.length() == 1)
                    return (double)(unsigned char)v.s_val[0];
                return std::nullopt;
            };
            auto v1 = try_get_val(args[0]);
            auto v2 = try_get_val(args[1]);
            if (v1 && v2)
                return Value{Value::Type::NUMBER, (double)(*v1 != *v2)};
            return Value{Value::Type::NUMBER, 1.0};
        }
        static Value op_and(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "&") & (int32_t)get_numeric_value(ctx, args[1], "&"))};
        }
        static Value op_xor(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "^") ^ (int32_t)get_numeric_value(ctx, args[1], "^"))};
        }
        static Value op_or(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "|") | (int32_t)get_numeric_value(ctx, args[1], "|"))};
        }
        static Value op_land(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], "&&") && get_numeric_value(ctx, args[1], "&&"))};
        }
        static Value op_lor(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)(get_numeric_value(ctx, args[0], "||") || get_numeric_value(ctx, args[1], "||"))};
        }
        static Value op_ternary(Context& ctx, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::NUMBER)
                ctx.assembler.report_error("Ternary condition must be a number.");
            if (args[0].n_val != 0)
                return args[1];
            return Value{Value::Type::TERNARY_SKIP};
        }
        static Value op_colon(Context& ctx, const std::vector<Value>& args) {
            return (args[0].type == Value::Type::TERNARY_SKIP) ? args[1] : args[0];
        }
        static Value func_isstring(Context& context, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (args[0].type == Value::Type::STRING) ? 1.0 : 0.0};
        }
        static Value func_isnumber(Context& context, const std::vector<Value>& args) {
            if (args[0].type == Value::Type::NUMBER)
                return Value{Value::Type::NUMBER, 1.0};
            if (args[0].type == Value::Type::STRING) {
                int32_t dummy;
                if (Strings::is_number(args[0].s_val, dummy, context.assembler.m_options.numbers))
                    return Value{Value::Type::NUMBER, 1.0};
            }
            return Value{Value::Type::NUMBER, 0.0};
        }
        static Value func_str(Context& context, const std::vector<Value>& args) {
            return Value{Value::Type::STRING, 0.0, std::to_string((int32_t)get_numeric_value(context, args[0], "STR"))};
        }
        static Value func_val(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to VAL must be a string.");
            int32_t num_val;
            if (Strings::is_number(args[0].s_val, num_val, context.assembler.m_options.numbers))
                return Value{Value::Type::NUMBER, (double)num_val};
            context.assembler.report_error("VAL argument is not a valid number: \"" + args[0].s_val + "\"");
            return Value{Value::Type::NUMBER, 0.0};
        }
        static Value func_chr(Context& context, const std::vector<Value>& args) {
            char c = (char)((int32_t)get_numeric_value(context, args[0], "CHR"));
            return Value{Value::Type::STRING, 0.0, std::string(1, c)};
        }
        static Value func_asc(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to ASC must be a string.");
            if (args[0].s_val.empty())
                context.assembler.report_error("ASC argument cannot be an empty string.");
            return Value{Value::Type::NUMBER, (double)(unsigned char)args[0].s_val[0]};
        }
        static Value func_chars(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to CHARS must be a string.");
            const std::string& s = args[0].s_val;
            if (s.length() > 4)
                context.assembler.report_error("CHARS argument string cannot be longer than 4 bytes.");
            uint32_t val = 0;
            for (size_t i = 0; i < s.length(); ++i)
                val |= ((uint32_t)(unsigned char)s[i]) << (i * 8);
            return Value{Value::Type::NUMBER, (double)val};
        }
        static Value func_int(Context& context, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(context, args[0], "INT"))};
        }
        static Value func_strlen(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to STRLEN must be a string.");
            return Value{Value::Type::NUMBER, (double)args[0].s_val.length()};
        }
        static Value func_substr(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("SUBSTR: First argument must be a string.");
            const std::string& str = args[0].s_val;
            int32_t pos_val = (int32_t)get_numeric_value(context, args[1], "SUBSTR");
            int32_t len_val = (int32_t)get_numeric_value(context, args[2], "SUBSTR");
            if (pos_val < 0 || len_val < 0)
                context.assembler.report_error("SUBSTR: Position and length cannot be negative.");
            size_t pos = pos_val;
            size_t len = len_val;
            return Value{Value::Type::STRING, 0.0, str.substr(pos, len)};
        }
        static Value func_strin(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("STRIN: First argument must be a string.");
            if (args[1].type != Value::Type::STRING)
                context.assembler.report_error("STRIN: Second argument must be a string.");
            const std::string& str = args[0].s_val;
            const std::string& sub = args[1].s_val;
            size_t pos = str.find(sub);
            if (pos == std::string::npos)
                return Value{Value::Type::NUMBER, 0.0};
            return Value{Value::Type::NUMBER, (double)(pos + 1)};
        }
        static Value func_replace(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("REPLACE: First argument must be a string.");
            if (args[1].type != Value::Type::STRING)
                context.assembler.report_error("REPLACE: Second argument must be a string.");
            if (args[2].type != Value::Type::STRING)
                context.assembler.report_error("REPLACE: Third argument must be a string.");
            std::string s = args[0].s_val;
            const std::string& old_str = args[1].s_val;
            const std::string& new_str = args[2].s_val;
            if (old_str.empty())
                return Value{Value::Type::STRING, 0.0, s};
            size_t start_pos = 0;
            while((start_pos = s.find(old_str, start_pos)) != std::string::npos) {
                s.replace(start_pos, old_str.length(), new_str);
                start_pos += new_str.length();
            }
            return Value{Value::Type::STRING, 0.0, s};
        }
        static Value func_lcase(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to LCASE must be a string.");
            std::string s = args[0].s_val;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return Value{Value::Type::STRING, 0.0, s};
        }
        static Value func_ucase(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to UCASE must be a string.");
            std::string s = args[0].s_val;
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return Value{Value::Type::STRING, 0.0, s};
        }
        static Value func_mem(Context& context, const std::vector<Value>& args) {
            uint16_t addr = (uint16_t)((int32_t)get_numeric_value(context, args[0], "MEM"));
            return Value{Value::Type::NUMBER, (double)context.memory->peek(addr)};
        }
        static Value func_filesize(Context& context, const std::vector<Value>& args) {
            if (args[0].type != Value::Type::STRING)
                context.assembler.report_error("Argument to FILESIZE must be a string.");
            const std::string& filename = args[0].s_val;
            if (!context.source_provider->exists(filename))
                context.assembler.report_error("File not found for FILESIZE: " + filename);
            return Value{Value::Type::NUMBER, (double)context.source_provider->file_size(filename)};
        }
        static Value func_high(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)(((int32_t)get_numeric_value(ctx, args[0], "HIGH") >> 8) & 0xFF)};
        }
        static Value func_low(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, (double)((int32_t)get_numeric_value(ctx, args[0], "LOW") & 0xFF)};
        }
        static Value func_min(Context& ctx, const std::vector<Value>& args) {
            if (args.size() < 2)
                throw std::runtime_error("MIN requires at least two arguments.");
            double result = get_numeric_value(ctx, args[0], "MIN");
            for (size_t i = 1; i < args.size(); ++i)
                result = std::min(result, get_numeric_value(ctx, args[i], "MIN"));
            return Value{Value::Type::NUMBER, result};
        }
        static Value func_max(Context& ctx, const std::vector<Value>& args) {
            if (args.size() < 2)
                throw std::runtime_error("MAX requires at least two arguments.");
            double result = get_numeric_value(ctx, args[0], "MAX");
            for (size_t i = 1; i < args.size(); ++i)
                result = std::max(result, get_numeric_value(ctx, args[i], "MAX"));
            return Value{Value::Type::NUMBER, result};
        }
        static Value func_sin(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, sin(get_numeric_value(ctx, args[0], "SIN"))};
        }
        static Value func_cos(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, cos(get_numeric_value(ctx, args[0], "COS"))};
        }
        static Value func_tan(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, tan(get_numeric_value(ctx, args[0], "TAN"))};
        }
        static Value func_asin(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, asin(get_numeric_value(ctx, args[0], "ASIN"))};
        }
        static Value func_acos(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, acos(get_numeric_value(ctx, args[0], "ACOS"))};
        }
        static Value func_atan(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, atan(get_numeric_value(ctx, args[0], "ATAN"))};
        }
        static Value func_atan2(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, atan2(get_numeric_value(ctx, args[0], "ATAN2"), get_numeric_value(ctx, args[1], "ATAN2"))};
        }
        static Value func_sinh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, sinh(get_numeric_value(ctx, args[0], "SINH"))};
        }
        static Value func_cosh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, cosh(get_numeric_value(ctx, args[0], "COSH"))};
        }
        static Value func_tanh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, tanh(get_numeric_value(ctx, args[0], "TANH"))};
        }
        static Value func_asinh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, asinh(get_numeric_value(ctx, args[0], "ASINH"))};
        }
        static Value func_acosh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, acosh(get_numeric_value(ctx, args[0], "ACOSH"))};
        }
        static Value func_atanh(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, atanh(get_numeric_value(ctx, args[0], "ATANH"))};
        }
        static Value func_abs(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, fabs(get_numeric_value(ctx, args[0], "ABS"))};
        }
        static Value func_pow(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, pow(get_numeric_value(ctx, args[0], "POW"), get_numeric_value(ctx, args[1], "POW"))};
        }
        static Value func_hypot(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, hypot(get_numeric_value(ctx, args[0], "HYPOT"), get_numeric_value(ctx, args[1], "HYPOT"))};
        }
        static Value func_fmod(Context& ctx, const std::vector<Value>& args) {
            double v2 = get_numeric_value(ctx, args[1], "FMOD");
            if (std::abs(v2) < 1e-12)
                throw std::runtime_error("FMOD by zero.");
            return Value{Value::Type::NUMBER, fmod(get_numeric_value(ctx, args[0], "FMOD"), v2)};
        }
        static Value func_sqrt(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, sqrt(get_numeric_value(ctx, args[0], "SQRT"))};
        }
        static Value func_log(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, log(get_numeric_value(ctx, args[0], "LOG"))}; }
        static Value func_log10(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, log10(get_numeric_value(ctx, args[0], "LOG10"))};
        }
        static Value func_log2(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, log2(get_numeric_value(ctx, args[0], "LOG2"))};
        }
        static Value func_exp(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, exp(get_numeric_value(ctx, args[0], "EXP"))};
        }
        static Value func_rand(Context& ctx, const std::vector<Value>& args) {
            static std::mt19937 gen(0);
            std::uniform_int_distribution<> distrib((int)get_numeric_value(ctx, args[0], "RAND"), (int)get_numeric_value(ctx, args[1], "RAND"));
            return Value{Value::Type::NUMBER, (double)distrib(gen)};
        }
        static Value func_rnd(Context&, const std::vector<Value>& args) {
            static std::mt19937 gen(1);
            std::uniform_real_distribution<> distrib(0.0, 1.0);
            return Value{Value::Type::NUMBER, distrib(gen)};
        }
        static Value func_rrnd(Context& ctx, const std::vector<Value>& args) {
            static std::mt19937 gen(0);
            std::uniform_int_distribution<> distrib((int)get_numeric_value(ctx, args[0], "RRND"), (int)get_numeric_value(ctx, args[1], "RRND"));
            return Value{Value::Type::NUMBER, (double)distrib(gen)};
        }
        static Value func_floor(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, floor(get_numeric_value(ctx, args[0], "FLOOR"))};
        }
        static Value func_ceil(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, ceil(get_numeric_value(ctx, args[0], "CEIL"))};
        }
        static Value func_round(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, round(get_numeric_value(ctx, args[0], "ROUND"))};
        }
        static Value func_trunc(Context& ctx, const std::vector<Value>& args) {
            return Value{Value::Type::NUMBER, trunc(get_numeric_value(ctx, args[0], "TRUNC"))};
        }
        static Value func_sgn(Context& ctx, const std::vector<Value>& args) {
            double val = get_numeric_value(ctx, args[0], "SGN");
            return Value{Value::Type::NUMBER, (double)((val > 0) - (val < 0))};
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
            if (!isalpha(expr[i]) && expr[i] != '_' && expr[i] != '@' && expr[i] != '$' && expr[i] != '?' && !(expr[i] == '.' && i + 1 < expr.length() && (isalpha(expr[i+1]) || expr[i+1] == '_')))
                return false;
            size_t j = i;
            if (expr[j] == '$' && j + 1 < expr.length() && isalpha(expr[j+1])) j++; // $PASS
            while (j < expr.length() && (isalnum(expr[j]) || expr[j] == '_' || expr[j] == '.' || expr[j] == '@' || expr[j] == '$' || expr[j] == '?')) {
                if (expr[j] == '.' && j == i && (j + 1 >= expr.length() || !isalnum(expr[j+1])))
                    break;
                j++;
            }
            std::string symbol_str = expr.substr(i, j - i);
            std::string upper_symbol = symbol_str;
            Strings::to_upper(upper_symbol);
            auto builtin_const_it = get_constant_map().find(upper_symbol);
            auto custom_const_it = m_policy.context().assembler.custom_constants.find(upper_symbol);
            if (builtin_const_it != get_constant_map().end())
                tokens.push_back({Token::Type::NUMBER, "", builtin_const_it->second});
            else if (custom_const_it != m_policy.context().assembler.custom_constants.end())
                tokens.push_back({Token::Type::NUMBER, "", custom_const_it->second});
            else if (get_function_map().count(upper_symbol) || m_policy.context().assembler.custom_functions.count(upper_symbol)) {
                size_t next_char_idx = j;
                while (next_char_idx < expr.length() && isspace(expr[next_char_idx]))
                    next_char_idx++;
                if (next_char_idx < expr.length() && expr[next_char_idx] == '(')
                    tokens.push_back({Token::Type::FUNCTION, upper_symbol, 0.0, 12, false});
                else
                    tokens.push_back({Token::Type::SYMBOL, symbol_str});
            } else {
                const OperatorInfo* op_info = find_operator(upper_symbol);
                if (op_info)
                    tokens.push_back({Token::Type::OPERATOR, upper_symbol, 0, op_info->precedence, op_info->left_assoc, op_info});
                else
                    tokens.push_back({Token::Type::SYMBOL, symbol_str});
            }
            i = j - 1;
            return true;
        }
        bool parse_number(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            const auto& options = m_policy.context().assembler.m_options.numbers;
            if (expr[i] == '$') {
                if (!options.allow_hex_prefix_dollar) return false;
                size_t j = i + 1;
                if (j < expr.length() && isxdigit(expr[j])) {
                    while (j < expr.length() && isxdigit(expr[j]))
                        j++;
                    std::string num_str = expr.substr(i + 1, j - i - 1);
                    int32_t val;
                    auto result = std::from_chars(num_str.data(), num_str.data() + num_str.size(), val, 16);
                    if (result.ec == std::errc()) {
                        tokens.push_back({Token::Type::NUMBER, "", (double)val});
                        i = j - 1;
                        return true;
                    }
                }
            }
            if (expr[i] == '%') {
                if (!options.allow_bin_prefix_percent) return false;
                size_t j = i + 1;
                if (j < expr.length() && (expr[j] == '0' || expr[j] == '1')) {
                    while (j < expr.length() && (expr[j] == '0' || expr[j] == '1'))
                        j++;
                    std::string num_str = expr.substr(i + 1, j - i - 1);
                    int32_t val;
                    auto result = std::from_chars(num_str.data(), num_str.data() + num_str.size(), val, 2);
                    if (result.ec == std::errc()) {
                        tokens.push_back({Token::Type::NUMBER, "", (double)val});
                        i = j - 1;
                        return true;
                    }
                }
            }
            if (isdigit(expr[i]) || (expr[i] == '.' && i + 1 < expr.length() && isdigit(expr[i + 1]))) {
                size_t j = i;
                bool has_dot = false;
                while (j < expr.length() && (isdigit(expr[j]) || (!has_dot && expr[j] == '.'))) {
                    if (expr[j] == '.')
                        has_dot = true;
                    j++;
                }
                if (!has_dot) {
                    j = i;
                    if (options.allow_hex_prefix_0x && (expr.substr(i, 2) == "0x" || expr.substr(i, 2) == "0X"))
                        j += 2;
                    while (j < expr.length() && isalnum(expr[j]))
                        j++;
                    if (j < expr.length() && (expr[j] == 'h' || expr[j] == 'H' || expr[j] == 'b' || expr[j] == 'B')) {
                        char last_char = toupper(expr[j - 1]);
                        if (last_char != 'B' && last_char != 'H')
                            j++;
                    }
                    int32_t val;
                    if (Strings::is_number(expr.substr(i, j - i), val, options)) {
                        tokens.push_back({Token::Type::NUMBER, "", (double)(val)});
                        i = j - 1;
                        return true;
                    }
                    j = i;
                    while (j < expr.length() && isdigit(expr[j]))
                        j++;
                }
                std::string num_str = expr.substr(i, j - i);
                size_t processed = 0;
                double d_val = 0.0;
                bool stod_success = false;
                try {
                    d_val = std::stod(num_str, &processed);
                    stod_success = true;
                } catch (...) {}
                if (stod_success && processed > 0) {
                    tokens.push_back({Token::Type::NUMBER, "", d_val});
                    i = i + processed - 1;
                    return true;
                }
                m_policy.context().assembler.report_error("Invalid number in expression: " + num_str);
                i = j - 1;
                return true;
            }
            return false;
        }
        bool parse_ternary_operator(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            if (expr[i] == '?') {
                bool is_operator = true;
                if (!tokens.empty()) {
                    const auto& last_token = tokens.back();
                    if (last_token.type == Token::Type::SYMBOL)
                        is_operator = false;
                }
                return is_operator && parse_operator(expr, i, tokens);
            }
            return false;
        }
        bool parse_operator(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            std::string op_str;
            for (size_t len = m_policy.context().assembler.max_operator_len; len > 0; --len) {
                if (i + len <= expr.length()) {
                    std::string potential_op = expr.substr(i, len);
                    if (find_operator(potential_op)) {
                        op_str = potential_op;
                        break;
                    }
                }
            }
            if (op_str.empty()) return false; // No operator found
            bool is_unary = (tokens.empty() || tokens.back().type == Token::Type::OPERATOR || tokens.back().type == Token::Type::LPAREN);
            std::string op_key = op_str;
            if (is_unary && (op_str == "-" || op_str == "~" || op_str == "!"))
                op_key = (op_str == "-") ? "_" : op_str;
            else if (is_unary && op_str == "+") {
                i += op_str.length() - 1;
                return true;
            }
            const OperatorInfo* op_info = find_operator(op_key);
            if (!op_info)
                return false;
            tokens.push_back({Token::Type::OPERATOR, op_key, 0.0, op_info->precedence, op_info->left_assoc, op_info});
            i += op_str.length() - 1;
            return true;
        }
        bool parse_parens(const std::string& expr, size_t& i, std::vector<Token>& tokens) const {
            if (expr[i] == '(') {
                tokens.push_back({Token::Type::LPAREN, "("});
                return true;
            } else if (expr[i] == ')') {
                tokens.push_back({Token::Type::RPAREN, ")"});
                return true;
            } else if (expr[i] == '{') {
                tokens.push_back({Token::Type::MEM_LBRACE, "{"});
                return true;
            } else if (expr[i] == '}') {
                tokens.push_back({Token::Type::MEM_RBRACE, "}"});
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
                if (parse_ternary_operator(expr, i, tokens))
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
                    case Token::Type::MEM_LBRACE:
                        op_stack.push_back(token);
                        break;
                    case Token::Type::MEM_RBRACE:
                        while (!op_stack.empty() && op_stack.back().type != Token::Type::MEM_LBRACE) {
                            postfix.push_back(op_stack.back());
                            op_stack.pop_back();
                        }
                        if (op_stack.empty())
                            m_policy.context().assembler.report_error("Mismatched braces {} in expression.");
                        op_stack.pop_back();
                        postfix.push_back({Token::Type::OPERATOR, "{}"});
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
                    m_policy.context().assembler.report_error("Mismatched parentheses or braces in expression.");
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
                    const FunctionInfo* func_info_ptr = nullptr;
                    auto builtin_it = get_function_map().find(token.s_val);
                    if (builtin_it != get_function_map().end())
                        func_info_ptr = &builtin_it->second;
                    else {
                        auto custom_it = m_policy.context().assembler.custom_functions.find(token.s_val);
                        if (custom_it != m_policy.context().assembler.custom_functions.end())
                            func_info_ptr = &custom_it->second;
                    }
                    if (!func_info_ptr)
                        m_policy.context().assembler.report_error("Unknown function in RPN evaluation: " + token.s_val);
                    const auto& func_info = *func_info_ptr;
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
                    val_stack.push_back(func_info.apply(m_policy.context(), args));
                } else if (token.type == Token::Type::OPERATOR) {
                    if (token.s_val == "{}") {
                        if (val_stack.empty())
                            m_policy.context().assembler.report_error("Invalid memory access expression {}.");
                        Value addr_val = val_stack.back();
                        val_stack.pop_back();
                        val_stack.push_back({Value::Type::NUMBER, (double)m_policy.context().memory->peek((uint16_t)addr_val.n_val)});
                        continue;
                    }
                    const OperatorInfo* op_info_ptr = find_operator(token.s_val);
                    if (!op_info_ptr)
                        m_policy.context().assembler.report_error("Unknown operator in RPN evaluation: " + token.s_val);
                    const auto& op_info = *op_info_ptr;
                    if (op_info.is_unary) {
                        if (val_stack.size() < 1)
                            m_policy.context().assembler.report_error("Invalid expression syntax for unary operator.");
                        Value v1 = val_stack.back();
                        val_stack.pop_back();
                        val_stack.push_back(op_info.apply(m_policy.context(), {v1}));
                        continue;
                    }
                    if (val_stack.size() < 2)
                        m_policy.context().assembler.report_error("Invalid expression syntax for binary operator.");
                    Value v2 = val_stack.back(); 
                    val_stack.pop_back();
                    Value v1 = val_stack.back();
                    val_stack.pop_back();
                    val_stack.push_back(op_info.apply(m_policy.context(), {v1, v2}));
                } else if (token.type == Token::Type::OPERATOR) {
                    if (token.s_val == "{}") {
                        if (val_stack.empty())
                            m_policy.context().assembler.report_error("Invalid memory access expression {}.");
                        Value addr_val = val_stack.back();
                        val_stack.pop_back();
                        val_stack.push_back({Value::Type::NUMBER, (double)m_policy.context().memory->peek((uint16_t)addr_val.n_val)});
                        continue;
                    }
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
                if (result_val.s_val.length() == 1) {
                    out_value = (int32_t)(unsigned char)result_val.s_val[0];
                    return true;
                }
                m_policy.context().assembler.report_error("Expression resulted in a string, but a numeric value was expected.");
                return false;
            }
            out_value = (int32_t)result_val.n_val;
            return true;
        }
        const OperatorInfo* find_operator(const std::string& op_str) const {
            auto it = get_operator_map().find(op_str);
            if (it != get_operator_map().end()) {
                return &it->second;
            }
            auto custom_it = m_policy.context().assembler.custom_operators.find(op_str);
            if (custom_it != m_policy.context().assembler.custom_operators.end()) {
                return &custom_it->second;
            }
            return nullptr;
        }
        IPhasePolicy& m_policy;
    };
    void add_custom_operator(const std::string& op_string, const typename Expressions::OperatorInfo& op_info) {
        m_context.assembler.custom_operators[op_string] = op_info;
        custom_operators[op_string] = op_info;
        if (op_string.length() > max_operator_len)
            max_operator_len = op_string.length();
    }
    void add_custom_function(const std::string& func_name, const typename Expressions::FunctionInfo& func_info) {
        std::string upper_name = func_name;
        Strings::to_upper(upper_name);
        if (Expressions::get_function_map().count(upper_name))
            m_context.assembler.report_error("Cannot override built-in function: " + func_name);
        custom_functions[upper_name] = func_info;
    }
    void add_custom_constant(const std::string& const_name, double value) {
        std::string upper_name = const_name;
        Strings::to_upper(upper_name);
        if (Expressions::get_constant_map().count(upper_name))
            m_context.assembler.report_error("Cannot override built-in constant: " + const_name);
        custom_constants[upper_name] = value;
    }
    class IPhasePolicy {
    public:
        using Operand = typename Operands::Operand;
        using OperandType = typename Operands::Operand::Type;

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
        virtual void on_proc_end(const std::string& name) = 0;
        virtual void on_local_directive(const std::vector<std::string>& symbols) = 0;
        virtual void on_optimize_directive(const std::vector<std::string>& args) = 0;
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) = 0;
        virtual void on_if_directive(const std::string& expression) = 0;
        virtual void on_ifdef_directive(const std::string& symbol) = 0;
        virtual void on_ifexist_directive(const std::string& filename) = 0;
        virtual void on_ifndef_directive(const std::string& symbol) = 0;
        virtual void on_ifnb_directive(const std::string& arg) = 0;
        virtual void on_ifidn_directive(const std::string& arg1, const std::string& arg2) = 0;
        virtual void on_else_directive() = 0;
        virtual void on_define_directive(const std::string& key, const std::string& value) = 0;
        virtual void on_display_directive(const std::vector<typename Strings::Tokens::Token>& tokens) = 0;
        virtual void on_endif_directive() = 0;
        virtual void on_error_directive(const std::string& message) = 0;
        virtual void on_assert_directive(const std::string& expression) = 0;
        virtual void on_while_directive(const std::string& expression) = 0;
        virtual void on_endw_directive() = 0;
        virtual void on_exitw_directive() = 0;
        virtual void on_exitr_directive() = 0;
        virtual void on_break_directive() = 0;
        virtual bool on_while_recording(const std::string& line) = 0;
        virtual void on_rept_directive(const std::string& expression) = 0;
        virtual bool on_repeat_recording(const std::string& line) = 0;
        virtual void on_endr_directive() = 0;
        virtual void on_macro(const std::string& name, const std::vector<std::string>& parameters) = 0;
        virtual void on_undefine_directive(const std::string& key) = 0;
        virtual void on_macro_line() = 0;
        virtual void on_unknown_operand(const std::string& operand) = 0;
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) = 0;
        virtual void on_source_line_begin() = 0;
        virtual void on_source_line_end() = 0;
        virtual void on_assemble(std::vector<uint8_t> bytes) = 0;
    };
    void add_custom_directive(const std::string& name, std::function<void(IPhasePolicy&, const std::vector<typename Strings::Tokens::Token>&)> func) {
        std::string upper_name = name;
        Strings::to_upper(upper_name);
        if (m_keywords.is_directive(upper_name))
            m_context.assembler.report_error("Cannot override built-in directive: " + name);
        custom_directives[upper_name] = func;
    }
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
            m_context.while_loop.stack.clear();
            m_context.defines.map.clear();
            m_context.optimization = {};
        }
        virtual void on_finalize() override {
            if (m_context.macros.in_expansion)
                m_context.assembler.report_error("Unterminated macro expansion at end of file.");
            if (!m_context.source.control_stack.empty()) {
                switch (m_context.source.control_stack.back()) {
                    case Context::Source::ControlType::CONDITIONAL:
                        m_context.assembler.report_error("Unterminated conditional compilation block (missing ENDIF).");
                    case Context::Source::ControlType::REPEAT:
                        m_context.assembler.report_error("Unterminated REPT block (missing ENDR).");
                    case Context::Source::ControlType::WHILE:
                        m_context.assembler.report_error("Unterminated WHILE block (missing ENDW).");
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
            m_context.optimization = {};
            m_context.optimization_stack.clear();
            m_context.prev_jump_targets = std::move(m_context.jump_targets);
            m_context.jump_targets.clear();
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
            if (symbol == "$PHASE") {
                out_value = this->m_context.phase_index;
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
            if (this->m_context.source_provider->read_file(filename, data))
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
        virtual void on_proc_end(const std::string& name) override {
            if (m_context.symbols.scope_stack.empty())
                m_context.assembler.report_error("ENDP without PROC.");
            if (!name.empty()) {
                if (get_absolute_symbol_name(name) != get_absolute_symbol_name(m_context.symbols.scope_stack.back().full_name))
                    m_context.assembler.report_error("ENDP name '" + name + "' does not match current procedure '" + m_context.symbols.scope_stack.back().full_name + "'.");
            }
            m_context.symbols.scope_stack.pop_back();
        }
        virtual void on_local_directive(const std::vector<std::string>& symbols) override {
            if (m_context.symbols.scope_stack.empty())
                m_context.assembler.report_error("LOCAL directive used outside of a PROC block.");
            auto& current_scope = m_context.symbols.scope_stack.back();
            for (const auto& symbol : symbols) {
            if (!m_context.assembler.m_keywords.is_valid_label_name(symbol) || symbol.find('.') != std::string::npos)
                    m_context.assembler.report_error("Invalid symbol name in LOCAL directive: '" + symbol + "'");
                current_scope.local_symbols.insert(symbol);
            }
        }
        virtual void on_optimize_directive(const std::vector<std::string>& args) override {
            auto& ctx = m_context;
            bool enable_opt = ctx.assembler.m_options.compilation.enable_optimization;
            for (const auto& arg : args) {
                std::string upper_arg = arg;
                Strings::to_upper(upper_arg);
                if ((upper_arg == "PUSH" || upper_arg == "POP") && args.size() > 1)
                    ctx.assembler.report_error("OPTIMIZE PUSH/POP cannot be mixed with other arguments.");
            }
            for (const auto& arg : args) {
                std::string upper_arg = arg;
                Strings::to_upper(upper_arg);
                if (upper_arg == "PUSH")
                    ctx.optimization_stack.push_back(ctx.optimization);
                else if (upper_arg == "POP") {
                    if (!ctx.optimization_stack.empty()) {
                        ctx.optimization = ctx.optimization_stack.back();
                        ctx.optimization_stack.pop_back();
                    } else
                        ctx.assembler.report_error("OPTIMIZE POP without matching PUSH");
                } else {
                    bool enable = true;
                    std::string flag = upper_arg;
                    if (flag.front() == '+')
                        flag = flag.substr(1);
                    else if (flag.front() == '-') {
                        enable = false;
                        flag = flag.substr(1);
                    }
                    if (flag == "NONE") {
                         ctx.optimization = {};
                         continue;
                    }
                    bool is_valid = false;
                    if (enable_opt) {
                        if (flag == "BRANCH_SHORT")
                            is_valid = true;
                        else if (flag == "JUMP_THREAD")
                            is_valid = true;
                        else if (flag == "DCE")
                            is_valid = true;
                        else if (flag == "OPS_XOR")
                            is_valid = true;
                        else if (flag == "OPS_INC")
                            is_valid = true;
                        else if (flag == "OPS_OR")
                            is_valid = true;
                        else if (flag == "OPS_LOGIC")
                            is_valid = true;
                        else if (flag == "OPS_SLA")
                            is_valid = true;
                        else if (flag == "OPS_ROT")
                            is_valid = true;
                        else if (flag == "OPS_RST")
                        is_valid = true;
                        else if (flag == "OPS_ADD0")
                        is_valid = true;
                        else if (flag == "BRANCH_LONG")
                            is_valid = true;
                        else if (flag == "OPS") {
                            is_valid = true;
                        }
                        else if (flag == "UNSAFE") {
                            is_valid = true;
                        }
                        else if (flag == "SPEED") {
                            is_valid = true;
                        }
                        else if (flag == "SIZE" || flag == "ALL") {
                            is_valid = true;
                        }
                    }
                    if (!is_valid)
                        ctx.assembler.report_error("Invalid parameter: " + arg);
                    if (flag == "BRANCH_SHORT")
                        ctx.optimization.branch_short = enable;
                    else if (flag == "JUMP_THREAD")
                        ctx.optimization.jump_thread = enable;
                    else if (flag == "DCE")
                        ctx.optimization.dce = enable;
                    else if (flag == "OPS_XOR")
                        ctx.optimization.ops_xor = enable;
                    else if (flag == "OPS_INC")
                        ctx.optimization.ops_inc = enable;
                    else if (flag == "OPS_OR")
                        ctx.optimization.ops_or = enable;
                    else if (flag == "OPS_LOGIC")
                        ctx.optimization.ops_logic = enable;
                    else if (flag == "OPS_SLA")
                        ctx.optimization.ops_sla = enable;
                    else if (flag == "OPS_ROT")
                        ctx.optimization.ops_rot = enable;
                    else if (flag == "OPS_ROT")
                        ctx.optimization.ops_rot = enable;
                    else if (flag == "OPS_RST")
                        ctx.optimization.ops_rst = enable;
                    else if (flag == "OPS_ADD0")
                        ctx.optimization.ops_add0 = enable;
                    else if (flag == "BRANCH_LONG")
                        ctx.optimization.branch_long = enable;
                    else if (flag == "OPS") {
                        ctx.optimization.ops_xor = enable;
                        ctx.optimization.ops_inc = enable;
                        ctx.optimization.ops_or = enable;
                        ctx.optimization.ops_logic = enable;
                        ctx.optimization.ops_sla = enable;
                        ctx.optimization.ops_rot = enable;
                        ctx.optimization.ops_rst = enable;
                        ctx.optimization.ops_add0 = enable;
                    }
                    else if (flag == "UNSAFE") {
                        ctx.optimization.ops_xor = enable;
                        ctx.optimization.ops_inc = enable;
                        ctx.optimization.ops_or = enable;
                        ctx.optimization.ops_logic = enable;
                        ctx.optimization.ops_sla = enable;
                        ctx.optimization.ops_rot = enable;
                        ctx.optimization.ops_add0 = enable;
                    }
                    else if (flag == "SIZE" || flag == "ALL") {
                        ctx.optimization.branch_short = enable;
                        ctx.optimization.ops_xor = enable;
                        ctx.optimization.ops_inc = enable;
                        ctx.optimization.ops_or = enable;
                        ctx.optimization.dce = enable;
                        ctx.optimization.jump_thread = enable;
                        ctx.optimization.branch_long = enable;
                        ctx.optimization.ops_logic = enable;
                        ctx.optimization.ops_sla = enable;
                        ctx.optimization.ops_rot = enable;
                        ctx.optimization.ops_rst = enable;
                        ctx.optimization.ops_add0 = enable;
                    }
                    else if (flag == "SPEED") {
                         if (enable) {
                             ctx.optimization.branch_short = false;
                             ctx.optimization.ops_xor = true;
                             ctx.optimization.ops_inc = true;
                             ctx.optimization.ops_or = true;
                             ctx.optimization.dce = true;
                             ctx.optimization.jump_thread = true;
                             ctx.optimization.ops_logic = true;
                             ctx.optimization.ops_sla = true;
                             ctx.optimization.ops_rot = true;
                             ctx.optimization.ops_rst = true;
                             ctx.optimization.ops_add0 = true;
                         } else {
                             ctx.optimization.ops_xor = false;
                             ctx.optimization.ops_inc = false;
                             ctx.optimization.ops_or = false;
                             ctx.optimization.dce = false;
                             ctx.optimization.jump_thread = false;
                             ctx.optimization.ops_logic = false;
                             ctx.optimization.ops_sla = false;
                             ctx.optimization.ops_rot = false;
                             ctx.optimization.ops_rot = false;
                             ctx.optimization.ops_rst = false;
                             ctx.optimization.ops_add0 = false;
                         }
                    }
                }
            }
        }
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) override { return false; }
        virtual void on_jump_out_of_range(const std::string& mnemonic, int16_t offset) override {}
        virtual void on_ifdef_directive(const std::string& symbol) override {
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            bool is_defined_in_symbols = false;
            int32_t dummy;
            is_defined_in_symbols = on_symbol_resolve(symbol, dummy);
            bool is_defined_in_defines = m_context.defines.map.count(symbol) > 0;
            bool condition_result = parent_active && (is_defined_in_symbols || is_defined_in_defines);
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifexist_directive(const std::string& filename) override {
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            bool file_exists = m_context.source_provider->exists(filename);
            bool condition_result = parent_active && file_exists;
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifndef_directive(const std::string& symbol) override {
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            bool is_defined_in_symbols = false;
            int32_t dummy;
            is_defined_in_symbols = on_symbol_resolve(symbol, dummy);
            bool is_defined_in_defines = m_context.defines.map.count(symbol) > 0;
            bool condition_result = parent_active && !is_defined_in_symbols && !is_defined_in_defines;
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifnb_directive(const std::string& arg) override {
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            bool condition_result = parent_active && !arg.empty();
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            m_context.source.conditional_stack.push_back({condition_result, false});
        }
        virtual void on_ifidn_directive(const std::string& arg1, const std::string& arg2) override {
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            std::string s1 = arg1;
            std::string s2 = arg2;
            if (s1.length() >= 2 && s1.front() == '<' && s1.back() == '>')
                s1 = s1.substr(1, s1.length() - 2);
            if (s2.length() >= 2 && s2.front() == '<' && s2.back() == '>')
                s2 = s2.substr(1, s2.length() - 2);
            bool condition_result = parent_active && (s1 == s2);
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
        virtual void on_endw_directive() override {
            if (m_context.source.control_stack.empty() || m_context.source.control_stack.back() != Context::Source::ControlType::WHILE) {
                m_context.assembler.report_error("Mismatched ENDW.");
                return;
            }
            typename Context::While::State while_block = std::move(m_context.while_loop.stack.back());
            m_context.while_loop.stack.pop_back();
            m_context.source.control_stack.pop_back();
            while_block.body.insert(while_block.body.begin(), "WHILE " + while_block.expression);
            while_block.body.push_back("ENDW");
            if (while_block.active) {
                if (m_context.macros.in_expansion && !m_context.macros.stack.empty()) {
                    typename Context::Macros::ExpansionState& current_macro_state = m_context.macros.stack.back();
                    current_macro_state.macro.body.insert(current_macro_state.macro.body.begin() + current_macro_state.next_line_index, std::make_move_iterator(while_block.body.begin()), std::make_move_iterator(while_block.body.end()));
                } else
                    m_context.source.lines_stack.insert(m_context.source.lines_stack.end(), std::make_move_iterator(while_block.body.rbegin()), std::make_move_iterator(while_block.body.rend()));
            } else {
                if (!m_context.while_loop.stack.empty()) {
                    auto& parent_while = m_context.while_loop.stack.back();
                    parent_while.body.insert(parent_while.body.end(), std::make_move_iterator(while_block.body.begin()), std::make_move_iterator(while_block.body.end()));
                }
            }
            if (!while_block.active) {
                if (!m_context.while_loop.iteration_counters.empty())
                    m_context.while_loop.iteration_counters.pop_back();
            }
        }
        virtual void on_exitw_directive() override {
            if (this->m_context.source.parser->is_in_while_block()) {
                if (!m_context.while_loop.stack.empty())
                    m_context.while_loop.stack.back().is_exiting = true;
            } else
                m_context.assembler.report_error("EXITW directive used outside of a WHILE block.");
        }
        virtual void on_break_directive() override {
            if (!m_context.source.control_stack.empty()) {
                auto& control_stack = m_context.source.control_stack;
                auto it = std::find_if(control_stack.rbegin(), control_stack.rend(), [](const auto& type) {
                    return type == Context::Source::ControlType::WHILE || type == Context::Source::ControlType::REPEAT;
                });
                if (it != control_stack.rend()) {
                    if (*it == Context::Source::ControlType::WHILE)
                        on_exitw_directive();
                    else if (*it == Context::Source::ControlType::REPEAT)
                        on_exitr_directive();
                }
                return;
            }
            m_context.assembler.report_error("BREAK directive used outside of a loop block.");
        }
        virtual bool on_while_recording(const std::string& line) override {
            if (!m_context.while_loop.stack.empty()) {
                auto& while_block = m_context.while_loop.stack.back();
                if (while_block.skip_lines > 0) {
                    while_block.skip_lines--;
                } else
                    while_block.body.push_back(line);
                return !while_block.active || while_block.is_exiting;
            }
            return false; 
        }
        virtual void on_exitr_directive() override {
            if (!this->m_context.source.parser->is_in_repeat_block())
                m_context.assembler.report_error("EXITR directive used outside of a REPT block.");
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
                m_context.assembler.report_error("Mismatched ENDIF.");
            m_context.source.control_stack.pop_back();
            m_context.source.conditional_stack.pop_back();
        }
        virtual void on_source_line_begin() override {}
        virtual void on_source_line_end() override {}
        virtual void on_assemble(std::vector<uint8_t> bytes) override {}
        virtual bool on_repeat_recording(const std::string& line) override {
            if (!m_context.repeat.stack.empty()) {
                m_context.repeat.stack.back().body.push_back(line);
                return true;
            }
            return false;
        }
        virtual void on_endr_directive() override {
            if (m_context.source.control_stack.empty() || m_context.source.control_stack.back() != Context::Source::ControlType::REPEAT)
                m_context.assembler.report_error("Mismatched ENDR.");
            typename Context::Repeat::State& rept_block = m_context.repeat.stack.back();
            std::vector<std::string> expanded_lines;
            for (size_t i = 0; i < rept_block.count; ++i) {
                rept_block.current_iteration = i + 1;
                std::string iteration_str = std::to_string(rept_block.current_iteration);
                for (const auto& line_template : rept_block.body) {
                    std::string line = line_template;
                    typename Strings::Tokens tokens;
                    tokens.process(line);
                    if (tokens.count() > 0 && tokens[0].upper() == "EXITR") {
                        on_exitr_directive();
                        break;
                    }
                    Strings::replace_words(line, "\\@", iteration_str);
                    expanded_lines.push_back(line);
                }
            }
            if (m_context.macros.in_expansion && !m_context.macros.stack.empty()) {
                typename Context::Macros::ExpansionState& current_macro_state = m_context.macros.stack.back();
                current_macro_state.macro.body.insert(current_macro_state.macro.body.begin() + current_macro_state.next_line_index, expanded_lines.begin(), expanded_lines.end());
            } else
                m_context.source.lines_stack.insert(m_context.source.lines_stack.end(), expanded_lines.rbegin(), expanded_lines.rend());
            if (this->m_context.source.parser->is_in_while_block()) {
                auto& while_block = m_context.while_loop.stack.back();
                while_block.body.insert(m_context.while_loop.stack.back().body.end(), "REPT " + rept_block.expression);
                while_block.body.insert(m_context.while_loop.stack.back().body.end(), rept_block.body.begin(), rept_block.body.end());
                while_block.body.insert(m_context.while_loop.stack.back().body.end(), "ENDR");
                while_block.skip_lines = expanded_lines.size();
            }
            m_context.repeat.stack.pop_back();
            m_context.source.control_stack.pop_back();
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
                    if (tokens.count() > 0) {
                        const std::string& directive = tokens[0].upper();
                        if (directive == "SHIFT") {
                            if (tokens.count() > 1) m_context.assembler.report_error("SHIFT directive expects no parameters.");
                            if (!current_macro_state.parameters.empty())
                                current_macro_state.parameters.erase(current_macro_state.parameters.begin());
                            return;
                        }
                        else if (directive == "EXITM") {
                            if (tokens.count() > 1)
                                m_context.assembler.report_error("EXITM directive expects no parameters.");
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
    protected:
        void on_if_directive(const std::string& expression, bool stop_on_evaluate_error) {
            m_context.source.control_stack.push_back(Context::Source::ControlType::CONDITIONAL);
            bool parent_active = this->m_context.source.parser->is_in_active_block();
            bool condition_result = false;
            if (parent_active) {
                Expressions expr_eval(*this);
                int32_t value;
                if (expr_eval.evaluate(expression, value))
                    condition_result = (value != 0);
                else {
                    if (stop_on_evaluate_error)
                        m_context.assembler.report_error("Invalid IF expression: " + expression);
                }
            }
            m_context.source.conditional_stack.push_back({parent_active && condition_result, false});
        }
        void on_rept_directive(const std::string& counter_expr, bool stop_on_evaluate_error) {
            m_context.source.control_stack.push_back(Context::Source::ControlType::REPEAT);
            int32_t count = 0;
            if (m_context.while_loop.stack.empty() || (m_context.while_loop.stack.back().active && !m_context.while_loop.stack.back().is_exiting)) {
                Expressions expression(*this);
                if (expression.evaluate(counter_expr, count)) {
                    if (count < 0)
                        m_context.assembler.report_error("REPT count cannot be negative.");
                } else {
                    if (stop_on_evaluate_error)
                        m_context.assembler.report_error("Invalid REPT expression: " + counter_expr);
                }
            }
            m_context.repeat.stack.push_back({(size_t)count, 0, {}, counter_expr});
        }
        virtual void on_while_directive(const std::string& expression, bool stop_on_evaluate_error) {
            bool condition_result = false;
            if (m_context.while_loop.stack.empty() || (m_context.while_loop.stack.back().active && !m_context.while_loop.stack.back().is_exiting)) {
                Expressions expr_eval(*this);
                int32_t value;
                if (expr_eval.evaluate(expression, value))
                    condition_result = (value != 0);
                else {
                    if (stop_on_evaluate_error)
                        m_context.assembler.report_error("Invalid WHILE expression: " + expression);
                }
            }
            if (m_context.while_loop.iteration_counters.size() <= m_context.while_loop.stack.size()) {
                m_context.while_loop.iteration_counters.push_back(0);
            }
            m_context.source.control_stack.push_back(Context::Source::ControlType::WHILE);
            if (this->m_context.source.parser->is_in_active_block() && !m_context.while_loop.iteration_counters.empty()) {
                m_context.while_loop.iteration_counters.back()++;
                if (m_context.while_loop.iteration_counters.back() > m_context.assembler.m_options.compilation.max_while_iterations) {
                    m_context.assembler.report_error("WHILE loop exceeded max iterations (" + std::to_string(m_context.assembler.m_options.compilation.max_while_iterations) + "). Possible infinite loop.");
                }
            }
            m_context.while_loop.stack.push_back({expression, {}, condition_result, 0, false});
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
        using Operand = typename Operands::Operand;
        using OperandType = typename Operands::Operand::Type;

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
                                this->m_context.results.symbols_table[symbol_pair.first] = {symbol_pair.first, symbol->value[index], symbol->label};
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
            update_symbol(label, this->m_context.address.current_logical, false, false, true);
        };
        virtual void on_equ_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, false);
        };
        virtual void on_set_directive(const std::string& label, const std::string& value) override {
            on_const(label, value, true);
        };
        virtual void on_org_directive(const std::string& label) override {
            int32_t num_val;
            if (Strings::is_number(label, num_val, this->m_context.assembler.m_options.numbers))
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
            if (Strings::is_number(label, num_val, this->m_context.assembler.m_options.numbers)) {
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
        virtual bool on_operand_not_matching(const Operand& operand, OperandType expected) override {
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
        virtual void on_while_directive(const std::string& expression) override {
            BasePolicy::on_while_directive(expression, false);
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
            update_symbol(label, num_val, !evaluated, redefinable, false);
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
        void update_symbol(const std::string& name, int32_t value, bool undefined, bool redefinable, bool label) {
            std::string actual_name = this->get_absolute_symbol_name(name);
            auto it = this->m_context.symbols.map.find(actual_name);
            if (it == this->m_context.symbols.map.end()) {
                typename Context::Symbols::Symbol* new_symbol = new typename Context::Symbols::Symbol{redefinable, 0, {value}, {undefined}, false, label};
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
                        symbol->undefined.push_back(undefined);
                        m_symbols_stable = false;
                        return;
                    }
                    if (symbol->value[index] != value || symbol->undefined[index] != undefined) {
                        symbol->value[index] = value;
                        symbol->undefined[index] = undefined;
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
        using Operand = typename Operands::Operand;
        using OperandType = typename Operands::Operand::Type;
        
        AssemblyPhase(Context& context) : BasePolicy(context) {}
        virtual ~AssemblyPhase() = default;

        virtual void on_initialize() override {
            this->reset_symbols_index();
            this->m_context.results.listing.clear();
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
        virtual void on_while_directive(const std::string& expression) override {
            BasePolicy::on_while_directive(expression, true);
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
        virtual void on_source_line_begin() override {
            m_line_start_address = this->m_context.address.current_logical;
        }
        virtual void on_source_line_end() override {
            if (this->m_context.source.source_location) {
                uint16_t end_addr = this->m_context.address.current_logical;
                std::vector<uint8_t> bytes;
                for(uint16_t i = m_line_start_address; i < end_addr; ++i)
                    bytes.push_back(this->m_context.memory->peek(i));
                this->m_context.results.listing.push_back({*this->m_context.source.source_location, m_line_start_address, bytes});
            }
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
        uint16_t m_line_start_address = 0;
    };
    class Keywords {
    public:
        Keywords(Context& context) : m_context(context) {}
        bool is_mnemonic(const std::string& s) const { return is_in_set(s, mnemonics()); }
        bool is_directive(const std::string& s) const { return is_in_set(s, directives()) || m_context.assembler.custom_directives.count(s); }
        bool is_register(const std::string& s) const { return is_in_set(s, registers()); }
        bool is_reserved(const std::string& s) const { return is_mnemonic(s) || is_directive(s) || is_register(s); }
        bool is_valid_label_name(const std::string& s) const {
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
        Context& m_context;
        static bool is_in_set(const std::string& s, const std::set<std::string>& set) { return set.count(s); }
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
                "ALIGN", "ASCIZ", "ASSERT", "BINARY", "BLOCK", "BREAK", "BYTE", "DB", "DD", "DEFB", "DEFH",
                "DEFINE", "DEFL", "DEFG", "DEFM", "DEFS", "DEFW", "DEPHASE", "DG", "DH", "DISPLAY", "DM", "EXITW",
                "DQ", "DS", "DUP", "DW", "DWORD", "DZ", "ECHO", "EDUP", "ELSE", "END", "ENDIF", "ENDM",
                "ENDP", "ENDR", "ENDW", "EQU", "ERROR", "EXITM", "EXITR", "HEX", "IF", "IFDEF", "OPTIMIZE",
                "IFIDN", "IFNB", "IFNDEF", "INCBIN", "INCLUDE", "LOCAL", "MACRO", "ORG", "PHASE",
                "PROC", "REPT", "SET", "SHIFT", "UNDEFINE", "UNPHASE", "WEND", "WHILE", "WORD"
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
        bool encode(const std::string& mnemonic, const std::vector<typename Operands::Operand>& operands) {            
            if (m_policy.context().assembler.m_keywords.is_directive(mnemonic)) {
                if (encode_data_block(mnemonic, operands))
                    return true;
            } else if (!m_policy.context().assembler.m_keywords.is_mnemonic(mnemonic))
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
            return false;
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
        using Operand = typename Operands::Operand;
        using OperandType = typename Operands::Operand::Type;
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
        void optimize_jump_target(int32_t& target) {
            if (!m_policy.context().optimization.jump_thread || !m_policy.context().assembler.m_options.compilation.enable_optimization)
                return;
            std::set<int32_t> visited;
            visited.insert(target);
            auto& targets = m_policy.context().prev_jump_targets;
            while (targets.count(target)) {
                int32_t next_target = targets.at(target);
                if (visited.count(next_target))
                    break; 
                target = next_target;
                visited.insert(target);
            }
        }
        bool encode_data_block(const std::string& mnemonic, const std::vector<Operand>& ops) {
            const auto& directive_options = m_policy.context().assembler.m_options.directives;
            if (!directive_options.enabled || !directive_options.allow_data_definitions)
                return false;
            if (mnemonic == "DB" || mnemonic == "DEFB" || mnemonic == "BYTE" || mnemonic == "DM" || mnemonic == "DEFM") {
                std::vector<uint8_t> bytes;
                for (const auto& op : ops) {
                    if (op.type == OperandType::STRING_LITERAL) {
                        for (char c : op.str_val)
                            bytes.push_back((uint8_t)c);
                    } else if (match_imm8(op)) 
                        bytes.push_back((uint8_t)op.num_val);
                    else
                        m_policy.context().assembler.report_error("Unsupported or out-of-range operand for DB: " + op.str_val);
                }
                if (!bytes.empty())
                    assemble(bytes);
                return true;
            } else if (mnemonic == "DW" || mnemonic == "DEFW" || mnemonic == "WORD") {
                std::vector<uint8_t> bytes;
                for (const auto& op : ops) {
                    if (match_imm16(op) || match_char(op)) {
                        bytes.push_back((uint8_t)(op.num_val & 0xFF));
                        bytes.push_back((uint8_t)(op.num_val >> 8));
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DW: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                if (!bytes.empty())
                    assemble(bytes);
                return true;
            } else if (mnemonic == "DWORD" || mnemonic == "DD") {
                std::vector<uint8_t> bytes;
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        bytes.push_back((uint8_t)(op.num_val & 0xFF));
                        bytes.push_back((uint8_t)((op.num_val >> 8) & 0xFF));
                        bytes.push_back((uint8_t)((op.num_val >> 16) & 0xFF));
                        bytes.push_back((uint8_t)((op.num_val >> 24) & 0xFF));
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DWORD/DD: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                if (!bytes.empty())
                    assemble(bytes);
                return true;
            } else if (mnemonic == "DQ") {
                std::vector<uint8_t> bytes;
                for (const auto& op : ops) {
                    if (match(op, OperandType::IMMEDIATE)) {
                        uint64_t val = (uint64_t)(op.num_val);
                        for (int i = 0; i < 8; ++i)
                            bytes.push_back((uint8_t)((val >> (i*8)) & 0xFF));
                    } else
                        m_policy.context().assembler.report_error("Unsupported operand for DQ: " + (op.str_val.empty() ? "unknown" : op.str_val));
                }
                if (!bytes.empty())
                    assemble(bytes);
                return true;
            } else if (mnemonic == "DH" || mnemonic == "HEX" || mnemonic == "DEFH") {
                if (ops.empty())
                    m_policy.context().assembler.report_error(mnemonic + " requires at least one string argument.");
                std::vector<uint8_t> bytes;
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
                        bytes.push_back(byte_val);
                    }
                }
                if (!bytes.empty()) assemble(bytes);
                return true;
            } else if (mnemonic == "DZ" || mnemonic == "ASCIZ") {
                if (ops.empty())
                    m_policy.context().assembler.report_error(mnemonic + " requires at least one argument.");
                std::vector<uint8_t> bytes;
                for (const auto& op : ops) {
                    if (match_string(op)) {
                        for (char c : op.str_val)
                            bytes.push_back((uint8_t)c);
                    } else if (match_imm8(op))
                        bytes.push_back((uint8_t)op.num_val);
                    else
                        m_policy.context().assembler.report_error("Unsupported operand for " + mnemonic + ": " + op.str_val);
                }
                bytes.push_back(0x00);
                assemble(bytes);
                return true;
            } else if (mnemonic == "DS" || mnemonic == "DEFS" || mnemonic == "BLOCK") {
                if (ops.empty() || ops.size() > 2)
                    m_policy.context().assembler.report_error(mnemonic + " requires 1 or 2 operands.");
                if (!match_imm16(ops[0]))
                    m_policy.context().assembler.report_error(mnemonic + " size must be a number.");
                size_t count = ops[0].num_val;
                uint8_t fill_value = (ops.size() == 2) ? (uint8_t)(ops[1].num_val) : 0;
                std::vector<uint8_t> bytes(count, fill_value);
                assemble(bytes);
                return true;
            } else if (mnemonic == "DG" || mnemonic == "DEFG") {
                std::vector<uint8_t> bytes;
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
                        bytes.push_back((uint8_t)std::stoul(byte_str, nullptr, 2));
                    }
                }
                if (!bytes.empty())
                    assemble(bytes);
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
                if (op.num_val == 1 && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else if ((uint8_t)op.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else
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
                if (op.type == OperandType::IMMEDIATE)
                    m_policy.context().jump_targets[m_policy.context().address.current_logical] = op.num_val;
                int32_t target_addr = op.num_val;
                if (op.type == OperandType::IMMEDIATE)
                    optimize_jump_target(target_addr);
                if (m_policy.context().optimization.branch_short && m_policy.context().assembler.m_options.compilation.enable_optimization && op.type == OperandType::IMMEDIATE) {
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
                    if (offset >= -128 && offset <= 127) {
                        assemble({0x18, (uint8_t)(offset)});
                        return true;
                    }
                }
                assemble({0xC3, (uint8_t)(target_addr & 0xFF), (uint8_t)(target_addr >> 8)});
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
                if (op.type == OperandType::IMMEDIATE)
                    m_policy.context().jump_targets[m_policy.context().address.current_logical] = op.num_val;
                int32_t target_addr = op.num_val;
                int32_t original_target = target_addr;
                if (op.type == OperandType::IMMEDIATE) {
                    optimize_jump_target(target_addr);
                }
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
                if (offset < -128 || offset > 127) {
                    if (m_policy.context().optimization.branch_long && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                        assemble({0xC3, (uint8_t)(target_addr & 0xFF), (uint8_t)(target_addr >> 8)});
                        return true;
                    }
                    offset = original_target - (m_policy.context().address.current_logical + instruction_size);
                    if (offset < -128 || offset > 127)
                        m_policy.on_jump_out_of_range(mnemonic, offset);
                }
                if (offset == 0 && m_policy.context().optimization.dce && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                    // JR 0 / JR cc, 0 is effectively a NOP (or conditional NOP) that takes time but does nothing.
                    return true;
                }
                assemble({0x18, (uint8_t)(offset)});
                return true;
            }
            if (mnemonic == "ADD" && match_imm8(op)) {
                if (op.num_val == 1 && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else if ((uint8_t)op.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else if (op.num_val == 0 && m_policy.context().optimization.ops_add0 && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
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
                if (op.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xAF}); // XOR A
                else if ((uint8_t)op.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else
                    assemble({0xE6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "XOR" && match_imm8(op)) {
                if (op.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
                    assemble({0xEE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "OR" && match_imm8(op)) {
                if (op.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else if ((uint8_t)op.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else
                    assemble({0xF6, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "CP" && match_imm8(op)) {
                if (op.num_val == 0 && m_policy.context().optimization.ops_or && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
                    assemble({0xFE, (uint8_t)op.num_val});
                return true;
            }
            if (mnemonic == "DJNZ" && match_imm16(op)) {
                int32_t target_addr = op.num_val;
                int32_t original_target = target_addr;
                if (op.type == OperandType::IMMEDIATE)
                    optimize_jump_target(target_addr);
                uint16_t instruction_size = 2;
                int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
                if (offset < -128 || offset > 127) {
                    offset = original_target - (m_policy.context().address.current_logical + instruction_size);
                    if (offset < -128 || offset > 127)
                        m_policy.on_jump_out_of_range(mnemonic, offset);
                }
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
                if (m_policy.context().optimization.ops_rst && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                    uint32_t addr = op.num_val;
                    if (addr <= 0x38 && (addr % 8 == 0)) {
                        // RST 00, 08, 10, 18, 20, 28, 30, 38
                        // RST 00 -> C7, RST 08 -> CF ...
                        // Base C7. 00->0, 08->1, 10->2 ...
                        // Opcode = 0xC7 + (addr / 8) * 8 = 0xC7 + addr
                        assemble({(uint8_t)(0xC7 + addr)});
                        return true;
                    }
                }
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
                if (mnemonic == "SLA" && op.str_val == "A" && m_policy.context().optimization.ops_sla && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                    assemble({0x87}); // ADD A, A
                    return true;
                }
                if (op.str_val == "A" && m_policy.context().optimization.ops_rot && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                    if (mnemonic == "RLC") { assemble({0x07}); return true; } // RLCA
                    if (mnemonic == "RRC") { assemble({0x0F}); return true; } // RRCA
                    if (mnemonic == "RL")  { assemble({0x17}); return true; } // RLA
                    if (mnemonic == "RR")  { assemble({0x1F}); return true; } // RRA
                }
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
                if (m_policy.context().optimization.dce && m_policy.context().assembler.m_options.compilation.enable_optimization && op1.str_val == op2.str_val)
                    return true;
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
                if (op1.str_val == "A" && op2.num_val == 0 && m_policy.context().optimization.ops_xor && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                    assemble({0xAF}); // XOR A optimization
                } else {
                    uint8_t dest_code = reg8_map().at(op1.str_val);
                    assemble({(uint8_t)(0x06 | (dest_code << 3)), (uint8_t)op2.num_val});
                }
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
                if (op2.num_val == 1 && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else if ((uint8_t)op2.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else if (op2.num_val == 0 && m_policy.context().optimization.ops_add0 && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
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
                if (op2.num_val == 1 && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else if ((uint8_t)op2.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else
                    assemble({0xD6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "AND" && op1.str_val == "A" && match_imm8(op2)) {
                if (op2.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xAF}); // XOR A
                else if ((uint8_t)op2.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3D}); // DEC A
                else
                    assemble({0xE6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "XOR" && op1.str_val == "A" && match_imm8(op2)) {
                if (op2.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
                    assemble({0xEE, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "OR" && op1.str_val == "A" && match_imm8(op2)) {
                if (op2.num_val == 0 && m_policy.context().optimization.ops_logic && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else if ((uint8_t)op2.num_val == 0xFF && m_policy.context().optimization.ops_inc && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0x3C}); // INC A
                else
                    assemble({0xF6, (uint8_t)op2.num_val});
                return true;
            }
            if (mnemonic == "CP" && op1.str_val == "A" && match_imm8(op2)) {
                if (op2.num_val == 0 && m_policy.context().optimization.ops_or && m_policy.context().assembler.m_options.compilation.enable_optimization)
                    assemble({0xB7}); // OR A
                else
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
                int32_t target_addr = op2.num_val;
                if (op2.type == OperandType::IMMEDIATE)
                    optimize_jump_target(target_addr);
                if (m_policy.context().optimization.branch_short && m_policy.context().assembler.m_options.compilation.enable_optimization && relative_jump_condition_map().count(op1.str_val) && op2.type == OperandType::IMMEDIATE) {
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
                    if (offset >= -128 && offset <= 127) {
                        assemble({relative_jump_condition_map().at(op1.str_val), (uint8_t)(offset)});
                        return true;
                    }
                }
                uint8_t cond_code = condition_map().at(op1.str_val);
                assemble({(uint8_t)(0xC2 | (cond_code << 3)), (uint8_t)(target_addr & 0xFF), (uint8_t)(target_addr >> 8)});
                return true;
            }
            if (mnemonic == "JR" && match_condition(op1) && match_imm16(op2)) {
                if (relative_jump_condition_map().count(op1.str_val)) {
                    int32_t target_addr = op2.num_val;
                    int32_t original_target = target_addr;
                    if (op2.type == OperandType::IMMEDIATE) {
                        optimize_jump_target(target_addr);
                    }
                    uint16_t instruction_size = 2;
                    int32_t offset = target_addr - (m_policy.context().address.current_logical + instruction_size);
                    if (offset < -128 || offset > 127) {
                        if (m_policy.context().optimization.branch_long && m_policy.context().assembler.m_options.compilation.enable_optimization) {
                            uint8_t cond_code = condition_map().at(op1.str_val);
                            assemble({(uint8_t)(0xC2 | (cond_code << 3)), (uint8_t)(target_addr & 0xFF), (uint8_t)(target_addr >> 8)});
                            return true;
                        }
                        offset = original_target - (m_policy.context().address.current_logical + instruction_size);
                        if (offset < -128 || offset > 127)
                            m_policy.on_jump_out_of_range(mnemonic + " " + op1.str_val, offset);
                    }
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
            m_policy.on_source_line_begin();
            m_policy.context().source.lines_stack.clear();
            m_policy.context().source.lines_stack.push_back(initial_line);
            while (!m_policy.context().source.lines_stack.empty() || m_policy.context().macros.in_expansion) {
                if (expand_macro())
                    continue;
                m_line = m_policy.context().source.lines_stack.back();
                m_policy.context().source.lines_stack.pop_back();
                m_tokens.process(m_line);
                if (m_tokens.count() == 0)
                    continue;
                apply_defines();
                if (is_in_active_block() && process_loops())
                    continue;
                if (process_recordings())
                    continue;
                if (process_conditional_directives())
                    continue;
                if (is_in_active_block()) {
                    if (process_defines())
                        continue;
                    if (process_macro())
                        continue;
                    if (process_label())
                        continue;
                    if (process_non_conditional_directives())
                        continue;
                    if (m_end_of_source)
                        return false;
                    process_instruction();
                }
            }
            m_policy.on_source_line_end();
            return true;
        }
        bool is_in_active_block() const { return m_policy.context().source.conditional_stack.empty() || m_policy.context().source.conditional_stack.back().is_active; }
        bool is_in_repeat_block() const { return !m_policy.context().repeat.stack.empty(); }
        bool is_in_while_block() const { return !m_policy.context().while_loop.stack.empty(); }
    private:
        bool expand_macro() {
            if (m_policy.context().macros.in_expansion) {
                m_policy.on_macro_line();
                return m_policy.context().source.lines_stack.empty();
            }
            return false;
        }
        void apply_defines() {
            const auto& defines_map = m_policy.context().defines.map;
            if (defines_map.empty())
                return;
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
        }
        bool process_defines() {
            const auto& const_opts = m_policy.context().assembler.m_options.directives.constants;
            if (const_opts.enabled && const_opts.allow_define && m_tokens.count() >= 2) {
                size_t define_idx = 0;
                if (m_tokens.count() > 1 && m_policy.context().assembler.m_keywords.is_valid_label_name(m_tokens[0].original()) && !m_policy.context().assembler.m_keywords.is_reserved(m_tokens[0].upper())) {
                    define_idx = 1;
                }
                if (m_tokens.count() > define_idx && m_tokens[define_idx].upper() == "DEFINE") {
                    if (m_tokens.count() < define_idx + 2)
                        m_policy.context().assembler.report_error("DEFINE directive requires a key.");
                    const std::string& key = m_tokens[define_idx + 1].original();
                    if (!m_policy.context().assembler.m_keywords.is_valid_label_name(key))
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
        bool process_loops() {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (m_policy.context().assembler.m_options.directives.allow_while && !is_in_repeat_block()) {
                if (m_tokens.count() >= 2 && m_tokens[0].upper() == "WHILE") {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    const std::string& expr_str = m_tokens[1].original();
                    m_policy.on_while_directive(expr_str);
                    return true;
                }
                if (m_tokens.count() == 1 && (m_tokens[0].upper() == "ENDW")) {
                    m_policy.on_endw_directive();
                    return true;
                }
                if (m_tokens.count() == 1 && m_tokens[0].upper() == "EXITW") {
                    m_policy.on_exitw_directive();
                    return true;
                }
                if (m_tokens.count() == 1 && m_tokens[0].upper() == "BREAK") {
                    m_policy.on_break_directive();
                    return true;
                }
            }
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
                if (m_tokens.count() == 1 && m_tokens[0].upper() == "EXITR") {
                    m_policy.on_exitr_directive();
                    return false;
                }
                if (m_tokens.count() == 1 && m_tokens[0].upper() == "BREAK") {
                    m_policy.on_break_directive();
                    return true;
                }
            }
            return false;
        }
        bool process_recordings() {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (m_policy.context().assembler.m_options.directives.allow_while && !is_in_repeat_block()) {
                if (m_policy.on_while_recording(m_line))
                    return true;
            }
            if (m_policy.context().assembler.m_options.directives.allow_repeat) {
                if (this->is_in_active_block() && m_policy.on_repeat_recording(m_line))
                    return true;
            }
            return false;
        }
        bool process_non_conditional_directives() {
            if (!m_policy.context().assembler.m_options.directives.enabled)
                return false;
            if (process_constant_directives())
                return true;
            if (process_custom_directives())
                return true;
            if (process_procedures())
                return true;
            if (process_memory_directives())
                return true;
            if (process_error_directives())
                return true;
            if (process_optimize_directive())
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
                if (!m_policy.context().assembler.m_keywords.is_reserved(first_token.upper())) {
                    if (m_tokens.count() > 1) {
                        const std::string& next_token_upper = m_tokens[1].upper();
                        if (next_token_upper != "EQU" && next_token_upper != "SET" && next_token_upper != "DEFL" && next_token_upper != "=" && next_token_upper != "PROC" && next_token_upper != "ENDP")
                            is_label = true;
                    } else
                        is_label = true;
                }
            }
            if (is_label) {
                if (!m_policy.context().assembler.m_keywords.is_valid_label_name(label_str))
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
                Operands operand_parser(m_policy);
                std::vector<typename Operands::Operand> operands;
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
            if (directive == "IF") {
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
            } else if (directive == "IFEXIST") {
                if (m_tokens.count() != 2)
                    m_policy.context().assembler.report_error("IFEXIST requires a single filename argument.");
                std::string filename = m_tokens[1].original();
                if (filename.length() > 1 && filename.front() == '"' && filename.back() == '"')
                    filename = filename.substr(1, filename.length() - 2);
                m_policy.on_ifexist_directive(filename);
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
                if (m_policy.context().assembler.m_keywords.is_valid_label_name(label)) {
                    m_tokens.merge(2, m_tokens.count() - 1);
                    const std::string& value = m_tokens[2].original();
                    if (!const_opts.assignments_as_set && const_opts.allow_equ)
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
                    if (!m_policy.context().assembler.m_keywords.is_valid_label_name(label))
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
        bool process_custom_directives() {
            if (m_tokens.count() == 0)
                return false;
            const std::string& directive_upper = m_tokens[0].upper();
            auto it = m_policy.context().assembler.custom_directives.find(directive_upper);
            if (it != m_policy.context().assembler.custom_directives.end()) {
                std::vector<typename Strings::Tokens::Token> args;
                if (m_tokens.count() > 1) {
                    m_tokens.merge(1, m_tokens.count() - 1);
                    args = m_tokens[1].to_arguments();
                }
                it->second(m_policy, args);
                return true;
            }
            return false;
        }
        bool process_procedures() {
            if (m_policy.context().assembler.m_options.directives.allow_proc) {
                if (m_tokens.count() == 2 && m_tokens[1].upper() == "PROC") {
                    const std::string& proc_name = m_tokens[0].original();
                    if (m_policy.context().assembler.m_keywords.is_valid_label_name(proc_name)) {
                        m_policy.on_proc_begin(proc_name);
                        m_policy.context().source.control_stack.push_back(Context::Source::ControlType::PROCEDURE);
                        return true;
                    }
                }
                if ((m_tokens.count() == 1 && m_tokens[0].upper() == "ENDP") || (m_tokens.count() == 2 && m_tokens[1].upper() == "ENDP")) {
                    if (m_policy.context().source.control_stack.empty() || m_policy.context().source.control_stack.back() != Context::Source::ControlType::PROCEDURE)
                        m_policy.context().assembler.report_error("Mismatched ENDP.");
                    std::string name;
                    if (m_tokens.count() == 2)
                        name = m_tokens[0].original();
                    m_policy.on_proc_end(name);
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
                if (m_tokens.count() < 2) {
                    m_policy.on_error_directive("");
                    return true;
                }
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
            } else if (directive_upper == "DISPLAY" || directive_upper == "ECHO") {
                if (m_tokens.count() < 2)
                    m_policy.context().assembler.report_error("DISPLAY directive requires arguments.");
                m_tokens.merge(1, m_tokens.count() - 1);
                auto args = m_tokens[1].to_arguments();
                m_policy.on_display_directive(args);
                return true;
            }
            if (directive_upper == "END") {
                m_end_of_source = true;
                return true;
            }
            return false;
        }
        bool process_optimize_directive() {
            if (m_tokens.count() > 0 && m_tokens[0].upper() == "OPTIMIZE") {
                if (!m_policy.context().assembler.m_options.directives.allow_optimize)
                    return false;
                std::vector<std::string> args;
                for (size_t i = 1; i < m_tokens.count(); ++i)
                    args.push_back(m_tokens[i].original());
                m_policy.on_optimize_directive(args);
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
        std::string m_line;
        typename Strings::Tokens m_tokens;
        bool m_end_of_source = false;
    };
    std::map<std::string, typename Expressions::FunctionInfo> custom_functions;
    std::map<std::string, typename Expressions::OperatorInfo> custom_operators;
    std::map<std::string, double> custom_constants;
    std::map<std::string, std::function<void(IPhasePolicy&, const std::vector<typename Strings::Tokens::Token>&)>> custom_directives;
    Keywords m_keywords;
    size_t max_operator_len = 0;
    const Options m_options;
    Context m_context;
};

#endif //__Z80ASSEMBLE_H__