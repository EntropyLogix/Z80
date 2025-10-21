//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄   
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██  
//      ██▀   ██▄  ▄██  ██    ██ 
//    ▄██▀     ██████   ██ ██ ██ 
//   ▄██      ██▀  ▀██  ██    ██ 
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██  
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Dump.cpp
// Verson: 1.0.4
// 
// This file contains a command-line utility for dumping memory
// and disassembling code from Z80 binary files and snapshots.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80.h"
#include "Z80Analyze.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <iomanip>

template <typename T>
std::string format_hex(T value, int width) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return ss.str();
}

void print_usage() {
    std::cerr << "Usage: Z80Dump <file_path> [options]\n"
            << "File formats supported: .bin, .sna, .z80\n\n"
            << "Options:\n"
            << "  --mem-dump <address> <bytes_hex>\n"
            << "    Dumps memory. <address> can be a hex value, a register (PC, SP, HL),\n"
            << "    or an expression like 'PC+10' or 'HL-0x20'.\n"
            << "    Example: --mem-dump 4000 100\n\n"
            << "  --disassemble <address> <lines_dec>\n"
            << "    Disassembles code. <address> can be a hex value, a register, or an expression.\n"
            << "    Example: --disassemble 8000 20\n\n"
            << "  --load-addr <address_hex>\n"
            << "    Specifies the loading address for .bin files (default: 0x0000).\n"
            << "    Example: --load-addr 8000\n\n"
            << "  --reg-dump [format_string]\n"
            << "    Dumps CPU registers. An optional format string can be provided.\n"
            << "    Example: --reg-dump \"PC=%pc SP=%sp AF=%af BC=%bc DE=%de HL=%hl\"\n";
    std::cerr << "  --run-ticks <ticks_dec>\n"
            << "    Runs the emulation for <ticks_dec> T-states before other actions.\n"
            << "    Example: --run-ticks 100000\n";
    std::cerr << "  --run-steps <steps_dec>\n"
            << "    Runs the emulation for <steps_dec> instructions (steps) before other actions.\n"
            << "    Example: --run-steps 500\n";
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
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

bool load_bin_file(Z80DefaultBus& bus, const std::vector<uint8_t>& data, uint16_t load_addr) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (load_addr + i > 0xFFFF) {
            std::cerr << "Warning: Binary file too large, truncated at 0xFFFF." << std::endl;
            break;
        }
        bus.write(load_addr + i, data[i]);
    }
    return true;
}

bool load_sna_file(Z80<>& cpu, const std::vector<uint8_t>& data) {
    if (data.size() != 49179) { // 27-byte header + 48KB RAM
        std::cerr << "Error: Invalid 48K SNA file size." << std::endl;
        return false;
    }
    Z80<>::State state;
    state.m_I = data[0];
    state.m_HLp.w = (data[2] << 8) | data[1];
    state.m_DEp.w = (data[4] << 8) | data[3];
    state.m_BCp.w = (data[6] << 8) | data[5];
    state.m_AFp.w = (data[8] << 8) | data[7];
    state.m_HL.w = (data[10] << 8) | data[9];
    state.m_DE.w = (data[12] << 8) | data[11];
    state.m_BC.w = (data[14] << 8) | data[13];
    state.m_IY.w = (data[16] << 8) | data[15];
    state.m_IX.w = (data[18] << 8) | data[17];
    state.m_IFF2 = (data[19] & 0x04) != 0;
    state.m_IFF1 = state.m_IFF2;
    state.m_R = data[20];
    state.m_AF.w = (data[22] << 8) | data[21];
    state.m_SP.w = (data[24] << 8) | data[23];
    state.m_IRQ_mode = data[25];
    for (size_t i = 0; i < 49152; ++i) {
        cpu.get_bus()->write(0x4000 + i, data[27 + i]);
    }
    state.m_PC.w = cpu.get_bus()->peek(state.m_SP.w) | (cpu.get_bus()->peek(state.m_SP.w + 1) << 8);
    state.m_SP.w += 2;
    cpu.restore_state(state);
    return true;
}

bool load_z80_file(Z80<>& cpu, const std::vector<uint8_t>& data) {
    if (data.size() < 30) {
        std::cerr << "Error: Z80 file is too small." << std::endl;
        return false;
    }

    Z80<>::State state;
    memset(&state, 0, sizeof(state));
    state.m_AF.h = data[0];
    state.m_AF.l = data[1];
    state.m_BC.w = (data[3] << 8) | data[2];
    state.m_HL.w = (data[5] << 8) | data[4];
    state.m_PC.w = (data[7] << 8) | data[6];
    state.m_SP.w = (data[9] << 8) | data[8];
    state.m_I = data[10];
    state.m_R = data[11];

    uint8_t byte12 = data[12];
    if (byte12 == 0xFF) byte12 = 1;
    state.m_R = (state.m_R & 0x7F) | ((byte12 & 0x01) ? 0x80 : 0);
    bool compressed = (byte12 & 0x20) != 0;

    state.m_DE.w = (data[14] << 8) | data[13];
    state.m_BCp.w = (data[16] << 8) | data[15];
    state.m_DEp.w = (data[18] << 8) | data[17];
    state.m_HLp.w = (data[20] << 8) | data[19];
    state.m_AFp.h = data[21];
    state.m_AFp.l = data[22];
    state.m_IY.w = (data[24] << 8) | data[23];
    state.m_IX.w = (data[26] << 8) | data[25];
    state.m_IFF1 = data[27] != 0;
    state.m_IFF2 = data[28] != 0;
    state.m_IRQ_mode = data[29] & 0x03;
    cpu.restore_state(state);
    if (state.m_PC.w == 0) {
        std::cerr << "Error: Z80 v2/v3 files are not supported yet." << std::endl;
        return false;
    }
    size_t data_ptr = 30;
    if (compressed) {
        uint16_t mem_addr = 0x4000;
        while (data_ptr < data.size() && mem_addr < 0xFFFF) {
            if (data_ptr + 1 < data.size() && data[data_ptr] == 0xED && data[data_ptr + 1] == 0xED) {
                if (data_ptr + 2 >= data.size())
                    break; // Corrupted sequence
                uint8_t count = 4;
                uint8_t value = data[data_ptr + 2];
                data_ptr += 3;
                for (int i = 0; i < count && mem_addr < 0xFFFF; ++i) {
                    cpu.get_bus()->write(mem_addr++, value);
                }
            } else {
                cpu.get_bus()->write(mem_addr++, data[data_ptr++]);
            }
        }
    } else {
        if (data.size() - 30 != 49152) {
            std::cerr << "Error: Invalid uncompressed 48K Z80 file size." << std::endl;
            return false;
        }
        for (int i = 0; i < 3; ++i) {
            uint16_t mem_addr;
            switch(i) {
                case 0: mem_addr = 0x4000; break; // Page 8
                case 1: mem_addr = 0x8000; break; // Page 4
                case 2: mem_addr = 0xC000; break; // Page 5
            }
            for (int j = 0; j < 16384; ++j)
                cpu.get_bus()->write(mem_addr + j, data[data_ptr++]);
        }
    }
    return true;
}

uint16_t resolve_address(const std::string& addr_str, const Z80<>& cpu) {
    if (addr_str.empty()) {
        throw std::runtime_error("Address argument is empty.");
    }

    size_t plus_pos = addr_str.find('+');
    size_t minus_pos = addr_str.find('-');
    size_t operator_pos = (plus_pos != std::string::npos) ? plus_pos : minus_pos;

    if (operator_pos != std::string::npos) {
        std::string reg_str = addr_str.substr(0, operator_pos);
        std::string offset_str = addr_str.substr(operator_pos + 1);
        char op = addr_str[operator_pos];

        // Trim whitespace
        reg_str.erase(0, reg_str.find_first_not_of(" \t"));
        reg_str.erase(reg_str.find_last_not_of(" \t") + 1);
        offset_str.erase(0, offset_str.find_first_not_of(" \t"));
        offset_str.erase(offset_str.find_last_not_of(" \t") + 1);

        uint16_t base_addr = resolve_address(reg_str, cpu);
        int offset = 0;
        try {
            size_t pos;
            if (offset_str.size() > 2 && (offset_str.substr(0, 2) == "0x" || offset_str.substr(0, 2) == "0X")) {
                offset = std::stoi(offset_str.substr(2), &pos, 16);
                pos += 2;
            } else {
                offset = std::stoi(offset_str, &pos, 10);
            }
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
        if (upper_str == "PC") return cpu.get_PC();
        if (upper_str == "SP") return cpu.get_SP();
        if (upper_str == "HL") return cpu.get_HL();
        if (upper_str == "BC") return cpu.get_BC();
        if (upper_str == "DE") return cpu.get_DE();
        if (upper_str == "IX") return cpu.get_IX();
        if (upper_str == "IY") return cpu.get_IY();
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
    long long run_steps = 0;
    bool reg_dump_action = false;
    std::string reg_dump_format;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mem-dump") {
            mem_dump_addr_str = argv[++i];
            mem_dump_size = std::stoul(argv[++i], nullptr, 16);
        } else if (arg == "--disassemble") {
            disasm_addr_str = argv[++i];
            disasm_lines = std::stoul(argv[++i], nullptr, 10);
        } else if (arg == "--load-addr") {
            load_addr_str = argv[++i];
        } else if (arg == "--reg-dump") {
            reg_dump_action = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                reg_dump_format = argv[++i];
            }
        } else if (arg == "--run-ticks") {
            run_ticks = std::stoll(argv[++i], nullptr, 10);
        } else if (arg == "--run-steps") {
            run_steps = std::stoll(argv[++i], nullptr, 10);
        } else {
            std::cerr << "Error: Unknown or incomplete argument '" << arg << "'." << std::endl;
            print_usage();
            return 1;
        }
    }
    auto file_data = read_file(file_path);
    if (file_data.empty()) {
        std::cerr << "Error: Could not read file '" << file_path << "'." << std::endl;
        return 1;
    }
    Z80<> cpu;
    Z80Analyzer<Z80DefaultBus, Z80<>> analyzer(cpu.get_bus(), &cpu);
    std::string ext = get_file_extension(file_path);
    bool loaded = false;
    std::cout << "Loading file: " << file_path << " (type: " << (ext.empty() ? "bin" : ext) << ")" << std::endl;
    if (ext == "sna") {
        loaded = load_sna_file(cpu, file_data);
    } else if (ext == "z80") {
        loaded = load_z80_file(cpu, file_data);
    } else if (ext == "bin" || ext.empty()) {
        uint16_t load_addr = resolve_address(load_addr_str, cpu);
        loaded = load_bin_file(*cpu.get_bus(), file_data, load_addr);
        cpu.set_PC(load_addr);
    } else {
        std::cerr << "Error: Unsupported file extension '" << ext << "'." << std::endl;
        return 1;
    }
    if (!loaded) {
        std::cerr << "Error: Failed to load file content into emulator." << std::endl;
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
        for (long long i = 0; i < run_steps; ++i) {
            total_ticks_for_steps += cpu.step();
        }
        std::cout << "Executed " << run_steps << " instructions (" << total_ticks_for_steps << " T-states). CPU is now at tick " << cpu.get_ticks() << ".\n" << std::endl;
    }
    if (mem_dump_size == 0 && disasm_lines == 0 && !reg_dump_action) {
        reg_dump_action = true;
    }
    if (reg_dump_action) {
        std::string format = reg_dump_format.empty() ? "AF=%af BC=%bc DE=%de HL=%hl IX=%ix IY=%iy PC=%pc SP=%sp | %flags" : reg_dump_format;
        std::cout << "--- Register Dump ---\n";
        std::cout << analyzer.dump_registers(format) << std::endl;
    }
    if (mem_dump_size > 0) {
        uint16_t mem_dump_addr = resolve_address(mem_dump_addr_str, cpu);
        std::cout << "\n--- Memory Dump from " << format_hex(mem_dump_addr, 4)
                << " (" << mem_dump_size << " bytes) ---\n";
        
        uint16_t current_addr = mem_dump_addr;
        size_t bytes_remaining = mem_dump_size;
        const size_t cols = 16;

        while (bytes_remaining > 0) {
            size_t rows_to_dump = (bytes_remaining + cols - 1) / cols;
            if (rows_to_dump == 0) break;

            auto dump = analyzer.dump_memory(current_addr, 1, cols);
            for (const auto& line : dump) {
                std::cout << line << std::endl;
            }
            
            size_t bytes_dumped = std::min(bytes_remaining, cols);
            bytes_remaining -= bytes_dumped;
        }
    }
    if (disasm_lines > 0) {
        uint16_t disasm_addr = resolve_address(disasm_addr_str, cpu);
        std::cout << "\n--- Disassembly from " << format_hex(disasm_addr, 4)
                << " (" << disasm_lines << " lines) ---\n";
        
        uint16_t pc = disasm_addr;
        auto listing = analyzer.disassemble(pc, disasm_lines);
        for (const auto& line : listing) {
            std::cout << line << std::endl;
        }
    }
    if (mem_dump_size == 0 && disasm_lines == 0 && !reg_dump_action) {
        std::cout << "\nNo action specified. Use --reg-dump, --mem-dump, or --disassemble to see output." << std::endl;
    }
    return 0;
}
