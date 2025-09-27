#include "Z80.h"
#include <cstdint>

void Z80::request_interrupt(uint8_t data) {
    set_interrupt_pending(true);
    set_interrupt_data(data);
}

void Z80::handle_nmi() {
    add_ticks(11);
    set_halted(false);
    set_IFF2(get_IFF1());
    set_IFF1(false);
    push_word(get_PC());
    set_PC(0x0066);
    set_nmi_pending(false);
}

void Z80::handle_interrupt() {
    add_ticks(13);
    set_IFF2(get_IFF1());
    set_IFF1(false);
    push_word(get_PC());
    switch (get_interrupt_mode()) {
        case 0: {
            uint8_t opcode = get_interrupt_data();
            switch (opcode) {
                case 0xC7: opcode_0xC7_RST_00H(); break;
                case 0xCF: opcode_0xCF_RST_08H(); break;
                case 0xD7: opcode_0xD7_RST_10H(); break;
                case 0xDF: opcode_0xDF_RST_18H(); break;
                case 0xE7: opcode_0xE7_RST_20H(); break;
                case 0xEF: opcode_0xEF_RST_28H(); break;
                case 0xF7: opcode_0xF7_RST_30H(); break;
                case 0xFF: opcode_0xFF_RST_38H(); break;
            }
            break;
        }
        case 1: {
            set_PC(0x0038);
            add_ticks(13);
            break;
        }
        case 2: {
            uint16_t vector_address = (static_cast<uint16_t>(get_I()) << 8) | get_interrupt_data();
            uint16_t handler_address = read_word(vector_address);
            set_PC(handler_address);
            add_ticks(19);
            break;
        }
    }
}

Z80::State Z80::save_state() const {
    Z80::State state;

    // Main Registers
    state.AF = get_AF();
    state.BC = get_BC();
    state.DE = get_DE();
    state.HL = get_HL();
    state.IX = get_IX();
    state.IY = get_IY();
    state.SP = get_SP();
    state.PC = get_PC();

    // Alternate Registers
    state.AFp = get_AFp();
    state.BCp = get_BCp();
    state.DEp = get_DEp();
    state.HLp = get_HLp();

    // Special Registers
    state.I = get_I();
    state.R = get_R();

    // Core State Flags
    state.IFF1 = get_IFF1();
    state.IFF2 = get_IFF2();
    state.halted = is_halted();
    
    // Interrupt State
    state.nmi_pending = is_nmi_pending();
    state.interrupt_pending = is_interrupt_pending();
    state.interrupt_enable_pending = is_interrupt_enable_pending();
    state.interrupt_data = get_interrupt_data();
    state.interrupt_mode = get_interrupt_mode();
    
    // Index mode
    state.index_mode = get_index_mode();

    // Timing
    state.ticks = get_ticks();
    
    return state;
}

void Z80::load_state(const Z80::State& state) {
    // Main Registers
    set_AF(state.AF);
    set_BC(state.BC);
    set_DE(state.DE);
    set_HL(state.HL);
    set_IX(state.IX);
    set_IY(state.IY);
    set_SP(state.SP);
    set_PC(state.PC);

    // Alternate Registers
    set_AFp(state.AFp);
    set_BCp(state.BCp);
    set_DEp(state.DEp);
    set_HLp(state.HLp);

    // Special Registers
    set_I(state.I);
    set_R(state.R);

    // Core State Flags
    set_IFF1(state.IFF1);
    set_IFF2(state.IFF2);
    set_halted(state.halted);

    // Interrupt State
    set_nmi_pending(state.nmi_pending);
    set_interrupt_pending(state.interrupt_pending);
    set_interrupt_enable_pending(state.interrupt_enable_pending);
    set_interrupt_data(state.interrupt_data);
    set_interrupt_mode(state.interrupt_mode);

    // Index Mode
    set_index_mode(state.index_mode);

    // Timing
    set_ticks(state.ticks);
}

uint8_t Z80::inc_8bit(uint8_t value) {
    uint8_t result = value + 1;
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (value & 0x0F) == 0x0F);
    set_flag_if(FLAG_PV, value == 0x7F);
    clear_flag(FLAG_N);
    set_flag_if(FLAG_Y, (result & 0x20) != 0);
    set_flag_if(FLAG_X, (result & 0x08) != 0);
    return result;
}

uint8_t Z80::dec_8bit(uint8_t value) {
    uint8_t result = value - 1;
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (value & 0x0F) == 0x00);
    set_flag_if(FLAG_PV, value == 0x80);
    set_flag(FLAG_N);
    set_flag_if(FLAG_Y, (result & 0x20) != 0);
    set_flag_if(FLAG_X, (result & 0x08) != 0);
    return result;
}

void Z80::and_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a & value;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, true);
    set_flag_if(FLAG_PV, is_parity_even(result));
    clear_flag(FLAG_N);
    clear_flag(FLAG_C);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::or_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a | value;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    clear_flag(FLAG_H);
    set_flag_if(FLAG_PV, is_parity_even(result));
    clear_flag(FLAG_N);
    clear_flag(FLAG_C);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::xor_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a ^ value;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    clear_flag(FLAG_H);
    set_flag_if(FLAG_PV, is_parity_even(result));
    clear_flag(FLAG_N);
    clear_flag(FLAG_C);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::cp_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a - value;
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (a & 0x0F) < (value & 0x0F));
    set_flag_if(FLAG_PV, (a ^ value) & (a ^ result) & 0x80);
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, a < value);
    set_flag_if(FLAG_X, (value & FLAG_X));
    set_flag_if(FLAG_Y, (value & FLAG_Y));
}

void Z80::add_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a + value;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (a & 0x0F) + (value & 0x0F) > 0x0F);
    set_flag_if(FLAG_PV, ((a ^ value ^ 0x80) & (a ^ result)) & 0x80);
    clear_flag(FLAG_N);
    set_flag_if(FLAG_C, (uint16_t)a + (uint16_t)value > 0xFF);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::adc_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t carry = is_C_flag_set();
    uint8_t result = a + value + carry;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (a & 0x0F) + (value & 0x0F) + carry > 0x0F);
    set_flag_if(FLAG_PV, ((a ^ value ^ 0x80) & (a ^ result)) & 0x80);
    clear_flag(FLAG_N);
    set_flag_if(FLAG_C, (uint16_t)a + (uint16_t)value + (uint16_t)carry > 0xFF);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::sub_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t result = a - value;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (a & 0x0F) < (value & 0x0F));
    set_flag_if(FLAG_PV, ((a ^ value) & (a ^ result)) & 0x80);
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, a < value);
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

void Z80::sbc_8bit(uint8_t value) {
    uint8_t a = get_A();
    uint8_t carry = is_C_flag_set();
    uint8_t result = a - value - carry;
    set_A(result);
    set_flag_if(FLAG_S, result & FLAG_S);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (a & 0x0F) < ((value & 0x0F) + carry));
    set_flag_if(FLAG_PV, ((a ^ value) & (a ^ result)) & 0x80);
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, (uint16_t)a < ((uint16_t)value + (uint16_t)carry));
    set_flag_if(FLAG_X, result & FLAG_X);
    set_flag_if(FLAG_Y, result & FLAG_Y);
}

uint16_t Z80::add_16bit(uint16_t reg, uint16_t value) {
    uint32_t result = (uint32_t)reg + (uint32_t)value;
    clear_flag(FLAG_N);
    set_flag_if(FLAG_H, ((reg & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
    set_flag_if(FLAG_C, result > 0xFFFF);
    set_flag_if(FLAG_Y, (result & 0x2000) != 0); // Bit 13
    set_flag_if(FLAG_X, (result & 0x0800) != 0); // Bit 11
    return (uint16_t)result;
}

uint16_t Z80::adc_16bit(uint16_t reg, uint16_t value) {
    uint8_t carry = is_C_flag_set();
    uint32_t result = reg + value + carry;
    set_flag_if(FLAG_S, (result & 0x8000) != 0);
    set_flag_if(FLAG_Z, (result & 0xFFFF) == 0);
    set_flag_if(FLAG_H, ((reg & 0xFFF) + (value & 0xFFF) + carry) & 0x1000);
    set_flag_if(FLAG_PV, (static_cast<int16_t>((reg ^ result) & (value ^ result)) < 0));
    clear_flag(FLAG_N);
    set_flag_if(FLAG_C, (result & 0x10000) != 0);
    set_flag_if(FLAG_Y, (result & 0x2000) != 0); 
    set_flag_if(FLAG_X, (result & 0x0800) != 0); 
    return static_cast<uint16_t>(result);
}

uint16_t Z80::sbc_16bit(uint16_t reg, uint16_t value) {
    uint8_t carry = is_C_flag_set();
    uint32_t result = reg - value - carry;
    set_flag_if(FLAG_S, (result & 0x8000) != 0);
    set_flag_if(FLAG_Z, (result & 0xFFFF) == 0);
    set_flag_if(FLAG_H, ((reg & 0xFFF) - (value & 0xFFF) - carry) & 0x1000);
    set_flag_if(FLAG_PV, (static_cast<int16_t>((reg ^ result) & (reg ^ value)) < 0));
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, (result & 0x10000) != 0);
    set_flag_if(FLAG_Y, (result & 0x2000) != 0);
    set_flag_if(FLAG_X, (result & 0x0800) != 0);
    return static_cast<uint16_t>(result);
}

uint8_t Z80::rlc_8bit(uint8_t value) {
    uint8_t result = (value << 1) | (value >> 7);
    set_flag_if(FLAG_C, (value & 0x80) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::rrc_8bit(uint8_t value) {
    uint8_t result = (value >> 1) | (value << 7);
    set_flag_if(FLAG_C, (value & 0x01) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::rl_8bit(uint8_t value) {
    uint8_t old_carry = is_C_flag_set() ? 1 : 0;
    uint8_t result = (value << 1) | old_carry;
    set_flag_if(FLAG_C, (value & 0x80) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::rr_8bit(uint8_t value) {
    uint8_t old_carry = is_C_flag_set() ? 0x80 : 0;
    uint8_t result = (value >> 1) | old_carry;
    set_flag_if(FLAG_C, (value & 0x01) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::sla_8bit(uint8_t value) {
    uint8_t result = value << 1;
    set_flag_if(FLAG_C, (value & 0x80) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::sra_8bit(uint8_t value) {
    uint8_t result = (value >> 1) | (value & 0x80);
    set_flag_if(FLAG_C, (value & 0x01) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::sll_8bit(uint8_t value) {
    uint8_t result = (value << 1) | 0x01;
    set_flag_if(FLAG_C, (value & 0x80) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

uint8_t Z80::srl_8bit(uint8_t value) {
    uint8_t result = value >> 1;
    set_flag_if(FLAG_C, (value & 0x01) != 0);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_PV, is_parity_even(result));
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    return result;
}

void Z80::bit_8bit(uint8_t bit, uint8_t value) {
    set_flag(FLAG_H);
    clear_flag(FLAG_N);

    bool bit_is_zero = (value & (1 << bit)) == 0;
    set_flag_if(FLAG_Z, bit_is_zero);
    set_flag_if(FLAG_PV, bit_is_zero);

    if (bit == 7) {
        set_flag_if(FLAG_S, (value & 0x80) != 0); // S = V7
    } else {
        clear_flag(FLAG_S);
    }
}

uint8_t Z80::res_8bit(uint8_t bit, uint8_t value) {
    return value & ~(1 << bit);
}

uint8_t Z80::set_8bit(uint8_t bit, uint8_t value) {
    return value | (1 << bit);
}

uint8_t Z80::in_r_c() {
    uint8_t value = read_byte_from_io(get_BC());
    
    set_flag_if(FLAG_S, (value & 0x80) != 0);
    set_flag_if(FLAG_Z, value == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, is_parity_even(value));
    set_flag_if(FLAG_X, (value & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (value & FLAG_Y) != 0);
    
    return value;
}

void Z80::out_c_r(uint8_t value) {
    write_byte_to_io(get_BC(), value);
}

void Z80::handle_CB() {
    uint8_t opcode = fetch_next_opcode();
    uint8_t operation_group = opcode >> 6;
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t target_reg = opcode & 0x07;
    uint8_t value;
    uint8_t result = 0;
    uint16_t flags_source = 0;

    switch (target_reg) {
        case 0: value = get_B(); break;
        case 1: value = get_C(); break;
        case 2: value = get_D(); break;
        case 3: value = get_E(); break;
        case 4: value = get_H(); break;
        case 5: value = get_L(); break;
        case 6:
            flags_source = get_HL();
            value = read_byte(flags_source);
            add_ticks(3);
            break;
        case 7: value = get_A(); break;
    }
    switch (operation_group) {
        case 0:
            switch(bit) {
                case 0: result = rlc_8bit(value); break;
                case 1: result = rrc_8bit(value); break;
                case 2: result = rl_8bit(value); break;
                case 3: result = rr_8bit(value); break;
                case 4: result = sla_8bit(value); break;
                case 5: result = sra_8bit(value); break;
                case 6: result = sll_8bit(value); break;
                case 7: result = srl_8bit(value); break;
            }
            break;
        case 1: {
            bit_8bit(bit, value);
            if (target_reg == 6) {
                add_ticks(1);
                set_flag_if(FLAG_X, (flags_source & 0x0800) != 0); // Bit 11 -> F3 (XF)
                set_flag_if(FLAG_Y, (flags_source & 0x2000) != 0); // Bit 13 -> F5 (YF)
            } else {
                set_flag_if(FLAG_X, (value & 0x08) != 0); // Bit 3 -> F3 (XF)
                set_flag_if(FLAG_Y, (value & 0x20) != 0); // Bit 5 -> F5 (YF)
            }
            return;
        }
        case 2: result = res_8bit(bit, value); break;
        case 3: result = set_8bit(bit, value); break;
    }
    switch (target_reg) {
        case 0: set_B(result); break;
        case 1: set_C(result); break;
        case 2: set_D(result); break;
        case 3: set_E(result); break;
        case 4: set_H(result); break;
        case 5: set_L(result); break;
        case 6:
            write_byte(get_HL(), result);
            add_ticks(3);
            add_ticks(1);
            break;
        case 7: set_A(result); break;
    }
}

void Z80::handle_CB_indexed(uint16_t index_register) {
    int8_t offset = static_cast<int8_t>(fetch_next_byte()); // Memory read cost (3T)
    uint8_t opcode = fetch_next_byte(); // Opcode read treated as memryread (3T), no R register modification
    uint16_t address = index_register + offset;
    add_ticks(2); // Internal summary cost (2T)
    uint8_t value = read_byte(address);
    add_ticks(3); // Memory read cost (3T)
    uint8_t operation_group = opcode >> 6;
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t result = value;
    if (operation_group == 1) {
        bit_8bit(bit, value); 
        add_ticks(1);
        set_flag_if(FLAG_X, (address & 0x0800) != 0); // Kopiuj bit 11 z adresu
        set_flag_if(FLAG_Y, (address & 0x2000) != 0); // Kopiuj bit 13 z adresu
        return;
    }
    switch(operation_group) {
        case 0:
            switch(bit) {
                case 0: result = rlc_8bit(value); break;
                case 1: result = rrc_8bit(value); break;
                case 2: result = rl_8bit(value); break;
                case 3: result = rr_8bit(value); break;
                case 4: result = sla_8bit(value); break;
                case 5: result = sra_8bit(value); break;
                case 6: result = sll_8bit(value); break;
                case 7: result = srl_8bit(value); break;
            }
            break;
        case 2: result = res_8bit(bit, value); break;
        case 3: result = set_8bit(bit, value); break;
    }
    write_byte(address, result);
    add_ticks(3); // Memory write cost (3T)
    add_ticks(1); // Internal cycle (1T)
    uint8_t target_reg_code = opcode & 0x07;
    if (target_reg_code != 0x06) {
        switch(target_reg_code) {
            case 0: set_B(result); break;
            case 1: set_C(result); break;
            case 2: set_D(result); break;
            case 3: set_E(result); break;
            case 4: set_H(result); break;
            case 5: set_L(result); break;
            case 7: set_A(result); break;
        }
    }
}

void Z80::opcode_0x00_NOP() {
    // Total Ticks: 4
    // 4 from fetch_next_opcode()
}

void Z80::opcode_0x01_LD_BC_nn() {
    // Total Ticks: 10
    // 4 (fetch op) + 6 (fetch word)
    set_BC(fetch_next_word());
}

void Z80::opcode_0x02_LD_BC_ptr_A() {
    // Total Ticks: 7
    // 4 (fetch op) + 3 (mem write)
    write_byte(get_BC(), get_A());
    add_ticks(3);
}

void Z80::opcode_0x03_INC_BC() {
    // Total Ticks: 6
    // 4 (fetch op) + 2 (internal op)
    add_ticks(2);
    set_BC(get_BC() + 1);
}

void Z80::opcode_0x04_INC_B() {
    // Total Ticks: 4
    set_B(inc_8bit(get_B()));
}

void Z80::opcode_0x05_DEC_B() {
    // Total Ticks: 4
    set_B(dec_8bit(get_B()));
}

void Z80::opcode_0x06_LD_B_n() {
    // Total Ticks: 7
    // 4 (fetch op) + 3 (fetch byte)
    set_B(fetch_next_byte());
}

void Z80::opcode_0x07_RLCA() {
    // Total Ticks: 4
    uint8_t value = get_A();
    uint8_t carry_bit = (value >> 7) & 0x01;
    uint8_t result = (value << 1) | carry_bit;
    set_A(result);

    set_flag_if(FLAG_C, carry_bit == 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
}

void Z80::opcode_0x08_EX_AF_AFp() {
    // Total Ticks: 4
    uint16_t temp_AF = get_AF();
    set_AF(get_AFp());
    set_AFp(temp_AF);
}

void Z80::opcode_0x09_ADD_HL_BC() {
    // Total Ticks HL: 11, IX/IY: 15
    // 4 (fetch op) + 7 (internal op) -> 11T
    // 4 (DD) + 4 (fetch op) + 7 (internal op) -> 15T
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(add_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(add_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(add_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0x0A_LD_A_BC_ptr() {
    // Total Ticks: 7
    // 4 (fetch op) + 3 (mem read)
    set_A(read_byte(get_BC()));
    add_ticks(3);
}

void Z80::opcode_0x0B_DEC_BC() {
    // Total Ticks: 6
    // 4 (fetch op) + 2 (internal op)
    add_ticks(2);
    set_BC(get_BC() - 1);
}

void Z80::opcode_0x0C_INC_C() {
    // Total Ticks: 4
    set_C(inc_8bit(get_C()));
}

void Z80::opcode_0x0D_DEC_C() {
    // Total Ticks: 4
    set_C(dec_8bit(get_C()));
}

void Z80::opcode_0x0E_LD_C_n() {
    // Total Ticks: 7
    set_C(fetch_next_byte());
}

void Z80::opcode_0x0F_RRCA() {
    // Total Ticks: 4
    uint8_t value = get_A();
    uint8_t carry_bit = value & 0x01;
    uint8_t result = (value >> 1) | (carry_bit << 7);
    set_A(result);

    set_flag_if(FLAG_C, carry_bit == 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
}

void Z80::opcode_0x10_DJNZ_d() {
    // Total Ticks: 8 (no jump) / 13 (jump)
    // 4(fetch op) + 1(internal op) + 3(fetch d) = 8
    add_ticks(1);
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    uint8_t new_b_value = get_B() - 1;
    set_B(new_b_value);
    
    if (new_b_value != 0) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x11_LD_DE_nn() {
    // Total Ticks: 10
    set_DE(fetch_next_word());
}

void Z80::opcode_0x12_LD_DE_ptr_A() {
    // Total Ticks: 7
    write_byte(get_DE(), get_A());
    add_ticks(3);
}

void Z80::opcode_0x13_INC_DE() {
    // Total Ticks: 6
    add_ticks(2);
    set_DE(get_DE() + 1);
}

void Z80::opcode_0x14_INC_D() {
    // Total Ticks: 4
    set_D(inc_8bit(get_D()));
}

void Z80::opcode_0x15_DEC_D() {
    // Total Ticks: 4
    set_D(dec_8bit(get_D()));
}

void Z80::opcode_0x16_LD_D_n() {
    // Total Ticks: 7
    set_D(fetch_next_byte());
}

void Z80::opcode_0x17_RLA() {
    // Total Ticks: 4
    uint8_t value = get_A();
    uint8_t old_carry_bit = is_C_flag_set() ? 1 : 0;
    uint8_t new_carry_bit = (value >> 7) & 0x01;
    uint8_t result = (value << 1) | old_carry_bit;
    set_A(result);

    set_flag_if(FLAG_C, new_carry_bit);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
}

void Z80::opcode_0x18_JR_d() {
    // Total Ticks: 12 = 4(fetch op) + 3(fetch d) + 5(internal)
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    add_ticks(5);
    set_PC(get_PC() + offset);
}

void Z80::opcode_0x19_ADD_HL_DE() {
    // Total Ticks HL: 11, IX/IY: 15
    add_ticks(7);
    uint16_t val_de = get_DE();
    switch (get_index_mode()) {
        case IndexMode::HL:
            set_HL(add_16bit(get_HL(), val_de));
            break;
        case IndexMode::IX:
            set_IX(add_16bit(get_IX(), val_de));
            break;
        case IndexMode::IY:
            set_IY(add_16bit(get_IY(), val_de));
            break;
    }
}

void Z80::opcode_0x1A_LD_A_DE_ptr() {
    // Total Ticks: 7
    set_A(read_byte(get_DE()));
    add_ticks(3);
}

void Z80::opcode_0x1B_DEC_DE() {
    // Total Ticks: 6
    add_ticks(2);
    set_DE(get_DE() - 1);
}

void Z80::opcode_0x1C_INC_E() {
    // Total Ticks: 4
    set_E(inc_8bit(get_E()));
}

void Z80::opcode_0x1D_DEC_E() {
    // Total Ticks: 4
    set_E(dec_8bit(get_E()));
}

void Z80::opcode_0x1E_LD_E_n() {
    // Total Ticks: 7
    set_E(fetch_next_byte());
}

void Z80::opcode_0x1F_RRA() {
    // Total Ticks: 4
    uint8_t value = get_A();
    bool old_carry_bit = is_C_flag_set();
    bool new_carry_bit = (value & 0x01) != 0;
    uint8_t result = (value >> 1) | (old_carry_bit ? 0x80 : 0);
    set_A(result);

    set_flag_if(FLAG_C, new_carry_bit);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
}

void Z80::opcode_0x20_JR_NZ_d() {
    // Total Ticks: 7 (no jump) / 12 (jump)
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (!is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x21_LD_HL_nn() {
    // Total Ticks HL: 10, IX/IY: 14
    uint16_t value = fetch_next_word();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(value); break;
        case IndexMode::IX: set_IX(value); break;
        case IndexMode::IY: set_IY(value); break;
    }
}

void Z80::opcode_0x22_LD_nn_ptr_HL() {
    // Total Ticks HL: 16, IX/IY: 20
    uint16_t address = fetch_next_word();
    add_ticks(6); // 3T for L write, 3T for H write
    switch (get_index_mode()) {
        case IndexMode::HL: write_word(address, get_HL()); break;
        case IndexMode::IX: write_word(address, get_IX()); break;
        case IndexMode::IY: write_word(address, get_IY()); break;
    }
}

void Z80::opcode_0x23_INC_HL() {
    // Total Ticks HL: 6, IX/IY: 10
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(get_HL() + 1); break;
        case IndexMode::IX: set_IX(get_IX() + 1); break;
        case IndexMode::IY: set_IY(get_IY() + 1); break;
    }
}

void Z80::opcode_0x24_INC_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(inc_8bit(get_H())); break;
        case IndexMode::IX: set_IXH(inc_8bit(get_IXH())); break;
        case IndexMode::IY: set_IYH(inc_8bit(get_IYH())); break;
    }
}

void Z80::opcode_0x25_DEC_H() {
    // Total Ticks HL: 4, IX/IY: 8
     switch (get_index_mode()) {
        case IndexMode::HL: set_H(dec_8bit(get_H())); break;
        case IndexMode::IX: set_IXH(dec_8bit(get_IXH())); break;
        case IndexMode::IY: set_IYH(dec_8bit(get_IYH())); break;
    }
}

void Z80::opcode_0x26_LD_H_n() {
    // Total Ticks HL: 7, IX/IY: 11
    uint8_t value = fetch_next_byte();
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(value); break;
        case IndexMode::IX: set_IXH(value); break;
        case IndexMode::IY: set_IYH(value); break;
    }
}

void Z80::opcode_0x27_DAA() {
    uint8_t a = get_A();
    uint8_t correction = 0;
    bool carry = is_C_flag_set();
    if (is_N_flag_set()) {
        if (carry || (a > 0x99)) { 
            correction = 0x60; 
        }
        if (is_H_flag_set() || ((a & 0x0F) > 0x09)) {
            correction |= 0x06;
        }
        set_flag_if(FLAG_H, is_H_flag_set() && ((a & 0x0F) < 0x06));
        set_A(a - correction);
    } else {
        if (carry || (a > 0x99)) {
            correction = 0x60;
            set_flag(FLAG_C);
        }
        if (is_H_flag_set() || ((a & 0x0F) > 0x09)) {
            correction |= 0x06;
        }
        set_flag_if(FLAG_H, (a & 0x0F) > 0x09);
        set_A(a + correction);
    }
    if (correction >= 0x60) {
        set_flag(FLAG_C);
    }
    set_flag_if(FLAG_S, get_A() & 0x80);
    set_flag_if(FLAG_Z, get_A() == 0);
    set_flag_if(FLAG_PV, is_parity_even(get_A()));
    set_flag_if(FLAG_X, get_A() & FLAG_X);
    set_flag_if(FLAG_Y, get_A() & FLAG_Y);
}

void Z80::opcode_0x28_JR_Z_d() {
    // Total Ticks: 7/12
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x29_ADD_HL_HL() {
    // Total Ticks HL: 11, IX/IY: 15
    add_ticks(7);
    switch (get_index_mode()) {
        case IndexMode::HL:
            set_HL(add_16bit(get_HL(), get_HL()));
            break;
        case IndexMode::IX:
            set_IX(add_16bit(get_IX(), get_IX()));
            break;
        case IndexMode::IY:
            set_IY(add_16bit(get_IY(), get_IY()));
            break;
    }
}

void Z80::opcode_0x2A_LD_HL_nn_ptr() {
    // Total Ticks HL: 16, IX/IY: 20
    uint16_t address = fetch_next_word();
    add_ticks(6);
    switch (get_index_mode()) {
        case IndexMode::HL:
            set_HL(read_word(address));
            break;
        case IndexMode::IX:
            set_IX(read_word(address));
            break;
        case IndexMode::IY:
            set_IY(read_word(address));
            break;
    }
}

void Z80::opcode_0x2B_DEC_HL() {
    // Total Ticks HL: 6, IX/IY: 10
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(get_HL() - 1); break;
        case IndexMode::IX: set_IX(get_IX() - 1); break;
        case IndexMode::IY: set_IY(get_IY() - 1); break;
    }
}

void Z80::opcode_0x2C_INC_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(inc_8bit(get_L())); break;
        case IndexMode::IX: set_IXL(inc_8bit(get_IXL())); break;
        case IndexMode::IY: set_IYL(inc_8bit(get_IYL())); break;
    }
}

void Z80::opcode_0x2D_DEC_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(dec_8bit(get_L())); break;
        case IndexMode::IX: set_IXL(dec_8bit(get_IXL())); break;
        case IndexMode::IY: set_IYL(dec_8bit(get_IYL())); break;
    }
}

void Z80::opcode_0x2E_LD_L_n() {
    // Total Ticks HL: 7, IX/IY: 11
    uint8_t value = fetch_next_byte();
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(value); break;
        case IndexMode::IX: set_IXL(value); break;
        case IndexMode::IY: set_IYL(value); break;
    }
}

void Z80::opcode_0x2F_CPL() {
    // Total Ticks: 4
    uint8_t a = get_A();
    a = ~a;
    set_A(a);
    set_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (a & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (a & FLAG_X) != 0);
}

void Z80::opcode_0x30_JR_NC_d() {
    // Total Ticks: 7/12
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (!is_C_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x31_LD_SP_nn() {
    // Total Ticks: 10
    set_SP(fetch_next_word());
}

void Z80::opcode_0x32_LD_nn_ptr_A() {
    // Total Ticks: 13 = 4(fetch op)+6(fetch word)+3(write)
    uint16_t address = fetch_next_word();
    write_byte(address, get_A());
    add_ticks(3);
}

void Z80::opcode_0x33_INC_SP() {
    // Total Ticks: 6
    add_ticks(2);
    set_SP(get_SP() + 1);
}

void Z80::opcode_0x34_INC_HL_ptr() {
    // Total Ticks HL: 11, IX/IY: 23
    if (get_index_mode() == IndexMode::HL) {
        uint16_t address = get_HL();
        uint8_t value = read_byte(address);
        add_ticks(3);
        add_ticks(1);
        write_byte(address, inc_8bit(value));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        uint8_t value = read_byte(address);
        add_ticks(3);
        write_byte(address, inc_8bit(value));
        add_ticks(4);
    }
}

void Z80::opcode_0x35_DEC_HL_ptr() {
    // Total Ticks HL: 11, IX/IY: 23
    if (get_index_mode() == IndexMode::HL) {
        uint16_t address = get_HL();
        uint8_t value = read_byte(address);
        add_ticks(3);
        add_ticks(1);
        write_byte(address, dec_8bit(value));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        uint8_t value = read_byte(address);
        add_ticks(3);
        write_byte(address, dec_8bit(value));
        add_ticks(4);
    }
}

void Z80::opcode_0x36_LD_HL_ptr_n() {
    // Total Ticks HL: 10, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        uint8_t value = fetch_next_byte();
        write_byte(get_HL(), value);
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint8_t value = fetch_next_byte();
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, value);
    }
}

void Z80::opcode_0x37_SCF() {
    // Total Ticks: 4
    set_flag(FLAG_C);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_X, (get_A() & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (get_A() & FLAG_Y) != 0);
}

void Z80::opcode_0x38_JR_C_d() {
    // Total Ticks: 7/12
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (is_C_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x39_ADD_HL_SP() {
    // Total Ticks HL: 11, IX/IY: 15
    add_ticks(7);
    uint16_t val_sp = get_SP();
    switch (get_index_mode()) {
        case IndexMode::HL:
            set_HL(add_16bit(get_HL(), val_sp));
            break;
        case IndexMode::IX:
            set_IX(add_16bit(get_IX(), val_sp));
            break;
        case IndexMode::IY:
            set_IY(add_16bit(get_IY(), val_sp));
            break;
    }
}

void Z80::opcode_0x3A_LD_A_nn_ptr() {
    // Total Ticks: 13
    uint16_t address = fetch_next_word();
    set_A(read_byte(address));
    add_ticks(3);
}

void Z80::opcode_0x3B_DEC_SP() {
    // Total Ticks: 6
    add_ticks(2);
    set_SP(get_SP() - 1);
}

void Z80::opcode_0x3C_INC_A() {
    // Total Ticks: 4
    set_A(inc_8bit(get_A()));
}

void Z80::opcode_0x3D_DEC_A() {
    // Total Ticks: 4
    set_A(dec_8bit(get_A()));
}

void Z80::opcode_0x3E_LD_A_n() {
    // Total Ticks: 7
    set_A(fetch_next_byte());
}

void Z80::opcode_0x3F_CCF() {
    // Total Ticks: 4
    bool old_C_flag = is_C_flag_set();
    set_flag_if(FLAG_C, !old_C_flag);
    clear_flag(FLAG_N);
    set_flag_if(FLAG_H, old_C_flag);
    set_flag_if(FLAG_Y, (get_A() & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (get_A() & FLAG_X) != 0);
}
void Z80::opcode_0x40_LD_B_B() {
    // Total Ticks: 4
}

void Z80::opcode_0x41_LD_B_C() {
    // Total Ticks: 4
    set_B(get_C());
}

void Z80::opcode_0x42_LD_B_D() {
    // Total Ticks: 4
    set_B(get_D());
}

void Z80::opcode_0x43_LD_B_E() {
    // Total Ticks: 4
    set_B(get_E());
}

void Z80::opcode_0x44_LD_B_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_B(get_H()); break;
        case IndexMode::IX: set_B(get_IXH()); break;
        case IndexMode::IY: set_B(get_IYH()); break;
    }
}

void Z80::opcode_0x45_LD_B_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_B(get_L()); break;
        case IndexMode::IX: set_B(get_IXL()); break;
        case IndexMode::IY: set_B(get_IYL()); break;
    }
}

void Z80::opcode_0x46_LD_B_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_B(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_B(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x47_LD_B_A() {
    // Total Ticks: 4
    set_B(get_A());
}

void Z80::opcode_0x48_LD_C_B() {
    // Total Ticks: 4
    set_C(get_B());
}

void Z80::opcode_0x49_LD_C_C() {
    // Total Ticks: 4
}

void Z80::opcode_0x4A_LD_C_D() {
    // Total Ticks: 4
    set_C(get_D());
}

void Z80::opcode_0x4B_LD_C_E() {
    // Total Ticks: 4
    set_C(get_E());
}

void Z80::opcode_0x4C_LD_C_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_C(get_H()); break;
        case IndexMode::IX: set_C(get_IXH()); break;
        case IndexMode::IY: set_C(get_IYH()); break;
    }
}

void Z80::opcode_0x4D_LD_C_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_C(get_L()); break;
        case IndexMode::IX: set_C(get_IXL()); break;
        case IndexMode::IY: set_C(get_IYL()); break;
    }
}

void Z80::opcode_0x4E_LD_C_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_C(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_C(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x4F_LD_C_A() {
    // Total Ticks: 4
    set_C(get_A());
}

void Z80::opcode_0x50_LD_D_B() {
    // Total Ticks: 4
    set_D(get_B());
}

void Z80::opcode_0x51_LD_D_C() {
    // Total Ticks: 4
    set_D(get_C());
}

void Z80::opcode_0x52_LD_D_D() {
    // Total Ticks: 4
}

void Z80::opcode_0x53_LD_D_E() {
    // Total Ticks: 4
    set_D(get_E());
}

void Z80::opcode_0x54_LD_D_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_D(get_H()); break;
        case IndexMode::IX: set_D(get_IXH()); break;
        case IndexMode::IY: set_D(get_IYH()); break;
    }
}

void Z80::opcode_0x55_LD_D_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_D(get_L()); break;
        case IndexMode::IX: set_D(get_IXL()); break;
        case IndexMode::IY: set_D(get_IYL()); break;
    }
}

void Z80::opcode_0x56_LD_D_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_D(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_D(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x57_LD_D_A() {
    // Total Ticks: 4
    set_D(get_A());
}

void Z80::opcode_0x58_LD_E_B() {
    // Total Ticks: 4
    set_E(get_B());
}

void Z80::opcode_0x59_LD_E_C() {
    // Total Ticks: 4
    set_E(get_C());
}

void Z80::opcode_0x5A_LD_E_D() {
    // Total Ticks: 4
    set_E(get_D());
}

void Z80::opcode_0x5B_LD_E_E() {
    // Total Ticks: 4
}

void Z80::opcode_0x5C_LD_E_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_E(get_H()); break;
        case IndexMode::IX: set_E(get_IXH()); break;
        case IndexMode::IY: set_E(get_IYH()); break;
    }
}

void Z80::opcode_0x5D_LD_E_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_E(get_L()); break;
        case IndexMode::IX: set_E(get_IXL()); break;
        case IndexMode::IY: set_E(get_IYL()); break;
    }
}

void Z80::opcode_0x5E_LD_E_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_E(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_E(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x5F_LD_E_A() {
    // Total Ticks: 4
    set_E(get_A());
}

void Z80::opcode_0x60_LD_H_B() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_B()); break;
        case IndexMode::IX: set_IXH(get_B()); break;
        case IndexMode::IY: set_IYH(get_B()); break;
    }
}

void Z80::opcode_0x61_LD_H_C() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_C()); break;
        case IndexMode::IX: set_IXH(get_C()); break;
        case IndexMode::IY: set_IYH(get_C()); break;
    }
}

void Z80::opcode_0x62_LD_H_D() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_D()); break;
        case IndexMode::IX: set_IXH(get_D()); break;
        case IndexMode::IY: set_IYH(get_D()); break;
    }
}

void Z80::opcode_0x63_LD_H_E() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_E()); break;
        case IndexMode::IX: set_IXH(get_E()); break;
        case IndexMode::IY: set_IYH(get_E()); break;
    }
}

void Z80::opcode_0x64_LD_H_H() {
    // Total Ticks HL: 4, IX/IY: 8
}

void Z80::opcode_0x65_LD_H_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_L()); break;
        case IndexMode::IX: set_IXH(get_IXL()); break;
        case IndexMode::IY: set_IYH(get_IYL()); break;
    }
}

void Z80::opcode_0x66_LD_H_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_H(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_H(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x67_LD_H_A() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_A()); break;
        case IndexMode::IX: set_IXH(get_A()); break;
        case IndexMode::IY: set_IYH(get_A()); break;
    }
}

void Z80::opcode_0x68_LD_L_B() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_B()); break;
        case IndexMode::IX: set_IXL(get_B()); break;
        case IndexMode::IY: set_IYL(get_B()); break;
    }
}

void Z80::opcode_0x69_LD_L_C() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_C()); break;
        case IndexMode::IX: set_IXL(get_C()); break;
        case IndexMode::IY: set_IYL(get_C()); break;
    }
}

void Z80::opcode_0x6A_LD_L_D() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_D()); break;
        case IndexMode::IX: set_IXL(get_D()); break;
        case IndexMode::IY: set_IYL(get_D()); break;
    }
}

void Z80::opcode_0x6B_LD_L_E() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_E()); break;
        case IndexMode::IX: set_IXL(get_E()); break;
        case IndexMode::IY: set_IYL(get_E()); break;
    }
}

void Z80::opcode_0x6C_LD_L_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_H()); break;
        case IndexMode::IX: set_IXL(get_IXH()); break;
        case IndexMode::IY: set_IYL(get_IYH()); break;
    }
}

void Z80::opcode_0x6D_LD_L_L() {
    // Total Ticks HL: 4, IX/IY: 8
}

void Z80::opcode_0x6E_LD_L_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_L(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_L(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x6F_LD_L_A() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_A()); break;
        case IndexMode::IX: set_IXL(get_A()); break;
        case IndexMode::IY: set_IYL(get_A()); break;
    }
}

void Z80::opcode_0x70_LD_HL_ptr_B() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_B());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_B());
        add_ticks(3);
    }
}

void Z80::opcode_0x71_LD_HL_ptr_C() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_C());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_C());
        add_ticks(3);
    }
}

void Z80::opcode_0x72_LD_HL_ptr_D() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_D());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_D());
        add_ticks(3);
    }
}

void Z80::opcode_0x73_LD_HL_ptr_E() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_E());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_E());
        add_ticks(3);
    }
}

void Z80::opcode_0x74_LD_HL_ptr_H() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_H());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_H());
        add_ticks(3);
    }
}

void Z80::opcode_0x75_LD_HL_ptr_L() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_L());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_L());
        add_ticks(3);
    }
}

void Z80::opcode_0x76_HALT() {
    // Total Ticks: 4
    set_halted(true);
}

void Z80::opcode_0x77_LD_HL_ptr_A() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_A());
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_A());
        add_ticks(3);
    }
}

void Z80::opcode_0x78_LD_A_B() {
    // Total Ticks: 4
    set_A(get_B());
}

void Z80::opcode_0x79_LD_A_C() {
    // Total Ticks: 4
    set_A(get_C());
}

void Z80::opcode_0x7A_LD_A_D() {
    // Total Ticks: 4
    set_A(get_D());
}

void Z80::opcode_0x7B_LD_A_E() {
    // Total Ticks: 4
    set_A(get_E());
}

void Z80::opcode_0x7C_LD_A_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_A(get_H()); break;
        case IndexMode::IX: set_A(get_IXH()); break;
        case IndexMode::IY: set_A(get_IYH()); break;
    }
}

void Z80::opcode_0x7D_LD_A_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_A(get_L()); break;
        case IndexMode::IX: set_A(get_IXL()); break;
        case IndexMode::IY: set_A(get_IYL()); break;
    }
}

void Z80::opcode_0x7E_LD_A_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        set_A(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_A(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x7F_LD_A_A() {
    // Total Ticks: 4
}

void Z80::opcode_0x80_ADD_A_B() {
    // Total Ticks: 4
    add_8bit(get_B());
}

void Z80::opcode_0x81_ADD_A_C() {
    // Total Ticks: 4
    add_8bit(get_C());
}

void Z80::opcode_0x82_ADD_A_D() {
    // Total Ticks: 4
    add_8bit(get_D());
}

void Z80::opcode_0x83_ADD_A_E() {
    // Total Ticks: 4
    add_8bit(get_E());
}

void Z80::opcode_0x84_ADD_A_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: add_8bit(get_H()); break;
        case IndexMode::IX: add_8bit(get_IXH()); break;
        case IndexMode::IY: add_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x85_ADD_A_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: add_8bit(get_L()); break;
        case IndexMode::IX: add_8bit(get_IXL()); break;
        case IndexMode::IY: add_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x86_ADD_A_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        add_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        add_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x87_ADD_A_A() {
    // Total Ticks: 4
    add_8bit(get_A());
}

void Z80::opcode_0x88_ADC_A_B() {
    // Total Ticks: 4
    adc_8bit(get_B());
}

void Z80::opcode_0x89_ADC_A_C() {
    // Total Ticks: 4
    adc_8bit(get_C());
}

void Z80::opcode_0x8A_ADC_A_D() {
    // Total Ticks: 4
    adc_8bit(get_D());
}

void Z80::opcode_0x8B_ADC_A_E() {
    // Total Ticks: 4
    adc_8bit(get_E());
}

void Z80::opcode_0x8C_ADC_A_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: adc_8bit(get_H()); break;
        case IndexMode::IX: adc_8bit(get_IXH()); break;
        case IndexMode::IY: adc_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x8D_ADC_A_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: adc_8bit(get_L()); break;
        case IndexMode::IX: adc_8bit(get_IXL()); break;
        case IndexMode::IY: adc_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x8E_ADC_A_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        adc_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        adc_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x8F_ADC_A_A() {
    // Total Ticks: 4
    adc_8bit(get_A());
}

void Z80::opcode_0x90_SUB_B() {
    // Total Ticks: 4
    sub_8bit(get_B());
}

void Z80::opcode_0x91_SUB_C() {
    // Total Ticks: 4
    sub_8bit(get_C());
}

void Z80::opcode_0x92_SUB_D() {
    // Total Ticks: 4
    sub_8bit(get_D());
}

void Z80::opcode_0x93_SUB_E() {
    // Total Ticks: 4
    sub_8bit(get_E());
}

void Z80::opcode_0x94_SUB_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: sub_8bit(get_H()); break;
        case IndexMode::IX: sub_8bit(get_IXH()); break;
        case IndexMode::IY: sub_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x95_SUB_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: sub_8bit(get_L()); break;
        case IndexMode::IX: sub_8bit(get_IXL()); break;
        case IndexMode::IY: sub_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x96_SUB_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        sub_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        sub_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x97_SUB_A() {
    // Total Ticks: 4
    sub_8bit(get_A());
}

void Z80::opcode_0x98_SBC_A_B() {
    // Total Ticks: 4
    sbc_8bit(get_B());
}

void Z80::opcode_0x99_SBC_A_C() {
    // Total Ticks: 4
    sbc_8bit(get_C());
}

void Z80::opcode_0x9A_SBC_A_D() {
    // Total Ticks: 4
    sbc_8bit(get_D());
}

void Z80::opcode_0x9B_SBC_A_E() {
    // Total Ticks: 4
    sbc_8bit(get_E());
}

void Z80::opcode_0x9C_SBC_A_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: sbc_8bit(get_H()); break;
        case IndexMode::IX: sbc_8bit(get_IXH()); break;
        case IndexMode::IY: sbc_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x9D_SBC_A_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: sbc_8bit(get_L()); break;
        case IndexMode::IX: sbc_8bit(get_IXL()); break;
        case IndexMode::IY: sbc_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x9E_SBC_A_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        sbc_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        sbc_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0x9F_SBC_A_A() {
    // Total Ticks: 4
    sbc_8bit(get_A());
}

void Z80::opcode_0xA0_AND_B() {
    // Total Ticks: 4
    and_8bit(get_B());
}

void Z80::opcode_0xA1_AND_C() {
    // Total Ticks: 4
    and_8bit(get_C());
}

void Z80::opcode_0xA2_AND_D() {
    // Total Ticks: 4
    and_8bit(get_D());
}

void Z80::opcode_0xA3_AND_E() {
    // Total Ticks: 4
    and_8bit(get_E());
}

void Z80::opcode_0xA4_AND_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: and_8bit(get_H()); break;
        case IndexMode::IX: and_8bit(get_IXH()); break;
        case IndexMode::IY: and_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xA5_AND_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: and_8bit(get_L()); break;
        case IndexMode::IX: and_8bit(get_IXL()); break;
        case IndexMode::IY: and_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xA6_AND_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        and_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        and_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0xA7_AND_A() {
    // Total Ticks: 4
    and_8bit(get_A());
}

void Z80::opcode_0xA8_XOR_B() {
    // Total Ticks: 4
    xor_8bit(get_B());
}

void Z80::opcode_0xA9_XOR_C() {
    // Total Ticks: 4
    xor_8bit(get_C());
}

void Z80::opcode_0xAA_XOR_D() {
    // Total Ticks: 4
    xor_8bit(get_D());
}

void Z80::opcode_0xAB_XOR_E() {
    // Total Ticks: 4
    xor_8bit(get_E());
}

void Z80::opcode_0xAC_XOR_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: xor_8bit(get_H()); break;
        case IndexMode::IX: xor_8bit(get_IXH()); break;
        case IndexMode::IY: xor_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xAD_XOR_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: xor_8bit(get_L()); break;
        case IndexMode::IX: xor_8bit(get_IXL()); break;
        case IndexMode::IY: xor_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xAE_XOR_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        xor_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        xor_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0xAF_XOR_A() {
    // Total Ticks: 4
    xor_8bit(get_A());
}

void Z80::opcode_0xB0_OR_B() {
    // Total Ticks: 4
    or_8bit(get_B());
}

void Z80::opcode_0xB1_OR_C() {
    // Total Ticks: 4
    or_8bit(get_C());
}

void Z80::opcode_0xB2_OR_D() {
    // Total Ticks: 4
    or_8bit(get_D());
}

void Z80::opcode_0xB3_OR_E() {
    // Total Ticks: 4
    or_8bit(get_E());
}

void Z80::opcode_0xB4_OR_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: or_8bit(get_H()); break;
        case IndexMode::IX: or_8bit(get_IXH()); break;
        case IndexMode::IY: or_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xB5_OR_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: or_8bit(get_L()); break;
        case IndexMode::IX: or_8bit(get_IXL()); break;
        case IndexMode::IY: or_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xB6_OR_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        or_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        or_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0xB7_OR_A() {
    // Total Ticks: 4
    or_8bit(get_A());
}

void Z80::opcode_0xB8_CP_B() {
    // Total Ticks: 4
    cp_8bit(get_B());
}

void Z80::opcode_0xB9_CP_C() {
    // Total Ticks: 4
    cp_8bit(get_C());
}

void Z80::opcode_0xBA_CP_D() {
    // Total Ticks: 4
    cp_8bit(get_D());
}

void Z80::opcode_0xBB_CP_E() {
    // Total Ticks: 4
    cp_8bit(get_E());
}

void Z80::opcode_0xBC_CP_H() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: cp_8bit(get_H()); break;
        case IndexMode::IX: cp_8bit(get_IXH()); break;
        case IndexMode::IY: cp_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xBD_CP_L() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: cp_8bit(get_L()); break;
        case IndexMode::IX: cp_8bit(get_IXL()); break;
        case IndexMode::IY: cp_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xBE_CP_HL_ptr() {
    // Total Ticks HL: 7, IX/IY: 19
    if (get_index_mode() == IndexMode::HL) {
        cp_8bit(read_byte(get_HL()));
        add_ticks(3);
    } else {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        add_ticks(5);
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        cp_8bit(read_byte(address));
        add_ticks(3);
    }
}

void Z80::opcode_0xBF_CP_A() {
    // Total Ticks: 4
    cp_8bit(get_A());
}

void Z80::opcode_0xC0_RET_NZ() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (!is_Z_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xC1_POP_BC() {
    // Ticks: 10 = 4 (fetch) + 6 (pop)
    set_BC(pop_word());
    add_ticks(6);
}

void Z80::opcode_0xC2_JP_NZ_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (!is_Z_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xC3_JP_nn() {
    // Total Ticks: 10
    set_PC(fetch_next_word());
}

void Z80::opcode_0xC4_CALL_NZ_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (!is_Z_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xC5_PUSH_BC() {
    // Ticks: 11 = 4 (fetch) + 1 (wew.) + 6 (push)
    push_word(get_BC());
    add_ticks(7);
}

void Z80::opcode_0xC6_ADD_A_n() {
    // Total Ticks: 7
    add_8bit(fetch_next_byte());
}

void Z80::opcode_0xC7_RST_00H() {
    // Total Ticks: 11 = 4(fetch)+1(op)+6(push)
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0000);
}

void Z80::opcode_0xC8_RET_Z() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (is_Z_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xC9_RET() {
    // Total Ticks: 10
    set_PC(pop_word());
}

void Z80::opcode_0xCA_JP_Z_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (is_Z_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xCC_CALL_Z_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (is_Z_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xCD_CALL_nn() {
    // Total Ticks: 17 = 4(fetch)+6(fetch word)+1(op)+6(push)
    uint16_t address = fetch_next_word();
    add_ticks(1);
    push_word(get_PC());
    set_PC(address);
}

void Z80::opcode_0xCE_ADC_A_n() {
    // Total Ticks: 7
    adc_8bit(fetch_next_byte());
}

void Z80::opcode_0xCF_RST_08H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0008);
}

void Z80::opcode_0xD0_RET_NC() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (!is_C_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xD1_POP_DE() {
    // Ticks: 10 = 4 (fetch) + 6 (pop)
    set_DE(pop_word());
    add_ticks(6);
}

void Z80::opcode_0xD2_JP_NC_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (!is_C_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xD3_OUT_n_ptr_A() {
    // Total Ticks: 11 = 4(fetch)+3(fetch byte)+4(I/O)
    uint8_t port_lo = fetch_next_byte();
    write_byte_to_io((get_A() << 8) | port_lo, get_A());
    add_ticks(4);
}

void Z80::opcode_0xD4_CALL_NC_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (!is_C_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xD5_PUSH_DE() {
    // Ticks: 11 = 4 (fetch) + 1 (wew.) + 6 (push)
    push_word(get_DE());
    add_ticks(7);
}

void Z80::opcode_0xD6_SUB_n() {
    // Total Ticks: 7
    sub_8bit(fetch_next_byte());
}

void Z80::opcode_0xD7_RST_10H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0010);
}

void Z80::opcode_0xD8_RET_C() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (is_C_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xD9_EXX() {
    // Total Ticks: 4
    uint16_t temp_bc = get_BC();
    uint16_t temp_de = get_DE();
    uint16_t temp_hl = get_HL();
    set_BC(get_BCp());
    set_DE(get_DEp());
    set_HL(get_HLp());
    set_BCp(temp_bc);
    set_DEp(temp_de);
    set_HLp(temp_hl);
}

void Z80::opcode_0xDA_JP_C_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (is_C_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xDB_IN_A_n_ptr() {
    // Total Ticks: 11 = 4(fetch)+3(fetch byte)+4(I/O)
    uint8_t port_lo = fetch_next_byte();
    uint16_t port = (get_A() << 8) | port_lo;
    set_A(read_byte_from_io(port));
    add_ticks(4);
}

void Z80::opcode_0xDC_CALL_C_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (is_C_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xDE_SBC_A_n() {
    // Total Ticks: 7
    sbc_8bit(fetch_next_byte());
}

void Z80::opcode_0xDF_RST_18H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0018);
}

void Z80::opcode_0xE0_RET_PO() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (!is_PV_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xE1_POP_HL() {
    // Ticks HL: 10 = 4 (fetch) + 6 (op)
    // Ticks IX/IY: 14 = 4 (DD) + 4 (fetch) + 6 (op)
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(pop_word()); break;
        case IndexMode::IX: set_IX(pop_word()); break;
        case IndexMode::IY: set_IY(pop_word()); break;
    }
    add_ticks(6);
}

void Z80::opcode_0xE2_JP_PO_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (!is_PV_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xE3_EX_SP_ptr_HL() {
    // Total Ticks HL: 19, IX/IY: 23
    // HL: 4(fetch)+3(readL)+3(readH)+2(op)+3(writeL)+4(writeH) = 19
    // IX/IY: +4 for prefix
    uint16_t from_stack = read_word(get_SP()); add_ticks(6);
    add_ticks(2);
    switch(get_index_mode()) {
        case IndexMode::HL:
            write_word(get_SP(), get_HL()); add_ticks(7);
            set_HL(from_stack);
            break;
        case IndexMode::IX:
            write_word(get_SP(), get_IX()); add_ticks(7);
            set_IX(from_stack);
            break;
        case IndexMode::IY:
            write_word(get_SP(), get_IY()); add_ticks(7);
            set_IY(from_stack);
            break;
    }
}

void Z80::opcode_0xE4_CALL_PO_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (!is_PV_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xE5_PUSH_HL() {
    // Ticks HL: 11 = 4 (fetch) + 7 (op)
    // Ticks IX/IY: 15 = 4 (DD) + 4 (fetch) + 7 (op)
    switch (get_index_mode()) {
        case IndexMode::HL: push_word(get_HL()); break;
        case IndexMode::IX: push_word(get_IX()); break;
        case IndexMode::IY: push_word(get_IY()); break;
    }
    add_ticks(7); // 1 (wew.) + 6 (push)
}

void Z80::opcode_0xE6_AND_n() {
    // Total Ticks: 7
    and_8bit(fetch_next_byte());
}

void Z80::opcode_0xE7_RST_20H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0020);
}

void Z80::opcode_0xE8_RET_PE() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (is_PV_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xE9_JP_HL_ptr() {
    // Total Ticks HL: 4, IX/IY: 8
    switch (get_index_mode()) {
        case IndexMode::HL: set_PC(get_HL()); break;
        case IndexMode::IX: set_PC(get_IX()); break;
        case IndexMode::IY: set_PC(get_IY()); break;
    }
}

void Z80::opcode_0xEA_JP_PE_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (is_PV_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xEB_EX_DE_HL() {
    // Total Ticks: 4
    uint16_t temp = get_HL();
    set_HL(get_DE());
    set_DE(temp);
}

void Z80::opcode_0xEC_CALL_PE_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (is_PV_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xEE_XOR_n() {
    // Total Ticks: 7
    xor_8bit(fetch_next_byte());
}

void Z80::opcode_0xEF_RST_28H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0028);
}

void Z80::opcode_0xF0_RET_P() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (!is_S_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xF1_POP_AF() {
    // Ticks: 10 = 4 (fetch) + 6 (pop)
    set_AF(pop_word());
    add_ticks(6);
}

void Z80::opcode_0xF2_JP_P_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (!is_S_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xF3_DI() {
    // Total Ticks: 4
    set_IFF1(false);
    set_IFF2(false);
}

void Z80::opcode_0xF4_CALL_P_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (!is_S_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xF5_PUSH_AF() {
    // Ticks: 11 = 4 (fetch) + 1 (wew.) + 6 (push)
    push_word(get_AF());
    add_ticks(7);
}

void Z80::opcode_0xF6_OR_n() {
    // Total Ticks: 7
    or_8bit(fetch_next_byte());
}

void Z80::opcode_0xF7_RST_30H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0030);
}

void Z80::opcode_0xF8_RET_M() {
    // Ticks: 5 (warunek niespełniony) / 11 (spełniony)
    // Koszt bazowy: 4 (fetch)
    if (is_S_flag_set()) {
        set_PC(pop_word());
        add_ticks(7); // 1 (wew.) + 6 (pop)
    } else {
        add_ticks(1); // 1 (wew.)
    }
}

void Z80::opcode_0xF9_LD_SP_HL() {
    // Total Ticks HL: 6, IX/IY: 10
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_SP(get_HL()); break;
        case IndexMode::IX: set_SP(get_IX()); break;
        case IndexMode::IY: set_SP(get_IY()); break;
    }
}

void Z80::opcode_0xFA_JP_M_nn() {
    // Total Ticks: 10
    uint16_t address = fetch_next_word();
    if (is_S_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xFB_EI() {
    // Total Ticks: 4
    set_interrupt_enable_pending(true);
}

void Z80::opcode_0xFC_CALL_M_nn() {
    // Ticks: 10 (warunek niespełniony) / 17 (spełniony)
    // Koszt bazowy: 4 (fetch op) + 6 (fetch addr) = 10
    uint16_t address = fetch_next_word();
    if (is_S_flag_set()) {
        push_word(get_PC());
        set_PC(address);
        add_ticks(7); // 1 (wew.) + 6 (push)
    }
}

void Z80::opcode_0xFE_CP_n() {
    // Total Ticks: 7
    cp_8bit(fetch_next_byte());
}

void Z80::opcode_0xFF_RST_38H() {
    // Total Ticks: 11
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0038);
}

#include "Z80.h"

void Z80::opcode_0xED_0x40_IN_B_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_B(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x41_OUT_C_ptr_B() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_B());
    add_ticks(4);
}

void Z80::opcode_0xED_0x42_SBC_HL_BC() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0xED_0x43_LD_nn_ptr_BC() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(write L) + 3(write H)
    write_word(fetch_next_word(), get_BC());
    add_ticks(6);
}

void Z80::opcode_0xED_0x44_NEG() {
    // Total Ticks: 8 = 4 (fetch ED) + 4 (fetch op)
    uint8_t value = get_A();
    uint8_t result = -value;
    set_A(result);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (0x00 & 0x0F) < (value & 0x0F));
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, value != 0); 
    set_flag_if(FLAG_PV, value == 0x80); // Przepełnienie występuje tylko przy negacji -128
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x45_RETN() {
    // Total Ticks: 14 = 8 (fetch ED 45) + 6 (pop PC)
    set_PC(pop_word());
    add_ticks(6);
    set_IFF1(get_IFF2());
}

void Z80::opcode_0xED_0x46_IM_0() {
    // Total Ticks: 8 = 4(ED) + 4(op)
    set_interrupt_mode(0);
}

void Z80::opcode_0xED_0x47_LD_I_A() {
    // Total Ticks: 9 = 4(ED) + 4(op) + 1(internal op)
    // Flags are NOT affected by this instruction.
    set_I(get_A());
    add_ticks(1);
}

void Z80::opcode_0xED_0x48_IN_C_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_C(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x49_OUT_C_ptr_C() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_C());
    add_ticks(4);
}

void Z80::opcode_0xED_0x4A_ADC_HL_BC() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0xED_0x4B_LD_BC_nn_ptr() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(read L) + 3(read H)
    set_BC(read_word(fetch_next_word()));
    add_ticks(6);
}

void Z80::opcode_0xED_0x4D_RETI() {
    // Total Ticks: 14 = 8 (fetch ED 4D) + 6 (pop PC)
    set_PC(pop_word());
    add_ticks(6);
    set_IFF1(get_IFF2());
    set_reti_signaled(true);
}

void Z80::opcode_0xED_0x4F_LD_R_A() {
    // Total Ticks: 9 = 4(ED) + 4(op) + 1(internal op)
    // Flags are NOT affected by this instruction.
    set_R(get_A());
    add_ticks(1);
}

void Z80::opcode_0xED_0x50_IN_D_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_D(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x51_OUT_C_ptr_D() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_D());
    add_ticks(4);
}

void Z80::opcode_0xED_0x52_SBC_HL_DE() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_de = get_DE();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_de)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_de)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_de)); break;
    }
}

void Z80::opcode_0xED_0x53_LD_nn_ptr_DE() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(write L) + 3(write H)
    write_word(fetch_next_word(), get_DE());
    add_ticks(6);
}

void Z80::opcode_0xED_0x56_IM_1() {
    // Total Ticks: 8 = 4(ED) + 4(op)
    set_interrupt_mode(1);
}

void Z80::opcode_0xED_0x57_LD_A_I() {
    // Total Ticks: 9 = 4(ED) + 4(op) + 1(internal op)
    uint8_t i_value = get_I();
    set_A(i_value);
    add_ticks(1);
    set_flag_if(FLAG_S, (i_value & 0x80) != 0);
    set_flag_if(FLAG_Z, i_value == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_IFF2()); // P/V flag is set to the state of the IFF2 flip-flop
    set_flag_if(FLAG_Y, (i_value & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (i_value & FLAG_X) != 0);
}

void Z80::opcode_0xED_0x58_IN_E_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_E(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x59_OUT_C_ptr_E() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_E());
    add_ticks(4);
}

void Z80::opcode_0xED_0x5A_ADC_HL_DE() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_de = get_DE();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_de)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_de)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_de)); break;
    }
}

void Z80::opcode_0xED_0x5B_LD_DE_nn_ptr() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(read L) + 3(read H)
    set_DE(read_word(fetch_next_word()));
    add_ticks(6);
}

void Z80::opcode_0xED_0x5E_IM_2() {
    // Total Ticks: 8 = 4(ED) + 4(op)
    set_interrupt_mode(2);
}

void Z80::opcode_0xED_0x5F_LD_A_R() {
    // Total Ticks: 9 = 4(ED) + 4(op) + 1(internal op)
    uint8_t r_value = get_R();
    set_A(r_value);
    add_ticks(1);
    set_flag_if(FLAG_S, (r_value & 0x80) != 0);
    set_flag_if(FLAG_Z, r_value == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_IFF2()); // P/V flag is set to the state of the IFF2 flip-flop
    set_flag_if(FLAG_Y, (r_value & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (r_value & FLAG_X) != 0);
}

void Z80::opcode_0xED_0x60_IN_H_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_H(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x61_OUT_C_ptr_H() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_H());
    add_ticks(4);
}

void Z80::opcode_0xED_0x62_SBC_HL_HL() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), get_HL())); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), get_IX())); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), get_IY())); break;
    }
}

void Z80::opcode_0xED_0x63_LD_nn_ptr_HL_ED() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(write L) + 3(write H)
    write_word(fetch_next_word(), get_HL());
    add_ticks(6);
}

void Z80::opcode_0xED_0x67_RRD() {
    // Total Ticks: 18 = 4(ED) + 4(op) + 3(read) + 4(internal) + 3(write)
    uint16_t address = get_HL();
    uint8_t mem_val = read_byte(address);
    add_ticks(3); // Memory Read
    add_ticks(4); // Internal operation
    uint8_t a_val = get_A();
    uint8_t new_a = (a_val & 0xF0) | (mem_val & 0x0F);
    uint8_t new_mem = (mem_val >> 4) | ((a_val & 0x0F) << 4);
    set_A(new_a);
    write_byte(address, new_mem);
    add_ticks(3); // Memory Write
    set_flag_if(FLAG_S, (new_a & 0x80) != 0);
    set_flag_if(FLAG_Z, new_a == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, is_parity_even(new_a));
    set_flag_if(FLAG_X, (new_a & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (new_a & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x68_IN_L_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_L(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x69_OUT_C_ptr_L() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_L());
    add_ticks(4);
}


void Z80::opcode_0xED_0x6A_ADC_HL_HL() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), get_HL())); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), get_IX())); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), get_IY())); break;
    }
}

void Z80::opcode_0xED_0x6B_LD_HL_nn_ptr_ED() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(read L) + 3(read H)
    set_HL(read_word(fetch_next_word()));
    add_ticks(6);
}

void Z80::opcode_0xED_0x6F_RLD() {
    // Total Ticks: 18 = 4(ED) + 4(op) + 3(read) + 4(internal) + 3(write)
    uint16_t address = get_HL();
    uint8_t mem_val = read_byte(address);
    add_ticks(3); // Memory Read
    add_ticks(4); // Internal operation
    uint8_t a_val = get_A();
    uint8_t new_a = (a_val & 0xF0) | (mem_val >> 4);
    uint8_t new_mem = (mem_val << 4) | (a_val & 0x0F);
    set_A(new_a);
    write_byte(address, new_mem);
    add_ticks(3);
    set_flag_if(FLAG_S, (new_a & 0x80) != 0);
    set_flag_if(FLAG_Z, new_a == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, is_parity_even(new_a));
    set_flag_if(FLAG_X, (new_a & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (new_a & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x70_IN_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    in_r_c();
    add_ticks(4);
}

void Z80::opcode_0xED_0x71_OUT_C_ptr_0() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(0x00);
    add_ticks(4);
}
   
void Z80::opcode_0xED_0x72_SBC_HL_SP() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_sp = get_SP();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_sp)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_sp)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_sp)); break;
    }
}

void Z80::opcode_0xED_0x73_LD_nn_ptr_SP() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(write L) + 3(write H)
    write_word(fetch_next_word(), get_SP());
    add_ticks(6);
}

void Z80::opcode_0xED_0x78_IN_A_C_ptr() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O read)
    set_A(in_r_c());
    add_ticks(4);
}

void Z80::opcode_0xED_0x79_OUT_C_ptr_A() {
    // Total Ticks: 12 = 4(ED) + 4(op) + 4(I/O write)
    out_c_r(get_A());
    add_ticks(4);
}

void Z80::opcode_0xED_0x7A_ADC_HL_SP() {
    // Total Ticks HL: 15, IX/IY: 19
    add_ticks(7);
    uint16_t val_sp = get_SP();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_sp)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_sp)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_sp)); break;
    }
}

void Z80::opcode_0xED_0x7B_LD_SP_nn_ptr() {
    // Total Ticks: 20 = 4(ED) + 4(op) + 6(fetch addr) + 3(read L) + 3(read H)
    set_SP(read_word(fetch_next_word()));
    add_ticks(6);
}



void Z80::opcode_0xED_0xA0_LDI() {
    // Total Ticks: 16 = 8(fetch) + 3(read) + 3(write) + 2(internal)
    uint8_t value = read_byte(get_HL());
    add_ticks(3);
    write_byte(get_DE(), value);
    add_ticks(3);
    add_ticks(2);
    set_HL(get_HL() + 1);
    set_DE(get_DE() + 1);
    set_BC(get_BC() - 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_BC() != 0);
    // Undocumented flags (quirky Z80 behavior)
    uint8_t temp = get_A() + value;
    set_flag_if(FLAG_Y, (temp & 0x02) != 0); // Flag Y is set from bit 1 of (A + value)
    set_flag_if(FLAG_X, (temp & 0x08) != 0); // Flag X is set from bit 3 of (A + value)
}

void Z80::opcode_0xED_0xA1_CPI() {
    // Total Ticks: 16 = 8(fetch) + 3(read) + 5(internal)
    uint8_t value = read_byte(get_HL());
    add_ticks(3);
    add_ticks(5);
    uint8_t result = get_A() - value;
    bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
    set_HL(get_HL() + 1);
    set_BC(get_BC() - 1);
    set_flag_if(FLAG_S, result & 0x80);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, half_carry);
    set_flag_if(FLAG_PV, get_BC() != 0);
    set_flag(FLAG_N);
    // Undocumented flags (quirky Z80 behavior)
    uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
    set_flag_if(FLAG_Y, (temp & 0x02) != 0); // Flag Y is set from bit 1 of (A - value - H)
    set_flag_if(FLAG_X, (temp & 0x08) != 0); // Flag X is set from bit 3 of (A - value - H)
}


void Z80::opcode_0xED_0xA2_INI() {
    // Total Ticks: 16 = 8(fetch) + 4(I/O read) + 1(internal) + 3(write)
    add_ticks(1);
    uint8_t port_val = read_byte_from_io(get_BC());
    add_ticks(4);
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte(get_HL(), port_val);
    add_ticks(3);
    set_HL(get_HL() + 1);

    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);
    
    uint16_t temp = static_cast<uint16_t>(get_C()) + 1;
    uint8_t k = port_val + (temp & 0xFF);
    set_flag_if(FLAG_C, k > 0xFF);
    set_flag_if(FLAG_H, k > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ (b_val - 1)) & 0xFF) );
}

void Z80::opcode_0xED_0xA3_OUTI() {
    // Total Ticks: 16 = 8(fetch) + 1(internal) + 3(read) + 4(I/O write)
    add_ticks(1);
    uint8_t mem_val = read_byte(get_HL());
    add_ticks(3);
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte_to_io(get_BC(), mem_val);
    add_ticks(4);
    set_HL(get_HL() + 1);

    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);
    
    uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
    set_flag_if(FLAG_C, temp > 0xFF);
    set_flag_if(FLAG_H, temp > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
}


void Z80::opcode_0xED_0xA8_LDD() {
    // Total Ticks: 16 = 8(fetch) + 3(read) + 3(write) + 2(internal)
    uint8_t value = read_byte(get_HL());
    add_ticks(3);
    write_byte(get_DE(), value);
    add_ticks(3);
    add_ticks(2);
    set_HL(get_HL() - 1);
    set_DE(get_DE() - 1);
    set_BC(get_BC() - 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_BC() != 0);
    // Undocumented flags (quirky Z80 behavior)
    uint8_t temp = get_A() + value;
    set_flag_if(FLAG_Y, (temp & 0x02) != 0); // Flag Y is set from bit 1 of (A + value)
    set_flag_if(FLAG_X, (temp & 0x08) != 0); // Flag X is set from bit 3 of (A + value)
}

void Z80::opcode_0xED_0xA9_CPD() {
    // Total Ticks: 16 = 8(fetch) + 3(read) + 5(internal)
    uint8_t value = read_byte(get_HL());
    add_ticks(3);
    add_ticks(5);
    uint8_t result = get_A() - value;
    bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
    set_HL(get_HL() - 1);
    set_BC(get_BC() - 1);
    set_flag_if(FLAG_S, result & 0x80);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, half_carry);
    set_flag_if(FLAG_PV, get_BC() != 0);
    set_flag(FLAG_N);
    // Undocumented flags (quirky Z80 behavior)
    uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
    set_flag_if(FLAG_Y, (temp & 0x02) != 0); // Flag Y is set from bit 1 of (A - value - H)
    set_flag_if(FLAG_X, (temp & 0x08) != 0); // Flag X is set from bit 3 of (A - value - H)
}

void Z80::opcode_0xED_0xAA_IND() {
    // Total Ticks: 16 = 8(fetch) + 4(I/O read) + 1(internal) + 3(write)
    add_ticks(1);
    uint8_t port_val = read_byte_from_io(get_BC());
    add_ticks(4);
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte(get_HL(), port_val);
    add_ticks(3);
    set_HL(get_HL() - 1);

    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);

    uint16_t temp = static_cast<uint16_t>(get_C()) - 1;
    uint8_t k = port_val + (temp & 0xFF);
    set_flag_if(FLAG_C, k > 0xFF);
    set_flag_if(FLAG_H, k > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ (b_val - 1)) & 0xFF) );
}

void Z80::opcode_0xED_0xAB_OUTD() {
    // Total Ticks: 16 = 8(fetch) + 1(internal) + 3(read) + 4(I/O write)
    add_ticks(1);
    uint8_t mem_val = read_byte(get_HL());
    add_ticks(3);
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte_to_io(get_BC(), mem_val);
    add_ticks(4);
    set_HL(get_HL() - 1);

    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);

    uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
    set_flag_if(FLAG_C, temp > 0xFF);
    set_flag_if(FLAG_H, temp > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
}

void Z80::opcode_0xED_0xB0_LDIR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xA0_LDI();
    if (get_BC() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB1_CPIR() {
    // Total Ticks: 16 (last/match) or 21 (repeat)
    opcode_0xED_0xA1_CPI();
    if (get_BC() != 0 && !is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB2_INIR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xA2_INI();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB3_OTIR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xA3_OUTI();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB8_LDDR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xA8_LDD();
    if (get_BC() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB9_CPDR() {
    // Total Ticks: 16 (last/match) or 21 (repeat)
    opcode_0xED_0xA9_CPD();
    if (get_BC() != 0 && !is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xBA_INDR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xAA_IND();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xBB_OTDR() {
    // Total Ticks: 16 (last) or 21 (repeat)
    opcode_0xED_0xAB_OUTD();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}
