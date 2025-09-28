#ifndef __Z80_H__
#define __Z80_H__

#include <cstdint>

class Z80 {
public:
    enum class IndexMode { HL, IX, IY };
    struct State {
        // 16-bit Main Registers
        uint16_t AF, BC, DE, HL;
        uint16_t IX, IY, SP, PC;
        
        // 16-bit Alternate Registers
        uint16_t AFp, BCp, DEp, HLp;
        
        // 8-bit Special Registers
        uint8_t I, R;
        
        // Core State Flags
        bool IFF1, IFF2;
        bool halted;
        
        // Interrupt State
        bool nmi_pending;
        bool interrupt_pending;
        bool interrupt_enable_pending;
        uint8_t interrupt_data;
        uint8_t interrupt_mode;
        
        // Index mode
        IndexMode index_mode;

        // Cycle Counter
        long long ticks;
    };

    class MemoryBus {
    public:
        virtual ~MemoryBus() = default;

        virtual void connect(Z80* processor) {cpu = processor;}
        virtual void reset() = 0;

        virtual uint8_t read(uint16_t address) = 0;
        virtual void write(uint16_t address, uint8_t value) = 0;
    protected:
        Z80* cpu;
    };

    class IOBus {
        public:
            virtual ~IOBus() = default;

            virtual void connect(Z80* processor) {cpu = processor;}
            virtual void reset() = 0;

            virtual uint8_t read(uint16_t port) = 0;
            virtual void write(uint16_t port, uint8_t value) = 0;
        protected:
            Z80* cpu;
    };

    // --- Constructor ---
    Z80(MemoryBus& mem_bus, IOBus& io_bus) : memory(mem_bus), io(io_bus) {
        memory.connect(this);
        io.connect(this);
        memory.reset();
        io.reset();
        reset();
    }

    // Main execution and control interface
    long long run(long long ticks_limit) {return operate<OperateMode::ToLimit>(ticks_limit);}
    int step() {return operate<OperateMode::SingleStep>(0);}
    void reset(); 
    void request_interrupt(uint8_t data);
    void request_nmi();

    // High-Level State Management Methods ---
    State save_state() const;
    void load_state(const State& state);

    // Cycle counter
    long long get_ticks() const { return ticks; }
    void set_ticks(long long value) { ticks = value; }
    void add_ticks(int delta) { ticks += delta; }

    // 16-bit main registers
    uint16_t get_AF() const { return AF; }
    void set_AF(uint16_t value) { AF = value; }
    uint16_t get_BC() const { return BC; }
    void set_BC(uint16_t value) { BC = value; }
    uint16_t get_DE() const { return DE; }
    void set_DE(uint16_t value) { DE = value; }
    uint16_t get_HL() const { return HL; }
    void set_HL(uint16_t value) { HL = value; }
    uint16_t get_IX() const { return IX; }
    void set_IX(uint16_t value) { IX = value; }
    uint16_t get_IY() const { return IY; }
    void set_IY(uint16_t value) { IY = value; }
    uint16_t get_SP() const { return SP; }
    void set_SP(uint16_t value) { SP = value; }
    uint16_t get_PC() const { return PC; }
    void set_PC(uint16_t value) { PC = value; }

    // 16-bit alternate registers
    uint16_t get_AFp() const { return AFp; }
    void set_AFp(uint16_t value) { AFp = value; }
    uint16_t get_BCp() const { return BCp; }
    void set_BCp(uint16_t value) { BCp = value; }
    uint16_t get_DEp() const { return DEp; }
    void set_DEp(uint16_t value) { DEp = value; }
    uint16_t get_HLp() const { return HLp; }
    void set_HLp(uint16_t value) { HLp = value; }

    // 8-bit registers (computed from 16-bit pairs)
    uint8_t get_A() const { return (get_AF() >> 8) & 0xFF; }
    void set_A(uint8_t value) { set_AF((static_cast<uint16_t>(value) << 8) | (get_AF() & 0xFF)); }
    uint8_t get_F() const { return get_AF() & 0xFF; }
    void set_F(uint8_t value) { set_AF((get_AF() & 0xFF00) | value); }
    uint8_t get_B() const { return (get_BC() >> 8) & 0xFF; }
    void set_B(uint8_t value) { set_BC((static_cast<uint16_t>(value) << 8) | (get_BC() & 0xFF)); }
    uint8_t get_C() const { return get_BC() & 0xFF; }
    void set_C(uint8_t value) { set_BC((get_BC() & 0xFF00) | value); }
    uint8_t get_D() const { return (get_DE() >> 8) & 0xFF; }
    void set_D(uint8_t value) { set_DE((static_cast<uint16_t>(value) << 8) | (get_DE() & 0xFF)); }
    uint8_t get_E() const { return get_DE() & 0xFF; }
    void set_E(uint8_t value) { set_DE((get_DE() & 0xFF00) | value); }
    uint8_t get_H() const { return (get_HL() >> 8) & 0xFF; }
    void set_H(uint8_t value) { set_HL((static_cast<uint16_t>(value) << 8) | (get_HL() & 0xFF)); }
    uint8_t get_L() const { return get_HL() & 0xFF; }
    void set_L(uint8_t value) { set_HL((get_HL() & 0xFF00) | value); }
    uint8_t get_IXH() const { return (get_IX() >> 8) & 0xFF; }
    void set_IXH(uint8_t value) { set_IX((static_cast<uint16_t>(value) << 8) | (get_IX() & 0xFF)); }
    uint8_t get_IXL() const { return get_IX() & 0xFF; }
    void set_IXL(uint8_t value) { set_IX((get_IX() & 0xFF00) | value); }
    uint8_t get_IYH() const { return (get_IY() >> 8) & 0xFF; }
    void set_IYH(uint8_t value) { set_IY((static_cast<uint16_t>(value) << 8) | (get_IY() & 0xFF)); }
    uint8_t get_IYL() const { return get_IY() & 0xFF; }
    void set_IYL(uint8_t value) { set_IY((get_IY() & 0xFF00) | value); }

    // Special purpose registers
    uint8_t get_I() const { return I; }
    void set_I(uint8_t value) { I = value; }
    uint8_t get_R() const { return R; }
    void set_R(uint8_t value) { R = value; }

    // CPU state flags
    bool get_IFF1() const { return IFF1; }
    void set_IFF1(bool state) { IFF1 = state; }
    bool get_IFF2() const { return IFF2; }
    void set_IFF2(bool state) { IFF2 = state; }
    bool is_halted() const { return halted; }
    void set_halted(bool state) { halted = state; }
    
    // Interrupt state flags
    bool is_nmi_pending() const { return nmi_pending; }
    void set_nmi_pending(bool state) { nmi_pending = state; }
    bool is_interrupt_pending() const { return interrupt_pending; }
    void set_interrupt_pending(bool state) { interrupt_pending = state; }
    bool is_interrupt_enable_pending() const { return interrupt_enable_pending; }
    void set_interrupt_enable_pending(bool state) { interrupt_enable_pending = state; }
    uint8_t get_interrupt_data() const { return interrupt_data; }
    void set_interrupt_data(uint8_t data) { interrupt_data = data; }
    uint8_t get_interrupt_mode() const { return interrupt_mode; }
    void set_interrupt_mode(uint8_t mode) { interrupt_mode = mode; }
    void set_reti_signaled(bool state) { reti_signaled = state; }
    
    // Processing opcodes DD and FD index
    IndexMode get_index_mode() const { return index_mode;}
    void set_index_mode(IndexMode mode) { index_mode = mode; }

    // --- Flag constants and helpers ---
    static constexpr uint8_t FLAG_C  = 1 << 0;
    static constexpr uint8_t FLAG_N  = 1 << 1;
    static constexpr uint8_t FLAG_PV = 1 << 2;
    static constexpr uint8_t FLAG_X  = 1 << 3;
    static constexpr uint8_t FLAG_H  = 1 << 4;
    static constexpr uint8_t FLAG_Y  = 1 << 5;
    static constexpr uint8_t FLAG_Z  = 1 << 6;
    static constexpr uint8_t FLAG_S  = 1 << 7;
    bool is_S_flag_set() const { return (get_F() & FLAG_S) != 0; }
    bool is_Z_flag_set() const { return (get_F() & FLAG_Z) != 0; }
    bool is_H_flag_set() const { return (get_F() & FLAG_H) != 0; }
    bool is_PV_flag_set() const { return (get_F() & FLAG_PV) != 0; }
    bool is_N_flag_set() const { return (get_F() & FLAG_N) != 0; }
    bool is_C_flag_set() const { return (get_F() & FLAG_C) != 0; }
    void set_flag(uint8_t mask) { set_F(get_F() | mask); }
    void clear_flag(uint8_t mask) { set_F(get_F() & ~mask); }
    void set_flag_if(uint8_t mask, bool condition) {
        if (condition)
            set_flag(mask);
        else
            clear_flag(mask);
    }
    
private:
    uint16_t AF, BC, DE, HL;
    uint16_t AFp, BCp, DEp, HLp;
    uint16_t IX, IY;
    uint16_t SP, PC;
    uint8_t I, R;
    bool IFF1, IFF2, halted, nmi_pending, interrupt_pending, interrupt_enable_pending, reti_signaled;
    uint8_t interrupt_data, interrupt_mode;
    long long ticks;
    IndexMode index_mode;

    MemoryBus& memory;
    IOBus& io;
    
    uint8_t read_byte(uint16_t address);
    void write_byte(uint16_t address, uint8_t value);
    uint16_t read_word(uint16_t address);
    void write_word(uint16_t address, uint16_t value);
    void push_word(uint16_t value);
    uint16_t pop_word();
    uint8_t fetch_next_opcode();
    uint8_t fetch_next_byte();
    uint16_t fetch_next_word();
    
    bool is_parity_even(uint8_t value) {
        int count = 0;
        while (value > 0) {
            value &= (value - 1);
            count++;
        }
        return (count % 2) == 0;
    }

    uint16_t get_indexed_HL() const;
    void set_indexed_HL(uint16_t value);
    uint8_t get_indexed_H() const;
    void set_indexed_H(uint8_t value);
    uint8_t get_indexed_L() const;
    void set_indexed_L(uint8_t value);

    uint16_t get_indexed_address();
    uint8_t read_byte_from_indexed_address();
    void write_byte_to_indexed_address(uint8_t value);

    uint8_t inc_8bit(uint8_t value);
    uint8_t dec_8bit(uint8_t value);
    void and_8bit(uint8_t value);
    void or_8bit(uint8_t value);
    void xor_8bit(uint8_t value);
    void cp_8bit(uint8_t value);
    void add_8bit(uint8_t value);
    void adc_8bit(uint8_t value);
    void sub_8bit(uint8_t value);
    void sbc_8bit(uint8_t value);
    uint16_t add_16bit(uint16_t reg, uint16_t value);
    uint16_t adc_16bit(uint16_t reg, uint16_t value);
    uint16_t sbc_16bit(uint16_t reg, uint16_t value);
    uint8_t rlc_8bit(uint8_t value);
    uint8_t rrc_8bit(uint8_t value);
    uint8_t rl_8bit(uint8_t value);
    uint8_t rr_8bit(uint8_t value);
    uint8_t sla_8bit(uint8_t value);
    uint8_t sra_8bit(uint8_t value);
    uint8_t sll_8bit(uint8_t value);
    uint8_t srl_8bit(uint8_t value);
    void bit_8bit(uint8_t bit, uint8_t value);
    uint8_t res_8bit(uint8_t bit, uint8_t value);
    uint8_t set_8bit(uint8_t bit, uint8_t value);
    uint8_t in_r_c();
    void out_c_r(uint8_t value);

    void handle_nmi();
    void handle_interrupt();

    void handle_CB();
    void handle_CB_indexed(uint16_t index_register);

    void opcode_0x00_NOP();
    void opcode_0x01_LD_BC_nn();
    void opcode_0x02_LD_BC_ptr_A();
    void opcode_0x03_INC_BC();
    void opcode_0x04_INC_B();
    void opcode_0x05_DEC_B();
    void opcode_0x06_LD_B_n();
    void opcode_0x07_RLCA();
    void opcode_0x08_EX_AF_AFp();
    void opcode_0x09_ADD_HL_BC();
    void opcode_0x0A_LD_A_BC_ptr();
    void opcode_0x0B_DEC_BC();
    void opcode_0x0C_INC_C();
    void opcode_0x0D_DEC_C();
    void opcode_0x0E_LD_C_n();
    void opcode_0x0F_RRCA();
    void opcode_0x10_DJNZ_d();
    void opcode_0x11_LD_DE_nn();
    void opcode_0x12_LD_DE_ptr_A();
    void opcode_0x13_INC_DE();
    void opcode_0x14_INC_D();
    void opcode_0x15_DEC_D();
    void opcode_0x16_LD_D_n();
    void opcode_0x17_RLA();
    void opcode_0x18_JR_d();
    void opcode_0x19_ADD_HL_DE();
    void opcode_0x1A_LD_A_DE_ptr();
    void opcode_0x1B_DEC_DE();
    void opcode_0x1C_INC_E();
    void opcode_0x1D_DEC_E();
    void opcode_0x1E_LD_E_n();
    void opcode_0x1F_RRA();
    void opcode_0x20_JR_NZ_d();
    void opcode_0x21_LD_HL_nn();
    void opcode_0x22_LD_nn_ptr_HL();
    void opcode_0x23_INC_HL();
    void opcode_0x24_INC_H();
    void opcode_0x25_DEC_H();
    void opcode_0x26_LD_H_n();
    void opcode_0x27_DAA();
    void opcode_0x28_JR_Z_d();
    void opcode_0x29_ADD_HL_HL();
    void opcode_0x2A_LD_HL_nn_ptr();
    void opcode_0x2B_DEC_HL();
    void opcode_0x2C_INC_L();
    void opcode_0x2D_DEC_L();
    void opcode_0x2E_LD_L_n();
    void opcode_0x2F_CPL();
    void opcode_0x30_JR_NC_d();
    void opcode_0x31_LD_SP_nn();
    void opcode_0x32_LD_nn_ptr_A();
    void opcode_0x33_INC_SP();
    void opcode_0x34_INC_HL_ptr();
    void opcode_0x35_DEC_HL_ptr();
    void opcode_0x36_LD_HL_ptr_n();
    void opcode_0x37_SCF();
    void opcode_0x38_JR_C_d();
    void opcode_0x39_ADD_HL_SP();
    void opcode_0x3A_LD_A_nn_ptr();
    void opcode_0x3B_DEC_SP();
    void opcode_0x3C_INC_A();
    void opcode_0x3D_DEC_A();
    void opcode_0x3E_LD_A_n();
    void opcode_0x3F_CCF();
    void opcode_0x40_LD_B_B();
    void opcode_0x41_LD_B_C();
    void opcode_0x42_LD_B_D();
    void opcode_0x43_LD_B_E();
    void opcode_0x44_LD_B_H();
    void opcode_0x45_LD_B_L();
    void opcode_0x46_LD_B_HL_ptr();
    void opcode_0x47_LD_B_A();
    void opcode_0x48_LD_C_B();
    void opcode_0x49_LD_C_C();
    void opcode_0x4A_LD_C_D();
    void opcode_0x4B_LD_C_E();
    void opcode_0x4C_LD_C_H();
    void opcode_0x4D_LD_C_L();
    void opcode_0x4E_LD_C_HL_ptr();
    void opcode_0x4F_LD_C_A();
    void opcode_0x50_LD_D_B();
    void opcode_0x51_LD_D_C();
    void opcode_0x52_LD_D_D();
    void opcode_0x53_LD_D_E();
    void opcode_0x54_LD_D_H();
    void opcode_0x55_LD_D_L();
    void opcode_0x56_LD_D_HL_ptr();
    void opcode_0x57_LD_D_A();
    void opcode_0x58_LD_E_B();
    void opcode_0x59_LD_E_C();
    void opcode_0x5A_LD_E_D();
    void opcode_0x5B_LD_E_E();
    void opcode_0x5C_LD_E_H();
    void opcode_0x5D_LD_E_L();
    void opcode_0x5E_LD_E_HL_ptr();
    void opcode_0x5F_LD_E_A();
    void opcode_0x60_LD_H_B();
    void opcode_0x61_LD_H_C();
    void opcode_0x62_LD_H_D();
    void opcode_0x63_LD_H_E();
    void opcode_0x64_LD_H_H();
    void opcode_0x65_LD_H_L();
    void opcode_0x66_LD_H_HL_ptr();
    void opcode_0x67_LD_H_A();
    void opcode_0x68_LD_L_B();
    void opcode_0x69_LD_L_C();
    void opcode_0x6A_LD_L_D();
    void opcode_0x6B_LD_L_E();
    void opcode_0x6C_LD_L_H();
    void opcode_0x6D_LD_L_L();
    void opcode_0x6E_LD_L_HL_ptr();
    void opcode_0x6F_LD_L_A();
    void opcode_0x70_LD_HL_ptr_B();
    void opcode_0x71_LD_HL_ptr_C();
    void opcode_0x72_LD_HL_ptr_D();
    void opcode_0x73_LD_HL_ptr_E();
    void opcode_0x74_LD_HL_ptr_H();
    void opcode_0x75_LD_HL_ptr_L();
    void opcode_0x76_HALT();
    void opcode_0x77_LD_HL_ptr_A();
    void opcode_0x78_LD_A_B();
    void opcode_0x79_LD_A_C();
    void opcode_0x7A_LD_A_D();
    void opcode_0x7B_LD_A_E();
    void opcode_0x7C_LD_A_H();
    void opcode_0x7D_LD_A_L();
    void opcode_0x7E_LD_A_HL_ptr();
    void opcode_0x7F_LD_A_A();
    void opcode_0x80_ADD_A_B();
    void opcode_0x81_ADD_A_C();
    void opcode_0x82_ADD_A_D();
    void opcode_0x83_ADD_A_E();
    void opcode_0x84_ADD_A_H();
    void opcode_0x85_ADD_A_L();
    void opcode_0x86_ADD_A_HL_ptr();
    void opcode_0x87_ADD_A_A();
    void opcode_0x88_ADC_A_B();
    void opcode_0x89_ADC_A_C();
    void opcode_0x8A_ADC_A_D();
    void opcode_0x8B_ADC_A_E();
    void opcode_0x8C_ADC_A_H();
    void opcode_0x8D_ADC_A_L();
    void opcode_0x8E_ADC_A_HL_ptr();
    void opcode_0x8F_ADC_A_A();
    void opcode_0x90_SUB_B();
    void opcode_0x91_SUB_C();
    void opcode_0x92_SUB_D();
    void opcode_0x93_SUB_E();
    void opcode_0x94_SUB_H();
    void opcode_0x95_SUB_L();
    void opcode_0x96_SUB_HL_ptr();
    void opcode_0x97_SUB_A();
    void opcode_0x98_SBC_A_B();
    void opcode_0x99_SBC_A_C();
    void opcode_0x9A_SBC_A_D();
    void opcode_0x9B_SBC_A_E();
    void opcode_0x9C_SBC_A_H();
    void opcode_0x9D_SBC_A_L();
    void opcode_0x9E_SBC_A_HL_ptr();
    void opcode_0x9F_SBC_A_A();
    void opcode_0xA0_AND_B();
    void opcode_0xA1_AND_C();
    void opcode_0xA2_AND_D();
    void opcode_0xA3_AND_E();
    void opcode_0xA4_AND_H();
    void opcode_0xA5_AND_L();
    void opcode_0xA6_AND_HL_ptr();
    void opcode_0xA7_AND_A();
    void opcode_0xA8_XOR_B();
    void opcode_0xA9_XOR_C();
    void opcode_0xAA_XOR_D();
    void opcode_0xAB_XOR_E();
    void opcode_0xAC_XOR_H();
    void opcode_0xAD_XOR_L();
    void opcode_0xAE_XOR_HL_ptr();
    void opcode_0xAF_XOR_A();
    void opcode_0xB0_OR_B();
    void opcode_0xB1_OR_C();
    void opcode_0xB2_OR_D();
    void opcode_0xB3_OR_E();
    void opcode_0xB4_OR_H();
    void opcode_0xB5_OR_L();
    void opcode_0xB6_OR_HL_ptr();
    void opcode_0xB7_OR_A();
    void opcode_0xB8_CP_B();
    void opcode_0xB9_CP_C();
    void opcode_0xBA_CP_D();
    void opcode_0xBB_CP_E();
    void opcode_0xBC_CP_H();
    void opcode_0xBD_CP_L();
    void opcode_0xBE_CP_HL_ptr();
    void opcode_0xBF_CP_A();
    void opcode_0xC0_RET_NZ();
    void opcode_0xC1_POP_BC();
    void opcode_0xC2_JP_NZ_nn();
    void opcode_0xC3_JP_nn();
    void opcode_0xC4_CALL_NZ_nn();
    void opcode_0xC5_PUSH_BC();
    void opcode_0xC6_ADD_A_n();
    void opcode_0xC7_RST_00H();
    void opcode_0xC8_RET_Z();
    void opcode_0xC9_RET();
    void opcode_0xCA_JP_Z_nn();
    void opcode_0xCC_CALL_Z_nn();
    void opcode_0xCD_CALL_nn();
    void opcode_0xCE_ADC_A_n();
    void opcode_0xCF_RST_08H();
    void opcode_0xD0_RET_NC();
    void opcode_0xD1_POP_DE();
    void opcode_0xD2_JP_NC_nn();
    void opcode_0xD3_OUT_n_ptr_A();
    void opcode_0xD4_CALL_NC_nn();
    void opcode_0xD5_PUSH_DE();
    void opcode_0xD6_SUB_n();
    void opcode_0xD7_RST_10H();
    void opcode_0xD8_RET_C();
    void opcode_0xD9_EXX();
    void opcode_0xDA_JP_C_nn();
    void opcode_0xDB_IN_A_n_ptr();
    void opcode_0xDC_CALL_C_nn();
    void opcode_0xDE_SBC_A_n();
    void opcode_0xDF_RST_18H();
    void opcode_0xE0_RET_PO();
    void opcode_0xE1_POP_HL();
    void opcode_0xE2_JP_PO_nn();
    void opcode_0xE3_EX_SP_ptr_HL();
    void opcode_0xE4_CALL_PO_nn();
    void opcode_0xE5_PUSH_HL();
    void opcode_0xE6_AND_n();
    void opcode_0xE7_RST_20H();
    void opcode_0xE8_RET_PE();
    void opcode_0xE9_JP_HL_ptr();
    void opcode_0xEA_JP_PE_nn();
    void opcode_0xEB_EX_DE_HL();
    void opcode_0xEC_CALL_PE_nn();
    void opcode_0xEE_XOR_n();
    void opcode_0xEF_RST_28H();
    void opcode_0xF0_RET_P();
    void opcode_0xF1_POP_AF();
    void opcode_0xF2_JP_P_nn();
    void opcode_0xF3_DI();
    void opcode_0xF4_CALL_P_nn();
    void opcode_0xF5_PUSH_AF();
    void opcode_0xF6_OR_n();
    void opcode_0xF7_RST_30H();
    void opcode_0xF8_RET_M();
    void opcode_0xF9_LD_SP_HL();
    void opcode_0xFA_JP_M_nn();
    void opcode_0xFB_EI();
    void opcode_0xFC_CALL_M_nn();
    void opcode_0xFE_CP_n();
    void opcode_0xFF_RST_38H();
    void opcode_0xED_0x40_IN_B_C_ptr();
    void opcode_0xED_0x41_OUT_C_ptr_B();
    void opcode_0xED_0x42_SBC_HL_BC();
    void opcode_0xED_0x43_LD_nn_ptr_BC();
    void opcode_0xED_0x44_NEG();
    void opcode_0xED_0x45_RETN();
    void opcode_0xED_0x46_IM_0();
    void opcode_0xED_0x47_LD_I_A();
    void opcode_0xED_0x48_IN_C_C_ptr();
    void opcode_0xED_0x49_OUT_C_ptr_C();
    void opcode_0xED_0x4A_ADC_HL_BC();
    void opcode_0xED_0x4B_LD_BC_nn_ptr();
    void opcode_0xED_0x4D_RETI();
    void opcode_0xED_0x4F_LD_R_A();
    void opcode_0xED_0x50_IN_D_C_ptr();
    void opcode_0xED_0x51_OUT_C_ptr_D();
    void opcode_0xED_0x52_SBC_HL_DE();
    void opcode_0xED_0x53_LD_nn_ptr_DE();
    void opcode_0xED_0x56_IM_1();
    void opcode_0xED_0x57_LD_A_I();
    void opcode_0xED_0x58_IN_E_C_ptr();
    void opcode_0xED_0x59_OUT_C_ptr_E();
    void opcode_0xED_0x5A_ADC_HL_DE();
    void opcode_0xED_0x5B_LD_DE_nn_ptr();
    void opcode_0xED_0x5E_IM_2();
    void opcode_0xED_0x5F_LD_A_R();
    void opcode_0xED_0x60_IN_H_C_ptr();
    void opcode_0xED_0x61_OUT_C_ptr_H();
    void opcode_0xED_0x62_SBC_HL_HL();
    void opcode_0xED_0x63_LD_nn_ptr_HL_ED();
    void opcode_0xED_0x67_RRD();
    void opcode_0xED_0x68_IN_L_C_ptr();
    void opcode_0xED_0x69_OUT_C_ptr_L();
    void opcode_0xED_0x6A_ADC_HL_HL();
    void opcode_0xED_0x6B_LD_HL_nn_ptr_ED();
    void opcode_0xED_0x6F_RLD();
    void opcode_0xED_0x70_IN_C_ptr();
    void opcode_0xED_0x71_OUT_C_ptr_0();
    void opcode_0xED_0x72_SBC_HL_SP();
    void opcode_0xED_0x73_LD_nn_ptr_SP();
    void opcode_0xED_0x78_IN_A_C_ptr();
    void opcode_0xED_0x79_OUT_C_ptr_A();
    void opcode_0xED_0x7A_ADC_HL_SP();
    void opcode_0xED_0x7B_LD_SP_nn_ptr();
    void opcode_0xED_0xA0_LDI();
    void opcode_0xED_0xA1_CPI();
    void opcode_0xED_0xA2_INI();
    void opcode_0xED_0xA3_OUTI();
    void opcode_0xED_0xA8_LDD();
    void opcode_0xED_0xA9_CPD();
    void opcode_0xED_0xAA_IND();
    void opcode_0xED_0xAB_OUTD();
    void opcode_0xED_0xB0_LDIR();
    void opcode_0xED_0xB1_CPIR();
    void opcode_0xED_0xB2_INIR();
    void opcode_0xED_0xB3_OTIR();
    void opcode_0xED_0xB8_LDDR();
    void opcode_0xED_0xB9_CPDR();
    void opcode_0xED_0xBA_INDR();
    void opcode_0xED_0xBB_OTDR();

    enum class OperateMode {
        ToLimit,
        SingleStep
    };
    template<OperateMode TMode> long long operate(long long ticks_limit);
};

template<Z80::OperateMode TMode>
long long Z80::operate(long long ticks_limit) {
    long long initial_ticks = get_ticks();

    while (true) {
/*
        std::cout << std::hex << std::setfill('0')
                  << "PC:" << std::setw(4) << get_PC()
                  << " AF:" << std::setw(4) << get_AF()
                  << " BC:" << std::setw(4) << get_BC()
                  << " DE:" << std::setw(4) << get_DE()
                  << " HL:" << std::setw(4) << get_HL()
                  << " IX:" << std::setw(4) << get_IX()
                  << " IY:" << std::setw(4) << get_IY()
                  << " SP:" << std::setw(4) << get_SP()
                  << std::endl;
*/
        if (is_halted()) {
            if (is_nmi_pending() || (is_interrupt_pending() && get_IFF1())) {
                set_halted(false);
            } else {
                add_ticks(ticks_limit - get_ticks());
                continue;
            }
        }
        if (is_nmi_pending()) {
            handle_nmi();
            continue;
        }
        if (is_interrupt_enable_pending()) {
            set_IFF1(true);
            set_IFF2(true);
            set_interrupt_enable_pending(false);
        }
        if (is_interrupt_pending() && get_IFF1()) {
            handle_interrupt();
            set_interrupt_pending(false);
            continue;
        }

        set_index_mode(IndexMode::HL);
        uint8_t opcode = fetch_next_opcode();
        
        while (opcode == 0xDD || opcode == 0xFD) {
            set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
            opcode = fetch_next_opcode();
        }

        if (opcode == 0xCB) {
            if (get_index_mode() != IndexMode::HL)
                handle_CB_indexed((get_index_mode() == IndexMode::IX) ? get_IX() : get_IY());
            else
                handle_CB();
            continue;
        }
        
        switch (opcode) {
            case 0x00: opcode_0x00_NOP(); break;
            case 0x01: opcode_0x01_LD_BC_nn(); break;
            case 0x02: opcode_0x02_LD_BC_ptr_A(); break;
            case 0x03: opcode_0x03_INC_BC(); break;
            case 0x04: opcode_0x04_INC_B(); break;
            case 0x05: opcode_0x05_DEC_B(); break;
            case 0x06: opcode_0x06_LD_B_n(); break;
            case 0x07: opcode_0x07_RLCA(); break;
            case 0x08: opcode_0x08_EX_AF_AFp(); break;
            case 0x09: opcode_0x09_ADD_HL_BC(); break;
            case 0x0A: opcode_0x0A_LD_A_BC_ptr(); break;
            case 0x0B: opcode_0x0B_DEC_BC(); break;
            case 0x0C: opcode_0x0C_INC_C(); break;
            case 0x0D: opcode_0x0D_DEC_C(); break;
            case 0x0E: opcode_0x0E_LD_C_n(); break;
            case 0x0F: opcode_0x0F_RRCA(); break;
            case 0x10: opcode_0x10_DJNZ_d(); break;
            case 0x11: opcode_0x11_LD_DE_nn(); break;
            case 0x12: opcode_0x12_LD_DE_ptr_A(); break;
            case 0x13: opcode_0x13_INC_DE(); break;
            case 0x14: opcode_0x14_INC_D(); break;
            case 0x15: opcode_0x15_DEC_D(); break;
            case 0x16: opcode_0x16_LD_D_n(); break;
            case 0x17: opcode_0x17_RLA(); break;
            case 0x18: opcode_0x18_JR_d(); break;
            case 0x19: opcode_0x19_ADD_HL_DE(); break;
            case 0x1A: opcode_0x1A_LD_A_DE_ptr(); break;
            case 0x1B: opcode_0x1B_DEC_DE(); break;
            case 0x1C: opcode_0x1C_INC_E(); break;
            case 0x1D: opcode_0x1D_DEC_E(); break;
            case 0x1E: opcode_0x1E_LD_E_n(); break;
            case 0x1F: opcode_0x1F_RRA(); break;
            case 0x20: opcode_0x20_JR_NZ_d(); break;
            case 0x21: opcode_0x21_LD_HL_nn(); break;
            case 0x22: opcode_0x22_LD_nn_ptr_HL(); break;
            case 0x23: opcode_0x23_INC_HL(); break;
            case 0x24: opcode_0x24_INC_H(); break;
            case 0x25: opcode_0x25_DEC_H(); break;
            case 0x26: opcode_0x26_LD_H_n(); break;
            case 0x27: opcode_0x27_DAA(); break;
            case 0x28: opcode_0x28_JR_Z_d(); break;
            case 0x29: opcode_0x29_ADD_HL_HL(); break;
            case 0x2A: opcode_0x2A_LD_HL_nn_ptr(); break;
            case 0x2B: opcode_0x2B_DEC_HL(); break;
            case 0x2C: opcode_0x2C_INC_L(); break;
            case 0x2D: opcode_0x2D_DEC_L(); break;
            case 0x2E: opcode_0x2E_LD_L_n(); break;
            case 0x2F: opcode_0x2F_CPL(); break;
            case 0x30: opcode_0x30_JR_NC_d(); break;
            case 0x31: opcode_0x31_LD_SP_nn(); break;
            case 0x32: opcode_0x32_LD_nn_ptr_A(); break;
            case 0x33: opcode_0x33_INC_SP(); break;
            case 0x34: opcode_0x34_INC_HL_ptr(); break;
            case 0x35: opcode_0x35_DEC_HL_ptr(); break;
            case 0x36: opcode_0x36_LD_HL_ptr_n(); break;
            case 0x37: opcode_0x37_SCF(); break;
            case 0x38: opcode_0x38_JR_C_d(); break;
            case 0x39: opcode_0x39_ADD_HL_SP(); break;
            case 0x3A: opcode_0x3A_LD_A_nn_ptr(); break;
            case 0x3B: opcode_0x3B_DEC_SP(); break;
            case 0x3C: opcode_0x3C_INC_A(); break;
            case 0x3D: opcode_0x3D_DEC_A(); break;
            case 0x3E: opcode_0x3E_LD_A_n(); break;
            case 0x3F: opcode_0x3F_CCF(); break;
            case 0x40: opcode_0x40_LD_B_B(); break;
            case 0x41: opcode_0x41_LD_B_C(); break;
            case 0x42: opcode_0x42_LD_B_D(); break;
            case 0x43: opcode_0x43_LD_B_E(); break;
            case 0x44: opcode_0x44_LD_B_H(); break;
            case 0x45: opcode_0x45_LD_B_L(); break;
            case 0x46: opcode_0x46_LD_B_HL_ptr(); break;
            case 0x47: opcode_0x47_LD_B_A(); break;
            case 0x48: opcode_0x48_LD_C_B(); break;
            case 0x49: opcode_0x49_LD_C_C(); break;
            case 0x4A: opcode_0x4A_LD_C_D(); break;
            case 0x4B: opcode_0x4B_LD_C_E(); break;
            case 0x4C: opcode_0x4C_LD_C_H(); break;
            case 0x4D: opcode_0x4D_LD_C_L(); break;
            case 0x4E: opcode_0x4E_LD_C_HL_ptr(); break;
            case 0x4F: opcode_0x4F_LD_C_A(); break;
            case 0x50: opcode_0x50_LD_D_B(); break;
            case 0x51: opcode_0x51_LD_D_C(); break;
            case 0x52: opcode_0x52_LD_D_D(); break;
            case 0x53: opcode_0x53_LD_D_E(); break;
            case 0x54: opcode_0x54_LD_D_H(); break;
            case 0x55: opcode_0x55_LD_D_L(); break;
            case 0x56: opcode_0x56_LD_D_HL_ptr(); break;
            case 0x57: opcode_0x57_LD_D_A(); break;
            case 0x58: opcode_0x58_LD_E_B(); break;
            case 0x59: opcode_0x59_LD_E_C(); break;
            case 0x5A: opcode_0x5A_LD_E_D(); break;
            case 0x5B: opcode_0x5B_LD_E_E(); break;
            case 0x5C: opcode_0x5C_LD_E_H(); break;
            case 0x5D: opcode_0x5D_LD_E_L(); break;
            case 0x5E: opcode_0x5E_LD_E_HL_ptr(); break;
            case 0x5F: opcode_0x5F_LD_E_A(); break;
            case 0x60: opcode_0x60_LD_H_B(); break;
            case 0x61: opcode_0x61_LD_H_C(); break;
            case 0x62: opcode_0x62_LD_H_D(); break;
            case 0x63: opcode_0x63_LD_H_E(); break;
            case 0x64: opcode_0x64_LD_H_H(); break;
            case 0x65: opcode_0x65_LD_H_L(); break;
            case 0x66: opcode_0x66_LD_H_HL_ptr(); break;
            case 0x67: opcode_0x67_LD_H_A(); break;
            case 0x68: opcode_0x68_LD_L_B(); break;
            case 0x69: opcode_0x69_LD_L_C(); break;
            case 0x6A: opcode_0x6A_LD_L_D(); break;
            case 0x6B: opcode_0x6B_LD_L_E(); break;
            case 0x6C: opcode_0x6C_LD_L_H(); break;
            case 0x6D: opcode_0x6D_LD_L_L(); break;
            case 0x6E: opcode_0x6E_LD_L_HL_ptr(); break;
            case 0x6F: opcode_0x6F_LD_L_A(); break;
            case 0x70: opcode_0x70_LD_HL_ptr_B(); break;
            case 0x71: opcode_0x71_LD_HL_ptr_C(); break;
            case 0x72: opcode_0x72_LD_HL_ptr_D(); break;
            case 0x73: opcode_0x73_LD_HL_ptr_E(); break;
            case 0x74: opcode_0x74_LD_HL_ptr_H(); break;
            case 0x75: opcode_0x75_LD_HL_ptr_L(); break;
            case 0x76: opcode_0x76_HALT(); break;
            case 0x77: opcode_0x77_LD_HL_ptr_A(); break;
            case 0x78: opcode_0x78_LD_A_B(); break;
            case 0x79: opcode_0x79_LD_A_C(); break;
            case 0x7A: opcode_0x7A_LD_A_D(); break;
            case 0x7B: opcode_0x7B_LD_A_E(); break;
            case 0x7C: opcode_0x7C_LD_A_H(); break;
            case 0x7D: opcode_0x7D_LD_A_L(); break;
            case 0x7E: opcode_0x7E_LD_A_HL_ptr(); break;
            case 0x7F: opcode_0x7F_LD_A_A(); break;
            case 0x80: opcode_0x80_ADD_A_B(); break;
            case 0x81: opcode_0x81_ADD_A_C(); break;
            case 0x82: opcode_0x82_ADD_A_D(); break;
            case 0x83: opcode_0x83_ADD_A_E(); break;
            case 0x84: opcode_0x84_ADD_A_H(); break;
            case 0x85: opcode_0x85_ADD_A_L(); break;
            case 0x86: opcode_0x86_ADD_A_HL_ptr(); break;
            case 0x87: opcode_0x87_ADD_A_A(); break;
            case 0x88: opcode_0x88_ADC_A_B(); break;
            case 0x89: opcode_0x89_ADC_A_C(); break;
            case 0x8A: opcode_0x8A_ADC_A_D(); break;
            case 0x8B: opcode_0x8B_ADC_A_E(); break;
            case 0x8C: opcode_0x8C_ADC_A_H(); break;
            case 0x8D: opcode_0x8D_ADC_A_L(); break;
            case 0x8E: opcode_0x8E_ADC_A_HL_ptr(); break;
            case 0x8F: opcode_0x8F_ADC_A_A(); break;
            case 0x90: opcode_0x90_SUB_B(); break;
            case 0x91: opcode_0x91_SUB_C(); break;
            case 0x92: opcode_0x92_SUB_D(); break;
            case 0x93: opcode_0x93_SUB_E(); break;
            case 0x94: opcode_0x94_SUB_H(); break;
            case 0x95: opcode_0x95_SUB_L(); break;
            case 0x96: opcode_0x96_SUB_HL_ptr(); break;
            case 0x97: opcode_0x97_SUB_A(); break;
            case 0x98: opcode_0x98_SBC_A_B(); break;
            case 0x99: opcode_0x99_SBC_A_C(); break;
            case 0x9A: opcode_0x9A_SBC_A_D(); break;
            case 0x9B: opcode_0x9B_SBC_A_E(); break;
            case 0x9C: opcode_0x9C_SBC_A_H(); break;
            case 0x9D: opcode_0x9D_SBC_A_L(); break;
            case 0x9E: opcode_0x9E_SBC_A_HL_ptr(); break;
            case 0x9F: opcode_0x9F_SBC_A_A(); break;
            case 0xA0: opcode_0xA0_AND_B(); break;
            case 0xA1: opcode_0xA1_AND_C(); break;
            case 0xA2: opcode_0xA2_AND_D(); break;
            case 0xA3: opcode_0xA3_AND_E(); break;
            case 0xA4: opcode_0xA4_AND_H(); break;
            case 0xA5: opcode_0xA5_AND_L(); break;
            case 0xA6: opcode_0xA6_AND_HL_ptr(); break;
            case 0xA7: opcode_0xA7_AND_A(); break;
            case 0xA8: opcode_0xA8_XOR_B(); break;
            case 0xA9: opcode_0xA9_XOR_C(); break;
            case 0xAA: opcode_0xAA_XOR_D(); break;
            case 0xAB: opcode_0xAB_XOR_E(); break;
            case 0xAC: opcode_0xAC_XOR_H(); break;
            case 0xAD: opcode_0xAD_XOR_L(); break;
            case 0xAE: opcode_0xAE_XOR_HL_ptr(); break;
            case 0xAF: opcode_0xAF_XOR_A(); break;
            case 0xB0: opcode_0xB0_OR_B(); break;
            case 0xB1: opcode_0xB1_OR_C(); break;
            case 0xB2: opcode_0xB2_OR_D(); break;
            case 0xB3: opcode_0xB3_OR_E(); break;
            case 0xB4: opcode_0xB4_OR_H(); break;
            case 0xB5: opcode_0xB5_OR_L(); break;
            case 0xB6: opcode_0xB6_OR_HL_ptr(); break;
            case 0xB7: opcode_0xB7_OR_A(); break;
            case 0xB8: opcode_0xB8_CP_B(); break;
            case 0xB9: opcode_0xB9_CP_C(); break;
            case 0xBA: opcode_0xBA_CP_D(); break;
            case 0xBB: opcode_0xBB_CP_E(); break;
            case 0xBC: opcode_0xBC_CP_H(); break;
            case 0xBD: opcode_0xBD_CP_L(); break;
            case 0xBE: opcode_0xBE_CP_HL_ptr(); break;
            case 0xBF: opcode_0xBF_CP_A(); break;
            case 0xC0: opcode_0xC0_RET_NZ(); break;
            case 0xC1: opcode_0xC1_POP_BC(); break;
            case 0xC2: opcode_0xC2_JP_NZ_nn(); break;
            case 0xC3: opcode_0xC3_JP_nn(); break;
            case 0xC4: opcode_0xC4_CALL_NZ_nn(); break;
            case 0xC5: opcode_0xC5_PUSH_BC(); break;
            case 0xC6: opcode_0xC6_ADD_A_n(); break;
            case 0xC7: opcode_0xC7_RST_00H(); break;
            case 0xC8: opcode_0xC8_RET_Z(); break;
            case 0xC9: opcode_0xC9_RET(); break;
            case 0xCA: opcode_0xCA_JP_Z_nn(); break;
            case 0xCC: opcode_0xCC_CALL_Z_nn(); break;
            case 0xCD: opcode_0xCD_CALL_nn(); break;
            case 0xCE: opcode_0xCE_ADC_A_n(); break;
            case 0xCF: opcode_0xCF_RST_08H(); break;
            case 0xD0: opcode_0xD0_RET_NC(); break;
            case 0xD1: opcode_0xD1_POP_DE(); break;
            case 0xD2: opcode_0xD2_JP_NC_nn(); break;
            case 0xD3: opcode_0xD3_OUT_n_ptr_A(); break;
            case 0xD4: opcode_0xD4_CALL_NC_nn(); break;
            case 0xD5: opcode_0xD5_PUSH_DE(); break;
            case 0xD6: opcode_0xD6_SUB_n(); break;
            case 0xD7: opcode_0xD7_RST_10H(); break;
            case 0xD8: opcode_0xD8_RET_C(); break;
            case 0xD9: opcode_0xD9_EXX(); break;
            case 0xDA: opcode_0xDA_JP_C_nn(); break;
            case 0xDB: opcode_0xDB_IN_A_n_ptr(); break;
            case 0xDC: opcode_0xDC_CALL_C_nn(); break;
            case 0xDE: opcode_0xDE_SBC_A_n(); break;
            case 0xDF: opcode_0xDF_RST_18H(); break;
            case 0xE0: opcode_0xE0_RET_PO(); break;
            case 0xE1: opcode_0xE1_POP_HL(); break;
            case 0xE2: opcode_0xE2_JP_PO_nn(); break;
            case 0xE3: opcode_0xE3_EX_SP_ptr_HL(); break;
            case 0xE4: opcode_0xE4_CALL_PO_nn(); break;
            case 0xE5: opcode_0xE5_PUSH_HL(); break;
            case 0xE6: opcode_0xE6_AND_n(); break;
            case 0xE7: opcode_0xE7_RST_20H(); break;
            case 0xE8: opcode_0xE8_RET_PE(); break;
            case 0xE9: opcode_0xE9_JP_HL_ptr(); break;
            case 0xEA: opcode_0xEA_JP_PE_nn(); break;
            case 0xEB: opcode_0xEB_EX_DE_HL(); break;
            case 0xEC: opcode_0xEC_CALL_PE_nn(); break;
            case 0xED: {
                uint8_t opcodeED = fetch_next_opcode();
                switch (opcodeED) {
                    case 0x40: opcode_0xED_0x40_IN_B_C_ptr(); break;
                    case 0x41: opcode_0xED_0x41_OUT_C_ptr_B(); break;
                    case 0x42: opcode_0xED_0x42_SBC_HL_BC(); break;
                    case 0x43: opcode_0xED_0x43_LD_nn_ptr_BC(); break;
                    case 0x44: opcode_0xED_0x44_NEG(); break;
                    case 0x45: opcode_0xED_0x45_RETN(); break;
                    case 0x46: opcode_0xED_0x46_IM_0(); break;
                    case 0x47: opcode_0xED_0x47_LD_I_A(); break;
                    case 0x48: opcode_0xED_0x48_IN_C_C_ptr(); break;
                    case 0x49: opcode_0xED_0x49_OUT_C_ptr_C(); break;
                    case 0x4A: opcode_0xED_0x4A_ADC_HL_BC(); break;
                    case 0x4B: opcode_0xED_0x4B_LD_BC_nn_ptr(); break;
                    case 0x4D: opcode_0xED_0x4D_RETI(); break;
                    case 0x4F: opcode_0xED_0x4F_LD_R_A(); break;
                    case 0x50: opcode_0xED_0x50_IN_D_C_ptr(); break;
                    case 0x51: opcode_0xED_0x51_OUT_C_ptr_D(); break;
                    case 0x52: opcode_0xED_0x52_SBC_HL_DE(); break;
                    case 0x53: opcode_0xED_0x53_LD_nn_ptr_DE(); break;
                    case 0x56: opcode_0xED_0x56_IM_1(); break;
                    case 0x57: opcode_0xED_0x57_LD_A_I(); break;
                    case 0x58: opcode_0xED_0x58_IN_E_C_ptr(); break;
                    case 0x59: opcode_0xED_0x59_OUT_C_ptr_E(); break;
                    case 0x5A: opcode_0xED_0x5A_ADC_HL_DE(); break;
                    case 0x5B: opcode_0xED_0x5B_LD_DE_nn_ptr(); break;
                    case 0x5E: opcode_0xED_0x5E_IM_2(); break;
                    case 0x5F: opcode_0xED_0x5F_LD_A_R(); break;
                    case 0x60: opcode_0xED_0x60_IN_H_C_ptr(); break;
                    case 0x61: opcode_0xED_0x61_OUT_C_ptr_H(); break;
                    case 0x62: opcode_0xED_0x62_SBC_HL_HL(); break;
                    case 0x63: opcode_0xED_0x63_LD_nn_ptr_HL_ED(); break;
                    case 0x67: opcode_0xED_0x67_RRD(); break;
                    case 0x68: opcode_0xED_0x68_IN_L_C_ptr(); break;
                    case 0x69: opcode_0xED_0x69_OUT_C_ptr_L(); break;
                    case 0x6A: opcode_0xED_0x6A_ADC_HL_HL(); break;
                    case 0x6B: opcode_0xED_0x6B_LD_HL_nn_ptr_ED(); break;
                    case 0x6F: opcode_0xED_0x6F_RLD(); break;
                    case 0x70: opcode_0xED_0x70_IN_C_ptr(); break;
                    case 0x71: opcode_0xED_0x71_OUT_C_ptr_0(); break;
                    case 0x72: opcode_0xED_0x72_SBC_HL_SP(); break;
                    case 0x73: opcode_0xED_0x73_LD_nn_ptr_SP(); break;
                    case 0x78: opcode_0xED_0x78_IN_A_C_ptr(); break;
                    case 0x79: opcode_0xED_0x79_OUT_C_ptr_A(); break;
                    case 0x7A: opcode_0xED_0x7A_ADC_HL_SP(); break;
                    case 0x7B: opcode_0xED_0x7B_LD_SP_nn_ptr(); break;
                    case 0xA0: opcode_0xED_0xA0_LDI(); break;
                    case 0xA1: opcode_0xED_0xA1_CPI(); break;
                    case 0xA2: opcode_0xED_0xA2_INI(); break;
                    case 0xA3: opcode_0xED_0xA3_OUTI(); break;
                    case 0xA8: opcode_0xED_0xA8_LDD(); break;
                    case 0xA9: opcode_0xED_0xA9_CPD(); break;
                    case 0xAA: opcode_0xED_0xAA_IND(); break;
                    case 0xAB: opcode_0xED_0xAB_OUTD(); break;
                    case 0xB0: opcode_0xED_0xB0_LDIR(); break;
                    case 0xB1: opcode_0xED_0xB1_CPIR(); break;
                    case 0xB2: opcode_0xED_0xB2_INIR(); break;
                    case 0xB3: opcode_0xED_0xB3_OTIR(); break;
                    case 0xB8: opcode_0xED_0xB8_LDDR(); break;
                    case 0xB9: opcode_0xED_0xB9_CPDR(); break;
                    case 0xBA: opcode_0xED_0xBA_INDR(); break;
                    case 0xBB: opcode_0xED_0xBB_OTDR(); break;
                }
                break;
            }
            case 0xEE: opcode_0xEE_XOR_n(); break;
            case 0xEF: opcode_0xEF_RST_28H(); break;
            case 0xF0: opcode_0xF0_RET_P(); break;
            case 0xF1: opcode_0xF1_POP_AF(); break;
            case 0xF2: opcode_0xF2_JP_P_nn(); break;
            case 0xF3: opcode_0xF3_DI(); break;
            case 0xF4: opcode_0xF4_CALL_P_nn(); break;
            case 0xF5: opcode_0xF5_PUSH_AF(); break;
            case 0xF6: opcode_0xF6_OR_n(); break;
            case 0xF7: opcode_0xF7_RST_30H(); break;
            case 0xF8: opcode_0xF8_RET_M(); break;
            case 0xF9: opcode_0xF9_LD_SP_HL(); break;
            case 0xFA: opcode_0xFA_JP_M_nn(); break;
            case 0xFB: opcode_0xFB_EI(); break;
            case 0xFC: opcode_0xFC_CALL_M_nn(); break;
            case 0xFE: opcode_0xFE_CP_n(); break;
            case 0xFF: opcode_0xFF_RST_38H(); break;
        }
        
        if constexpr (TMode == Z80::OperateMode::SingleStep) {
            break;
        } else {
            if (get_ticks() >= ticks_limit) {
                break;
            }
        }
    }
    return get_ticks() - initial_ticks;
}

#endif //__Z80_H__