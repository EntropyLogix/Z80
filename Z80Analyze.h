#ifndef __Z80ANALYZE_H__
#define __Z80ANALYZE_H__

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

template <class TBus>
class Z80Disassembler {
public:
    Z80Disassembler(TBus& bus) : m_bus(bus) {}

    void handle_opcode_0x00_NOP() {
        m_mnemonic = "NOP";
    }
    void handle_opcode_0x01_LD_BC_nn() {
        m_mnemonic = "LD BC, " + format_hex(peek_next_word());
    }
    void handle_opcode_0x02_LD_BC_ptr_A() {
        m_mnemonic = "LD (BC), A";
    }
    void handle_opcode_0x03_INC_BC() {
        m_mnemonic = "INC BC";
    }
    void handle_opcode_0x04_INC_B() {
        m_mnemonic = "INC B";
    }
    void handle_opcode_0x05_DEC_B() {
        m_mnemonic = "DEC B";
    }
    void handle_opcode_0x06_LD_B_n() {
        m_mnemonic = "LD B, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x07_RLCA() {
        m_mnemonic = "RLCA";

    }
    void handle_opcode_0x08_EX_AF_AFp() {
        m_mnemonic = "EX AF, AF'";
    }
    void handle_opcode_0x09_ADD_HL_BC() {
        m_mnemonic = "ADD " + get_indexed_reg_str() + ", BC";
    }
    std::string get_indexed_reg_str() {
        if (get_index_mode() == IndexMode::IX) return "IX";
        if (get_index_mode() == IndexMode::IY) return "IY";
        return "HL";
    }
    void handle_opcode_0x0A_LD_A_BC_ptr() {
        m_mnemonic = "LD A, (BC)";
    }
    void handle_opcode_0x0B_DEC_BC() {
        m_mnemonic = "DEC BC";
    }
    void handle_opcode_0x0C_INC_C() {
        m_mnemonic = "INC C";
    }
    void handle_opcode_0x0D_DEC_C() {
        m_mnemonic = "DEC C";
    }
    void handle_opcode_0x0E_LD_C_n() {
        m_mnemonic = "LD C, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x0F_RRCA() {
        m_mnemonic = "RRCA";
    }
    void handle_opcode_0x10_DJNZ_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "DJNZ " + format_hex(address);
    }
    void handle_opcode_0x11_LD_DE_nn() {
        m_mnemonic = "LD DE, " + format_hex(peek_next_word());
    }
    void handle_opcode_0x12_LD_DE_ptr_A() {
        m_mnemonic = "LD (DE), A";
    }
    void handle_opcode_0x13_INC_DE() {
        m_mnemonic = "INC DE";
    }
    void handle_opcode_0x14_INC_D() {
        m_mnemonic = "INC D";
    }
    void handle_opcode_0x15_DEC_D() {
        m_mnemonic = "DEC D";
    }
    void handle_opcode_0x16_LD_D_n() {
        m_mnemonic = "LD D, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x17_RLA() {
        m_mnemonic = "RLA";
    }
    void handle_opcode_0x18_JR_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "JR " + format_hex(address);
    }
    void handle_opcode_0x19_ADD_HL_DE() {
        m_mnemonic = "ADD " + get_indexed_reg_str() + ", DE";
    }
    void handle_opcode_0x1A_LD_A_DE_ptr() {
        m_mnemonic = "LD A, (DE)";
    }
    void handle_opcode_0x1B_DEC_DE() {
        m_mnemonic = "DEC DE";
    }
    void handle_opcode_0x1C_INC_E() {
        m_mnemonic = "INC E";
    }
    void handle_opcode_0x1D_DEC_E() {
        m_mnemonic = "DEC E";
    }
    void handle_opcode_0x1E_LD_E_n() {
        m_mnemonic = "LD E, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x1F_RRA() {
        m_mnemonic = "RRA";
    }
    void handle_opcode_0x20_JR_NZ_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "JR NZ, " + format_hex(address);
    }
    void handle_opcode_0x21_LD_HL_nn() {
        m_mnemonic = "LD " + get_indexed_reg_str() + ", " + format_hex(peek_next_word());
    }
    void handle_opcode_0x22_LD_nn_ptr_HL() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), " + get_indexed_reg_str();
    }
    void handle_opcode_0x23_INC_HL() {
        m_mnemonic = "INC " + get_indexed_reg_str();
    }
    void handle_opcode_0x24_INC_H() {
        m_mnemonic = "INC " + get_indexed_h_str();
    }
    void handle_opcode_0x25_DEC_H() {
        m_mnemonic = "DEC " + get_indexed_h_str();
    }
    void handle_opcode_0x26_LD_H_n() {
        m_mnemonic = "LD " + get_indexed_h_str() + ", " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x27_DAA() {
        m_mnemonic = "DAA";
    }
    void handle_opcode_0x28_JR_Z_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "JR Z, " + format_hex(address);
    }
    void handle_opcode_0x29_ADD_HL_HL() {
        std::string reg = get_indexed_reg_str();
        m_mnemonic = "ADD " + reg + ", " + reg;
    }
    void handle_opcode_0x2A_LD_HL_nn_ptr() {
        m_mnemonic = "LD " + get_indexed_reg_str() + ", (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0x2B_DEC_HL() {
        m_mnemonic = "DEC " + get_indexed_reg_str();
    }
    void handle_opcode_0x2C_INC_L() {
        m_mnemonic = "INC " + get_indexed_l_str();
    }
    void handle_opcode_0x2D_DEC_L() {
        m_mnemonic = "DEC " + get_indexed_l_str();
    }
    void handle_opcode_0x2E_LD_L_n() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x2F_CPL() {
        m_mnemonic = "CPL";
    }
    void handle_opcode_0x30_JR_NC_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "JR NC, " + format_hex(address);
    }
    void handle_opcode_0x31_LD_SP_nn() {
        m_mnemonic = "LD SP, " + format_hex(peek_next_word());
    }
    void handle_opcode_0x32_LD_nn_ptr_A() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), A";
    }
    void handle_opcode_0x33_INC_SP() {
        m_mnemonic = "INC SP";
    }
    void handle_opcode_0x34_INC_HL_ptr() {
        m_mnemonic = "INC " + get_indexed_addr_str();
    }
    void handle_opcode_0x35_DEC_HL_ptr() {
        m_mnemonic = "DEC " + get_indexed_addr_str();
    }
    void handle_opcode_0x36_LD_HL_ptr_n() {
        // Ensure offset 'd' is read before immediate value 'n'
        std::string addr_str = get_indexed_addr_str(); 
        std::string val_str = format_hex(peek_next_byte());
        m_mnemonic = "LD " + addr_str + ", " + val_str;
    }
    void handle_opcode_0x37_SCF() {
        m_mnemonic = "SCF";
    }
    void handle_opcode_0x38_JR_C_d() {
        int8_t offset = static_cast<int8_t>(peek_next_byte());
        uint16_t address = m_address + 1 + offset;
        m_mnemonic = "JR C, " + format_hex(address);
    }
    void handle_opcode_0x39_ADD_HL_SP() {
        m_mnemonic = "ADD " + get_indexed_reg_str() + ", SP";
    }
    void handle_opcode_0x3A_LD_A_nn_ptr() {
        m_mnemonic = "LD A, (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0x3B_DEC_SP() {
        m_mnemonic = "DEC SP";
    }
    void handle_opcode_0x3C_INC_A() {
        m_mnemonic = "INC A";
    }
    void handle_opcode_0x3D_DEC_A() {
        m_mnemonic = "DEC A";
    }
    void handle_opcode_0x3E_LD_A_n() {
        m_mnemonic = "LD A, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0x3F_CCF() {
        m_mnemonic = "CCF";
    }
    void handle_opcode_0x40_LD_B_B() {
        m_mnemonic = "LD B, B";
    }
    void handle_opcode_0x41_LD_B_C() {
        m_mnemonic = "LD B, C";
    }
    void handle_opcode_0x42_LD_B_D() {
        m_mnemonic = "LD B, D";
    }
    void handle_opcode_0x43_LD_B_E() {
        m_mnemonic = "LD B, E";
    }
    void handle_opcode_0x44_LD_B_H() {
        m_mnemonic = "LD B, " + get_indexed_h_str();
    }
    void handle_opcode_0x45_LD_B_L() {
        m_mnemonic = "LD B, " + get_indexed_l_str();
    }
    void handle_opcode_0x46_LD_B_HL_ptr() {
        m_mnemonic = "LD B, " + get_indexed_addr_str();
    }
    void handle_opcode_0x47_LD_B_A() {
        m_mnemonic = "LD B, A";
    }
    void handle_opcode_0x48_LD_C_B() {
        m_mnemonic = "LD C, B";
    }
    void handle_opcode_0x49_LD_C_C() {
        m_mnemonic = "LD C, C";
    }
    void handle_opcode_0x4A_LD_C_D() {
        m_mnemonic = "LD C, D";
    }
    void handle_opcode_0x4B_LD_C_E() {
        m_mnemonic = "LD C, E";
    }
    void handle_opcode_0x4C_LD_C_H() {
        m_mnemonic = "LD C, " + get_indexed_h_str();
    }
    void handle_opcode_0x4D_LD_C_L() {
        m_mnemonic = "LD C, " + get_indexed_l_str();
    }
    void handle_opcode_0x4E_LD_C_HL_ptr() {
        m_mnemonic = "LD C, " + get_indexed_addr_str();
    }
    void handle_opcode_0x4F_LD_C_A() {
        m_mnemonic = "LD C, A";
    }
    void handle_opcode_0x50_LD_D_B() {
        m_mnemonic = "LD D, B";
    }
    void handle_opcode_0x51_LD_D_C() {
        m_mnemonic = "LD D, C";
    }
    void handle_opcode_0x52_LD_D_D() {
        m_mnemonic = "LD D, D";
    }
    void handle_opcode_0x53_LD_D_E() {
        m_mnemonic = "LD D, E";
    }
    void handle_opcode_0x54_LD_D_H() {
        m_mnemonic = "LD D, " + get_indexed_h_str();
    }
    void handle_opcode_0x55_LD_D_L() {
        m_mnemonic = "LD D, " + get_indexed_l_str();
    }
    void handle_opcode_0x56_LD_D_HL_ptr() {
        m_mnemonic = "LD D, " + get_indexed_addr_str();
    }
    void handle_opcode_0x57_LD_D_A() {
        m_mnemonic = "LD D, A";
    }
    void handle_opcode_0x58_LD_E_B() {
        m_mnemonic = "LD E, B";
    }
    void handle_opcode_0x59_LD_E_C() {
        m_mnemonic = "LD E, C";
    }
    void handle_opcode_0x5A_LD_E_D() {
        m_mnemonic = "LD E, D";
    }
    void handle_opcode_0x5B_LD_E_E() {
        m_mnemonic = "LD E, E";
    }
    void handle_opcode_0x5C_LD_E_H() {
        m_mnemonic = "LD E, " + get_indexed_h_str();
    }
    void handle_opcode_0x5D_LD_E_L() {
        m_mnemonic = "LD E, " + get_indexed_l_str();
    }
    void handle_opcode_0x5E_LD_E_HL_ptr() {
        m_mnemonic = "LD E, " + get_indexed_addr_str();
    }
    void handle_opcode_0x5F_LD_E_A() {
        m_mnemonic = "LD E, A";
    }
    void handle_opcode_0x60_LD_H_B() {
        m_mnemonic = "LD H, B";
    }
    void handle_opcode_0x61_LD_H_C() {
        m_mnemonic = "LD H, C";
    }
    void handle_opcode_0x62_LD_H_D() {
        m_mnemonic = "LD H, D";
    }
    void handle_opcode_0x63_LD_H_E() {
        m_mnemonic = "LD H, E";
    }
    void handle_opcode_0x64_LD_H_H() {
        std::string reg = get_indexed_h_str();
        m_mnemonic = "LD " + reg + ", " + reg;
    }
    void handle_opcode_0x65_LD_H_L() {
        m_mnemonic = "LD " + get_indexed_h_str() + ", " + get_indexed_l_str();
    }
    void handle_opcode_0x66_LD_H_HL_ptr() {
        m_mnemonic = "LD H, " + get_indexed_addr_str();
    }
    void handle_opcode_0x67_LD_H_A() {
        m_mnemonic = "LD " + get_indexed_h_str() + ", A";
    }
    void handle_opcode_0x68_LD_L_B() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", B";
    }
    void handle_opcode_0x69_LD_L_C() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", C";
    }
    void handle_opcode_0x6A_LD_L_D() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", D";
    }
    void handle_opcode_0x6B_LD_L_E() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", E";
    }
    void handle_opcode_0x6C_LD_L_H() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", " + get_indexed_h_str();
    }
    void handle_opcode_0x6D_LD_L_L() {
        std::string reg = get_indexed_l_str();
        m_mnemonic = "LD " + reg + ", " + reg;
    }
    void handle_opcode_0x6E_LD_L_HL_ptr() {
        m_mnemonic = "LD L, " + get_indexed_addr_str();
    }
    void handle_opcode_0x6F_LD_L_A() {
        m_mnemonic = "LD " + get_indexed_l_str() + ", A";
    }
    void handle_opcode_0x70_LD_HL_ptr_B() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", B";
    }
    void handle_opcode_0x71_LD_HL_ptr_C() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", C";
    }
    void handle_opcode_0x72_LD_HL_ptr_D() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", D";
    }
    void handle_opcode_0x73_LD_HL_ptr_E() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", E";
    }
    void handle_opcode_0x74_LD_HL_ptr_H() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", H";
    }
    void handle_opcode_0x75_LD_HL_ptr_L() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", L";
    }
    void handle_opcode_0x76_HALT() {
        m_mnemonic = "HALT";
    }
    void handle_opcode_0x77_LD_HL_ptr_A() {
        m_mnemonic = "LD " + get_indexed_addr_str() + ", A";
    }
    void handle_opcode_0x78_LD_A_B() {
        m_mnemonic = "LD A, B";
    }
    void handle_opcode_0x79_LD_A_C() {
        m_mnemonic = "LD A, C";
    }
    void handle_opcode_0x7A_LD_A_D() {
        m_mnemonic = "LD A, D";
    }
    void handle_opcode_0x7B_LD_A_E() {
        m_mnemonic = "LD A, E";
    }
    void handle_opcode_0x7C_LD_A_H() {
        m_mnemonic = "LD A, " + get_indexed_h_str();
    }
    void handle_opcode_0x7D_LD_A_L() {
        m_mnemonic = "LD A, " + get_indexed_l_str();
    }
    void handle_opcode_0x7E_LD_A_HL_ptr() {
        m_mnemonic = "LD A, " + get_indexed_addr_str();
    }
    void handle_opcode_0x7F_LD_A_A() {
        m_mnemonic = "LD A, A";
    }
    void handle_opcode_0x80_ADD_A_B() {
        m_mnemonic = "ADD A, B";
    }
    void handle_opcode_0x81_ADD_A_C() {
        m_mnemonic = "ADD A, C";
    }
    void handle_opcode_0x82_ADD_A_D() {
        m_mnemonic = "ADD A, D";
    }
    void handle_opcode_0x83_ADD_A_E() {
        m_mnemonic = "ADD A, E";
    }
    void handle_opcode_0x84_ADD_A_H() {
        m_mnemonic = "ADD A, " + get_indexed_h_str();
    }
    void handle_opcode_0x85_ADD_A_L() {
        m_mnemonic = "ADD A, " + get_indexed_l_str();
    }
    void handle_opcode_0x86_ADD_A_HL_ptr() {
        m_mnemonic = "ADD A, " + get_indexed_addr_str();
    }
    void handle_opcode_0x87_ADD_A_A() {
        m_mnemonic = "ADD A, A";
    }
    void handle_opcode_0x88_ADC_A_B() {
        m_mnemonic = "ADC A, B";
    }
    void handle_opcode_0x89_ADC_A_C() {
        m_mnemonic = "ADC A, C";
    }
    void handle_opcode_0x8A_ADC_A_D() {
        m_mnemonic = "ADC A, D";
    }
    void handle_opcode_0x8B_ADC_A_E() {
        m_mnemonic = "ADC A, E";
    }
    void handle_opcode_0x8C_ADC_A_H() {
        m_mnemonic = "ADC A, " + get_indexed_h_str();
    }
    void handle_opcode_0x8D_ADC_A_L() {
        m_mnemonic = "ADC A, " + get_indexed_l_str();
    }
    void handle_opcode_0x8E_ADC_A_HL_ptr() {
        m_mnemonic = "ADC A, " + get_indexed_addr_str();
    }
    void handle_opcode_0x8F_ADC_A_A() {
        m_mnemonic = "ADC A, A";
    }
    void handle_opcode_0x90_SUB_B() {
        m_mnemonic = "SUB B";
    }
    void handle_opcode_0x91_SUB_C() {
        m_mnemonic = "SUB C";
    }
    void handle_opcode_0x92_SUB_D() {
        m_mnemonic = "SUB D";
    }
    void handle_opcode_0x93_SUB_E() {
        m_mnemonic = "SUB E";
    }
    void handle_opcode_0x94_SUB_H() {
        m_mnemonic = "SUB " + get_indexed_h_str();
    }
    void handle_opcode_0x95_SUB_L() {
        m_mnemonic = "SUB " + get_indexed_l_str();
    }
    void handle_opcode_0x96_SUB_HL_ptr() {
        m_mnemonic = "SUB " + get_indexed_addr_str();
    }
    void handle_opcode_0x97_SUB_A() {
        m_mnemonic = "SUB A";
    }
    void handle_opcode_0x98_SBC_A_B() {
        m_mnemonic = "SBC A, B";
    }
    void handle_opcode_0x99_SBC_A_C() {
        m_mnemonic = "SBC A, C";
    }
    void handle_opcode_0x9A_SBC_A_D() {
        m_mnemonic = "SBC A, D";
    }
    void handle_opcode_0x9B_SBC_A_E() {
        m_mnemonic = "SBC A, E";
    }
    void handle_opcode_0x9C_SBC_A_H() {
        m_mnemonic = "SBC A, " + get_indexed_h_str();
    }
    void handle_opcode_0x9D_SBC_A_L() {
        m_mnemonic = "SBC A, " + get_indexed_l_str();
    }
    void handle_opcode_0x9E_SBC_A_HL_ptr() {
        m_mnemonic = "SBC A, " + get_indexed_addr_str();
    }
    void handle_opcode_0x9F_SBC_A_A() {
        m_mnemonic = "SBC A, A";
    }
    void handle_opcode_0xA0_AND_B() {
        m_mnemonic = "AND B";
    }
    void handle_opcode_0xA1_AND_C() {
        m_mnemonic = "AND C";
    }
    void handle_opcode_0xA2_AND_D() {
        m_mnemonic = "AND D";
    }
    void handle_opcode_0xA3_AND_E() {
        m_mnemonic = "AND E";
    }
    void handle_opcode_0xA4_AND_H() {
        m_mnemonic = "AND " + get_indexed_h_str();
    }
    void handle_opcode_0xA5_AND_L() {
        m_mnemonic = "AND " + get_indexed_l_str();
    }
    void handle_opcode_0xA6_AND_HL_ptr() {
        m_mnemonic = "AND " + get_indexed_addr_str();
    }
    void handle_opcode_0xA7_AND_A() {
        m_mnemonic = "AND A";
    }
    void handle_opcode_0xA8_XOR_B() {
        m_mnemonic = "XOR B";
    }
    void handle_opcode_0xA9_XOR_C() {
        m_mnemonic = "XOR C";
    }
    void handle_opcode_0xAA_XOR_D() {
        m_mnemonic = "XOR D";
    }
    void handle_opcode_0xAB_XOR_E() {
        m_mnemonic = "XOR E";
    }
    void handle_opcode_0xAC_XOR_H() {
        m_mnemonic = "XOR " + get_indexed_h_str();
    }
    void handle_opcode_0xAD_XOR_L() {
        m_mnemonic = "XOR " + get_indexed_l_str();
    }
    void handle_opcode_0xAE_XOR_HL_ptr() {
        m_mnemonic = "XOR " + get_indexed_addr_str();
    }
    void handle_opcode_0xAF_XOR_A() {
        m_mnemonic = "XOR A";
    }
    void handle_opcode_0xB0_OR_B() {
        m_mnemonic = "OR B";
    }
    void handle_opcode_0xB1_OR_C() {
        m_mnemonic = "OR C";
    }
    void handle_opcode_0xB2_OR_D() {
        m_mnemonic = "OR D";
    }
    void handle_opcode_0xB3_OR_E() {
        m_mnemonic = "OR E";
    }
    void handle_opcode_0xB4_OR_H() {
        m_mnemonic = "OR " + get_indexed_h_str();
    }
    void handle_opcode_0xB5_OR_L() {
        m_mnemonic = "OR " + get_indexed_l_str();
    }
    void handle_opcode_0xB6_OR_HL_ptr() {
        m_mnemonic = "OR " + get_indexed_addr_str();
    }
    void handle_opcode_0xB7_OR_A() {
        m_mnemonic = "OR A";
    }
    void handle_opcode_0xB8_CP_B() {
        m_mnemonic = "CP B";
    }
    void handle_opcode_0xB9_CP_C() {
        m_mnemonic = "CP C";
    }
    void handle_opcode_0xBA_CP_D() {
        m_mnemonic = "CP D";
    }
    void handle_opcode_0xBB_CP_E() {
        m_mnemonic = "CP E";
    }
    void handle_opcode_0xBC_CP_H() {
        m_mnemonic = "CP " + get_indexed_h_str();
    }
    void handle_opcode_0xBD_CP_L() {
        m_mnemonic = "CP " + get_indexed_l_str();
    }
    void handle_opcode_0xBE_CP_HL_ptr() {
        m_mnemonic = "CP " + get_indexed_addr_str();
    }
    void handle_opcode_0xBF_CP_A() {
        m_mnemonic = "CP A";
    }
    void handle_opcode_0xC0_RET_NZ() {
        m_mnemonic = "RET NZ";
    }
    void handle_opcode_0xC1_POP_BC() {
        m_mnemonic = "POP BC";
    }
    void handle_opcode_0xC2_JP_NZ_nn() {
        m_mnemonic = "JP NZ, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xC3_JP_nn() {
        m_mnemonic = "JP " + format_hex(peek_next_word());
    }
    void handle_opcode_0xC4_CALL_NZ_nn() {
        m_mnemonic = "CALL NZ, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xC5_PUSH_BC() {
        m_mnemonic = "PUSH BC";
    }
    void handle_opcode_0xC6_ADD_A_n() {
        m_mnemonic = "ADD A, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xC7_RST_00H() {
        m_mnemonic = "RST 00H";
    }
    void handle_opcode_0xC8_RET_Z() {
        m_mnemonic = "RET Z";
    }
    void handle_opcode_0xC9_RET() {
        m_mnemonic = "RET";
    }
    void handle_opcode_0xCA_JP_Z_nn() {
        m_mnemonic = "JP Z, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xCC_CALL_Z_nn() {
        m_mnemonic = "CALL Z, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xCD_CALL_nn() {
        m_mnemonic = "CALL " + format_hex(peek_next_word());
    }
    void handle_opcode_0xCE_ADC_A_n() {
        m_mnemonic = "ADC A, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xCF_RST_08H() {
        m_mnemonic = "RST 08H";
    }
    void handle_opcode_0xD0_RET_NC() {
        m_mnemonic = "RET NC";
    }
    void handle_opcode_0xD1_POP_DE() {
        m_mnemonic = "POP DE";
    }
    void handle_opcode_0xD2_JP_NC_nn() {
        m_mnemonic = "JP NC, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xD3_OUT_n_ptr_A() {
        m_mnemonic = "OUT (" + format_hex(peek_next_byte()) + "), A";
    }
    void handle_opcode_0xD4_CALL_NC_nn() {
        m_mnemonic = "CALL NC, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xD5_PUSH_DE() {
        m_mnemonic = "PUSH DE";
    }
    void handle_opcode_0xD6_SUB_n() {
        m_mnemonic = "SUB " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xD7_RST_10H() {
        m_mnemonic = "RST 10H";
    }
    void handle_opcode_0xD8_RET_C() {
        m_mnemonic = "RET C";
    }
    void handle_opcode_0xD9_EXX() {
        m_mnemonic = "EXX";
    }
    void handle_opcode_0xDA_JP_C_nn() {
        m_mnemonic = "JP C, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xDB_IN_A_n_ptr() {
        m_mnemonic = "IN A, (" + format_hex(peek_next_byte()) + ")";
    }
    void handle_opcode_0xDC_CALL_C_nn() {
        m_mnemonic = "CALL C, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xDE_SBC_A_n() {
        m_mnemonic = "SBC A, " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xDF_RST_18H() {
        m_mnemonic = "RST 18H";
    }
    void handle_opcode_0xE0_RET_PO() {
        m_mnemonic = "RET PO";
    }
    void handle_opcode_0xE1_POP_HL() {
        m_mnemonic = "POP " + get_indexed_reg_str();
    }
    void handle_opcode_0xE2_JP_PO_nn() {
        m_mnemonic = "JP PO, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xE3_EX_SP_ptr_HL() {
        m_mnemonic = "EX (SP), " + get_indexed_reg_str();
    }
    void handle_opcode_0xE4_CALL_PO_nn() {
        m_mnemonic = "CALL PO, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xE5_PUSH_HL() {
        m_mnemonic = "PUSH HL";
    }
    void handle_opcode_0xE6_AND_n() {
        m_mnemonic = "AND " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xE7_RST_20H() {
        m_mnemonic = "RST 20H";
    }
    void handle_opcode_0xE8_RET_PE() {
        m_mnemonic = "RET PE";
    }
    void handle_opcode_0xE9_JP_HL_ptr() {
        m_mnemonic = "JP (" + get_indexed_reg_str() + ")";
    }
    void handle_opcode_0xEA_JP_PE_nn() {
        m_mnemonic = "JP PE, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xEB_EX_DE_HL() {
        m_mnemonic = "EX DE, HL";
    }
    void handle_opcode_0xEC_CALL_PE_nn() {
        m_mnemonic = "CALL PE, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xEE_XOR_n() {
        m_mnemonic = "XOR " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xEF_RST_28H() {
        m_mnemonic = "RST 28H";
    }
    void handle_opcode_0xF0_RET_P() {
        m_mnemonic = "RET P";
    }
    void handle_opcode_0xF1_POP_AF() {
        m_mnemonic = "POP AF";
    }
    void handle_opcode_0xF2_JP_P_nn() {
        m_mnemonic = "JP P, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xF3_DI() {
        m_mnemonic = "DI";
    }
    void handle_opcode_0xF4_CALL_P_nn() {
        m_mnemonic = "CALL P, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xF5_PUSH_AF() {
        m_mnemonic = "PUSH AF";
    }
    void handle_opcode_0xF6_OR_n() {
        m_mnemonic = "OR " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xF7_RST_30H() {
        m_mnemonic = "RST 30H";
    }
    void handle_opcode_0xF8_RET_M() {
        m_mnemonic = "RET M";
    }
    void handle_opcode_0xF9_LD_SP_HL() {
        m_mnemonic = "LD SP, " + get_indexed_reg_str();
    }
    void handle_opcode_0xFA_JP_M_nn() {
        m_mnemonic = "JP M, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xFB_EI() {
        m_mnemonic = "EI";
    }
    void handle_opcode_0xFC_CALL_M_nn() {
        m_mnemonic = "CALL M, " + format_hex(peek_next_word());
    }
    void handle_opcode_0xFE_CP_n() {
        m_mnemonic = "CP " + format_hex(peek_next_byte());
    }
    void handle_opcode_0xFF_RST_38H() {
        m_mnemonic = "RST 38H";
    }
    void handle_opcode_0xED_0x40_IN_B_C_ptr() {
        m_mnemonic = "IN B, (C)";
    }
    void handle_opcode_0xED_0x41_OUT_C_ptr_B() {
        m_mnemonic = "OUT (C), B";
    }
    void handle_opcode_0xED_0x42_SBC_HL_BC() {
        m_mnemonic = "SBC HL, BC";
    }
    void handle_opcode_0xED_0x43_LD_nn_ptr_BC() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), BC";
    }
    void handle_opcode_0xED_0x44_NEG() {
        m_mnemonic = "NEG";
    }
    void handle_opcode_0xED_0x45_RETN() {
        m_mnemonic = "RETN";
    }
    void handle_opcode_0xED_0x46_IM_0() {
        m_mnemonic = "IM 0";
    }
    void handle_opcode_0xED_0x47_LD_I_A() {
        m_mnemonic = "LD I, A";
    }
    void handle_opcode_0xED_0x48_IN_C_C_ptr() {
        m_mnemonic = "IN C, (C)";
    }
    void handle_opcode_0xED_0x49_OUT_C_ptr_C() {
        m_mnemonic = "OUT (C), C";
    }
    void handle_opcode_0xED_0x4A_ADC_HL_BC() {
        m_mnemonic = "ADC HL, BC";
    }
    void handle_opcode_0xED_0x4B_LD_BC_nn_ptr() {
        m_mnemonic = "LD BC, (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0xED_0x4D_RETI() {
        m_mnemonic = "RETI";
    }
    void handle_opcode_0xED_0x4F_LD_R_A() {
        m_mnemonic = "LD R, A";
    }
    void handle_opcode_0xED_0x50_IN_D_C_ptr() {
        m_mnemonic = "IN D, (C)";
    }
    void handle_opcode_0xED_0x51_OUT_C_ptr_D() {
        m_mnemonic = "OUT (C), D";
    }
    void handle_opcode_0xED_0x52_SBC_HL_DE() {
        m_mnemonic = "SBC HL, DE";
    }
    void handle_opcode_0xED_0x53_LD_nn_ptr_DE() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), DE";
    }
    void handle_opcode_0xED_0x56_IM_1() {
        m_mnemonic = "IM 1";
    }
    void handle_opcode_0xED_0x57_LD_A_I() {
        m_mnemonic = "LD A, I";
    }
    void handle_opcode_0xED_0x58_IN_E_C_ptr() {
        m_mnemonic = "IN E, (C)";
    }
    void handle_opcode_0xED_0x59_OUT_C_ptr_E() {
        m_mnemonic = "OUT (C), E";
    }
    void handle_opcode_0xED_0x5A_ADC_HL_DE() {
        m_mnemonic = "ADC HL, DE";
    }
    void handle_opcode_0xED_0x5B_LD_DE_nn_ptr() {
        m_mnemonic = "LD DE, (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0xED_0x5E_IM_2() {
        m_mnemonic = "IM 2";
    }
    void handle_opcode_0xED_0x5F_LD_A_R() {
        m_mnemonic = "LD A, R";
    }
    void handle_opcode_0xED_0x60_IN_H_C_ptr() {
        m_mnemonic = "IN H, (C)";
    }
    void handle_opcode_0xED_0x61_OUT_C_ptr_H() {
        m_mnemonic = "OUT (C), H";
    }
    void handle_opcode_0xED_0x62_SBC_HL_HL() {
        m_mnemonic = "SBC HL, HL";
    }
    void handle_opcode_0xED_0x63_LD_nn_ptr_HL_ED() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), HL";
    }
    void handle_opcode_0xED_0x67_RRD() {
        m_mnemonic = "RRD";
    }
    void handle_opcode_0xED_0x68_IN_L_C_ptr() {
        m_mnemonic = "IN L, (C)";
    }
    void handle_opcode_0xED_0x69_OUT_C_ptr_L() {
        m_mnemonic = "OUT (C), L";
    }
    void handle_opcode_0xED_0x6A_ADC_HL_HL() {
        m_mnemonic = "ADC HL, HL";
    }
    void handle_opcode_0xED_0x6B_LD_HL_nn_ptr_ED() {
        m_mnemonic = "LD HL, (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0xED_0x6F_RLD() {
        m_mnemonic = "RLD";
    }
    void handle_opcode_0xED_0x70_IN_C_ptr() {
        m_mnemonic = "IN (C)";
    }
    void handle_opcode_0xED_0x71_OUT_C_ptr_0() {
        m_mnemonic = "OUT (C), 0";
    }
    void handle_opcode_0xED_0x72_SBC_HL_SP() {
        m_mnemonic = "SBC HL, SP";
    }
    void handle_opcode_0xED_0x73_LD_nn_ptr_SP() {
        m_mnemonic = "LD (" + format_hex(peek_next_word()) + "), SP";
    }
    void handle_opcode_0xED_0x78_IN_A_C_ptr() {
        m_mnemonic = "IN A, (C)";
    }
    void handle_opcode_0xED_0x79_OUT_C_ptr_A() {
        m_mnemonic = "OUT (C), A";
    }
    void handle_opcode_0xED_0x7A_ADC_HL_SP() {
        m_mnemonic = "ADC HL, SP";
    }
    void handle_opcode_0xED_0x7B_LD_SP_nn_ptr() {
        m_mnemonic = "LD SP, (" + format_hex(peek_next_word()) + ")";
    }
    void handle_opcode_0xED_0xA0_LDI() {
        m_mnemonic = "LDI";
    }
    void handle_opcode_0xED_0xA1_CPI() {
        m_mnemonic = "CPI";
    }
    void handle_opcode_0xED_0xA2_INI() {
        m_mnemonic = "INI";
    }
    void handle_opcode_0xED_0xA3_OUTI() {
        m_mnemonic = "OUTI";
    }
    void handle_opcode_0xED_0xA8_LDD() {
        m_mnemonic = "LDD";
    }
    void handle_opcode_0xED_0xA9_CPD() {
        m_mnemonic = "CPD";
    }
    void handle_opcode_0xED_0xAA_IND() {
        m_mnemonic = "IND";
    }
    void handle_opcode_0xED_0xAB_OUTD() {
        m_mnemonic = "OUTD";
    }
    void handle_opcode_0xED_0xB0_LDIR() {
        m_mnemonic = "LDIR";
    }
    void handle_opcode_0xED_0xB1_CPIR() {
        m_mnemonic = "CPIR";
    }
    void handle_opcode_0xED_0xB2_INIR() {
        m_mnemonic = "INIR";
    }
    void handle_opcode_0xED_0xB3_OTIR() {
        m_mnemonic = "OTIR";
    }
    void handle_opcode_0xED_0xB8_LDDR() {
        m_mnemonic = "LDDR";
    }
    void handle_opcode_0xED_0xB9_CPDR() {
        m_mnemonic = "CPDR";
    }
    void handle_opcode_0xED_0xBA_INDR() {
        m_mnemonic = "INDR";
    }
    void handle_opcode_0xED_0xBB_OTDR() {
        m_mnemonic = "OTDR";
    }

void handle_CB_opcodes(uint8_t opcode) {
    const char* registers[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    const char* operations[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
    const char* bit_ops[] = {"BIT", "RES", "SET"};

    uint8_t operation_group = opcode >> 6;
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t reg_code = opcode & 0x07;

    std::string reg_str = registers[reg_code];

    if (operation_group == 0) { // Rotates/Shifts
        m_mnemonic = std::string(operations[bit]) + " " + reg_str;
    } else { // BIT, RES, SET
        std::stringstream ss;
        ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << reg_str;
        m_mnemonic = ss.str();
    }
}
    void handle_CB_indexed_opcodes(int8_t offset, uint8_t opcode) {
        const char* operations[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
        const char* bit_ops[] = {"BIT", "RES", "SET"};
        const char* registers[] = {"B", "C", "D", "E", "H", "L", "", "A"};

        std::string index_reg_str = (get_index_mode() == IndexMode::IX) ? "IX" : "IY";
        std::stringstream address_ss;
        address_ss << "(" << index_reg_str << (offset >= 0 ? "+" : "") << static_cast<int>(offset) << ")";
        std::string address_str = address_ss.str();

        uint8_t operation_group = opcode >> 6;
        uint8_t bit = (opcode >> 3) & 0x07;
        uint8_t reg_code = opcode & 0x07;

        std::stringstream ss;
        if (operation_group == 0) { // Rotates/Shifts
            ss << operations[bit] << " " << address_str;
        } else { // BIT, RES, SET
            ss << bit_ops[operation_group - 1] << " " << (int)bit << ", " << address_str;
        }

        // For undocumented instructions that also store the result in a register
        if (reg_code != 6) {
            ss << ", " << registers[reg_code];
        }
        m_mnemonic = ss.str();
    }

    std::string disassemble(uint16_t& address) {
        m_address = address;
        m_mnemonic.clear();
        set_index_mode(IndexMode::HL);
        uint8_t opcode = peek_next_opcode();
        while (opcode == 0xDD || opcode == 0xFD) {
            set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
            opcode = peek_next_opcode();
        }
        switch (opcode) {
            case 0x00: handle_opcode_0x00_NOP(); break;
            case 0x01: handle_opcode_0x01_LD_BC_nn(); break;
            case 0x02: handle_opcode_0x02_LD_BC_ptr_A(); break;
            case 0x03: handle_opcode_0x03_INC_BC(); break;
            case 0x04: handle_opcode_0x04_INC_B(); break;
            case 0x05: handle_opcode_0x05_DEC_B(); break;
            case 0x06: handle_opcode_0x06_LD_B_n(); break;
            case 0x07: handle_opcode_0x07_RLCA(); break;
            case 0x08: handle_opcode_0x08_EX_AF_AFp(); break;
            case 0x09: handle_opcode_0x09_ADD_HL_BC(); break;
            case 0x0A: handle_opcode_0x0A_LD_A_BC_ptr(); break;
            case 0x0B: handle_opcode_0x0B_DEC_BC(); break;
            case 0x0C: handle_opcode_0x0C_INC_C(); break;
            case 0x0D: handle_opcode_0x0D_DEC_C(); break;
            case 0x0E: handle_opcode_0x0E_LD_C_n(); break;
            case 0x0F: handle_opcode_0x0F_RRCA(); break;
            case 0x10: handle_opcode_0x10_DJNZ_d(); break;
            case 0x11: handle_opcode_0x11_LD_DE_nn(); break;
            case 0x12: handle_opcode_0x12_LD_DE_ptr_A(); break;
            case 0x13: handle_opcode_0x13_INC_DE(); break;
            case 0x14: handle_opcode_0x14_INC_D(); break;
            case 0x15: handle_opcode_0x15_DEC_D(); break;
            case 0x16: handle_opcode_0x16_LD_D_n(); break;
            case 0x17: handle_opcode_0x17_RLA(); break;
            case 0x18: handle_opcode_0x18_JR_d(); break;
            case 0x19: handle_opcode_0x19_ADD_HL_DE(); break;
            case 0x1A: handle_opcode_0x1A_LD_A_DE_ptr(); break;
            case 0x1B: handle_opcode_0x1B_DEC_DE(); break;
            case 0x1C: handle_opcode_0x1C_INC_E(); break;
            case 0x1D: handle_opcode_0x1D_DEC_E(); break;
            case 0x1E: handle_opcode_0x1E_LD_E_n(); break;
            case 0x1F: handle_opcode_0x1F_RRA(); break;
            case 0x20: handle_opcode_0x20_JR_NZ_d(); break;
            case 0x21: handle_opcode_0x21_LD_HL_nn(); break;
            case 0x22: handle_opcode_0x22_LD_nn_ptr_HL(); break;
            case 0x23: handle_opcode_0x23_INC_HL(); break;
            case 0x24: handle_opcode_0x24_INC_H(); break;
            case 0x25: handle_opcode_0x25_DEC_H(); break;
            case 0x26: handle_opcode_0x26_LD_H_n(); break;
            case 0x27: handle_opcode_0x27_DAA(); break;
            case 0x28: handle_opcode_0x28_JR_Z_d(); break;
            case 0x29: handle_opcode_0x29_ADD_HL_HL(); break;
            case 0x2A: handle_opcode_0x2A_LD_HL_nn_ptr(); break;
            case 0x2B: handle_opcode_0x2B_DEC_HL(); break;
            case 0x2C: handle_opcode_0x2C_INC_L(); break;
            case 0x2D: handle_opcode_0x2D_DEC_L(); break;
            case 0x2E: handle_opcode_0x2E_LD_L_n(); break;
            case 0x2F: handle_opcode_0x2F_CPL(); break;
            case 0x30: handle_opcode_0x30_JR_NC_d(); break;
            case 0x31: handle_opcode_0x31_LD_SP_nn(); break;
            case 0x32: handle_opcode_0x32_LD_nn_ptr_A(); break;
            case 0x33: handle_opcode_0x33_INC_SP(); break;
            case 0x34: handle_opcode_0x34_INC_HL_ptr(); break;
            case 0x35: handle_opcode_0x35_DEC_HL_ptr(); break;
            case 0x36: handle_opcode_0x36_LD_HL_ptr_n(); break;
            case 0x37: handle_opcode_0x37_SCF(); break;
            case 0x38: handle_opcode_0x38_JR_C_d(); break;
            case 0x39: handle_opcode_0x39_ADD_HL_SP(); break;
            case 0x3A: handle_opcode_0x3A_LD_A_nn_ptr(); break;
            case 0x3B: handle_opcode_0x3B_DEC_SP(); break;
            case 0x3C: handle_opcode_0x3C_INC_A(); break;
            case 0x3D: handle_opcode_0x3D_DEC_A(); break;
            case 0x3E: handle_opcode_0x3E_LD_A_n(); break;
            case 0x3F: handle_opcode_0x3F_CCF(); break;
            case 0x40: handle_opcode_0x40_LD_B_B(); break;
            case 0x41: handle_opcode_0x41_LD_B_C(); break;
            case 0x42: handle_opcode_0x42_LD_B_D(); break;
            case 0x43: handle_opcode_0x43_LD_B_E(); break;
            case 0x44: handle_opcode_0x44_LD_B_H(); break;
            case 0x45: handle_opcode_0x45_LD_B_L(); break;
            case 0x46: handle_opcode_0x46_LD_B_HL_ptr(); break;
            case 0x47: handle_opcode_0x47_LD_B_A(); break;
            case 0x48: handle_opcode_0x48_LD_C_B(); break;
            case 0x49: handle_opcode_0x49_LD_C_C(); break;
            case 0x4A: handle_opcode_0x4A_LD_C_D(); break;
            case 0x4B: handle_opcode_0x4B_LD_C_E(); break;
            case 0x4C: handle_opcode_0x4C_LD_C_H(); break;
            case 0x4D: handle_opcode_0x4D_LD_C_L(); break;
            case 0x4E: handle_opcode_0x4E_LD_C_HL_ptr(); break;
            case 0x4F: handle_opcode_0x4F_LD_C_A(); break;
            case 0x50: handle_opcode_0x50_LD_D_B(); break;
            case 0x51: handle_opcode_0x51_LD_D_C(); break;
            case 0x52: handle_opcode_0x52_LD_D_D(); break;
            case 0x53: handle_opcode_0x53_LD_D_E(); break;
            case 0x54: handle_opcode_0x54_LD_D_H(); break;
            case 0x55: handle_opcode_0x55_LD_D_L(); break;
            case 0x56: handle_opcode_0x56_LD_D_HL_ptr(); break;
            case 0x57: handle_opcode_0x57_LD_D_A(); break;
            case 0x58: handle_opcode_0x58_LD_E_B(); break;
            case 0x59: handle_opcode_0x59_LD_E_C(); break;
            case 0x5A: handle_opcode_0x5A_LD_E_D(); break;
            case 0x5B: handle_opcode_0x5B_LD_E_E(); break;
            case 0x5C: handle_opcode_0x5C_LD_E_H(); break;
            case 0x5D: handle_opcode_0x5D_LD_E_L(); break;
            case 0x5E: handle_opcode_0x5E_LD_E_HL_ptr(); break;
            case 0x5F: handle_opcode_0x5F_LD_E_A(); break;
            case 0x60: handle_opcode_0x60_LD_H_B(); break;
            case 0x61: handle_opcode_0x61_LD_H_C(); break;
            case 0x62: handle_opcode_0x62_LD_H_D(); break;
            case 0x63: handle_opcode_0x63_LD_H_E(); break;
            case 0x64: handle_opcode_0x64_LD_H_H(); break;
            case 0x65: handle_opcode_0x65_LD_H_L(); break;
            case 0x66: handle_opcode_0x66_LD_H_HL_ptr(); break;
            case 0x67: handle_opcode_0x67_LD_H_A(); break;
            case 0x68: handle_opcode_0x68_LD_L_B(); break;
            case 0x69: handle_opcode_0x69_LD_L_C(); break;
            case 0x6A: handle_opcode_0x6A_LD_L_D(); break;
            case 0x6B: handle_opcode_0x6B_LD_L_E(); break;
            case 0x6C: handle_opcode_0x6C_LD_L_H(); break;
            case 0x6D: handle_opcode_0x6D_LD_L_L(); break;
            case 0x6E: handle_opcode_0x6E_LD_L_HL_ptr(); break;
            case 0x6F: handle_opcode_0x6F_LD_L_A(); break;
            case 0x70: handle_opcode_0x70_LD_HL_ptr_B(); break;
            case 0x71: handle_opcode_0x71_LD_HL_ptr_C(); break;
            case 0x72: handle_opcode_0x72_LD_HL_ptr_D(); break;
            case 0x73: handle_opcode_0x73_LD_HL_ptr_E(); break;
            case 0x74: handle_opcode_0x74_LD_HL_ptr_H(); break;
            case 0x75: handle_opcode_0x75_LD_HL_ptr_L(); break;
            case 0x76: handle_opcode_0x76_HALT(); break;
            case 0x77: handle_opcode_0x77_LD_HL_ptr_A(); break;
            case 0x78: handle_opcode_0x78_LD_A_B(); break;
            case 0x79: handle_opcode_0x79_LD_A_C(); break;
            case 0x7A: handle_opcode_0x7A_LD_A_D(); break;
            case 0x7B: handle_opcode_0x7B_LD_A_E(); break;
            case 0x7C: handle_opcode_0x7C_LD_A_H(); break;
            case 0x7D: handle_opcode_0x7D_LD_A_L(); break;
            case 0x7E: handle_opcode_0x7E_LD_A_HL_ptr(); break;
            case 0x7F: handle_opcode_0x7F_LD_A_A(); break;
            case 0x80: handle_opcode_0x80_ADD_A_B(); break;
            case 0x81: handle_opcode_0x81_ADD_A_C(); break;
            case 0x82: handle_opcode_0x82_ADD_A_D(); break;
            case 0x83: handle_opcode_0x83_ADD_A_E(); break;
            case 0x84: handle_opcode_0x84_ADD_A_H(); break;
            case 0x85: handle_opcode_0x85_ADD_A_L(); break;
            case 0x86: handle_opcode_0x86_ADD_A_HL_ptr(); break;
            case 0x87: handle_opcode_0x87_ADD_A_A(); break;
            case 0x88: handle_opcode_0x88_ADC_A_B(); break;
            case 0x89: handle_opcode_0x89_ADC_A_C(); break;
            case 0x8A: handle_opcode_0x8A_ADC_A_D(); break;
            case 0x8B: handle_opcode_0x8B_ADC_A_E(); break;
            case 0x8C: handle_opcode_0x8C_ADC_A_H(); break;
            case 0x8D: handle_opcode_0x8D_ADC_A_L(); break;
            case 0x8E: handle_opcode_0x8E_ADC_A_HL_ptr(); break;
            case 0x8F: handle_opcode_0x8F_ADC_A_A(); break;
            case 0x90: handle_opcode_0x90_SUB_B(); break;
            case 0x91: handle_opcode_0x91_SUB_C(); break;
            case 0x92: handle_opcode_0x92_SUB_D(); break;
            case 0x93: handle_opcode_0x93_SUB_E(); break;
            case 0x94: handle_opcode_0x94_SUB_H(); break;
            case 0x95: handle_opcode_0x95_SUB_L(); break;
            case 0x96: handle_opcode_0x96_SUB_HL_ptr(); break;
            case 0x97: handle_opcode_0x97_SUB_A(); break;
            case 0x98: handle_opcode_0x98_SBC_A_B(); break;
            case 0x99: handle_opcode_0x99_SBC_A_C(); break;
            case 0x9A: handle_opcode_0x9A_SBC_A_D(); break;
            case 0x9B: handle_opcode_0x9B_SBC_A_E(); break;
            case 0x9C: handle_opcode_0x9C_SBC_A_H(); break;
            case 0x9D: handle_opcode_0x9D_SBC_A_L(); break;
            case 0x9E: handle_opcode_0x9E_SBC_A_HL_ptr(); break;
            case 0x9F: handle_opcode_0x9F_SBC_A_A(); break;
            case 0xA0: handle_opcode_0xA0_AND_B(); break;
            case 0xA1: handle_opcode_0xA1_AND_C(); break;
            case 0xA2: handle_opcode_0xA2_AND_D(); break;
            case 0xA3: handle_opcode_0xA3_AND_E(); break;
            case 0xA4: handle_opcode_0xA4_AND_H(); break;
            case 0xA5: handle_opcode_0xA5_AND_L(); break;
            case 0xA6: handle_opcode_0xA6_AND_HL_ptr(); break;
            case 0xA7: handle_opcode_0xA7_AND_A(); break;
            case 0xA8: handle_opcode_0xA8_XOR_B(); break;
            case 0xA9: handle_opcode_0xA9_XOR_C(); break;
            case 0xAA: handle_opcode_0xAA_XOR_D(); break;
            case 0xAB: handle_opcode_0xAB_XOR_E(); break;
            case 0xAC: handle_opcode_0xAC_XOR_H(); break;
            case 0xAD: handle_opcode_0xAD_XOR_L(); break;
            case 0xAE: handle_opcode_0xAE_XOR_HL_ptr(); break;
            case 0xAF: handle_opcode_0xAF_XOR_A(); break;
            case 0xB0: handle_opcode_0xB0_OR_B(); break;
            case 0xB1: handle_opcode_0xB1_OR_C(); break;
            case 0xB2: handle_opcode_0xB2_OR_D(); break;
            case 0xB3: handle_opcode_0xB3_OR_E(); break;
            case 0xB4: handle_opcode_0xB4_OR_H(); break;
            case 0xB5: handle_opcode_0xB5_OR_L(); break;
            case 0xB6: handle_opcode_0xB6_OR_HL_ptr(); break;
            case 0xB7: handle_opcode_0xB7_OR_A(); break;
            case 0xB8: handle_opcode_0xB8_CP_B(); break;
            case 0xB9: handle_opcode_0xB9_CP_C(); break;
            case 0xBA: handle_opcode_0xBA_CP_D(); break;
            case 0xBB: handle_opcode_0xBB_CP_E(); break;
            case 0xBC: handle_opcode_0xBC_CP_H(); break;
            case 0xBD: handle_opcode_0xBD_CP_L(); break;
            case 0xBE: handle_opcode_0xBE_CP_HL_ptr(); break;
            case 0xBF: handle_opcode_0xBF_CP_A(); break;
            case 0xC0: handle_opcode_0xC0_RET_NZ(); break;
            case 0xC1: handle_opcode_0xC1_POP_BC(); break;
            case 0xC2: handle_opcode_0xC2_JP_NZ_nn(); break;
            case 0xC3: handle_opcode_0xC3_JP_nn(); break;
            case 0xC4: handle_opcode_0xC4_CALL_NZ_nn(); break;
            case 0xC5: handle_opcode_0xC5_PUSH_BC(); break;
            case 0xC6: handle_opcode_0xC6_ADD_A_n(); break;
            case 0xC7: handle_opcode_0xC7_RST_00H(); break;
            case 0xC8: handle_opcode_0xC8_RET_Z(); break;
            case 0xC9: handle_opcode_0xC9_RET(); break;
            case 0xCA: handle_opcode_0xCA_JP_Z_nn(); break;
            case 0xCB:
                if (get_index_mode() == IndexMode::HL)
                {
                    uint8_t cb_opcode = peek_next_opcode();
                    handle_CB_opcodes(cb_opcode);
                } else {
                    int8_t offset = static_cast<int8_t>(peek_next_byte());
                    uint8_t cb_opcode = peek_next_byte();
                    handle_CB_indexed_opcodes(offset, cb_opcode);
                }
                break;
            case 0xCC: handle_opcode_0xCC_CALL_Z_nn(); break;
            case 0xCD: handle_opcode_0xCD_CALL_nn(); break;
            case 0xCE: handle_opcode_0xCE_ADC_A_n(); break;
            case 0xCF: handle_opcode_0xCF_RST_08H(); break;
            case 0xD0: handle_opcode_0xD0_RET_NC(); break;
            case 0xD1: handle_opcode_0xD1_POP_DE(); break;
            case 0xD2: handle_opcode_0xD2_JP_NC_nn(); break;
            case 0xD3: handle_opcode_0xD3_OUT_n_ptr_A(); break;
            case 0xD4: handle_opcode_0xD4_CALL_NC_nn(); break;
            case 0xD5: handle_opcode_0xD5_PUSH_DE(); break;
            case 0xD6: handle_opcode_0xD6_SUB_n(); break;
            case 0xD7: handle_opcode_0xD7_RST_10H(); break;
            case 0xD8: handle_opcode_0xD8_RET_C(); break;
            case 0xD9: handle_opcode_0xD9_EXX(); break;
            case 0xDA: handle_opcode_0xDA_JP_C_nn(); break;
            case 0xDB: handle_opcode_0xDB_IN_A_n_ptr(); break;
            case 0xDC: handle_opcode_0xDC_CALL_C_nn(); break;
            case 0xDE: handle_opcode_0xDE_SBC_A_n(); break;
            case 0xDF: handle_opcode_0xDF_RST_18H(); break;
            case 0xE0: handle_opcode_0xE0_RET_PO(); break;
            case 0xE1: handle_opcode_0xE1_POP_HL(); break;
            case 0xE2: handle_opcode_0xE2_JP_PO_nn(); break;
            case 0xE3: handle_opcode_0xE3_EX_SP_ptr_HL(); break;
            case 0xE4: handle_opcode_0xE4_CALL_PO_nn(); break;
            case 0xE5: handle_opcode_0xE5_PUSH_HL(); break;
            case 0xE6: handle_opcode_0xE6_AND_n(); break;
            case 0xE7: handle_opcode_0xE7_RST_20H(); break;
            case 0xE8: handle_opcode_0xE8_RET_PE(); break;
            case 0xE9: handle_opcode_0xE9_JP_HL_ptr(); break;
            case 0xEA: handle_opcode_0xEA_JP_PE_nn(); break;
            case 0xEB: handle_opcode_0xEB_EX_DE_HL(); break;
            case 0xEC: handle_opcode_0xEC_CALL_PE_nn(); break;
            case 0xED: {
                uint8_t opcodeED = peek_next_opcode();
                set_index_mode(IndexMode::HL);
                switch (opcodeED) {
                    case 0x40: handle_opcode_0xED_0x40_IN_B_C_ptr(); break;
                    case 0x41: handle_opcode_0xED_0x41_OUT_C_ptr_B(); break;
                    case 0x42: handle_opcode_0xED_0x42_SBC_HL_BC(); break;
                    case 0x43: handle_opcode_0xED_0x43_LD_nn_ptr_BC(); break;
                    case 0x44: handle_opcode_0xED_0x44_NEG(); break;
                    case 0x45: handle_opcode_0xED_0x45_RETN(); break;
                    case 0x46: handle_opcode_0xED_0x46_IM_0(); break;
                    case 0x47: handle_opcode_0xED_0x47_LD_I_A(); break;
                    case 0x48: handle_opcode_0xED_0x48_IN_C_C_ptr(); break;
                    case 0x49: handle_opcode_0xED_0x49_OUT_C_ptr_C(); break;
                    case 0x4A: handle_opcode_0xED_0x4A_ADC_HL_BC(); break;
                    case 0x4B: handle_opcode_0xED_0x4B_LD_BC_nn_ptr(); break;
                    case 0x4D: handle_opcode_0xED_0x4D_RETI(); break;
                    case 0x4F: handle_opcode_0xED_0x4F_LD_R_A(); break;
                    case 0x50: handle_opcode_0xED_0x50_IN_D_C_ptr(); break;
                    case 0x51: handle_opcode_0xED_0x51_OUT_C_ptr_D(); break;
                    case 0x52: handle_opcode_0xED_0x52_SBC_HL_DE(); break;
                    case 0x53: handle_opcode_0xED_0x53_LD_nn_ptr_DE(); break;
                    case 0x56: handle_opcode_0xED_0x56_IM_1(); break;
                    case 0x57: handle_opcode_0xED_0x57_LD_A_I(); break;
                    case 0x58: handle_opcode_0xED_0x58_IN_E_C_ptr(); break;
                    case 0x59: handle_opcode_0xED_0x59_OUT_C_ptr_E(); break;
                    case 0x5A: handle_opcode_0xED_0x5A_ADC_HL_DE(); break;
                    case 0x5B: handle_opcode_0xED_0x5B_LD_DE_nn_ptr(); break;
                    case 0x5E: handle_opcode_0xED_0x5E_IM_2(); break;
                    case 0x5F: handle_opcode_0xED_0x5F_LD_A_R(); break;
                    case 0x60: handle_opcode_0xED_0x60_IN_H_C_ptr(); break;
                    case 0x61: handle_opcode_0xED_0x61_OUT_C_ptr_H(); break;
                    case 0x62: handle_opcode_0xED_0x62_SBC_HL_HL(); break;
                    case 0x63: handle_opcode_0xED_0x63_LD_nn_ptr_HL_ED(); break;
                    case 0x67: handle_opcode_0xED_0x67_RRD(); break;
                    case 0x68: handle_opcode_0xED_0x68_IN_L_C_ptr(); break;
                    case 0x69: handle_opcode_0xED_0x69_OUT_C_ptr_L(); break;
                    case 0x6A: handle_opcode_0xED_0x6A_ADC_HL_HL(); break;
                    case 0x6B: handle_opcode_0xED_0x6B_LD_HL_nn_ptr_ED(); break;
                    case 0x6F: handle_opcode_0xED_0x6F_RLD(); break;
                    case 0x70: handle_opcode_0xED_0x70_IN_C_ptr(); break;
                    case 0x71: handle_opcode_0xED_0x71_OUT_C_ptr_0(); break;
                    case 0x72: handle_opcode_0xED_0x72_SBC_HL_SP(); break;
                    case 0x73: handle_opcode_0xED_0x73_LD_nn_ptr_SP(); break;
                    case 0x78: handle_opcode_0xED_0x78_IN_A_C_ptr(); break;
                    case 0x79: handle_opcode_0xED_0x79_OUT_C_ptr_A(); break;
                    case 0x7A: handle_opcode_0xED_0x7A_ADC_HL_SP(); break;
                    case 0x7B: handle_opcode_0xED_0x7B_LD_SP_nn_ptr(); break;
                    case 0xA0: handle_opcode_0xED_0xA0_LDI(); break;
                    case 0xA1: handle_opcode_0xED_0xA1_CPI(); break;
                    case 0xA2: handle_opcode_0xED_0xA2_INI(); break;
                    case 0xA3: handle_opcode_0xED_0xA3_OUTI(); break;
                    case 0xA8: handle_opcode_0xED_0xA8_LDD(); break;
                    case 0xA9: handle_opcode_0xED_0xA9_CPD(); break;
                    case 0xAA: handle_opcode_0xED_0xAA_IND(); break;
                    case 0xAB: handle_opcode_0xED_0xAB_OUTD(); break;
                    case 0xB0: handle_opcode_0xED_0xB0_LDIR(); break;
                    case 0xB1: handle_opcode_0xED_0xB1_CPIR(); break;
                    case 0xB2: handle_opcode_0xED_0xB2_INIR(); break;
                    case 0xB3: handle_opcode_0xED_0xB3_OTIR(); break;
                    case 0xB8: handle_opcode_0xED_0xB8_LDDR(); break;
                    case 0xB9: handle_opcode_0xED_0xB9_CPDR(); break;
                    case 0xBA: handle_opcode_0xED_0xBA_INDR(); break;
                    case 0xBB: handle_opcode_0xED_0xBB_OTDR(); break;
                    default:
                        return "NOP (DB 0xED, " + format_hex(opcodeED) + ")";
                }
                break;
            }
            case 0xEE: handle_opcode_0xEE_XOR_n(); break;
            case 0xEF: handle_opcode_0xEF_RST_28H(); break;
            case 0xF0: handle_opcode_0xF0_RET_P(); break;
            case 0xF1: handle_opcode_0xF1_POP_AF(); break;
            case 0xF2: handle_opcode_0xF2_JP_P_nn(); break;
            case 0xF3: handle_opcode_0xF3_DI(); break;
            case 0xF4: handle_opcode_0xF4_CALL_P_nn(); break;
            case 0xF5: handle_opcode_0xF5_PUSH_AF(); break;
            case 0xF6: handle_opcode_0xF6_OR_n(); break;
            case 0xF7: handle_opcode_0xF7_RST_30H(); break;
            case 0xF8: handle_opcode_0xF8_RET_M(); break;
            case 0xF9: handle_opcode_0xF9_LD_SP_HL(); break;
            case 0xFA: handle_opcode_0xFA_JP_M_nn(); break;
            case 0xFB: handle_opcode_0xFB_EI(); break;
            case 0xFC: handle_opcode_0xFC_CALL_M_nn(); break;
            case 0xFE: handle_opcode_0xFE_CP_n(); break;
            case 0xFF: handle_opcode_0xFF_RST_38H(); break;
        }
    }

private:
    uint16_t m_address;
    std::string m_mnemonic;
    std::vector<uint8_t> m_bytes;

    enum class IndexMode { HL, IX, IY };
    IndexMode m_index_mode;
    
    IndexMode get_index_mode() const { return m_index_mode;}
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

    uint8_t peek_next_byte() {
        uint8_t value = m_bus->peek(m_address++);
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