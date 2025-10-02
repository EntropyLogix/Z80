#ifndef __Z80_H__
#define __Z80_H__

#include <cstdint>

#if defined(__GNUC__) || defined(__clang__) // GCC i Clang
    #define Z80_LIKELY(expr)   __builtin_expect(!!(expr), 1)
#elif __cplusplus >= 202002L // C++20
    #define Z80_LIKELY(expr)   (expr) [[likely]]
#else
    #define Z80_LIKELY(expr)   (expr)
#endif

template <typename TMemory, typename TIO, typename TEvents>
class Z80 {
public:
    union Register {
        uint16_t w;
        struct {
            #if defined(Z80_BIG_ENDIAN)
            uint8_t h; 
            uint8_t l;
            #else
            uint8_t l; 
            uint8_t h; 
            #endif
        };
    };
    enum class IndexMode { HL, IX, IY };
    struct State {
        Register m_AF, m_BC, m_DE, m_HL;
        Register m_IX, m_IY, m_SP, m_PC;
        Register m_AFp, m_BCp, m_DEp, m_HLp;
        uint8_t m_I, m_R;
        bool m_IFF1, m_IFF2;
        bool m_halted;
        bool m_NMI_pending;
        bool m_IRQ_request;
        bool m_EI_delay;
        bool m_RETI_signaled;
        uint8_t m_IRQ_data;
        uint8_t m_IRQ_mode;
        IndexMode m_index_mode;
        long long m_ticks;
        long long m_operation_ticks;
    };

    // Flags
    class Flags {
    public:
        static constexpr uint8_t C  = 1 << 0;
        static constexpr uint8_t N  = 1 << 1;
        static constexpr uint8_t PV = 1 << 2;
        static constexpr uint8_t X  = 1 << 3;
        static constexpr uint8_t H  = 1 << 4;
        static constexpr uint8_t Y  = 1 << 5;
        static constexpr uint8_t Z  = 1 << 6;
        static constexpr uint8_t S  = 1 << 7;
        Flags(uint8_t value) : m_value(value) {}
        operator uint8_t() const { return m_value; }
        Flags& operator=(uint8_t new_value) { return assign(new_value); }
        Flags& zero() { m_value = 0; return *this; }
        Flags& assign(uint8_t value) { m_value = value; return *this; }
        Flags& set(uint8_t mask) { m_value |= mask; return *this; }
        Flags& clear(uint8_t mask) { m_value &= ~mask; return *this; }
        Flags& update(uint8_t mask, bool state) {
            m_value = (m_value & ~mask) | (-static_cast<int8_t>(state) & mask);
            return *this;
        }
        bool is_set(uint8_t mask) const { return (m_value & mask) != 0; }
    private:
        uint8_t m_value;
    };

    // Constructor
    Z80(TMemory& mem, TIO& io, TEvents& events) : m_memory(mem), m_io(io), m_events(events){
        precompute_parity();
        m_memory.connect(this);
        m_memory.reset();
        m_io.connect(this);
        m_io.reset();
        m_events.connect(this);
        m_events.reset();
        reset();
    }

    // Main execution and control interface
    long long run(long long ticks_limit) {return operate<OperateMode::ToLimit>(ticks_limit);}
    int step() {return operate<OperateMode::SingleStep>(0);}
    void reset() {
        set_AF(0); set_BC(0); set_DE(0); set_HL(0);
        set_AFp(0); set_BCp(0); set_DEp(0); set_HLp(0);
        set_IX(0); set_IY(0);
        set_SP(0xFFFF);
        set_PC(0);
        set_R(0);
        set_I(0);
        set_IFF1(false);
        set_IFF2(false);
        set_halted(false);
        set_NMI_pending(false);
        set_IRQ_request(false);
        set_EI_delay(false);
        set_RETI_signaled(false);
        set_IRQ_data(0);
        set_IRQ_mode(0);
        set_ticks(0);
        set_index_mode(IndexMode::HL);
    }
    void request_interrupt(uint8_t data) {
        set_IRQ_request(true);
        set_IRQ_data(data);
    }
    void request_NMI() { set_NMI_pending(true); }

    // High-Level State Management Methods ---
    State save_state() const {
        State state;
        state.m_AF = get_AF();
        state.m_BC = get_BC();
        state.m_DE = get_DE();
        state.m_HL = get_HL();
        state.m_IX = get_IX();
        state.m_IY = get_IY();
        state.m_SP = get_SP();
        state.m_PC = get_PC();
        state.m_AFp = get_AFp();
        state.m_BCp = get_BCp();
        state.m_DEp = get_DEp();
        state.m_HLp = get_HLp();
        state.m_I = get_I();
        state.m_R = get_R();
        state.m_IFF1 = get_IFF1();
        state.m_IFF2 = get_IFF2();
        state.m_halted = is_halted();
        state.m_NMI_pending = is_NMI_pending();
        state.m_IRQ_request = is_IRQ_requested();
        state.m_EI_delay = get_EI_delay();
        state.m_IRQ_data = get_IRQ_data();
        state.m_IRQ_mode = get_IRQ_mode();
        state.m_index_mode = get_index_mode();
        state.m_ticks = get_ticks();
        state.m_operation_ticks = get_operation_ticks();
        state.m_RETI_signaled = is_RETI_signaled();
        return state;
    }
    void restore_state(const State& state) {
        set_AF(state.m_AF);
        set_BC(state.m_BC);
        set_DE(state.m_DE);
        set_HL(state.m_HL);
        set_IX(state.m_IX);
        set_IY(state.m_IY);
        set_SP(state.m_SP);
        set_PC(state.m_PC);
        set_AFp(state.m_AFp);
        set_BCp(state.m_BCp);
        set_DEp(state.m_DEp);
        set_HLp(state.m_HLp);
        set_I(state.m_I);
        set_R(state.m_R);
        set_IFF1(state.m_IFF1);
        set_IFF2(state.m_IFF2);
        set_halted(state.m_halted);
        set_NMI_pending(state.m_NMI_pending);
        set_IRQ_request(state.m_IRQ_request);
        set_EI_delay(state.m_EI_delay);
        set_IRQ_data(state.m_IRQ_data);
        set_IRQ_mode(state.m_IRQ_mode);
        set_index_mode(state.m_index_mode);
        set_ticks(state.m_ticks);
        set_operation_ticks(state.m_operation_ticks);
        set_RETI_signaled(state.m_RETI_signaled);
    }

    // Cycle counter
    long long get_ticks() const { return m_ticks; }
    void set_ticks(long long value) { m_ticks = value; }
    void add_ticks(long long delta) {
        long long target_ticks = m_ticks + delta;
        while (m_ticks < target_ticks) {
            long long cycles_in_step = target_ticks - m_ticks; 
            long long cycles_to_sync = m_events.get_event_limit() - m_ticks;
            if (cycles_to_sync > 0 && cycles_to_sync <= cycles_in_step) {
                m_ticks += cycles_to_sync; 
                m_events.handle_event(m_ticks); 
            } else {
                m_ticks = target_ticks;
                break;
            }
        }
    }

    // 16-bit main registers
    uint16_t get_AF() const { return m_AF.w; }
    void set_AF(uint16_t value) { m_AF.w = value; }
    uint16_t get_BC() const { return m_BC.w; }
    void set_BC(uint16_t value) { m_BC.w = value; }
    uint16_t get_DE() const { return m_DE.w; }
    void set_DE(uint16_t value) { m_DE.w = value; }
    uint16_t get_HL() const { return m_HL.w; }
    void set_HL(uint16_t value) { m_HL.w = value; }
    uint16_t get_IX() const { return m_IX.w; }
    void set_IX(uint16_t value) { m_IX.w = value; }
    uint16_t get_IY() const { return m_IY.w; }
    void set_IY(uint16_t value) { m_IY.w = value; }
    uint16_t get_SP() const { return m_SP; }
    void set_SP(uint16_t value) { m_SP = value; }
    uint16_t get_PC() const { return m_PC; }
    void set_PC(uint16_t value) { m_PC = value; }

    // 16-bit alternate registers
    uint16_t get_AFp() const { return m_AFp.w; }
    void set_AFp(uint16_t value) { m_AFp.w = value; }
    uint16_t get_BCp() const { return m_BCp.w; }
    void set_BCp(uint16_t value) { m_BCp.w = value; }
    uint16_t get_DEp() const { return m_DEp.w; }
    void set_DEp(uint16_t value) { m_DEp.w = value; }
    uint16_t get_HLp() const { return m_HLp.w; }
    void set_HLp(uint16_t value) { m_HLp.w = value; }

    // 8-bit registers
    uint8_t get_A() const { return m_AF.h; }
    void set_A(uint8_t value) { m_AF.h = value; }
    Flags get_F() const { return m_AF.l; }
    void set_F(Flags value) { m_AF.l = value; }
    uint8_t get_B() const { return m_BC.h; }
    void set_B(uint8_t value) { m_BC.h = value; }
    uint8_t get_C() const { return m_BC.l; }
    void set_C(uint8_t value) { m_BC.l = value; }
    uint8_t get_D() const { return m_DE.h; }
    void set_D(uint8_t value) { m_DE.h = value; }
    uint8_t get_E() const { return m_DE.l; }
    void set_E(uint8_t value) { m_DE.l = value; }
    uint8_t get_H() const { return m_HL.h; }
    void set_H(uint8_t value) { m_HL.h = value; }
    uint8_t get_L() const { return m_HL.l; }
    void set_L(uint8_t value) { m_HL.l = value; }
    uint8_t get_IXH() const { return m_IX.h; }
    void set_IXH(uint8_t value) { m_IX.h = value; }
    uint8_t get_IXL() const { return m_IX.l; }
    void set_IXL(uint8_t value) { m_IX.l = value; }
    uint8_t get_IYH() const { return m_IY.h; }
    void set_IYH(uint8_t value) { m_IY.h = value; }
    uint8_t get_IYL() const { return m_IY.l; }
    void set_IYL(uint8_t value) { m_IY.l = value; }

    // Special purpose registers
    uint8_t get_I() const { return m_I; }
    void set_I(uint8_t value) { m_I = value; }
    uint8_t get_R() const { return m_R; }
    void set_R(uint8_t value) { m_R = value; }

    // CPU state flags
    bool get_IFF1() const { return m_IFF1; }
    void set_IFF1(bool state) { m_IFF1 = state; }
    bool get_IFF2() const { return m_IFF2; }
    void set_IFF2(bool state) { m_IFF2 = state; }
    bool is_halted() const { return m_halted; }
    void set_halted(bool state) { m_halted = state; }
    
    // Interrupt state flags
    bool is_NMI_pending() const { return m_NMI_pending; }
    void set_NMI_pending(bool state) { m_NMI_pending = state; }
    bool is_IRQ_requested() const { return m_IRQ_request; }
    void set_IRQ_request(bool state) { m_IRQ_request = state; }
    bool is_IRQ_pending() const {return is_IRQ_requested() && get_IFF1(); }
    bool get_EI_delay() const { return m_EI_delay; }
    void set_EI_delay(bool state) { m_EI_delay = state; }
    uint8_t get_IRQ_data() const { return m_IRQ_data; }
    void set_IRQ_data(uint8_t data) { m_IRQ_data = data; }
    uint8_t get_IRQ_mode() const { return m_IRQ_mode; }
    void set_IRQ_mode(uint8_t mode) { m_IRQ_mode = mode; }
    void set_RETI_signaled(bool state) { m_RETI_signaled = state; }
    bool is_RETI_signaled() const { return m_RETI_signaled;}
    
    // Processing opcodes DD and FD index
    IndexMode get_index_mode() const { return m_index_mode;}
    void set_index_mode(IndexMode mode) { m_index_mode = mode; }
    
private:
    //CPU registers
    Register m_AF, m_BC, m_DE, m_HL;
    Register m_AFp, m_BCp, m_DEp, m_HLp;
    Register m_IX, m_IY;
    uint16_t m_SP, m_PC;
    uint8_t m_I, m_R;
    bool m_IFF1, m_IFF2;
    
    //internal CPU states
    bool m_halted, m_NMI_pending, m_IRQ_request, m_EI_delay, m_RETI_signaled;
    uint8_t m_IRQ_data, m_IRQ_mode;
    IndexMode m_index_mode;
    
    //CPU T-states
    long long m_ticks, m_operation_ticks;

    //Memory and IO operations
    TMemory& m_memory;
    TIO& m_io;
    TEvents& m_events;

    //Ticks calculated for operation (including IRQ/NMI ticks if any)
    void set_operation_ticks(long long value) { m_operation_ticks = value; }
    long long get_operation_ticks() const { return m_operation_ticks; }
    void add_operation_ticks(int delta) { m_operation_ticks += delta; }

    //Internal memory access helpers
    uint8_t read_byte(uint16_t address) {
        add_operation_ticks(3);
        return m_memory.read(address); 
    }
    void write_byte(uint16_t address, uint8_t value) {
        add_operation_ticks(3);
        m_memory.write(address, value);
    }
    uint16_t read_word(uint16_t address) {
        add_operation_ticks(6);
        uint8_t low_byte = m_memory.read(address);
        uint8_t high_byte = m_memory.read(address + 1);
        return (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    }

    void write_word(uint16_t address, uint16_t value) {
        add_operation_ticks(6);
        m_memory.write(address, value & 0xFF);
        m_memory.write(address + 1, (value >> 8));
    }
    void push_word(uint16_t value) {
        uint16_t new_sp = get_SP() - 2;
        set_SP(new_sp);
        write_word(new_sp, value);
    }
    uint16_t pop_word() {
        uint16_t current_sp = get_SP();
        uint16_t value = read_word(current_sp);
        set_SP(current_sp + 2);
        return value;
    }
    uint8_t fetch_next_opcode() {
        add_operation_ticks(1);
        uint8_t r_val = get_R();
        set_R(((r_val + 1) & 0x7F) | (r_val & 0x80));
        uint16_t current_pc = get_PC();
        uint8_t opcode = read_byte(current_pc);
        set_PC(current_pc + 1);
        return opcode;
    }
    uint8_t fetch_next_byte() {
        uint16_t current_pc = get_PC();
        uint8_t byte_val = read_byte(current_pc);
        set_PC(current_pc + 1);
        return byte_val;
    }
    uint16_t fetch_next_word() {
        uint8_t low_byte = fetch_next_byte();
        uint8_t high_byte = fetch_next_byte();
        return (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    }
    
    //Parity bits
    bool parity_table[256];
    bool is_parity_even(uint8_t value) { return parity_table[value]; }
    void precompute_parity() {
        for (int i = 0; i < 256; ++i) {
            int count = 0;
            int temp = i;
            while (temp > 0) {
                temp &= (temp - 1);
                count++;
            }
            parity_table[i] = (count % 2) == 0;
        }
    }

    //Indexed opcodes helpers
    uint16_t get_indexed_HL() const {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL))
            return m_HL.w;
        else if (m_index_mode == IndexMode::IX)
            return m_IX.w;
        else
            return m_IY.w;
    }
    void set_indexed_HL(uint16_t value) {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL))
            set_HL(value);
        else if (m_index_mode == IndexMode::IX)
            set_IX(value);
        else
            set_IY(value);
    }
    uint8_t get_indexed_H() const {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL))
            return get_H();
        else if (m_index_mode == IndexMode::IX)
            return get_IXH();
        else
            return get_IYH();
    }
    void set_indexed_H(uint8_t value) {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL)) 
            set_H(value);
        else if (m_index_mode == IndexMode::IX) 
            set_IXH(value);
        else
            set_IYH(value);
    }
    uint8_t get_indexed_L() const {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL)) return get_L();
        else if (m_index_mode == IndexMode::IX)
            return get_IXL();
        else
            return get_IYL();
    }
    void set_indexed_L(uint8_t value) {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL))
            set_L(value);
        else if (m_index_mode == IndexMode::IX)
            set_IXL(value);
        else
            set_IYL(value);
    }
    uint16_t get_indexed_address() {
        if (Z80_LIKELY(m_index_mode == IndexMode::HL))
            return get_HL(); 
        else {
            add_operation_ticks(5);
            int8_t offset = static_cast<int8_t>(fetch_next_byte()); 
            return get_indexed_HL() + offset;
        }
    }
    uint8_t get_indexed_HL_ptr() {
        uint16_t address = get_indexed_address();
        return read_byte(address);
    }
    void set_indexed_HL_ptr(uint8_t value) {
        uint16_t address = get_indexed_address();
        write_byte(address, value);
    }

    //Arithmetics and logics operations helpers
    uint8_t inc_8bit(uint8_t value) {
        uint8_t result = value + 1;
        Flags flags(get_F() & Flags::C);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (value & 0x0F) == 0x0F)
            .update(Flags::PV, value == 0x7F)
            .clear(Flags::N)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t dec_8bit(uint8_t value) {
        uint8_t result = value - 1;
        Flags flags(get_F() & Flags::C);
        flags.set(Flags::N)
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (value & 0x0F) == 0x00)
            .update(Flags::PV, value == 0x80)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    void and_8bit(uint8_t value) {
        uint8_t result = get_A() & value;
        set_A(result);
        Flags flags(Flags::H);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
    }
    void or_8bit(uint8_t value) {
        uint8_t result = get_A() | value;
        set_A(result);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
    }
    void xor_8bit(uint8_t value) {
        uint8_t result = get_A() ^ value;
        set_A(result);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
    }
    void cp_8bit(uint8_t value) {
        uint8_t a = get_A();
        uint8_t result = a - value;
        Flags flags(0);
        flags.set(Flags::N)
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (a & 0x0F) < (value & 0x0F))
            .update(Flags::PV, ((a ^ value) & (a ^ result)) & 0x80)
            .update(Flags::C, a < value)
            .update(Flags::X, (value & Flags::X) != 0)
            .update(Flags::Y, (value & Flags::Y) != 0);
        set_F(flags);
    }
    void add_8bit(uint8_t value) {
        uint8_t a = get_A();
        uint16_t result16 = (uint16_t)a + (uint16_t)value;
        uint8_t result = result16 & 0xFF;
        set_A(result);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, ((a & 0x0F) + (value & 0x0F)) > 0x0F)
            .update(Flags::PV, ((a ^ value ^ 0x80) & (a ^ result)) & 0x80)
            .update(Flags::C, result16 > 0xFF)
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    void adc_8bit(uint8_t value) {
        uint8_t a = get_A();
        Flags flags = get_F();
        uint8_t carry = flags.is_set(Flags::C);
        uint16_t result16 = (uint16_t)a + (uint16_t)value + carry;
        uint8_t result = result16 & 0xFF;
        set_A(result);
        flags.zero()
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, ((a & 0x0F) + (value & 0x0F) + carry) > 0x0F)
            .update(Flags::PV, ((a ^ value ^ 0x80) & (a ^ result)) & 0x80)
            .update(Flags::C, result16 > 0xFF)
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    void sub_8bit(uint8_t value) {
        uint8_t a = get_A();
        uint8_t result = a - value;
        set_A(result);
        Flags flags(Flags::N);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (a & 0x0F) < (value & 0x0F))
            .update(Flags::PV, ((a ^ value) & (a ^ result)) & 0x80)
            .update(Flags::C, a < value)
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    void sbc_8bit(uint8_t value) {
        Flags flags = get_F();
        uint8_t a = get_A();
        uint8_t carry = flags.is_set(Flags::C);
        uint16_t result16 = (uint16_t)a - (uint16_t)value - carry;
        uint8_t result = result16 & 0xFF;
        set_A(result);
        flags.assign(Flags::N)
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (a & 0x0F) < ((value & 0x0F) + carry))
            .update(Flags::PV, ((a ^ value) & (a ^ result)) & 0x80)
            .update(Flags::C, result16 > 0x00FF)
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    uint16_t add_16bit(uint16_t reg, uint16_t value) {
        uint32_t result32 = (uint32_t)reg + (uint32_t)value;
        uint16_t result = result32 & 0xFFFF;
        Flags flags = get_F();
        flags.clear(Flags::N)
            .update(Flags::H, ((reg & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF)
            .update(Flags::C, result32 > 0xFFFF)
            .update(Flags::Y, (result & 0x2000) != 0)
            .update(Flags::X, (result & 0x0800) != 0);
        set_F(flags);
        return result;
    }
    uint16_t adc_16bit(uint16_t reg, uint16_t value) {
        Flags flags = get_F();
        uint8_t carry = flags.is_set(Flags::C);
        uint32_t result32 = (uint32_t)reg + (uint32_t)value + carry;
        uint16_t result = result32 & 0xFFFF;
        flags.zero()
            .update(Flags::S, (result & 0x8000) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, ((reg & 0x0FFF) + (value & 0x0FFF) + carry) > 0x0FFF)
            .update(Flags::PV, (((reg ^ result) & (value ^ result)) & 0x8000) != 0)
            .update(Flags::C, result32 > 0xFFFF)
            .update(Flags::Y, (result & 0x2000) != 0)
            .update(Flags::X, (result & 0x0800) != 0);
        set_F(flags);
        return result;
    }
    uint16_t sbc_16bit(uint16_t reg, uint16_t value) {
        Flags flags = get_F();
        uint8_t carry = flags.is_set(Flags::C);
        uint32_t result32 = (uint32_t)reg - (uint32_t)value - carry;
        uint16_t result = result32 & 0xFFFF;
        flags.assign(Flags::N)
            .update(Flags::S, (result & 0x8000) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (reg & 0x0FFF) < ((value & 0x0FFF) + carry))
            .update(Flags::PV, (((reg ^ result) & (reg ^ value)) & 0x8000) != 0)
            .update(Flags::C, result32 > 0xFFFF)
            .update(Flags::Y, (result & 0x2000) != 0)
            .update(Flags::X, (result & 0x0800) != 0);
        set_F(flags);
        return result;
    }
    uint8_t rlc_8bit(uint8_t value) {
        uint8_t result = (value << 1) | (value >> 7);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x80) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t rrc_8bit(uint8_t value) {
        uint8_t result = (value >> 1) | (value << 7);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x01) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t rl_8bit(uint8_t value) {
        Flags flags = get_F();
        uint8_t result = (value << 1) | (flags.is_set(Flags::C) ? 1 : 0);
        flags.zero()
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x80) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t rr_8bit(uint8_t value) {
        Flags flags = get_F();
        uint8_t result = (value >> 1) | (flags.is_set(Flags::C) ? 0x80 : 0);
        flags.zero()
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x01) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t sla_8bit(uint8_t value) {
        uint8_t result = value << 1;
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x80) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t sra_8bit(uint8_t value) {
        uint8_t result = (value >> 1) | (value & 0x80);
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x01) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t sll_8bit(uint8_t value) {
        uint8_t result = (value << 1) | 0x01;
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x80) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    uint8_t srl_8bit(uint8_t value) {
        uint8_t result = value >> 1;
        Flags flags(0);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::C, (value & 0x01) != 0)
            .update(Flags::Y, (result & 0x20) != 0)
            .update(Flags::X, (result & 0x08) != 0);
        set_F(flags);
        return result;
    }
    void bit_8bit(uint8_t bit, uint8_t value) {
        bool bit_is_zero = (value & (1 << bit)) == 0;
        Flags flags = get_F();
        flags.set(Flags::H)
            .clear(Flags::N)
            .update(Flags::Z, bit_is_zero)
            .update(Flags::PV, bit_is_zero)
            .update(Flags::S, bit == 7 && !bit_is_zero);
        set_F(flags);
    }
    uint8_t res_8bit(uint8_t bit, uint8_t value) {
        return value & ~(1 << bit);
    }
    uint8_t set_8bit(uint8_t bit, uint8_t value) {
        return value | (1 << bit);
    }

    //Input and output helpers
    uint8_t in_r_c() {
        uint8_t value = m_io.read(get_BC());
        Flags flags = get_F();
        flags.update(Flags::S, (value & 0x80) != 0)
            .update(Flags::Z, value == 0)
            .clear(Flags::H | Flags::N)
            .update(Flags::PV, is_parity_even(value))
            .update(Flags::X, (value & Flags::X) != 0)
            .update(Flags::Y, (value & Flags::Y) != 0);
        set_F(flags);
        return value;
    }
    void out_c_r(uint8_t value) { m_io.write(get_BC(), value); }

    //Interrupt handling
    void handle_NMI() {
        add_operation_ticks(5);
        set_halted(false);
        set_IFF2(get_IFF1());
        set_IFF1(false);
        push_word(get_PC());
        set_PC(0x0066);
        set_NMI_pending(false);
    }
    void handle_interrupt() {
        add_operation_ticks(7);
        set_IFF2(get_IFF1());
        set_IFF1(false);
        push_word(get_PC());
        switch (get_IRQ_mode()) {
            case 0: {
                uint8_t opcode = get_IRQ_data();
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
                uint16_t vector_address = (static_cast<uint16_t>(get_I()) << 8) | get_IRQ_data();
                uint16_t handler_address = read_word(vector_address);
                set_PC(handler_address);
                break;
            }
        }
        set_IRQ_request(false);
    }

    //Opcodes handling
    void handle_CB_opcodes() {
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
                Flags flags = get_F();
                if (target_reg == 6) {
                    add_operation_ticks(1);
                    flags.update(Flags::X, (flags_source & 0x0800) != 0);
                    flags.update(Flags::Y, (flags_source & 0x2000) != 0);
                } else {
                    flags.update(Flags::X, (value & 0x08) != 0);
                    flags.update(Flags::Y, (value & 0x20) != 0);
                }
                set_F(flags);
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
                add_operation_ticks(1);
                write_byte(get_HL(), result);
                break;
            case 7: set_A(result); break;
        }
    }
    void handle_CB_indexed_opcodes(uint16_t index_register) {
        add_operation_ticks(2);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint8_t opcode = fetch_next_byte();
        uint16_t address = index_register + offset;
        uint8_t value = read_byte(address);
        uint8_t operation_group = opcode >> 6;
        uint8_t bit = (opcode >> 3) & 0x07;
        uint8_t result = value;
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
            
            case 1: {
                add_operation_ticks(1);
                bit_8bit(bit, value);
                Flags flags = get_F();
                flags.update(Flags::X, (address & 0x0800) != 0);
                flags.update(Flags::Y, (address & 0x2000) != 0);
                set_F(flags);
                return;
            }
            case 2: result = res_8bit(bit, value); break;
            case 3: result = set_8bit(bit, value); break;
        }
        add_operation_ticks(1);
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
    void opcode_0x00_NOP() {
    }
    void opcode_0x01_LD_BC_nn() {
        set_BC(fetch_next_word());
    }
    void opcode_0x02_LD_BC_ptr_A() {
        write_byte(get_BC(), get_A());
    }
    void opcode_0x03_INC_BC() {
        add_operation_ticks(2);
        set_BC(get_BC() + 1);
    }
    void opcode_0x04_INC_B() {
        set_B(inc_8bit(get_B()));
    }
    void opcode_0x05_DEC_B() {
        set_B(dec_8bit(get_B()));
    }
    void opcode_0x06_LD_B_n() {
        set_B(fetch_next_byte());
    }
    void opcode_0x07_RLCA() {
        uint8_t value = get_A();
        uint8_t carry_bit = (value >> 7) & 0x01;
        uint8_t result = (value << 1) | carry_bit;
        set_A(result);
        Flags flags = get_F();
        flags.update(Flags::C, carry_bit == 1)
            .clear(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void opcode_0x08_EX_AF_AFp() {
        uint16_t temp_AF = get_AF();
        set_AF(get_AFp());
        set_AFp(temp_AF);
    }
    void opcode_0x09_ADD_HL_BC() {
        add_operation_ticks(7);
        set_indexed_HL(add_16bit(get_indexed_HL(), get_BC()));
    }
    void opcode_0x0A_LD_A_BC_ptr() {
        set_A(read_byte(get_BC()));
    }
    void opcode_0x0B_DEC_BC() {
        add_operation_ticks(2);
        set_BC(get_BC() - 1);
    }
    void opcode_0x0C_INC_C() {
        set_C(inc_8bit(get_C()));
    }
    void opcode_0x0D_DEC_C() {
        set_C(dec_8bit(get_C()));
    }
    void opcode_0x0E_LD_C_n() {
        set_C(fetch_next_byte());
    }
    void opcode_0x0F_RRCA() {
        uint8_t value = get_A();
        uint8_t carry_bit = value & 0x01;
        uint8_t result = (value >> 1) | (carry_bit << 7);
        set_A(result);
        Flags flags = get_F();
        flags.update(Flags::C, carry_bit == 1)
            .clear(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void opcode_0x10_DJNZ_d() {
        add_operation_ticks(1);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint8_t new_b_value = get_B() - 1;
        set_B(new_b_value);
        if (new_b_value != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() + offset);
        }
    }
    void opcode_0x11_LD_DE_nn() {
        set_DE(fetch_next_word());
    }
    void opcode_0x12_LD_DE_ptr_A() {
        write_byte(get_DE(), get_A());
    }
    void opcode_0x13_INC_DE() {
        add_operation_ticks(2);
        set_DE(get_DE() + 1);
    }
    void opcode_0x14_INC_D() {
        set_D(inc_8bit(get_D()));
    }
    void opcode_0x15_DEC_D() {
        set_D(dec_8bit(get_D()));
    }
    void opcode_0x16_LD_D_n() {
        set_D(fetch_next_byte());
    }
    void opcode_0x17_RLA() {
        uint8_t value = get_A();
        uint8_t old_carry_bit = get_F().is_set(Flags::C) ? 1 : 0;
        uint8_t new_carry_bit = (value >> 7) & 0x01;
        uint8_t result = (value << 1) | old_carry_bit;
        set_A(result);
        Flags flags = get_F();
        flags.update(Flags::C, new_carry_bit == 1)
            .clear(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void opcode_0x18_JR_d() {
        add_operation_ticks(5);
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        set_PC(get_PC() + offset);
    }
    void opcode_0x19_ADD_HL_DE() {
        add_operation_ticks(7);
        set_indexed_HL(add_16bit(get_indexed_HL(), get_DE()));
    }
    void opcode_0x1A_LD_A_DE_ptr() {
        set_A(read_byte(get_DE()));
    }
    void opcode_0x1B_DEC_DE() {
        add_operation_ticks(2);
        set_DE(get_DE() - 1);
    }
    void opcode_0x1C_INC_E() {
        set_E(inc_8bit(get_E()));
    }
    void opcode_0x1D_DEC_E() {
        set_E(dec_8bit(get_E()));
    }
    void opcode_0x1E_LD_E_n() {
        set_E(fetch_next_byte());
    }
    void opcode_0x1F_RRA() {
        uint8_t value = get_A();
        bool old_carry_bit = get_F().is_set(Flags::C);
        bool new_carry_bit = (value & 0x01) != 0;
        uint8_t result = (value >> 1) | (old_carry_bit ? 0x80 : 0);
        set_A(result);
        Flags flags = get_F();
        flags.update(Flags::C, new_carry_bit)
            .clear(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void opcode_0x20_JR_NZ_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        if (!get_F().is_set(Flags::Z)) {
            add_operation_ticks(5);
            set_PC(get_PC() + offset);
        }
    }
    void opcode_0x21_LD_HL_nn() {
        set_indexed_HL(fetch_next_word());
    }
    void opcode_0x22_LD_nn_ptr_HL() {
        write_word(fetch_next_word(), get_indexed_HL());
    }
    void opcode_0x23_INC_HL() {
        add_operation_ticks(2);
        set_indexed_HL(get_indexed_HL() + 1);
    }
    void opcode_0x24_INC_H() {
        set_indexed_H(inc_8bit(get_indexed_H()));
    }
    void opcode_0x25_DEC_H() {
        set_indexed_H(dec_8bit(get_indexed_H()));
    }
    void opcode_0x26_LD_H_n() {
        set_indexed_H(fetch_next_byte());
    }
    void opcode_0x27_DAA() {
        uint8_t a = get_A();
        uint8_t correction = 0;
        Flags flags = get_F();
        bool carry = flags.is_set(Flags::C);
        if (flags.is_set(Flags::N)) {
            if (carry || (a > 0x99))
                correction = 0x60;
            if (flags.is_set(Flags::H) || ((a & 0x0F) > 0x09)) 
                correction |= 0x06;
            set_A(a - correction);
            flags.update(Flags::H, flags.is_set(Flags::H) && ((a & 0x0F) < 0x06));
        } else {
            if (carry || (a > 0x99)) {
                correction = 0x60;
                flags.set(Flags::C);
            }
            if (flags.is_set(Flags::H) || ((a & 0x0F) > 0x09))
                correction |= 0x06;
            set_A(a + correction);
            flags.update(Flags::H, (a & 0x0F) > 0x09);
        }
        if (correction >= 0x60)
            flags.set(Flags::C);
        uint8_t result = get_A();
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::PV, is_parity_even(result))
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0x28_JR_Z_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        if (get_F().is_set(Flags::Z)) {
            add_operation_ticks(5);
            set_PC(get_PC() + offset);
        }
    }
    void opcode_0x29_ADD_HL_HL() {
        add_operation_ticks(7);
        set_indexed_HL(add_16bit(get_indexed_HL(), get_indexed_HL()));
    }
    void opcode_0x2A_LD_HL_nn_ptr() {
        set_indexed_HL(read_word(fetch_next_word()));
    }
    void opcode_0x2B_DEC_HL() {
        add_operation_ticks(2);
        set_indexed_HL(get_indexed_HL() - 1);
    }
    void opcode_0x2C_INC_L() {
        set_indexed_L(inc_8bit(get_indexed_L()));
    }
    void opcode_0x2D_DEC_L() {
        set_indexed_L(dec_8bit(get_indexed_L()));
    }
    void opcode_0x2E_LD_L_n() {
        set_indexed_L(fetch_next_byte());
    }
    void opcode_0x2F_CPL() {
        uint8_t result = ~get_A();
        set_A(result);
        Flags flags = get_F();
        flags.set(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void opcode_0x30_JR_NC_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        if (!get_F().is_set(Flags::C)) {
            add_operation_ticks(5);
            set_PC(get_PC() + offset);
        }
    }
    void opcode_0x31_LD_SP_nn() {
        set_SP(fetch_next_word());
    }
    void opcode_0x32_LD_nn_ptr_A() {
        uint16_t address = fetch_next_word();
        write_byte(address, get_A());
    }
    void opcode_0x33_INC_SP() {
        add_operation_ticks(2);
        set_SP(get_SP() + 1);
    }
    void opcode_0x34_INC_HL_ptr() {
        add_operation_ticks(1);
        uint16_t address = get_indexed_address();
        write_byte(address, inc_8bit(read_byte(address)));
    }
    void opcode_0x35_DEC_HL_ptr() {
        add_operation_ticks(1);
        uint16_t address = get_indexed_address();
        write_byte(address, dec_8bit(read_byte(address)));
    }
    void opcode_0x36_LD_HL_ptr_n() {
        if (get_index_mode() == IndexMode::HL) {
            uint8_t value = fetch_next_byte();
            write_byte(get_HL(), value);
        } else {
            add_operation_ticks(2);
            int8_t offset = static_cast<int8_t>(fetch_next_byte());
            uint8_t value = fetch_next_byte();
            uint16_t address = (get_index_mode() == IndexMode::IX ? get_IX() : get_IY()) + offset;
            write_byte(address, value);
        }
    }
    void opcode_0x37_SCF() {
        Flags flags = get_F();
        flags.set(Flags::C)
            .clear(Flags::H | Flags::N)
            .update(Flags::X, (get_A() & Flags::X) != 0)
            .update(Flags::Y, (get_A() & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0x38_JR_C_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        if (get_F().is_set(Flags::C)) {
            add_operation_ticks(5);
            set_PC(get_PC() + offset);
        }
    }
    void opcode_0x39_ADD_HL_SP() {
        add_operation_ticks(7);
        set_indexed_HL(add_16bit(get_indexed_HL(), get_SP()));
    }
    void opcode_0x3A_LD_A_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_A(read_byte(address));
    }
    void opcode_0x3B_DEC_SP() {
        add_operation_ticks(2);
        set_SP(get_SP() - 1);
    }
    void opcode_0x3C_INC_A() {
        set_A(inc_8bit(get_A()));
    }
    void opcode_0x3D_DEC_A() {
        set_A(dec_8bit(get_A()));
    }
    void opcode_0x3E_LD_A_n() {
        set_A(fetch_next_byte());
    }
    void opcode_0x3F_CCF() {
        Flags flags = get_F();
        bool old_carry = flags.is_set(Flags::C);
        flags.update(Flags::C, !old_carry)
            .update(Flags::H, old_carry)
            .clear(Flags::N)
            .update(Flags::X, (get_A() & Flags::X) != 0)
            .update(Flags::Y, (get_A() & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0x40_LD_B_B() {
    }
    void opcode_0x41_LD_B_C() {
        set_B(get_C());
    }
    void opcode_0x42_LD_B_D() {
        set_B(get_D());
    }
    void opcode_0x43_LD_B_E() {
        set_B(get_E());
    }
    void opcode_0x44_LD_B_H() {
        set_B(get_indexed_H());
    }
    void opcode_0x45_LD_B_L() {
        set_B(get_indexed_L());
    }
    void opcode_0x46_LD_B_HL_ptr() {
        set_B(get_indexed_HL_ptr());
    }
    void opcode_0x47_LD_B_A() {
        set_B(get_A());
    }
    void opcode_0x48_LD_C_B() {
        set_C(get_B());
    }
    void opcode_0x49_LD_C_C() {
    }
    void opcode_0x4A_LD_C_D() {
        set_C(get_D());
    }
    void opcode_0x4B_LD_C_E() {
        set_C(get_E());
    }
    void opcode_0x4C_LD_C_H() {
        set_C(get_indexed_H());
    }
    void opcode_0x4D_LD_C_L() {
        set_C(get_indexed_L());
    }
    void opcode_0x4E_LD_C_HL_ptr() {
        set_C(get_indexed_HL_ptr());
    }
    void opcode_0x4F_LD_C_A() {
        set_C(get_A());
    }
    void opcode_0x50_LD_D_B() {
        set_D(get_B());
    }
    void opcode_0x51_LD_D_C() {
        set_D(get_C());
    }
    void opcode_0x52_LD_D_D() {
    }
    void opcode_0x53_LD_D_E() {
        set_D(get_E());
    }
    void opcode_0x54_LD_D_H() {
        set_D(get_indexed_H());
    }
    void opcode_0x55_LD_D_L() {
        set_D(get_indexed_L());
    }
    void opcode_0x56_LD_D_HL_ptr() {
        set_D(get_indexed_HL_ptr());
    }
    void opcode_0x57_LD_D_A() {
        set_D(get_A());
    }
    void opcode_0x58_LD_E_B() {
        set_E(get_B());
    }
    void opcode_0x59_LD_E_C() {
        set_E(get_C());
    }
    void opcode_0x5A_LD_E_D() {
        set_E(get_D());
    }
    void opcode_0x5B_LD_E_E() {
    }
    void opcode_0x5C_LD_E_H() {
        set_E(get_indexed_H());
    }
    void opcode_0x5D_LD_E_L() {
        set_E(get_indexed_L());
    }
    void opcode_0x5E_LD_E_HL_ptr() {
        set_E(get_indexed_HL_ptr());
    }
    void opcode_0x5F_LD_E_A() {
        set_E(get_A());
    }
    void opcode_0x60_LD_H_B() {
        set_indexed_H(get_B());
    }
    void opcode_0x61_LD_H_C() {
        set_indexed_H(get_C());
    }
    void opcode_0x62_LD_H_D() {
        set_indexed_H(get_D());
    }
    void opcode_0x63_LD_H_E() {
        set_indexed_H(get_E());
    }
    void opcode_0x64_LD_H_H() {
    }
    void opcode_0x65_LD_H_L() {
        set_indexed_H(get_indexed_L());
    }
    void opcode_0x66_LD_H_HL_ptr() {
        set_H(get_indexed_HL_ptr());
    }
    void opcode_0x67_LD_H_A() {
        set_indexed_H(get_A());
    }
    void opcode_0x68_LD_L_B() {
        set_indexed_L(get_B());
    }
    void opcode_0x69_LD_L_C() {
        set_indexed_L(get_C());
    }
    void opcode_0x6A_LD_L_D() {
        set_indexed_L(get_D());
    }
    void opcode_0x6B_LD_L_E() {
        set_indexed_L(get_E());
    }
    void opcode_0x6C_LD_L_H() {
        set_indexed_L(get_indexed_H());
    }
    void opcode_0x6D_LD_L_L() {
    }
    void opcode_0x6E_LD_L_HL_ptr() {
        set_L(get_indexed_HL_ptr());
    }
    void opcode_0x6F_LD_L_A() {
        set_indexed_L(get_A());
    }
    void opcode_0x70_LD_HL_ptr_B() {
        set_indexed_HL_ptr(get_B());
    }
    void opcode_0x71_LD_HL_ptr_C() {
        set_indexed_HL_ptr(get_C());
    }
    void opcode_0x72_LD_HL_ptr_D() {
        set_indexed_HL_ptr(get_D());
    }
    void opcode_0x73_LD_HL_ptr_E() {
        set_indexed_HL_ptr(get_E());
    }
    void opcode_0x74_LD_HL_ptr_H() {
        set_indexed_HL_ptr(get_H());
    }
    void opcode_0x75_LD_HL_ptr_L() {
        set_indexed_HL_ptr(get_L());
    }
    void opcode_0x76_HALT() {
        set_halted(true);
    }
    void opcode_0x77_LD_HL_ptr_A() {
        set_indexed_HL_ptr(get_A());
    }
    void opcode_0x78_LD_A_B() {
        set_A(get_B());
    }
    void opcode_0x79_LD_A_C() {
        set_A(get_C());
    }
    void opcode_0x7A_LD_A_D() {
        set_A(get_D());
    }
    void opcode_0x7B_LD_A_E() {
        set_A(get_E());
    }
    void opcode_0x7C_LD_A_H() {
        set_A(get_indexed_H());
    }
    void opcode_0x7D_LD_A_L() {
        set_A(get_indexed_L());
    }
    void opcode_0x7E_LD_A_HL_ptr() {
        set_A(get_indexed_HL_ptr());
    }
    void opcode_0x7F_LD_A_A() {
    }
    void opcode_0x80_ADD_A_B() {
        add_8bit(get_B());
    }
    void opcode_0x81_ADD_A_C() {
        add_8bit(get_C());
    }
    void opcode_0x82_ADD_A_D() {
        add_8bit(get_D());
    }
    void opcode_0x83_ADD_A_E() {
        add_8bit(get_E());
    }
    void opcode_0x84_ADD_A_H() {
        add_8bit(get_indexed_H());
    }
    void opcode_0x85_ADD_A_L() {
        add_8bit(get_indexed_L());
    }
    void opcode_0x86_ADD_A_HL_ptr() {
        add_8bit(get_indexed_HL_ptr());
    }
    void opcode_0x87_ADD_A_A() {
        add_8bit(get_A());
    }
    void opcode_0x88_ADC_A_B() {
        adc_8bit(get_B());
    }
    void opcode_0x89_ADC_A_C() {
        adc_8bit(get_C());
    }
    void opcode_0x8A_ADC_A_D() {
        adc_8bit(get_D());
    }
    void opcode_0x8B_ADC_A_E() {
        adc_8bit(get_E());
    }
    void opcode_0x8C_ADC_A_H() {
        adc_8bit(get_indexed_H());
    }
    void opcode_0x8D_ADC_A_L() {
        adc_8bit(get_indexed_L());
    }
    void opcode_0x8E_ADC_A_HL_ptr() {
        adc_8bit(get_indexed_HL_ptr());
    }
    void opcode_0x8F_ADC_A_A() {
        adc_8bit(get_A());
    }
    void opcode_0x90_SUB_B() {
        sub_8bit(get_B());
    }
    void opcode_0x91_SUB_C() {
        sub_8bit(get_C());
    }
    void opcode_0x92_SUB_D() {
        sub_8bit(get_D());
    }
    void opcode_0x93_SUB_E() {
        sub_8bit(get_E());
    }
    void opcode_0x94_SUB_H() {
        sub_8bit(get_indexed_H());
    }
    void opcode_0x95_SUB_L() {
        sub_8bit(get_indexed_L());
    }
    void opcode_0x96_SUB_HL_ptr() {
        sub_8bit(get_indexed_HL_ptr());
    }
    void opcode_0x97_SUB_A() {
        sub_8bit(get_A());
    }
    void opcode_0x98_SBC_A_B() {
        sbc_8bit(get_B());
    }
    void opcode_0x99_SBC_A_C() {
        sbc_8bit(get_C());
    }
    void opcode_0x9A_SBC_A_D() {
        sbc_8bit(get_D());
    }
    void opcode_0x9B_SBC_A_E() {
        sbc_8bit(get_E());
    }
    void opcode_0x9C_SBC_A_H() {
        sbc_8bit(get_indexed_H());
    }
    void opcode_0x9D_SBC_A_L() {
        sbc_8bit(get_indexed_L());
    }
    void opcode_0x9E_SBC_A_HL_ptr() {
        sbc_8bit(get_indexed_HL_ptr());
    }
    void opcode_0x9F_SBC_A_A() {
        sbc_8bit(get_A());
    }
    void opcode_0xA0_AND_B() {
        and_8bit(get_B());
    }
    void opcode_0xA1_AND_C() {
        and_8bit(get_C());
    }
    void opcode_0xA2_AND_D() {
        and_8bit(get_D());
    }
    void opcode_0xA3_AND_E() {
        and_8bit(get_E());
    }
    void opcode_0xA4_AND_H() {
        and_8bit(get_indexed_H());
    }
    void opcode_0xA5_AND_L() {
        and_8bit(get_indexed_L());
    }
    void opcode_0xA6_AND_HL_ptr() {
        and_8bit(get_indexed_HL_ptr());
    }
    void opcode_0xA7_AND_A() {
        and_8bit(get_A());
    }
    void opcode_0xA8_XOR_B() {
        xor_8bit(get_B());
    }
    void opcode_0xA9_XOR_C() {
        xor_8bit(get_C());
    }
    void opcode_0xAA_XOR_D() {
        xor_8bit(get_D());
    }
    void opcode_0xAB_XOR_E() {
        xor_8bit(get_E());
    }
    void opcode_0xAC_XOR_H() {
        xor_8bit(get_indexed_H());
    }
    void opcode_0xAD_XOR_L() {
        xor_8bit(get_indexed_L());
    }
    void opcode_0xAE_XOR_HL_ptr() {
        xor_8bit(get_indexed_HL_ptr());
    }
    void opcode_0xAF_XOR_A() {
        xor_8bit(get_A());
    }
    void opcode_0xB0_OR_B() {
        or_8bit(get_B());
    }
    void opcode_0xB1_OR_C() {
        or_8bit(get_C());
    }
    void opcode_0xB2_OR_D() {
        or_8bit(get_D());
    }
    void opcode_0xB3_OR_E() {
        or_8bit(get_E());
    }
    void opcode_0xB4_OR_H() {
        or_8bit(get_indexed_H());
    }
    void opcode_0xB5_OR_L() {
        or_8bit(get_indexed_L());
    }
    void opcode_0xB6_OR_HL_ptr() {
        or_8bit(get_indexed_HL_ptr());
    }
    void opcode_0xB7_OR_A() {
        or_8bit(get_A());
    }
    void opcode_0xB8_CP_B() {
        cp_8bit(get_B());
    }
    void opcode_0xB9_CP_C() {
        cp_8bit(get_C());
    }
    void opcode_0xBA_CP_D() {
        cp_8bit(get_D());
    }
    void opcode_0xBB_CP_E() {
        cp_8bit(get_E());
    }
    void opcode_0xBC_CP_H() {
        cp_8bit(get_indexed_H());
    }
    void opcode_0xBD_CP_L() {
        cp_8bit(get_indexed_L());
    }
    void opcode_0xBE_CP_HL_ptr() {
        cp_8bit(get_indexed_HL_ptr());
    }
    void opcode_0xBF_CP_A() {
        cp_8bit(get_A());
    }
    void opcode_0xC0_RET_NZ() {
        add_operation_ticks(1);
        if (!get_F().is_set(Flags::Z))
            set_PC(pop_word());
    }
    void opcode_0xC1_POP_BC() {
        set_BC(pop_word());
    }
    void opcode_0xC2_JP_NZ_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::Z))
            set_PC(address);
    }
    void opcode_0xC3_JP_nn() {
        set_PC(fetch_next_word());
    }
    void opcode_0xC4_CALL_NZ_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::Z)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xC5_PUSH_BC() {
        add_operation_ticks(1);
        push_word(get_BC());
    }
    void opcode_0xC6_ADD_A_n() {
        add_8bit(fetch_next_byte());
    }
    void opcode_0xC7_RST_00H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0000);
    }
    void opcode_0xC8_RET_Z() {
        add_operation_ticks(1);
        if (get_F().is_set(Flags::Z))
            set_PC(pop_word());
    }
    void opcode_0xC9_RET() {
        set_PC(pop_word());
    }
    void opcode_0xCA_JP_Z_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::Z))
            set_PC(address);
    }
    void opcode_0xCC_CALL_Z_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::Z)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xCD_CALL_nn() {
        add_operation_ticks(1);
        uint16_t address = fetch_next_word();
        push_word(get_PC());
        set_PC(address);
    }
    void opcode_0xCE_ADC_A_n() {
        adc_8bit(fetch_next_byte());
    }
    void opcode_0xCF_RST_08H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0008);
    }
    void opcode_0xD0_RET_NC() {
        add_operation_ticks(1);
        if (!get_F().is_set(Flags::C))
            set_PC(pop_word());
    }
    void opcode_0xD1_POP_DE() {
        set_DE(pop_word());
    }
    void opcode_0xD2_JP_NC_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::C))
            set_PC(address);
    }
    void opcode_0xD3_OUT_n_ptr_A() {
        add_operation_ticks(4);
        uint8_t port_lo = fetch_next_byte();
        m_io.write((get_A() << 8) | port_lo, get_A());
    }
    void opcode_0xD4_CALL_NC_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::C)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xD5_PUSH_DE() {
        add_operation_ticks(1);
        push_word(get_DE());
    }
    void opcode_0xD6_SUB_n() {
        sub_8bit(fetch_next_byte());
    }
    void opcode_0xD7_RST_10H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0010);
    }
    void opcode_0xD8_RET_C() {
        add_operation_ticks(1);
        if (get_F().is_set(Flags::C))
            set_PC(pop_word());
    }
    void opcode_0xD9_EXX() {
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
    void opcode_0xDA_JP_C_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::C))
            set_PC(address);
    }
    void opcode_0xDB_IN_A_n_ptr() {
        add_operation_ticks(4);
        uint8_t port_lo = fetch_next_byte();
        uint16_t port = (get_A() << 8) | port_lo;
        set_A(m_io.read(port));
    }
    void opcode_0xDC_CALL_C_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::C)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xDE_SBC_A_n() {
        sbc_8bit(fetch_next_byte());
    }
    void opcode_0xDF_RST_18H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0018);
    }
    void opcode_0xE0_RET_PO() {
        add_operation_ticks(1);
        if (!get_F().is_set(Flags::PV))
            set_PC(pop_word());
    }
    void opcode_0xE1_POP_HL() {
        set_indexed_HL(pop_word());
    }
    void opcode_0xE2_JP_PO_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::PV))
            set_PC(address);
    }
    void opcode_0xE3_EX_SP_ptr_HL() {
        add_operation_ticks(3);
        uint16_t from_stack = read_word(get_SP());
        write_word(get_SP(), get_indexed_HL());
        set_indexed_HL(from_stack);
    }
    void opcode_0xE4_CALL_PO_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::PV)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xE5_PUSH_HL() {
        add_operation_ticks(1);
        push_word(get_indexed_HL());
    }
    void opcode_0xE6_AND_n() {
        and_8bit(fetch_next_byte());
    }
    void opcode_0xE7_RST_20H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0020);
    }
    void opcode_0xE8_RET_PE() {
        add_operation_ticks(1);
        if (get_F().is_set(Flags::PV))
            set_PC(pop_word());
    }
    void opcode_0xE9_JP_HL_ptr() {
        set_PC(get_indexed_HL());
    }
    void opcode_0xEA_JP_PE_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::PV))
            set_PC(address);
    }
    void opcode_0xEB_EX_DE_HL() {
        uint16_t temp = get_HL();
        set_HL(get_DE());
        set_DE(temp);
    }
    void opcode_0xEC_CALL_PE_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::PV)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xEE_XOR_n() {
        xor_8bit(fetch_next_byte());
    }
    void opcode_0xEF_RST_28H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0028);
    }
    void opcode_0xF0_RET_P() {
        add_operation_ticks(1);
        if (!get_F().is_set(Flags::S))
            set_PC(pop_word());
    }
    void opcode_0xF1_POP_AF() {
        set_AF(pop_word());
    }
    void opcode_0xF2_JP_P_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::S))
            set_PC(address);
    }
    void opcode_0xF3_DI() {
        set_IFF1(false);
        set_IFF2(false);
    }
    void opcode_0xF4_CALL_P_nn() {
        uint16_t address = fetch_next_word();
        if (!get_F().is_set(Flags::S)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xF5_PUSH_AF() {
        add_operation_ticks(1);
        push_word(get_AF());
    }
    void opcode_0xF6_OR_n() {
        or_8bit(fetch_next_byte());
    }
    void opcode_0xF7_RST_30H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0030);
    }
    void opcode_0xF8_RET_M() {
        add_operation_ticks(1);
        if (get_F().is_set(Flags::S))
            set_PC(pop_word());
    }
    void opcode_0xF9_LD_SP_HL() {
        add_operation_ticks(2);
        set_SP(get_indexed_HL());
    }
    void opcode_0xFA_JP_M_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::S))
            set_PC(address);
    }
    void opcode_0xFB_EI() {
        set_EI_delay(true);
    }
    void opcode_0xFC_CALL_M_nn() {
        uint16_t address = fetch_next_word();
        if (get_F().is_set(Flags::S)) {
            add_operation_ticks(1);
            push_word(get_PC());
            set_PC(address);
        }
    }
    void opcode_0xFE_CP_n() {
        cp_8bit(fetch_next_byte());
    }
    void opcode_0xFF_RST_38H() {
        add_operation_ticks(1);
        push_word(get_PC());
        set_PC(0x0038);
    }
    void opcode_0xED_0x40_IN_B_C_ptr() {
        add_operation_ticks(4);
        set_B(in_r_c());
    }
    void opcode_0xED_0x41_OUT_C_ptr_B() {
        add_operation_ticks(4);
        out_c_r(get_B());
    }
    void opcode_0xED_0x42_SBC_HL_BC() {
        add_operation_ticks(7);
        set_indexed_HL(sbc_16bit(get_indexed_HL(), get_BC()));
    }
    void opcode_0xED_0x43_LD_nn_ptr_BC() {
        write_word(fetch_next_word(), get_BC());
    }
    void opcode_0xED_0x44_NEG() {
        uint8_t value = get_A();
        uint8_t result = -value;
        set_A(result);
        Flags flags(Flags::N);
        flags.update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, (value & 0x0F) != 0)
            .update(Flags::C, value != 0)
            .update(Flags::PV, value == 0x80)
            .update(Flags::X, (result & Flags::X) != 0)
            .update(Flags::Y, (result & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0xED_0x45_RETN() {
        set_PC(pop_word());
        set_IFF1(get_IFF2());
    }
    void opcode_0xED_0x46_IM_0() {
        set_IRQ_mode(0);
    }
    void opcode_0xED_0x47_LD_I_A() {
        add_operation_ticks(1);
        set_I(get_A());
    }
    void opcode_0xED_0x48_IN_C_C_ptr() {
        add_operation_ticks(4);
        set_C(in_r_c());
    }
    void opcode_0xED_0x49_OUT_C_ptr_C() {
        add_operation_ticks(4);
        out_c_r(get_C());
    }
    void opcode_0xED_0x4A_ADC_HL_BC() {
        add_operation_ticks(7);
        set_indexed_HL(adc_16bit(get_indexed_HL(), get_BC()));
    }
    void opcode_0xED_0x4B_LD_BC_nn_ptr() {
        set_BC(read_word(fetch_next_word()));
    }
    void opcode_0xED_0x4D_RETI() {
        set_PC(pop_word());
        set_IFF1(get_IFF2());
        set_RETI_signaled(true);
    }
    void opcode_0xED_0x4F_LD_R_A() {
        add_operation_ticks(1);
        set_R(get_A());
    }
    void opcode_0xED_0x50_IN_D_C_ptr() {
        add_operation_ticks(4);
        set_D(in_r_c());
    }
    void opcode_0xED_0x51_OUT_C_ptr_D() {
        add_operation_ticks(4);
        out_c_r(get_D());
    }
    void opcode_0xED_0x52_SBC_HL_DE() {
        add_operation_ticks(7);
        set_indexed_HL(sbc_16bit(get_indexed_HL(), get_DE()));
    }
    void opcode_0xED_0x53_LD_nn_ptr_DE() {
        write_word(fetch_next_word(), get_DE());
    }
    void opcode_0xED_0x56_IM_1() {
        set_IRQ_mode(1);
    }
    void opcode_0xED_0x57_LD_A_I() {
        add_operation_ticks(1);
        uint8_t i_value = get_I();
        set_A(i_value);
        Flags flags(get_F() & Flags::C);
        flags.clear(Flags::H | Flags::N)
            .update(Flags::S, (i_value & 0x80) != 0)
            .update(Flags::Z, i_value == 0)
            .update(Flags::PV, get_IFF2())
            .update(Flags::X, (i_value & Flags::X) != 0)
            .update(Flags::Y, (i_value & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0xED_0x58_IN_E_C_ptr() {
        add_operation_ticks(4);
        set_E(in_r_c());
    }
    void opcode_0xED_0x59_OUT_C_ptr_E() {
        add_operation_ticks(4);
        out_c_r(get_E());
    }
    void opcode_0xED_0x5A_ADC_HL_DE() {
        add_operation_ticks(7);
        set_indexed_HL(adc_16bit(get_indexed_HL(), get_DE()));
    }
    void opcode_0xED_0x5B_LD_DE_nn_ptr() {
        set_DE(read_word(fetch_next_word()));
    }
    void opcode_0xED_0x5E_IM_2() {
        set_IRQ_mode(2);
    }
    void opcode_0xED_0x5F_LD_A_R() {
        add_operation_ticks(1);
        uint8_t r_value = get_R();
        set_A(r_value);
        Flags flags(get_F() & Flags::C);
        flags.clear(Flags::H | Flags::N)
            .update(Flags::S, (r_value & 0x80) != 0)
            .update(Flags::Z, r_value == 0)
            .update(Flags::PV, get_IFF2())
            .update(Flags::X, (r_value & Flags::X) != 0)
            .update(Flags::Y, (r_value & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0xED_0x60_IN_H_C_ptr() {
        add_operation_ticks(4);
        set_H(in_r_c());
    }
    void opcode_0xED_0x61_OUT_C_ptr_H() {
        add_operation_ticks(4);
        out_c_r(get_H());
    }
    void opcode_0xED_0x62_SBC_HL_HL() {
        add_operation_ticks(7);
        uint16_t value = get_indexed_HL();
        set_indexed_HL(sbc_16bit(value, value));
    }
    void opcode_0xED_0x63_LD_nn_ptr_HL_ED() {
        write_word(fetch_next_word(), get_HL());
    }
    void opcode_0xED_0x67_RRD() {
        add_operation_ticks(4);
        uint16_t address = get_HL();
        uint8_t mem_val = read_byte(address);
        uint8_t a_val = get_A();
        uint8_t new_a = (a_val & 0xF0) | (mem_val & 0x0F);
        uint8_t new_mem = (mem_val >> 4) | ((a_val & 0x0F) << 4);
        set_A(new_a);
        write_byte(address, new_mem);
        Flags flags(get_F() & Flags::C);
        flags.clear(Flags::H | Flags::N)
            .update(Flags::S, (new_a & 0x80) != 0)
            .update(Flags::Z, new_a == 0)
            .update(Flags::PV, is_parity_even(new_a))
            .update(Flags::X, (new_a & Flags::X) != 0)
            .update(Flags::Y, (new_a & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0xED_0x68_IN_L_C_ptr() {
        add_operation_ticks(4);
        set_L(in_r_c());
    }
    void opcode_0xED_0x69_OUT_C_ptr_L() {
        add_operation_ticks(4);
        out_c_r(get_L());
    }
    void opcode_0xED_0x6A_ADC_HL_HL() {
        add_operation_ticks(7);
        uint16_t value = get_indexed_HL();
        set_indexed_HL(adc_16bit(value, value));
    }
    void opcode_0xED_0x6B_LD_HL_nn_ptr_ED() {
        set_HL(read_word(fetch_next_word()));
    }
    void opcode_0xED_0x6F_RLD() {
        add_operation_ticks(4);
        uint16_t address = get_HL();
        uint8_t mem_val = read_byte(address);
        uint8_t a_val = get_A();
        uint8_t new_a = (a_val & 0xF0) | (mem_val >> 4);
        uint8_t new_mem = (mem_val << 4) | (a_val & 0x0F);
        set_A(new_a);
        write_byte(address, new_mem);
        Flags flags(get_F() & Flags::C);
        flags.clear(Flags::H | Flags::N)
            .update(Flags::S, (new_a & 0x80) != 0)
            .update(Flags::Z, new_a == 0)
            .update(Flags::PV, is_parity_even(new_a))
            .update(Flags::X, (new_a & Flags::X) != 0)
            .update(Flags::Y, (new_a & Flags::Y) != 0);
        set_F(flags);
    }
    void opcode_0xED_0x70_IN_C_ptr() {
        add_operation_ticks(4);
        in_r_c();
    }
    void opcode_0xED_0x71_OUT_C_ptr_0() {
        add_operation_ticks(4);
        out_c_r(0x00);
    }
    void opcode_0xED_0x72_SBC_HL_SP() {
        add_operation_ticks(7);
        set_indexed_HL(sbc_16bit(get_indexed_HL(), get_SP()));
    }
    void opcode_0xED_0x73_LD_nn_ptr_SP() {
        write_word(fetch_next_word(), get_SP());
    }
    void opcode_0xED_0x78_IN_A_C_ptr() {
        add_operation_ticks(4);
        set_A(in_r_c());
    }
    void opcode_0xED_0x79_OUT_C_ptr_A() {
        add_operation_ticks(4);
        out_c_r(get_A());
    }
    void opcode_0xED_0x7A_ADC_HL_SP() {
        add_operation_ticks(7);
        set_indexed_HL(adc_16bit(get_indexed_HL(), get_SP()));
    }
    void opcode_0xED_0x7B_LD_SP_nn_ptr() {
        set_SP(read_word(fetch_next_word()));
    }
    void opcode_0xED_0xA0_LDI() {
        add_operation_ticks(2);
        uint8_t value = read_byte(get_HL());
        write_byte(get_DE(), value);
        set_HL(get_HL() + 1);
        set_DE(get_DE() + 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        uint8_t temp = get_A() + value;
        flags.clear(Flags::H | Flags::N)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
        set_F(flags);
    }
    void opcode_0xED_0xA1_CPI() {
        add_operation_ticks(5);
        uint8_t value = read_byte(get_HL());
        uint8_t result = get_A() - value;
        bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
        set_HL(get_HL() + 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
        flags.set(Flags::N)
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, half_carry)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
        set_F(flags);
    }
    void opcode_0xED_0xA2_INI() {
        add_operation_ticks(5);
        uint8_t port_val = m_io.read(get_BC());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        write_byte(get_HL(), port_val);
        set_HL(get_HL() + 1);
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_C()) + 1;
        uint8_t k = port_val + (temp & 0xFF);
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, k > 0xFF)
            .update(Flags::H, k > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ (b_val - 1)) & 0xFF));
        set_F(flags);
    }
    void opcode_0xED_0xA3_OUTI() {
        add_operation_ticks(5);
        uint8_t mem_val = read_byte(get_HL());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        m_io.write(get_BC(), mem_val);
        set_HL(get_HL() + 1);
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, temp > 0xFF)
            .update(Flags::H, temp > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
        set_F(flags);
    }
    void opcode_0xED_0xA8_LDD() {
        add_operation_ticks(2);
        uint8_t value = read_byte(get_HL());
        write_byte(get_DE(), value);
        set_HL(get_HL() - 1);
        set_DE(get_DE() - 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        uint8_t temp = get_A() + value;
        flags.clear(Flags::H | Flags::N)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
        set_F(flags);
    }
    void opcode_0xED_0xA9_CPD() {
        add_operation_ticks(5);
        uint8_t value = read_byte(get_HL());
        uint8_t result = get_A() - value;
        bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
        set_HL(get_HL() - 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        uint8_t temp = get_A() - value - (half_carry ? 1 : 0);
        flags.set(Flags::N)
            .update(Flags::S, (result & 0x80) != 0)
            .update(Flags::Z, result == 0)
            .update(Flags::H, half_carry)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
        set_F(flags);
    }
    void opcode_0xED_0xAA_IND() {
        add_operation_ticks(5);
        uint8_t port_val = m_io.read(get_BC());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        write_byte(get_HL(), port_val);
        set_HL(get_HL() - 1);
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_C()) - 1;
        uint8_t k = port_val + (temp & 0xFF);
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, k > 0xFF)
            .update(Flags::H, k > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ (b_val - 1)) & 0xFF));
        set_F(flags);
    }
    void opcode_0xED_0xAB_OUTD() {
        add_operation_ticks(5);
        uint8_t mem_val = read_byte(get_HL());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        m_io.write(get_BC(), mem_val);
        set_HL(get_HL() - 1);
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, temp > 0xFF)
            .update(Flags::H, temp > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF));
        set_F(flags);
    }
    void opcode_0xED_0xB0_LDIR() {
        opcode_0xED_0xA0_LDI();
        if (get_BC() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xB1_CPIR() {
        opcode_0xED_0xA1_CPI();
        if (get_BC() != 0 && !get_F().is_set(Flags::Z)) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xB2_INIR() {
        opcode_0xED_0xA2_INI();
        if (get_B() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xB3_OTIR() {
        opcode_0xED_0xA3_OUTI();
        if (get_B() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xB8_LDDR() {
        opcode_0xED_0xA8_LDD();
        if (get_BC() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xB9_CPDR() {
        opcode_0xED_0xA9_CPD();
        if (get_BC() != 0 && !get_F().is_set(Flags::Z)) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xBA_INDR() {
        opcode_0xED_0xAA_IND();
        if (get_B() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }
    void opcode_0xED_0xBB_OTDR() {
        opcode_0xED_0xAB_OUTD();
        if (get_B() != 0) {
            add_operation_ticks(5);
            set_PC(get_PC() - 2);
        }
    }

    //Opcodes processing
    enum class OperateMode {
        ToLimit,
        SingleStep
    };
    template<OperateMode TMode> long long operate(long long ticks_limit) {
        long long initial_ticks = get_ticks();
        while (true) { 
            set_operation_ticks(0);
            if (get_EI_delay()) {
                set_IFF1(true);
                set_IFF2(true);
                set_EI_delay(false);
            }                
            if (is_halted()) {
                if constexpr (TMode == OperateMode::SingleStep)
                    add_operation_ticks(4); 
                else 
                    add_operation_ticks(ticks_limit - get_ticks());
            }
            else {
                set_index_mode(IndexMode::HL);
                uint8_t opcode = fetch_next_opcode();
                while (opcode == 0xDD || opcode == 0xFD) {
                    set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
                    opcode = fetch_next_opcode();
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
                    case 0xCB:
                        if (Z80_LIKELY((get_index_mode() == IndexMode::HL)))
                            handle_CB_opcodes();
                        else
                            handle_CB_indexed_opcodes((get_index_mode() == IndexMode::IX) ? get_IX() : get_IY());
                        break;
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
            }
            if (is_NMI_pending())
                handle_NMI();
            else if (is_IRQ_pending())
                handle_interrupt();
            add_ticks(get_operation_ticks());
            if constexpr (TMode == OperateMode::SingleStep) {
                break;
            } else {
                if (get_ticks() >= ticks_limit)
                    break;
            }
        }
        return get_ticks() - initial_ticks;
    }
};

#endif //__Z80_H__