#include <fstream>
#include <iomanip>
#include <ostream>
#include <vector>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

#include "Z80.h"

using Z80Processor = Z80<class Bus, class Z80DefaultEvents, class Z80DefaultDebugger>;

bool show_details = false;
bool show_passed_tests = false;

class TestBus : public Z80DefaultBus {
public:
    uint8_t in(uint16_t port) {
        if (m_ports.count(port)) {
            return m_ports[port];
        }
        return 0xFF;
    }
    void out(uint16_t port, uint8_t value) {
        // Można dodać logikę zapisu do portu, jeśli testy tego wymagają
    }
    template <typename TEvents, typename TDebugger>
    void connect(Z80<TestBus, TEvents, TDebugger>* cpu) {
        // Ta metoda jest potrzebna, aby dopasować typy w szablonie Z80.
    }
    std::map<uint16_t, uint8_t> m_ports;
};
 
class Bus {
public:
    Bus() { m_ram.resize(0x10000, 0); }
    void connect(Z80Processor* cpu) { m_cpu = cpu; }
    void reset() { std::fill(m_ram.begin(), m_ram.end(), 0); }
    uint8_t read(uint16_t address) {
        if (address == 0x0005) {
            // Obsługa wywołań BDOS dla testów CP/M
            handle_bdos_call();
            return 0xC9; //RET
        }
        else if (address == 0x0000) { is_finished = true; }
        return m_ram[address];
    }
    void write(uint16_t address, uint8_t value) { m_ram[address] = value; }
    bool has_finished() const { return is_finished; }
    uint8_t in(uint16_t port) {
        if (m_ports.count(port)) {
            return m_ports[port];
        }
        return 0xFF; // Domyślna wartość, jeśli port nie jest zdefiniowany
    }
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
    Z80Processor* m_cpu = nullptr;
    std::vector<uint8_t> m_ram;
    bool is_finished = false;
    std::map<uint16_t, uint8_t> m_ports;
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
using Z80TestProcessor = Z80<TestBus>;

// Function to set the processor state based on JSON data
void set_initial_state(Z80TestProcessor& cpu, const nlohmann::json& state) {
    cpu.reset(); // Reset CPU and bus before each test
    cpu.set_PC(state["pc"]);
    cpu.set_SP(state["sp"]);
    cpu.set_A(state["a"]);
    cpu.set_F(static_cast<uint8_t>(state["f"]));
    cpu.set_B(state["b"]);
    cpu.set_C(state["c"]);
    cpu.set_D(state["d"]);
    cpu.set_E(state["e"]);
    cpu.set_H(state["h"]);
    cpu.set_L(state["l"]);
    cpu.set_IX(state["ix"]);
    cpu.set_IY(state["iy"]);
    cpu.set_I(state["i"]);
    cpu.set_R(state["r"]);
    cpu.set_IFF1(state["iff1"].get<int>() != 0);
    cpu.set_IFF2(state["iff2"].get<int>() != 0);
    if (state.contains("im")) cpu.set_IRQ_mode(state["im"]);
    if (state.contains("wz")) cpu.set_WZ(state["wz"]);
    if (state.contains("ei")) cpu.set_EI_executed(state["ei"].get<int>() != 0);
    if (state.contains("q")) cpu.set_Q(state["q"]);
    // Set alternate registers if they exist in the test case
    if (state.contains("af_")) cpu.set_AFp(state["af_"]);
    if (state.contains("bc_")) cpu.set_BCp(state["bc_"]);
    if (state.contains("de_")) cpu.set_DEp(state["de_"]);
    if (state.contains("hl_")) cpu.set_HLp(state["hl_"]);

    // Wypełnij pamięć RAM
    for (const auto& ram_entry : state["ram"]) {
        uint16_t addr = ram_entry[0];
        uint8_t val = ram_entry[1];
        cpu.get_bus()->write(addr, val);
    }

    // Wypełnij porty I/O, jeśli istnieją w teście
    if (state.contains("ports")) {
        for (const auto& port_entry : state["ports"]) {
            uint16_t port = port_entry[0];
            cpu.get_bus()->m_ports[port] = (uint8_t)port_entry[1];
        }
    }
}

bool check_final_state(Z80TestProcessor& cpu, const nlohmann::json& test_case, const std::string& test_name, const std::string& full_test_name) {
    const auto& expected_state = test_case["final"];
    if (expected_state.is_null()) {
        return false;
    }

    bool pass = true;
    auto check = [&](const std::string& what, uint32_t actual, uint32_t expected, const std::string& full_name) {
        if (actual != expected) {
            if (show_details) {
                std::cout << "FAIL: " << test_name << " (" << full_name << ") - " << what << " | Expected: 0x" << std::hex << expected << ", Got: 0x" << actual << std::dec << std::endl;
            }
            pass = false;
        }
    };

    check("PC", cpu.get_PC(), (uint16_t)expected_state["pc"], full_test_name);
    check("SP", cpu.get_SP(), (uint16_t)expected_state["sp"], full_test_name);
    check("A", cpu.get_A(), (uint8_t)expected_state["a"], full_test_name);
    check("F", (uint8_t)cpu.get_F(), (uint8_t)expected_state["f"], full_test_name);
    check("B", cpu.get_B(), (uint8_t)expected_state["b"], full_test_name);
    check("C", cpu.get_C(), (uint8_t)expected_state["c"], full_test_name);
    check("D", cpu.get_D(), (uint8_t)expected_state["d"], full_test_name);
    check("E", cpu.get_E(), (uint8_t)expected_state["e"], full_test_name);
    check("H", cpu.get_H(), (uint8_t)expected_state["h"], full_test_name);
    check("L", cpu.get_L(), (uint8_t)expected_state["l"], full_test_name);
    check("IX", cpu.get_IX(), (uint16_t)expected_state["ix"], full_test_name);
    check("IY", cpu.get_IY(), (uint16_t)expected_state["iy"], full_test_name);
    check("I", cpu.get_I(), (uint8_t)expected_state["i"], full_test_name);
    check("R", cpu.get_R(), (uint8_t)expected_state["r"], full_test_name);
    check("IFF1", cpu.get_IFF1(), expected_state["iff1"].get<int>() != 0, full_test_name);
    check("IFF2", cpu.get_IFF2(), expected_state["iff2"].get<int>() != 0, full_test_name);
    if (expected_state.contains("wz")) check("WZ", cpu.get_WZ(), (uint16_t)expected_state["wz"], full_test_name);
    if (expected_state.contains("ei")) check("EI", cpu.is_EI_executed(), expected_state["ei"].get<int>() != 0, full_test_name);
    //if (expected_state.contains("q")) check("Q", cpu.get_Q(), (uint8_t)expected_state["q"], full_test_name);

    if (expected_state.contains("ram")) {
        for (const auto& ram_entry : expected_state["ram"]) {
            uint16_t addr = ram_entry[0];
            uint8_t expected_val = ram_entry[1];
            uint8_t actual_val = cpu.get_bus()->peek(addr);
            if (actual_val != expected_val && show_details) {
                std::cout << "FAIL: " << test_name << " (" << full_test_name << ") - RAM[0x" << std::hex << addr << "] | Expected: 0x" << (int)expected_val << ", Got: 0x" << (int)actual_val << std::dec << std::endl;
                pass = false;
            }
        }
    }

    if (test_case.contains("cycles")) {
        long long total_ticks = 0;
        if (!test_case["cycles"].is_null()) {
            total_ticks = test_case["cycles"].size();
            if (cpu.get_ticks() != total_ticks) {
                check("Ticks", cpu.get_ticks(), total_ticks, full_test_name);
            }
        }
    }
    return pass;
}

void run_test_file(const std::string& test_path) {
    std::ifstream f(test_path);
    if (!f) {
        std::cout << "Cannot open test file: " << test_path << std::endl;
        return;
    }
    nlohmann::json data = nlohmann::json::parse(f);

    bool all_passed = true;

    for (auto& [test_name, test_case] : data.items()) {
        Z80TestProcessor cpu;
        std::string full_test_name = test_case["name"];
        set_initial_state(cpu, test_case["initial"]);
        cpu.step();

        if (test_case.contains("final")) {
            if (!check_final_state(cpu, test_case, test_name, full_test_name)) {
                all_passed = false;
                if (show_details) {
                    std::cout << "----------------------------------------" << std::endl;
                }
            }
        }
    }

    if (!all_passed || show_passed_tests) {
        std::string opcode_str = std::filesystem::path(test_path).stem().string();
        std::transform(opcode_str.begin(), opcode_str.end(), opcode_str.begin(),
                   [](unsigned char c){ return std::toupper(c); });
        std::cout << "Test file: " << test_path << " (Opcode: " << opcode_str << "): ";
        std::cout << (all_passed ? "PASS" : "FAIL") << std::endl;
    }
}

void run_all_tests(const std::string& tests_dir) {
    std::vector<std::filesystem::path> test_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(tests_dir)) {
        if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".json") {
            test_files.push_back(entry.path());
        }
    }
    std::sort(test_files.begin(), test_files.end());
    for (const auto& path : test_files) {
        run_test_file(path);
    }
}

// ====================================================================

bool load_rom(const std::string& filepath, Bus& bus, uint16_t start_address) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "Error: Cannot open file " << filepath << std::endl;
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
        std::cout << "Error: No arguments provided." << std::endl;
        std::cout << "Usage (zex tests): " << argv[0] << " <path_to_zex_file>" << std::endl;
        std::cout << "Usage (json tests):" << argv[0] << " --test <path_to_test_directory>" << std::endl;
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--test") {
        if (argc < 3) { // Sprawdzenie, czy podano ścieżkę
            std::cout << "Error: Test path not provided." << std::endl;
            std::cout << "Usage: " << argv[0] << " --test <path_to_test_directory_or_file>" << std::endl;
            return 1;
        }
        std::cout << "Running test suite..." << std::endl;
        std::string test_path = argv[2];
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--details") {
                show_details = true;
            } else if (arg == "--pass") {
                show_passed_tests = true;
            }
        }
        if (std::filesystem::is_directory(test_path)) {
            run_all_tests(test_path);
        } else if (std::filesystem::is_regular_file(test_path)) {
            run_test_file(test_path);
        } else {
            std::cout << "Error: Provided test path is not a valid directory or file: " << test_path << std::endl;
            return 1;
        }
        return 0; // Zakończ po testach
    }

    const char* rom_filename = argv[1];

    Z80Processor cpu;
    Bus& bus = *cpu.get_bus();
    if (!load_rom(rom_filename, bus, 0x0100)) {
        std::cout << "Error: Failed to load ROM file: " << rom_filename << std::endl;
        return 1;
    }

    Timer timer;
    cpu.set_PC(0x0100);
    while (!cpu.get_bus()->has_finished()) {
        
        cpu.run(10000000000LL);
    }
    timer.stop();
    return 0;
}
