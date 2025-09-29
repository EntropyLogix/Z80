#include <fstream>
#include <iomanip>
#include <ostream>
#include <vector>
#include <iostream>
#include <chrono>
#include "Z80.h"

class Memory : public Z80::MemoryBus {
public:
    Memory() { m_ram.resize(0x10000, 0); }
    void reset() override { std::fill(m_ram.begin(), m_ram.end(), 0); }
    uint8_t read(uint16_t address) override { return m_ram[address]; }
    void write(uint16_t address, uint8_t value) override { m_ram[address] = value; }
    std::vector<uint8_t>& get_ram() { return m_ram; }
private:
    std::vector<uint8_t> m_ram;
};

class IO : public Z80::IOBus {
public:
    void reset() override {}
    uint8_t read(uint16_t port) override { return 0xFF; }
    void write(uint16_t port, uint8_t value) override {}
};

class ZEXTestBus : public Z80::MemoryBus {
public:
    ZEXTestBus() { m_ram.resize(0x10000, 0); }
    
    void reset() override {
        std::fill(m_ram.begin(), m_ram.end(), 0);
    }

    void write(uint16_t address, uint8_t value) override {
        m_ram[address] = value;
    }

    uint8_t read(uint16_t address) override {
        if (address == 0x0005) { //CP/M BDOS
            handle_bdos_call();
            return 0xC9; //RET
        }
        else if (address == 0x0000) {
            std::cout << "\n[System Reset Trap at 0x0000]" << std::endl;
            is_finished = true;
        }
        
        return m_ram[address];
    }

    bool has_finished() const { return is_finished; }
    std::vector<uint8_t>& get_ram() { return m_ram; }

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
    std::vector<uint8_t> m_ram;
    bool is_finished = false;
};

bool load_rom(const std::string& filepath, Z80::MemoryBus& memory, uint16_t start_address) {
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
            memory.write(start_address + i, static_cast<uint8_t>(buffer[i]));
        }
        std::cout << "Successfully loaded " << size << " bytes from " << filepath << std::endl;
        return true;
    }
    return false;
}

int main() {
    #ifdef NDEBUG
        std::cout << "--- Build: Release ---" << std::endl;
    #else
        std::cout << "--- Build: Debug ---" << std::endl;
    #endif

    auto start = std::chrono::high_resolution_clock::now();

    ZEXTestBus memory_bus;
    IO io_bus;
    Z80 cpu(memory_bus, io_bus);
    if (!load_rom("zexall.com", memory_bus, 0x0100)) {
        return 1;
    }
    cpu.set_PC(0x0100);
    while (!memory_bus.has_finished()) {
        cpu.step();
    }

    auto end = std::chrono::high_resolution_clock::now();
    long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    long long ms = total_ms % 1000;
    long long total_seconds = total_ms / 1000;
    long long seconds = total_seconds % 60;
    long long minutes = total_seconds / 60;

    std::cout << std::endl << "Time: " << std::setfill('0') << std::setw(2) << minutes << "m " << 
                            std::setfill('0') << std::setw(2) << seconds <<  "s " <<
                            std::setw(3) << ms << "ms" << std::endl;

    return 0;
}
