//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄   
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██  
//      ██▀   ██▄  ▄██  ██    ██ 
//    ▄██▀     ██████   ██ ██ ██ 
//   ▄██      ██▀  ▀██  ██    ██ 
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██  
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Analyze.h
// Verson: 1.0
// 
// This file contains the Z80Analyzer class,
// which provides functionality for disassembling Z80 machine code.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#ifndef __Z80ANALYZE_H__
#define __Z80ANALYZE_H__

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

template <class TBus>
class Z80Analyzer {
public:
    Z80Analyzer(TBus& bus) : m_bus(bus) {}

    std::string disassemble(uint16_t& address) {
        return disassemble(address, "%a: %-12b %m");
    }

    //Format specifiers:
    // - `%a`: Address (hex)
    // - `%A`: Address (dec)
    // - `%b`: Instruction bytes (hex)
    // - `%B`: Instruction bytes (dec)
    // - `%m`: Mnemonic
    // supports width (`%10`), alignment (`%-10`), and fill character (`%-10.b` uses `.` instead of space).
    std::string disassemble(uint16_t& address, const std::string& format) {
        uint16_t initial_address = address;
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
                        case 'a': // hex address
                            replacement = format_hex(initial_address, 4);
                            break;
                        case 'A': // dec address
                            replacement = std::to_string(initial_address);
                            break;
                        case 'b': // hex bytes
                            replacement = get_bytes_str(true);
                            break;
                        case 'B': // dec bytes
                            replacement = get_bytes_str(false);
                            break;
                        case 'm': // mnemonic
                            replacement = m_mnemonic;
                            break;
                    }
                    
                    ss << std::setfill(fill_char) << std::setw(width) << (left_align ? std::left : std::right) << replacement;
                    ss << std::setfill(' '); // Reset fill character
                    i = j;
                }
            } else {
                ss << format[i];
            }
        }
        return ss.str();
    }

private:
    void parse_instruction(uint16_t& address) {
        m_address = address;
        m_mnemonic.clear();
        m_bytes.clear();
        set_index_mode(IndexMode::HL);
        uint8_t opcode = peek_next_opcode();
        while (opcode == 0xDD || opcode == 0xFD) {
            set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
            opcode = peek_next_opcode();
        }
        switch (opcode) {
            case 0x00: m_mnemonic = "NOP"; break;
            case 0x01: m_mnemonic = "LD BC, " + format_hex(peek_next_word()); break;
            case 0x02: m_mnemonic = "LD (BC), A"; break;
            case 0x03: m_mnemonic = "INC BC"; break;
            case 0x04: m_mnemonic = "INC B"; break;
            case 0x05: m_mnemonic = "DEC B"; break;
            case 0x06: m_mnemonic = "LD B, " + format_hex(peek_next_byte()); break;
            case 0x07: m_mnemonic = "RLCA"; break;
            case 0x08: m_mnemonic = "EX AF, AF'"; break;
            case 0x09: m_mnemonic = "ADD " + get_indexed_reg_str() + ", BC"; break;
            case 0x0A: m_mnemonic = "LD A, (BC)"; break;
            case 0x0B: m_mnemonic = "DEC BC"; break;
            case 0x0C: m_mnemonic = "INC C"; break;
            case 0x0D: m_mnemonic = "DEC C"; break;
            case 0x0E: m_mnemonic = "LD C, " + format_hex(peek_next_byte()); break;
            case 0x0F: m_mnemonic = "RRCA"; break;
            case 0x10: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "DJNZ " + format_hex(address);
                break;
            }
            case 0x11: m_mnemonic = "LD DE, " + format_hex(peek_next_word()); break;
            case 0x12: m_mnemonic = "LD (DE), A"; break;
            case 0x13: m_mnemonic = "INC DE"; break;
            case 0x14: m_mnemonic = "INC D"; break;
            case 0x15: m_mnemonic = "DEC D"; break;
            case 0x16: m_mnemonic = "LD D, " + format_hex(peek_next_byte()); break;
            case 0x17: m_mnemonic = "RLA"; break;
            case 0x18: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "JR " + format_hex(address);
                break;
            }
            case 0x19: m_mnemonic = "ADD " + get_indexed_reg_str() + ", DE"; break;
            case 0x1A: m_mnemonic = "LD A, (DE)"; break;
            case 0x1B: m_mnemonic = "DEC DE"; break;
            case 0x1C: m_mnemonic = "INC E"; break;
            case 0x1D: m_mnemonic = "DEC E"; break;
            case 0x1E: m_mnemonic = "LD E, " + format_hex(peek_next_byte()); break;
            case 0x1F: m_mnemonic = "RRA"; break;
            case 0x20: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "JR NZ, " + format_hex(address);
                break;
            }
            case 0x21: m_mnemonic = "LD " + get_indexed_reg_str() + ", " + format_hex(peek_next_word()); break;
            case 0x22: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), " + get_indexed_reg_str(); break;
            case 0x23: m_mnemonic = "INC " + get_indexed_reg_str(); break;
            case 0x24: m_mnemonic = "INC " + get_indexed_h_str(); break;
            case 0x25: m_mnemonic = "DEC " + get_indexed_h_str(); break;
            case 0x26: m_mnemonic = "LD " + get_indexed_h_str() + ", " + format_hex(peek_next_byte()); break;
            case 0x27: m_mnemonic = "DAA"; break;
            case 0x28: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "JR Z, " + format_hex(address);
                break;
            }
            case 0x29: {
                std::string reg = get_indexed_reg_str();
                m_mnemonic = "ADD " + reg + ", " + reg;
                break;
            }
            case 0x2A: m_mnemonic = "LD " + get_indexed_reg_str() + ", (" + format_hex(peek_next_word()) + ")"; break;
            case 0x2B: m_mnemonic = "DEC " + get_indexed_reg_str(); break;
            case 0x2C: m_mnemonic = "INC " + get_indexed_l_str(); break;
            case 0x2D: m_mnemonic = "DEC " + get_indexed_l_str(); break;
            case 0x2E: m_mnemonic = "LD " + get_indexed_l_str() + ", " + format_hex(peek_next_byte()); break;
            case 0x2F: m_mnemonic = "CPL"; break;
            case 0x30: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "JR NC, " + format_hex(address);
                break;
            }
            case 0x31: m_mnemonic = "LD SP, " + format_hex(peek_next_word()); break;
            case 0x32: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), A"; break;
            case 0x33: m_mnemonic = "INC SP"; break;
            case 0x34: m_mnemonic = "INC " + get_indexed_addr_str(); break;
            case 0x35: m_mnemonic = "DEC " + get_indexed_addr_str(); break;
            case 0x36: {
                std::string addr_str = get_indexed_addr_str(); 
                std::string val_str = format_hex(peek_next_byte());
                m_mnemonic = "LD " + addr_str + ", " + val_str;
                break;
            }
            case 0x37: m_mnemonic = "SCF"; break;
            case 0x38: {
                int8_t offset = static_cast<int8_t>(peek_next_byte());
                uint16_t address = m_address + offset;
                m_mnemonic = "JR C, " + format_hex(address);
                break;
            }
            case 0x39: m_mnemonic = "ADD " + get_indexed_reg_str() + ", SP"; break;
            case 0x3A: m_mnemonic = "LD A, (" + format_hex(peek_next_word()) + ")"; break;
            case 0x3B: m_mnemonic = "DEC SP"; break;
            case 0x3C: m_mnemonic = "INC A"; break;
            case 0x3D: m_mnemonic = "DEC A"; break;
            case 0x3E: m_mnemonic = "LD A, " + format_hex(peek_next_byte()); break;
            case 0x3F: m_mnemonic = "CCF"; break;
            case 0x40: m_mnemonic = "LD B, B"; break;
            case 0x41: m_mnemonic = "LD B, C"; break;
            case 0x42: m_mnemonic = "LD B, D"; break;
            case 0x43: m_mnemonic = "LD B, E"; break;
            case 0x44: m_mnemonic = "LD B, " + get_indexed_h_str(); break;
            case 0x45: m_mnemonic = "LD B, " + get_indexed_l_str(); break;
            case 0x46: m_mnemonic = "LD B, " + get_indexed_addr_str(); break;
            case 0x47: m_mnemonic = "LD B, A"; break;
            case 0x48: m_mnemonic = "LD C, B"; break;
            case 0x49: m_mnemonic = "LD C, C"; break;
            case 0x4A: m_mnemonic = "LD C, D"; break;
            case 0x4B: m_mnemonic = "LD C, E"; break;
            case 0x4C: m_mnemonic = "LD C, " + get_indexed_h_str(); break;
            case 0x4D: m_mnemonic = "LD C, " + get_indexed_l_str(); break;
            case 0x4E: m_mnemonic = "LD C, " + get_indexed_addr_str(); break;
            case 0x4F: m_mnemonic = "LD C, A"; break;
            case 0x50: m_mnemonic = "LD D, B"; break;
            case 0x51: m_mnemonic = "LD D, C"; break;
            case 0x52: m_mnemonic = "LD D, D"; break;
            case 0x53: m_mnemonic = "LD D, E"; break;
            case 0x54: m_mnemonic = "LD D, " + get_indexed_h_str(); break;
            case 0x55: m_mnemonic = "LD D, " + get_indexed_l_str(); break;
            case 0x56: m_mnemonic = "LD D, " + get_indexed_addr_str(); break;
            case 0x57: m_mnemonic = "LD D, A"; break;
            case 0x58: m_mnemonic = "LD E, B"; break;
            case 0x59: m_mnemonic = "LD E, C"; break;
            case 0x5A: m_mnemonic = "LD E, D"; break;
            case 0x5B: m_mnemonic = "LD E, E"; break;
            case 0x5C: m_mnemonic = "LD E, " + get_indexed_h_str(); break;
            case 0x5D: m_mnemonic = "LD E, " + get_indexed_l_str(); break;
            case 0x5E: m_mnemonic = "LD E, " + get_indexed_addr_str(); break;
            case 0x5F: m_mnemonic = "LD E, A"; break;
            case 0x60: m_mnemonic = "LD H, B"; break;
            case 0x61: m_mnemonic = "LD H, C"; break;
            case 0x62: m_mnemonic = "LD H, D"; break;
            case 0x63: m_mnemonic = "LD H, E"; break;
            case 0x64: {
                std::string reg = get_indexed_h_str();
                m_mnemonic = "LD " + reg + ", " + reg;
                break;
            }
            case 0x65: m_mnemonic = "LD " + get_indexed_h_str() + ", " + get_indexed_l_str(); break;
            case 0x66: m_mnemonic = "LD H, " + get_indexed_addr_str(); break;
            case 0x67: m_mnemonic = "LD " + get_indexed_h_str() + ", A"; break;
            case 0x68: m_mnemonic = "LD " + get_indexed_l_str() + ", B"; break;
            case 0x69: m_mnemonic = "LD " + get_indexed_l_str() + ", C"; break;
            case 0x6A: m_mnemonic = "LD " + get_indexed_l_str() + ", D"; break;
            case 0x6B: m_mnemonic = "LD " + get_indexed_l_str() + ", E"; break;
            case 0x6C: m_mnemonic = "LD " + get_indexed_l_str() + ", " + get_indexed_h_str(); break;
            case 0x6D: {
                std::string reg = get_indexed_l_str();
                m_mnemonic = "LD " + reg + ", " + reg;
                break;
            }
            case 0x6E: m_mnemonic = "LD L, " + get_indexed_addr_str(); break;
            case 0x6F: m_mnemonic = "LD " + get_indexed_l_str() + ", A"; break;
            case 0x70: m_mnemonic = "LD " + get_indexed_addr_str() + ", B"; break;
            case 0x71: m_mnemonic = "LD " + get_indexed_addr_str() + ", C"; break;
            case 0x72: m_mnemonic = "LD " + get_indexed_addr_str() + ", D"; break;
            case 0x73: m_mnemonic = "LD " + get_indexed_addr_str() + ", E"; break;
            case 0x74: m_mnemonic = "LD " + get_indexed_addr_str() + ", H"; break;
            case 0x75: m_mnemonic = "LD " + get_indexed_addr_str() + ", L"; break;
            case 0x76: m_mnemonic = "HALT"; break;
            case 0x77: m_mnemonic = "LD " + get_indexed_addr_str() + ", A"; break;
            case 0x78: m_mnemonic = "LD A, B"; break;
            case 0x79: m_mnemonic = "LD A, C"; break;
            case 0x7A: m_mnemonic = "LD A, D"; break;
            case 0x7B: m_mnemonic = "LD A, E"; break;
            case 0x7C: m_mnemonic = "LD A, " + get_indexed_h_str(); break;
            case 0x7D: m_mnemonic = "LD A, " + get_indexed_l_str(); break;
            case 0x7E: m_mnemonic = "LD A, " + get_indexed_addr_str(); break;
            case 0x7F: m_mnemonic = "LD A, A"; break;
            case 0x80: m_mnemonic = "ADD A, B"; break;
            case 0x81: m_mnemonic = "ADD A, C"; break;
            case 0x82: m_mnemonic = "ADD A, D"; break;
            case 0x83: m_mnemonic = "ADD A, E"; break;
            case 0x84: m_mnemonic = "ADD A, " + get_indexed_h_str(); break;
            case 0x85: m_mnemonic = "ADD A, " + get_indexed_l_str(); break;
            case 0x86: m_mnemonic = "ADD A, " + get_indexed_addr_str(); break;
            case 0x87: m_mnemonic = "ADD A, A"; break;
            case 0x88: m_mnemonic = "ADC A, B"; break;
            case 0x89: m_mnemonic = "ADC A, C"; break;
            case 0x8A: m_mnemonic = "ADC A, D"; break;
            case 0x8B: m_mnemonic = "ADC A, E"; break;
            case 0x8C: m_mnemonic = "ADC A, " + get_indexed_h_str(); break;
            case 0x8D: m_mnemonic = "ADC A, " + get_indexed_l_str(); break;
            case 0x8E: m_mnemonic = "ADC A, " + get_indexed_addr_str(); break;
            case 0x8F: m_mnemonic = "ADC A, A"; break;
            case 0x90: m_mnemonic = "SUB B"; break;
            case 0x91: m_mnemonic = "SUB C"; break;
            case 0x92: m_mnemonic = "SUB D"; break;
            case 0x93: m_mnemonic = "SUB E"; break;
            case 0x94: m_mnemonic = "SUB " + get_indexed_h_str(); break;
            case 0x95: m_mnemonic = "SUB " + get_indexed_l_str(); break;
            case 0x96: m_mnemonic = "SUB " + get_indexed_addr_str(); break;
            case 0x97: m_mnemonic = "SUB A"; break;
            case 0x98: m_mnemonic = "SBC A, B"; break;
            case 0x99: m_mnemonic = "SBC A, C"; break;
            case 0x9A: m_mnemonic = "SBC A, D"; break;
            case 0x9B: m_mnemonic = "SBC A, E"; break;
            case 0x9C: m_mnemonic = "SBC A, " + get_indexed_h_str(); break;
            case 0x9D: m_mnemonic = "SBC A, " + get_indexed_l_str(); break;
            case 0x9E: m_mnemonic = "SBC A, " + get_indexed_addr_str(); break;
            case 0x9F: m_mnemonic = "SBC A, A"; break;
            case 0xA0: m_mnemonic = "AND B"; break;
            case 0xA1: m_mnemonic = "AND C"; break;
            case 0xA2: m_mnemonic = "AND D"; break;
            case 0xA3: m_mnemonic = "AND E"; break;
            case 0xA4: m_mnemonic = "AND " + get_indexed_h_str(); break;
            case 0xA5: m_mnemonic = "AND " + get_indexed_l_str(); break;
            case 0xA6: m_mnemonic = "AND " + get_indexed_addr_str(); break;
            case 0xA7: m_mnemonic = "AND A"; break;
            case 0xA8: m_mnemonic = "XOR B"; break;
            case 0xA9: m_mnemonic = "XOR C"; break;
            case 0xAA: m_mnemonic = "XOR D"; break;
            case 0xAB: m_mnemonic = "XOR E"; break;
            case 0xAC: m_mnemonic = "XOR " + get_indexed_h_str(); break;
            case 0xAD: m_mnemonic = "XOR " + get_indexed_l_str(); break;
            case 0xAE: m_mnemonic = "XOR " + get_indexed_addr_str(); break;
            case 0xAF: m_mnemonic = "XOR A"; break;
            case 0xB0: m_mnemonic = "OR B"; break;
            case 0xB1: m_mnemonic = "OR C"; break;
            case 0xB2: m_mnemonic = "OR D"; break;
            case 0xB3: m_mnemonic = "OR E"; break;
            case 0xB4: m_mnemonic = "OR " + get_indexed_h_str(); break;
            case 0xB5: m_mnemonic = "OR " + get_indexed_l_str(); break;
            case 0xB6: m_mnemonic = "OR " + get_indexed_addr_str(); break;
            case 0xB7: m_mnemonic = "OR A"; break;
            case 0xB8: m_mnemonic = "CP B"; break;
            case 0xB9: m_mnemonic = "CP C"; break;
            case 0xBA: m_mnemonic = "CP D"; break;
            case 0xBB: m_mnemonic = "CP E"; break;
            case 0xBC: m_mnemonic = "CP " + get_indexed_h_str(); break;
            case 0xBD: m_mnemonic = "CP " + get_indexed_l_str(); break;
            case 0xBE: m_mnemonic = "CP " + get_indexed_addr_str(); break;
            case 0xBF: m_mnemonic = "CP A"; break;
            case 0xC0: m_mnemonic = "RET NZ"; break;
            case 0xC1: m_mnemonic = "POP BC"; break;
            case 0xC2: m_mnemonic = "JP NZ, " + format_hex(peek_next_word()); break;
            case 0xC3: m_mnemonic = "JP " + format_hex(peek_next_word()); break;
            case 0xC4: m_mnemonic = "CALL NZ, " + format_hex(peek_next_word()); break;
            case 0xC5: m_mnemonic = "PUSH BC"; break;
            case 0xC6: m_mnemonic = "ADD A, " + format_hex(peek_next_byte()); break;
            case 0xC7: m_mnemonic = "RST 00H"; break;
            case 0xC8: m_mnemonic = "RET Z"; break;
            case 0xC9: m_mnemonic = "RET"; break;
            case 0xCA: m_mnemonic = "JP Z, " + format_hex(peek_next_word()); break;
            case 0xCB:
                if (get_index_mode() == IndexMode::HL)
                {
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
                    } else { // BIT, RES, SET
                        std::stringstream ss;
                        ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << reg_str;
                        m_mnemonic = ss.str();
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
                    } else { // BIT, RES, SET
                        ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << address_str;
                    }

                    if (reg_code != 6) { // For undocumented instructions
                        ss << ", " << registers[reg_code];
                    }
                    m_mnemonic = ss.str();
                }
                break;
            case 0xCC: m_mnemonic = "CALL Z, " + format_hex(peek_next_word()); break;
            case 0xCD: m_mnemonic = "CALL " + format_hex(peek_next_word()); break;
            case 0xCE: m_mnemonic = "ADC A, " + format_hex(peek_next_byte()); break;
            case 0xCF: m_mnemonic = "RST 08H"; break;
            case 0xD0: m_mnemonic = "RET NC"; break;
            case 0xD1: m_mnemonic = "POP DE"; break;
            case 0xD2: m_mnemonic = "JP NC, " + format_hex(peek_next_word()); break;
            case 0xD3: m_mnemonic = "OUT (" + format_hex(peek_next_byte()) + "), A"; break;
            case 0xD4: m_mnemonic = "CALL NC, " + format_hex(peek_next_word()); break;
            case 0xD5: m_mnemonic = "PUSH DE"; break;
            case 0xD6: m_mnemonic = "SUB " + format_hex(peek_next_byte()); break;
            case 0xD7: m_mnemonic = "RST 10H"; break;
            case 0xD8: m_mnemonic = "RET C"; break;
            case 0xD9: m_mnemonic = "EXX"; break;
            case 0xDA: m_mnemonic = "JP C, " + format_hex(peek_next_word()); break;
            case 0xDB: m_mnemonic = "IN A, (" + format_hex(peek_next_byte()) + ")"; break;
            case 0xDC: m_mnemonic = "CALL C, " + format_hex(peek_next_word()); break;
            case 0xDE: m_mnemonic = "SBC A, " + format_hex(peek_next_byte()); break;
            case 0xDF: m_mnemonic = "RST 18H"; break;
            case 0xE0: m_mnemonic = "RET PO"; break;
            case 0xE1: m_mnemonic = "POP " + get_indexed_reg_str(); break;
            case 0xE2: m_mnemonic = "JP PO, " + format_hex(peek_next_word()); break;
            case 0xE3: m_mnemonic = "EX (SP), " + get_indexed_reg_str(); break;
            case 0xE4: m_mnemonic = "CALL PO, " + format_hex(peek_next_word()); break;
            case 0xE5: m_mnemonic = "PUSH HL"; break;
            case 0xE6: m_mnemonic = "AND " + format_hex(peek_next_byte()); break;
            case 0xE7: m_mnemonic = "RST 20H"; break;
            case 0xE8: m_mnemonic = "RET PE"; break;
            case 0xE9: m_mnemonic = "JP (" + get_indexed_reg_str() + ")"; break;
            case 0xEA: m_mnemonic = "JP PE, " + format_hex(peek_next_word()); break;
            case 0xEB: m_mnemonic = "EX DE, HL"; break;
            case 0xEC: m_mnemonic = "CALL PE, " + format_hex(peek_next_word()); break;
            case 0xED: {
                uint8_t opcodeED = peek_next_opcode();
                set_index_mode(IndexMode::HL);
                switch (opcodeED)
                {
                    case 0x40: m_mnemonic = "IN B, (C)"; break;
                    case 0x41: m_mnemonic = "OUT (C), B"; break;
                    case 0x42: m_mnemonic = "SBC HL, BC"; break;
                    case 0x43: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), BC"; break;
                    case 0x44:
                    case 0x4C:
                    case 0x54:
                    case 0x5C:
                    case 0x64:
                    case 0x6C:
                    case 0x74:
                    case 0x7C:
                        m_mnemonic = "NEG";
                        break;
                    case 0x45:
                    case 0x55:
                    case 0x5D:
                    case 0x65:
                    case 0x6D:
                    case 0x75:
                    case 0x7D:
                        m_mnemonic = "RETN";
                        break;
                    case 0x46:
                    case 0x4E:
                    case 0x66:
                    case 0x6E:
                        m_mnemonic = "IM 0";
                        break;
                    case 0x47: m_mnemonic = "LD I, A"; break;
                    case 0x48: m_mnemonic = "IN C, (C)"; break;
                    case 0x49: m_mnemonic = "OUT (C), C"; break;
                    case 0x4A: m_mnemonic = "ADC HL, BC"; break;
                    case 0x4B: m_mnemonic = "LD BC, (" + format_hex(peek_next_word()) + ")"; break;
                    case 0x4D: m_mnemonic = "RETI"; break;
                    case 0x4F: m_mnemonic = "LD R, A"; break;
                    case 0x50: m_mnemonic = "IN D, (C)"; break;
                    case 0x51: m_mnemonic = "OUT (C), D"; break;
                    case 0x52: m_mnemonic = "SBC HL, DE"; break;
                    case 0x53: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), DE"; break;
                    case 0x56: // IM 1
                    case 0x76: // IM 1
                        m_mnemonic = "IM 1";
                        break;
                    case 0x57: m_mnemonic = "LD A, I"; break;
                    case 0x58: m_mnemonic = "IN E, (C)"; break;
                    case 0x59: m_mnemonic = "OUT (C), E"; break;
                    case 0x5A: m_mnemonic = "ADC HL, DE"; break;
                    case 0x5B: m_mnemonic = "LD DE, (" + format_hex(peek_next_word()) + ")"; break;
                    case 0x5E:
                    case 0x7E:
                        m_mnemonic = "IM 2";
                        break;
                    case 0x5F: m_mnemonic = "LD A, R"; break;
                    case 0x60: m_mnemonic = "IN H, (C)"; break;
                    case 0x61: m_mnemonic = "OUT (C), H"; break;
                    case 0x62: m_mnemonic = "SBC HL, HL"; break;
                    case 0x63: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), HL"; break;
                    case 0x67: m_mnemonic = "RRD"; break;
                    case 0x68: m_mnemonic = "IN L, (C)"; break;
                    case 0x69: m_mnemonic = "OUT (C), L"; break;
                    case 0x6A: m_mnemonic = "ADC HL, HL"; break;
                    case 0x6B: m_mnemonic = "LD HL, (" + format_hex(peek_next_word()) + ")"; break;
                    case 0x6F: m_mnemonic = "RLD"; break;
                    case 0x70: m_mnemonic = "IN (C)"; break;
                    case 0x71: m_mnemonic = "OUT (C), 0"; break;
                    case 0x72: m_mnemonic = "SBC HL, SP"; break;
                    case 0x73: m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), SP"; break;
                    case 0x78: m_mnemonic = "IN A, (C)"; break;
                    case 0x79: m_mnemonic = "OUT (C), A"; break;
                    case 0x7A: m_mnemonic = "ADC HL, SP"; break;
                    case 0x7B: m_mnemonic = "LD SP, (" + format_hex(peek_next_word()) + ")"; break;
                    case 0xA0: m_mnemonic = "LDI"; break;
                    case 0xA1: m_mnemonic = "CPI"; break;
                    case 0xA2: m_mnemonic = "INI"; break;
                    case 0xA3: m_mnemonic = "OUTI"; break;
                    case 0xA8: m_mnemonic = "LDD"; break;
                    case 0xA9: m_mnemonic = "CPD"; break;
                    case 0xAA: m_mnemonic = "IND"; break;
                    case 0xAB: m_mnemonic = "OUTD"; break;
                    case 0xB0: m_mnemonic = "LDIR"; break;
                    case 0xB1: m_mnemonic = "CPIR"; break;
                    case 0xB2: m_mnemonic = "INIR"; break;
                    case 0xB3: m_mnemonic = "OTIR"; break;
                    case 0xB8: m_mnemonic = "LDDR"; break;
                    case 0xB9: m_mnemonic = "CPDR"; break;
                    case 0xBA: m_mnemonic = "INDR"; break;
                    case 0xBB: m_mnemonic = "OTDR"; break;
                    default:
                        m_mnemonic = "NOP (DB 0xED, " + format_hex(opcodeED) + ")";
                }
                break;
            }
            case 0xEE: m_mnemonic = "XOR " + format_hex(peek_next_byte()); break;
            case 0xEF: m_mnemonic = "RST 28H"; break;
            case 0xF0: m_mnemonic = "RET P"; break;
            case 0xF1: m_mnemonic = "POP AF"; break;
            case 0xF2: m_mnemonic = "JP P, " + format_hex(peek_next_word()); break;
            case 0xF3: m_mnemonic = "DI"; break;
            case 0xF4: m_mnemonic = "CALL P, " + format_hex(peek_next_word()); break;
            case 0xF5: m_mnemonic = "PUSH AF"; break;
            case 0xF6: m_mnemonic = "OR " + format_hex(peek_next_byte()); break;
            case 0xF7: m_mnemonic = "RST 30H"; break;
            case 0xF8: m_mnemonic = "RET M"; break;
            case 0xF9: m_mnemonic = "LD SP, " + get_indexed_reg_str(); break;
            case 0xFA: m_mnemonic = "JP M, " + format_hex(peek_next_word()); break;
            case 0xFB: m_mnemonic = "EI"; break;
            case 0xFC: m_mnemonic = "CALL M, " + format_hex(peek_next_word()); break;
            case 0xFE: m_mnemonic = "CP " + format_hex(peek_next_byte()); break;
            case 0xFF: m_mnemonic = "RST 38H"; break;
        }
        address = m_address;
    }
    uint16_t m_address;
    std::string m_mnemonic;
    std::vector<uint8_t> m_bytes;

    enum class IndexMode { HL, IX, IY };
    IndexMode m_index_mode;
    
    IndexMode get_index_mode() const { return m_index_mode;}
    std::string get_indexed_reg_str() {
        if (get_index_mode() == IndexMode::IX) return "IX";
        if (get_index_mode() == IndexMode::IY) return "IY";
        return "HL";
    }

    void set_index_mode(IndexMode mode) { m_index_mode = mode; }

    std::string get_indexed_h_str() {
        if (get_index_mode() == IndexMode::IX) return "IXH";
        if (get_index_mode() == IndexMode::IY) return "IYH";
        return "H";
    }

    std::string get_indexed_l_str() {
        if (get_index_mode() == IndexMode::IX) return "IXL";
        if (get_index_mode() == IndexMode::IY) return "IYL";
        return "L";
    }

    std::string get_indexed_addr_str() {
        if (get_index_mode() == IndexMode::HL) return "(HL)";
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        std::string index_reg_str = (get_index_mode() == IndexMode::IX) ? "IX" : "IY";
        std::stringstream ss;
        ss << "(" << index_reg_str << (offset >= 0 ? "+" : "") << static_cast<int>(offset) << ")";
        return ss.str();
    }

    std::string get_bytes_str(bool hex_format = true) const {
        std::stringstream ss;
        for (size_t i = 0; i < m_bytes.size(); ++i) {
            if (hex_format) {
                ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                   << static_cast<int>(m_bytes[i]);
            } else {
                ss << std::dec << static_cast<int>(m_bytes[i]);
            }
            if (i < m_bytes.size() - 1) {
                ss << " ";
            }
        }
        return ss.str();
    }

    uint8_t peek_next_byte() {
        uint8_t value = m_bus.peek(m_address++);
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

    template<typename T>
    std::string format_hex(T value, int width = -1) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase;
        if (width > 0) ss << std::setw(width) << std::setfill('0');
        ss << static_cast<int>(value);
        return ss.str();
    }

    TBus& m_bus;
};

#endif//__Z80ANALYZE_H__