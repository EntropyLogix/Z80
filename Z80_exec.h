private:
    template<auto HandleFunc>
    void exec_helper() {
        add_ticks(4);
        (this->*HandleFunc)();
    }   
   
   template <auto HandleFunc>
    void exec_DD_helper() {
        add_ticks(8);
        IndexMode old_index_mode = get_index_mode();
        set_index_mode(IndexMode::IX);
        (this->*HandleFunc)();
        set_index_mode(old_index_mode);
    }

    template <auto HandleFunc>
    void exec_FD_helper() {
        add_ticks(8);
        IndexMode old_index_mode = get_index_mode();
        set_index_mode(IndexMode::IY);
        (this->*HandleFunc)();
        set_index_mode(old_index_mode);
    }

    template <auto HandleFunc>
    void exec_ED_helper() {
        add_ticks(8);
        (this->*HandleFunc)();
    }

    void exec_CB_helper(uint8_t opcode) {
        add_ticks(8);
        handle_CB_opcodes(opcode);
    }

    void exec_DDCB_helper(int8_t offset, uint8_t opcode) {
        add_ticks(12);
        IndexMode old_index_mode = get_index_mode();
        set_index_mode(IndexMode::IX);
        handle_CB_indexed_opcodes(get_IX(), offset, opcode);
        set_index_mode(old_index_mode);
    }

    void exec_FDCB_helper(int8_t offset, uint8_t opcode) {
        add_ticks(12);
        IndexMode old_index_mode = get_index_mode();
        set_index_mode(IndexMode::IY);
        handle_CB_indexed_opcodes(get_IY(), offset, opcode);
        set_index_mode(old_index_mode);
    }

public:
    void exec_NOP() {
        exec_helper<&Z80::handle_opcode_0x00_NOP>(); 
    }
    void exec_LD_BC_nn() {
        exec_helper<&Z80::handle_opcode_0x01_LD_BC_nn>();
    }
    void exec_LD_BC_ptr_A() {
        exec_helper<&Z80::handle_opcode_0x02_LD_BC_ptr_A>();
    }
    void exec_INC_BC() {
        exec_helper<&Z80::handle_opcode_0x03_INC_BC>();
    }
    void exec_INC_B() {
        exec_helper<&Z80::handle_opcode_0x04_INC_B>();
    }
    void exec_DEC_B() {
        exec_helper<&Z80::handle_opcode_0x05_DEC_B>();
    }
    void exec_LD_B_n() {
        exec_helper<&Z80::handle_opcode_0x06_LD_B_n>();
    }
    void exec_RLCA() {
        exec_helper<&Z80::handle_opcode_0x07_RLCA>();
    }
    void exec_EX_AF_AFp() {
        exec_helper<&Z80::handle_opcode_0x08_EX_AF_AFp>();
    }
    void exec_ADD_HL_BC() {
        exec_helper<&Z80::handle_opcode_0x09_ADD_HL_BC>();
    }
    void exec_LD_A_BC_ptr() {
        exec_helper<&Z80::handle_opcode_0x0A_LD_A_BC_ptr>();
    }
    void exec_DEC_BC() {
        exec_helper<&Z80::handle_opcode_0x0B_DEC_BC>();
    }
    void exec_INC_C() {
        exec_helper<&Z80::handle_opcode_0x0C_INC_C>();
    }
    void exec_DEC_C() {
        exec_helper<&Z80::handle_opcode_0x0D_DEC_C>();
    }
    void exec_LD_C_n() {
        exec_helper<&Z80::handle_opcode_0x0E_LD_C_n>();
    }
    void exec_RRCA() {
        exec_helper<&Z80::handle_opcode_0x0F_RRCA>();
    }
    void exec_DJNZ_d() {
        exec_helper<&Z80::handle_opcode_0x10_DJNZ_d>();
    }
    void exec_LD_DE_nn() {
        exec_helper<&Z80::handle_opcode_0x11_LD_DE_nn>();
    }
    void exec_LD_DE_ptr_A() {
        exec_helper<&Z80::handle_opcode_0x12_LD_DE_ptr_A>();
    }
    void exec_INC_DE() {
        exec_helper<&Z80::handle_opcode_0x13_INC_DE>();
    }
    void exec_INC_D() {
        exec_helper<&Z80::handle_opcode_0x14_INC_D>();
    }
    void exec_DEC_D() {
        exec_helper<&Z80::handle_opcode_0x15_DEC_D>();
    }
    void exec_LD_D_n() {
        exec_helper<&Z80::handle_opcode_0x16_LD_D_n>();
    }
    void exec_RLA() {
        exec_helper<&Z80::handle_opcode_0x17_RLA>();
    }
    void exec_JR_d() {
        exec_helper<&Z80::handle_opcode_0x18_JR_d>();
    }
    void exec_ADD_HL_DE() {
        exec_helper<&Z80::handle_opcode_0x19_ADD_HL_DE>(); 
    }
    void exec_LD_A_DE_ptr() {
        exec_helper<&Z80::handle_opcode_0x1A_LD_A_DE_ptr>();
    }
    void exec_DEC_DE() {
        exec_helper<&Z80::handle_opcode_0x1B_DEC_DE>(); 
    }
    void exec_INC_E() {
        exec_helper<&Z80::handle_opcode_0x1C_INC_E>(); 
    }
    void exec_DEC_E() {
        exec_helper<&Z80::handle_opcode_0x1D_DEC_E>(); 
    }
    void exec_LD_E_n() {
        exec_helper<&Z80::handle_opcode_0x1E_LD_E_n>(); 
    }
    void exec_RRA() {
        exec_helper<&Z80::handle_opcode_0x1F_RRA>(); 
    }
    void exec_JR_NZ_d() {
        exec_helper<&Z80::handle_opcode_0x20_JR_NZ_d>(); 
    }
    void exec_LD_HL_nn() {
        exec_helper<&Z80::handle_opcode_0x21_LD_HL_nn>(); 
    }
    void exec_LD_nn_ptr_HL() {
        exec_helper<&Z80::handle_opcode_0x22_LD_nn_ptr_HL>(); 
    }
    void exec_INC_HL() {
        exec_helper<&Z80::handle_opcode_0x23_INC_HL>(); 
    }
    void exec_INC_H() {
        exec_helper<&Z80::handle_opcode_0x24_INC_H>(); 
    }
    void exec_DEC_H() {
        exec_helper<&Z80::handle_opcode_0x25_DEC_H>(); 
    }
    void exec_LD_H_n() {
        exec_helper<&Z80::handle_opcode_0x26_LD_H_n>(); 
    }
    void exec_DAA() {
        exec_helper<&Z80::handle_opcode_0x27_DAA>(); 
    }
    void exec_JR_Z_d() {
        exec_helper<&Z80::handle_opcode_0x28_JR_Z_d>(); 
    }
    void exec_ADD_HL_HL() {
        exec_helper<&Z80::handle_opcode_0x29_ADD_HL_HL>(); 
    }
    void exec_LD_HL_nn_ptr() {
        exec_helper<&Z80::handle_opcode_0x2A_LD_HL_nn_ptr>(); 
    }
    void exec_DEC_HL() {
        exec_helper<&Z80::handle_opcode_0x2B_DEC_HL>(); 
    }
    void exec_INC_L() {
        exec_helper<&Z80::handle_opcode_0x2C_INC_L>(); 
    }
    void exec_DEC_L() {
        exec_helper<&Z80::handle_opcode_0x2D_DEC_L>(); 
    }
    void exec_LD_L_n() {
        exec_helper<&Z80::handle_opcode_0x2E_LD_L_n>(); 
    }
    void exec_CPL() {
        exec_helper<&Z80::handle_opcode_0x2F_CPL>(); 
    }
    void exec_JR_NC_d() {
        exec_helper<&Z80::handle_opcode_0x30_JR_NC_d>(); 
    }
    void exec_LD_SP_nn() {
        exec_helper<&Z80::handle_opcode_0x31_LD_SP_nn>(); 
    }
    void exec_LD_nn_ptr_A() {
        exec_helper<&Z80::handle_opcode_0x32_LD_nn_ptr_A>(); 
    }
    void exec_INC_SP() {
        exec_helper<&Z80::handle_opcode_0x33_INC_SP>(); 
    }
    void exec_INC_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x34_INC_HL_ptr>(); 
    }
    void exec_DEC_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x35_DEC_HL_ptr>(); 
    }
    void exec_LD_HL_ptr_n() {
        exec_helper<&Z80::handle_opcode_0x36_LD_HL_ptr_n>(); 
    }
    void exec_SCF() {
        exec_helper<&Z80::handle_opcode_0x37_SCF>(); 
    }
    void exec_JR_C_d() {
        exec_helper<&Z80::handle_opcode_0x38_JR_C_d>(); 
    }
    void exec_ADD_HL_SP() {
        exec_helper<&Z80::handle_opcode_0x39_ADD_HL_SP>(); 
    }
    void exec_LD_A_nn_ptr() {
        exec_helper<&Z80::handle_opcode_0x3A_LD_A_nn_ptr>(); 
    }
    void exec_DEC_SP() {
        exec_helper<&Z80::handle_opcode_0x3B_DEC_SP>(); 
    }
    void exec_INC_A() {
        exec_helper<&Z80::handle_opcode_0x3C_INC_A>(); 
    }
    void exec_DEC_A() {
        exec_helper<&Z80::handle_opcode_0x3D_DEC_A>(); 
    }
    void exec_LD_A_n() {
        exec_helper<&Z80::handle_opcode_0x3E_LD_A_n>(); 
    }
    void exec_CCF() {
        exec_helper<&Z80::handle_opcode_0x3F_CCF>(); 
    }
    void exec_LD_B_B() {
        exec_helper<&Z80::handle_opcode_0x40_LD_B_B>(); 
    }
    void exec_LD_B_C() {
        exec_helper<&Z80::handle_opcode_0x41_LD_B_C>(); 
    }
    void exec_LD_B_D() {
        exec_helper<&Z80::handle_opcode_0x42_LD_B_D>(); 
    }
    void exec_LD_B_E() {
        exec_helper<&Z80::handle_opcode_0x43_LD_B_E>(); 
    }
    void exec_LD_B_H() {
        exec_helper<&Z80::handle_opcode_0x44_LD_B_H>(); 
    }
    void exec_LD_B_L() {
        exec_helper<&Z80::handle_opcode_0x45_LD_B_L>(); 
    }
    void exec_LD_B_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x46_LD_B_HL_ptr>(); 
    }
    void exec_LD_B_A() {
        exec_helper<&Z80::handle_opcode_0x47_LD_B_A>(); 
    }
    void exec_LD_C_B() {
        exec_helper<&Z80::handle_opcode_0x48_LD_C_B>(); 
    }
    void exec_LD_C_C() {
        exec_helper<&Z80::handle_opcode_0x49_LD_C_C>(); 
    }
    void exec_LD_C_D() {
        exec_helper<&Z80::handle_opcode_0x4A_LD_C_D>(); 
    }
    void exec_LD_C_E() {
        exec_helper<&Z80::handle_opcode_0x4B_LD_C_E>(); 
    }
    void exec_LD_C_H() {
        exec_helper<&Z80::handle_opcode_0x4C_LD_C_H>(); 
    }
    void exec_LD_C_L() {
        exec_helper<&Z80::handle_opcode_0x4D_LD_C_L>(); 
    }
    void exec_LD_C_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x4E_LD_C_HL_ptr>(); 
    }
    void exec_LD_C_A() {
        exec_helper<&Z80::handle_opcode_0x4F_LD_C_A>(); 
    }
    void exec_LD_D_B() {
        exec_helper<&Z80::handle_opcode_0x50_LD_D_B>(); 
    }
    void exec_LD_D_C() {
        exec_helper<&Z80::handle_opcode_0x51_LD_D_C>(); 
    }
    void exec_LD_D_D() {
        exec_helper<&Z80::handle_opcode_0x52_LD_D_D>(); 
    }
    void exec_LD_D_E() {
        exec_helper<&Z80::handle_opcode_0x53_LD_D_E>(); 
    }
    void exec_LD_D_H() {
        exec_helper<&Z80::handle_opcode_0x54_LD_D_H>(); 
    }
    void exec_LD_D_L() {
        exec_helper<&Z80::handle_opcode_0x55_LD_D_L>(); 
    }
    void exec_LD_D_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x56_LD_D_HL_ptr>(); 
    }
    void exec_LD_D_A() {
        exec_helper<&Z80::handle_opcode_0x57_LD_D_A>(); 
    }
    void exec_LD_E_B() {
        exec_helper<&Z80::handle_opcode_0x58_LD_E_B>(); 
    }
    void exec_LD_E_C() {
        exec_helper<&Z80::handle_opcode_0x59_LD_E_C>(); 
    }
    void exec_LD_E_D() {
        exec_helper<&Z80::handle_opcode_0x5A_LD_E_D>(); 
    }
    void exec_LD_E_E() {
        exec_helper<&Z80::handle_opcode_0x5B_LD_E_E>(); 
    }
    void exec_LD_E_H() {
        exec_helper<&Z80::handle_opcode_0x5C_LD_E_H>(); 
    }
    void exec_LD_E_L() {
        exec_helper<&Z80::handle_opcode_0x5D_LD_E_L>(); 
    }
    void exec_LD_E_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x5E_LD_E_HL_ptr>(); 
    }
    void exec_LD_E_A() {
        exec_helper<&Z80::handle_opcode_0x5F_LD_E_A>(); 
    }
    void exec_LD_H_B() {
        exec_helper<&Z80::handle_opcode_0x60_LD_H_B>(); 
    }
    void exec_LD_H_C() {
        exec_helper<&Z80::handle_opcode_0x61_LD_H_C>(); 
    }
    void exec_LD_H_D() {
        exec_helper<&Z80::handle_opcode_0x62_LD_H_D>(); 
    }
    void exec_LD_H_E() {
        exec_helper<&Z80::handle_opcode_0x63_LD_H_E>(); 
    }
    void exec_LD_H_H() {
        exec_helper<&Z80::handle_opcode_0x64_LD_H_H>(); 
    }
    void exec_LD_H_L() {
        exec_helper<&Z80::handle_opcode_0x65_LD_H_L>(); 
    }
    void exec_LD_H_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x66_LD_H_HL_ptr>(); 
    }
    void exec_LD_H_A() {
        exec_helper<&Z80::handle_opcode_0x67_LD_H_A>(); 
    }
    void exec_LD_L_B() {
        exec_helper<&Z80::handle_opcode_0x68_LD_L_B>(); 
    }
    void exec_LD_L_C() {
        exec_helper<&Z80::handle_opcode_0x69_LD_L_C>(); 
    }
    void exec_LD_L_D() {
        exec_helper<&Z80::handle_opcode_0x6A_LD_L_D>(); 
    }
    void exec_LD_L_E() {
        exec_helper<&Z80::handle_opcode_0x6B_LD_L_E>(); 
    }
    void exec_LD_L_H() {
        exec_helper<&Z80::handle_opcode_0x6C_LD_L_H>(); 
    }
    void exec_LD_L_L() {
        exec_helper<&Z80::handle_opcode_0x6D_LD_L_L>(); 
    }
    void exec_LD_L_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x6E_LD_L_HL_ptr>(); 
    }
    void exec_LD_L_A() {
        exec_helper<&Z80::handle_opcode_0x6F_LD_L_A>(); 
    }
    void exec_LD_HL_ptr_B() {
        exec_helper<&Z80::handle_opcode_0x70_LD_HL_ptr_B>(); 
    }
    void exec_LD_HL_ptr_C() {
        exec_helper<&Z80::handle_opcode_0x71_LD_HL_ptr_C>(); 
    }
    void exec_LD_HL_ptr_D() {
        exec_helper<&Z80::handle_opcode_0x72_LD_HL_ptr_D>(); 
    }
    void exec_LD_HL_ptr_E() {
        exec_helper<&Z80::handle_opcode_0x73_LD_HL_ptr_E>(); 
    }
    void exec_LD_HL_ptr_H() {
        exec_helper<&Z80::handle_opcode_0x74_LD_HL_ptr_H>(); 
    }
    void exec_LD_HL_ptr_L() {
        exec_helper<&Z80::handle_opcode_0x75_LD_HL_ptr_L>(); 
    }
    void exec_HALT() {
        exec_helper<&Z80::handle_opcode_0x76_HALT>(); 
    }
    void exec_LD_HL_ptr_A() {
        exec_helper<&Z80::handle_opcode_0x77_LD_HL_ptr_A>(); 
    }
    void exec_LD_A_B() {
        exec_helper<&Z80::handle_opcode_0x78_LD_A_B>(); 
    }
    void exec_LD_A_C() {
        exec_helper<&Z80::handle_opcode_0x79_LD_A_C>(); 
    }
    void exec_LD_A_D() {
        exec_helper<&Z80::handle_opcode_0x7A_LD_A_D>(); 
    }
    void exec_LD_A_E() {
        exec_helper<&Z80::handle_opcode_0x7B_LD_A_E>(); 
    }
    void exec_LD_A_H() {
        exec_helper<&Z80::handle_opcode_0x7C_LD_A_H>(); 
    }
    void exec_LD_A_L() {
        exec_helper<&Z80::handle_opcode_0x7D_LD_A_L>(); 
    }
    void exec_LD_A_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x7E_LD_A_HL_ptr>(); 
    }
    void exec_LD_A_A() {
        exec_helper<&Z80::handle_opcode_0x7F_LD_A_A>(); 
    }
    void exec_ADD_A_B() {
        exec_helper<&Z80::handle_opcode_0x80_ADD_A_B>(); 
    }
    void exec_ADD_A_C() {
        exec_helper<&Z80::handle_opcode_0x81_ADD_A_C>(); 
    }
    void exec_ADD_A_D() {
        exec_helper<&Z80::handle_opcode_0x82_ADD_A_D>(); 
    }
    void exec_ADD_A_E() {
        exec_helper<&Z80::handle_opcode_0x83_ADD_A_E>(); 
    }
    void exec_ADD_A_H() {
        exec_helper<&Z80::handle_opcode_0x84_ADD_A_H>(); 
    }
    void exec_ADD_A_L() {
        exec_helper<&Z80::handle_opcode_0x85_ADD_A_L>(); 
    }
    void exec_ADD_A_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x86_ADD_A_HL_ptr>(); 
    }
    void exec_ADD_A_A() {
        exec_helper<&Z80::handle_opcode_0x87_ADD_A_A>(); 
    }
    void exec_ADC_A_B() {
        exec_helper<&Z80::handle_opcode_0x88_ADC_A_B>(); 
    }
    void exec_ADC_A_C() {
        exec_helper<&Z80::handle_opcode_0x89_ADC_A_C>(); 
    }
    void exec_ADC_A_D() {
        exec_helper<&Z80::handle_opcode_0x8A_ADC_A_D>(); 
    }
    void exec_ADC_A_E() {
        exec_helper<&Z80::handle_opcode_0x8B_ADC_A_E>(); 
    }
    void exec_ADC_A_H() {
        exec_helper<&Z80::handle_opcode_0x8C_ADC_A_H>(); 
    }
    void exec_ADC_A_L() {
        exec_helper<&Z80::handle_opcode_0x8D_ADC_A_L>(); 
    }
    void exec_ADC_A_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x8E_ADC_A_HL_ptr>(); 
    }
    void exec_ADC_A_A() {
        exec_helper<&Z80::handle_opcode_0x8F_ADC_A_A>(); 
    }
    void exec_SUB_B() {
        exec_helper<&Z80::handle_opcode_0x90_SUB_B>(); 
    }
    void exec_SUB_C() {
        exec_helper<&Z80::handle_opcode_0x91_SUB_C>(); 
    }
    void exec_SUB_D() {
        exec_helper<&Z80::handle_opcode_0x92_SUB_D>(); 
    }
    void exec_SUB_E() {
        exec_helper<&Z80::handle_opcode_0x93_SUB_E>(); 
    }
    void exec_SUB_H() {
        exec_helper<&Z80::handle_opcode_0x94_SUB_H>(); 
    }
    void exec_SUB_L() {
        exec_helper<&Z80::handle_opcode_0x95_SUB_L>(); 
    }
    void exec_SUB_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x96_SUB_HL_ptr>(); 
    }
    void exec_SUB_A() {
        exec_helper<&Z80::handle_opcode_0x97_SUB_A>(); 
    }
    void exec_SBC_A_B() {
        exec_helper<&Z80::handle_opcode_0x98_SBC_A_B>(); 
    }
    void exec_SBC_A_C() {
        exec_helper<&Z80::handle_opcode_0x99_SBC_A_C>(); 
    }
    void exec_SBC_A_D() {
        exec_helper<&Z80::handle_opcode_0x9A_SBC_A_D>(); 
    }
    void exec_SBC_A_E() {
        exec_helper<&Z80::handle_opcode_0x9B_SBC_A_E>(); 
    }
    void exec_SBC_A_H() {
        exec_helper<&Z80::handle_opcode_0x9C_SBC_A_H>(); 
    }
    void exec_SBC_A_L() {
        exec_helper<&Z80::handle_opcode_0x9D_SBC_A_L>(); 
    }
    void exec_SBC_A_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0x9E_SBC_A_HL_ptr>(); 
    }
    void exec_SBC_A_A() {
        exec_helper<&Z80::handle_opcode_0x9F_SBC_A_A>(); 
    }
    void exec_AND_B() {
        exec_helper<&Z80::handle_opcode_0xA0_AND_B>(); 
    }
    void exec_AND_C() {
        exec_helper<&Z80::handle_opcode_0xA1_AND_C>(); 
    }
    void exec_AND_D() {
        exec_helper<&Z80::handle_opcode_0xA2_AND_D>(); 
    }
    void exec_AND_E() {
        exec_helper<&Z80::handle_opcode_0xA3_AND_E>(); 
    }
    void exec_AND_H() {
        exec_helper<&Z80::handle_opcode_0xA4_AND_H>(); 
    }
    void exec_AND_L() {
        exec_helper<&Z80::handle_opcode_0xA5_AND_L>(); 
    }
    void exec_AND_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0xA6_AND_HL_ptr>(); 
    }
    void exec_AND_A() {
        exec_helper<&Z80::handle_opcode_0xA7_AND_A>(); 
    }
    void exec_XOR_B() {
        exec_helper<&Z80::handle_opcode_0xA8_XOR_B>(); 
    }
    void exec_XOR_C() {
        exec_helper<&Z80::handle_opcode_0xA9_XOR_C>(); 
    }
    void exec_XOR_D() {
        exec_helper<&Z80::handle_opcode_0xAA_XOR_D>(); 
    }
    void exec_XOR_E() {
        exec_helper<&Z80::handle_opcode_0xAB_XOR_E>(); 
    }
    void exec_XOR_H() {
        exec_helper<&Z80::handle_opcode_0xAC_XOR_H>(); 
    }
    void exec_XOR_L() {
        exec_helper<&Z80::handle_opcode_0xAD_XOR_L>(); 
    }
    void exec_XOR_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0xAE_XOR_HL_ptr>(); 
    }
    void exec_XOR_A() {
        exec_helper<&Z80::handle_opcode_0xAF_XOR_A>(); 
    }
    void exec_OR_B() {
        exec_helper<&Z80::handle_opcode_0xB0_OR_B>(); 
    }
    void exec_OR_C() {
        exec_helper<&Z80::handle_opcode_0xB1_OR_C>(); 
    }
    void exec_OR_D() {
        exec_helper<&Z80::handle_opcode_0xB2_OR_D>(); 
    }
    void exec_OR_E() {
        exec_helper<&Z80::handle_opcode_0xB3_OR_E>(); 
    }
    void exec_OR_H() {
        exec_helper<&Z80::handle_opcode_0xB4_OR_H>(); 
    }
    void exec_OR_L() {
        exec_helper<&Z80::handle_opcode_0xB5_OR_L>(); 
    }
    void exec_OR_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0xB6_OR_HL_ptr>(); 
    }
    void exec_OR_A() {
        exec_helper<&Z80::handle_opcode_0xB7_OR_A>(); 
    }
    void exec_CP_B() {
        exec_helper<&Z80::handle_opcode_0xB8_CP_B>(); 
    }
    void exec_CP_C() {
        exec_helper<&Z80::handle_opcode_0xB9_CP_C>(); 
    }
    void exec_CP_D() {
        exec_helper<&Z80::handle_opcode_0xBA_CP_D>(); 
    }
    void exec_CP_E() {
        exec_helper<&Z80::handle_opcode_0xBB_CP_E>(); 
    }
    void exec_CP_H() {
        exec_helper<&Z80::handle_opcode_0xBC_CP_H>(); 
    }
    void exec_CP_L() {
        exec_helper<&Z80::handle_opcode_0xBD_CP_L>(); 
    }
    void exec_CP_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0xBE_CP_HL_ptr>(); 
    }
    void exec_CP_A() {
        exec_helper<&Z80::handle_opcode_0xBF_CP_A>(); 
    }
    void exec_RET_NZ() {
        exec_helper<&Z80::handle_opcode_0xC0_RET_NZ>(); 
    }
    void exec_POP_BC() {
        exec_helper<&Z80::handle_opcode_0xC1_POP_BC>(); 
    }
    void exec_JP_NZ_nn() {
        exec_helper<&Z80::handle_opcode_0xC2_JP_NZ_nn>(); 
    }
    void exec_JP_nn() {
        exec_helper<&Z80::handle_opcode_0xC3_JP_nn>(); 
    }
    void exec_CALL_NZ_nn() {
        exec_helper<&Z80::handle_opcode_0xC4_CALL_NZ_nn>(); 
    }
    void exec_PUSH_BC() {
        exec_helper<&Z80::handle_opcode_0xC5_PUSH_BC>(); 
    }
    void exec_ADD_A_n() {
        exec_helper<&Z80::handle_opcode_0xC6_ADD_A_n>(); 
    }
    void exec_RST_00H() {
        exec_helper<&Z80::handle_opcode_0xC7_RST_00H>(); 
    }
    void exec_RET_Z() {
        exec_helper<&Z80::handle_opcode_0xC8_RET_Z>(); 
    }
    void exec_RET() {
        exec_helper<&Z80::handle_opcode_0xC9_RET>(); 
    }
    void exec_JP_Z_nn() {
        exec_helper<&Z80::handle_opcode_0xCA_JP_Z_nn>(); 
    }
    void exec_CALL_Z_nn() {
        exec_helper<&Z80::handle_opcode_0xCC_CALL_Z_nn>(); 
    }
    void exec_CALL_nn() {
        exec_helper<&Z80::handle_opcode_0xCD_CALL_nn>(); 
    }
    void exec_ADC_A_n() {
        exec_helper<&Z80::handle_opcode_0xCE_ADC_A_n>(); 
    }
    void exec_RST_08H() {
        exec_helper<&Z80::handle_opcode_0xCF_RST_08H>(); 
    }
    void exec_RET_NC() {
        exec_helper<&Z80::handle_opcode_0xD0_RET_NC>(); 
    }
    void exec_POP_DE() {
        exec_helper<&Z80::handle_opcode_0xD1_POP_DE>(); 
    }
    void exec_JP_NC_nn() {
        exec_helper<&Z80::handle_opcode_0xD2_JP_NC_nn>(); 
    }
    void exec_OUT_n_ptr_A() {
        exec_helper<&Z80::handle_opcode_0xD3_OUT_n_ptr_A>(); 
    }
    void exec_CALL_NC_nn() {
        exec_helper<&Z80::handle_opcode_0xD4_CALL_NC_nn>(); 
    }
    void exec_PUSH_DE() {
        exec_helper<&Z80::handle_opcode_0xD5_PUSH_DE>(); 
    }
    void exec_SUB_n() {
        exec_helper<&Z80::handle_opcode_0xD6_SUB_n>(); 
    }
    void exec_RST_10H() {
        exec_helper<&Z80::handle_opcode_0xD7_RST_10H>(); 
    }
    void exec_RET_C() {
        exec_helper<&Z80::handle_opcode_0xD8_RET_C>(); 
    }
    void exec_EXX() {
        exec_helper<&Z80::handle_opcode_0xD9_EXX>(); 
    }
    void exec_JP_C_nn() {
        exec_helper<&Z80::handle_opcode_0xDA_JP_C_nn>(); 
    }
    void exec_IN_A_n_ptr() {
        exec_helper<&Z80::handle_opcode_0xDB_IN_A_n_ptr>(); 
    }
    void exec_CALL_C_nn() {
        exec_helper<&Z80::handle_opcode_0xDC_CALL_C_nn>(); 
    }
    void exec_SBC_A_n() {
        exec_helper<&Z80::handle_opcode_0xDE_SBC_A_n>(); 
    }
    void exec_RST_18H() {
        exec_helper<&Z80::handle_opcode_0xDF_RST_18H>(); 
    }
    void exec_RET_PO() {
        exec_helper<&Z80::handle_opcode_0xE0_RET_PO>(); 
    }
    void exec_POP_HL() {
        exec_helper<&Z80::handle_opcode_0xE1_POP_HL>(); 
    }
    void exec_JP_PO_nn() {
        exec_helper<&Z80::handle_opcode_0xE2_JP_PO_nn>(); 
    }
    void exec_EX_SP_ptr_HL() {
        exec_helper<&Z80::handle_opcode_0xE3_EX_SP_ptr_HL>(); 
    }
    void exec_CALL_PO_nn() {
        exec_helper<&Z80::handle_opcode_0xE4_CALL_PO_nn>(); 
    }
    void exec_PUSH_HL() {
        exec_helper<&Z80::handle_opcode_0xE5_PUSH_HL>(); 
    }
    void exec_AND_n() {
        exec_helper<&Z80::handle_opcode_0xE6_AND_n>(); 
    }
    void exec_RST_20H() {
        exec_helper<&Z80::handle_opcode_0xE7_RST_20H>(); 
    }
    void exec_RET_PE() {
        exec_helper<&Z80::handle_opcode_0xE8_RET_PE>(); 
    }
    void exec_JP_HL_ptr() {
        exec_helper<&Z80::handle_opcode_0xE9_JP_HL_ptr>(); 
    }
    void exec_JP_PE_nn() {
        exec_helper<&Z80::handle_opcode_0xEA_JP_PE_nn>(); 
    }
    void exec_EX_DE_HL() {
        exec_helper<&Z80::handle_opcode_0xEB_EX_DE_HL>(); 
    }
    void exec_CALL_PE_nn() {
        exec_helper<&Z80::handle_opcode_0xEC_CALL_PE_nn>(); 
    }
    void exec_XOR_n() {
        exec_helper<&Z80::handle_opcode_0xEE_XOR_n>(); 
    }
    void exec_RST_28H() {
        exec_helper<&Z80::handle_opcode_0xEF_RST_28H>(); 
    }
    void exec_RET_P() {
        exec_helper<&Z80::handle_opcode_0xF0_RET_P>(); 
    }
    void exec_POP_AF() {
        exec_helper<&Z80::handle_opcode_0xF1_POP_AF>(); 
    }
    void exec_JP_P_nn() {
        exec_helper<&Z80::handle_opcode_0xF2_JP_P_nn>(); 
    }
    void exec_DI() {
        exec_helper<&Z80::handle_opcode_0xF3_DI>(); 
    }
    void exec_CALL_P_nn() {
        exec_helper<&Z80::handle_opcode_0xF4_CALL_P_nn>(); 
    }
    void exec_PUSH_AF() {
        exec_helper<&Z80::handle_opcode_0xF5_PUSH_AF>(); 
    }
    void exec_OR_n() {
        exec_helper<&Z80::handle_opcode_0xF6_OR_n>(); 
    }
    void exec_RST_30H() {
        exec_helper<&Z80::handle_opcode_0xF7_RST_30H>(); 
    }
    void exec_RET_M() {
        exec_helper<&Z80::handle_opcode_0xF8_RET_M>(); 
    }
    void exec_LD_SP_HL() {
        exec_helper<&Z80::handle_opcode_0xF9_LD_SP_HL>(); 
    }
    void exec_JP_M_nn() {
        exec_helper<&Z80::handle_opcode_0xFA_JP_M_nn>(); 
    }
    void exec_EI() {
        exec_helper<&Z80::handle_opcode_0xFB_EI>(); 
    }
    void exec_CALL_M_nn() {
        exec_helper<&Z80::handle_opcode_0xFC_CALL_M_nn>(); 
    }
    void exec_CP_n() {
        exec_helper<&Z80::handle_opcode_0xFE_CP_n>(); 
    }
    void exec_RST_38H() {
        exec_helper<&Z80::handle_opcode_0xFF_RST_38H>(); 
    }
    //DD / FD
    void exec_ADD_IX_BC() {
        exec_DD_helper<&Z80::handle_opcode_0x09_ADD_HL_BC>(); 
    }
    void exec_ADD_IY_BC() {
        exec_FD_helper<&Z80::handle_opcode_0x09_ADD_HL_BC>(); 
    }
    void exec_ADD_IX_DE() {
        exec_DD_helper<&Z80::handle_opcode_0x19_ADD_HL_DE>(); 
    }
    void exec_ADD_IY_DE() {
        exec_FD_helper<&Z80::handle_opcode_0x19_ADD_HL_DE>(); 
    }
    void exec_LD_IX_nn() {
        exec_DD_helper<&Z80::handle_opcode_0x21_LD_HL_nn>(); 
    }
    void exec_LD_IY_nn() {
        exec_FD_helper<&Z80::handle_opcode_0x21_LD_HL_nn>(); 
    }
    void exec_LD_nn_ptr_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x22_LD_nn_ptr_HL>(); 
    }
    void exec_LD_nn_ptr_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x22_LD_nn_ptr_HL>(); 
    }
    void exec_INC_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x23_INC_HL>(); 
    }
    void exec_INC_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x23_INC_HL>(); 
    }
    void exec_INC_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x24_INC_H>(); 
    }
    void exec_INC_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x24_INC_H>(); 
    }
    void exec_DEC_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x25_DEC_H>(); 
    }
    void exec_DEC_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x25_DEC_H>(); 
    }
    void exec_LD_IXH_n() {
        exec_DD_helper<&Z80::handle_opcode_0x26_LD_H_n>(); 
    }
    void exec_LD_IYH_n() {
        exec_FD_helper<&Z80::handle_opcode_0x26_LD_H_n>(); 
    }
    void exec_ADD_IX_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x29_ADD_HL_HL>(); 
    }
    void exec_ADD_IY_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x29_ADD_HL_HL>(); 
    }
    void exec_LD_IX_nn_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x2A_LD_HL_nn_ptr>(); 
    }
    void exec_LD_IY_nn_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x2A_LD_HL_nn_ptr>(); 
    }
    void exec_DEC_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x2B_DEC_HL>(); 
    }
    void exec_DEC_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x2B_DEC_HL>(); 
    }
    void exec_INC_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x2C_INC_L>(); 
    }
    void exec_INC_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x2C_INC_L>(); 
    }
    void exec_DEC_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x2D_DEC_L>(); 
    }
    void exec_DEC_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x2D_DEC_L>(); 
    }
    void exec_LD_IXL_n() {
        exec_DD_helper<&Z80::handle_opcode_0x2E_LD_L_n>(); 
    }
    void exec_LD_IYL_n() {
        exec_FD_helper<&Z80::handle_opcode_0x2E_LD_L_n>(); 
    }
    void exec_INC_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x34_INC_HL_ptr>(); 
    }
    void exec_INC_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x34_INC_HL_ptr>(); 
    }
    void exec_DEC_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x35_DEC_HL_ptr>(); 
    }
    void exec_DEC_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x35_DEC_HL_ptr>(); 
    }
    void exec_LD_IX_d_ptr_n() {
        exec_DD_helper<&Z80::handle_opcode_0x36_LD_HL_ptr_n>(); 
    }
    void exec_LD_IY_d_ptr_n() {
        exec_FD_helper<&Z80::handle_opcode_0x36_LD_HL_ptr_n>(); 
    }
    void exec_ADD_IX_SP() {
        exec_DD_helper<&Z80::handle_opcode_0x39_ADD_HL_SP>(); 
    }
    void exec_ADD_IY_SP() {
        exec_FD_helper<&Z80::handle_opcode_0x39_ADD_HL_SP>(); 
    }
    void exec_LD_B_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x44_LD_B_H>(); 
    }
    void exec_LD_B_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x44_LD_B_H>(); 
    }
    void exec_LD_B_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x45_LD_B_L>(); 
    }
    void exec_LD_B_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x45_LD_B_L>(); 
    }
    void exec_LD_B_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x46_LD_B_HL_ptr>(); 
    }
    void exec_LD_B_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x46_LD_B_HL_ptr>(); 
    }
    void exec_LD_C_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x4C_LD_C_H>(); 
    }
    void exec_LD_C_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x4C_LD_C_H>(); 
    }
    void exec_LD_C_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x4D_LD_C_L>(); 
    }
    void exec_LD_C_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x4D_LD_C_L>(); 
    }
    void exec_LD_C_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x4E_LD_C_HL_ptr>(); 
    }
    void exec_LD_C_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x4E_LD_C_HL_ptr>(); 
    }
    void exec_LD_D_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x54_LD_D_H>(); 
    }
    void exec_LD_D_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x54_LD_D_H>(); 
    }
    void exec_LD_D_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x55_LD_D_L>(); 
    }
    void exec_LD_D_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x55_LD_D_L>(); 
    }
    void exec_LD_D_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x56_LD_D_HL_ptr>(); 
    }
    void exec_LD_D_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x56_LD_D_HL_ptr>(); 
    }
    void exec_LD_E_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x5C_LD_E_H>(); 
    }
    void exec_LD_E_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x5C_LD_E_H>(); 
    }
    void exec_LD_E_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x5D_LD_E_L>(); 
    }
    void exec_LD_E_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x5D_LD_E_L>(); 
    }
    void exec_LD_E_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x5E_LD_E_HL_ptr>(); 
    }
    void exec_LD_E_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x5E_LD_E_HL_ptr>(); 
    }
    void exec_LD_IXH_B() {
        exec_DD_helper<&Z80::handle_opcode_0x60_LD_H_B>(); 
    }
    void exec_LD_IYH_B() {
        exec_FD_helper<&Z80::handle_opcode_0x60_LD_H_B>(); 
    }
    void exec_LD_IXH_C() {
        exec_DD_helper<&Z80::handle_opcode_0x61_LD_H_C>(); 
    }
    void exec_LD_IYH_C() {
        exec_FD_helper<&Z80::handle_opcode_0x61_LD_H_C>(); 
    }
    void exec_LD_IXH_D() {
        exec_DD_helper<&Z80::handle_opcode_0x62_LD_H_D>(); 
    }
    void exec_LD_IYH_D() {
        exec_FD_helper<&Z80::handle_opcode_0x62_LD_H_D>(); 
    }
    void exec_LD_IXH_E() {
        exec_DD_helper<&Z80::handle_opcode_0x63_LD_H_E>(); 
    }
    void exec_LD_IYH_E() {
        exec_FD_helper<&Z80::handle_opcode_0x63_LD_H_E>(); 
    }
    void exec_LD_IXH_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x64_LD_H_H>(); 
    }
    void exec_LD_IYH_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x64_LD_H_H>(); 
    }
    void exec_LD_IXH_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x65_LD_H_L>(); 
    }
    void exec_LD_IYH_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x65_LD_H_L>(); 
    }
    void exec_LD_H_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x66_LD_H_HL_ptr>(); 
    }
    void exec_LD_H_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x66_LD_H_HL_ptr>(); 
    }
    void exec_LD_IXH_A() {
        exec_DD_helper<&Z80::handle_opcode_0x67_LD_H_A>(); 
    }
    void exec_LD_IYH_A() {
        exec_FD_helper<&Z80::handle_opcode_0x67_LD_H_A>(); 
    }
    void exec_LD_IXL_B() {
        exec_DD_helper<&Z80::handle_opcode_0x68_LD_L_B>(); 
    }
    void exec_LD_IYL_B() {
        exec_FD_helper<&Z80::handle_opcode_0x68_LD_L_B>(); 
    }
    void exec_LD_IXL_C() {
        exec_DD_helper<&Z80::handle_opcode_0x69_LD_L_C>(); 
    }
    void exec_LD_IYL_C() {
        exec_FD_helper<&Z80::handle_opcode_0x69_LD_L_C>(); 
    }
    void exec_LD_IXL_D() {
        exec_DD_helper<&Z80::handle_opcode_0x6A_LD_L_D>(); 
    }
    void exec_LD_IYL_D() {
        exec_FD_helper<&Z80::handle_opcode_0x6A_LD_L_D>(); 
    }
    void exec_LD_IXL_E() {
        exec_DD_helper<&Z80::handle_opcode_0x6B_LD_L_E>(); 
    }
    void exec_LD_IYL_E() {
        exec_FD_helper<&Z80::handle_opcode_0x6B_LD_L_E>(); 
    }
    void exec_LD_IXL_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x6C_LD_L_H>(); 
    }
    void exec_LD_IYL_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x6C_LD_L_H>(); 
    }
    void exec_LD_IXL_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x6D_LD_L_L>(); 
    }
    void exec_LD_IYL_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x6D_LD_L_L>(); 
    }
    void exec_LD_L_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x6E_LD_L_HL_ptr>(); 
    }
    void exec_LD_L_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x6E_LD_L_HL_ptr>(); 
    }
    void exec_LD_IXL_A() {
        exec_DD_helper<&Z80::handle_opcode_0x6F_LD_L_A>(); 
    }
    void exec_LD_IYL_A() {
        exec_FD_helper<&Z80::handle_opcode_0x6F_LD_L_A>(); 
    }
    void exec_LD_IX_d_ptr_B() {
        exec_DD_helper<&Z80::handle_opcode_0x70_LD_HL_ptr_B>(); 
    }
    void exec_LD_IY_d_ptr_B() {
        exec_FD_helper<&Z80::handle_opcode_0x70_LD_HL_ptr_B>(); 
    }
    void exec_LD_IX_d_ptr_C() {
        exec_DD_helper<&Z80::handle_opcode_0x71_LD_HL_ptr_C>(); 
    }
    void exec_LD_IY_d_ptr_C() {
        exec_FD_helper<&Z80::handle_opcode_0x71_LD_HL_ptr_C>(); 
    }
    void exec_LD_IX_d_ptr_D() {
        exec_DD_helper<&Z80::handle_opcode_0x72_LD_HL_ptr_D>(); 
    }
    void exec_LD_IY_d_ptr_D() {
        exec_FD_helper<&Z80::handle_opcode_0x72_LD_HL_ptr_D>(); 
    }
    void exec_LD_IX_d_ptr_E() {
        exec_DD_helper<&Z80::handle_opcode_0x73_LD_HL_ptr_E>(); 
    }
    void exec_LD_IY_d_ptr_E() {
        exec_FD_helper<&Z80::handle_opcode_0x73_LD_HL_ptr_E>(); 
    }
    void exec_LD_IX_d_ptr_H() {
        exec_DD_helper<&Z80::handle_opcode_0x74_LD_HL_ptr_H>(); 
    }
    void exec_LD_IY_d_ptr_H() {
        exec_FD_helper<&Z80::handle_opcode_0x74_LD_HL_ptr_H>(); 
    }
    void exec_LD_IX_d_ptr_L() {
        exec_DD_helper<&Z80::handle_opcode_0x75_LD_HL_ptr_L>(); 
    }
    void exec_LD_IY_d_ptr_L() {
        exec_FD_helper<&Z80::handle_opcode_0x75_LD_HL_ptr_L>(); 
    }
    void exec_LD_IX_d_ptr_A() {
        exec_DD_helper<&Z80::handle_opcode_0x77_LD_HL_ptr_A>(); 
    }
    void exec_LD_IY_d_ptr_A() {
        exec_FD_helper<&Z80::handle_opcode_0x77_LD_HL_ptr_A>(); 
    }
    void exec_LD_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x7C_LD_A_H>(); 
    }
    void exec_LD_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x7C_LD_A_H>(); 
    }
    void exec_LD_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x7D_LD_A_L>(); 
    }
    void exec_LD_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x7D_LD_A_L>(); 
    }
    void exec_LD_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x7E_LD_A_HL_ptr>(); 
    }
    void exec_LD_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x7E_LD_A_HL_ptr>(); 
    }
    void exec_ADD_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x84_ADD_A_H>(); 
    }
    void exec_ADD_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x84_ADD_A_H>(); 
    }
    void exec_ADD_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x85_ADD_A_L>(); 
    }
    void exec_ADD_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x85_ADD_A_L>(); 
    }
    void exec_ADD_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x86_ADD_A_HL_ptr>(); 
    }
    void exec_ADD_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x86_ADD_A_HL_ptr>(); 
    }
    void exec_ADC_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x8C_ADC_A_H>(); 
    }
    void exec_ADC_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x8C_ADC_A_H>(); 
    }
    void exec_ADC_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x8D_ADC_A_L>(); 
    }
    void exec_ADC_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x8D_ADC_A_L>(); 
    }
    void exec_ADC_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x8E_ADC_A_HL_ptr>(); 
    }
    void exec_ADC_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x8E_ADC_A_HL_ptr>(); 
    }
    void exec_SUB_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x94_SUB_H>(); 
    }
    void exec_SUB_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x94_SUB_H>(); 
    }
    void exec_SUB_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x95_SUB_L>(); 
    }
    void exec_SUB_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x95_SUB_L>(); 
    }
    void exec_SUB_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x96_SUB_HL_ptr>(); 
    }
    void exec_SUB_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x96_SUB_HL_ptr>(); 
    }
    void exec_SBC_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x9C_SBC_A_H>(); 
    }
    void exec_SBC_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x9C_SBC_A_H>(); 
    }
    void exec_SBC_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x9D_SBC_A_L>(); 
    }
    void exec_SBC_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x9D_SBC_A_L>(); 
    }
    void exec_SBC_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x9E_SBC_A_HL_ptr>(); 
    }
    void exec_SBC_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x9E_SBC_A_HL_ptr>(); 
    }
    void exec_AND_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xA4_AND_H>(); 
    }
    void exec_AND_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xA4_AND_H>(); 
    }
    void exec_AND_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xA5_AND_L>(); 
    }
    void exec_AND_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xA5_AND_L>(); 
    }
    void exec_AND_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xA6_AND_HL_ptr>(); 
    }
    void exec_AND_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xA6_AND_HL_ptr>(); 
    }
    void exec_XOR_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xAC_XOR_H>(); 
    }
    void exec_XOR_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xAC_XOR_H>(); 
    }
    void exec_XOR_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xAD_XOR_L>(); 
    }
    void exec_XOR_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xAD_XOR_L>(); 
    }
    void exec_XOR_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xAE_XOR_HL_ptr>(); 
    }
    void exec_XOR_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xAE_XOR_HL_ptr>(); 
    }
    void exec_OR_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xB4_OR_H>(); 
    }
    void exec_OR_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xB4_OR_H>(); 
    }
    void exec_OR_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xB5_OR_L>(); 
    }
    void exec_OR_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xB5_OR_L>(); 
    }
    void exec_OR_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xB6_OR_HL_ptr>(); 
    }
    void exec_OR_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xB6_OR_HL_ptr>(); 
    }
    void exec_CP_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xBC_CP_H>(); 
    }
    void exec_CP_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xBC_CP_H>(); 
    }
    void exec_CP_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xBD_CP_L>(); 
    }
    void exec_CP_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xBD_CP_L>(); 
    }
    void exec_CP_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xBE_CP_HL_ptr>(); 
    }
    void exec_CP_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xBE_CP_HL_ptr>(); 
    }
    void exec_POP_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE1_POP_HL>(); 
    }
    void exec_POP_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE1_POP_HL>(); 
    }
    void exec_EX_SP_ptr_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE3_EX_SP_ptr_HL>(); 
    }
    void exec_EX_SP_ptr_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE3_EX_SP_ptr_HL>(); 
    }
    void exec_PUSH_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE5_PUSH_HL>(); 
    }
    void exec_PUSH_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE5_PUSH_HL>(); 
    }
    void exec_JP_IX_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xE9_JP_HL_ptr>(); 
    }
    void exec_JP_IY_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xE9_JP_HL_ptr>(); 
    }
    void exec_LD_SP_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xF9_LD_SP_HL>(); 
    }
    void exec_LD_SP_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xF9_LD_SP_HL>(); 
    }

    // --- ED Prefixed Opcodes ---
    void exec_IN_B_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x40_IN_B_C_ptr>();
    }
    void exec_OUT_C_ptr_B() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x41_OUT_C_ptr_B>();
    }
    void exec_SBC_HL_BC() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x42_SBC_HL_BC>();
    }
    void exec_LD_nn_ptr_BC() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x43_LD_nn_ptr_BC>();
    }
    void exec_NEG() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x44_NEG>();
    }
    void exec_RETN() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x45_RETN>();
    }
    void exec_IM_0() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x46_IM_0>();
    }
    void exec_LD_I_A() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x47_LD_I_A>();
    }
    void exec_IN_C_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x48_IN_C_C_ptr>();
    }
    void exec_OUT_C_ptr_C() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x49_OUT_C_ptr_C>();
    }
    void exec_ADC_HL_BC() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x4A_ADC_HL_BC>();
    }
    void exec_LD_BC_nn_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x4B_LD_BC_nn_ptr>();
    }
    void exec_RETI() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x4D_RETI>();
    }
    void exec_LD_R_A() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x4F_LD_R_A>();
    }
    void exec_IN_D_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x50_IN_D_C_ptr>(); 
    }
    void exec_OUT_C_ptr_D() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x51_OUT_C_ptr_D>(); 
    }
    void exec_SBC_HL_DE() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x52_SBC_HL_DE>(); 
    }
    void exec_LD_nn_ptr_DE() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x53_LD_nn_ptr_DE>(); 
    }
    void exec_IM_1() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x56_IM_1>(); 
    }
    void exec_LD_A_I() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x57_LD_A_I>(); 
    }
    void exec_IN_E_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x58_IN_E_C_ptr>(); 
    }
    void exec_OUT_C_ptr_E() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x59_OUT_C_ptr_E>(); 
    }
    void exec_ADC_HL_DE() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x5A_ADC_HL_DE>(); 
    }
    void exec_LD_DE_nn_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x5B_LD_DE_nn_ptr>(); 
    }
    void exec_IM_2() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x5E_IM_2>(); 
    }
    void exec_LD_A_R() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x5F_LD_A_R>(); 
    }
    void exec_IN_H_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x60_IN_H_C_ptr>(); 
    }
    void exec_OUT_C_ptr_H() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x61_OUT_C_ptr_H>(); 
    }
    void exec_SBC_HL_HL() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x62_SBC_HL_HL>(); 
    }
    void exec_LD_nn_ptr_HL_ED() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x63_LD_nn_ptr_HL_ED>(); 
    }
    void exec_RRD() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x67_RRD>(); 
    }
    void exec_IN_L_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x68_IN_L_C_ptr>(); 
    }
    void exec_OUT_C_ptr_L() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x69_OUT_C_ptr_L>(); 
    }
    void exec_ADC_HL_HL() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x6A_ADC_HL_HL>(); 
    }
    void exec_LD_HL_nn_ptr_ED() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x6B_LD_HL_nn_ptr_ED>(); 
    }
    void exec_RLD() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x6F_RLD>(); 
    }
    void exec_IN_F_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x70_IN_C_ptr>(); 
    }
    void exec_OUT_C_ptr_0() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x71_OUT_C_ptr_0>(); 
    }
    void exec_SBC_HL_SP() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x72_SBC_HL_SP>(); 
    }
    void exec_LD_nn_ptr_SP() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x73_LD_nn_ptr_SP>(); 
    }
    void exec_IN_A_C_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x78_IN_A_C_ptr>(); 
    }
    void exec_OUT_C_ptr_A() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x79_OUT_C_ptr_A>(); 
    }
    void exec_ADC_HL_SP() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x7A_ADC_HL_SP>(); 
    }
    void exec_LD_SP_nn_ptr() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0x7B_LD_SP_nn_ptr>(); 
    }
    void exec_LDI() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA0_LDI>();
    }
    void exec_CPI() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA1_CPI>();
    }
    void exec_INI() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA2_INI>();
    }
    void exec_OUTI() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA3_OUTI>();
    }
    void exec_LDD() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA8_LDD>();
    }
    void exec_CPD() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xA9_CPD>();
    }
    void exec_IND() {
       
        exec_ED_helper<&Z80::handle_opcode_0xED_0xAA_IND>();
    }
    void exec_OUTD() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xAB_OUTD>();
    }
    void exec_LDIR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB0_LDIR>();
    }
    void exec_CPIR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB1_CPIR>();
    }
    void exec_INIR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB2_INIR>();
    }
    void exec_OTIR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB3_OTIR>();
    }
    void exec_LDDR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB8_LDDR>();
    }
    void exec_CPDR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xB9_CPDR>();
    }
    void exec_INDR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xBA_INDR>();
    }
    void exec_OTDR() {
        exec_ED_helper<&Z80::handle_opcode_0xED_0xBB_OTDR>();
    }

    // --- DDCB Prefixed Opcodes ---
    void exec_RLC_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x00); }
    void exec_RLC_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x01); }
    void exec_RLC_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x02); }
    void exec_RLC_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x03); }
    void exec_RLC_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x04); }
    void exec_RLC_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x05); }
    void exec_RLC_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x06); }
    void exec_RLC_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x07); }
    void exec_RRC_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x08); }
    void exec_RRC_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x09); }
    void exec_RRC_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x0A); }
    void exec_RRC_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x0B); }
    void exec_RRC_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x0C); }
    void exec_RRC_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x0D); }
    void exec_RRC_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x0E); }
    void exec_RRC_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x0F); }
    void exec_RL_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x10); }
    void exec_RL_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x11); }
    void exec_RL_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x12); }
    void exec_RL_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x13); }
    void exec_RL_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x14); }
    void exec_RL_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x15); }
    void exec_RL_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x16); }
    void exec_RL_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x17); }
    void exec_RR_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x18); }
    void exec_RR_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x19); }
    void exec_RR_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x1A); }
    void exec_RR_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x1B); }
    void exec_RR_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x1C); }
    void exec_RR_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x1D); }
    void exec_RR_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x1E); }
    void exec_RR_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x1F); }
    void exec_SLA_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x20); }
    void exec_SLA_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x21); }
    void exec_SLA_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x22); }
    void exec_SLA_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x23); }
    void exec_SLA_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x24); }
    void exec_SLA_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x25); }
    void exec_SLA_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x26); }
    void exec_SLA_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x27); }
    void exec_SRA_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x28); }
    void exec_SRA_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x29); }
    void exec_SRA_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x2A); }
    void exec_SRA_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x2B); }
    void exec_SRA_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x2C); }
    void exec_SRA_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x2D); }
    void exec_SRA_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x2E); }
    void exec_SRA_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x2F); }
    void exec_SLL_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x30); }
    void exec_SLL_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x31); }
    void exec_SLL_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x32); }
    void exec_SLL_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x33); }
    void exec_SLL_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x34); }
    void exec_SLL_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x35); }
    void exec_SLL_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x36); }
    void exec_SLL_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x37); }
    void exec_SRL_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x38); }
    void exec_SRL_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x39); }
    void exec_SRL_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x3A); }
    void exec_SRL_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x3B); }
    void exec_SRL_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x3C); }
    void exec_SRL_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x3D); }
    void exec_SRL_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x3E); }
    void exec_SRL_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x3F); }
    void exec_BIT_0_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x46); }
    void exec_BIT_1_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x4E); }
    void exec_BIT_2_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x56); }
    void exec_BIT_3_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x5E); }
    void exec_BIT_4_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x66); }
    void exec_BIT_5_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x6E); }
    void exec_BIT_6_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x76); }
    void exec_BIT_7_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x7E); }
    void exec_RES_0_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x80); }
    void exec_RES_0_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x81); }
    void exec_RES_0_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x82); }
    void exec_RES_0_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x83); }
    void exec_RES_0_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x84); }
    void exec_RES_0_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x85); }
    void exec_RES_0_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x86); }
    void exec_RES_0_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x87); }
    void exec_RES_1_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x88); }
    void exec_RES_1_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x89); }
    void exec_RES_1_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x8A); }
    void exec_RES_1_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x8B); }
    void exec_RES_1_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x8C); }
    void exec_RES_1_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x8D); }
    void exec_RES_1_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x8E); }
    void exec_RES_1_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x8F); }
    void exec_RES_2_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x90); }
    void exec_RES_2_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x91); }
    void exec_RES_2_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x92); }
    void exec_RES_2_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x93); }
    void exec_RES_2_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x94); }
    void exec_RES_2_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x95); }
    void exec_RES_2_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x96); }
    void exec_RES_2_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x97); }
    void exec_RES_3_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0x98); }
    void exec_RES_3_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0x99); }
    void exec_RES_3_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0x9A); }
    void exec_RES_3_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0x9B); }
    void exec_RES_3_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0x9C); }
    void exec_RES_3_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0x9D); }
    void exec_RES_3_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0x9E); }
    void exec_RES_3_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0x9F); }
    void exec_RES_4_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xA0); }
    void exec_RES_4_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xA1); }
    void exec_RES_4_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xA2); }
    void exec_RES_4_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xA3); }
    void exec_RES_4_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xA4); }
    void exec_RES_4_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xA5); }
    void exec_RES_4_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xA6); }
    void exec_RES_4_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xA7); }
    void exec_RES_5_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xA8); }
    void exec_RES_5_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xA9); }
    void exec_RES_5_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xAA); }
    void exec_RES_5_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xAB); }
    void exec_RES_5_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xAC); }
    void exec_RES_5_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xAD); }
    void exec_RES_5_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xAE); }
    void exec_RES_5_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xAF); }
    void exec_RES_6_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xB0); }
    void exec_RES_6_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xB1); }
    void exec_RES_6_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xB2); }
    void exec_RES_6_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xB3); }
    void exec_RES_6_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xB4); }
    void exec_RES_6_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xB5); }
    void exec_RES_6_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xB6); }
    void exec_RES_6_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xB7); }
    void exec_RES_7_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xB8); }
    void exec_RES_7_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xB9); }
    void exec_RES_7_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xBA); }
    void exec_RES_7_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xBB); }
    void exec_RES_7_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xBC); }
    void exec_RES_7_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xBD); }
    void exec_RES_7_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xBE); }
    void exec_RES_7_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xBF); }
    void exec_SET_0_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xC0); }
    void exec_SET_0_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xC1); }
    void exec_SET_0_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xC2); }
    void exec_SET_0_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xC3); }
    void exec_SET_0_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xC4); }
    void exec_SET_0_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xC5); }
    void exec_SET_0_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xC6); }
    void exec_SET_0_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xC7); }
    void exec_SET_1_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xC8); }
    void exec_SET_1_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xC9); }
    void exec_SET_1_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xCA); }
    void exec_SET_1_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xCB); }
    void exec_SET_1_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xCC); }
    void exec_SET_1_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xCD); }
    void exec_SET_1_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xCE); }
    void exec_SET_1_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xCF); }
    void exec_SET_2_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xD0); }
    void exec_SET_2_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xD1); }
    void exec_SET_2_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xD2); }
    void exec_SET_2_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xD3); }
    void exec_SET_2_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xD4); }
    void exec_SET_2_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xD5); }
    void exec_SET_2_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xD6); }
    void exec_SET_2_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xD7); }
    void exec_SET_3_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xD8); }
    void exec_SET_3_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xD9); }
    void exec_SET_3_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xDA); }
    void exec_SET_3_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xDB); }
    void exec_SET_3_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xDC); }
    void exec_SET_3_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xDD); }
    void exec_SET_3_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xDE); }
    void exec_SET_3_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xDF); }
    void exec_SET_4_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xE0); }
    void exec_SET_4_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xE1); }
    void exec_SET_4_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xE2); }
    void exec_SET_4_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xE3); }
    void exec_SET_4_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xE4); }
    void exec_SET_4_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xE5); }
    void exec_SET_4_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xE6); }
    void exec_SET_4_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xE7); }
    void exec_SET_5_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xE8); }
    void exec_SET_5_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xE9); }
    void exec_SET_5_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xEA); }
    void exec_SET_5_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xEB); }
    void exec_SET_5_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xEC); }
    void exec_SET_5_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xED); }
    void exec_SET_5_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xEE); }
    void exec_SET_5_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xEF); }
    void exec_SET_6_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xF0); }
    void exec_SET_6_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xF1); }
    void exec_SET_6_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xF2); }
    void exec_SET_6_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xF3); }
    void exec_SET_6_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xF4); }
    void exec_SET_6_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xF5); }
    void exec_SET_6_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xF6); }
    void exec_SET_6_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xF7); }
    void exec_SET_7_IX_d_ptr_B(int8_t offset) { exec_DDCB_helper(offset, 0xF8); }
    void exec_SET_7_IX_d_ptr_C(int8_t offset) { exec_DDCB_helper(offset, 0xF9); }
    void exec_SET_7_IX_d_ptr_D(int8_t offset) { exec_DDCB_helper(offset, 0xFA); }
    void exec_SET_7_IX_d_ptr_E(int8_t offset) { exec_DDCB_helper(offset, 0xFB); }
    void exec_SET_7_IX_d_ptr_H(int8_t offset) { exec_DDCB_helper(offset, 0xFC); }
    void exec_SET_7_IX_d_ptr_L(int8_t offset) { exec_DDCB_helper(offset, 0xFD); }
    void exec_SET_7_IX_d_ptr(int8_t offset) { exec_DDCB_helper(offset, 0xFE); }
    void exec_SET_7_IX_d_ptr_A(int8_t offset) { exec_DDCB_helper(offset, 0xFF); }

    // --- FDCB Prefixed Opcodes ---
    void exec_RLC_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x00); }
    void exec_RLC_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x01); }
    void exec_RLC_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x02); }
    void exec_RLC_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x03); }
    void exec_RLC_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x04); }
    void exec_RLC_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x05); }
    void exec_RLC_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x06); }
    void exec_RLC_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x07); }
    void exec_RRC_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x08); }
    void exec_RRC_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x09); }
    void exec_RRC_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x0A); }
    void exec_RRC_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x0B); }
    void exec_RRC_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x0C); }
    void exec_RRC_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x0D); }
    void exec_RRC_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x0E); }
    void exec_RRC_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x0F); }
    void exec_RL_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x10); }
    void exec_RL_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x11); }
    void exec_RL_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x12); }
    void exec_RL_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x13); }
    void exec_RL_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x14); }
    void exec_RL_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x15); }
    void exec_RL_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x16); }
    void exec_RL_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x17); }
    void exec_RR_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x18); }
    void exec_RR_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x19); }
    void exec_RR_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x1A); }
    void exec_RR_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x1B); }
    void exec_RR_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x1C); }
    void exec_RR_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x1D); }
    void exec_RR_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x1E); }
    void exec_RR_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x1F); }
    void exec_SLA_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x20); }
    void exec_SLA_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x21); }
    void exec_SLA_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x22); }
    void exec_SLA_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x23); }
    void exec_SLA_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x24); }
    void exec_SLA_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x25); }
    void exec_SLA_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x26); }
    void exec_SLA_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x27); }
    void exec_SRA_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x28); }
    void exec_SRA_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x29); }
    void exec_SRA_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x2A); }
    void exec_SRA_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x2B); }
    void exec_SRA_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x2C); }
    void exec_SRA_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x2D); }
    void exec_SRA_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x2E); }
    void exec_SRA_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x2F); }
    void exec_SLL_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x30); }
    void exec_SLL_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x31); }
    void exec_SLL_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x32); }
    void exec_SLL_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x33); }
    void exec_SLL_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x34); }
    void exec_SLL_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x35); }
    void exec_SLL_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x36); }
    void exec_SLL_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x37); }
    void exec_SRL_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x38); }
    void exec_SRL_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x39); }
    void exec_SRL_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x3A); }
    void exec_SRL_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x3B); }
    void exec_SRL_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x3C); }
    void exec_SRL_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x3D); }
    void exec_SRL_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x3E); }
    void exec_SRL_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x3F); }
    void exec_BIT_0_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x46); }
    void exec_BIT_1_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x4E); }
    void exec_BIT_2_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x56); }
    void exec_BIT_3_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x5E); }
    void exec_BIT_4_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x66); }
    void exec_BIT_5_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x6E); }
    void exec_BIT_6_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x76); }
    void exec_BIT_7_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x7E); }
    void exec_RES_0_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x80); }
    void exec_RES_0_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x81); }
    void exec_RES_0_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x82); }
    void exec_RES_0_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x83); }
    void exec_RES_0_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x84); }
    void exec_RES_0_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x85); }
    void exec_RES_0_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x86); }
    void exec_RES_0_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x87); }
    void exec_RES_1_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x88); }
    void exec_RES_1_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x89); }
    void exec_RES_1_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x8A); }
    void exec_RES_1_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x8B); }
    void exec_RES_1_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x8C); }
    void exec_RES_1_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x8D); }
    void exec_RES_1_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x8E); }
    void exec_RES_1_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x8F); }
    void exec_RES_2_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x90); }
    void exec_RES_2_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x91); }
    void exec_RES_2_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x92); }
    void exec_RES_2_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x93); }
    void exec_RES_2_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x94); }
    void exec_RES_2_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x95); }
    void exec_RES_2_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x96); }
    void exec_RES_2_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x97); }
    void exec_RES_3_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0x98); }
    void exec_RES_3_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0x99); }
    void exec_RES_3_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0x9A); }
    void exec_RES_3_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0x9B); }
    void exec_RES_3_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0x9C); }
    void exec_RES_3_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0x9D); }
    void exec_RES_3_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0x9E); }
    void exec_RES_3_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0x9F); }
    void exec_RES_4_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xA0); }
    void exec_RES_4_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xA1); }
    void exec_RES_4_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xA2); }
    void exec_RES_4_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xA3); }
    void exec_RES_4_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xA4); }
    void exec_RES_4_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xA5); }
    void exec_RES_4_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xA6); }
    void exec_RES_4_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xA7); }
    void exec_RES_5_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xA8); }
    void exec_RES_5_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xA9); }
    void exec_RES_5_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xAA); }
    void exec_RES_5_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xAB); }
    void exec_RES_5_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xAC); }
    void exec_RES_5_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xAD); }
    void exec_RES_5_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xAE); }
    void exec_RES_5_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xAF); }
    void exec_RES_6_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xB0); }
    void exec_RES_6_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xB1); }
    void exec_RES_6_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xB2); }
    void exec_RES_6_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xB3); }
    void exec_RES_6_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xB4); }
    void exec_RES_6_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xB5); }
    void exec_RES_6_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xB6); }
    void exec_RES_6_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xB7); }
    void exec_RES_7_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xB8); }
    void exec_RES_7_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xB9); }
    void exec_RES_7_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xBA); }
    void exec_RES_7_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xBB); }
    void exec_RES_7_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xBC); }
    void exec_RES_7_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xBD); }
    void exec_RES_7_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xBE); }
    void exec_RES_7_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xBF); }
    void exec_SET_0_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xC0); }
    void exec_SET_0_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xC1); }
    void exec_SET_0_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xC2); }
    void exec_SET_0_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xC3); }
    void exec_SET_0_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xC4); }
    void exec_SET_0_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xC5); }
    void exec_SET_0_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xC6); }
    void exec_SET_0_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xC7); }
    void exec_SET_1_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xC8); }
    void exec_SET_1_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xC9); }
    void exec_SET_1_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xCA); }
    void exec_SET_1_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xCB); }
    void exec_SET_1_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xCC); }
    void exec_SET_1_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xCD); }
    void exec_SET_1_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xCE); }
    void exec_SET_1_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xCF); }
    void exec_SET_2_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xD0); }
    void exec_SET_2_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xD1); }
    void exec_SET_2_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xD2); }
    void exec_SET_2_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xD3); }
    void exec_SET_2_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xD4); }
    void exec_SET_2_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xD5); }
    void exec_SET_2_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xD6); }
    void exec_SET_2_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xD7); }
    void exec_SET_3_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xD8); }
    void exec_SET_3_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xD9); }
    void exec_SET_3_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xDA); }
    void exec_SET_3_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xDB); }
    void exec_SET_3_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xDC); }
    void exec_SET_3_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xDD); }
    void exec_SET_3_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xDE); }
    void exec_SET_3_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xDF); }
    void exec_SET_4_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xE0); }
    void exec_SET_4_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xE1); }
    void exec_SET_4_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xE2); }
    void exec_SET_4_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xE3); }
    void exec_SET_4_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xE4); }
    void exec_SET_4_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xE5); }
    void exec_SET_4_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xE6); }
    void exec_SET_4_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xE7); }
    void exec_SET_5_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xE8); }
    void exec_SET_5_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xE9); }
    void exec_SET_5_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xEA); }
    void exec_SET_5_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xEB); }
    void exec_SET_5_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xEC); }
    void exec_SET_5_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xED); }
    void exec_SET_5_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xEE); }
    void exec_SET_5_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xEF); }
    void exec_SET_6_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xF0); }
    void exec_SET_6_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xF1); }
    void exec_SET_6_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xF2); }
    void exec_SET_6_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xF3); }
    void exec_SET_6_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xF4); }
    void exec_SET_6_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xF5); }
    void exec_SET_6_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xF6); }
    void exec_SET_6_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xF7); }
    void exec_SET_7_IY_d_ptr_B(int8_t offset) { exec_FDCB_helper(offset, 0xF8); }
    void exec_SET_7_IY_d_ptr_C(int8_t offset) { exec_FDCB_helper(offset, 0xF9); }
    void exec_SET_7_IY_d_ptr_D(int8_t offset) { exec_FDCB_helper(offset, 0xFA); }
    void exec_SET_7_IY_d_ptr_E(int8_t offset) { exec_FDCB_helper(offset, 0xFB); }
    void exec_SET_7_IY_d_ptr_H(int8_t offset) { exec_FDCB_helper(offset, 0xFC); }
    void exec_SET_7_IY_d_ptr_L(int8_t offset) { exec_FDCB_helper(offset, 0xFD); }
    void exec_SET_7_IY_d_ptr(int8_t offset) { exec_FDCB_helper(offset, 0xFE); }
    void exec_SET_7_IY_d_ptr_A(int8_t offset) { exec_FDCB_helper(offset, 0xFF); }


    // --- CB Prefixed Opcodes ---
    void exec_RLC_B() { exec_CB_helper(0x00); }
    void exec_RLC_C() { exec_CB_helper(0x01); }
    void exec_RLC_D() { exec_CB_helper(0x02); }
    void exec_RLC_E() { exec_CB_helper(0x03); }
    void exec_RLC_H() { exec_CB_helper(0x04); }
    void exec_RLC_L() { exec_CB_helper(0x05); }
    void exec_RLC_HL_ptr() { exec_CB_helper(0x06); }
    void exec_RLC_A() { exec_CB_helper(0x07); }
    void exec_RRC_B() { exec_CB_helper(0x08); }
    void exec_RRC_C() { exec_CB_helper(0x09); }
    void exec_RRC_D() { exec_CB_helper(0x0A); }
    void exec_RRC_E() { exec_CB_helper(0x0B); }
    void exec_RRC_H() { exec_CB_helper(0x0C); }
    void exec_RRC_L() { exec_CB_helper(0x0D); }
    void exec_RRC_HL_ptr() { exec_CB_helper(0x0E); }
    void exec_RRC_A() { exec_CB_helper(0x0F); }
    void exec_RL_B() { exec_CB_helper(0x10); }
    void exec_RL_C() { exec_CB_helper(0x11); }
    void exec_RL_D() { exec_CB_helper(0x12); }
    void exec_RL_E() { exec_CB_helper(0x13); }
    void exec_RL_H() { exec_CB_helper(0x14); }
    void exec_RL_L() { exec_CB_helper(0x15); }
    void exec_RL_HL_ptr() { exec_CB_helper(0x16); }
    void exec_RL_A() { exec_CB_helper(0x17); }
    void exec_RR_B() { exec_CB_helper(0x18); }
    void exec_RR_C() { exec_CB_helper(0x19); }
    void exec_RR_D() { exec_CB_helper(0x1A); }
    void exec_RR_E() { exec_CB_helper(0x1B); }
    void exec_RR_H() { exec_CB_helper(0x1C); }
    void exec_RR_L() { exec_CB_helper(0x1D); }
    void exec_RR_HL_ptr() { exec_CB_helper(0x1E); }
    void exec_RR_A() { exec_CB_helper(0x1F); }
    void exec_SLA_B() { exec_CB_helper(0x20); }
    void exec_SLA_C() { exec_CB_helper(0x21); }
    void exec_SLA_D() { exec_CB_helper(0x22); }
    void exec_SLA_E() { exec_CB_helper(0x23); }
    void exec_SLA_H() { exec_CB_helper(0x24); }
    void exec_SLA_L() { exec_CB_helper(0x25); }
    void exec_SLA_HL_ptr() { exec_CB_helper(0x26); }
    void exec_SLA_A() { exec_CB_helper(0x27); }
    void exec_SRA_B() { exec_CB_helper(0x28); }
    void exec_SRA_C() { exec_CB_helper(0x29); }
    void exec_SRA_D() { exec_CB_helper(0x2A); }
    void exec_SRA_E() { exec_CB_helper(0x2B); }
    void exec_SRA_H() { exec_CB_helper(0x2C); }
    void exec_SRA_L() { exec_CB_helper(0x2D); }
    void exec_SRA_HL_ptr() { exec_CB_helper(0x2E); }
    void exec_SRA_A() { exec_CB_helper(0x2F); }
    void exec_SLL_B() { exec_CB_helper(0x30); }
    void exec_SLL_C() { exec_CB_helper(0x31); }
    void exec_SLL_D() { exec_CB_helper(0x32); }
    void exec_SLL_E() { exec_CB_helper(0x33); }
    void exec_SLL_H() { exec_CB_helper(0x34); }
    void exec_SLL_L() { exec_CB_helper(0x35); }
    void exec_SLL_HL_ptr() { exec_CB_helper(0x36); }
    void exec_SLL_A() { exec_CB_helper(0x37); }
    void exec_SRL_B() { exec_CB_helper(0x38); }
    void exec_SRL_C() { exec_CB_helper(0x39); }
    void exec_SRL_D() { exec_CB_helper(0x3A); }
    void exec_SRL_E() { exec_CB_helper(0x3B); }
    void exec_SRL_H() { exec_CB_helper(0x3C); }
    void exec_SRL_L() { exec_CB_helper(0x3D); }
    void exec_SRL_HL_ptr() { exec_CB_helper(0x3E); }
    void exec_SRL_A() { exec_CB_helper(0x3F); }
    void exec_BIT_0_B() { exec_CB_helper(0x40); }
    void exec_BIT_0_C() { exec_CB_helper(0x41); }
    void exec_BIT_0_D() { exec_CB_helper(0x42); }
    void exec_BIT_0_E() { exec_CB_helper(0x43); }
    void exec_BIT_0_H() { exec_CB_helper(0x44); }
    void exec_BIT_0_L() { exec_CB_helper(0x45); }
    void exec_BIT_0_HL_ptr() { exec_CB_helper(0x46); }
    void exec_BIT_0_A() { exec_CB_helper(0x47); }
    void exec_BIT_1_B() { exec_CB_helper(0x48); }
    void exec_BIT_1_C() { exec_CB_helper(0x49); }
    void exec_BIT_1_D() { exec_CB_helper(0x4A); }
    void exec_BIT_1_E() { exec_CB_helper(0x4B); }
    void exec_BIT_1_H() { exec_CB_helper(0x4C); }
    void exec_BIT_1_L() { exec_CB_helper(0x4D); }
    void exec_BIT_1_HL_ptr() { exec_CB_helper(0x4E); }
    void exec_BIT_1_A() { exec_CB_helper(0x4F); }
    void exec_BIT_2_B() { exec_CB_helper(0x50); }
    void exec_BIT_2_C() { exec_CB_helper(0x51); }
    void exec_BIT_2_D() { exec_CB_helper(0x52); }
    void exec_BIT_2_E() { exec_CB_helper(0x53); }
    void exec_BIT_2_H() { exec_CB_helper(0x54); }
    void exec_BIT_2_L() { exec_CB_helper(0x55); }
    void exec_BIT_2_HL_ptr() { exec_CB_helper(0x56); }
    void exec_BIT_2_A() { exec_CB_helper(0x57); }
    void exec_BIT_3_B() { exec_CB_helper(0x58); }
    void exec_BIT_3_C() { exec_CB_helper(0x59); }
    void exec_BIT_3_D() { exec_CB_helper(0x5A); }
    void exec_BIT_3_E() { exec_CB_helper(0x5B); }
    void exec_BIT_3_H() { exec_CB_helper(0x5C); }
    void exec_BIT_3_L() { exec_CB_helper(0x5D); }
    void exec_BIT_3_HL_ptr() { exec_CB_helper(0x5E); }
    void exec_BIT_3_A() { exec_CB_helper(0x5F); }
    void exec_BIT_4_B() { exec_CB_helper(0x60); }
    void exec_BIT_4_C() { exec_CB_helper(0x61); }
    void exec_BIT_4_D() { exec_CB_helper(0x62); }
    void exec_BIT_4_E() { exec_CB_helper(0x63); }
    void exec_BIT_4_H() { exec_CB_helper(0x64); }
    void exec_BIT_4_L() { exec_CB_helper(0x65); }
    void exec_BIT_4_HL_ptr() { exec_CB_helper(0x66); }
    void exec_BIT_4_A() { exec_CB_helper(0x67); }
    void exec_BIT_5_B() { exec_CB_helper(0x68); }
    void exec_BIT_5_C() { exec_CB_helper(0x69); }
    void exec_BIT_5_D() { exec_CB_helper(0x6A); }
    void exec_BIT_5_E() { exec_CB_helper(0x6B); }
    void exec_BIT_5_H() { exec_CB_helper(0x6C); }
    void exec_BIT_5_L() { exec_CB_helper(0x6D); }
    void exec_BIT_5_HL_ptr() { exec_CB_helper(0x6E); }
    void exec_BIT_5_A() { exec_CB_helper(0x6F); }
    void exec_BIT_6_B() { exec_CB_helper(0x70); }
    void exec_BIT_6_C() { exec_CB_helper(0x71); }
    void exec_BIT_6_D() { exec_CB_helper(0x72); }
    void exec_BIT_6_E() { exec_CB_helper(0x73); }
    void exec_BIT_6_H() { exec_CB_helper(0x74); }
    void exec_BIT_6_L() { exec_CB_helper(0x75); }
    void exec_BIT_6_HL_ptr() { exec_CB_helper(0x76); }
    void exec_BIT_6_A() { exec_CB_helper(0x77); }
    void exec_BIT_7_B() { exec_CB_helper(0x78); }
    void exec_BIT_7_C() { exec_CB_helper(0x79); }
    void exec_BIT_7_D() { exec_CB_helper(0x7A); }
    void exec_BIT_7_E() { exec_CB_helper(0x7B); }
    void exec_BIT_7_H() { exec_CB_helper(0x7C); }
    void exec_BIT_7_L() { exec_CB_helper(0x7D); }
    void exec_BIT_7_HL_ptr() { exec_CB_helper(0x7E); }
    void exec_BIT_7_A() { exec_CB_helper(0x7F); }
    void exec_RES_0_B() { exec_CB_helper(0x80); }
    void exec_RES_0_C() { exec_CB_helper(0x81); }
    void exec_RES_0_D() { exec_CB_helper(0x82); }
    void exec_RES_0_E() { exec_CB_helper(0x83); }
    void exec_RES_0_H() { exec_CB_helper(0x84); }
    void exec_RES_0_L() { exec_CB_helper(0x85); }
    void exec_RES_0_HL_ptr() { exec_CB_helper(0x86); }
    void exec_RES_0_A() { exec_CB_helper(0x87); }
    void exec_RES_1_B() { exec_CB_helper(0x88); }
    void exec_RES_1_C() { exec_CB_helper(0x89); }
    void exec_RES_1_D() { exec_CB_helper(0x8A); }
    void exec_RES_1_E() { exec_CB_helper(0x8B); }
    void exec_RES_1_H() { exec_CB_helper(0x8C); }
    void exec_RES_1_L() { exec_CB_helper(0x8D); }
    void exec_RES_1_HL_ptr() { exec_CB_helper(0x8E); }
    void exec_RES_1_A() { exec_CB_helper(0x8F); }
    void exec_RES_2_B() { exec_CB_helper(0x90); }
    void exec_RES_2_C() { exec_CB_helper(0x91); }
    void exec_RES_2_D() { exec_CB_helper(0x92); }
    void exec_RES_2_E() { exec_CB_helper(0x93); }
    void exec_RES_2_H() { exec_CB_helper(0x94); }
    void exec_RES_2_L() { exec_CB_helper(0x95); }
    void exec_RES_2_HL_ptr() { exec_CB_helper(0x96); }
    void exec_RES_2_A() { exec_CB_helper(0x97); }
    void exec_RES_3_B() { exec_CB_helper(0x98); }
    void exec_RES_3_C() { exec_CB_helper(0x99); }
    void exec_RES_3_D() { exec_CB_helper(0x9A); }
    void exec_RES_3_E() { exec_CB_helper(0x9B); }
    void exec_RES_3_H() { exec_CB_helper(0x9C); }
    void exec_RES_3_L() { exec_CB_helper(0x9D); }
    void exec_RES_3_HL_ptr() { exec_CB_helper(0x9E); }
    void exec_RES_3_A() { exec_CB_helper(0x9F); }
    void exec_RES_4_B() { exec_CB_helper(0xA0); }
    void exec_RES_4_C() { exec_CB_helper(0xA1); }
    void exec_RES_4_D() { exec_CB_helper(0xA2); }
    void exec_RES_4_E() { exec_CB_helper(0xA3); }
    void exec_RES_4_H() { exec_CB_helper(0xA4); }
    void exec_RES_4_L() { exec_CB_helper(0xA5); }
    void exec_RES_4_HL_ptr() { exec_CB_helper(0xA6); }
    void exec_RES_4_A() { exec_CB_helper(0xA7); }
    void exec_RES_5_B() { exec_CB_helper(0xA8); }
    void exec_RES_5_C() { exec_CB_helper(0xA9); }
    void exec_RES_5_D() { exec_CB_helper(0xAA); }
    void exec_RES_5_E() { exec_CB_helper(0xAB); }
    void exec_RES_5_H() { exec_CB_helper(0xAC); }
    void exec_RES_5_L() { exec_CB_helper(0xAD); }
    void exec_RES_5_HL_ptr() { exec_CB_helper(0xAE); }
    void exec_RES_5_A() { exec_CB_helper(0xAF); }
    void exec_RES_6_B() { exec_CB_helper(0xB0); }
    void exec_RES_6_C() { exec_CB_helper(0xB1); }
    void exec_RES_6_D() { exec_CB_helper(0xB2); }
    void exec_RES_6_E() { exec_CB_helper(0xB3); }
    void exec_RES_6_H() { exec_CB_helper(0xB4); }
    void exec_RES_6_L() { exec_CB_helper(0xB5); }
    void exec_RES_6_HL_ptr() { exec_CB_helper(0xB6); }
    void exec_RES_6_A() { exec_CB_helper(0xB7); }
    void exec_RES_7_B() { exec_CB_helper(0xB8); }
    void exec_RES_7_C() { exec_CB_helper(0xB9); }
    void exec_RES_7_D() { exec_CB_helper(0xBA); }
    void exec_RES_7_E() { exec_CB_helper(0xBB); }
    void exec_RES_7_H() { exec_CB_helper(0xBC); }
    void exec_RES_7_L() { exec_CB_helper(0xBD); }
    void exec_RES_7_HL_ptr() { exec_CB_helper(0xBE); }
    void exec_RES_7_A() { exec_CB_helper(0xBF); }
    void exec_SET_0_B() { exec_CB_helper(0xC0); }
    void exec_SET_0_C() { exec_CB_helper(0xC1); }
    void exec_SET_0_D() { exec_CB_helper(0xC2); }
    void exec_SET_0_E() { exec_CB_helper(0xC3); }
    void exec_SET_0_H() { exec_CB_helper(0xC4); }
    void exec_SET_0_L() { exec_CB_helper(0xC5); }
    void exec_SET_0_HL_ptr() { exec_CB_helper(0xC6); }
    void exec_SET_0_A() { exec_CB_helper(0xC7); }
    void exec_SET_1_B() { exec_CB_helper(0xC8); }
    void exec_SET_1_C() { exec_CB_helper(0xC9); }
    void exec_SET_1_D() { exec_CB_helper(0xCA); }
    void exec_SET_1_E() { exec_CB_helper(0xCB); }
    void exec_SET_1_H() { exec_CB_helper(0xCC); }
    void exec_SET_1_L() { exec_CB_helper(0xCD); }
    void exec_SET_1_HL_ptr() { exec_CB_helper(0xCE); }
    void exec_SET_1_A() { exec_CB_helper(0xCF); }
    void exec_SET_2_B() { exec_CB_helper(0xD0); }
    void exec_SET_2_C() { exec_CB_helper(0xD1); }
    void exec_SET_2_D() { exec_CB_helper(0xD2); }
    void exec_SET_2_E() { exec_CB_helper(0xD3); }
    void exec_SET_2_H() { exec_CB_helper(0xD4); }
    void exec_SET_2_L() { exec_CB_helper(0xD5); }
    void exec_SET_2_HL_ptr() { exec_CB_helper(0xD6); }
    void exec_SET_2_A() { exec_CB_helper(0xD7); }
    void exec_SET_3_B() { exec_CB_helper(0xD8); }
    void exec_SET_3_C() { exec_CB_helper(0xD9); }
    void exec_SET_3_D() { exec_CB_helper(0xDA); }
    void exec_SET_3_E() { exec_CB_helper(0xDB); }
    void exec_SET_3_H() { exec_CB_helper(0xDC); }
    void exec_SET_3_L() { exec_CB_helper(0xDD); }
    void exec_SET_3_HL_ptr() { exec_CB_helper(0xDE); }
    void exec_SET_3_A() { exec_CB_helper(0xDF); }
    void exec_SET_4_B() { exec_CB_helper(0xE0); }
    void exec_SET_4_C() { exec_CB_helper(0xE1); }
    void exec_SET_4_D() { exec_CB_helper(0xE2); }
    void exec_SET_4_E() { exec_CB_helper(0xE3); }
    void exec_SET_4_H() { exec_CB_helper(0xE4); }
    void exec_SET_4_L() { exec_CB_helper(0xE5); }
    void exec_SET_4_HL_ptr() { exec_CB_helper(0xE6); }
    void exec_SET_4_A() { exec_CB_helper(0xE7); }
    void exec_SET_5_B() { exec_CB_helper(0xE8); }
    void exec_SET_5_C() { exec_CB_helper(0xE9); }
    void exec_SET_5_D() { exec_CB_helper(0xEA); }
    void exec_SET_5_E() { exec_CB_helper(0xEB); }
    void exec_SET_5_H() { exec_CB_helper(0xEC); }
    void exec_SET_5_L() { exec_CB_helper(0xED); }
    void exec_SET_5_HL_ptr() { exec_CB_helper(0xEE); }
    void exec_SET_5_A() { exec_CB_helper(0xEF); }
    void exec_SET_6_B() { exec_CB_helper(0xF0); }
    void exec_SET_6_C() { exec_CB_helper(0xF1); }
    void exec_SET_6_D() { exec_CB_helper(0xF2); }
    void exec_SET_6_E() { exec_CB_helper(0xF3); }
    void exec_SET_6_H() { exec_CB_helper(0xF4); }
    void exec_SET_6_L() { exec_CB_helper(0xF5); }
    void exec_SET_6_HL_ptr() { exec_CB_helper(0xF6); }
    void exec_SET_6_A() { exec_CB_helper(0xF7); }
    void exec_SET_7_B() { exec_CB_helper(0xF8); }
    void exec_SET_7_C() { exec_CB_helper(0xF9); }
    void exec_SET_7_D() { exec_CB_helper(0xFA); }
    void exec_SET_7_E() { exec_CB_helper(0xFB); }
    void exec_SET_7_H() { exec_CB_helper(0xFC); }
    void exec_SET_7_L() { exec_CB_helper(0xFD); }
    void exec_SET_7_HL_ptr() { exec_CB_helper(0xFE); }
    void exec_SET_7_A() { exec_CB_helper(0xFF); }