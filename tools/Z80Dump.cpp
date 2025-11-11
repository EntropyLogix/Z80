//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Dump.cpp
// Verson: 1.0.5
//
// This file contains a command-line utility for dumping memory, registers,
// and disassembling code from Z80 binary files and snapshots.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80.h"
#include "Z80Analyze.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

template <typename T> std::string format_hex(T value, int width) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return ss.str();
}

void print_usage() {
    std::cerr << "Usage: Z80Dump <file_path> [options]\n"
              << "File formats supported: .bin, .sna, .z80, .hex\n\n"
              << "Options:\n"
              << "  --mem-dump <address> <bytes_hex>\n"
              << "    Dumps memory. <address> can be a hex value, a register "
                 "(PC, SP, HL),\n"
              << "    or an expression like 'PC+10' or 'HL-0x20'.\n"
              << "    Example: --mem-dump 4000 100\n\n"
              << "  --disassemble <address> <lines_dec>\n"
              << "    Disassembles code. <address> can be a hex value, a "
                 "register, or an expression.\n"
              << "    Example: --disassemble 8000 20\n\n"
              << "  --load-addr <address_hex>\n"
              << "    Specifies the loading address for .bin files (default: "
                 "0x0000).\n"
              << "    Example: --load-addr 8000\n\n"
              << "  --map <file_path> (can be used multiple times)\n"
              << "    Loads labels from a .map file for disassembly.\n\n"
              << "  --ctl <file_path> (can be used multiple times)\n"
              << "    Loads labels from a .ctl file for disassembly.\n\n"
              << "  --reg-dump [format_string]\n"
              << "    Dumps CPU registers. An optional format string can be "
                 "provided.\n"
              << "    Example: --reg-dump \"PC=%pc SP=%sp AF=%af BC=%bc DE=%de "
                 "HL=%hl\"\n";
    std::cerr << "  --run-ticks <ticks_dec>\n"
              << "    Runs the emulation for <ticks_dec> T-states before other "
                 "actions.\n"
              << "    Example: --run-ticks 100000\n";
    std::cerr << "  --run-steps <steps_dec>\n"
              << "    Runs the emulation for <steps_dec> instructions (steps) "
                 "before other actions.\n"
              << "    Example: --run-steps 500\n";
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

uint16_t resolve_address(const std::string& addr_str, const Z80<>& cpu) {
    if (addr_str.empty())
        throw std::runtime_error("Address argument is empty.");
    size_t plus_pos = addr_str.find('+');
    size_t minus_pos = addr_str.find('-');
    size_t operator_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;
    if (operator_pos != std::string::npos) {
        std::string reg_str = addr_str.substr(0, operator_pos);
        std::string offset_str = addr_str.substr(operator_pos + 1);
        char op = addr_str[operator_pos];
        reg_str.erase(0, reg_str.find_first_not_of(" \t"));
        reg_str.erase(reg_str.find_last_not_of(" \t") + 1);
        offset_str.erase(0, offset_str.find_first_not_of(" \t"));
        offset_str.erase(offset_str.find_last_not_of(" \t") + 1);
        uint16_t base_addr = resolve_address(reg_str, cpu);
        int offset = 0;
        try {
            size_t pos;
            std::string upper_offset_str = offset_str;
            std::transform(upper_offset_str.begin(), upper_offset_str.end(), upper_offset_str.begin(), ::toupper);
            if (upper_offset_str.size() > 2 && upper_offset_str.substr(0, 2) == "0X") {
                offset = std::stoi(offset_str.substr(2), &pos, 16);
                pos += 2;
            } else if (upper_offset_str.back() == 'H') {
                offset = std::stoi(offset_str.substr(0, offset_str.length() - 1), &pos, 16);
                pos += 1;
            } else 
                offset = std::stoi(offset_str, &pos, 10);
            if (pos != offset_str.length()) {
                throw std::invalid_argument("Invalid characters in offset");
            }
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid offset in address expression: " + offset_str);
        }
        return (op == '+') ? (base_addr + offset) : (base_addr - offset);
    }

    try {
        std::string upper_str = addr_str;
        std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
        if (upper_str.size() > 2 && upper_str.substr(0, 2) == "0X") {
            return std::stoul(upper_str.substr(2), nullptr, 16);
        } else if (upper_str.back() == 'H') {
            return std::stoul(upper_str.substr(0, upper_str.length() - 1), nullptr, 16);
        } else {
            return std::stoul(addr_str, nullptr, 10);
        }
    } catch (const std::invalid_argument&) {
        std::string upper_str = addr_str;
        std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
        if (upper_str == "PC")
            return cpu.get_PC();
        if (upper_str == "SP")
            return cpu.get_SP();
        if (upper_str == "HL")
            return cpu.get_HL();
        if (upper_str == "BC")
            return cpu.get_BC();
        if (upper_str == "DE")
            return cpu.get_DE();
        if (upper_str == "IX")
            return cpu.get_IX();
        if (upper_str == "IY")
            return cpu.get_IY();
        throw std::runtime_error("Invalid address or register name: " + addr_str);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Address value out of range: " + addr_str);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    std::string file_path = argv[1];
    std::string mem_dump_addr_str, disasm_addr_str, load_addr_str = "0x0000";
    size_t mem_dump_size = 0, disasm_lines = 0;
    long long run_ticks = 0;
    std::vector<std::string> map_files;
    std::vector<std::string> ctl_files;
    long long run_steps = 0;
    bool reg_dump_action = false;
    std::string reg_dump_format;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mem-dump" && i + 2 < argc) {
            mem_dump_addr_str = argv[++i];
            mem_dump_size = std::stoul(argv[++i], nullptr, 16);
        } else if (arg == "--disassemble" && i + 2 < argc) {
            disasm_addr_str = argv[++i];
            disasm_lines = std::stoul(argv[++i], nullptr, 10);
        } else if (arg == "--load-addr" && i + 1 < argc)
            load_addr_str = argv[++i];
        else if (arg == "--map" && i + 1 < argc)
            map_files.push_back(argv[++i]);
        else if (arg == "--ctl" && i + 1 < argc)
            ctl_files.push_back(argv[++i]);
        else if (arg == "--reg-dump") {
            reg_dump_action = true;
            if (i + 1 < argc && argv[i + 1][0] != '-')
                reg_dump_format = argv[++i];
        } else if (arg == "--run-ticks" && i + 1 < argc)
            run_ticks = std::stoll(argv[++i], nullptr, 10);
        else if (arg == "--run-steps" && i + 1 < argc)
            run_steps = std::stoll(argv[++i], nullptr, 10);
        else {
            bool is_known_arg_with_missing_value =
                ((arg == "--mem-dump" || arg == "--disassemble") && i + 2 >= argc) ||
                ((arg == "--load-addr" || arg == "--map" || arg == "--ctl" || arg == "--run-ticks" || arg == "--run-steps") && i + 1 >= argc);
            if (is_known_arg_with_missing_value)
                std::cerr << "Error: Incomplete argument for '" << arg << "'. Expected two values." << std::endl;
            else
                std::cerr << "Error: Unknown or incomplete argument '" << arg << "'." << std::endl;
            print_usage();
            return 1;
        }
    }    
    std::string ext = get_file_extension(file_path);
    std::vector<uint8_t> file_data_bytes;
    std::string file_data_str;
    if (ext == "hex") {
        std::ifstream file(file_path);
        if (!file) { std::cerr << "Error: Could not read file '" << file_path << "'." << std::endl; return 1; }
        file_data_str.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    Z80<> cpu;
    Z80DefaultLabels label_handler;
    Z80Analyzer<Z80DefaultBus, Z80<>, Z80DefaultLabels> analyzer(cpu.get_bus(), &cpu, &label_handler);
    try {
        Z80DefaultFiles<Z80DefaultBus, Z80<>> file_loader(cpu.get_bus(), &cpu);
        for (const auto& map_file : map_files) {
            std::ifstream file(map_file);
            if (!file) throw std::runtime_error("Cannot open map file: " + map_file);
            std::stringstream buffer;
            buffer << file.rdbuf();
            label_handler.load_map(buffer.str());
            std::cout << "Loaded labels from " << map_file << std::endl;
        }
        for (const auto& ctl_file : ctl_files) {
            std::ifstream file(ctl_file);
            if (!file) throw std::runtime_error("Cannot open ctl file: " + ctl_file);
            std::stringstream buffer;
            buffer << file.rdbuf();
            label_handler.load_ctl(buffer.str());
            std::cout << "Loaded labels from " << ctl_file << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading label file: " << e.what() << std::endl;
    }
    bool loaded = false;
    std::cout << "Loading file: " << file_path << " (type: " << (ext.empty() ? "bin" : ext) << ")" << std::endl;
    if (ext != "hex") {
        file_data_bytes = read_file(file_path);
        if (file_data_bytes.empty()) {
            std::cerr << "Error: Could not read file or file is empty '" << file_path << "'." << std::endl;
            return 1;
        }
    }
    Z80DefaultFiles<Z80DefaultBus, Z80<>> file_loader(cpu.get_bus(), &cpu);
    try {
        if (ext == "hex")
            loaded = file_loader.load_hex_file(file_data_str);
        else if (ext == "sna")
            loaded = file_loader.load_sna_file(file_data_bytes);
        else if (ext == "z80")
            loaded = file_loader.load_z80_file(file_data_bytes);
        else if (ext == "bin" || ext.empty()) {
            uint16_t load_addr = resolve_address(load_addr_str, cpu);
            loaded = file_loader.load_bin_file(file_data_bytes, load_addr);
            cpu.set_PC(load_addr);
        } else {
            std::cerr << "Error: Unsupported file extension '" << ext << "'." << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading file: " << e.what() << std::endl;
        return 1;
    }
    if (!loaded) {
        std::cerr << "Error: Failed to load file content into emulator for an unknown reason." << std::endl;
        return 1;
    }
    std::cout << "File loaded successfully.\n" << std::endl;
    if (run_ticks > 0) {
        std::cout << "--- Running emulation for " << run_ticks << " T-states ---\n";
        long long executed_ticks = cpu.run(cpu.get_ticks() + run_ticks);
        std::cout << "Executed " << executed_ticks << " T-states. CPU is now at tick " << cpu.get_ticks() << ".\n" << std::endl;
    }
    if (run_steps > 0) {
        std::cout << "--- Running emulation for " << run_steps << " instructions (steps) ---\n";
        long long total_ticks_for_steps = 0;
        for (long long i = 0; i < run_steps; ++i)
            total_ticks_for_steps += cpu.step();
        std::cout << "Executed " << run_steps << " instructions (" << total_ticks_for_steps
                  << " T-states). CPU is now at tick " << cpu.get_ticks() << ".\n"
                  << std::endl;
    }
    if (mem_dump_size == 0 && disasm_lines == 0 && !reg_dump_action)
        reg_dump_action = true;
    if (reg_dump_action) {
        std::string format = reg_dump_format.empty() ? "AF=%af BC=%bc DE=%de HL=%hl IX=%ix IY=%iy "
                                                       "PC=%pc SP=%sp | %flags"
                                                     : reg_dump_format;
        std::cout << "--- Register Dump ---\n";
        std::cout << analyzer.dump_registers(format) << std::endl;
    }
    if (mem_dump_size > 0) {
        uint16_t mem_dump_addr = resolve_address(mem_dump_addr_str, cpu);
        std::cout << "\n--- Memory Dump from " << format_hex(mem_dump_addr, 4) << " (" << mem_dump_size
                  << " bytes) ---\n";
        uint16_t current_addr = mem_dump_addr;
        size_t bytes_remaining = mem_dump_size;
        const size_t cols = 16;
        while (bytes_remaining > 0) {
            size_t rows_to_dump = (bytes_remaining + cols - 1) / cols;
            if (rows_to_dump == 0)
                break;
            auto dump = analyzer.dump_memory(current_addr, 1, cols);
            for (const auto& line : dump)
                std::cout << line << std::endl;
            size_t bytes_dumped = std::min(bytes_remaining, cols);
            bytes_remaining -= bytes_dumped;
        }
    }
    if (disasm_lines > 0) {
        uint16_t disasm_addr = resolve_address(disasm_addr_str, cpu);
        std::cout << "\n--- Disassembly from " << format_hex(disasm_addr, 4) << " (" << disasm_lines << " lines) ---\n";
        uint16_t pc = disasm_addr;        
        auto listing = analyzer.disassemble(pc, disasm_lines);
        for (const auto& line : listing)
            std::cout << line << std::endl;
    }
    if (mem_dump_size == 0 && disasm_lines == 0 && !reg_dump_action) {
        std::cout << "\nNo action specified. Use --reg-dump, --mem-dump, or "
                     "--disassemble to see output."
                  << std::endl;
    }
    return 0;
}
