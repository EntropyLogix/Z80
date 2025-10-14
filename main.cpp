#include <fstream>
#include <iomanip>
#include <ostream>
#include <vector>
#include <iostream>
#include <chrono>

#define Z80_ENABLE_EXEC_API
#include "Z80.h"

using namespace Z80;

class Bus;

class Events {
public:
    static constexpr long long CYCLES_PER_EVENT = 100000000;

    template<typename TBus, typename TDebugger>
    void connect(::Z80::Core<TBus, Events, TDebugger>* cpu) { m_cpu = cpu; }
    void reset() {
        m_next_event_tick = CYCLES_PER_EVENT;
        m_system_timer_value = 0;
    }
    long long get_event_limit() const { return m_next_event_tick; }
    void handle_event(long long tick) {
        m_system_timer_value++;
        m_next_event_tick += CYCLES_PER_EVENT;
    }
private:
    ::Z80::Core<Bus, Events, NoDebugger>* m_cpu = nullptr;
    long long m_next_event_tick = CYCLES_PER_EVENT; 
    int m_system_timer_value = 0; 
};

class Bus {
public:
    Bus() { m_ram.resize(0x10000, 0); }
    template<typename TEvents, typename TDebugger>
    void connect(::Z80::Core<Bus, TEvents, TDebugger>* cpu) { m_cpu = cpu; }
    void reset() { std::fill(m_ram.begin(), m_ram.end(), 0); }
    uint8_t read(uint16_t address) {
        if (address == 0x0005) {
            handle_bdos_call();
            return 0xC9; //RET
        }
        else if (address == 0x0000) { is_finished = true; }
        return m_ram[address];
    }
    void write(uint16_t address, uint8_t value) { m_ram[address] = value; }
    bool has_finished() const { return is_finished; }
    uint8_t in(uint16_t port) { return 0xFF; }
    void out(uint16_t port, uint8_t value) {}

private:
    void handle_bdos_call() {
        uint8_t func = m_cpu->get_C();
        if (func == 2) { // C_WRITE
            std::cout << static_cast<char>(m_cpu->get_E());
        } else if (func == 9) { // C_WRITESTR
            uint16_t addr = m_cpu->get_DE();
            char c;
            while ((c = m_ram[addr++]) != '$') {
                std::cout << c;
            }
        }
    }
    ::Z80::Core<Bus, Events, NoDebugger>* m_cpu = nullptr;
    std::vector<uint8_t> m_ram;
    bool is_finished = false;
};

class Timer {
public:
    Timer() { m_start = std::chrono::high_resolution_clock::now(); }
    void stop() {
        m_end = std::chrono::high_resolution_clock::now();
        long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count();
        long long ms = total_ms % 1000;
        long long total_seconds = total_ms / 1000;
        long long seconds = total_seconds % 60;
        long long minutes = total_seconds / 60;
        std::cout << std::endl << "Time: " << std::setfill('0') << std::setw(2) << minutes << "m " << 
                                std::setfill('0') << std::setw(2) << seconds <<  "s " <<
                                std::setw(3) << ms << "ms" << std::endl;
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start, m_end;
};

bool load_rom(const std::string& filepath, Bus& bus, uint16_t start_address) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filepath << std::endl;
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        for (int i = 0; i < size; ++i) {
            bus.write(start_address + i, static_cast<uint8_t>(buffer[i]));
        }
        std::cout << "Successfully loaded " << size << " bytes from " << filepath << std::endl;
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No ROM file path provided." << std::endl;
        std::cerr << "Usage: " << argv[0] << " <rom_file_path>" << std::endl;
        return 1;
    }
    const char* rom_filename = argv[1];
    Bus bus;
    Events events;
    NoDebugger debugger;
    ::Z80::Core cpu(bus, events, debugger);
    if (!load_rom(rom_filename, bus, 0x0100)) {
        std::cerr << "Error: Failed to load ROM file: " << rom_filename << std::endl;
        return 1;
    }
    Timer timer;
    cpu.set_PC(0x0100);
    while (!bus.has_finished()) {
        
        cpu.run(10000000000LL);
    }
    timer.stop();
    return 0;
}
