//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Analyze.h
// Verson: 1.0.5
//
// This file contains the Z80Analyzer class,
// which provides functionality for disassembling Z80 machine code.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#ifndef __Z80ANALYZE_H__
#define __Z80ANALYZE_H__

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#if defined(__GNUC__) || defined(__clang__)
#define Z80_PACKED_STRUCT __attribute__((packed))
#else
#define Z80_PACKED_STRUCT
#endif

#if defined(_MSC_VER)
#define Z80_PUSH_PACK(n) __pragma(pack(push, n))
#define Z80_POP_PACK() __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
#define Z80_PUSH_PACK(n) _Pragma("pack(push, n)")
#define Z80_POP_PACK() _Pragma("pack(pop)")
#else
#define Z80_PUSH_PACK(n)
#define Z80_POP_PACK()
#endif

template <typename, typename, typename> class Z80;

template <typename TMemory, typename TRegisters, typename TLabels> class Z80Analyzer {
public:
    Z80Analyzer(TMemory* memory, TRegisters* registers, TLabels* labels)
        : m_memory(memory), m_registers(registers), m_labels(labels) {
    }
    // Dumps a region of memory into a vector of formatted strings.
    // - address: Starting memory address (will be updated).
    // - rows: Number of rows to dump.
    // - cols: Number of bytes per row.
    // - format: A string specifying the output format for each row.
    //
    // Format specifiers for dump_memory:
    // - %a: Row start address (hex).
    // - %A: Row start address (dec).
    // - %h: Byte values in hexadecimal, separated by spaces.
    // - %d: Byte values in decimal, separated by spaces.
    // - %c: ASCII representation of bytes (non-printable chars are replaced with '.').
    std::vector<std::string> dump_memory(uint16_t& address, size_t rows, size_t cols, const std::string& format = "%a: %h  %c") {
        std::vector<std::string> result;
        result.reserve(rows);
        for (size_t i = 0; i < rows; ++i) {
            uint16_t row_address = address;
            std::vector<uint8_t> row_bytes;
            row_bytes.reserve(cols);
            for (size_t j = 0; j < cols; ++j) {
                if (address > 0xFFFF)
                    break;
                row_bytes.push_back(m_memory->peek(address++));
            }
            if (row_bytes.empty())
                break;
            std::stringstream ss;
            for (size_t k = 0; k < format.length(); ++k) {
                if (format[k] == '%' && k + 1 < format.length()) {
                    char specifier = format[++k];
                    ss << format_dump_segment(specifier, row_address, row_bytes);
                } else
                    ss << format[k];
            }
            result.push_back(ss.str());
        }
        return result;
    }
    // Dumps the current state of CPU registers into a formatted string.
    //
    // Format specifiers for dump_registers:
    // - Lowercase (e.g., %pc, %af, %a): Hexadecimal output.
    // - Uppercase (e.g., %PC, %AF, %A): Decimal output.
    //
    // - %af, %bc, %de, %hl, %ix, %iy, %sp, %pc: 16-bit registers.
    // - %af', %bc', %de', %hl': Alternate 16-bit registers.
    // - %a, %f, %b, %c, %d, %e, %h, %l: 8-bit main registers.
    // - %i, %r: 8-bit special registers.
    // - %flags: String representation of the F register (e.g., "SZ-H-PNC").
    std::string dump_registers(const std::string& format) {
        std::stringstream ss;
        for (size_t i = 0; i < format.length(); ++i) {
            if (format[i] == '%' && i + 1 < format.length()) {
                std::string specifier;
                size_t j = i + 1;
                // Consume multi-character specifiers (e.g., "af", "af'", "flags")
                while (j < format.length() && (isalnum(format[j]) || format[j] == '\''))
                    specifier += format[j++];
                if (!specifier.empty()) {
                    ss << format_register_segment(specifier);
                    i = j - 1;
                } else
                    ss << format[i];
            } else if (format[i] == '\\' && i + 1 < format.length()) {
                switch (format[i + 1]) {
                case 'n':
                    ss << '\n';
                    break;
                case 't':
                    ss << '\t';
                    break;
                default:
                    ss << format[i + 1];
                    break;
                }
                i++;
            } else
                ss << format[i];
        }
        return ss.str();
    }
    // Format specifiers:
    //  - `%a`: Address (hex)
    //  - `%A`: Address (dec)
    //  - `%b`: Instruction bytes (hex)
    //  - `%B`: Instruction bytes (dec)
    //  - `%m`: Mnemonic
    //  - `%t`: T-states (cycle count)
    //  supports width (`%10`), alignment (`%-10`), and fill character (`%-10.b` uses `.` instead of space).
    std::string disassemble(uint16_t& address, const std::string& format) {
        uint16_t initial_address = address;
        std::string label_prefix;
        if constexpr (!std::is_same_v<TLabels, void>) {
            if (m_labels) {
                std::string labels_str = m_labels->get_label(initial_address);
                if (!labels_str.empty()) {
                    label_prefix = labels_str + ":\n";
                }
            }
        }
        parse_instruction(address);
        std::stringstream ss;
        for (size_t i = 0; i < format.length(); ++i) {
            if (format[i] == '%') {
                int width = 0;
                bool left_align = false;
                char fill_char = ' ';
                size_t j = i + 1;
                if (format[j] == '-') {
                    left_align = true;
                    j++;
                }
                while (isdigit(format[j])) {
                    width = width * 10 + (format[j] - '0');
                    j++;
                }
                if (format[j] == '.') {
                    fill_char = '.';
                    j++;
                }
                if (j < format.length()) {
                    char specifier = format[j];
                    std::string replacement;
                    switch (specifier) {
                    case 'a':
                        replacement = format_hex(initial_address, 4);
                        break;
                    case 'A':
                        replacement = format_dec(initial_address);
                        break;
                    case 'b':
                        replacement = get_bytes_str(true);
                        break;
                    case 'B':
                        replacement = get_bytes_str(false);
                        break;
                    case 'm':
                        replacement = m_mnemonic;
                        break;
                    case 't':
                        if (m_ticks_alt > 0) {
                            replacement = "(" + std::to_string(m_ticks) + "/" + std::to_string(m_ticks_alt) + "T)";
                        } else
                            replacement = "(" + std::to_string(m_ticks) + "T)";
                        break;
                    }
                    ss << std::setfill(fill_char) << std::setw(width) << (left_align ? std::left : std::right)
                       << replacement;
                    ss << std::setfill(' ');
                    i = j;
                }
            } else
                ss << format[i];
        }
        return label_prefix + ss.str();
    }
    std::string disassemble(uint16_t& address) { return disassemble(address, "%a: %-12b %m %t"); }
    std::vector<std::string> disassemble(uint16_t& address, size_t lines, const std::string& format = "%a: %-12b %-15m %t") {
        std::vector<std::string> result;
        result.reserve(lines);
        for (size_t i = 0; i < lines; ++i)
            result.push_back(disassemble(address, format));
        return result;
    }
private:
    enum class IndexMode { HL, IX, IY };

    std::string format_register_segment(const std::string& specifier) {
        std::string s_lower = specifier;
        std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(), ::tolower);
        bool is_upper = (specifier.length() > 0 && isupper(specifier[0]));
        if (s_lower == "af")
            return is_upper ? format_dec(m_registers->get_AF()) : format_hex(m_registers->get_AF(), 4);
        if (s_lower == "bc")
            return is_upper ? format_dec(m_registers->get_BC()) : format_hex(m_registers->get_BC(), 4);
        if (s_lower == "de")
            return is_upper ? format_dec(m_registers->get_DE()) : format_hex(m_registers->get_DE(), 4);
        if (s_lower == "hl")
            return is_upper ? format_dec(m_registers->get_HL()) : format_hex(m_registers->get_HL(), 4);
        if (s_lower == "ix")
            return is_upper ? format_dec(m_registers->get_IX()) : format_hex(m_registers->get_IX(), 4);
        if (s_lower == "iy")
            return is_upper ? format_dec(m_registers->get_IY()) : format_hex(m_registers->get_IY(), 4);
        if (s_lower == "sp")
            return is_upper ? format_dec(m_registers->get_SP()) : format_hex(m_registers->get_SP(), 4);
        if (s_lower == "pc")
            return is_upper ? format_dec(m_registers->get_PC()) : format_hex(m_registers->get_PC(), 4);
        if (s_lower == "af'")
            return is_upper ? format_dec(m_registers->get_AFp()) : format_hex(m_registers->get_AFp(), 4);
        if (s_lower == "bc'")
            return is_upper ? format_dec(m_registers->get_BCp()) : format_hex(m_registers->get_BCp(), 4);
        if (s_lower == "de'")
            return is_upper ? format_dec(m_registers->get_DEp()) : format_hex(m_registers->get_DEp(), 4);
        if (s_lower == "hl'")
            return is_upper ? format_dec(m_registers->get_HLp()) : format_hex(m_registers->get_HLp(), 4);
        if (s_lower == "a")
            return is_upper ? format_dec(m_registers->get_A()) : format_hex(m_registers->get_A(), 2);
        if (s_lower == "f")
            return is_upper ? format_dec(m_registers->get_F()) : format_hex(m_registers->get_F(), 2);
        if (s_lower == "b")
            return is_upper ? format_dec(m_registers->get_B()) : format_hex(m_registers->get_B(), 2);
        if (s_lower == "c")
            return is_upper ? format_dec(m_registers->get_C()) : format_hex(m_registers->get_C(), 2);
        if (s_lower == "d")
            return is_upper ? format_dec(m_registers->get_D()) : format_hex(m_registers->get_D(), 2);
        if (s_lower == "e")
            return is_upper ? format_dec(m_registers->get_E()) : format_hex(m_registers->get_E(), 2);
        if (s_lower == "h")
            return is_upper ? format_dec(m_registers->get_H()) : format_hex(m_registers->get_H(), 2);
        if (s_lower == "l")
            return is_upper ? format_dec(m_registers->get_L()) : format_hex(m_registers->get_L(), 2);
        if (s_lower == "i")
            return is_upper ? format_dec(m_registers->get_I()) : format_hex(m_registers->get_I(), 2);
        if (s_lower == "r")
            return is_upper ? format_dec(m_registers->get_R()) : format_hex(m_registers->get_R(), 2);
        if (s_lower == "flags")
            return format_flags_string();
        return "%" + specifier;
    }
    std::string format_flags_string() {
        std::stringstream ss;
        auto f = m_registers->get_F();
        using Flags = typename TRegisters::Flags;
        ss << (f.is_set(Flags::S) ? 'S' : '-');
        ss << (f.is_set(Flags::Z) ? 'Z' : '-');
        ss << (f.is_set(Flags::Y) ? 'Y' : '-');
        ss << (f.is_set(Flags::H) ? 'H' : '-');
        ss << (f.is_set(Flags::X) ? 'X' : '-');
        ss << (f.is_set(Flags::PV) ? 'P' : '-');
        ss << (f.is_set(Flags::N) ? 'N' : '-');
        ss << (f.is_set(Flags::C) ? 'C' : '-');
        return ss.str();
    }
    std::string format_dump_segment(char specifier, uint16_t row_address, const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        switch (specifier) {
        case 'a':
            ss << format_hex(row_address, 4);
            break;
        case 'A':
            ss << std::dec << std::setw(5) << std::setfill('0') << row_address;
            break;
        case 'h':
            for (size_t i = 0; i < bytes.size(); ++i)
                ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
            break;
        case 'd':
            for (size_t i = 0; i < bytes.size(); ++i)
                ss << std::dec << std::setw(3) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
            break;
        case 'c':
            for (uint8_t byte : bytes)
                ss << (isprint(byte) ? static_cast<char>(byte) : '.');
            break;
        default:
            ss << '%' << specifier;
            break;
        }
        return ss.str();
    }
    void parse_instruction(uint16_t& address) {
        m_address = address;
        m_mnemonic.clear();
        m_bytes.clear();
        m_ticks = 0;
        m_ticks_alt = 0;
        set_index_mode(IndexMode::HL);
        uint8_t opcode = peek_next_opcode();
        while (opcode == 0xDD || opcode == 0xFD) {
            set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
            opcode = peek_next_opcode();
        }
        switch (opcode) {
        case 0x00:
            m_mnemonic = "NOP";
            m_ticks = 4;
            break;
        case 0x01:
            m_mnemonic = "LD BC, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0x02:
            m_mnemonic = "LD (BC), A";
            m_ticks = 7;
            break;
        case 0x03:
            m_mnemonic = "INC BC";
            m_ticks = 6;
            break;
        case 0x04:
            m_mnemonic = "INC B";
            m_ticks = 4;
            break;
        case 0x05:
            m_mnemonic = "DEC B";
            m_ticks = 4;
            break;
        case 0x06:
            m_mnemonic = "LD B, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0x07:
            m_mnemonic = "RLCA";
            m_ticks = 4;
            break;
        case 0x08:
            m_mnemonic = "EX AF, AF'";
            m_ticks = 4;
            break;
        case 0x09:
            m_mnemonic = "ADD " + get_indexed_reg_str() + ", BC";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 15;
            break;
        case 0x0A:
            m_mnemonic = "LD A, (BC)";
            m_ticks = 7;
            break;
        case 0x0B:
            m_mnemonic = "DEC BC";
            m_ticks = 6;
            break;
        case 0x0C:
            m_mnemonic = "INC C";
            m_ticks = 4;
            break;
        case 0x0D:
            m_mnemonic = "DEC C";
            m_ticks = 4;
            break;
        case 0x0E:
            m_mnemonic = "LD C, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0x0F:
            m_mnemonic = "RRCA";
            m_ticks = 4;
            break;
        case 0x10: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "DJNZ " + format_address_label(address);
            m_ticks = 8;
            m_ticks_alt = 13;
            break;
        }
        case 0x11:
            m_mnemonic = "LD DE, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0x12:
            m_mnemonic = "LD (DE), A";
            m_ticks = 7;
            break;
        case 0x13:
            m_mnemonic = "INC DE";
            m_ticks = 6;
            break;
        case 0x14:
            m_mnemonic = "INC D";
            m_ticks = 4;
            break;
        case 0x15:
            m_mnemonic = "DEC D";
            m_ticks = 4;
            break;
        case 0x16:
            m_mnemonic = "LD D, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0x17:
            m_mnemonic = "RLA";
            m_ticks = 4;
            break;
        case 0x18: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "JR " + format_address_label(address);
            m_ticks = 12;
            break;
        }
        case 0x19:
            m_mnemonic = "ADD " + get_indexed_reg_str() + ", DE";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 15;
            break;
        case 0x1A:
            m_mnemonic = "LD A, (DE)";
            m_ticks = 7;
            break;
        case 0x1B:
            m_mnemonic = "DEC DE";
            m_ticks = 6;
            break;
        case 0x1C:
            m_mnemonic = "INC E";
            m_ticks = 4;
            break;
        case 0x1D:
            m_mnemonic = "DEC E";
            m_ticks = 4;
            break;
        case 0x1E:
            m_mnemonic = "LD E, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0x1F:
            m_mnemonic = "RRA";
            m_ticks = 4;
            break;
        case 0x20: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "JR NZ, " + format_address_label(address);
            m_ticks = 7;
            m_ticks_alt = 12;
            break;
        }
        case 0x21:
            m_mnemonic = "LD " + get_indexed_reg_str() + ", " + format_address_label(peek_next_word());
            m_ticks = (get_index_mode() == IndexMode::HL) ? 10 : 14;
            break;
        case 0x22:
            m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 16 : 20;
            break;
        case 0x23:
            m_mnemonic = "INC " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 6 : 10;
            break;
        case 0x24:
            m_mnemonic = "INC " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x25:
            m_mnemonic = "DEC " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x26:
            m_mnemonic = "LD " + get_indexed_h_str() + ", " + format_hex(peek_next_byte());
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 11;
            break;
        case 0x27:
            m_mnemonic = "DAA";
            m_ticks = 4;
            break;
        case 0x28: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "JR Z, " + format_address_label(address);
            m_ticks = 7;
            m_ticks_alt = 12;
            break;
        }
        case 0x29: {
            std::string reg = get_indexed_reg_str();
            m_mnemonic = "ADD " + reg + ", " + reg;
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 15;
            break;
        }
        case 0x2A:
            m_mnemonic = "LD " + get_indexed_reg_str() + ", (" + format_address_label(peek_next_word()) + ")";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 16 : 20;
            break;
        case 0x2B:
            m_mnemonic = "DEC " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 6 : 10;
            break;
        case 0x2C:
            m_mnemonic = "INC " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x2D:
            m_mnemonic = "DEC " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x2E:
            m_mnemonic = "LD " + get_indexed_l_str() + ", " + format_hex(peek_next_byte());
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 11;
            break;
        case 0x2F:
            m_mnemonic = "CPL";
            m_ticks = 4;
            break;
        case 0x30: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "JR NC, " + format_address_label(address);
            m_ticks = 7;
            m_ticks_alt = 12;
            break;
        }
        case 0x31:
            m_mnemonic = "LD SP, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0x32:
            m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), A";
            m_ticks = 13;
            break;
        case 0x33:
            m_mnemonic = "INC SP";
            m_ticks = 6;
            break;
        case 0x34:
            m_mnemonic = "INC " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 23;
            break;
        case 0x35:
            m_mnemonic = "DEC " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 23;
            break;
        case 0x36: {
            std::string addr_str = get_indexed_addr_str();
            std::string val_str = format_hex(peek_next_byte());
            m_mnemonic = "LD " + addr_str + ", " + val_str;
            m_ticks = (get_index_mode() == IndexMode::HL) ? 10 : 19;
            break;
        }
        case 0x37:
            m_mnemonic = "SCF";
            m_ticks = 4;
            break;
        case 0x38: {
            int8_t offset = static_cast<int8_t>(peek_next_byte());
            uint16_t address = m_address + offset;
            m_mnemonic = "JR C, " + format_address_label(address);
            m_ticks = 7;
            m_ticks_alt = 12;
            break;
        }
        case 0x39:
            m_mnemonic = "ADD " + get_indexed_reg_str() + ", SP";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 15;
            break;
        case 0x3A:
            m_mnemonic = "LD A, (" + format_address_label(peek_next_word()) + ")";
            m_ticks = 13;
            break;
        case 0x3B:
            m_mnemonic = "DEC SP";
            m_ticks = 6;
            break;
        case 0x3C:
            m_mnemonic = "INC A";
            m_ticks = 4;
            break;
        case 0x3D:
            m_mnemonic = "DEC A";
            m_ticks = 4;
            break;
        case 0x3E:
            m_mnemonic = "LD A, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0x3F:
            m_mnemonic = "CCF";
            m_ticks = 4;
            break;
        case 0x40:
            m_mnemonic = "LD B, B";
            m_ticks = 4;
            break;
        case 0x41:
            m_mnemonic = "LD B, C";
            m_ticks = 4;
            break;
        case 0x42:
            m_mnemonic = "LD B, D";
            m_ticks = 4;
            break;
        case 0x43:
            m_mnemonic = "LD B, E";
            m_ticks = 4;
            break;
        case 0x44:
            m_mnemonic = "LD B, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x45:
            m_mnemonic = "LD B, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x46:
            m_mnemonic = "LD B, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x47:
            m_mnemonic = "LD B, A";
            m_ticks = 4;
            break;
        case 0x48:
            m_mnemonic = "LD C, B";
            m_ticks = 4;
            break;
        case 0x49:
            m_mnemonic = "LD C, C";
            m_ticks = 4;
            break;
        case 0x4A:
            m_mnemonic = "LD C, D";
            m_ticks = 4;
            break;
        case 0x4B:
            m_mnemonic = "LD C, E";
            m_ticks = 4;
            break;
        case 0x4C:
            m_mnemonic = "LD C, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x4D:
            m_mnemonic = "LD C, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x4E:
            m_mnemonic = "LD C, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x4F:
            m_mnemonic = "LD C, A";
            m_ticks = 4;
            break;
        case 0x50:
            m_mnemonic = "LD D, B";
            m_ticks = 4;
            break;
        case 0x51:
            m_mnemonic = "LD D, C";
            m_ticks = 4;
            break;
        case 0x52:
            m_mnemonic = "LD D, D";
            m_ticks = 4;
            break;
        case 0x53:
            m_mnemonic = "LD D, E";
            m_ticks = 4;
            break;
        case 0x54:
            m_mnemonic = "LD D, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x55:
            m_mnemonic = "LD D, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x56:
            m_mnemonic = "LD D, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x57:
            m_mnemonic = "LD D, A";
            m_ticks = 4;
            break;
        case 0x58:
            m_mnemonic = "LD E, B";
            m_ticks = 4;
            break;
        case 0x59:
            m_mnemonic = "LD E, C";
            m_ticks = 4;
            break;
        case 0x5A:
            m_mnemonic = "LD E, D";
            m_ticks = 4;
            break;
        case 0x5B:
            m_mnemonic = "LD E, E";
            m_ticks = 4;
            break;
        case 0x5C:
            m_mnemonic = "LD E, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x5D:
            m_mnemonic = "LD E, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x5E:
            m_mnemonic = "LD E, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x5F:
            m_mnemonic = "LD E, A";
            m_ticks = 4;
            break;
        case 0x60:
            m_mnemonic = "LD H, B";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x61:
            m_mnemonic = "LD H, C";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x62:
            m_mnemonic = "LD H, D";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x63:
            m_mnemonic = "LD H, E";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x64: {
            std::string reg = get_indexed_h_str();
            m_mnemonic = "LD " + reg + ", " + reg;
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        }
        case 0x65:
            m_mnemonic = "LD " + get_indexed_h_str() + ", " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x66:
            m_mnemonic = "LD H, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x67:
            m_mnemonic = "LD " + get_indexed_h_str() + ", A";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x68:
            m_mnemonic = "LD " + get_indexed_l_str() + ", B";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x69:
            m_mnemonic = "LD " + get_indexed_l_str() + ", C";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x6A:
            m_mnemonic = "LD " + get_indexed_l_str() + ", D";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x6B:
            m_mnemonic = "LD " + get_indexed_l_str() + ", E";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x6C:
            m_mnemonic = "LD " + get_indexed_l_str() + ", " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x6D: {
            std::string reg = get_indexed_l_str();
            m_mnemonic = "LD " + reg + ", " + reg;
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        }
        case 0x6E:
            m_mnemonic = "LD L, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x6F:
            m_mnemonic = "LD " + get_indexed_l_str() + ", A";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x70:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", B";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x71:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", C";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x72:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", D";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x73:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", E";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x74:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", H";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x75:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", L";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x76:
            m_mnemonic = "HALT";
            m_ticks = 4;
            break;
        case 0x77:
            m_mnemonic = "LD " + get_indexed_addr_str() + ", A";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x78:
            m_mnemonic = "LD A, B";
            m_ticks = 4;
            break;
        case 0x79:
            m_mnemonic = "LD A, C";
            m_ticks = 4;
            break;
        case 0x7A:
            m_mnemonic = "LD A, D";
            m_ticks = 4;
            break;
        case 0x7B:
            m_mnemonic = "LD A, E";
            m_ticks = 4;
            break;
        case 0x7C:
            m_mnemonic = "LD A, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x7D:
            m_mnemonic = "LD A, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x7E:
            m_mnemonic = "LD A, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x7F:
            m_mnemonic = "LD A, A";
            m_ticks = 4;
            break;
        case 0x80:
            m_mnemonic = "ADD A, B";
            m_ticks = 4;
            break;
        case 0x81:
            m_mnemonic = "ADD A, C";
            m_ticks = 4;
            break;
        case 0x82:
            m_mnemonic = "ADD A, D";
            m_ticks = 4;
            break;
        case 0x83:
            m_mnemonic = "ADD A, E";
            m_ticks = 4;
            break;
        case 0x84:
            m_mnemonic = "ADD A, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x85:
            m_mnemonic = "ADD A, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x86:
            m_mnemonic = "ADD A, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x87:
            m_mnemonic = "ADD A, A";
            m_ticks = 4;
            break;
        case 0x88:
            m_mnemonic = "ADC A, B";
            m_ticks = 4;
            break;
        case 0x89:
            m_mnemonic = "ADC A, C";
            m_ticks = 4;
            break;
        case 0x8A:
            m_mnemonic = "ADC A, D";
            m_ticks = 4;
            break;
        case 0x8B:
            m_mnemonic = "ADC A, E";
            m_ticks = 4;
            break;
        case 0x8C:
            m_mnemonic = "ADC A, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x8D:
            m_mnemonic = "ADC A, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x8E:
            m_mnemonic = "ADC A, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x8F:
            m_mnemonic = "ADC A, A";
            m_ticks = 4;
            break;
        case 0x90:
            m_mnemonic = "SUB B";
            m_ticks = 4;
            break;
        case 0x91:
            m_mnemonic = "SUB C";
            m_ticks = 4;
            break;
        case 0x92:
            m_mnemonic = "SUB D";
            m_ticks = 4;
            break;
        case 0x93:
            m_mnemonic = "SUB E";
            m_ticks = 4;
            break;
        case 0x94:
            m_mnemonic = "SUB " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x95:
            m_mnemonic = "SUB " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x96:
            m_mnemonic = "SUB " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x97:
            m_mnemonic = "SUB A";
            m_ticks = 4;
            break;
        case 0x98:
            m_mnemonic = "SBC A, B";
            m_ticks = 4;
            break;
        case 0x99:
            m_mnemonic = "SBC A, C";
            m_ticks = 4;
            break;
        case 0x9A:
            m_mnemonic = "SBC A, D";
            m_ticks = 4;
            break;
        case 0x9B:
            m_mnemonic = "SBC A, E";
            m_ticks = 4;
            break;
        case 0x9C:
            m_mnemonic = "SBC A, " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x9D:
            m_mnemonic = "SBC A, " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0x9E:
            m_mnemonic = "SBC A, " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0x9F:
            m_mnemonic = "SBC A, A";
            m_ticks = 4;
            break;
        case 0xA0:
            m_mnemonic = "AND B";
            m_ticks = 4;
            break;
        case 0xA1:
            m_mnemonic = "AND C";
            m_ticks = 4;
            break;
        case 0xA2:
            m_mnemonic = "AND D";
            m_ticks = 4;
            break;
        case 0xA3:
            m_mnemonic = "AND E";
            m_ticks = 4;
            break;
        case 0xA4:
            m_mnemonic = "AND " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xA5:
            m_mnemonic = "AND " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xA6:
            m_mnemonic = "AND " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0xA7:
            m_mnemonic = "AND A";
            m_ticks = 4;
            break;
        case 0xA8:
            m_mnemonic = "XOR B";
            m_ticks = 4;
            break;
        case 0xA9:
            m_mnemonic = "XOR C";
            m_ticks = 4;
            break;
        case 0xAA:
            m_mnemonic = "XOR D";
            m_ticks = 4;
            break;
        case 0xAB:
            m_mnemonic = "XOR E";
            m_ticks = 4;
            break;
        case 0xAC:
            m_mnemonic = "XOR " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xAD:
            m_mnemonic = "XOR " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xAE:
            m_mnemonic = "XOR " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0xAF:
            m_mnemonic = "XOR A";
            m_ticks = 4;
            break;
        case 0xB0:
            m_mnemonic = "OR B";
            m_ticks = 4;
            break;
        case 0xB1:
            m_mnemonic = "OR C";
            m_ticks = 4;
            break;
        case 0xB2:
            m_mnemonic = "OR D";
            m_ticks = 4;
            break;
        case 0xB3:
            m_mnemonic = "OR E";
            m_ticks = 4;
            break;
        case 0xB4:
            m_mnemonic = "OR " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xB5:
            m_mnemonic = "OR " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xB6:
            m_mnemonic = "OR " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0xB7:
            m_mnemonic = "OR A";
            m_ticks = 4;
            break;
        case 0xB8:
            m_mnemonic = "CP B";
            m_ticks = 4;
            break;
        case 0xB9:
            m_mnemonic = "CP C";
            m_ticks = 4;
            break;
        case 0xBA:
            m_mnemonic = "CP D";
            m_ticks = 4;
            break;
        case 0xBB:
            m_mnemonic = "CP E";
            m_ticks = 4;
            break;
        case 0xBC:
            m_mnemonic = "CP " + get_indexed_h_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xBD:
            m_mnemonic = "CP " + get_indexed_l_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xBE:
            m_mnemonic = "CP " + get_indexed_addr_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 7 : 19;
            break;
        case 0xBF:
            m_mnemonic = "CP A";
            m_ticks = 4;
            break;
        case 0xC0:
            m_mnemonic = "RET NZ";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xC1:
            m_mnemonic = "POP BC";
            m_ticks = 10;
            break;
        case 0xC2:
            m_mnemonic = "JP NZ, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xC3:
            m_mnemonic = "JP " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xC4:
            m_mnemonic = "CALL NZ, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xC5:
            m_mnemonic = "PUSH BC";
            m_ticks = 11;
            break;
        case 0xC6:
            m_mnemonic = "ADD A, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xC7:
            m_mnemonic = "RST 00H";
            m_ticks = 11;
            break;
        case 0xC8:
            m_mnemonic = "RET Z";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xC9:
            m_mnemonic = "RET";
            m_ticks = 10;
            break;
        case 0xCA:
            m_mnemonic = "JP Z, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xCB:
            if (get_index_mode() == IndexMode::HL) {
                uint8_t cb_opcode = peek_next_opcode();
                const char* registers[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
                const char* operations[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
                const char* bit_ops[] = {"BIT", "RES", "SET"};
                uint8_t operation_group = cb_opcode >> 6;
                uint8_t bit = (cb_opcode >> 3) & 0x07;
                uint8_t reg_code = cb_opcode & 0x07;
                std::string reg_str = registers[reg_code];
                if (operation_group == 0) { // Rotates/Shifts
                    m_mnemonic = std::string(operations[bit]) + " " + reg_str;
                    m_ticks = (reg_code == 6) ? 15 : 8;
                } else { // BIT, RES, SET
                    std::stringstream ss;
                    ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << reg_str;
                    m_mnemonic = ss.str();
                    m_ticks = (reg_code == 6) ? (operation_group == 1 ? 12 : 15) : 8;
                }
            } else {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint8_t cb_opcode = peek_next_byte();
                const char* operations[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
                const char* bit_ops[] = {"BIT", "RES", "SET"};
                const char* registers[] = {"B", "C", "D", "E", "H", "L", "", "A"};
                std::string index_reg_str = (get_index_mode() == IndexMode::IX) ? "IX" : "IY";
                std::stringstream address_ss;
                address_ss << "(" << index_reg_str << (offset >= 0 ? "+" : "") << static_cast<int>(offset) << ")";
                std::string address_str = address_ss.str();
                uint8_t operation_group = cb_opcode >> 6;
                uint8_t bit = (cb_opcode >> 3) & 0x07;
                uint8_t reg_code = cb_opcode & 0x07;
                std::stringstream ss;
                if (operation_group == 0) { // Rotates/Shifts
                    ss << operations[bit] << " " << address_str;
                    m_ticks = 23;
                } else { // BIT, RES, SET
                    ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << address_str;
                    m_ticks = (operation_group == 1) ? 20 : 23;
                }
                if (reg_code != 6)
                    ss << ", " << registers[reg_code];
                m_mnemonic = ss.str();
            }
            break;
        case 0xCC:
            m_mnemonic = "CALL Z, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xCD:
            m_mnemonic = "CALL " + format_address_label(peek_next_word());
            m_ticks = 17;
            break;
        case 0xCE:
            m_mnemonic = "ADC A, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xCF:
            m_mnemonic = "RST 08H";
            m_ticks = 11;
            break;
        case 0xD0:
            m_mnemonic = "RET NC";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xD1:
            m_mnemonic = "POP DE";
            m_ticks = 10;
            break;
        case 0xD2:
            m_mnemonic = "JP NC, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xD3:
            m_mnemonic = "OUT (" + format_hex(peek_next_byte()) + "), A";
            m_ticks = 11;
            break;
        case 0xD4:
            m_mnemonic = "CALL NC, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xD5:
            m_mnemonic = "PUSH DE";
            m_ticks = 11;
            break;
        case 0xD6:
            m_mnemonic = "SUB " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xD7:
            m_mnemonic = "RST 10H";
            m_ticks = 11;
            break;
        case 0xD8:
            m_mnemonic = "RET C";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xD9:
            m_mnemonic = "EXX";
            m_ticks = 4;
            break;
        case 0xDA:
            m_mnemonic = "JP C, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xDB:
            m_mnemonic = "IN A, (" + format_hex(peek_next_byte()) + ")";
            m_ticks = 11;
            break;
        case 0xDC:
            m_mnemonic = "CALL C, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xDE:
            m_mnemonic = "SBC A, " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xDF:
            m_mnemonic = "RST 18H";
            m_ticks = 11;
            break;
        case 0xE0:
            m_mnemonic = "RET PO";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xE1:
            m_mnemonic = "POP " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 10 : 14;
            break;
        case 0xE2:
            m_mnemonic = "JP PO, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xE3:
            m_mnemonic = "EX (SP), " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 19 : 23;
            break;
        case 0xE4:
            m_mnemonic = "CALL PO, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xE5:
            m_mnemonic = "PUSH HL";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 11 : 15;
            break;
        case 0xE6:
            m_mnemonic = "AND " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xE7:
            m_mnemonic = "RST 20H";
            m_ticks = 11;
            break;
        case 0xE8:
            m_mnemonic = "RET PE";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xE9:
            m_mnemonic = "JP (" + get_indexed_reg_str() + ")";
            m_ticks = (get_index_mode() == IndexMode::HL) ? 4 : 8;
            break;
        case 0xEA:
            m_mnemonic = "JP PE, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xEB:
            m_mnemonic = "EX DE, HL";
            m_ticks = 4;
            break;
        case 0xEC:
            m_mnemonic = "CALL PE, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xED: {
            uint8_t opcodeED = peek_next_opcode();
            set_index_mode(IndexMode::HL);
            switch (opcodeED) {
            case 0x40:
                m_mnemonic = "IN B, (C)";
                m_ticks = 12;
                break;
            case 0x41:
                m_mnemonic = "OUT (C), B";
                m_ticks = 12;
                break;
            case 0x42:
                m_mnemonic = "SBC HL, BC";
                m_ticks = 15;
                break;
            case 0x43:
                m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), BC";
                m_ticks = 20;
                break;
            case 0x44:
            case 0x4C:
            case 0x54:
            case 0x5C:
            case 0x64:
            case 0x6C:
            case 0x74:
            case 0x7C:
                m_mnemonic = "NEG";
                m_ticks = 8;
                break;
            case 0x45:
            case 0x55:
            case 0x5D:
            case 0x65:
            case 0x6D:
            case 0x75:
            case 0x7D:
                m_mnemonic = "RETN";
                m_ticks = 14;
                break;
            case 0x46:
            case 0x4E:
            case 0x66:
            case 0x6E:
                m_mnemonic = "IM 0";
                m_ticks = 8;
                break;
            case 0x47:
                m_mnemonic = "LD I, A";
                m_ticks = 9;
                break;
            case 0x48:
                m_mnemonic = "IN C, (C)";
                m_ticks = 12;
                break;
            case 0x49:
                m_mnemonic = "OUT (C), C";
                m_ticks = 12;
                break;
            case 0x4A:
                m_mnemonic = "ADC HL, BC";
                m_ticks = 15;
                break;
            case 0x4B:
                m_mnemonic = "LD BC, (" + format_address_label(peek_next_word()) + ")";
                m_ticks = 20;
                break;
            case 0x4D:
                m_mnemonic = "RETI";
                m_ticks = 14;
                break;
            case 0x4F:
                m_mnemonic = "LD R, A";
                m_ticks = 9;
                break;
            case 0x50:
                m_mnemonic = "IN D, (C)";
                m_ticks = 12;
                break;
            case 0x51:
                m_mnemonic = "OUT (C), D";
                m_ticks = 12;
                break;
            case 0x52:
                m_mnemonic = "SBC HL, DE";
                m_ticks = 15;
                break;
            case 0x53:
                m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), DE";
                m_ticks = 20;
                break;
            case 0x56: // IM 1
            case 0x76: // IM 1
                m_mnemonic = "IM 1";
                m_ticks = 8;
                break;
            case 0x57:
                m_mnemonic = "LD A, I";
                m_ticks = 9;
                break;
            case 0x58:
                m_mnemonic = "IN E, (C)";
                m_ticks = 12;
                break;
            case 0x59:
                m_mnemonic = "OUT (C), E";
                m_ticks = 12;
                break;
            case 0x5A:
                m_mnemonic = "ADC HL, DE";
                m_ticks = 15;
                break;
            case 0x5B:
                m_mnemonic = "LD DE, (" + format_address_label(peek_next_word()) + ")";
                m_ticks = 20;
                break;
            case 0x5E:
            case 0x7E:
                m_mnemonic = "IM 2";
                m_ticks = 8;
                break;
            case 0x5F:
                m_mnemonic = "LD A, R";
                m_ticks = 9;
                break;
            case 0x60:
                m_mnemonic = "IN H, (C)";
                m_ticks = 12;
                break;
            case 0x61:
                m_mnemonic = "OUT (C), H";
                m_ticks = 12;
                break;
            case 0x62:
                m_mnemonic = "SBC HL, HL";
                m_ticks = 15;
                break;
            case 0x63:
                m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), HL";
                m_ticks = 20;
                break;
            case 0x67:
                m_mnemonic = "RRD";
                m_ticks = 18;
                break;
            case 0x68:
                m_mnemonic = "IN L, (C)";
                m_ticks = 12;
                break;
            case 0x69:
                m_mnemonic = "OUT (C), L";
                m_ticks = 12;
                break;
            case 0x6A:
                m_mnemonic = "ADC HL, HL";
                m_ticks = 15;
                break;
            case 0x6B:
                m_mnemonic = "LD HL, (" + format_address_label(peek_next_word()) + ")";
                m_ticks = 20;
                break;
            case 0x6F:
                m_mnemonic = "RLD";
                m_ticks = 18;
                break;
            case 0x70:
                m_mnemonic = "IN (C)";
                m_ticks = 12;
                break;
            case 0x71:
                m_mnemonic = "OUT (C), 0";
                m_ticks = 12;
                break;
            case 0x72:
                m_mnemonic = "SBC HL, SP";
                m_ticks = 15;
                break;
            case 0x73:
                m_mnemonic = "LD (" + format_address_label(peek_next_word()) + "), SP";
                m_ticks = 20;
                break;
            case 0x78:
                m_mnemonic = "IN A, (C)";
                m_ticks = 12;
                break;
            case 0x79:
                m_mnemonic = "OUT (C), A";
                m_ticks = 12;
                break;
            case 0x7A:
                m_mnemonic = "ADC HL, SP";
                m_ticks = 15;
                break;
            case 0x7B:
                m_mnemonic = "LD SP, (" + format_address_label(peek_next_word()) + ")";
                m_ticks = 20;
                break;
            case 0xA0:
                m_mnemonic = "LDI";
                m_ticks = 16;
                break;
            case 0xA1:
                m_mnemonic = "CPI";
                m_ticks = 16;
                break;
            case 0xA2:
                m_mnemonic = "INI";
                m_ticks = 16;
                break;
            case 0xA3:
                m_mnemonic = "OUTI";
                m_ticks = 16;
                break;
            case 0xA8:
                m_mnemonic = "LDD";
                m_ticks = 16;
                break;
            case 0xA9:
                m_mnemonic = "CPD";
                m_ticks = 16;
                break;
            case 0xAA:
                m_mnemonic = "IND";
                m_ticks = 16;
                break;
            case 0xAB:
                m_mnemonic = "OUTD";
                m_ticks = 16;
                break;
            case 0xB0:
                m_mnemonic = "LDIR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xB1:
                m_mnemonic = "CPIR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xB2:
                m_mnemonic = "INIR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xB3:
                m_mnemonic = "OTIR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xB8:
                m_mnemonic = "LDDR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xB9:
                m_mnemonic = "CPDR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xBA:
                m_mnemonic = "INDR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            case 0xBB:
                m_mnemonic = "OTDR";
                m_ticks = 16;
                m_ticks_alt = 21;
                break;
            default:
                m_mnemonic = "NOP (DB 0xED, " + format_hex(opcodeED) + ")";
                m_ticks = 8;
            }
            break;
        }
        case 0xEE:
            m_mnemonic = "XOR " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xEF:
            m_mnemonic = "RST 28H";
            m_ticks = 11;
            break;
        case 0xF0:
            m_mnemonic = "RET P";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xF1:
            m_mnemonic = "POP AF";
            m_ticks = 10;
            break;
        case 0xF2:
            m_mnemonic = "JP P, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xF3:
            m_mnemonic = "DI";
            m_ticks = 4;
            break;
        case 0xF4:
            m_mnemonic = "CALL P, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xF5:
            m_mnemonic = "PUSH AF";
            m_ticks = 11;
            break;
        case 0xF6:
            m_mnemonic = "OR " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xF7:
            m_mnemonic = "RST 30H";
            m_ticks = 11;
            break;
        case 0xF8:
            m_mnemonic = "RET M";
            m_ticks = 5;
            m_ticks_alt = 11;
            break;
        case 0xF9:
            m_mnemonic = "LD SP, " + get_indexed_reg_str();
            m_ticks = (get_index_mode() == IndexMode::HL) ? 6 : 10;
            break;
        case 0xFA:
            m_mnemonic = "JP M, " + format_address_label(peek_next_word());
            m_ticks = 10;
            break;
        case 0xFB:
            m_mnemonic = "EI";
            m_ticks = 4;
            break;
        case 0xFC:
            m_mnemonic = "CALL M, " + format_address_label(peek_next_word());
            m_ticks = 10;
            m_ticks_alt = 17;
            break;
        case 0xFE:
            m_mnemonic = "CP " + format_hex(peek_next_byte());
            m_ticks = 7;
            break;
        case 0xFF:
            m_mnemonic = "RST 38H";
            m_ticks = 11;
            break;
        }
        address = m_address;
    }
    IndexMode get_index_mode() const { return m_index_mode; }
    std::string get_indexed_reg_str() {
        if (get_index_mode() == IndexMode::IX)
            return "IX";
        if (get_index_mode() == IndexMode::IY)
            return "IY";
        return "HL";
    }
    void set_index_mode(IndexMode mode) { m_index_mode = mode; }
    std::string get_indexed_h_str() {
        if (get_index_mode() == IndexMode::IX)
            return "IXH";
        if (get_index_mode() == IndexMode::IY)
            return "IYH";
        return "H";
    }
    std::string get_indexed_l_str() {
        if (get_index_mode() == IndexMode::IX)
            return "IXL";
        if (get_index_mode() == IndexMode::IY)
            return "IYL";
        return "L";
    }
    std::string get_indexed_addr_str() {
        if (get_index_mode() == IndexMode::HL)
            return "(HL)";
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        std::string index_reg_str = (get_index_mode() == IndexMode::IX) ? "IX" : "IY";
        std::stringstream ss;
        ss << "(" << index_reg_str << (offset >= 0 ? "+" : "") << static_cast<int>(offset) << ")";
        return ss.str();
    }
    std::string get_bytes_str(bool hex_format = true) const {
        std::stringstream ss;
        for (size_t i = 0; i < m_bytes.size(); ++i) {
            if (hex_format)
                ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(m_bytes[i]);
            else
                ss << std::dec << static_cast<int>(m_bytes[i]);
            if (i < m_bytes.size() - 1)
                ss << " ";
        }
        return ss.str();
    }
    std::string format_address_label(uint16_t address, int width = -1, bool hex = true) {
        if constexpr (!std::is_same_v<TLabels, void>) {
            if (m_labels) {
                std::string labels_str = m_labels->get_label(address);
                if (!labels_str.empty()) {
                    return labels_str;
                }
            }
        }
        return hex ? format_hex(address, width) : format_dec(address);
    }
    uint8_t peek_next_byte() {
        uint8_t value = m_memory->peek(m_address++);
        m_bytes.push_back(value);
        return value;
    }
    uint16_t peek_next_word() {
        uint8_t low_byte = peek_next_byte();
        uint8_t high_byte = peek_next_byte();
        return (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    }
    uint8_t peek_next_opcode() {
        return peek_next_byte();
    }
    template <typename T> std::string format_hex(T value, int width = -1) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase;
        if (width > 0)
            ss << std::setw(width) << std::setfill('0');
        ss << static_cast<int>(value);
        return ss.str();
    }
    template <typename T> std::string format_dec(T value) {
        std::stringstream ss;
        ss << std::dec << static_cast<int>(value);
        return ss.str();
    }
    TRegisters* m_registers;
    TMemory* m_memory;
    IndexMode m_index_mode;
    uint16_t m_address;
    std::string m_mnemonic;
    std::vector<uint8_t> m_bytes;
    int m_ticks, m_ticks_alt;
    TLabels* m_labels;
};

class Z80DefaultLabels {
public:
    void clear_labels() {
        m_labels.clear();
    }
    void load_map(const std::string& content) {
        std::stringstream file(content);
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            uint16_t address;
            std::string label, equals;
            ss >> std::hex >> address;
            if (ss.fail() || ss.get() != ':') {
                ss.clear();
                ss.str(line);
                if (ss >> label >> equals && (equals == "=" || equals == "EQU")) {
                    ss >> std::ws;
                    if (ss.peek() == '$')
                        ss.ignore();
                    ss >> std::hex >> address;
                    if (!ss.fail())
                        add_label(address, label);
                }
                continue;
            }
            ss >> label;
            add_label(address, label);
        }
    }
    void load_ctl(const std::string& content) {
        std::stringstream file(content);
        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("L ", 0) == 0) {
                std::stringstream ss(line.substr(2));
                uint16_t address;
                std::string label;
                ss >> std::hex >> address >> label;
                if (!ss.fail() && !label.empty())
                    add_label(address, label);
            } else if (line.rfind("$", 0) == 0 || line.rfind("@ $", 0) == 0) {
                std::stringstream ss(line);
                char prefix;
                uint16_t address;
                std::string keyword;
                if (line.rfind("@ $", 0) == 0)
                    ss.ignore(3);
                else
                    ss.ignore(1);
                ss >> std::hex >> address;
                add_label_from_spectaculator_format(ss, address);
            }
        }
    }
    std::string get_label(uint16_t address) const {
        auto range = m_labels.equal_range(address);
        if (range.first == range.second)
            return "";
        std::string labels_str;
        for (auto it = range.first; it != range.second; ++it) {
            if (!labels_str.empty())
                labels_str += "/";
            labels_str += it->second;
        }
        return labels_str;
    }
    void add_label(uint16_t address, const std::string& label) {
        auto range = m_labels.equal_range(address);
        for (auto it = range.first; it != range.second; ++it)
            if (it->second == label)
                return;
        m_labels.insert({address, label});
        m_reverse_labels[label] = address;
    }

    uint16_t get_addr(const std::string& label) const {
        auto it = m_reverse_labels.find(label);
        if (it != m_reverse_labels.end()) {
            return it->second;
        }
        throw std::runtime_error("Label not found: " + label);
    }

    const std::map<std::string, uint16_t>& get_labels() const { return m_reverse_labels; }

private:
    void add_label_from_spectaculator_format(std::stringstream& ss, uint16_t address) {
        std::string remaining;
        std::getline(ss, remaining);
        constexpr const char* label_keyword = "label=";
        size_t label_pos = remaining.find(label_keyword);
        if (label_pos != std::string::npos) {
            std::string label = remaining.substr(label_pos + strlen(label_keyword));
            label.erase(0, label.find_first_not_of(" \t"));
            label.erase(label.find_last_not_of(" \t") + 1);
            if (!label.empty())
                add_label(address, label);
        }
    }
    std::multimap<uint16_t, std::string> m_labels;
    std::map<std::string, uint16_t> m_reverse_labels;
};

template <typename TMemory, typename TRegisters> class Z80DefaultFiles {
public:
    Z80DefaultFiles(TMemory* memory, TRegisters* registers) : m_memory(memory), m_registers(registers) {}

    bool load_bin_file(const std::vector<uint8_t>& data, uint16_t load_addr) {
        for (size_t i = 0; i < data.size(); ++i) {
            if (load_addr + i > MAX_ADDRESS)
                throw std::runtime_error("Binary file too large.");
            m_memory->poke(load_addr + i, data[i]);
        }
        return true;
    }
    bool load_hex_file(const std::string& content) {
        enum class IntelHexRecordType : uint8_t {
            Data = 0x00,
            EndOfFile = 0x01,
            ExtendedLinearAddress = 0x04,
        };
        std::stringstream file_stream(content);
        std::string line;
        uint32_t extended_linear_address = 0;
        while (std::getline(file_stream, line)) {
            if (line.empty() || line[0] != ':')
                continue;
            try {
                uint8_t byte_count = std::stoul(line.substr(1, 2), nullptr, 16);
                uint16_t address = std::stoul(line.substr(3, 4), nullptr, 16);
                uint8_t record_type = std::stoul(line.substr(7, 2), nullptr, 16);
                if (line.length() < 11 + (size_t)byte_count * 2)
                    throw std::runtime_error("Malformed HEX line (too short): " + line);
                uint8_t checksum = byte_count + (address >> 8) + (address & 0xFF) + record_type;
                std::vector<uint8_t> data_bytes;
                data_bytes.reserve(byte_count);
                for (int i = 0; i < byte_count; ++i) {
                    uint8_t byte = std::stoul(line.substr(9 + i * 2, 2), nullptr, 16);
                    data_bytes.push_back(byte);
                    checksum += byte;
                }
                uint8_t file_checksum = std::stoul(line.substr(9 + byte_count * 2, 2), nullptr, 16);
                if (((checksum + file_checksum) & 0xFF) != 0)
                    throw std::runtime_error("Checksum error in HEX line: " + line);
                switch (static_cast<IntelHexRecordType>(record_type)) {
                case IntelHexRecordType::Data: {
                    uint32_t full_address = extended_linear_address + address;
                    for (size_t i = 0; i < data_bytes.size(); ++i) {
                        if (full_address + i <= MAX_ADDRESS)
                            m_memory->poke(full_address + i, data_bytes[i]);
                    }
                    break;
                }
                case IntelHexRecordType::EndOfFile:
                    return true;
                case IntelHexRecordType::ExtendedLinearAddress:
                    if (byte_count == 2)
                        extended_linear_address = (data_bytes[0] << 24) | (data_bytes[1] << 16);
                    break;
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Could not parse HEX line: " + line + " (" + e.what() + ")");
            }
        }
        return true;
    }
    bool load_sna_file(const std::vector<uint8_t>& data) {
        if (data.size() != SNA_48K_SIZE)
            throw std::runtime_error("Invalid 48K SNA file size. Expected " + std::to_string(SNA_48K_SIZE) + " bytes, got " + std::to_string(data.size()));
        const SNAHeader* header = reinterpret_cast<const SNAHeader*>(data.data());
        typename TRegisters::State state;
        state.m_I = header->I;
        state.m_HLp.w = header->HL_prime;
        state.m_DEp.w = header->DE_prime;
        state.m_BCp.w = header->BC_prime;
        state.m_AFp.w = header->AF_prime;
        state.m_HL.w = header->HL;
        state.m_DE.w = header->DE;
        state.m_BC.w = header->BC;
        state.m_IY.w = header->IY;
        state.m_IX.w = header->IX;
        state.m_IFF2 = (header->IFF2 & 0x04) != 0;
        state.m_IFF1 = state.m_IFF2;
        state.m_R = header->R;
        state.m_AF.w = header->AF;
        state.m_SP.w = header->SP;
        state.m_IRQ_mode = header->InterruptMode;
        for (size_t i = 0; i < ZX48K_RAM_SIZE; ++i)
            m_memory->poke(ZX48K_RAM_START + i, data[sizeof(SNAHeader) + i]);
        state.m_PC.w = m_memory->peek(state.m_SP.w) | (m_memory->peek(state.m_SP.w + 1) << 8);
        state.m_SP.w += 2;
        m_registers->restore_state(state);
        return true;
    }
    bool load_z80_file(const std::vector<uint8_t>& data) {
        if (data.size() < sizeof(Z80HeaderV1))
            throw std::runtime_error("Z80 file is too small.");
        const Z80HeaderV1* header = reinterpret_cast<const Z80HeaderV1*>(data.data());
        typename TRegisters::State state;
        memset(&state, 0, sizeof(state));
        state.m_AF.h = header->A; state.m_AF.l = header->F;
        state.m_BC.w = header->BC;
        state.m_HL.w = header->HL;
        state.m_PC.w = header->PC;
        state.m_SP.w = header->SP;
        state.m_I = header->I; state.m_R = header->R;
        uint8_t byte12 = header->Flags1;
        if (byte12 == 0xFF)
            byte12 = 1;
        state.m_R = (state.m_R & 0x7F) | ((byte12 & 0x01) ? 0x80 : 0);
        bool compressed = (byte12 & 0x20) != 0;
        state.m_DE.w = header->DE;
        state.m_BCp.w = header->BC_prime;
        state.m_DEp.w = header->DE_prime;
        state.m_HLp.w = header->HL_prime;
        state.m_AFp.h = header->A_prime; state.m_AFp.l = header->F_prime;
        state.m_IY.w = header->IY;
        state.m_IX.w = header->IX;
        state.m_IFF1 = header->IFF1 != 0; state.m_IFF2 = header->IFF2 != 0;
        state.m_IRQ_mode = header->Flags2 & 0x03;
        m_registers->restore_state(state);
        if (state.m_PC.w == 0)
            throw std::runtime_error("Z80 v2/v3 files are not supported.");
        size_t data_ptr = sizeof(Z80HeaderV1);
        if (compressed) {
            for (uint16_t mem_addr = ZX48K_RAM_START; data_ptr < data.size() && mem_addr < 0xFFFF;) {
                if (data_ptr + 3 < data.size() && data[data_ptr] == 0xED && data[data_ptr + 1] == 0xED) {
                    uint8_t count = data[data_ptr + 2], value = data[data_ptr + 3];
                    data_ptr += 4;
                    for (int i = 0; i < count && mem_addr < 0xFFFF; ++i)
                        m_memory->poke(mem_addr++, value);
                } else
                    m_memory->poke(mem_addr++, data[data_ptr++]);
            }
        } else {
            for (size_t i = data_ptr; i < data.size(); ++i)
                m_memory->poke(ZX48K_RAM_START + (i - data_ptr), data[i]);
        }
        return true;
    }
private:
    Z80_PUSH_PACK(1)
    struct SNAHeader {
        uint8_t I;
        uint16_t HL_prime, DE_prime, BC_prime, AF_prime;
        uint16_t HL, DE, BC, IY, IX;
        uint8_t IFF2;
        uint8_t R;
        uint16_t AF, SP;
        uint8_t InterruptMode;
        uint8_t BorderColor;
    } Z80_PACKED_STRUCT;
    struct Z80HeaderV1 {
        uint8_t A, F;
        uint16_t BC, HL;
        uint16_t PC, SP;
        uint8_t I, R;
        uint8_t Flags1;
        uint16_t DE, BC_prime, DE_prime, HL_prime;
        uint8_t A_prime, F_prime;
        uint16_t IY, IX;
        uint8_t IFF1, IFF2;
        uint8_t Flags2;
    } Z80_PACKED_STRUCT;
    Z80_POP_PACK()

    TMemory* m_memory;
    TRegisters* m_registers;
public:
    static constexpr size_t ZX48K_RAM_SIZE = 49152;
    static constexpr uint16_t ZX48K_RAM_START = 0x4000;
    static constexpr uint16_t MAX_ADDRESS = 0xFFFF;
    static constexpr size_t SNA_48K_SIZE = sizeof(SNAHeader) + ZX48K_RAM_SIZE;
};

#endif //__Z80ANALYZE_H__