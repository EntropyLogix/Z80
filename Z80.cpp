#include "Z80.h"
#include <cstdint>

void Z80::request_interrupt(uint8_t data) {
    set_interrupt_pending(true);
    set_interrupt_data(data);
}

void Z80::handle_nmi() {
    add_ticks(5);
    set_halted(false);
    set_IFF2(get_IFF1());
    set_IFF1(false);
    push_word(get_PC());
    set_PC(0x0066);
    set_nmi_pending(false);
}

void Z80::handle_interrupt() {
    add_ticks(7);
    set_IFF2(get_IFF1());
    set_IFF1(false);
    push_word(get_PC());
    switch (get_interrupt_mode()) {
        case 0: {
            uint8_t opcode = get_interrupt_data();
            switch (opcode) {
                case 0xC7: set_PC(0x0000); break;
                case 0xCF: set_PC(0x0008); break;
                case 0xD7: set_PC(0x0010); break;
                case 0xDF: set_PC(0x0018); break;
                case 0xE7: set_PC(0x0020); break;
                case 0xEF: set_PC(0x0028); break;
                case 0xF7: set_PC(0x0030); break;
                case 0xFF: set_PC(0x0038); break;
            }
            break;
        }
        case 1: {
            set_PC(0x0038);
            break;
        }
        case 2: {
            uint16_t vector_address = (static_cast<uint16_t>(get_I()) << 8) | get_interrupt_data();
            uint16_t handler_address = read_word(vector_address);
            set_PC(handler_address);
            break;
        }
    }
}

Z80::State Z80::save_state() const {
    Z80::State state;
    state.AF = get_AF();
    state.BC = get_BC();
    state.DE = get_DE();
    state.HL = get_HL();
    state.IX = get_IX();
    state.IY = get_IY();
    state.SP = get_SP();
    state.PC = get_PC();
    state.AFp = get_AFp();
    state.BCp = get_BCp();
    state.DEp = get_DEp();
    state.HLp = get_HLp();
    state.I = get_I();
    state.R = get_R();
    state.IFF1 = get_IFF1();
    state.IFF2 = get_IFF2();
    state.halted = is_halted();
    state.nmi_pending = is_nmi_pending();
    state.interrupt_pending = is_interrupt_pending();
    state.interrupt_enable_pending = is_interrupt_enable_pending();
    state.interrupt_data = get_interrupt_data();
    state.interrupt_mode = get_interrupt_mode();
    state.index_mode = get_index_mode();
    state.ticks = get_ticks();
    return state;
}

void Z80::load_state(const Z80::State& state) {
    set_AF(state.AF);
    set_BC(state.BC);
    set_DE(state.DE);
    set_HL(state.HL);
    set_IX(state.IX);
    set_IY(state.IY);
    set_SP(state.SP);
    set_PC(state.PC);
    set_AFp(state.AFp);
    set_BCp(state.BCp);
    set_DEp(state.DEp);
    set_HLp(state.HLp);
    set_I(state.I);
    set_R(state.R);
    set_IFF1(state.IFF1);
    set_IFF2(state.IFF2);
    set_halted(state.halted);
    set_nmi_pending(state.nmi_pending);
    set_interrupt_pending(state.interrupt_pending);
    set_interrupt_enable_pending(state.interrupt_enable_pending);
    set_interrupt_data(state.interrupt_data);
    set_interrupt_mode(state.interrupt_mode);
    set_index_mode(state.index_mode);
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
    set_flag_if(FLAG_Y, (result & 0x2000) != 0);
    set_flag_if(FLAG_X, (result & 0x0800) != 0);
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
        set_flag_if(FLAG_S, (value & 0x80) != 0);
    } else
        clear_flag(FLAG_S);
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
                set_flag_if(FLAG_X, (flags_source & 0x0800) != 0);
                set_flag_if(FLAG_Y, (flags_source & 0x2000) != 0);
            } else {
                set_flag_if(FLAG_X, (value & 0x08) != 0);
                set_flag_if(FLAG_Y, (value & 0x20) != 0);
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
            add_ticks(1);
            write_byte(get_HL(), result);
            break;
        case 7: set_A(result); break;
    }
}

void Z80::handle_CB_indexed(uint16_t index_register) {
    add_ticks(2);
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    uint8_t opcode = fetch_next_byte();
    uint16_t address = index_register + offset;
    uint8_t value = read_byte(address);
    uint8_t operation_group = opcode >> 6;
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t result = value;
    if (operation_group == 1) {
        add_ticks(1);
        bit_8bit(bit, value);
        set_flag_if(FLAG_X, (address & 0x0800) != 0);
        set_flag_if(FLAG_Y, (address & 0x2000) != 0);
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
    add_ticks(1);
    write_byte(address, result);
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
}

void Z80::opcode_0x01_LD_BC_nn() {
    set_BC(fetch_next_word());
}

void Z80::opcode_0x02_LD_BC_ptr_A() {
    write_byte(get_BC(), get_A());
}

void Z80::opcode_0x03_INC_BC() {
    add_ticks(2);
    set_BC(get_BC() + 1);
}

void Z80::opcode_0x04_INC_B() {
    set_B(inc_8bit(get_B()));
}

void Z80::opcode_0x05_DEC_B() {
    set_B(dec_8bit(get_B()));
}

void Z80::opcode_0x06_LD_B_n() {
    set_B(fetch_next_byte());
}

void Z80::opcode_0x07_RLCA() {
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
    uint16_t temp_AF = get_AF();
    set_AF(get_AFp());
    set_AFp(temp_AF);
}

void Z80::opcode_0x09_ADD_HL_BC() {
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(add_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(add_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(add_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0x0A_LD_A_BC_ptr() {
    set_A(read_byte(get_BC()));
}

void Z80::opcode_0x0B_DEC_BC() {
    add_ticks(2);
    set_BC(get_BC() - 1);
}

void Z80::opcode_0x0C_INC_C() {
    set_C(inc_8bit(get_C()));
}

void Z80::opcode_0x0D_DEC_C() {
    set_C(dec_8bit(get_C()));
}

void Z80::opcode_0x0E_LD_C_n() {
    set_C(fetch_next_byte());
}

void Z80::opcode_0x0F_RRCA() {
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
    set_DE(fetch_next_word());
}

void Z80::opcode_0x12_LD_DE_ptr_A() {
    write_byte(get_DE(), get_A());
}

void Z80::opcode_0x13_INC_DE() {
    add_ticks(2);
    set_DE(get_DE() + 1);
}

void Z80::opcode_0x14_INC_D() {
    set_D(inc_8bit(get_D()));
}

void Z80::opcode_0x15_DEC_D() {
    set_D(dec_8bit(get_D()));
}

void Z80::opcode_0x16_LD_D_n() {
    set_D(fetch_next_byte());
}

void Z80::opcode_0x17_RLA() {
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
    add_ticks(5);
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    set_PC(get_PC() + offset);
}

void Z80::opcode_0x19_ADD_HL_DE() {
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
    set_A(read_byte(get_DE()));
}

void Z80::opcode_0x1B_DEC_DE() {
    add_ticks(2);
    set_DE(get_DE() - 1);
}

void Z80::opcode_0x1C_INC_E() {
    set_E(inc_8bit(get_E()));
}

void Z80::opcode_0x1D_DEC_E() {
    set_E(dec_8bit(get_E()));
}

void Z80::opcode_0x1E_LD_E_n() {
    set_E(fetch_next_byte());
}

void Z80::opcode_0x1F_RRA() {
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
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (!is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x21_LD_HL_nn() {
    uint16_t value = fetch_next_word();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(value); break;
        case IndexMode::IX: set_IX(value); break;
        case IndexMode::IY: set_IY(value); break;
    }
}

void Z80::opcode_0x22_LD_nn_ptr_HL() {
    uint16_t address = fetch_next_word();
    switch (get_index_mode()) {
        case IndexMode::HL: write_word(address, get_HL()); break;
        case IndexMode::IX: write_word(address, get_IX()); break;
        case IndexMode::IY: write_word(address, get_IY()); break;
    }
}

void Z80::opcode_0x23_INC_HL() {
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(get_HL() + 1); break;
        case IndexMode::IX: set_IX(get_IX() + 1); break;
        case IndexMode::IY: set_IY(get_IY() + 1); break;
    }
}

void Z80::opcode_0x24_INC_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(inc_8bit(get_H())); break;
        case IndexMode::IX: set_IXH(inc_8bit(get_IXH())); break;
        case IndexMode::IY: set_IYH(inc_8bit(get_IYH())); break;
    }
}

void Z80::opcode_0x25_DEC_H() {
     switch (get_index_mode()) {
        case IndexMode::HL: set_H(dec_8bit(get_H())); break;
        case IndexMode::IX: set_IXH(dec_8bit(get_IXH())); break;
        case IndexMode::IY: set_IYH(dec_8bit(get_IYH())); break;
    }
}

void Z80::opcode_0x26_LD_H_n() {
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
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x29_ADD_HL_HL() {
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
    uint16_t address = fetch_next_word();
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
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(get_HL() - 1); break;
        case IndexMode::IX: set_IX(get_IX() - 1); break;
        case IndexMode::IY: set_IY(get_IY() - 1); break;
    }
}

void Z80::opcode_0x2C_INC_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(inc_8bit(get_L())); break;
        case IndexMode::IX: set_IXL(inc_8bit(get_IXL())); break;
        case IndexMode::IY: set_IYL(inc_8bit(get_IYL())); break;
    }
}

void Z80::opcode_0x2D_DEC_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(dec_8bit(get_L())); break;
        case IndexMode::IX: set_IXL(dec_8bit(get_IXL())); break;
        case IndexMode::IY: set_IYL(dec_8bit(get_IYL())); break;
    }
}

void Z80::opcode_0x2E_LD_L_n() {
    uint8_t value = fetch_next_byte();
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(value); break;
        case IndexMode::IX: set_IXL(value); break;
        case IndexMode::IY: set_IYL(value); break;
    }
}

void Z80::opcode_0x2F_CPL() {
    uint8_t a = get_A();
    a = ~a;
    set_A(a);
    set_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_Y, (a & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (a & FLAG_X) != 0);
}

void Z80::opcode_0x30_JR_NC_d() {
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (!is_C_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x31_LD_SP_nn() {
    set_SP(fetch_next_word());
}

void Z80::opcode_0x32_LD_nn_ptr_A() {
    uint16_t address = fetch_next_word();
    write_byte(address, get_A());
}

void Z80::opcode_0x33_INC_SP() {
    add_ticks(2);
    set_SP(get_SP() + 1);
}

void Z80::opcode_0x34_INC_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        add_ticks(1);
        uint16_t address = get_HL();
        uint8_t value = read_byte(address);
        write_byte(address, inc_8bit(value));
    } else {
        add_ticks(6);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        uint8_t value = read_byte(address);
        write_byte(address, inc_8bit(value));
    }
}

void Z80::opcode_0x35_DEC_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        add_ticks(1);
        uint16_t address = get_HL();
        uint8_t value = read_byte(address);
        write_byte(address, dec_8bit(value));
    } else {
        add_ticks(6);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        uint8_t value = read_byte(address);
        write_byte(address, dec_8bit(value));
    }
}

void Z80::opcode_0x36_LD_HL_ptr_n() {
    if (get_index_mode() == IndexMode::HL) {
        uint8_t value = fetch_next_byte();
        write_byte(get_HL(), value);
    } else {
        add_ticks(2);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint8_t value = fetch_next_byte();
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, value);
    }
}

void Z80::opcode_0x37_SCF() {
    set_flag(FLAG_C);
    clear_flag(FLAG_N | FLAG_H);
    set_flag_if(FLAG_X, (get_A() & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (get_A() & FLAG_Y) != 0);
}

void Z80::opcode_0x38_JR_C_d() {
    int8_t offset = static_cast<int8_t>(fetch_next_byte());
    if (is_C_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() + offset);
    }
}

void Z80::opcode_0x39_ADD_HL_SP() {
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
    uint16_t address = fetch_next_word();
    set_A(read_byte(address));
}

void Z80::opcode_0x3B_DEC_SP() {
    add_ticks(2);
    set_SP(get_SP() - 1);
}

void Z80::opcode_0x3C_INC_A() {
    set_A(inc_8bit(get_A()));
}

void Z80::opcode_0x3D_DEC_A() {
    set_A(dec_8bit(get_A()));
}

void Z80::opcode_0x3E_LD_A_n() {
    set_A(fetch_next_byte());
}

void Z80::opcode_0x3F_CCF() {
    bool old_C_flag = is_C_flag_set();
    set_flag_if(FLAG_C, !old_C_flag);
    clear_flag(FLAG_N);
    set_flag_if(FLAG_H, old_C_flag);
    set_flag_if(FLAG_Y, (get_A() & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (get_A() & FLAG_X) != 0);
}

void Z80::opcode_0x40_LD_B_B() {
}

void Z80::opcode_0x41_LD_B_C() {
    set_B(get_C());
}

void Z80::opcode_0x42_LD_B_D() {
    set_B(get_D());
}

void Z80::opcode_0x43_LD_B_E() {
    set_B(get_E());
}

void Z80::opcode_0x44_LD_B_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_B(get_H()); break;
        case IndexMode::IX: set_B(get_IXH()); break;
        case IndexMode::IY: set_B(get_IYH()); break;
    }
}

void Z80::opcode_0x45_LD_B_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_B(get_L()); break;
        case IndexMode::IX: set_B(get_IXL()); break;
        case IndexMode::IY: set_B(get_IYL()); break;
    }
}

void Z80::opcode_0x46_LD_B_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_B(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_B(read_byte(address));
    }
}

void Z80::opcode_0x47_LD_B_A() {
    set_B(get_A());
}

void Z80::opcode_0x48_LD_C_B() {
    set_C(get_B());
}

void Z80::opcode_0x49_LD_C_C() {
}

void Z80::opcode_0x4A_LD_C_D() {
    set_C(get_D());
}

void Z80::opcode_0x4B_LD_C_E() {
    set_C(get_E());
}

void Z80::opcode_0x4C_LD_C_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_C(get_H()); break;
        case IndexMode::IX: set_C(get_IXH()); break;
        case IndexMode::IY: set_C(get_IYH()); break;
    }
}

void Z80::opcode_0x4D_LD_C_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_C(get_L()); break;
        case IndexMode::IX: set_C(get_IXL()); break;
        case IndexMode::IY: set_C(get_IYL()); break;
    }
}

void Z80::opcode_0x4E_LD_C_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_C(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_C(read_byte(address));
    }
}

void Z80::opcode_0x4F_LD_C_A() {
    set_C(get_A());
}

void Z80::opcode_0x50_LD_D_B() {
    set_D(get_B());
}

void Z80::opcode_0x51_LD_D_C() {
    set_D(get_C());
}

void Z80::opcode_0x52_LD_D_D() {
}

void Z80::opcode_0x53_LD_D_E() {
    set_D(get_E());
}

void Z80::opcode_0x54_LD_D_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_D(get_H()); break;
        case IndexMode::IX: set_D(get_IXH()); break;
        case IndexMode::IY: set_D(get_IYH()); break;
    }
}

void Z80::opcode_0x55_LD_D_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_D(get_L()); break;
        case IndexMode::IX: set_D(get_IXL()); break;
        case IndexMode::IY: set_D(get_IYL()); break;
    }
}

void Z80::opcode_0x56_LD_D_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_D(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_D(read_byte(address));
    }
}

void Z80::opcode_0x57_LD_D_A() {
    set_D(get_A());
}

void Z80::opcode_0x58_LD_E_B() {
    set_E(get_B());
}

void Z80::opcode_0x59_LD_E_C() {
    set_E(get_C());
}

void Z80::opcode_0x5A_LD_E_D() {
    set_E(get_D());
}

void Z80::opcode_0x5B_LD_E_E() {
}

void Z80::opcode_0x5C_LD_E_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_E(get_H()); break;
        case IndexMode::IX: set_E(get_IXH()); break;
        case IndexMode::IY: set_E(get_IYH()); break;
    }
}

void Z80::opcode_0x5D_LD_E_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_E(get_L()); break;
        case IndexMode::IX: set_E(get_IXL()); break;
        case IndexMode::IY: set_E(get_IYL()); break;
    }
}

void Z80::opcode_0x5E_LD_E_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_E(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_E(read_byte(address));
    }
}

void Z80::opcode_0x5F_LD_E_A() {
    set_E(get_A());
}

void Z80::opcode_0x60_LD_H_B() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_B()); break;
        case IndexMode::IX: set_IXH(get_B()); break;
        case IndexMode::IY: set_IYH(get_B()); break;
    }
}

void Z80::opcode_0x61_LD_H_C() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_C()); break;
        case IndexMode::IX: set_IXH(get_C()); break;
        case IndexMode::IY: set_IYH(get_C()); break;
    }
}

void Z80::opcode_0x62_LD_H_D() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_D()); break;
        case IndexMode::IX: set_IXH(get_D()); break;
        case IndexMode::IY: set_IYH(get_D()); break;
    }
}

void Z80::opcode_0x63_LD_H_E() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_E()); break;
        case IndexMode::IX: set_IXH(get_E()); break;
        case IndexMode::IY: set_IYH(get_E()); break;
    }
}

void Z80::opcode_0x64_LD_H_H() {
}

void Z80::opcode_0x65_LD_H_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_L()); break;
        case IndexMode::IX: set_IXH(get_IXL()); break;
        case IndexMode::IY: set_IYH(get_IYL()); break;
    }
}

void Z80::opcode_0x66_LD_H_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_H(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_H(read_byte(address));
    }
}

void Z80::opcode_0x67_LD_H_A() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_H(get_A()); break;
        case IndexMode::IX: set_IXH(get_A()); break;
        case IndexMode::IY: set_IYH(get_A()); break;
    }
}

void Z80::opcode_0x68_LD_L_B() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_B()); break;
        case IndexMode::IX: set_IXL(get_B()); break;
        case IndexMode::IY: set_IYL(get_B()); break;
    }
}

void Z80::opcode_0x69_LD_L_C() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_C()); break;
        case IndexMode::IX: set_IXL(get_C()); break;
        case IndexMode::IY: set_IYL(get_C()); break;
    }
}

void Z80::opcode_0x6A_LD_L_D() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_D()); break;
        case IndexMode::IX: set_IXL(get_D()); break;
        case IndexMode::IY: set_IYL(get_D()); break;
    }
}

void Z80::opcode_0x6B_LD_L_E() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_E()); break;
        case IndexMode::IX: set_IXL(get_E()); break;
        case IndexMode::IY: set_IYL(get_E()); break;
    }
}

void Z80::opcode_0x6C_LD_L_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_H()); break;
        case IndexMode::IX: set_IXL(get_IXH()); break;
        case IndexMode::IY: set_IYL(get_IYH()); break;
    }
}

void Z80::opcode_0x6D_LD_L_L() {
}

void Z80::opcode_0x6E_LD_L_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_L(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_L(read_byte(address));
    }
}

void Z80::opcode_0x6F_LD_L_A() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_L(get_A()); break;
        case IndexMode::IX: set_IXL(get_A()); break;
        case IndexMode::IY: set_IYL(get_A()); break;
    }
}

void Z80::opcode_0x70_LD_HL_ptr_B() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_B());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_B());
    }
}

void Z80::opcode_0x71_LD_HL_ptr_C() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_C());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_C());
    }
}

void Z80::opcode_0x72_LD_HL_ptr_D() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_D());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_D());
    }
}

void Z80::opcode_0x73_LD_HL_ptr_E() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_E());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_E());
    }
}

void Z80::opcode_0x74_LD_HL_ptr_H() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_H());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_H());
    }
}

void Z80::opcode_0x75_LD_HL_ptr_L() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_L());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_L());
    }
}

void Z80::opcode_0x76_HALT() {
    set_halted(true);
}

void Z80::opcode_0x77_LD_HL_ptr_A() {
    if (get_index_mode() == IndexMode::HL) {
        write_byte(get_HL(), get_A());
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        write_byte(address, get_A());
    }
}

void Z80::opcode_0x78_LD_A_B() {
    set_A(get_B());
}

void Z80::opcode_0x79_LD_A_C() {
    set_A(get_C());
}

void Z80::opcode_0x7A_LD_A_D() {
    set_A(get_D());
}

void Z80::opcode_0x7B_LD_A_E() {
    set_A(get_E());
}

void Z80::opcode_0x7C_LD_A_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_A(get_H()); break;
        case IndexMode::IX: set_A(get_IXH()); break;
        case IndexMode::IY: set_A(get_IYH()); break;
    }
}

void Z80::opcode_0x7D_LD_A_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_A(get_L()); break;
        case IndexMode::IX: set_A(get_IXL()); break;
        case IndexMode::IY: set_A(get_IYL()); break;
    }
}

void Z80::opcode_0x7E_LD_A_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        set_A(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        set_A(read_byte(address));
    }
}

void Z80::opcode_0x7F_LD_A_A() {
}

void Z80::opcode_0x80_ADD_A_B() {
    add_8bit(get_B());
}

void Z80::opcode_0x81_ADD_A_C() {
    add_8bit(get_C());
}

void Z80::opcode_0x82_ADD_A_D() {
    add_8bit(get_D());
}

void Z80::opcode_0x83_ADD_A_E() {
    add_8bit(get_E());
}

void Z80::opcode_0x84_ADD_A_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: add_8bit(get_H()); break;
        case IndexMode::IX: add_8bit(get_IXH()); break;
        case IndexMode::IY: add_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x85_ADD_A_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: add_8bit(get_L()); break;
        case IndexMode::IX: add_8bit(get_IXL()); break;
        case IndexMode::IY: add_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x86_ADD_A_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        add_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        add_8bit(read_byte(address));
    }
}

void Z80::opcode_0x87_ADD_A_A() {
    add_8bit(get_A());
}

void Z80::opcode_0x88_ADC_A_B() {
    adc_8bit(get_B());
}

void Z80::opcode_0x89_ADC_A_C() {
    adc_8bit(get_C());
}

void Z80::opcode_0x8A_ADC_A_D() {
    adc_8bit(get_D());
}

void Z80::opcode_0x8B_ADC_A_E() {
    adc_8bit(get_E());
}

void Z80::opcode_0x8C_ADC_A_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: adc_8bit(get_H()); break;
        case IndexMode::IX: adc_8bit(get_IXH()); break;
        case IndexMode::IY: adc_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x8D_ADC_A_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: adc_8bit(get_L()); break;
        case IndexMode::IX: adc_8bit(get_IXL()); break;
        case IndexMode::IY: adc_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x8E_ADC_A_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        adc_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        adc_8bit(read_byte(address));
    }
}

void Z80::opcode_0x8F_ADC_A_A() {
    adc_8bit(get_A());
}

void Z80::opcode_0x90_SUB_B() {
    sub_8bit(get_B());
}

void Z80::opcode_0x91_SUB_C() {
    sub_8bit(get_C());
}

void Z80::opcode_0x92_SUB_D() {
    sub_8bit(get_D());
}

void Z80::opcode_0x93_SUB_E() {
    sub_8bit(get_E());
}

void Z80::opcode_0x94_SUB_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: sub_8bit(get_H()); break;
        case IndexMode::IX: sub_8bit(get_IXH()); break;
        case IndexMode::IY: sub_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x95_SUB_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: sub_8bit(get_L()); break;
        case IndexMode::IX: sub_8bit(get_IXL()); break;
        case IndexMode::IY: sub_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x96_SUB_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        sub_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        sub_8bit(read_byte(address));
    }
}

void Z80::opcode_0x97_SUB_A() {
    sub_8bit(get_A());
}

void Z80::opcode_0x98_SBC_A_B() {
    sbc_8bit(get_B());
}

void Z80::opcode_0x99_SBC_A_C() {
    sbc_8bit(get_C());
}

void Z80::opcode_0x9A_SBC_A_D() {
    sbc_8bit(get_D());
}

void Z80::opcode_0x9B_SBC_A_E() {
    sbc_8bit(get_E());
}

void Z80::opcode_0x9C_SBC_A_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: sbc_8bit(get_H()); break;
        case IndexMode::IX: sbc_8bit(get_IXH()); break;
        case IndexMode::IY: sbc_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0x9D_SBC_A_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: sbc_8bit(get_L()); break;
        case IndexMode::IX: sbc_8bit(get_IXL()); break;
        case IndexMode::IY: sbc_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0x9E_SBC_A_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        sbc_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        sbc_8bit(read_byte(address));
    }
}

void Z80::opcode_0x9F_SBC_A_A() {
    sbc_8bit(get_A());
}

void Z80::opcode_0xA0_AND_B() {
    and_8bit(get_B());
}

void Z80::opcode_0xA1_AND_C() {
    and_8bit(get_C());
}

void Z80::opcode_0xA2_AND_D() {
    and_8bit(get_D());
}

void Z80::opcode_0xA3_AND_E() {
    and_8bit(get_E());
}

void Z80::opcode_0xA4_AND_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: and_8bit(get_H()); break;
        case IndexMode::IX: and_8bit(get_IXH()); break;
        case IndexMode::IY: and_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xA5_AND_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: and_8bit(get_L()); break;
        case IndexMode::IX: and_8bit(get_IXL()); break;
        case IndexMode::IY: and_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xA6_AND_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        and_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        and_8bit(read_byte(address));
    }
}

void Z80::opcode_0xA7_AND_A() {
    and_8bit(get_A());
}

void Z80::opcode_0xA8_XOR_B() {
    xor_8bit(get_B());
}

void Z80::opcode_0xA9_XOR_C() {
    xor_8bit(get_C());
}

void Z80::opcode_0xAA_XOR_D() {
    xor_8bit(get_D());
}

void Z80::opcode_0xAB_XOR_E() {
    xor_8bit(get_E());
}

void Z80::opcode_0xAC_XOR_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: xor_8bit(get_H()); break;
        case IndexMode::IX: xor_8bit(get_IXH()); break;
        case IndexMode::IY: xor_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xAD_XOR_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: xor_8bit(get_L()); break;
        case IndexMode::IX: xor_8bit(get_IXL()); break;
        case IndexMode::IY: xor_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xAE_XOR_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        xor_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        xor_8bit(read_byte(address));
    }
}

void Z80::opcode_0xAF_XOR_A() {
    xor_8bit(get_A());
}

void Z80::opcode_0xB0_OR_B() {
    or_8bit(get_B());
}

void Z80::opcode_0xB1_OR_C() {
    or_8bit(get_C());
}

void Z80::opcode_0xB2_OR_D() {
    or_8bit(get_D());
}

void Z80::opcode_0xB3_OR_E() {
    or_8bit(get_E());
}

void Z80::opcode_0xB4_OR_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: or_8bit(get_H()); break;
        case IndexMode::IX: or_8bit(get_IXH()); break;
        case IndexMode::IY: or_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xB5_OR_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: or_8bit(get_L()); break;
        case IndexMode::IX: or_8bit(get_IXL()); break;
        case IndexMode::IY: or_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xB6_OR_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        or_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        or_8bit(read_byte(address));
    }
}

void Z80::opcode_0xB7_OR_A() {
    or_8bit(get_A());
}

void Z80::opcode_0xB8_CP_B() {
    cp_8bit(get_B());
}

void Z80::opcode_0xB9_CP_C() {
    cp_8bit(get_C());
}

void Z80::opcode_0xBA_CP_D() {
    cp_8bit(get_D());
}

void Z80::opcode_0xBB_CP_E() {
    cp_8bit(get_E());
}

void Z80::opcode_0xBC_CP_H() {
    switch (get_index_mode()) {
        case IndexMode::HL: cp_8bit(get_H()); break;
        case IndexMode::IX: cp_8bit(get_IXH()); break;
        case IndexMode::IY: cp_8bit(get_IYH()); break;
    }
}

void Z80::opcode_0xBD_CP_L() {
    switch (get_index_mode()) {
        case IndexMode::HL: cp_8bit(get_L()); break;
        case IndexMode::IX: cp_8bit(get_IXL()); break;
        case IndexMode::IY: cp_8bit(get_IYL()); break;
    }
}

void Z80::opcode_0xBE_CP_HL_ptr() {
    if (get_index_mode() == IndexMode::HL) {
        cp_8bit(read_byte(get_HL()));
    } else {
        add_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = (get_index_mode() == IndexMode::IX) ? (get_IX() + offset) : (get_IY() + offset);
        cp_8bit(read_byte(address));
    }
}

void Z80::opcode_0xBF_CP_A() {
    cp_8bit(get_A());
}

void Z80::opcode_0xC0_RET_NZ() {
    add_ticks(1);
    if (!is_Z_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xC1_POP_BC() {
    set_BC(pop_word());
}

void Z80::opcode_0xC2_JP_NZ_nn() {
    uint16_t address = fetch_next_word();
    if (!is_Z_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xC3_JP_nn() {
    set_PC(fetch_next_word());
}

void Z80::opcode_0xC4_CALL_NZ_nn() {
    uint16_t address = fetch_next_word();
    if (!is_Z_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xC5_PUSH_BC() {
    add_ticks(1);
    push_word(get_BC());
}

void Z80::opcode_0xC6_ADD_A_n() {
    add_8bit(fetch_next_byte());
}

void Z80::opcode_0xC7_RST_00H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0000);
}

void Z80::opcode_0xC8_RET_Z() {
    add_ticks(1);
    if (is_Z_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xC9_RET() {
    set_PC(pop_word());
}

void Z80::opcode_0xCA_JP_Z_nn() {
    uint16_t address = fetch_next_word();
    if (is_Z_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xCC_CALL_Z_nn() {
    uint16_t address = fetch_next_word();
    if (is_Z_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xCD_CALL_nn() {
    add_ticks(1);
    uint16_t address = fetch_next_word();
    push_word(get_PC());
    set_PC(address);
}

void Z80::opcode_0xCE_ADC_A_n() {
    adc_8bit(fetch_next_byte());
}

void Z80::opcode_0xCF_RST_08H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0008);
}

void Z80::opcode_0xD0_RET_NC() {
    add_ticks(1);
    if (!is_C_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xD1_POP_DE() {
    set_DE(pop_word());
}

void Z80::opcode_0xD2_JP_NC_nn() {
    uint16_t address = fetch_next_word();
    if (!is_C_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xD3_OUT_n_ptr_A() {
    add_ticks(4);
    uint8_t port_lo = fetch_next_byte();
    write_byte_to_io((get_A() << 8) | port_lo, get_A());
}

void Z80::opcode_0xD4_CALL_NC_nn() {
    uint16_t address = fetch_next_word();
    if (!is_C_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xD5_PUSH_DE() {
    add_ticks(1);
    push_word(get_DE());
}

void Z80::opcode_0xD6_SUB_n() {
    sub_8bit(fetch_next_byte());
}

void Z80::opcode_0xD7_RST_10H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0010);
}

void Z80::opcode_0xD8_RET_C() {
    add_ticks(1);
    if (is_C_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xD9_EXX() {
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
    uint16_t address = fetch_next_word();
    if (is_C_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xDB_IN_A_n_ptr() {
    add_ticks(4);
    uint8_t port_lo = fetch_next_byte();
    uint16_t port = (get_A() << 8) | port_lo;
    set_A(read_byte_from_io(port));
}

void Z80::opcode_0xDC_CALL_C_nn() {
    uint16_t address = fetch_next_word();
    if (is_C_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xDE_SBC_A_n() {
    sbc_8bit(fetch_next_byte());
}

void Z80::opcode_0xDF_RST_18H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0018);
}

void Z80::opcode_0xE0_RET_PO() {
    add_ticks(1);
    if (!is_PV_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xE1_POP_HL() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(pop_word()); break;
        case IndexMode::IX: set_IX(pop_word()); break;
        case IndexMode::IY: set_IY(pop_word()); break;
    }
}

void Z80::opcode_0xE2_JP_PO_nn() {
    uint16_t address = fetch_next_word();
    if (!is_PV_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xE3_EX_SP_ptr_HL() {
    add_ticks(3);
    uint16_t from_stack = read_word(get_SP());
    switch(get_index_mode()) {
        case IndexMode::HL:
            write_word(get_SP(), get_HL());
            set_HL(from_stack);
            break;
        case IndexMode::IX:
            write_word(get_SP(), get_IX());
            set_IX(from_stack);
            break;
        case IndexMode::IY:
            write_word(get_SP(), get_IY());
            set_IY(from_stack);
            break;
    }
}

void Z80::opcode_0xE4_CALL_PO_nn() {
    uint16_t address = fetch_next_word();
    if (!is_PV_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xE5_PUSH_HL() {
    add_ticks(1);
    switch (get_index_mode()) {
        case IndexMode::HL: push_word(get_HL()); break;
        case IndexMode::IX: push_word(get_IX()); break;
        case IndexMode::IY: push_word(get_IY()); break;
    }
}

void Z80::opcode_0xE6_AND_n() {
    and_8bit(fetch_next_byte());
}

void Z80::opcode_0xE7_RST_20H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0020);
}

void Z80::opcode_0xE8_RET_PE() {
    add_ticks(1);
    if (is_PV_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xE9_JP_HL_ptr() {
    switch (get_index_mode()) {
        case IndexMode::HL: set_PC(get_HL()); break;
        case IndexMode::IX: set_PC(get_IX()); break;
        case IndexMode::IY: set_PC(get_IY()); break;
    }
}

void Z80::opcode_0xEA_JP_PE_nn() {
    uint16_t address = fetch_next_word();
    if (is_PV_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xEB_EX_DE_HL() {
    uint16_t temp = get_HL();
    set_HL(get_DE());
    set_DE(temp);
}

void Z80::opcode_0xEC_CALL_PE_nn() {
    uint16_t address = fetch_next_word();
    if (is_PV_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xEE_XOR_n() {
    xor_8bit(fetch_next_byte());
}

void Z80::opcode_0xEF_RST_28H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0028);
}

void Z80::opcode_0xF0_RET_P() {
    add_ticks(1);
    if (!is_S_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xF1_POP_AF() {
    set_AF(pop_word());
}

void Z80::opcode_0xF2_JP_P_nn() {
    uint16_t address = fetch_next_word();
    if (!is_S_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xF3_DI() {
    set_IFF1(false);
    set_IFF2(false);
}

void Z80::opcode_0xF4_CALL_P_nn() {
    uint16_t address = fetch_next_word();
    if (!is_S_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xF5_PUSH_AF() {
    add_ticks(1);
    push_word(get_AF());
}

void Z80::opcode_0xF6_OR_n() {
    or_8bit(fetch_next_byte());
}

void Z80::opcode_0xF7_RST_30H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0030);
}

void Z80::opcode_0xF8_RET_M() {
    add_ticks(1);
    if (is_S_flag_set()) {
        set_PC(pop_word());
    }
}

void Z80::opcode_0xF9_LD_SP_HL() {
    add_ticks(2);
    switch (get_index_mode()) {
        case IndexMode::HL: set_SP(get_HL()); break;
        case IndexMode::IX: set_SP(get_IX()); break;
        case IndexMode::IY: set_SP(get_IY()); break;
    }
}

void Z80::opcode_0xFA_JP_M_nn() {
    uint16_t address = fetch_next_word();
    if (is_S_flag_set()) {
        set_PC(address);
    }
}

void Z80::opcode_0xFB_EI() {
    set_interrupt_enable_pending(true);
}

void Z80::opcode_0xFC_CALL_M_nn() {
    uint16_t address = fetch_next_word();
    if (is_S_flag_set()) {
        add_ticks(1);
        push_word(get_PC());
        set_PC(address);
    }
}

void Z80::opcode_0xFE_CP_n() {
    cp_8bit(fetch_next_byte());
}

void Z80::opcode_0xFF_RST_38H() {
    add_ticks(1);
    push_word(get_PC());
    set_PC(0x0038);
}

void Z80::opcode_0xED_0x40_IN_B_C_ptr() {
    add_ticks(4);
    set_B(in_r_c());
}

void Z80::opcode_0xED_0x41_OUT_C_ptr_B() {
    add_ticks(4);
    out_c_r(get_B());
}

void Z80::opcode_0xED_0x42_SBC_HL_BC() {
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0xED_0x43_LD_nn_ptr_BC() {
    write_word(fetch_next_word(), get_BC());
}

void Z80::opcode_0xED_0x44_NEG() {
    uint8_t value = get_A();
    uint8_t result = -value;
    set_A(result);
    set_flag_if(FLAG_S, (result & 0x80) != 0);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, (0x00 & 0x0F) < (value & 0x0F));
    set_flag(FLAG_N);
    set_flag_if(FLAG_C, value != 0);
    set_flag_if(FLAG_PV, value == 0x80);
    set_flag_if(FLAG_X, (result & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (result & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x45_RETN() {
    set_PC(pop_word());
    set_IFF1(get_IFF2());
}

void Z80::opcode_0xED_0x46_IM_0() {
    set_interrupt_mode(0);
}

void Z80::opcode_0xED_0x47_LD_I_A() {
    add_ticks(1);
    set_I(get_A());
}

void Z80::opcode_0xED_0x48_IN_C_C_ptr() {
    add_ticks(4);
    set_C(in_r_c());
}

void Z80::opcode_0xED_0x49_OUT_C_ptr_C() {
    add_ticks(4);
    out_c_r(get_C());
}

void Z80::opcode_0xED_0x4A_ADC_HL_BC() {
    add_ticks(7);
    uint16_t val_bc = get_BC();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_bc)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_bc)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_bc)); break;
    }
}

void Z80::opcode_0xED_0x4B_LD_BC_nn_ptr() {
    set_BC(read_word(fetch_next_word()));
}

void Z80::opcode_0xED_0x4D_RETI() {
    set_PC(pop_word());
    set_IFF1(get_IFF2());
    set_reti_signaled(true);
}

void Z80::opcode_0xED_0x4F_LD_R_A() {
    add_ticks(1);
    set_R(get_A());
}

void Z80::opcode_0xED_0x50_IN_D_C_ptr() {
    add_ticks(4);
    set_D(in_r_c());
}

void Z80::opcode_0xED_0x51_OUT_C_ptr_D() {
    add_ticks(4);
    out_c_r(get_D());
}

void Z80::opcode_0xED_0x52_SBC_HL_DE() {
    add_ticks(7);
    uint16_t val_de = get_DE();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_de)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_de)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_de)); break;
    }
}

void Z80::opcode_0xED_0x53_LD_nn_ptr_DE() {
    write_word(fetch_next_word(), get_DE());
}

void Z80::opcode_0xED_0x56_IM_1() {
    set_interrupt_mode(1);
}

void Z80::opcode_0xED_0x57_LD_A_I() {
    add_ticks(1);
    uint8_t i_value = get_I();
    set_A(i_value);
    set_flag_if(FLAG_S, (i_value & 0x80) != 0);
    set_flag_if(FLAG_Z, i_value == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_IFF2());
    set_flag_if(FLAG_Y, (i_value & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (i_value & FLAG_X) != 0);
}

void Z80::opcode_0xED_0x58_IN_E_C_ptr() {
    add_ticks(4);
    set_E(in_r_c());
}

void Z80::opcode_0xED_0x59_OUT_C_ptr_E() {
    add_ticks(4);
    out_c_r(get_E());
}

void Z80::opcode_0xED_0x5A_ADC_HL_DE() {
    add_ticks(7);
    uint16_t val_de = get_DE();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_de)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_de)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_de)); break;
    }
}

void Z80::opcode_0xED_0x5B_LD_DE_nn_ptr() {
    set_DE(read_word(fetch_next_word()));
}

void Z80::opcode_0xED_0x5E_IM_2() {
    set_interrupt_mode(2);
}

void Z80::opcode_0xED_0x5F_LD_A_R() {
    add_ticks(1);
    uint8_t r_value = get_R();
    set_A(r_value);
    set_flag_if(FLAG_S, (r_value & 0x80) != 0);
    set_flag_if(FLAG_Z, r_value == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_IFF2());
    set_flag_if(FLAG_Y, (r_value & FLAG_Y) != 0);
    set_flag_if(FLAG_X, (r_value & FLAG_X) != 0);
}

void Z80::opcode_0xED_0x60_IN_H_C_ptr() {
    add_ticks(4);
    set_H(in_r_c());
}

void Z80::opcode_0xED_0x61_OUT_C_ptr_H() {
    add_ticks(4);
    out_c_r(get_H());
}

void Z80::opcode_0xED_0x62_SBC_HL_HL() {
    add_ticks(7);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), get_HL())); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), get_IX())); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), get_IY())); break;
    }
}

void Z80::opcode_0xED_0x63_LD_nn_ptr_HL_ED() {
    write_word(fetch_next_word(), get_HL());
}

void Z80::opcode_0xED_0x67_RRD() {
    add_ticks(4);
    uint16_t address = get_HL();
    uint8_t mem_val = read_byte(address);
    uint8_t a_val = get_A();
    uint8_t new_a = (a_val & 0xF0) | (mem_val & 0x0F);
    uint8_t new_mem = (mem_val >> 4) | ((a_val & 0x0F) << 4);
    set_A(new_a);
    write_byte(address, new_mem);
    set_flag_if(FLAG_S, (new_a & 0x80) != 0);
    set_flag_if(FLAG_Z, new_a == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, is_parity_even(new_a));
    set_flag_if(FLAG_X, (new_a & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (new_a & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x68_IN_L_C_ptr() {
    add_ticks(4);
    set_L(in_r_c());
}

void Z80::opcode_0xED_0x69_OUT_C_ptr_L() {
    add_ticks(4);
    out_c_r(get_L());
}

void Z80::opcode_0xED_0x6A_ADC_HL_HL() {
    add_ticks(7);
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), get_HL())); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), get_IX())); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), get_IY())); break;
    }
}

void Z80::opcode_0xED_0x6B_LD_HL_nn_ptr_ED() {
    set_HL(read_word(fetch_next_word()));
}

void Z80::opcode_0xED_0x6F_RLD() {
    add_ticks(4);
    uint16_t address = get_HL();
    uint8_t mem_val = read_byte(address);
    uint8_t a_val = get_A();
    uint8_t new_a = (a_val & 0xF0) | (mem_val >> 4);
    uint8_t new_mem = (mem_val << 4) | (a_val & 0x0F);
    set_A(new_a);
    write_byte(address, new_mem);
    set_flag_if(FLAG_S, (new_a & 0x80) != 0);
    set_flag_if(FLAG_Z, new_a == 0);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, is_parity_even(new_a));
    set_flag_if(FLAG_X, (new_a & FLAG_X) != 0);
    set_flag_if(FLAG_Y, (new_a & FLAG_Y) != 0);
}

void Z80::opcode_0xED_0x70_IN_C_ptr() {
    add_ticks(4);
    in_r_c();
}

void Z80::opcode_0xED_0x71_OUT_C_ptr_0() {
    add_ticks(4);
    out_c_r(0x00);
}

void Z80::opcode_0xED_0x72_SBC_HL_SP() {
    add_ticks(7);
    uint16_t val_sp = get_SP();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(sbc_16bit(get_HL(), val_sp)); break;
        case IndexMode::IX: set_IX(sbc_16bit(get_IX(), val_sp)); break;
        case IndexMode::IY: set_IY(sbc_16bit(get_IY(), val_sp)); break;
    }
}

void Z80::opcode_0xED_0x73_LD_nn_ptr_SP() {
    write_word(fetch_next_word(), get_SP());
}

void Z80::opcode_0xED_0x78_IN_A_C_ptr() {
    add_ticks(4);
    set_A(in_r_c());
}

void Z80::opcode_0xED_0x79_OUT_C_ptr_A() {
    add_ticks(4);
    out_c_r(get_A());
}

void Z80::opcode_0xED_0x7A_ADC_HL_SP() {
    add_ticks(7);
    uint16_t val_sp = get_SP();
    switch (get_index_mode()) {
        case IndexMode::HL: set_HL(adc_16bit(get_HL(), val_sp)); break;
        case IndexMode::IX: set_IX(adc_16bit(get_IX(), val_sp)); break;
        case IndexMode::IY: set_IY(adc_16bit(get_IY(), val_sp)); break;
    }
}

void Z80::opcode_0xED_0x7B_LD_SP_nn_ptr() {
    set_SP(read_word(fetch_next_word()));
}

void Z80::opcode_0xED_0xA0_LDI() {
    add_ticks(2);
    uint8_t value = read_byte(get_HL());
    write_byte(get_DE(), value);
    set_HL(get_HL() + 1);
    set_DE(get_DE() + 1);
    set_BC(get_BC() - 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_BC() != 0);
    uint8_t temp = get_A() + value;
    set_flag_if(FLAG_Y, (temp & 0x02) != 0);
    set_flag_if(FLAG_X, (temp & 0x08) != 0);
}

void Z80::opcode_0xED_0xA1_CPI() {
    add_ticks(5);
    uint8_t value = read_byte(get_HL());
    uint8_t result = get_A() - value;
    bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
    set_HL(get_HL() + 1);
    set_BC(get_BC() - 1);
    set_flag_if(FLAG_S, result & 0x80);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, half_carry);
    set_flag_if(FLAG_PV, get_BC() != 0);
    set_flag(FLAG_N);
    uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
    set_flag_if(FLAG_Y, (temp & 0x02) != 0);
    set_flag_if(FLAG_X, (temp & 0x08) != 0);
}


void Z80::opcode_0xED_0xA2_INI() {
    add_ticks(5);
    uint8_t port_val = read_byte_from_io(get_BC());
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte(get_HL(), port_val);
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
    add_ticks(5);
    uint8_t mem_val = read_byte(get_HL());
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte_to_io(get_BC(), mem_val);
    set_HL(get_HL() + 1);
    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);
    uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
    set_flag_if(FLAG_C, temp > 0xFF);
    set_flag_if(FLAG_H, temp > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
}


void Z80::opcode_0xED_0xA8_LDD() {
    add_ticks(2);
    uint8_t value = read_byte(get_HL());
    write_byte(get_DE(), value);
    set_HL(get_HL() - 1);
    set_DE(get_DE() - 1);
    set_BC(get_BC() - 1);
    clear_flag(FLAG_H | FLAG_N);
    set_flag_if(FLAG_PV, get_BC() != 0);
    uint8_t temp = get_A() + value;
    set_flag_if(FLAG_Y, (temp & 0x02) != 0);
    set_flag_if(FLAG_X, (temp & 0x08) != 0);
}

void Z80::opcode_0xED_0xA9_CPD() {
    add_ticks(5);
    uint8_t value = read_byte(get_HL());
    uint8_t result = get_A() - value;
    bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
    set_HL(get_HL() - 1);
    set_BC(get_BC() - 1);
    set_flag_if(FLAG_S, result & 0x80);
    set_flag_if(FLAG_Z, result == 0);
    set_flag_if(FLAG_H, half_carry);
    set_flag_if(FLAG_PV, get_BC() != 0);
    set_flag(FLAG_N);
    uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
    set_flag_if(FLAG_Y, (temp & 0x02) != 0);
    set_flag_if(FLAG_X, (temp & 0x08) != 0);
}

void Z80::opcode_0xED_0xAA_IND() {
    add_ticks(5);
    uint8_t port_val = read_byte_from_io(get_BC());
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte(get_HL(), port_val);
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
    add_ticks(5);
    uint8_t mem_val = read_byte(get_HL());
    uint8_t b_val = get_B();
    set_B(b_val - 1);
    write_byte_to_io(get_BC(), mem_val);
    set_HL(get_HL() - 1);
    set_flag(FLAG_N);
    set_flag_if(FLAG_Z, b_val - 1 == 0);
    uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
    set_flag_if(FLAG_C, temp > 0xFF);
    set_flag_if(FLAG_H, temp > 0xFF);
    set_flag_if(FLAG_PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
}

void Z80::opcode_0xED_0xB0_LDIR() {
    opcode_0xED_0xA0_LDI();
    if (get_BC() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB1_CPIR() {
    opcode_0xED_0xA1_CPI();
    if (get_BC() != 0 && !is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB2_INIR() {
    opcode_0xED_0xA2_INI();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB3_OTIR() {
    opcode_0xED_0xA3_OUTI();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB8_LDDR() {
    opcode_0xED_0xA8_LDD();
    if (get_BC() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xB9_CPDR() {
    opcode_0xED_0xA9_CPD();
    if (get_BC() != 0 && !is_Z_flag_set()) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xBA_INDR() {
    opcode_0xED_0xAA_IND();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}

void Z80::opcode_0xED_0xBB_OTDR() {
    opcode_0xED_0xAB_OUTD();
    if (get_B() != 0) {
        add_ticks(5);
        set_PC(get_PC() - 2);
    }
}