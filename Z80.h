#ifndef __Z80_H__
#define __Z80_H__

#include <cstdint>
#include <climits>
#include <type_traits>
#include <vector>
#include <algorithm>

#if defined(__GNUC__) || defined(__clang__) // GCC i Clang
    #define Z80_LIKELY(expr)   __builtin_expect(!!(expr), 1)
#elif __cplusplus >= 202002L // C++20
    #define Z80_LIKELY(expr)   (expr) [[likely]]
#else
    #define Z80_LIKELY(expr)   (expr)
#endif

class Z80DefaultBus;
class Z80DefaultEvents;
class Z80DefaultDebugger;
template <typename TBus = Z80DefaultBus, typename TEvents = Z80DefaultEvents, typename TDebugger = Z80DefaultDebugger>
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
        Register m_WZ;
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

    // Constructors
    Z80(TBus* bus = nullptr, TEvents* events = nullptr, TDebugger* debugger = nullptr) {
        if (bus) {
            m_bus = bus;
            m_owns_bus = false;
        } else {
            m_bus = new TBus();
            m_owns_bus = true;
        }
        if (events) {
            m_events = events;
            m_owns_events = false;
        } else {
            m_events = new TEvents();
            m_owns_events = true;
        }
        if (debugger) {
            m_debugger = debugger;
            m_owns_debugger = false;
        } else {
            m_debugger = new TDebugger();
            m_owns_debugger = true;
        }
        precompute_parity();
        m_bus->connect(this);
        m_events->connect(this);
        m_debugger->connect(this);
        reset();
    }
    ~Z80() {
        if (m_owns_bus) delete m_bus;
        if (m_owns_events) delete m_events;
        if (m_owns_debugger) delete m_debugger;
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
        set_WZ(0);
        set_IRQ_request(false);
        set_EI_delay(false);
        set_RETI_signaled(false);
        set_IRQ_data(0);
        set_IRQ_mode(0);
        set_ticks(0);
        set_index_mode(IndexMode::HL);
        m_bus->reset();
        m_events->reset();
        m_debugger->reset();
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
        state.m_WZ = get_WZ();
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
        set_WZ(state.m_WZ);
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
        set_RETI_signaled(state.m_RETI_signaled);
    }

    // Cycle counter
    long long get_ticks() const { return m_ticks; }
    void set_ticks(long long value) { m_ticks = value; }
    void add_tick() {
        if constexpr (std::is_same_v<TEvents, Z80DefaultEvents>) {
            ++m_ticks;
        } else {
            if (Z80_LIKELY(++m_ticks != m_events->get_event_limit()))
                return;
            m_events->handle_event(m_ticks);
        }
    }
    void add_ticks(long long delta) {
        if constexpr (std::is_same_v<TEvents, Z80DefaultEvents>) {
            m_ticks += delta;
        } else {
            long long target_ticks = m_ticks + delta;
            if (Z80_LIKELY(target_ticks < m_events->get_event_limit())) {
                m_ticks = target_ticks;
                return;
            }
            long long next_event;
            while ((next_event = m_events->get_event_limit()) <= target_ticks) {
                m_ticks = next_event;
                m_events->handle_event(m_ticks);
            }
            m_ticks = target_ticks;
        }
    }

    //Bus
    uint16_t get_address_bus() const { return m_address_bus; }
    void set_address_bus(uint8_t value) { m_address_bus = value; }
    uint8_t get_data_bus() const { return m_data_bus; }
    void set_data_bus(uint8_t value) { m_data_bus = value; }

    // Access to internal components
    TBus* get_bus() { return m_bus; }
    const TBus* get_bus() const { return m_bus; }
    TEvents* get_events() { return m_events; }
    const TEvents* get_events() const { return m_events; }

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

    // 16-bit internal temporary register
    uint16_t get_WZ() const { return m_WZ.w; }
    void set_WZ(uint16_t value) { m_WZ.w = value; }

    // 8-bit parts of WZ
    uint8_t get_W() const { return m_WZ.h; }
    void set_W(uint8_t value) { m_WZ.h = value; }
    uint8_t get_Z() const { return m_WZ.l; }
    void set_Z(uint8_t value) { m_WZ.l = value; }

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
    Register m_AFp, m_BCp, m_DEp, m_HLp, m_WZ;
    Register m_IX, m_IY;
    uint16_t m_SP, m_PC;
    uint8_t m_I, m_R;
    bool m_IFF1, m_IFF2;
    
    //internal CPU states
    bool m_halted, m_NMI_pending, m_IRQ_request, m_EI_delay, m_RETI_signaled;
    uint8_t m_IRQ_data, m_IRQ_mode;
    IndexMode m_index_mode;
    
    //CPU T-states
    long long m_ticks;
    
    //Bus
    uint16_t m_address_bus; //A0-A15
    uint8_t m_data_bus; //D0-D7

    //Memory and IO operations
    TBus* m_bus;
    TEvents* m_events;
    TDebugger* m_debugger;
    bool m_owns_bus = false;
    bool m_owns_events = false;
    bool m_owns_debugger = false;
    std::vector<uint8_t> m_opcodes;

    //Internal memory access helpers
    uint8_t read_byte(uint16_t address) {
        m_address_bus = address;
        add_tick(); //T1
        add_tick(); //T2
        uint8_t data = m_bus->read(address);
        m_data_bus = data;
        add_tick(); //T3
        return data;
    }
    uint16_t read_word(uint16_t address) {
        uint8_t low_byte = read_byte(address);
        uint8_t high_byte = read_byte(address + 1);
        return (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    }
    void write_byte(uint16_t address, uint8_t value) {
        m_address_bus = address;
        add_tick(); //T1
        m_data_bus = value;
        add_tick(); //T2
        m_bus->write(address, value);
        add_tick(); //T3
    }
    void write_word(uint16_t address, uint16_t value) {
        write_byte(address, value & 0xFF);
        write_byte(address + 1, value >> 8);
    }
    void push_word(uint16_t value) {
        set_SP(get_SP() - 1);
        add_tick();
        write_byte(get_SP(), value >> 8);
        set_SP(get_SP() - 1);
        write_byte(get_SP(), value & 0xFF);
    }
    uint16_t pop_word() {
        uint8_t low_byte = read_byte(get_SP());
        set_SP(get_SP() + 1);
        uint8_t high_byte = read_byte(get_SP());
        set_SP(get_SP() + 1);
        return  (static_cast<uint16_t>(high_byte) << 8) | low_byte;
    }
    uint8_t fetch_next_opcode() {
        uint16_t current_pc = get_PC();
        m_address_bus = current_pc;
        add_tick(); //T1
        add_tick(); //T2
        uint8_t opcode = m_bus->read(current_pc);
        m_data_bus = opcode;
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>)
            m_opcodes.push_back(opcode);
        uint8_t r_val = get_R();
        set_R(((r_val + 1) & 0x7F) | (r_val & 0x80));
        add_tick(); //T3
        add_tick(); //T4
        set_PC(current_pc + 1);
        return opcode;
    }
    uint8_t fetch_next_byte() {
        uint16_t current_pc = get_PC();
        uint8_t byte_val = read_byte(current_pc);
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>)
            m_opcodes.push_back(byte_val);
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
            int8_t offset = static_cast<int8_t>(fetch_next_byte()); 
            uint16_t address = get_indexed_HL() + offset;
            set_WZ(address);
            add_ticks(5);
            return address;
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
        uint16_t port = get_BC();
        uint8_t value = m_bus->in(port);
        Flags flags = get_F();
        set_WZ(port + 1);
        flags.update(Flags::S, (value & 0x80) != 0)
            .update(Flags::Z, value == 0)
            .clear(Flags::H | Flags::N)
            .update(Flags::PV, is_parity_even(value))
            .update(Flags::X, (value & Flags::X) != 0)
            .update(Flags::Y, (value & Flags::Y) != 0);
        set_F(flags);
        return value;
    }
    void out_c_r(uint8_t value) {
        m_bus->out(get_BC(), value);
        set_WZ(get_BC() + 1);
    }

    //Interrupt handling
    void handle_NMI() {
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>) {
            m_debugger->before_NMI();
        }
        set_halted(false);
        set_IFF2(get_IFF1());
        set_IFF1(false);
        push_word(get_PC());
        set_WZ(0x0066);
        set_PC(0x0066);
        set_NMI_pending(false);
        add_ticks(4);
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>) {
            m_debugger->after_NMI();
        }
    }
    void handle_IRQ() {
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>) {
            m_debugger->before_IRQ();
        }
        set_halted(false);
        add_ticks(2); // Two wait states during interrupt acknowledge cycle
        set_IFF2(get_IFF1());
        set_IFF1(false);
        push_word(get_PC());
        switch (get_IRQ_mode()) {
            case 0: {
                add_ticks(4); // Internal operations for RST
                uint8_t opcode = get_IRQ_data();
                switch (opcode) {
                    case 0xC7: set_WZ(0x0000); set_PC(0x0000); break;
                    case 0xCF: set_WZ(0x0008); set_PC(0x0008); break;
                    case 0xD7: set_WZ(0x0010); set_PC(0x0010); break;
                    case 0xDF: set_WZ(0x0018); set_PC(0x0018); break;
                    case 0xE7: set_WZ(0x0020); set_PC(0x0020); break;
                    case 0xEF: set_WZ(0x0028); set_PC(0x0028); break;
                    case 0xF7: set_WZ(0x0030); set_PC(0x0030); break;
                    case 0xFF: set_WZ(0x0038); set_PC(0x0038); break;
                }
                break;
            }
            case 1: {
                add_ticks(4); // Internal operations for RST 38H
                set_WZ(0x0038);
                set_PC(0x0038);
                break;
            }
            case 2: {
                uint16_t vector_address = (static_cast<uint16_t>(get_I()) << 8) | get_IRQ_data();
                uint16_t handler_address = read_word(vector_address);
                add_ticks(4); // Internal operations
                set_WZ(handler_address);
                set_PC(handler_address);
                break;
            }
        }
        set_IRQ_request(false);
        if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>) {
            m_debugger->after_IRQ();
        }
    }

    //Opcodes handling
    void handle_CB_opcodes(uint8_t opcode) {
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
                    set_WZ(flags_source);
                    add_tick();
                    flags.update(Flags::X, (get_W() & 0x08) != 0);
                    flags.update(Flags::Y, (get_W() & 0x20) != 0);
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
                add_tick();
                write_byte(get_HL(), result);
                break;
            case 7: set_A(result); break;
        }
    }
    void handle_CB_indexed_opcodes(uint16_t index_register, int8_t offset, uint8_t opcode) {
        uint16_t address = index_register + offset;
        set_WZ(address);
        add_ticks(2); // 2 T-states for address calculation
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
                add_tick(); // 1 T-state for internal operation
                bit_8bit(bit, value);
                Flags flags = get_F();
                flags.update(Flags::X, (get_W() & 0x08) != 0);
                flags.update(Flags::Y, (get_W() & 0x20) != 0);
                set_F(flags);
                return;
            }
            case 2: result = res_8bit(bit, value); break;
            case 3: result = set_8bit(bit, value); break;
        }
        add_tick(); // 1 T-state for internal operation
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
    void handle_opcode_0x00_NOP() {
    }
    void handle_opcode_0x01_LD_BC_nn() {
        set_BC(fetch_next_word());
    }
    void handle_opcode_0x02_LD_BC_ptr_A() {
        uint16_t address = get_BC();
        write_byte(address, get_A());
        set_WZ((static_cast<uint16_t>(get_A()) << 8) | ((address + 1) & 0xFF));
    }
    void handle_opcode_0x03_INC_BC() {
        set_BC(get_BC() + 1);
        add_ticks(2);
    }
    void handle_opcode_0x04_INC_B() {
        set_B(inc_8bit(get_B()));
    }
    void handle_opcode_0x05_DEC_B() {
        set_B(dec_8bit(get_B()));
    }
    void handle_opcode_0x06_LD_B_n() {
        set_B(fetch_next_byte());
    }
    void handle_opcode_0x07_RLCA() {
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
    void handle_opcode_0x08_EX_AF_AFp() {
        uint16_t temp_AF = get_AF();
        set_AF(get_AFp());
        set_AFp(temp_AF);
    }
    void handle_opcode_0x09_ADD_HL_BC() {
        add_ticks(7);
        uint16_t value = get_indexed_HL();
        set_WZ(value + 1);
        set_indexed_HL(add_16bit(value, get_BC()));
    }
    void handle_opcode_0x0A_LD_A_BC_ptr() {
        uint16_t address = get_BC();
        set_A(read_byte(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0x0B_DEC_BC() {
        set_BC(get_BC() - 1);
        add_ticks(2);
    }
    void handle_opcode_0x0C_INC_C() {
        set_C(inc_8bit(get_C()));
    }
    void handle_opcode_0x0D_DEC_C() {
        set_C(dec_8bit(get_C()));
    }
    void handle_opcode_0x0E_LD_C_n() {
        set_C(fetch_next_byte());
    }
    void handle_opcode_0x0F_RRCA() {
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
    void handle_opcode_0x10_DJNZ_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        uint8_t new_b_value = get_B() - 1;
        set_B(new_b_value);
        add_tick();
        if (new_b_value != 0) {
            set_PC(address);
            add_ticks(5);
        }
    }
    void handle_opcode_0x11_LD_DE_nn() {
        set_DE(fetch_next_word());
    }
    void handle_opcode_0x12_LD_DE_ptr_A() {
        uint16_t address = get_DE();
        write_byte(address, get_A());
        set_WZ((static_cast<uint16_t>(get_A()) << 8) | ((address + 1) & 0xFF));
    }
    void handle_opcode_0x13_INC_DE() {
        set_DE(get_DE() + 1);
        add_ticks(2);
    }
    void handle_opcode_0x14_INC_D() {
        set_D(inc_8bit(get_D()));
    }
    void handle_opcode_0x15_DEC_D() {
        set_D(dec_8bit(get_D()));
    }
    void handle_opcode_0x16_LD_D_n() {
        set_D(fetch_next_byte());
    }
    void handle_opcode_0x17_RLA() {
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
    void handle_opcode_0x18_JR_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        set_PC(address);
        add_ticks(5);
    }
    void handle_opcode_0x19_ADD_HL_DE() {
        add_ticks(7);
        uint16_t value = get_indexed_HL();
        set_WZ(value + 1);
        set_indexed_HL(add_16bit(value, get_DE()));
    }
    void handle_opcode_0x1A_LD_A_DE_ptr() {
        uint16_t address = get_DE();
        set_A(read_byte(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0x1B_DEC_DE() {
        set_DE(get_DE() - 1);
        add_ticks(2);
    }
    void handle_opcode_0x1C_INC_E() {
        set_E(inc_8bit(get_E()));
    }
    void handle_opcode_0x1D_DEC_E() {
        set_E(dec_8bit(get_E()));
    }
    void handle_opcode_0x1E_LD_E_n() {
        set_E(fetch_next_byte());
    }
    void handle_opcode_0x1F_RRA() {
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
    void handle_opcode_0x20_JR_NZ_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        if (!get_F().is_set(Flags::Z)) {
            set_PC(address);
            add_ticks(5);
        }
    }
    void handle_opcode_0x21_LD_HL_nn() {
        set_indexed_HL(fetch_next_word());
    }
    void handle_opcode_0x22_LD_nn_ptr_HL() {
        uint16_t address = fetch_next_word();
        write_word(address, get_indexed_HL());
        set_WZ(address + 1);
    }
    void handle_opcode_0x23_INC_HL() {
        set_indexed_HL(get_indexed_HL() + 1);
        add_ticks(2);
    }
    void handle_opcode_0x24_INC_H() {
        set_indexed_H(inc_8bit(get_indexed_H()));
    }
    void handle_opcode_0x25_DEC_H() {
        set_indexed_H(dec_8bit(get_indexed_H()));
    }
    void handle_opcode_0x26_LD_H_n() {
        set_indexed_H(fetch_next_byte());
    }
    void handle_opcode_0x27_DAA() {
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
    void handle_opcode_0x28_JR_Z_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        if (get_F().is_set(Flags::Z)) {
            set_PC(address);
            add_ticks(5);
        }
    }
    void handle_opcode_0x29_ADD_HL_HL() {
        add_ticks(7);
        uint16_t value = get_indexed_HL();
        set_WZ(value + 1);
        set_indexed_HL(add_16bit(value, value));
    }
    void handle_opcode_0x2A_LD_HL_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_indexed_HL(read_word(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0x2B_DEC_HL() {
        set_indexed_HL(get_indexed_HL() - 1);
        add_ticks(2);
    }
    void handle_opcode_0x2C_INC_L() {
        set_indexed_L(inc_8bit(get_indexed_L()));
    }
    void handle_opcode_0x2D_DEC_L() {
        set_indexed_L(dec_8bit(get_indexed_L()));
    }
    void handle_opcode_0x2E_LD_L_n() {
        set_indexed_L(fetch_next_byte());
    }
    void handle_opcode_0x2F_CPL() {
        uint8_t result = ~get_A();
        set_A(result);
        Flags flags = get_F();
        flags.set(Flags::H | Flags::N)
            .update(Flags::Y, (result & Flags::Y) != 0)
            .update(Flags::X, (result & Flags::X) != 0);
        set_F(flags);
    }
    void handle_opcode_0x30_JR_NC_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        if (!get_F().is_set(Flags::C)) {
            set_PC(address);
            add_ticks(5);
        }
    }
    void handle_opcode_0x31_LD_SP_nn() {
        set_SP(fetch_next_word());
    }
    void handle_opcode_0x32_LD_nn_ptr_A() {
        uint16_t address = fetch_next_word();
        write_byte(address, get_A());
        set_WZ((static_cast<uint16_t>(get_A()) << 8) | ((address + 1) & 0xFF));
    }
    void handle_opcode_0x33_INC_SP() {
        set_SP(get_SP() + 1);
        add_ticks(2);
    }
    void handle_opcode_0x34_INC_HL_ptr() {
        uint16_t address = get_indexed_address();
        uint8_t value = read_byte(address);
        add_tick();
        write_byte(address, inc_8bit(value));
    }
    void handle_opcode_0x35_DEC_HL_ptr() {
        uint16_t address = get_indexed_address();
        uint8_t value = read_byte(address);
        add_tick();
        write_byte(address, dec_8bit(value));
    }
    void handle_opcode_0x36_LD_HL_ptr_n() {
        if (get_index_mode() == IndexMode::HL) {
            uint8_t value = fetch_next_byte();
            write_byte(get_HL(), value);
        } else {
            int8_t offset = static_cast<int8_t>(fetch_next_byte());
            uint16_t address = (get_index_mode() == IndexMode::IX ? get_IX() : get_IY()) + offset;
            add_ticks(2);
            uint8_t value = fetch_next_byte();
            write_byte(address, value);
        }
    }
    void handle_opcode_0x37_SCF() {
        Flags flags = get_F();
        flags.set(Flags::C)
            .clear(Flags::H | Flags::N)
            .update(Flags::X, (get_A() & Flags::X) != 0)
            .update(Flags::Y, (get_A() & Flags::Y) != 0);
        set_F(flags);
    }
    void handle_opcode_0x38_JR_C_d() {
        int8_t offset = static_cast<int8_t>(fetch_next_byte());
        uint16_t address = get_PC() + offset;
        set_WZ(address);
        if (get_F().is_set(Flags::C)) {
            set_PC(address);
            add_ticks(5);
        }
    }
    void handle_opcode_0x39_ADD_HL_SP() {
        add_ticks(7);
        uint16_t value = get_indexed_HL();
        set_WZ(value + 1);
        set_indexed_HL(add_16bit(value, get_SP()));
    }
    void handle_opcode_0x3A_LD_A_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_A(read_byte(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0x3B_DEC_SP() {
        set_SP(get_SP() - 1);
        add_ticks(2);
    }
    void handle_opcode_0x3C_INC_A() {
        set_A(inc_8bit(get_A()));
    }
    void handle_opcode_0x3D_DEC_A() {
        set_A(dec_8bit(get_A()));
    }
    void handle_opcode_0x3E_LD_A_n() {
        set_A(fetch_next_byte());
    }
    void handle_opcode_0x3F_CCF() {
        Flags flags = get_F();
        bool old_carry = flags.is_set(Flags::C);
        flags.update(Flags::C, !old_carry)
            .update(Flags::H, old_carry)
            .clear(Flags::N)
            .update(Flags::X, (get_A() & Flags::X) != 0)
            .update(Flags::Y, (get_A() & Flags::Y) != 0);
        set_F(flags);
    }
    void handle_opcode_0x40_LD_B_B() {
    }
    void handle_opcode_0x41_LD_B_C() {
        set_B(get_C());
    }
    void handle_opcode_0x42_LD_B_D() {
        set_B(get_D());
    }
    void handle_opcode_0x43_LD_B_E() {
        set_B(get_E());
    }
    void handle_opcode_0x44_LD_B_H() {
        set_B(get_indexed_H());
    }
    void handle_opcode_0x45_LD_B_L() {
        set_B(get_indexed_L());
    }
    void handle_opcode_0x46_LD_B_HL_ptr() {
        set_B(get_indexed_HL_ptr());
    }
    void handle_opcode_0x47_LD_B_A() {
        set_B(get_A());
    }
    void handle_opcode_0x48_LD_C_B() {
        set_C(get_B());
    }
    void handle_opcode_0x49_LD_C_C() {
    }
    void handle_opcode_0x4A_LD_C_D() {
        set_C(get_D());
    }
    void handle_opcode_0x4B_LD_C_E() {
        set_C(get_E());
    }
    void handle_opcode_0x4C_LD_C_H() {
        set_C(get_indexed_H());
    }
    void handle_opcode_0x4D_LD_C_L() {
        set_C(get_indexed_L());
    }
    void handle_opcode_0x4E_LD_C_HL_ptr() {
        set_C(get_indexed_HL_ptr());
    }
    void handle_opcode_0x4F_LD_C_A() {
        set_C(get_A());
    }
    void handle_opcode_0x50_LD_D_B() {
        set_D(get_B());
    }
    void handle_opcode_0x51_LD_D_C() {
        set_D(get_C());
    }
    void handle_opcode_0x52_LD_D_D() {
    }
    void handle_opcode_0x53_LD_D_E() {
        set_D(get_E());
    }
    void handle_opcode_0x54_LD_D_H() {
        set_D(get_indexed_H());
    }
    void handle_opcode_0x55_LD_D_L() {
        set_D(get_indexed_L());
    }
    void handle_opcode_0x56_LD_D_HL_ptr() {
        set_D(get_indexed_HL_ptr());
    }
    void handle_opcode_0x57_LD_D_A() {
        set_D(get_A());
    }
    void handle_opcode_0x58_LD_E_B() {
        set_E(get_B());
    }
    void handle_opcode_0x59_LD_E_C() {
        set_E(get_C());
    }
    void handle_opcode_0x5A_LD_E_D() {
        set_E(get_D());
    }
    void handle_opcode_0x5B_LD_E_E() {
    }
    void handle_opcode_0x5C_LD_E_H() {
        set_E(get_indexed_H());
    }
    void handle_opcode_0x5D_LD_E_L() {
        set_E(get_indexed_L());
    }
    void handle_opcode_0x5E_LD_E_HL_ptr() {
        set_E(get_indexed_HL_ptr());
    }
    void handle_opcode_0x5F_LD_E_A() {
        set_E(get_A());
    }
    void handle_opcode_0x60_LD_H_B() {
        set_indexed_H(get_B());
    }
    void handle_opcode_0x61_LD_H_C() {
        set_indexed_H(get_C());
    }
    void handle_opcode_0x62_LD_H_D() {
        set_indexed_H(get_D());
    }
    void handle_opcode_0x63_LD_H_E() {
        set_indexed_H(get_E());
    }
    void handle_opcode_0x64_LD_H_H() {
    }
    void handle_opcode_0x65_LD_H_L() {
        set_indexed_H(get_indexed_L());
    }
    void handle_opcode_0x66_LD_H_HL_ptr() {
        set_H(get_indexed_HL_ptr());
    }
    void handle_opcode_0x67_LD_H_A() {
        set_indexed_H(get_A());
    }
    void handle_opcode_0x68_LD_L_B() {
        set_indexed_L(get_B());
    }
    void handle_opcode_0x69_LD_L_C() {
        set_indexed_L(get_C());
    }
    void handle_opcode_0x6A_LD_L_D() {
        set_indexed_L(get_D());
    }
    void handle_opcode_0x6B_LD_L_E() {
        set_indexed_L(get_E());
    }
    void handle_opcode_0x6C_LD_L_H() {
        set_indexed_L(get_indexed_H());
    }
    void handle_opcode_0x6D_LD_L_L() {
    }
    void handle_opcode_0x6E_LD_L_HL_ptr() {
        set_L(get_indexed_HL_ptr());
    }
    void handle_opcode_0x6F_LD_L_A() {
        set_indexed_L(get_A());
    }
    void handle_opcode_0x70_LD_HL_ptr_B() {
        set_indexed_HL_ptr(get_B());
    }
    void handle_opcode_0x71_LD_HL_ptr_C() {
        set_indexed_HL_ptr(get_C());
    }
    void handle_opcode_0x72_LD_HL_ptr_D() {
        set_indexed_HL_ptr(get_D());
    }
    void handle_opcode_0x73_LD_HL_ptr_E() {
        set_indexed_HL_ptr(get_E());
    }
    void handle_opcode_0x74_LD_HL_ptr_H() {
        set_indexed_HL_ptr(get_H());
    }
    void handle_opcode_0x75_LD_HL_ptr_L() {
        set_indexed_HL_ptr(get_L());
    }
    void handle_opcode_0x76_HALT() {
        set_halted(true);
    }
    void handle_opcode_0x77_LD_HL_ptr_A() {
        set_indexed_HL_ptr(get_A());
    }
    void handle_opcode_0x78_LD_A_B() {
        set_A(get_B());
    }
    void handle_opcode_0x79_LD_A_C() {
        set_A(get_C());
    }
    void handle_opcode_0x7A_LD_A_D() {
        set_A(get_D());
    }
    void handle_opcode_0x7B_LD_A_E() {
        set_A(get_E());
    }
    void handle_opcode_0x7C_LD_A_H() {
        set_A(get_indexed_H());
    }
    void handle_opcode_0x7D_LD_A_L() {
        set_A(get_indexed_L());
    }
    void handle_opcode_0x7E_LD_A_HL_ptr() {
        set_A(get_indexed_HL_ptr());
    }
    void handle_opcode_0x7F_LD_A_A() {
    }
    void handle_opcode_0x80_ADD_A_B() {
        add_8bit(get_B());
    }
    void handle_opcode_0x81_ADD_A_C() {
        add_8bit(get_C());
    }
    void handle_opcode_0x82_ADD_A_D() {
        add_8bit(get_D());
    }
    void handle_opcode_0x83_ADD_A_E() {
        add_8bit(get_E());
    }
    void handle_opcode_0x84_ADD_A_H() {
        add_8bit(get_indexed_H());
    }
    void handle_opcode_0x85_ADD_A_L() {
        add_8bit(get_indexed_L());
    }
    void handle_opcode_0x86_ADD_A_HL_ptr() {
        add_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0x87_ADD_A_A() {
        add_8bit(get_A());
    }
    void handle_opcode_0x88_ADC_A_B() {
        adc_8bit(get_B());
    }
    void handle_opcode_0x89_ADC_A_C() {
        adc_8bit(get_C());
    }
    void handle_opcode_0x8A_ADC_A_D() {
        adc_8bit(get_D());
    }
    void handle_opcode_0x8B_ADC_A_E() {
        adc_8bit(get_E());
    }
    void handle_opcode_0x8C_ADC_A_H() {
        adc_8bit(get_indexed_H());
    }
    void handle_opcode_0x8D_ADC_A_L() {
        adc_8bit(get_indexed_L());
    }
    void handle_opcode_0x8E_ADC_A_HL_ptr() {
        adc_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0x8F_ADC_A_A() {
        adc_8bit(get_A());
    }
    void handle_opcode_0x90_SUB_B() {
        sub_8bit(get_B());
    }
    void handle_opcode_0x91_SUB_C() {
        sub_8bit(get_C());
    }
    void handle_opcode_0x92_SUB_D() {
        sub_8bit(get_D());
    }
    void handle_opcode_0x93_SUB_E() {
        sub_8bit(get_E());
    }
    void handle_opcode_0x94_SUB_H() {
        sub_8bit(get_indexed_H());
    }
    void handle_opcode_0x95_SUB_L() {
        sub_8bit(get_indexed_L());
    }
    void handle_opcode_0x96_SUB_HL_ptr() {
        sub_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0x97_SUB_A() {
        sub_8bit(get_A());
    }
    void handle_opcode_0x98_SBC_A_B() {
        sbc_8bit(get_B());
    }
    void handle_opcode_0x99_SBC_A_C() {
        sbc_8bit(get_C());
    }
    void handle_opcode_0x9A_SBC_A_D() {
        sbc_8bit(get_D());
    }
    void handle_opcode_0x9B_SBC_A_E() {
        sbc_8bit(get_E());
    }
    void handle_opcode_0x9C_SBC_A_H() {
        sbc_8bit(get_indexed_H());
    }
    void handle_opcode_0x9D_SBC_A_L() {
        sbc_8bit(get_indexed_L());
    }
    void handle_opcode_0x9E_SBC_A_HL_ptr() {
        sbc_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0x9F_SBC_A_A() {
        sbc_8bit(get_A());
    }
    void handle_opcode_0xA0_AND_B() {
        and_8bit(get_B());
    }
    void handle_opcode_0xA1_AND_C() {
        and_8bit(get_C());
    }
    void handle_opcode_0xA2_AND_D() {
        and_8bit(get_D());
    }
    void handle_opcode_0xA3_AND_E() {
        and_8bit(get_E());
    }
    void handle_opcode_0xA4_AND_H() {
        and_8bit(get_indexed_H());
    }
    void handle_opcode_0xA5_AND_L() {
        and_8bit(get_indexed_L());
    }
    void handle_opcode_0xA6_AND_HL_ptr() {
        and_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0xA7_AND_A() {
        and_8bit(get_A());
    }
    void handle_opcode_0xA8_XOR_B() {
        xor_8bit(get_B());
    }
    void handle_opcode_0xA9_XOR_C() {
        xor_8bit(get_C());
    }
    void handle_opcode_0xAA_XOR_D() {
        xor_8bit(get_D());
    }
    void handle_opcode_0xAB_XOR_E() {
        xor_8bit(get_E());
    }
    void handle_opcode_0xAC_XOR_H() {
        xor_8bit(get_indexed_H());
    }
    void handle_opcode_0xAD_XOR_L() {
        xor_8bit(get_indexed_L());
    }
    void handle_opcode_0xAE_XOR_HL_ptr() {
        xor_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0xAF_XOR_A() {
        xor_8bit(get_A());
    }
    void handle_opcode_0xB0_OR_B() {
        or_8bit(get_B());
    }
    void handle_opcode_0xB1_OR_C() {
        or_8bit(get_C());
    }
    void handle_opcode_0xB2_OR_D() {
        or_8bit(get_D());
    }
    void handle_opcode_0xB3_OR_E() {
        or_8bit(get_E());
    }
    void handle_opcode_0xB4_OR_H() {
        or_8bit(get_indexed_H());
    }
    void handle_opcode_0xB5_OR_L() {
        or_8bit(get_indexed_L());
    }
    void handle_opcode_0xB6_OR_HL_ptr() {
        or_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0xB7_OR_A() {
        or_8bit(get_A());
    }
    void handle_opcode_0xB8_CP_B() {
        cp_8bit(get_B());
    }
    void handle_opcode_0xB9_CP_C() {
        cp_8bit(get_C());
    }
    void handle_opcode_0xBA_CP_D() {
        cp_8bit(get_D());
    }
    void handle_opcode_0xBB_CP_E() {
        cp_8bit(get_E());
    }
    void handle_opcode_0xBC_CP_H() {
        cp_8bit(get_indexed_H());
    }
    void handle_opcode_0xBD_CP_L() {
        cp_8bit(get_indexed_L());
    }
    void handle_opcode_0xBE_CP_HL_ptr() {
        cp_8bit(get_indexed_HL_ptr());
    }
    void handle_opcode_0xBF_CP_A() {
        cp_8bit(get_A());
    }
    void handle_opcode_0xC0_RET_NZ() {
        add_tick();
        if (!get_F().is_set(Flags::Z)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xC1_POP_BC() {
        set_BC(pop_word());
    }
    void handle_opcode_0xC2_JP_NZ_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::Z))
            set_PC(address);
    }
    void handle_opcode_0xC3_JP_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        set_PC(address);
    }
    void handle_opcode_0xC4_CALL_NZ_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::Z)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xC5_PUSH_BC() {
        push_word(get_BC());
    }
    void handle_opcode_0xC6_ADD_A_n() {
        add_8bit(fetch_next_byte());
    }
    void handle_opcode_0xC7_RST_00H() {
        push_word(get_PC());
        set_WZ(0x0000);
        set_PC(0x0000);
    }
    void handle_opcode_0xC8_RET_Z() {
        add_tick();
        if (get_F().is_set(Flags::Z)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xC9_RET() {
        uint16_t address = pop_word();
        set_WZ(address);
        set_PC(address);
    }
    void handle_opcode_0xCA_JP_Z_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::Z))
            set_PC(address);
    }
    void handle_opcode_0xCC_CALL_Z_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::Z)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xCD_CALL_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        push_word(get_PC());
        set_PC(address);
    }
    void handle_opcode_0xCE_ADC_A_n() {
        adc_8bit(fetch_next_byte());
    }
    void handle_opcode_0xCF_RST_08H() {
        push_word(get_PC());
        set_WZ(0x0008);
        set_PC(0x0008);
    }
    void handle_opcode_0xD0_RET_NC() {
        add_tick();
        if (!get_F().is_set(Flags::C)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xD1_POP_DE() {
        set_DE(pop_word());
    }
    void handle_opcode_0xD2_JP_NC_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::C))
            set_PC(address);
    }
    void handle_opcode_0xD3_OUT_n_ptr_A() {
        uint8_t port_lo = fetch_next_byte();
        uint16_t port = (get_A() << 8) | port_lo;
        m_bus->out(port, get_A());
        add_ticks(4);
        set_WZ((static_cast<uint16_t>(get_A()) << 8) | ((port_lo + 1) & 0xFF));
    }
    void handle_opcode_0xD4_CALL_NC_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::C)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xD5_PUSH_DE() {
        push_word(get_DE());
    }
    void handle_opcode_0xD6_SUB_n() {
        sub_8bit(fetch_next_byte());
    }
    void handle_opcode_0xD7_RST_10H() {
        push_word(get_PC());
        set_WZ(0x0010);
        set_PC(0x0010);
    }
    void handle_opcode_0xD8_RET_C() {
        add_tick();
        if (get_F().is_set(Flags::C)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xD9_EXX() {
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
    void handle_opcode_0xDA_JP_C_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::C))
            set_PC(address);
    }
    void handle_opcode_0xDB_IN_A_n_ptr() {
        uint8_t port_lo = fetch_next_byte();
        uint16_t port = (get_A() << 8) | port_lo;
        set_WZ(port + 1);
        set_A(m_bus->in(port));
        add_ticks(4);
    }
    void handle_opcode_0xDC_CALL_C_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::C)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xDE_SBC_A_n() {
        sbc_8bit(fetch_next_byte());
    }
    void handle_opcode_0xDF_RST_18H() {
        push_word(get_PC());
        set_WZ(0x0018);
        set_PC(0x0018);
    }
    void handle_opcode_0xE0_RET_PO() {
        add_tick();
        if (!get_F().is_set(Flags::PV)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xE1_POP_HL() {
        set_indexed_HL(pop_word());
    }
    void handle_opcode_0xE2_JP_PO_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::PV))
            set_PC(address);
    }
    void handle_opcode_0xE3_EX_SP_ptr_HL() {
        uint16_t from_stack = read_word(get_SP());
        add_tick(); // 1 T-state for internal operation
        set_WZ(from_stack);
        write_word(get_SP(), get_indexed_HL());
        set_indexed_HL(from_stack);
        add_ticks(2); // 2 T-states for internal operation
    }
    void handle_opcode_0xE4_CALL_PO_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::PV)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xE5_PUSH_HL() {
        push_word(get_indexed_HL());
    }
    void handle_opcode_0xE6_AND_n() {
        and_8bit(fetch_next_byte());
    }
    void handle_opcode_0xE7_RST_20H() {
        push_word(get_PC());
        set_WZ(0x0020);
        set_PC(0x0020);
    }
    void handle_opcode_0xE8_RET_PE() {
        add_tick();
        if (get_F().is_set(Flags::PV)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xE9_JP_HL_ptr() {
        set_PC(get_indexed_HL());
    }
    void handle_opcode_0xEA_JP_PE_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::PV))
            set_PC(address);
    }
    void handle_opcode_0xEB_EX_DE_HL() {
        uint16_t temp = get_HL();
        set_HL(get_DE());
        set_DE(temp);
    }
    void handle_opcode_0xEC_CALL_PE_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::PV)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xEE_XOR_n() {
        xor_8bit(fetch_next_byte());
    }
    void handle_opcode_0xEF_RST_28H() {
        push_word(get_PC());
        set_WZ(0x0028);
        set_PC(0x0028);
    }
    void handle_opcode_0xF0_RET_P() {
        add_tick();
        if (!get_F().is_set(Flags::S)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xF1_POP_AF() {
        set_AF(pop_word());
    }
    void handle_opcode_0xF2_JP_P_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::S))
            set_PC(address);
    }
    void handle_opcode_0xF3_DI() {
        set_IFF1(false);
        set_IFF2(false);
    }
    void handle_opcode_0xF4_CALL_P_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (!get_F().is_set(Flags::S)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xF5_PUSH_AF() {
        push_word(get_AF());
    }
    void handle_opcode_0xF6_OR_n() {
        or_8bit(fetch_next_byte());
    }
    void handle_opcode_0xF7_RST_30H() {
        push_word(get_PC());
        set_WZ(0x0030);
        set_PC(0x0030);
    }
    void handle_opcode_0xF8_RET_M() {
        add_tick();
        if (get_F().is_set(Flags::S)) {
            uint16_t address = pop_word();
            set_WZ(address);
            set_PC(address);
        }
    }
    void handle_opcode_0xF9_LD_SP_HL() {
        set_SP(get_indexed_HL());
        set_WZ(get_SP() + 1);
        add_ticks(2);
    }
    void handle_opcode_0xFA_JP_M_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::S))
            set_PC(address);
    }
    void handle_opcode_0xFB_EI() {
        set_EI_delay(true);
    }
    void handle_opcode_0xFC_CALL_M_nn() {
        uint16_t address = fetch_next_word();
        set_WZ(address);
        if (get_F().is_set(Flags::S)) {
            push_word(get_PC());
            set_PC(address);
        }
    }
    void handle_opcode_0xFE_CP_n() {
        cp_8bit(fetch_next_byte());
    }
    void handle_opcode_0xFF_RST_38H() {
        push_word(get_PC());
        set_WZ(0x0038);
        set_PC(0x0038);
    }
    void handle_opcode_0xED_0x40_IN_B_C_ptr() {
        set_B(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x41_OUT_C_ptr_B() {
        out_c_r(get_B());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x42_SBC_HL_BC() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(sbc_16bit(value, get_BC()));
    }
    void handle_opcode_0xED_0x43_LD_nn_ptr_BC() {
        uint16_t address = fetch_next_word();
        write_word(address, get_BC());
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x44_NEG() {
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
    void handle_opcode_0xED_0x45_RETN() {
        set_IFF1(get_IFF2());
        uint16_t address = pop_word();
        set_WZ(address);
        set_PC(address);
    }
    void handle_opcode_0xED_0x46_IM_0() {
        set_IRQ_mode(0);
    }
    void handle_opcode_0xED_0x47_LD_I_A() {
        add_tick();
        set_I(get_A());
    }
    void handle_opcode_0xED_0x48_IN_C_C_ptr() {
        set_C(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x49_OUT_C_ptr_C() {
        out_c_r(get_C());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x4A_ADC_HL_BC() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(adc_16bit(value, get_BC()));
    }
    void handle_opcode_0xED_0x4B_LD_BC_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_BC(read_word(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x4D_RETI() {
        set_IFF1(get_IFF2());
        set_RETI_signaled(true);
        uint16_t address = pop_word();
        set_WZ(address);
        set_PC(address);
    }
    void handle_opcode_0xED_0x4F_LD_R_A() {
        add_tick();
        set_R(get_A());
    }
    void handle_opcode_0xED_0x50_IN_D_C_ptr() {
        set_D(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x51_OUT_C_ptr_D() {
        out_c_r(get_D());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x52_SBC_HL_DE() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(sbc_16bit(value, get_DE()));
    }
    void handle_opcode_0xED_0x53_LD_nn_ptr_DE() {
        uint16_t address = fetch_next_word();
        write_word(address, get_DE());
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x56_IM_1() {
        set_IRQ_mode(1);
    }
    void handle_opcode_0xED_0x57_LD_A_I() {
        add_tick();
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
    void handle_opcode_0xED_0x58_IN_E_C_ptr() {
        set_E(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x59_OUT_C_ptr_E() {
        out_c_r(get_E());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x5A_ADC_HL_DE() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(adc_16bit(value, get_DE()));
    }
    void handle_opcode_0xED_0x5B_LD_DE_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_DE(read_word(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x5E_IM_2() {
        set_IRQ_mode(2);
    }
    void handle_opcode_0xED_0x5F_LD_A_R() {
        add_tick();
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
    void handle_opcode_0xED_0x60_IN_H_C_ptr() {
        set_H(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x61_OUT_C_ptr_H() {
        out_c_r(get_H());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x62_SBC_HL_HL() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(sbc_16bit(value, value));
    }
    void handle_opcode_0xED_0x63_LD_nn_ptr_HL_ED() {
        uint16_t address = fetch_next_word();
        write_word(address, get_HL());
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x67_RRD() {
        uint16_t address = get_HL();
        uint8_t mem_val = read_byte(address);
        uint8_t a_val = get_A();
        uint8_t new_a = (a_val & 0xF0) | (mem_val & 0x0F);
        uint8_t new_mem = (mem_val >> 4) | ((a_val & 0x0F) << 4);
        set_A(new_a);
        add_ticks(4);
        write_byte(address, new_mem);
        set_WZ(address + 1);
        Flags flags(get_F() & Flags::C);
        flags.clear(Flags::H | Flags::N)
            .update(Flags::S, (new_a & 0x80) != 0)
            .update(Flags::Z, new_a == 0)
            .update(Flags::PV, is_parity_even(new_a))
            .update(Flags::X, (new_a & Flags::X) != 0)
            .update(Flags::Y, (new_a & Flags::Y) != 0);
        set_F(flags);
    }
    void handle_opcode_0xED_0x68_IN_L_C_ptr() {
        set_L(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x69_OUT_C_ptr_L() {
        out_c_r(get_L());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x6A_ADC_HL_HL() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(adc_16bit(value, value));
    }
    void handle_opcode_0xED_0x6B_LD_HL_nn_ptr_ED() {
        uint16_t address = fetch_next_word();
        set_HL(read_word(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x6F_RLD() {
        uint16_t address = get_HL();
        uint8_t mem_val = read_byte(address);
        uint8_t a_val = get_A();
        uint8_t new_a = (a_val & 0xF0) | (mem_val >> 4);
        uint8_t new_mem = (mem_val << 4) | (a_val & 0x0F);
        set_A(new_a);
        add_ticks(4);
        set_WZ(address + 1);
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
    void handle_opcode_0xED_0x70_IN_C_ptr() {
        in_r_c();
        add_ticks(4);
    }
    void handle_opcode_0xED_0x71_OUT_C_ptr_0() {
        out_c_r(0x00);
        add_ticks(4);
    }
    void handle_opcode_0xED_0x72_SBC_HL_SP() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(sbc_16bit(value, get_SP()));
    }
    void handle_opcode_0xED_0x73_LD_nn_ptr_SP() {
        uint16_t address = fetch_next_word();
        write_word(address, get_SP());
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0x78_IN_A_C_ptr() {
        set_A(in_r_c());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x79_OUT_C_ptr_A() {
        out_c_r(get_A());
        add_ticks(4);
    }
    void handle_opcode_0xED_0x7A_ADC_HL_SP() {
        add_ticks(7);
        uint16_t value = get_HL();
        set_WZ(value + 1);
        set_HL(adc_16bit(value, get_SP()));
    }
    void handle_opcode_0xED_0x7B_LD_SP_nn_ptr() {
        uint16_t address = fetch_next_word();
        set_SP(read_word(address));
        set_WZ(address + 1);
    }
    void handle_opcode_0xED_0xA0_LDI() {
        uint8_t value = read_byte(get_HL());
        write_byte(get_DE(), value);
        set_WZ(get_DE() + 1);
        set_HL(get_HL() + 1);
        set_DE(get_DE() + 1);
        set_BC(get_BC() - 1);
        add_ticks(2); // 2 T-states for internal operations
        Flags flags = get_F();
        uint8_t temp = get_A() + value;
        flags.clear(Flags::H | Flags::N)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
            
        set_F(flags);
    }
    void handle_opcode_0xED_0xA1_CPI() {
        uint8_t value = read_byte(get_HL());
        uint8_t result = get_A() - value;
        set_WZ(get_WZ() + 1);
        bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
        set_HL(get_HL() + 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        add_ticks(5); // 5 T-states for internal operations
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
    void handle_opcode_0xED_0xA2_INI() {
        uint8_t port_val = m_bus->in(get_BC());
        set_WZ(get_BC() + 1);
        add_ticks(4); // 4 T-states for I/O read cycle
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        add_tick(); // 1 T-state for wait cycle
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
    void handle_opcode_0xED_0xA3_OUTI() {
        uint8_t mem_val = read_byte(get_HL());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        m_bus->out(get_BC(), mem_val);
        set_WZ(get_BC() + 1);
        add_ticks(4); // 4 for I/O write
        set_HL(get_HL() + 1);
        add_tick(); // 1 for internal ops
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, temp > 0xFF)
            .update(Flags::H, temp > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF) );
        set_F(flags);
    }
    void handle_opcode_0xED_0xA8_LDD() {
        uint8_t value = read_byte(get_HL());
        write_byte(get_DE(), value);
        set_WZ(get_DE() - 1);
        set_HL(get_HL() - 1);
        set_DE(get_DE() - 1);
        set_BC(get_BC() - 1);
        add_ticks(2); // 2 T-states for internal operations
        Flags flags = get_F();
        uint8_t temp = get_A() + value;
        flags.clear(Flags::H | Flags::N)
            .update(Flags::PV, get_BC() != 0)
            .update(Flags::Y, (temp & 0x02) != 0)
            .update(Flags::X, (temp & 0x08) != 0);
        set_F(flags);
    }
    void handle_opcode_0xED_0xA9_CPD() {
        uint8_t value = read_byte(get_HL());
        uint8_t result = get_A() - value;
        set_WZ(get_WZ() - 1);
        bool half_carry = (get_A() & 0x0F) < (value & 0x0F);
        set_HL(get_HL() - 1);
        set_BC(get_BC() - 1);
        Flags flags = get_F();
        add_ticks(5); // 5 T-states for internal operations
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
    void handle_opcode_0xED_0xAA_IND() {
        uint8_t port_val = m_bus->in(get_BC());
        set_WZ(get_BC() - 1);
        add_ticks(4); // 4 T-states for I/O read cycle
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        add_tick(); // 1 T-state for wait cycle
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
    void handle_opcode_0xED_0xAB_OUTD() {
        uint8_t mem_val = read_byte(get_HL());
        uint8_t b_val = get_B();
        set_B(b_val - 1);
        m_bus->out(get_BC(), mem_val);
        set_WZ(get_BC() - 1);
        set_HL(get_HL() - 1);
        add_ticks(5); // 4 for I/O write, 1 for internal ops
        Flags flags = get_F();
        uint16_t temp = static_cast<uint16_t>(get_L()) + mem_val;
        flags.set(Flags::N)
            .update(Flags::Z, b_val - 1 == 0)
            .update(Flags::C, temp > 0xFF)
            .update(Flags::H, temp > 0xFF)
            .update(Flags::PV, is_parity_even( ((temp & 0x07) ^ b_val) & 0xFF));
        set_F(flags);
    }
    void handle_opcode_0xED_0xB0_LDIR() {
        handle_opcode_0xED_0xA0_LDI();
        if (get_BC() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xB1_CPIR() {
        handle_opcode_0xED_0xA1_CPI();
        if (get_BC() != 0 && !get_F().is_set(Flags::Z)) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xB2_INIR() {
        handle_opcode_0xED_0xA2_INI();
        if (get_B() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xB3_OTIR() {
        handle_opcode_0xED_0xA3_OUTI();
        if (get_B() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xB8_LDDR() {
        handle_opcode_0xED_0xA8_LDD();
        if (get_BC() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xB9_CPDR() {
        handle_opcode_0xED_0xA9_CPD();
        if (get_BC() != 0 && !get_F().is_set(Flags::Z)) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xBA_INDR() {
        handle_opcode_0xED_0xAA_IND();
        if (get_B() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
        }
    }
    void handle_opcode_0xED_0xBB_OTDR() {
        handle_opcode_0xED_0xAB_OUTD();
        if (get_B() != 0) {
            set_WZ(get_PC() + 1);
            set_PC(get_PC() - 2);
            add_ticks(5);
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
            if (get_EI_delay()) {
                set_IFF1(true);
                set_IFF2(true);
                set_EI_delay(false);
            }                
            if (is_halted()) {
                if constexpr (TMode == OperateMode::SingleStep)
                    add_ticks(4);
                else 
                    add_ticks(ticks_limit - get_ticks());
            }
            else {
                if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>)
                    m_opcodes.clear();
                set_index_mode(IndexMode::HL);
                uint8_t opcode = fetch_next_opcode();
                while (opcode == 0xDD || opcode == 0xFD) {
                    set_index_mode((opcode == 0xDD) ? IndexMode::IX : IndexMode::IY);
                    opcode = fetch_next_opcode();
                }
               if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>)
                    m_debugger->before_step(m_opcodes);
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
                        if (Z80_LIKELY((get_index_mode() == IndexMode::HL)))
                        {
                            uint8_t cb_opcode = fetch_next_opcode();
                            handle_CB_opcodes(cb_opcode);
                        } else { // DDCB d xx or FDCB d xx
                            uint16_t index_reg = (get_index_mode() == IndexMode::IX) ? get_IX() : get_IY();
                            int8_t offset = static_cast<int8_t>(fetch_next_byte());
                            uint8_t cb_opcode = fetch_next_byte();
                            handle_CB_indexed_opcodes(index_reg, offset, cb_opcode);
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
                        uint8_t opcodeED = fetch_next_opcode();
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
            if (is_NMI_pending())
                handle_NMI();
            else if (is_IRQ_pending())
                handle_IRQ();
            if constexpr (!std::is_same_v<TDebugger, Z80DefaultDebugger>) {
                m_debugger->after_step(m_opcodes);
            }
            if constexpr (TMode == OperateMode::SingleStep) {
                break;
            } else {
                if (get_ticks() >= ticks_limit)
                    break;
            }
        }
        return get_ticks() - initial_ticks;
    }

// Public execution API
#ifdef Z80_ENABLE_EXEC_API
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
    // --- DD Prefixed Opcodes (IX) ---
    void exec_ADD_IX_BC() {
        exec_DD_helper<&Z80::handle_opcode_0x09_ADD_HL_BC>();
    }
    void exec_ADD_IX_DE() {
        exec_DD_helper<&Z80::handle_opcode_0x19_ADD_HL_DE>();
    }
    void exec_LD_IX_nn() {
        exec_DD_helper<&Z80::handle_opcode_0x21_LD_HL_nn>();
    }
    void exec_LD_nn_ptr_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x22_LD_nn_ptr_HL>();
    }
    void exec_INC_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x23_INC_HL>();
    }
    void exec_INC_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x24_INC_H>();
    }
    void exec_DEC_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x25_DEC_H>();
    }
    void exec_LD_IXH_n() {
        exec_DD_helper<&Z80::handle_opcode_0x26_LD_H_n>();
    }
    void exec_ADD_IX_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x29_ADD_HL_HL>();
    }
    void exec_LD_IX_nn_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x2A_LD_HL_nn_ptr>();
    }
    void exec_DEC_IX() {
        exec_DD_helper<&Z80::handle_opcode_0x2B_DEC_HL>();
    }
    void exec_INC_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x2C_INC_L>();
    }
    void exec_DEC_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x2D_DEC_L>();
    }
    void exec_LD_IXL_n() {
        exec_DD_helper<&Z80::handle_opcode_0x2E_LD_L_n>();
    }
    void exec_INC_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x34_INC_HL_ptr>();
    }
    void exec_DEC_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x35_DEC_HL_ptr>();
    }
    void exec_LD_IX_d_ptr_n() {
        exec_DD_helper<&Z80::handle_opcode_0x36_LD_HL_ptr_n>();
    }
    void exec_ADD_IX_SP() {
        exec_DD_helper<&Z80::handle_opcode_0x39_ADD_HL_SP>();
    }
    void exec_LD_B_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x44_LD_B_H>();
    }
    void exec_LD_B_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x45_LD_B_L>();
    }
    void exec_LD_B_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x46_LD_B_HL_ptr>();
    }
    void exec_LD_C_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x4C_LD_C_H>();
    }
    void exec_LD_C_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x4D_LD_C_L>();
    }
    void exec_LD_C_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x4E_LD_C_HL_ptr>();
    }
    void exec_LD_D_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x54_LD_D_H>();
    }
    void exec_LD_D_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x55_LD_D_L>();
    }
    void exec_LD_D_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x56_LD_D_HL_ptr>();
    }
    void exec_LD_E_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x5C_LD_E_H>();
    }
    void exec_LD_E_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x5D_LD_E_L>();
    }
    void exec_LD_E_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x5E_LD_E_HL_ptr>();
    }
    void exec_LD_IXH_B() {
        exec_DD_helper<&Z80::handle_opcode_0x60_LD_H_B>();
    }
    void exec_LD_IXH_C() {
        exec_DD_helper<&Z80::handle_opcode_0x61_LD_H_C>();
    }
    void exec_LD_IXH_D() {
        exec_DD_helper<&Z80::handle_opcode_0x62_LD_H_D>();
    }
    void exec_LD_IXH_E() {
        exec_DD_helper<&Z80::handle_opcode_0x63_LD_H_E>();
    }
    void exec_LD_IXH_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x64_LD_H_H>();
    }
    void exec_LD_IXH_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x65_LD_H_L>();
    }
    void exec_LD_H_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x66_LD_H_HL_ptr>();
    }
    void exec_LD_IXH_A() {
        exec_DD_helper<&Z80::handle_opcode_0x67_LD_H_A>();
    }
    void exec_LD_IXL_B() {
        exec_DD_helper<&Z80::handle_opcode_0x68_LD_L_B>();
    }
    void exec_LD_IXL_C() {
        exec_DD_helper<&Z80::handle_opcode_0x69_LD_L_C>();
    }
    void exec_LD_IXL_D() {
        exec_DD_helper<&Z80::handle_opcode_0x6A_LD_L_D>();
    }
    void exec_LD_IXL_E() {
        exec_DD_helper<&Z80::handle_opcode_0x6B_LD_L_E>();
    }
    void exec_LD_IXL_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x6C_LD_L_H>();
    }
    void exec_LD_IXL_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x6D_LD_L_L>();
    }
    void exec_LD_L_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x6E_LD_L_HL_ptr>();
    }
    void exec_LD_IXL_A() {
        exec_DD_helper<&Z80::handle_opcode_0x6F_LD_L_A>();
    }
    void exec_LD_IX_d_ptr_B() {
        exec_DD_helper<&Z80::handle_opcode_0x70_LD_HL_ptr_B>();
    }
    void exec_LD_IX_d_ptr_C() {
        exec_DD_helper<&Z80::handle_opcode_0x71_LD_HL_ptr_C>();
    }
    void exec_LD_IX_d_ptr_D() {
        exec_DD_helper<&Z80::handle_opcode_0x72_LD_HL_ptr_D>();
    }
    void exec_LD_IX_d_ptr_E() {
        exec_DD_helper<&Z80::handle_opcode_0x73_LD_HL_ptr_E>();
    }
    void exec_LD_IX_d_ptr_H() {
        exec_DD_helper<&Z80::handle_opcode_0x74_LD_HL_ptr_H>();
    }
    void exec_LD_IX_d_ptr_L() {
        exec_DD_helper<&Z80::handle_opcode_0x75_LD_HL_ptr_L>();
    }
    void exec_LD_IX_d_ptr_A() {
        exec_DD_helper<&Z80::handle_opcode_0x77_LD_HL_ptr_A>();
    }
    void exec_LD_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x7C_LD_A_H>();
    }
    void exec_LD_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x7D_LD_A_L>();
    }
    void exec_LD_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x7E_LD_A_HL_ptr>();
    }
    void exec_ADD_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x84_ADD_A_H>();
    }
    void exec_ADD_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x85_ADD_A_L>();
    }
    void exec_ADD_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x86_ADD_A_HL_ptr>();
    }
    void exec_ADC_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x8C_ADC_A_H>();
    }
    void exec_ADC_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x8D_ADC_A_L>();
    }
    void exec_ADC_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x8E_ADC_A_HL_ptr>();
    }
    void exec_SUB_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x94_SUB_H>();
    }
    void exec_SUB_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x95_SUB_L>();
    }
    void exec_SUB_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x96_SUB_HL_ptr>();
    }
    void exec_SBC_A_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0x9C_SBC_A_H>();
    }
    void exec_SBC_A_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0x9D_SBC_A_L>();
    }
    void exec_SBC_A_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0x9E_SBC_A_HL_ptr>();
    }
    void exec_AND_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xA4_AND_H>();
    }
    void exec_AND_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xA5_AND_L>();
    }
    void exec_AND_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xA6_AND_HL_ptr>();
    }
    void exec_XOR_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xAC_XOR_H>();
    }
    void exec_XOR_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xAD_XOR_L>();
    }
    void exec_XOR_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xAE_XOR_HL_ptr>();
    }
    void exec_OR_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xB4_OR_H>();
    }
    void exec_OR_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xB5_OR_L>();
    }
    void exec_OR_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xB6_OR_HL_ptr>();
    }
    void exec_CP_IXH() {
        exec_DD_helper<&Z80::handle_opcode_0xBC_CP_H>();
    }
    void exec_CP_IXL() {
        exec_DD_helper<&Z80::handle_opcode_0xBD_CP_L>();
    }
    void exec_CP_IX_d_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xBE_CP_HL_ptr>();
    }
    void exec_POP_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE1_POP_HL>();
    }
    void exec_EX_SP_ptr_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE3_EX_SP_ptr_HL>();
    }
    void exec_PUSH_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xE5_PUSH_HL>();
    }
    void exec_JP_IX_ptr() {
        exec_DD_helper<&Z80::handle_opcode_0xE9_JP_HL_ptr>();
    }
    void exec_LD_SP_IX() {
        exec_DD_helper<&Z80::handle_opcode_0xF9_LD_SP_HL>();
    }

    // --- FD Prefixed Opcodes (IY) ---
    void exec_ADD_IY_BC() {
        exec_FD_helper<&Z80::handle_opcode_0x09_ADD_HL_BC>();
    }
    void exec_ADD_IY_DE() {
        exec_FD_helper<&Z80::handle_opcode_0x19_ADD_HL_DE>();
    }
    void exec_LD_IY_nn() {
        exec_FD_helper<&Z80::handle_opcode_0x21_LD_HL_nn>();
    }
    void exec_LD_nn_ptr_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x22_LD_nn_ptr_HL>();
    }
    void exec_INC_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x23_INC_HL>();
    }
    void exec_INC_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x24_INC_H>();
    }
    void exec_DEC_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x25_DEC_H>();
    }
    void exec_LD_IYH_n() {
        exec_FD_helper<&Z80::handle_opcode_0x26_LD_H_n>();
    }
    void exec_ADD_IY_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x29_ADD_HL_HL>();
    }
    void exec_LD_IY_nn_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x2A_LD_HL_nn_ptr>();
    }
    void exec_DEC_IY() {
        exec_FD_helper<&Z80::handle_opcode_0x2B_DEC_HL>();
    }
    void exec_INC_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x2C_INC_L>();
    }
    void exec_DEC_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x2D_DEC_L>();
    }
    void exec_LD_IYL_n() {
        exec_FD_helper<&Z80::handle_opcode_0x2E_LD_L_n>();
    }
    void exec_INC_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x34_INC_HL_ptr>();
    }
    void exec_DEC_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x35_DEC_HL_ptr>();
    }
    void exec_LD_IY_d_ptr_n() {
        exec_FD_helper<&Z80::handle_opcode_0x36_LD_HL_ptr_n>();
    }
    void exec_ADD_IY_SP() {
        exec_FD_helper<&Z80::handle_opcode_0x39_ADD_HL_SP>();
    }
    void exec_LD_B_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x44_LD_B_H>();
    }
    void exec_LD_B_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x45_LD_B_L>();
    }
    void exec_LD_B_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x46_LD_B_HL_ptr>();
    }
    void exec_LD_C_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x4C_LD_C_H>();
    }
    void exec_LD_C_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x4D_LD_C_L>();
    }
    void exec_LD_C_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x4E_LD_C_HL_ptr>();
    }
    void exec_LD_D_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x54_LD_D_H>();
    }
    void exec_LD_D_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x55_LD_D_L>();
    }
    void exec_LD_D_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x56_LD_D_HL_ptr>();
    }
    void exec_LD_E_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x5C_LD_E_H>();
    }
    void exec_LD_E_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x5D_LD_E_L>();
    }
    void exec_LD_E_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x5E_LD_E_HL_ptr>();
    }
    void exec_LD_IYH_B() {
        exec_FD_helper<&Z80::handle_opcode_0x60_LD_H_B>();
    }
    void exec_LD_IYH_C() {
        exec_FD_helper<&Z80::handle_opcode_0x61_LD_H_C>();
    }
    void exec_LD_IYH_D() {
        exec_FD_helper<&Z80::handle_opcode_0x62_LD_H_D>();
    }
    void exec_LD_IYH_E() {
        exec_FD_helper<&Z80::handle_opcode_0x63_LD_H_E>();
    }
    void exec_LD_IYH_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x64_LD_H_H>();
    }
    void exec_LD_IYH_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x65_LD_H_L>();
    }
    void exec_LD_H_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x66_LD_H_HL_ptr>();
    }
    void exec_LD_IYH_A() {
        exec_FD_helper<&Z80::handle_opcode_0x67_LD_H_A>();
    }
    void exec_LD_IYL_B() {
        exec_FD_helper<&Z80::handle_opcode_0x68_LD_L_B>();
    }
    void exec_LD_IYL_C() {
        exec_FD_helper<&Z80::handle_opcode_0x69_LD_L_C>();
    }
    void exec_LD_IYL_D() {
        exec_FD_helper<&Z80::handle_opcode_0x6A_LD_L_D>();
    }
    void exec_LD_IYL_E() {
        exec_FD_helper<&Z80::handle_opcode_0x6B_LD_L_E>();
    }
    void exec_LD_IYL_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x6C_LD_L_H>();
    }
    void exec_LD_IYL_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x6D_LD_L_L>();
    }
    void exec_LD_L_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x6E_LD_L_HL_ptr>();
    }
    void exec_LD_IYL_A() {
        exec_FD_helper<&Z80::handle_opcode_0x6F_LD_L_A>();
    }
    void exec_LD_IY_d_ptr_B() {
        exec_FD_helper<&Z80::handle_opcode_0x70_LD_HL_ptr_B>();
    }
    void exec_LD_IY_d_ptr_C() {
        exec_FD_helper<&Z80::handle_opcode_0x71_LD_HL_ptr_C>();
    }
    void exec_LD_IY_d_ptr_D() {
        exec_FD_helper<&Z80::handle_opcode_0x72_LD_HL_ptr_D>();
    }
    void exec_LD_IY_d_ptr_E() {
        exec_FD_helper<&Z80::handle_opcode_0x73_LD_HL_ptr_E>();
    }
    void exec_LD_IY_d_ptr_H() {
        exec_FD_helper<&Z80::handle_opcode_0x74_LD_HL_ptr_H>();
    }
    void exec_LD_IY_d_ptr_L() {
        exec_FD_helper<&Z80::handle_opcode_0x75_LD_HL_ptr_L>();
    }
    void exec_LD_IY_d_ptr_A() {
        exec_FD_helper<&Z80::handle_opcode_0x77_LD_HL_ptr_A>();
    }
    void exec_LD_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x7C_LD_A_H>();
    }
    void exec_LD_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x7D_LD_A_L>();
    }
    void exec_LD_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x7E_LD_A_HL_ptr>();
    }
    void exec_ADD_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x84_ADD_A_H>();
    }
    void exec_ADD_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x85_ADD_A_L>();
    }
    void exec_ADD_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x86_ADD_A_HL_ptr>();
    }
    void exec_ADC_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x8C_ADC_A_H>();
    }
    void exec_ADC_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x8D_ADC_A_L>();
    }
    void exec_ADC_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x8E_ADC_A_HL_ptr>();
    }
    void exec_SUB_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x94_SUB_H>();
    }
    void exec_SUB_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x95_SUB_L>();
    }
    void exec_SUB_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x96_SUB_HL_ptr>();
    }
    void exec_SBC_A_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0x9C_SBC_A_H>();
    }
    void exec_SBC_A_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0x9D_SBC_A_L>();
    }
    void exec_SBC_A_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0x9E_SBC_A_HL_ptr>();
    }
    void exec_AND_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xA4_AND_H>();
    }
    void exec_AND_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xA5_AND_L>();
    }
    void exec_AND_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xA6_AND_HL_ptr>();
    }
    void exec_XOR_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xAC_XOR_H>();
    }
    void exec_XOR_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xAD_XOR_L>();
    }
    void exec_XOR_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xAE_XOR_HL_ptr>();
    }
    void exec_OR_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xB4_OR_H>();
    }
    void exec_OR_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xB5_OR_L>();
    }
    void exec_OR_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xB6_OR_HL_ptr>();
    }
    void exec_CP_IYH() {
        exec_FD_helper<&Z80::handle_opcode_0xBC_CP_H>();
    }
    void exec_CP_IYL() {
        exec_FD_helper<&Z80::handle_opcode_0xBD_CP_L>();
    }
    void exec_CP_IY_d_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xBE_CP_HL_ptr>();
    }
    void exec_POP_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE1_POP_HL>();
    }
    void exec_EX_SP_ptr_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE3_EX_SP_ptr_HL>();
    }
    void exec_PUSH_IY() {
        exec_FD_helper<&Z80::handle_opcode_0xE5_PUSH_HL>();
    }
    void exec_JP_IY_ptr() {
        exec_FD_helper<&Z80::handle_opcode_0xE9_JP_HL_ptr>();
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

    // --- CB Prefixed Opcodes ---
    void exec_RLC_B()  {
        exec_CB_helper(0x00);
    }
    void exec_RLC_C()  {
        exec_CB_helper(0x01);
    }
    void exec_RLC_D()  {
        exec_CB_helper(0x02);
    }
    void exec_RLC_E()  {
        exec_CB_helper(0x03);
    }
    void exec_RLC_H()  {
        exec_CB_helper(0x04);
    }
    void exec_RLC_L()  {
        exec_CB_helper(0x05);
    }
    void exec_RLC_HL_ptr()  {
        exec_CB_helper(0x06);
    }
    void exec_RLC_A()  {
        exec_CB_helper(0x07);
    }
    void exec_RRC_B()  {
        exec_CB_helper(0x08);
    }
    void exec_RRC_C()  {
        exec_CB_helper(0x09);
    }
    void exec_RRC_D()  {
        exec_CB_helper(0x0A);
    }
    void exec_RRC_E()  {
        exec_CB_helper(0x0B);
    }
    void exec_RRC_H()  {
        exec_CB_helper(0x0C);
    }
    void exec_RRC_L()  {
        exec_CB_helper(0x0D);
    }
    void exec_RRC_HL_ptr()  {
        exec_CB_helper(0x0E);
    }
    void exec_RRC_A()  {
        exec_CB_helper(0x0F);
    }
    void exec_RL_B()  {
        exec_CB_helper(0x10);
    }
    void exec_RL_C()  {
        exec_CB_helper(0x11);
    }
    void exec_RL_D()  {
        exec_CB_helper(0x12);
    }
    void exec_RL_E()  {
        exec_CB_helper(0x13);
    }
    void exec_RL_H()  {
        exec_CB_helper(0x14);
    }
    void exec_RL_L()  {
        exec_CB_helper(0x15);
    }
    void exec_RL_HL_ptr()  {
        exec_CB_helper(0x16);
    }
    void exec_RL_A()  {
        exec_CB_helper(0x17);
    }
    void exec_RR_B()  {
        exec_CB_helper(0x18);
    }
    void exec_RR_C()  {
        exec_CB_helper(0x19);
    }
    void exec_RR_D()  {
        exec_CB_helper(0x1A);
    }
    void exec_RR_E()  {
        exec_CB_helper(0x1B);
    }
    void exec_RR_H()  {
        exec_CB_helper(0x1C);
    }
    void exec_RR_L()  {
        exec_CB_helper(0x1D);
    }
    void exec_RR_HL_ptr()  {
        exec_CB_helper(0x1E);
    }
    void exec_RR_A()  {
        exec_CB_helper(0x1F);
    }
    void exec_SLA_B()  {
        exec_CB_helper(0x20);
    }
    void exec_SLA_C()  {
        exec_CB_helper(0x21);
    }
    void exec_SLA_D()  {
        exec_CB_helper(0x22);
    }
    void exec_SLA_E()  {
        exec_CB_helper(0x23);
    }
    void exec_SLA_H()  {
        exec_CB_helper(0x24);
    }
    void exec_SLA_L()  {
        exec_CB_helper(0x25);
    }
    void exec_SLA_HL_ptr()  {
        exec_CB_helper(0x26);
    }
    void exec_SLA_A()  {
        exec_CB_helper(0x27);
    }
    void exec_SRA_B()  {
        exec_CB_helper(0x28);
    }
    void exec_SRA_C()  {
        exec_CB_helper(0x29);
    }
    void exec_SRA_D()  {
        exec_CB_helper(0x2A);
    }
    void exec_SRA_E()  {
        exec_CB_helper(0x2B);
    }
    void exec_SRA_H()  {
        exec_CB_helper(0x2C);
    }
    void exec_SRA_L()  {
        exec_CB_helper(0x2D);
    }
    void exec_SRA_HL_ptr()  {
        exec_CB_helper(0x2E);
    }
    void exec_SRA_A()  {
        exec_CB_helper(0x2F);
    }
    void exec_SLL_B()  {
        exec_CB_helper(0x30);
    }
    void exec_SLL_C()  {
        exec_CB_helper(0x31);
    }
    void exec_SLL_D()  {
        exec_CB_helper(0x32);
    }
    void exec_SLL_E()  {
        exec_CB_helper(0x33);
    }
    void exec_SLL_H()  {
        exec_CB_helper(0x34);
    }
    void exec_SLL_L()  {
        exec_CB_helper(0x35);
    }
    void exec_SLL_HL_ptr()  {
        exec_CB_helper(0x36);
    }
    void exec_SLL_A()  {
        exec_CB_helper(0x37);
    }
    void exec_SRL_B()  {
        exec_CB_helper(0x38);
    }
    void exec_SRL_C()  {
        exec_CB_helper(0x39);
    }
    void exec_SRL_D()  {
        exec_CB_helper(0x3A);
    }
    void exec_SRL_E()  {
        exec_CB_helper(0x3B);
    }
    void exec_SRL_H()  {
        exec_CB_helper(0x3C);
    }
    void exec_SRL_L()  {
        exec_CB_helper(0x3D);
    }
    void exec_SRL_HL_ptr()  {
        exec_CB_helper(0x3E);
    }
    void exec_SRL_A()  {
        exec_CB_helper(0x3F);
    }
    void exec_BIT_0_B()  {
        exec_CB_helper(0x40);
    }
    void exec_BIT_0_C()  {
        exec_CB_helper(0x41);
    }
    void exec_BIT_0_D()  {
        exec_CB_helper(0x42);
    }
    void exec_BIT_0_E()  {
        exec_CB_helper(0x43);
    }
    void exec_BIT_0_H()  {
        exec_CB_helper(0x44);
    }
    void exec_BIT_0_L()  {
        exec_CB_helper(0x45);
    }
    void exec_BIT_0_HL_ptr()  {
        exec_CB_helper(0x46);
    }
    void exec_BIT_0_A()  {
        exec_CB_helper(0x47);
    }
    void exec_BIT_1_B()  {
        exec_CB_helper(0x48);
    }
    void exec_BIT_1_C()  {
        exec_CB_helper(0x49);
    }
    void exec_BIT_1_D()  {
        exec_CB_helper(0x4A);
    }
    void exec_BIT_1_E()  {
        exec_CB_helper(0x4B);
    }
    void exec_BIT_1_H()  {
        exec_CB_helper(0x4C);
    }
    void exec_BIT_1_L()  {
        exec_CB_helper(0x4D);
    }
    void exec_BIT_1_HL_ptr()  {
        exec_CB_helper(0x4E);
    }
    void exec_BIT_1_A()  {
        exec_CB_helper(0x4F);
    }
    void exec_BIT_2_B()  {
        exec_CB_helper(0x50);
    }
    void exec_BIT_2_C()  {
        exec_CB_helper(0x51);
    }
    void exec_BIT_2_D()  {
        exec_CB_helper(0x52);
    }
    void exec_BIT_2_E()  {
        exec_CB_helper(0x53);
    }
    void exec_BIT_2_H()  {
        exec_CB_helper(0x54);
    }
    void exec_BIT_2_L()  {
        exec_CB_helper(0x55);
    }
    void exec_BIT_2_HL_ptr()  {
        exec_CB_helper(0x56);
    }
    void exec_BIT_2_A()  {
        exec_CB_helper(0x57);
    }
    void exec_BIT_3_B()  {
        exec_CB_helper(0x58);
    }
    void exec_BIT_3_C()  {
        exec_CB_helper(0x59);
    }
    void exec_BIT_3_D()  {
        exec_CB_helper(0x5A);
    }
    void exec_BIT_3_E()  {
        exec_CB_helper(0x5B);
    }
    void exec_BIT_3_H()  {
        exec_CB_helper(0x5C);
    }
    void exec_BIT_3_L()  {
        exec_CB_helper(0x5D);
    }
    void exec_BIT_3_HL_ptr()  {
        exec_CB_helper(0x5E);
    }
    void exec_BIT_3_A()  {
        exec_CB_helper(0x5F);
    }
    void exec_BIT_4_B()  {
        exec_CB_helper(0x60);
    }
    void exec_BIT_4_C()  {
        exec_CB_helper(0x61);
    }
    void exec_BIT_4_D()  {
        exec_CB_helper(0x62);
    }
    void exec_BIT_4_E()  {
        exec_CB_helper(0x63);
    }
    void exec_BIT_4_H()  {
        exec_CB_helper(0x64);
    }
    void exec_BIT_4_L()  {
        exec_CB_helper(0x65);
    }
    void exec_BIT_4_HL_ptr()  {
        exec_CB_helper(0x66);
    }
    void exec_BIT_4_A()  {
        exec_CB_helper(0x67);
    }
    void exec_BIT_5_B()  {
        exec_CB_helper(0x68);
    }
    void exec_BIT_5_C()  {
        exec_CB_helper(0x69);
    }
    void exec_BIT_5_D()  {
        exec_CB_helper(0x6A);
    }
    void exec_BIT_5_E()  {
        exec_CB_helper(0x6B);
    }
    void exec_BIT_5_H()  {
        exec_CB_helper(0x6C);
    }
    void exec_BIT_5_L()  {
        exec_CB_helper(0x6D);
    }
    void exec_BIT_5_HL_ptr()  {
        exec_CB_helper(0x6E);
    }
    void exec_BIT_5_A()  {
        exec_CB_helper(0x6F);
    }
    void exec_BIT_6_B()  {
        exec_CB_helper(0x70);
    }
    void exec_BIT_6_C()  {
        exec_CB_helper(0x71);
    }
    void exec_BIT_6_D()  {
        exec_CB_helper(0x72);
    }
    void exec_BIT_6_E()  {
        exec_CB_helper(0x73);
    }
    void exec_BIT_6_H()  {
        exec_CB_helper(0x74);
    }
    void exec_BIT_6_L()  {
        exec_CB_helper(0x75);
    }
    void exec_BIT_6_HL_ptr()  {
        exec_CB_helper(0x76);
    }
    void exec_BIT_6_A()  {
        exec_CB_helper(0x77);
    }
    void exec_BIT_7_B()  {
        exec_CB_helper(0x78);
    }
    void exec_BIT_7_C()  {
        exec_CB_helper(0x79);
    }
    void exec_BIT_7_D()  {
        exec_CB_helper(0x7A);
    }
    void exec_BIT_7_E()  {
        exec_CB_helper(0x7B);
    }
    void exec_BIT_7_H()  {
        exec_CB_helper(0x7C);
    }
    void exec_BIT_7_L()  {
        exec_CB_helper(0x7D);
    }
    void exec_BIT_7_HL_ptr()  {
        exec_CB_helper(0x7E);
    }
    void exec_BIT_7_A()  {
        exec_CB_helper(0x7F);
    }
    void exec_RES_0_B()  {
        exec_CB_helper(0x80);
    }
    void exec_RES_0_C()  {
        exec_CB_helper(0x81);
    }
    void exec_RES_0_D()  {
        exec_CB_helper(0x82);
    }
    void exec_RES_0_E()  {
        exec_CB_helper(0x83);
    }
    void exec_RES_0_H()  {
        exec_CB_helper(0x84);
    }
    void exec_RES_0_L()  {
        exec_CB_helper(0x85);
    }
    void exec_RES_0_HL_ptr()  {
        exec_CB_helper(0x86);
    }
    void exec_RES_0_A()  {
        exec_CB_helper(0x87);
    }
    void exec_RES_1_B()  {
        exec_CB_helper(0x88);
    }
    void exec_RES_1_C()  {
        exec_CB_helper(0x89);
    }
    void exec_RES_1_D()  {
        exec_CB_helper(0x8A);
    }
    void exec_RES_1_E()  {
        exec_CB_helper(0x8B);
    }
    void exec_RES_1_H()  {
        exec_CB_helper(0x8C);
    }
    void exec_RES_1_L()  {
        exec_CB_helper(0x8D);
    }
    void exec_RES_1_HL_ptr()  {
        exec_CB_helper(0x8E);
    }
    void exec_RES_1_A()  {
        exec_CB_helper(0x8F);
    }
    void exec_RES_2_B()  {
        exec_CB_helper(0x90);
    }
    void exec_RES_2_C()  {
        exec_CB_helper(0x91);
    }
    void exec_RES_2_D()  {
        exec_CB_helper(0x92);
    }
    void exec_RES_2_E()  {
        exec_CB_helper(0x93);
    }
    void exec_RES_2_H()  {
        exec_CB_helper(0x94);
    }
    void exec_RES_2_L()  {
        exec_CB_helper(0x95);
    }
    void exec_RES_2_HL_ptr()  {
        exec_CB_helper(0x96);
    }
    void exec_RES_2_A()  {
        exec_CB_helper(0x97);
    }
    void exec_RES_3_B()  {
        exec_CB_helper(0x98);
    }
    void exec_RES_3_C()  {
        exec_CB_helper(0x99);
    }
    void exec_RES_3_D()  {
        exec_CB_helper(0x9A);
    }
    void exec_RES_3_E()  {
        exec_CB_helper(0x9B);
    }
    void exec_RES_3_H()  {
        exec_CB_helper(0x9C);
    }
    void exec_RES_3_L()  {
        exec_CB_helper(0x9D);
    }
    void exec_RES_3_HL_ptr()  {
        exec_CB_helper(0x9E);
    }
    void exec_RES_3_A()  {
        exec_CB_helper(0x9F);
    }
    void exec_RES_4_B()  {
        exec_CB_helper(0xA0);
    }
    void exec_RES_4_C()  {
        exec_CB_helper(0xA1);
    }
    void exec_RES_4_D()  {
        exec_CB_helper(0xA2);
    }
    void exec_RES_4_E()  {
        exec_CB_helper(0xA3);
    }
    void exec_RES_4_H()  {
        exec_CB_helper(0xA4);
    }
    void exec_RES_4_L()  {
        exec_CB_helper(0xA5);
    }
    void exec_RES_4_HL_ptr()  {
        exec_CB_helper(0xA6);
    }
    void exec_RES_4_A()  {
        exec_CB_helper(0xA7);
    }
    void exec_RES_5_B()  {
        exec_CB_helper(0xA8);
    }
    void exec_RES_5_C()  {
        exec_CB_helper(0xA9);
    }
    void exec_RES_5_D()  {
        exec_CB_helper(0xAA);
    }
    void exec_RES_5_E()  {
        exec_CB_helper(0xAB);
    }
    void exec_RES_5_H()  {
        exec_CB_helper(0xAC);
    }
    void exec_RES_5_L()  {
        exec_CB_helper(0xAD);
    }
    void exec_RES_5_HL_ptr()  {
        exec_CB_helper(0xAE);
    }
    void exec_RES_5_A()  {
        exec_CB_helper(0xAF);
    }
    void exec_RES_6_B()  {
        exec_CB_helper(0xB0);
    }
    void exec_RES_6_C()  {
        exec_CB_helper(0xB1);
    }
    void exec_RES_6_D()  {
        exec_CB_helper(0xB2);
    }
    void exec_RES_6_E()  {
        exec_CB_helper(0xB3);
    }
    void exec_RES_6_H()  {
        exec_CB_helper(0xB4);
    }
    void exec_RES_6_L()  {
        exec_CB_helper(0xB5);
    }
    void exec_RES_6_HL_ptr()  {
        exec_CB_helper(0xB6);
    }
    void exec_RES_6_A()  {
        exec_CB_helper(0xB7);
    }
    void exec_RES_7_B()  {
        exec_CB_helper(0xB8);
    }
    void exec_RES_7_C()  {
        exec_CB_helper(0xB9);
    }
    void exec_RES_7_D()  {
        exec_CB_helper(0xBA);
    }
    void exec_RES_7_E()  {
        exec_CB_helper(0xBB);
    }
    void exec_RES_7_H()  {
        exec_CB_helper(0xBC);
    }
    void exec_RES_7_L()  {
        exec_CB_helper(0xBD);
    }
    void exec_RES_7_HL_ptr()  {
        exec_CB_helper(0xBE);
    }
    void exec_RES_7_A()  {
        exec_CB_helper(0xBF);
    }
    void exec_SET_0_B()  {
        exec_CB_helper(0xC0);
    }
    void exec_SET_0_C()  {
        exec_CB_helper(0xC1);
    }
    void exec_SET_0_D()  {
        exec_CB_helper(0xC2);
    }
    void exec_SET_0_E()  {
        exec_CB_helper(0xC3);
    }
    void exec_SET_0_H()  {
        exec_CB_helper(0xC4);
    }
    void exec_SET_0_L()  {
        exec_CB_helper(0xC5);
    }
    void exec_SET_0_HL_ptr()  {
        exec_CB_helper(0xC6);
    }
    void exec_SET_0_A()  {
        exec_CB_helper(0xC7);
    }
    void exec_SET_1_B()  {
        exec_CB_helper(0xC8);
    }
    void exec_SET_1_C()  {
        exec_CB_helper(0xC9);
    }
    void exec_SET_1_D()  {
        exec_CB_helper(0xCA);
    }
    void exec_SET_1_E()  {
        exec_CB_helper(0xCB);
    }
    void exec_SET_1_H()  {
        exec_CB_helper(0xCC);
    }
    void exec_SET_1_L()  {
        exec_CB_helper(0xCD);
    }
    void exec_SET_1_HL_ptr()  {
        exec_CB_helper(0xCE);
    }
    void exec_SET_1_A()  {
        exec_CB_helper(0xCF);
    }
    void exec_SET_2_B()  {
        exec_CB_helper(0xD0);
    }
    void exec_SET_2_C()  {
        exec_CB_helper(0xD1);
    }
    void exec_SET_2_D()  {
        exec_CB_helper(0xD2);
    }
    void exec_SET_2_E()  {
        exec_CB_helper(0xD3);
    }
    void exec_SET_2_H()  {
        exec_CB_helper(0xD4);
    }
    void exec_SET_2_L()  {
        exec_CB_helper(0xD5);
    }
    void exec_SET_2_HL_ptr()  {
        exec_CB_helper(0xD6);
    }
    void exec_SET_2_A()  {
        exec_CB_helper(0xD7);
    }
    void exec_SET_3_B()  {
        exec_CB_helper(0xD8);
    }
    void exec_SET_3_C()  {
        exec_CB_helper(0xD9);
    }
    void exec_SET_3_D()  {
        exec_CB_helper(0xDA);
    }
    void exec_SET_3_E()  {
        exec_CB_helper(0xDB);
    }
    void exec_SET_3_H()  {
        exec_CB_helper(0xDC);
    }
    void exec_SET_3_L()  {
        exec_CB_helper(0xDD);
    }
    void exec_SET_3_HL_ptr()  {
        exec_CB_helper(0xDE);
    }
    void exec_SET_3_A()  {
        exec_CB_helper(0xDF);
    }
    void exec_SET_4_B()  {
        exec_CB_helper(0xE0);
    }
    void exec_SET_4_C()  {
        exec_CB_helper(0xE1);
    }
    void exec_SET_4_D()  {
        exec_CB_helper(0xE2);
    }
    void exec_SET_4_E()  {
        exec_CB_helper(0xE3);
    }
    void exec_SET_4_H()  {
        exec_CB_helper(0xE4);
    }
    void exec_SET_4_L()  {
        exec_CB_helper(0xE5);
    }
    void exec_SET_4_HL_ptr()  {
        exec_CB_helper(0xE6);
    }
    void exec_SET_4_A()  {
        exec_CB_helper(0xE7);
    }
    void exec_SET_5_B()  {
        exec_CB_helper(0xE8);
    }
    void exec_SET_5_C()  {
        exec_CB_helper(0xE9);
    }
    void exec_SET_5_D()  {
        exec_CB_helper(0xEA);
    }
    void exec_SET_5_E()  {
        exec_CB_helper(0xEB);
    }
    void exec_SET_5_H()  {
        exec_CB_helper(0xEC);
    }
    void exec_SET_5_L()  {
        exec_CB_helper(0xED);
    }
    void exec_SET_5_HL_ptr()  {
        exec_CB_helper(0xEE);
    }
    void exec_SET_5_A()  {
        exec_CB_helper(0xEF);
    }
    void exec_SET_6_B()  {
        exec_CB_helper(0xF0);
    }
    void exec_SET_6_C()  {
        exec_CB_helper(0xF1);
    }
    void exec_SET_6_D()  {
        exec_CB_helper(0xF2);
    }
    void exec_SET_6_E()  {
        exec_CB_helper(0xF3);
    }
    void exec_SET_6_H()  {
        exec_CB_helper(0xF4);
    }
    void exec_SET_6_L()  {
        exec_CB_helper(0xF5);
    }
    void exec_SET_6_HL_ptr()  {
        exec_CB_helper(0xF6);
    }
    void exec_SET_6_A()  {
        exec_CB_helper(0xF7);
    }
    void exec_SET_7_B()  {
        exec_CB_helper(0xF8);
    }
    void exec_SET_7_C()  {
        exec_CB_helper(0xF9);
    }
    void exec_SET_7_D()  {
        exec_CB_helper(0xFA);
    }
    void exec_SET_7_E()  {
        exec_CB_helper(0xFB);
    }
    void exec_SET_7_H()  {
        exec_CB_helper(0xFC);
    }
    void exec_SET_7_L()  {
        exec_CB_helper(0xFD);
    }
    void exec_SET_7_HL_ptr()  {
        exec_CB_helper(0xFE);
    }
    void exec_SET_7_A()  {
        exec_CB_helper(0xFF);
    }

    // --- DDCB Prefixed Opcodes ---
    void exec_RLC_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x00);
    }
    void exec_RLC_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x01);
    }
    void exec_RLC_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x02);
    }
    void exec_RLC_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x03);
    }
    void exec_RLC_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x04);
    }
    void exec_RLC_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x05);
    }
    void exec_RLC_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x06);
    }
    void exec_RLC_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x07);
    }
    void exec_RRC_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x08);
    }
    void exec_RRC_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x09);
    }
    void exec_RRC_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0A);
    }
    void exec_RRC_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0B);
    }
    void exec_RRC_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0C);
    }
    void exec_RRC_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0D);
    }
    void exec_RRC_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0E);
    }
    void exec_RRC_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x0F);
    }
    void exec_RL_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x10);
    }
    void exec_RL_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x11);
    }
    void exec_RL_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x12);
    }
    void exec_RL_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x13);
    }
    void exec_RL_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x14);
    }
    void exec_RL_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x15);
    }
    void exec_RL_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x16);
    }
    void exec_RL_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x17);
    }
    void exec_RR_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x18);
    }
    void exec_RR_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x19);
    }
    void exec_RR_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1A);
    }
    void exec_RR_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1B);
    }
    void exec_RR_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1C);
    }
    void exec_RR_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1D);
    }
    void exec_RR_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1E);
    }
    void exec_RR_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x1F);
    }
    void exec_SLA_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x20);
    }
    void exec_SLA_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x21);
    }
    void exec_SLA_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x22);
    }
    void exec_SLA_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x23);
    }
    void exec_SLA_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x24);
    }
    void exec_SLA_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x25);
    }
    void exec_SLA_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x26);
    }
    void exec_SLA_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x27);
    }
    void exec_SRA_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x28);
    }
    void exec_SRA_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x29);
    }
    void exec_SRA_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2A);
    }
    void exec_SRA_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2B);
    }
    void exec_SRA_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2C);
    }
    void exec_SRA_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2D);
    }
    void exec_SRA_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2E);
    }
    void exec_SRA_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x2F);
    }
    void exec_SLL_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x30);
    }
    void exec_SLL_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x31);
    }
    void exec_SLL_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x32);
    }
    void exec_SLL_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x33);
    }
    void exec_SLL_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x34);
    }
    void exec_SLL_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x35);
    }
    void exec_SLL_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x36);
    }
    void exec_SLL_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x37);
    }
    void exec_SRL_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x38);
    }
    void exec_SRL_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x39);
    }
    void exec_SRL_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3A);
    }
    void exec_SRL_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3B);
    }
    void exec_SRL_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3C);
    }
    void exec_SRL_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3D);
    }
    void exec_SRL_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3E);
    }
    void exec_SRL_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x3F);
    }
    void exec_BIT_0_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x46);
    }
    void exec_BIT_1_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x4E);
    }
    void exec_BIT_2_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x56);
    }
    void exec_BIT_3_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x5E);
    }
    void exec_BIT_4_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x66);
    }
    void exec_BIT_5_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x6E);
    }
    void exec_BIT_6_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x76);
    }
    void exec_BIT_7_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x7E);
    }
    void exec_RES_0_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x80);
    }
    void exec_RES_0_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x81);
    }
    void exec_RES_0_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x82);
    }
    void exec_RES_0_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x83);
    }
    void exec_RES_0_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x84);
    }
    void exec_RES_0_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x85);
    }
    void exec_RES_0_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x86);
    }
    void exec_RES_0_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x87);
    }
    void exec_RES_1_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x88);
    }
    void exec_RES_1_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x89);
    }
    void exec_RES_1_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8A);
    }
    void exec_RES_1_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8B);
    }
    void exec_RES_1_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8C);
    }
    void exec_RES_1_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8D);
    }
    void exec_RES_1_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8E);
    }
    void exec_RES_1_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x8F);
    }
    void exec_RES_2_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x90);
    }
    void exec_RES_2_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x91);
    }
    void exec_RES_2_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x92);
    }
    void exec_RES_2_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x93);
    }
    void exec_RES_2_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x94);
    }
    void exec_RES_2_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x95);
    }
    void exec_RES_2_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x96);
    }
    void exec_RES_2_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x97);
    }
    void exec_RES_3_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0x98);
    }
    void exec_RES_3_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0x99);
    }
    void exec_RES_3_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9A);
    }
    void exec_RES_3_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9B);
    }
    void exec_RES_3_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9C);
    }
    void exec_RES_3_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9D);
    }
    void exec_RES_3_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9E);
    }
    void exec_RES_3_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0x9F);
    }
    void exec_RES_4_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA0);
    }
    void exec_RES_4_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA1);
    }
    void exec_RES_4_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA2);
    }
    void exec_RES_4_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA3);
    }
    void exec_RES_4_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA4);
    }
    void exec_RES_4_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA5);
    }
    void exec_RES_4_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA6);
    }
    void exec_RES_4_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA7);
    }
    void exec_RES_5_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA8);
    }
    void exec_RES_5_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xA9);
    }
    void exec_RES_5_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAA);
    }
    void exec_RES_5_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAB);
    }
    void exec_RES_5_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAC);
    }
    void exec_RES_5_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAD);
    }
    void exec_RES_5_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAE);
    }
    void exec_RES_5_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xAF);
    }
    void exec_RES_6_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB0);
    }
    void exec_RES_6_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB1);
    }
    void exec_RES_6_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB2);
    }
    void exec_RES_6_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB3);
    }
    void exec_RES_6_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB4);
    }
    void exec_RES_6_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB5);
    }
    void exec_RES_6_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB6);
    }
    void exec_RES_6_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB7);
    }
    void exec_RES_7_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB8);
    }
    void exec_RES_7_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xB9);
    }
    void exec_RES_7_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBA);
    }
    void exec_RES_7_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBB);
    }
    void exec_RES_7_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBC);
    }
    void exec_RES_7_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBD);
    }
    void exec_RES_7_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBE);
    }
    void exec_RES_7_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xBF);
    }
    void exec_SET_0_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC0);
    }
    void exec_SET_0_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC1);
    }
    void exec_SET_0_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC2);
    }
    void exec_SET_0_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC3);
    }
    void exec_SET_0_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC4);
    }
    void exec_SET_0_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC5);
    }
    void exec_SET_0_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC6);
    }
    void exec_SET_0_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC7);
    }
    void exec_SET_1_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC8);
    }
    void exec_SET_1_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xC9);
    }
    void exec_SET_1_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCA);
    }
    void exec_SET_1_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCB);
    }
    void exec_SET_1_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCC);
    }
    void exec_SET_1_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCD);
    }
    void exec_SET_1_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCE);
    }
    void exec_SET_1_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xCF);
    }
    void exec_SET_2_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD0);
    }
    void exec_SET_2_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD1);
    }
    void exec_SET_2_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD2);
    }
    void exec_SET_2_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD3);
    }
    void exec_SET_2_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD4);
    }
    void exec_SET_2_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD5);
    }
    void exec_SET_2_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD6);
    }
    void exec_SET_2_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD7);
    }
    void exec_SET_3_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD8);
    }
    void exec_SET_3_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xD9);
    }
    void exec_SET_3_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDA);
    }
    void exec_SET_3_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDB);
    }
    void exec_SET_3_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDC);
    }
    void exec_SET_3_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDD);
    }
    void exec_SET_3_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDE);
    }
    void exec_SET_3_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xDF);
    }
    void exec_SET_4_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE0);
    }
    void exec_SET_4_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE1);
    }
    void exec_SET_4_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE2);
    }
    void exec_SET_4_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE3);
    }
    void exec_SET_4_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE4);
    }
    void exec_SET_4_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE5);
    }
    void exec_SET_4_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE6);
    }
    void exec_SET_4_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE7);
    }
    void exec_SET_5_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE8);
    }
    void exec_SET_5_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xE9);
    }
    void exec_SET_5_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xEA);
    }
    void exec_SET_5_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xEB);
    }
    void exec_SET_5_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xEC);
    }
    void exec_SET_5_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xED);
    }
    void exec_SET_5_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xEE);
    }
    void exec_SET_5_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xEF);
    }
    void exec_SET_6_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF0);
    }
    void exec_SET_6_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF1);
    }
    void exec_SET_6_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF2);
    }
    void exec_SET_6_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF3);
    }
    void exec_SET_6_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF4);
    }
    void exec_SET_6_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF5);
    }
    void exec_SET_6_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF6);
    }
    void exec_SET_6_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF7);
    }
    void exec_SET_7_IX_d_ptr_B(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF8);
    }
    void exec_SET_7_IX_d_ptr_C(int8_t offset)  {
        exec_DDCB_helper(offset, 0xF9);
    }
    void exec_SET_7_IX_d_ptr_D(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFA);
    }
    void exec_SET_7_IX_d_ptr_E(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFB);
    }
    void exec_SET_7_IX_d_ptr_H(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFC);
    }
    void exec_SET_7_IX_d_ptr_L(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFD);
    }
    void exec_SET_7_IX_d_ptr(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFE);
    }
    void exec_SET_7_IX_d_ptr_A(int8_t offset)  {
        exec_DDCB_helper(offset, 0xFF);
    }

    // --- FDCB Prefixed Opcodes ---
    void exec_RLC_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x00);
    }
    void exec_RLC_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x01);
    }
    void exec_RLC_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x02);
    }
    void exec_RLC_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x03);
    }
    void exec_RLC_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x04);
    }
    void exec_RLC_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x05);
    }
    void exec_RLC_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x06);
    }
    void exec_RLC_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x07);
    }
    void exec_RRC_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x08);
    }
    void exec_RRC_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x09);
    }
    void exec_RRC_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0A);
    }
    void exec_RRC_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0B);
    }
    void exec_RRC_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0C);
    }
    void exec_RRC_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0D);
    }
    void exec_RRC_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0E);
    }
    void exec_RRC_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x0F);
    }
    void exec_RL_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x10);
    }
    void exec_RL_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x11);
    }
    void exec_RL_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x12);
    }
    void exec_RL_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x13);
    }
    void exec_RL_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x14);
    }
    void exec_RL_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x15);
    }
    void exec_RL_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x16);
    }
    void exec_RL_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x17);
    }
    void exec_RR_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x18);
    }
    void exec_RR_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x19);
    }
    void exec_RR_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1A);
    }
    void exec_RR_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1B);
    }
    void exec_RR_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1C);
    }
    void exec_RR_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1D);
    }
    void exec_RR_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1E);
    }
    void exec_RR_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x1F);
    }
    void exec_SLA_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x20);
    }
    void exec_SLA_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x21);
    }
    void exec_SLA_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x22);
    }
    void exec_SLA_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x23);
    }
    void exec_SLA_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x24);
    }
    void exec_SLA_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x25);
    }
    void exec_SLA_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x26);
    }
    void exec_SLA_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x27);
    }
    void exec_SRA_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x28);
    }
    void exec_SRA_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x29);
    }
    void exec_SRA_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2A);
    }
    void exec_SRA_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2B);
    }
    void exec_SRA_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2C);
    }
    void exec_SRA_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2D);
    }
    void exec_SRA_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2E);
    }
    void exec_SRA_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x2F);
    }
    void exec_SLL_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x30);
    }
    void exec_SLL_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x31);
    }
    void exec_SLL_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x32);
    }
    void exec_SLL_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x33);
    }
    void exec_SLL_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x34);
    }
    void exec_SLL_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x35);
    }
    void exec_SLL_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x36);
    }
    void exec_SLL_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x37);
    }
    void exec_SRL_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x38);
    }
    void exec_SRL_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x39);
    }
    void exec_SRL_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3A);
    }
    void exec_SRL_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3B);
    }
    void exec_SRL_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3C);
    }
    void exec_SRL_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3D);
    }
    void exec_SRL_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3E);
    }
    void exec_SRL_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x3F);
    }
    void exec_BIT_0_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x46);
    }
    void exec_BIT_1_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x4E);
    }
    void exec_BIT_2_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x56);
    }
    void exec_BIT_3_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x5E);
    }
    void exec_BIT_4_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x66);
    }
    void exec_BIT_5_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x6E);
    }
    void exec_BIT_6_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x76);
    }
    void exec_BIT_7_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x7E);
    }
    void exec_RES_0_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x80);
    }
    void exec_RES_0_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x81);
    }
    void exec_RES_0_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x82);
    }
    void exec_RES_0_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x83);
    }
    void exec_RES_0_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x84);
    }
    void exec_RES_0_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x85);
    }
    void exec_RES_0_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x86);
    }
    void exec_RES_0_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x87);
    }
    void exec_RES_1_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x88);
    }
    void exec_RES_1_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x89);
    }
    void exec_RES_1_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8A);
    }
    void exec_RES_1_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8B);
    }
    void exec_RES_1_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8C);
    }
    void exec_RES_1_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8D);
    }
    void exec_RES_1_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8E);
    }
    void exec_RES_1_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x8F);
    }
    void exec_RES_2_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x90);
    }
    void exec_RES_2_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x91);
    }
    void exec_RES_2_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x92);
    }
    void exec_RES_2_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x93);
    }
    void exec_RES_2_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x94);
    }
    void exec_RES_2_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x95);
    }
    void exec_RES_2_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x96);
    }
    void exec_RES_2_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x97);
    }
    void exec_RES_3_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0x98);
    }
    void exec_RES_3_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0x99);
    }
    void exec_RES_3_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9A);
    }
    void exec_RES_3_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9B);
    }
    void exec_RES_3_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9C);
    }
    void exec_RES_3_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9D);
    }
    void exec_RES_3_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9E);
    }
    void exec_RES_3_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0x9F);
    }
    void exec_RES_4_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA0);
    }
    void exec_RES_4_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA1);
    }
    void exec_RES_4_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA2);
    }
    void exec_RES_4_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA3);
    }
    void exec_RES_4_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA4);
    }
    void exec_RES_4_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA5);
    }
    void exec_RES_4_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA6);
    }
    void exec_RES_4_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA7);
    }
    void exec_RES_5_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA8);
    }
    void exec_RES_5_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xA9);
    }
    void exec_RES_5_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAA);
    }
    void exec_RES_5_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAB);
    }
    void exec_RES_5_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAC);
    }
    void exec_RES_5_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAD);
    }
    void exec_RES_5_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAE);
    }
    void exec_RES_5_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xAF);
    }
    void exec_RES_6_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB0);
    }
    void exec_RES_6_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB1);
    }
    void exec_RES_6_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB2);
    }
    void exec_RES_6_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB3);
    }
    void exec_RES_6_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB4);
    }
    void exec_RES_6_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB5);
    }
    void exec_RES_6_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB6);
    }
    void exec_RES_6_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB7);
    }
    void exec_RES_7_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB8);
    }
    void exec_RES_7_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xB9);
    }
    void exec_RES_7_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBA);
    }
    void exec_RES_7_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBB);
    }
    void exec_RES_7_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBC);
    }
    void exec_RES_7_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBD);
    }
    void exec_RES_7_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBE);
    }
    void exec_RES_7_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xBF);
    }
    void exec_SET_0_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC0);
    }
    void exec_SET_0_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC1);
    }
    void exec_SET_0_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC2);
    }
    void exec_SET_0_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC3);
    }
    void exec_SET_0_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC4);
    }
    void exec_SET_0_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC5);
    }
    void exec_SET_0_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC6);
    }
    void exec_SET_0_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC7);
    }
    void exec_SET_1_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC8);
    }
    void exec_SET_1_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xC9);
    }
    void exec_SET_1_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCA);
    }
    void exec_SET_1_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCB);
    }
    void exec_SET_1_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCC);
    }
    void exec_SET_1_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCD);
    }
    void exec_SET_1_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCE);
    }
    void exec_SET_1_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xCF);
    }
    void exec_SET_2_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD0);
    }
    void exec_SET_2_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD1);
    }
    void exec_SET_2_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD2);
    }
    void exec_SET_2_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD3);
    }
    void exec_SET_2_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD4);
    }
    void exec_SET_2_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD5);
    }
    void exec_SET_2_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD6);
    }
    void exec_SET_2_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD7);
    }
    void exec_SET_3_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD8);
    }
    void exec_SET_3_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xD9);
    }
    void exec_SET_3_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDA);
    }
    void exec_SET_3_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDB);
    }
    void exec_SET_3_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDC);
    }
    void exec_SET_3_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDD);
    }
    void exec_SET_3_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDE);
    }
    void exec_SET_3_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xDF);
    }
    void exec_SET_4_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE0);
    }
    void exec_SET_4_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE1);
    }
    void exec_SET_4_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE2);
    }
    void exec_SET_4_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE3);
    }
    void exec_SET_4_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE4);
    }
    void exec_SET_4_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE5);
    }
    void exec_SET_4_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE6);
    }
    void exec_SET_4_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE7);
    }
    void exec_SET_5_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE8);
    }
    void exec_SET_5_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xE9);
    }
    void exec_SET_5_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xEA);
    }
    void exec_SET_5_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xEB);
    }
    void exec_SET_5_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xEC);
    }
    void exec_SET_5_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xED);
    }
    void exec_SET_5_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xEE);
    }
    void exec_SET_5_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xEF);
    }
    void exec_SET_6_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF0);
    }
    void exec_SET_6_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF1);
    }
    void exec_SET_6_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF2);
    }
    void exec_SET_6_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF3);
    }
    void exec_SET_6_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF4);
    }
    void exec_SET_6_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF5);
    }
    void exec_SET_6_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF6);
    }
    void exec_SET_6_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF7);
    }
    void exec_SET_7_IY_d_ptr_B(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF8);
    }
    void exec_SET_7_IY_d_ptr_C(int8_t offset)  {
        exec_FDCB_helper(offset, 0xF9);
    }
    void exec_SET_7_IY_d_ptr_D(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFA);
    }
    void exec_SET_7_IY_d_ptr_E(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFB);
    }
    void exec_SET_7_IY_d_ptr_H(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFC);
    }
    void exec_SET_7_IY_d_ptr_L(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFD);
    }
    void exec_SET_7_IY_d_ptr(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFE);
    }
    void exec_SET_7_IY_d_ptr_A(int8_t offset)  {
        exec_FDCB_helper(offset, 0xFF);
    }
#endif
};

class Z80DefaultBus {
public:
    Z80DefaultBus() { m_ram.resize(0x10000, 0); }
    template <typename TEvents, typename TDebugger> void connect(Z80<Z80DefaultBus, TEvents, TDebugger>* cpu) {}
    void reset() { std::fill(m_ram.begin(), m_ram.end(), 0); }
    uint8_t read(uint16_t address) { return m_ram[address]; }
    void write(uint16_t address, uint8_t value) { m_ram[address] = value; }
    uint8_t in(uint16_t port) { return 0xFF; }
    void out(uint16_t port, uint8_t value) { /* no-op */ }

private:
    std::vector<uint8_t> m_ram;
};

class Z80DefaultEvents {
public:
    static constexpr long long CYCLES_PER_EVENT = LLONG_MAX;
    template <typename TBus, typename TDebugger>
    void connect(const Z80<TBus, Z80DefaultEvents, TDebugger>* cpu) {}
    void reset() {}
    long long get_event_limit() const { return LLONG_MAX; }
    void handle_event(long long tick) {}
};

class Z80DefaultDebugger {
public: 
    template <typename TBus, typename TEvents> 
    void connect(const Z80<TBus, TEvents, Z80DefaultDebugger>* cpu) {}
    void reset() {}
    void before_step(const std::vector<uint8_t>& opcodes) {}
    void after_step(const std::vector<uint8_t>& opcodes) {}
    void before_IRQ() {}
    void after_IRQ() {}
    void before_NMI() {}
    void after_NMI() {}
};

#endif //__Z80_H__