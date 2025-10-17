#include <fstream>
#include <iomanip>
#include <ostream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include "Z80.h"

using Z80TestProcessor = Z80<class TestBus>;

const std::string RED_TEXT = "\033[1;31m";
const std::string GREEN_TEXT = "\033[1;32m";
const std::string RESET_TEXT = "\033[0m";

bool show_details = false;

class TestBus : public Z80DefaultBus {
public:
    uint8_t in(uint16_t port) {
        if (m_ports.count(port)) {
            return m_ports[port];
        }
        return 0xFF;
    }
    void out(uint16_t port, uint8_t value) {
    }
    template <typename TEvents, typename TDebugger>
    void connect(Z80<TestBus, TEvents, TDebugger>* cpu) {}
    void reset() {
        Z80DefaultBus::reset();
        m_ports.clear();
    }
    std::map<uint16_t, uint8_t> m_ports;
};

void set_initial_state(Z80TestProcessor& cpu, const nlohmann::json& test_case) {
    cpu.reset();
    const auto& state = test_case["initial"];
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
    if (state.contains("ei")) cpu.set_block_interrupt(state["ei"].get<int>() != 0);
    if (state.contains("q")) cpu.set_Q(state["q"]);
    if (state.contains("af_")) cpu.set_AFp(state["af_"]);
    if (state.contains("bc_")) cpu.set_BCp(state["bc_"]);
    if (state.contains("de_")) cpu.set_DEp(state["de_"]);
    if (state.contains("hl_")) cpu.set_HLp(state["hl_"]);

    for (const auto& ram_entry : state["ram"]) {
        uint16_t addr = ram_entry[0];
        uint8_t val = ram_entry[1];
        cpu.get_bus()->write(addr, val);
    }

    if (test_case.contains("ports")) {
        for (const auto& port_entry : test_case["ports"]) {
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
                std::cout << RED_TEXT << "FAIL: " << RESET_TEXT << test_name << " (" << full_name << ") - " << what << " | Expected: 0x" << std::hex << expected << ", Got: 0x" << actual << std::dec << std::endl;
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
    if (expected_state.contains("ei")) check("block_interrupt", cpu.get_block_interrupt(), expected_state["ei"].get<int>() != 0, full_test_name);

    if (expected_state.contains("ram")) {
        for (const auto& ram_entry : expected_state["ram"]) {
            uint16_t addr = ram_entry[0];
            uint8_t expected_val = ram_entry[1];
            uint8_t actual_val = cpu.get_bus()->peek(addr);
            if (actual_val != expected_val && show_details) {
                std::cout << RED_TEXT << "FAIL: " << RESET_TEXT << test_name << " (" << full_test_name << ") - RAM[0x" << std::hex << addr << "] | Expected: 0x" << (int)expected_val << ", Got: 0x" << (int)actual_val << std::dec << std::endl;
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

    TestBus test_bus;

    for (auto& [test_name, test_case] : data.items()) {
        Z80TestProcessor cpu(&test_bus);
        std::string full_test_name = test_case["name"];
        set_initial_state(cpu, test_case);

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

    std::string opcode_str = std::filesystem::path(test_path).stem().string();
    std::transform(opcode_str.begin(), opcode_str.end(), opcode_str.begin(),
                [](unsigned char c){ return std::toupper(c); });
    std::cout << "Test file: " << test_path << " (Opcode: " << opcode_str << "): ";
    std::cout << (all_passed ? GREEN_TEXT + "PASS" : RED_TEXT + "FAIL") << RESET_TEXT << std::endl;
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

int main(int argc, char* argv[]) {
    std::cout << "Running test suite..." << std::endl;
    std::string test_path_str = Z80_TESTS_DIR;
    std::cout << "Using default test path: " << test_path_str << std::endl;
    std::filesystem::path test_path(test_path_str);
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--details") {
            show_details = true;
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
    return 0;
}