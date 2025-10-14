public:

    void exec_NOP() {
        add_ticks(4);
        handle_opcode_0x00_NOP();
    }

    void exec_LD_BC_nn() {
        add_ticks(4);
        handle_opcode_0x01_LD_BC_nn();
    }

    void exec_LD_BC_ptr_A() {
        add_ticks(4);
        handle_opcode_0x02_LD_BC_ptr_A();
    }

    void exec_INC_BC() {
        add_ticks(4);
        handle_opcode_0x03_INC_BC();
    }

    void exec_INC_B() {
        add_ticks(4);
        handle_opcode_0x04_INC_B();
    }

    void exec_DEC_B() {
        add_ticks(4);
        handle_opcode_0x05_DEC_B();
    }

    void exec_LD_B_n() {
        add_ticks(4);
        handle_opcode_0x06_LD_B_n();
    }

    void exec_RLCA() {
        add_ticks(4);
        handle_opcode_0x07_RLCA();
    }

    void exec_EX_AF_AFp() {
        add_ticks(4);
        handle_opcode_0x08_EX_AF_AFp();
    }

    void exec_ADD_HL_BC() {
        add_ticks(4);
        handle_opcode_0x09_ADD_HL_BC();
    }

    void exec_LD_A_BC_ptr() {
        add_ticks(4);
        handle_opcode_0x0A_LD_A_BC_ptr();
    }

    void exec_DEC_BC() {
        add_ticks(4);
        handle_opcode_0x0B_DEC_BC();
    }

    void exec_INC_C() {
        add_ticks(4);
        handle_opcode_0x0C_INC_C();
    }

    void exec_DEC_C() {
        add_ticks(4);
        handle_opcode_0x0D_DEC_C();
    }

    void exec_LD_C_n() {
        add_ticks(4);
        handle_opcode_0x0E_LD_C_n();
    }

    void exec_RRCA() {
        add_ticks(4);
        handle_opcode_0x0F_RRCA();
    }

    void exec_DJNZ_d() {
        add_ticks(4);
        handle_opcode_0x10_DJNZ_d();
    }

    void exec_LD_DE_nn() {
        add_ticks(4);
        handle_opcode_0x11_LD_DE_nn();
    }

    void exec_LD_DE_ptr_A() {
        add_ticks(4);
        handle_opcode_0x12_LD_DE_ptr_A();
    }

    void exec_INC_DE() {
        add_ticks(4);
        handle_opcode_0x13_INC_DE();
    }

    void exec_INC_D() {
        add_ticks(4);
        handle_opcode_0x14_INC_D();
    }

    void exec_DEC_D() {
        add_ticks(4);
        handle_opcode_0x15_DEC_D();
    }

    void exec_LD_D_n() {
        add_ticks(4);
        handle_opcode_0x16_LD_D_n();
    }

    void exec_RLA() {
        add_ticks(4);
        handle_opcode_0x17_RLA();
    }

    void exec_JR_d() {
        add_ticks(4);
        handle_opcode_0x18_JR_d();
    }

    void exec_ADD_HL_DE() {
        add_ticks(4);
        handle_opcode_0x19_ADD_HL_DE();
    }

    void exec_LD_A_DE_ptr() {
        add_ticks(4);
        handle_opcode_0x1A_LD_A_DE_ptr();
    }

    void exec_DEC_DE() {
        add_ticks(4);
        handle_opcode_0x1B_DEC_DE();
    }

    void exec_INC_E() {
        add_ticks(4);
        handle_opcode_0x1C_INC_E();
    }

    void exec_DEC_E() {
        add_ticks(4);
        handle_opcode_0x1D_DEC_E();
    }

    void exec_LD_E_n() {
        add_ticks(4);
        handle_opcode_0x1E_LD_E_n();
    }

    void exec_RRA() {
        add_ticks(4);
        handle_opcode_0x1F_RRA();
    }

    void exec_JR_NZ_d() {
        add_ticks(4);
        handle_opcode_0x20_JR_NZ_d();
    }

    void exec_LD_HL_nn() {
        add_ticks(4);
        handle_opcode_0x21_LD_HL_nn();
    }

    void exec_LD_nn_ptr_HL() {
        add_ticks(4);
        handle_opcode_0x22_LD_nn_ptr_HL();
    }

    void exec_INC_HL() {
        add_ticks(4);
        handle_opcode_0x23_INC_HL();
    }

    void exec_INC_H() {
        add_ticks(4);
        handle_opcode_0x24_INC_H();
    }

    void exec_DEC_H() {
        add_ticks(4);
        handle_opcode_0x25_DEC_H();
    }

    void exec_LD_H_n() {
        add_ticks(4);
        handle_opcode_0x26_LD_H_n();
    }

    void exec_DAA() {
        add_ticks(4);
        handle_opcode_0x27_DAA();
    }

    void exec_JR_Z_d() {
        add_ticks(4);
        handle_opcode_0x28_JR_Z_d();
    }

    void exec_ADD_HL_HL() {
        add_ticks(4);
        handle_opcode_0x29_ADD_HL_HL();
    }

    void exec_LD_HL_nn_ptr() {
        add_ticks(4);
        handle_opcode_0x2A_LD_HL_nn_ptr();
    }

    void exec_DEC_HL() {
        add_ticks(4);
        handle_opcode_0x2B_DEC_HL();
    }

    void exec_INC_L() {
        add_ticks(4);
        handle_opcode_0x2C_INC_L();
    }

    void exec_DEC_L() {
        add_ticks(4);
        handle_opcode_0x2D_DEC_L();
    }

    void exec_LD_L_n() {
        add_ticks(4);
        handle_opcode_0x2E_LD_L_n();
    }

    void exec_CPL() {
        add_ticks(4);
        handle_opcode_0x2F_CPL();
    }

    void exec_JR_NC_d() {
        add_ticks(4);
        handle_opcode_0x30_JR_NC_d();
    }

    void exec_LD_SP_nn() {
        add_ticks(4);
        handle_opcode_0x31_LD_SP_nn();
    }

    void exec_LD_nn_ptr_A() {
        add_ticks(4);
        handle_opcode_0x32_LD_nn_ptr_A();
    }

    void exec_INC_SP() {
        add_ticks(4);
        handle_opcode_0x33_INC_SP();
    }

    void exec_INC_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x34_INC_HL_ptr();
    }

    void exec_DEC_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x35_DEC_HL_ptr();
    }

    void exec_LD_HL_ptr_n() {
        add_ticks(4);
        handle_opcode_0x36_LD_HL_ptr_n();
    }

    void exec_SCF() {
        add_ticks(4);
        handle_opcode_0x37_SCF();
    }

    void exec_JR_C_d() {
        add_ticks(4);
        handle_opcode_0x38_JR_C_d();
    }

    void exec_ADD_HL_SP() {
        add_ticks(4);
        handle_opcode_0x39_ADD_HL_SP();
    }

    void exec_LD_A_nn_ptr() {
        add_ticks(4);
        handle_opcode_0x3A_LD_A_nn_ptr();
    }

    void exec_DEC_SP() {
        add_ticks(4);
        handle_opcode_0x3B_DEC_SP();
    }

    void exec_INC_A() {
        add_ticks(4);
        handle_opcode_0x3C_INC_A();
    }

    void exec_DEC_A() {
        add_ticks(4);
        handle_opcode_0x3D_DEC_A();
    }

    void exec_LD_A_n() {
        add_ticks(4);
        handle_opcode_0x3E_LD_A_n();
    }

    void exec_CCF() {
        add_ticks(4);
        handle_opcode_0x3F_CCF();
    }

    void exec_LD_B_B() {
        add_ticks(4);
        handle_opcode_0x40_LD_B_B();
    }

    void exec_LD_B_C() {
        add_ticks(4);
        handle_opcode_0x41_LD_B_C();
    }

    void exec_LD_B_D() {
        add_ticks(4);
        handle_opcode_0x42_LD_B_D();
    }

    void exec_LD_B_E() {
        add_ticks(4);
        handle_opcode_0x43_LD_B_E();
    }

    void exec_LD_B_H() {
        add_ticks(4);
        handle_opcode_0x44_LD_B_H();
    }

    void exec_LD_B_L() {
        add_ticks(4);
        handle_opcode_0x45_LD_B_L();
    }

    void exec_LD_B_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x46_LD_B_HL_ptr();
    }

    void exec_LD_B_A() {
        add_ticks(4);
        handle_opcode_0x47_LD_B_A();
    }

    void exec_LD_C_B() {
        add_ticks(4);
        handle_opcode_0x48_LD_C_B();
    }

    void exec_LD_C_C() {
        add_ticks(4);
        handle_opcode_0x49_LD_C_C();
    }

    void exec_LD_C_D() {
        add_ticks(4);
        handle_opcode_0x4A_LD_C_D();
    }

    void exec_LD_C_E() {
        add_ticks(4);
        handle_opcode_0x4B_LD_C_E();
    }

    void exec_LD_C_H() {
        add_ticks(4);
        handle_opcode_0x4C_LD_C_H();
    }

    void exec_LD_C_L() {
        add_ticks(4);
        handle_opcode_0x4D_LD_C_L();
    }

    void exec_LD_C_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x4E_LD_C_HL_ptr();
    }

    void exec_LD_C_A() {
        add_ticks(4);
        handle_opcode_0x4F_LD_C_A();
    }

    void exec_LD_D_B() {
        add_ticks(4);
        handle_opcode_0x50_LD_D_B();
    }

    void exec_LD_D_C() {
        add_ticks(4);
        handle_opcode_0x51_LD_D_C();
    }

    void exec_LD_D_D() {
        add_ticks(4);
        handle_opcode_0x52_LD_D_D();
    }

    void exec_LD_D_E() {
        add_ticks(4);
        handle_opcode_0x53_LD_D_E();
    }

    void exec_LD_D_H() {
        add_ticks(4);
        handle_opcode_0x54_LD_D_H();
    }

    void exec_LD_D_L() {
        add_ticks(4);
        handle_opcode_0x55_LD_D_L();
    }

    void exec_LD_D_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x56_LD_D_HL_ptr();
    }

    void exec_LD_D_A() {
        add_ticks(4);
        handle_opcode_0x57_LD_D_A();
    }

    void exec_LD_E_B() {
        add_ticks(4);
        handle_opcode_0x58_LD_E_B();
    }

    void exec_LD_E_C() {
        add_ticks(4);
        handle_opcode_0x59_LD_E_C();
    }

    void exec_LD_E_D() {
        add_ticks(4);
        handle_opcode_0x5A_LD_E_D();
    }

    void exec_LD_E_E() {
        add_ticks(4);
        handle_opcode_0x5B_LD_E_E();
    }

    void exec_LD_E_H() {
        add_ticks(4);
        handle_opcode_0x5C_LD_E_H();
    }

    void exec_LD_E_L() {
        add_ticks(4);
        handle_opcode_0x5D_LD_E_L();
    }

    void exec_LD_E_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x5E_LD_E_HL_ptr();
    }

    void exec_LD_E_A() {
        add_ticks(4);
        handle_opcode_0x5F_LD_E_A();
    }

    void exec_LD_H_B() {
        add_ticks(4);
        handle_opcode_0x60_LD_H_B();
    }

    void exec_LD_H_C() {
        add_ticks(4);
        handle_opcode_0x61_LD_H_C();
    }

    void exec_LD_H_D() {
        add_ticks(4);
        handle_opcode_0x62_LD_H_D();
    }

    void exec_LD_H_E() {
        add_ticks(4);
        handle_opcode_0x63_LD_H_E();
    }

    void exec_LD_H_H() {
        add_ticks(4);
        handle_opcode_0x64_LD_H_H();
    }

    void exec_LD_H_L() {
        add_ticks(4);
        handle_opcode_0x65_LD_H_L();
    }

    void exec_LD_H_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x66_LD_H_HL_ptr();
    }

    void exec_LD_H_A() {
        add_ticks(4);
        handle_opcode_0x67_LD_H_A();
    }

    void exec_LD_L_B() {
        add_ticks(4);
        handle_opcode_0x68_LD_L_B();
    }

    void exec_LD_L_C() {
        add_ticks(4);
        handle_opcode_0x69_LD_L_C();
    }

    void exec_LD_L_D() {
        add_ticks(4);
        handle_opcode_0x6A_LD_L_D();
    }

    void exec_LD_L_E() {
        add_ticks(4);
        handle_opcode_0x6B_LD_L_E();
    }

    void exec_LD_L_H() {
        add_ticks(4);
        handle_opcode_0x6C_LD_L_H();
    }

    void exec_LD_L_L() {
        add_ticks(4);
        handle_opcode_0x6D_LD_L_L();
    }

    void exec_LD_L_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x6E_LD_L_HL_ptr();
    }

    void exec_LD_L_A() {
        add_ticks(4);
        handle_opcode_0x6F_LD_L_A();
    }

    void exec_LD_HL_ptr_B() {
        add_ticks(4);
        handle_opcode_0x70_LD_HL_ptr_B();
    }

    void exec_LD_HL_ptr_C() {
        add_ticks(4);
        handle_opcode_0x71_LD_HL_ptr_C();
    }

    void exec_LD_HL_ptr_D() {
        add_ticks(4);
        handle_opcode_0x72_LD_HL_ptr_D();
    }

    void exec_LD_HL_ptr_E() {
        add_ticks(4);
        handle_opcode_0x73_LD_HL_ptr_E();
    }

    void exec_LD_HL_ptr_H() {
        add_ticks(4);
        handle_opcode_0x74_LD_HL_ptr_H();
    }

    void exec_LD_HL_ptr_L() {
        add_ticks(4);
        handle_opcode_0x75_LD_HL_ptr_L();
    }

    void exec_HALT() {
        add_ticks(4);
        handle_opcode_0x76_HALT();
    }

    void exec_LD_HL_ptr_A() {
        add_ticks(4);
        handle_opcode_0x77_LD_HL_ptr_A();
    }

    void exec_LD_A_B() {
        add_ticks(4);
        handle_opcode_0x78_LD_A_B();
    }

    void exec_LD_A_C() {
        add_ticks(4);
        handle_opcode_0x79_LD_A_C();
    }

    void exec_LD_A_D() {
        add_ticks(4);
        handle_opcode_0x7A_LD_A_D();
    }

    void exec_LD_A_E() {
        add_ticks(4);
        handle_opcode_0x7B_LD_A_E();
    }

    void exec_LD_A_H() {
        add_ticks(4);
        handle_opcode_0x7C_LD_A_H();
    }

    void exec_LD_A_L() {
        add_ticks(4);
        handle_opcode_0x7D_LD_A_L();
    }

    void exec_LD_A_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x7E_LD_A_HL_ptr();
    }

    void exec_LD_A_A() {
        add_ticks(4);
        handle_opcode_0x7F_LD_A_A();
    }

    void exec_ADD_A_B() {
        add_ticks(4);
        handle_opcode_0x80_ADD_A_B();
    }

    void exec_ADD_A_C() {
        add_ticks(4);
        handle_opcode_0x81_ADD_A_C();
    }

    void exec_ADD_A_D() {
        add_ticks(4);
        handle_opcode_0x82_ADD_A_D();
    }

    void exec_ADD_A_E() {
        add_ticks(4);
        handle_opcode_0x83_ADD_A_E();
    }

    void exec_ADD_A_H() {
        add_ticks(4);
        handle_opcode_0x84_ADD_A_H();
    }

    void exec_ADD_A_L() {
        add_ticks(4);
        handle_opcode_0x85_ADD_A_L();
    }

    void exec_ADD_A_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x86_ADD_A_HL_ptr();
    }

    void exec_ADD_A_A() {
        add_ticks(4);
        handle_opcode_0x87_ADD_A_A();
    }

    void exec_ADC_A_B() {
        add_ticks(4);
        handle_opcode_0x88_ADC_A_B();
    }

    void exec_ADC_A_C() {
        add_ticks(4);
        handle_opcode_0x89_ADC_A_C();
    }

    void exec_ADC_A_D() {
        add_ticks(4);
        handle_opcode_0x8A_ADC_A_D();
    }

    void exec_ADC_A_E() {
        add_ticks(4);
        handle_opcode_0x8B_ADC_A_E();
    }

    void exec_ADC_A_H() {
        add_ticks(4);
        handle_opcode_0x8C_ADC_A_H();
    }

    void exec_ADC_A_L() {
        add_ticks(4);
        handle_opcode_0x8D_ADC_A_L();
    }

    void exec_ADC_A_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x8E_ADC_A_HL_ptr();
    }

    void exec_ADC_A_A() {
        add_ticks(4);
        handle_opcode_0x8F_ADC_A_A();
    }

    void exec_SUB_B() {
        add_ticks(4);
        handle_opcode_0x90_SUB_B();
    }

    void exec_SUB_C() {
        add_ticks(4);
        handle_opcode_0x91_SUB_C();
    }

    void exec_SUB_D() {
        add_ticks(4);
        handle_opcode_0x92_SUB_D();
    }

    void exec_SUB_E() {
        add_ticks(4);
        handle_opcode_0x93_SUB_E();
    }

    void exec_SUB_H() {
        add_ticks(4);
        handle_opcode_0x94_SUB_H();
    }

    void exec_SUB_L() {
        add_ticks(4);
        handle_opcode_0x95_SUB_L();
    }

    void exec_SUB_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x96_SUB_HL_ptr();
    }

    void exec_SUB_A() {
        add_ticks(4);
        handle_opcode_0x97_SUB_A();
    }

    void exec_SBC_A_B() {
        add_ticks(4);
        handle_opcode_0x98_SBC_A_B();
    }

    void exec_SBC_A_C() {
        add_ticks(4);
        handle_opcode_0x99_SBC_A_C();
    }

    void exec_SBC_A_D() {
        add_ticks(4);
        handle_opcode_0x9A_SBC_A_D();
    }

    void exec_SBC_A_E() {
        add_ticks(4);
        handle_opcode_0x9B_SBC_A_E();
    }

    void exec_SBC_A_H() {
        add_ticks(4);
        handle_opcode_0x9C_SBC_A_H();
    }

    void exec_SBC_A_L() {
        add_ticks(4);
        handle_opcode_0x9D_SBC_A_L();
    }

    void exec_SBC_A_HL_ptr() {
        add_ticks(4);
        handle_opcode_0x9E_SBC_A_HL_ptr();
    }

    void exec_SBC_A_A() {
        add_ticks(4);
        handle_opcode_0x9F_SBC_A_A();
    }

    void exec_AND_B() {
        add_ticks(4);
        handle_opcode_0xA0_AND_B();
    }

    void exec_AND_C() {
        add_ticks(4);
        handle_opcode_0xA1_AND_C();
    }

    void exec_AND_D() {
        add_ticks(4);
        handle_opcode_0xA2_AND_D();
    }

    void exec_AND_E() {
        add_ticks(4);
        handle_opcode_0xA3_AND_E();
    }

    void exec_AND_H() {
        add_ticks(4);
        handle_opcode_0xA4_AND_H();
    }

    void exec_AND_L() {
        add_ticks(4);
        handle_opcode_0xA5_AND_L();
    }

    void exec_AND_HL_ptr() {
        add_ticks(4);
        handle_opcode_0xA6_AND_HL_ptr();
    }

    void exec_AND_A() {
        add_ticks(4);
        handle_opcode_0xA7_AND_A();
    }

    void exec_XOR_B() {
        add_ticks(4);
        handle_opcode_0xA8_XOR_B();
    }

    void exec_XOR_C() {
        add_ticks(4);
        handle_opcode_0xA9_XOR_C();
    }

    void exec_XOR_D() {
        add_ticks(4);
        handle_opcode_0xAA_XOR_D();
    }

    void exec_XOR_E() {
        add_ticks(4);
        handle_opcode_0xAB_XOR_E();
    }

    void exec_XOR_H() {
        add_ticks(4);
        handle_opcode_0xAC_XOR_H();
    }

    void exec_XOR_L() {
        add_ticks(4);
        handle_opcode_0xAD_XOR_L();
    }

    void exec_XOR_HL_ptr() {
        add_ticks(4);
        handle_opcode_0xAE_XOR_HL_ptr();
    }

    void exec_XOR_A() {
        add_ticks(4);
        handle_opcode_0xAF_XOR_A();
    }

    void exec_OR_B() {
        add_ticks(4);
        handle_opcode_0xB0_OR_B();
    }

    void exec_OR_C() {
        add_ticks(4);
        handle_opcode_0xB1_OR_C();
    }

    void exec_OR_D() {
        add_ticks(4);
        handle_opcode_0xB2_OR_D();
    }

    void exec_OR_E() {
        add_ticks(4);
        handle_opcode_0xB3_OR_E();
    }

    void exec_OR_H() {
        add_ticks(4);
        handle_opcode_0xB4_OR_H();
    }

    void exec_OR_L() {
        add_ticks(4);
        handle_opcode_0xB5_OR_L();
    }

    void exec_OR_HL_ptr() {
        add_ticks(4);
        handle_opcode_0xB6_OR_HL_ptr();
    }

    void exec_OR_A() {
        add_ticks(4);
        handle_opcode_0xB7_OR_A();
    }

    void exec_CP_B() {
        add_ticks(4);
        handle_opcode_0xB8_CP_B();
    }

    void exec_CP_C() {
        add_ticks(4);
        handle_opcode_0xB9_CP_C();
    }

    void exec_CP_D() {
        add_ticks(4);
        handle_opcode_0xBA_CP_D();
    }

    void exec_CP_E() {
        add_ticks(4);
        handle_opcode_0xBB_CP_E();
    }

    void exec_CP_H() {
        add_ticks(4);
        handle_opcode_0xBC_CP_H();
    }

    void exec_CP_L() {
        add_ticks(4);
        handle_opcode_0xBD_CP_L();
    }

    void exec_CP_HL_ptr() {
        add_ticks(4);
        handle_opcode_0xBE_CP_HL_ptr();
    }

    void exec_CP_A() {
        add_ticks(4);
        handle_opcode_0xBF_CP_A();
    }

    void exec_RET_NZ() {
        add_ticks(4);
        handle_opcode_0xC0_RET_NZ();
    }

    void exec_POP_BC() {
        add_ticks(4);
        handle_opcode_0xC1_POP_BC();
    }

    void exec_JP_NZ_nn() {
        add_ticks(4);
        handle_opcode_0xC2_JP_NZ_nn();
    }

    void exec_JP_nn() {
        add_ticks(4);
        handle_opcode_0xC3_JP_nn();
    }

    void exec_CALL_NZ_nn() {
        add_ticks(4);
        handle_opcode_0xC4_CALL_NZ_nn();
    }

    void exec_PUSH_BC() {
        add_ticks(4);
        handle_opcode_0xC5_PUSH_BC();
    }

    void exec_ADD_A_n() {
        add_ticks(4);
        handle_opcode_0xC6_ADD_A_n();
    }

    void exec_RST_00H() {
        add_ticks(4);
        handle_opcode_0xC7_RST_00H();
    }

    void exec_RET_Z() {
        add_ticks(4);
        handle_opcode_0xC8_RET_Z();
    }

    void exec_RET() {
        add_ticks(4);
        handle_opcode_0xC9_RET();
    }

    void exec_JP_Z_nn() {
        add_ticks(4);
        handle_opcode_0xCA_JP_Z_nn();
    }

    void exec_CALL_Z_nn() {
        add_ticks(4);
        handle_opcode_0xCC_CALL_Z_nn();
    }

    void exec_CALL_nn() {
        add_ticks(4);
        handle_opcode_0xCD_CALL_nn();
    }

    void exec_ADC_A_n() {
        add_ticks(4);
        handle_opcode_0xCE_ADC_A_n();
    }

    void exec_RST_08H() {
        add_ticks(4);
        handle_opcode_0xCF_RST_08H();
    }

    void exec_RET_NC() {
        add_ticks(4);
        handle_opcode_0xD0_RET_NC();
    }

    void exec_POP_DE() {
        add_ticks(4);
        handle_opcode_0xD1_POP_DE();
    }

    void exec_JP_NC_nn() {
        add_ticks(4);
        handle_opcode_0xD2_JP_NC_nn();
    }

    void exec_OUT_n_ptr_A() {
        add_ticks(4);
        handle_opcode_0xD3_OUT_n_ptr_A();
    }

    void exec_CALL_NC_nn() {
        add_ticks(4);
        handle_opcode_0xD4_CALL_NC_nn();
    }

    void exec_PUSH_DE() {
        add_ticks(4);
        handle_opcode_0xD5_PUSH_DE();
    }

    void exec_SUB_n() {
        add_ticks(4);
        handle_opcode_0xD6_SUB_n();
    }

    void exec_RST_10H() {
        add_ticks(4);
        handle_opcode_0xD7_RST_10H();
    }

    void exec_RET_C() {
        add_ticks(4);
        handle_opcode_0xD8_RET_C();
    }

    void exec_EXX() {
        add_ticks(4);
        handle_opcode_0xD9_EXX();
    }

    void exec_JP_C_nn() {
        add_ticks(4);
        handle_opcode_0xDA_JP_C_nn();
    }

    void exec_IN_A_n_ptr() {
        add_ticks(4);
        handle_opcode_0xDB_IN_A_n_ptr();
    }

    void exec_CALL_C_nn() {
        add_ticks(4);
        handle_opcode_0xDC_CALL_C_nn();
    }

    void exec_SBC_A_n() {
        add_ticks(4);
        handle_opcode_0xDE_SBC_A_n();
    }

    void exec_RST_18H() {
        add_ticks(4);
        handle_opcode_0xDF_RST_18H();
    }

    void exec_RET_PO() {
        add_ticks(4);
        handle_opcode_0xE0_RET_PO();
    }

    void exec_POP_HL() {
        add_ticks(4);
        handle_opcode_0xE1_POP_HL();
    }

    void exec_JP_PO_nn() {
        add_ticks(4);
        handle_opcode_0xE2_JP_PO_nn();
    }

    void exec_EX_SP_ptr_HL() {
        add_ticks(4);
        handle_opcode_0xE3_EX_SP_ptr_HL();
    }

    void exec_CALL_PO_nn() {
        add_ticks(4);
        handle_opcode_0xE4_CALL_PO_nn();
    }

    void exec_PUSH_HL() {
        add_ticks(4);
        handle_opcode_0xE5_PUSH_HL();
    }

    void exec_AND_n() {
        add_ticks(4);
        handle_opcode_0xE6_AND_n();
    }

    void exec_RST_20H() {
        add_ticks(4);
        handle_opcode_0xE7_RST_20H();
    }

    void exec_RET_PE() {
        add_ticks(4);
        handle_opcode_0xE8_RET_PE();
    }

    void exec_JP_HL_ptr() {
        add_ticks(4);
        handle_opcode_0xE9_JP_HL_ptr();
    }

    void exec_JP_PE_nn() {
        add_ticks(4);
        handle_opcode_0xEA_JP_PE_nn();
    }

    void exec_EX_DE_HL() {
        add_ticks(4);
        handle_opcode_0xEB_EX_DE_HL();
    }

    void exec_CALL_PE_nn() {
        add_ticks(4);
        handle_opcode_0xEC_CALL_PE_nn();
    }

    void exec_XOR_n() {
        add_ticks(4);
        handle_opcode_0xEE_XOR_n();
    }

    void exec_RST_28H() {
        add_ticks(4);
        handle_opcode_0xEF_RST_28H();
    }

    void exec_RET_P() {
        add_ticks(4);
        handle_opcode_0xF0_RET_P();
    }

    void exec_POP_AF() {
        add_ticks(4);
        handle_opcode_0xF1_POP_AF();
    }

    void exec_JP_P_nn() {
        add_ticks(4);
        handle_opcode_0xF2_JP_P_nn();
    }

    void exec_DI() {
        add_ticks(4);
        handle_opcode_0xF3_DI();
    }

    void exec_CALL_P_nn() {
        add_ticks(4);
        handle_opcode_0xF4_CALL_P_nn();
    }

    void exec_PUSH_AF() {
        add_ticks(4);
        handle_opcode_0xF5_PUSH_AF();
    }

    void exec_OR_n() {
        add_ticks(4);
        handle_opcode_0xF6_OR_n();
    }

    void exec_RST_30H() {
        add_ticks(4);
        handle_opcode_0xF7_RST_30H();
    }

    void exec_RET_M() {
        add_ticks(4);
        handle_opcode_0xF8_RET_M();
    }

    void exec_LD_SP_HL() {
        add_ticks(4);
        handle_opcode_0xF9_LD_SP_HL();
    }

    void exec_JP_M_nn() {
        add_ticks(4);
        handle_opcode_0xFA_JP_M_nn();
    }

    void exec_EI() {
        add_ticks(4);
        handle_opcode_0xFB_EI();
    }

    void exec_CALL_M_nn() {
        add_ticks(4);
        handle_opcode_0xFC_CALL_M_nn();
    }

    void exec_CP_n() {
        add_ticks(4);
        handle_opcode_0xFE_CP_n();
    }

    void exec_RST_38H() {
        add_ticks(4);
        handle_opcode_0xFF_RST_38H();
    }

    // --- DD/FD Prefixed Opcodes ---

    void exec_ADD_IX_BC() {}
    void exec_ADD_IY_BC() {}

    void exec_ADD_IX_DE() {}
    void exec_ADD_IY_DE() {}

    void exec_LD_IX_nn() {}
    void exec_LD_IY_nn() {}

    void exec_LD_nn_ptr_IX() {}
    void exec_LD_nn_ptr_IY() {}

    void exec_INC_IX() {}
    void exec_INC_IY() {}

    void exec_INC_IXH() {}
    void exec_INC_IYH() {}

    void exec_DEC_IXH() {}
    void exec_DEC_IYH() {}

    void exec_LD_IXH_n() {}
    void exec_LD_IYH_n() {}

    void exec_ADD_IX_IX() {}
    void exec_ADD_IY_IY() {}

    void exec_LD_IX_nn_ptr() {}
    void exec_LD_IY_nn_ptr() {}

    void exec_DEC_IX() {}
    void exec_DEC_IY() {}

    void exec_INC_IXL() {}
    void exec_INC_IYL() {}

    void exec_DEC_IXL() {}
    void exec_DEC_IYL() {}

    void exec_LD_IXL_n() {}
    void exec_LD_IYL_n() {}

    void exec_INC_IX_d_ptr() {}
    void exec_INC_IY_d_ptr() {}

    void exec_DEC_IX_d_ptr() {}
    void exec_DEC_IY_d_ptr() {}

    void exec_LD_IX_d_ptr_n() {}
    void exec_LD_IY_d_ptr_n() {}

    void exec_ADD_IX_SP() {}
    void exec_ADD_IY_SP() {}

    void exec_LD_B_IXH() {}
    void exec_LD_B_IYH() {}

    void exec_LD_B_IXL() {}
    void exec_LD_B_IYL() {}

    void exec_LD_B_IX_d_ptr() {}
    void exec_LD_B_IY_d_ptr() {}

    void exec_LD_C_IXH() {}
    void exec_LD_C_IYH() {}

    void exec_LD_C_IXL() {}
    void exec_LD_C_IYL() {}

    void exec_LD_C_IX_d_ptr() {}
    void exec_LD_C_IY_d_ptr() {}

    void exec_LD_D_IXH() {}
    void exec_LD_D_IYH() {}

    void exec_LD_D_IXL() {}
    void exec_LD_D_IYL() {}

    void exec_LD_D_IX_d_ptr() {}
    void exec_LD_D_IY_d_ptr() {}

    void exec_LD_E_IXH() {}
    void exec_LD_E_IYH() {}

    void exec_LD_E_IXL() {}
    void exec_LD_E_IYL() {}

    void exec_LD_E_IX_d_ptr() {}
    void exec_LD_E_IY_d_ptr() {}

    void exec_LD_IXH_B() {}
    void exec_LD_IYH_B() {}

    void exec_LD_IXH_C() {}
    void exec_LD_IYH_C() {}

    void exec_LD_IXH_D() {}
    void exec_LD_IYH_D() {}

    void exec_LD_IXH_E() {}
    void exec_LD_IYH_E() {}

    void exec_LD_IXH_IXH() {}
    void exec_LD_IYH_IYH() {}

    void exec_LD_IXH_IXL() {}
    void exec_LD_IYH_IYL() {}

    void exec_LD_H_IX_d_ptr() {}
    void exec_LD_H_IY_d_ptr() {}

    void exec_LD_IXH_A() {}
    void exec_LD_IYH_A() {}

    void exec_LD_IXL_B() {}
    void exec_LD_IYL_B() {}

    void exec_LD_IXL_C() {}
    void exec_LD_IYL_C() {}

    void exec_LD_IXL_D() {}
    void exec_LD_IYL_D() {}

    void exec_LD_IXL_E() {}
    void exec_LD_IYL_E() {}

    void exec_LD_IXL_IXH() {}
    void exec_LD_IYL_IYH() {}

    void exec_LD_IXL_IXL() {}
    void exec_LD_IYL_IYL() {}

    void exec_LD_L_IX_d_ptr() {}
    void exec_LD_L_IY_d_ptr() {}

    void exec_LD_IXL_A() {}
    void exec_LD_IYL_A() {}

    void exec_LD_IX_d_ptr_B() {}
    void exec_LD_IY_d_ptr_B() {}

    void exec_LD_IX_d_ptr_C() {}
    void exec_LD_IY_d_ptr_C() {}

    void exec_LD_IX_d_ptr_D() {}
    void exec_LD_IY_d_ptr_D() {}

    void exec_LD_IX_d_ptr_E() {}
    void exec_LD_IY_d_ptr_E() {}

    void exec_LD_IX_d_ptr_H() {}
    void exec_LD_IY_d_ptr_H() {}

    void exec_LD_IX_d_ptr_L() {}
    void exec_LD_IY_d_ptr_L() {}

    void exec_LD_IX_d_ptr_A() {}
    void exec_LD_IY_d_ptr_A() {}

    void exec_LD_A_IXH() {}
    void exec_LD_A_IYH() {}

    void exec_LD_A_IXL() {}
    void exec_LD_A_IYL() {}

    void exec_LD_A_IX_d_ptr() {}
    void exec_LD_A_IY_d_ptr() {}

    void exec_ADD_A_IXH() {}
    void exec_ADD_A_IYH() {}

    void exec_ADD_A_IXL() {}
    void exec_ADD_A_IYL() {}

    void exec_ADD_A_IX_d_ptr() {}
    void exec_ADD_A_IY_d_ptr() {}

    void exec_ADC_A_IXH() {}
    void exec_ADC_A_IYH() {}

    void exec_ADC_A_IXL() {}
    void exec_ADC_A_IYL() {}

    void exec_ADC_A_IX_d_ptr() {}
    void exec_ADC_A_IY_d_ptr() {}

    void exec_SUB_IXH() {}
    void exec_SUB_IYH() {}

    void exec_SUB_IXL() {}
    void exec_SUB_IYL() {}

    void exec_SUB_IX_d_ptr() {}
    void exec_SUB_IY_d_ptr() {}

    void exec_SBC_A_IXH() {}
    void exec_SBC_A_IYH() {}

    void exec_SBC_A_IXL() {}
    void exec_SBC_A_IYL() {}

    void exec_SBC_A_IX_d_ptr() {}
    void exec_SBC_A_IY_d_ptr() {}

    void exec_AND_IXH() {}
    void exec_AND_IYH() {}

    void exec_AND_IXL() {}
    void exec_AND_IYL() {}

    void exec_AND_IX_d_ptr() {}
    void exec_AND_IY_d_ptr() {}

    void exec_XOR_IXH() {}
    void exec_XOR_IYH() {}

    void exec_XOR_IXL() {}
    void exec_XOR_IYL() {}

    void exec_XOR_IX_d_ptr() {}
    void exec_XOR_IY_d_ptr() {}

    void exec_OR_IXH() {}
    void exec_OR_IYH() {}

    void exec_OR_IXL() {}
    void exec_OR_IYL() {}

    void exec_OR_IX_d_ptr() {}
    void exec_OR_IY_d_ptr() {}

    void exec_CP_IXH() {}
    void exec_CP_IYH() {}

    void exec_CP_IXL() {}
    void exec_CP_IYL() {}

    void exec_CP_IX_d_ptr() {}
    void exec_CP_IY_d_ptr() {}

    void exec_POP_IX() {}
    void exec_POP_IY() {}

    void exec_EX_SP_ptr_IX() {}
    void exec_EX_SP_ptr_IY() {}

    void exec_PUSH_IX() {}
    void exec_PUSH_IY() {}

    void exec_JP_IX_ptr() {}
    void exec_JP_IY_ptr() {}

    void exec_LD_SP_IX() {}
    void exec_LD_SP_IY() {}