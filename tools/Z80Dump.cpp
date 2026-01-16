
//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Dump.cpp
// Verson: 1.0
//
// This file contains a command-line utility for dumping memory, registers,
// and disassembling code from Z80 binary files and snapshots.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Decoder.h"
#include "Z80.h"
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
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << static_cast<int>(value);
    return ss.str();
}

void print_usage() {
    std::cerr << "Usage: Z80Dump <file_path> [options]\n"
              << "File formats supported: .bin, .sna, .z80\n\n"
              << "Options:\n"
              << "  -mem <address> <bytes_dec>\n"
              << "    Dumps memory from the specified <address> (hex/dec) for a number of <bytes_dec> (dec).\n"
              << "    Example: -mem 4000 100\n"
              << "  -dasm <address> <lines_dec>\n"
              << "    Disassembles code from the specified <address> (hex/dec) for a number of <lines_dec> (dec).\n"
              << "    Example: -dasm 8000 20\n"
              ;
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos)
        return "";
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

bool load_bin_file(Z80StandardBus& bus, const std::vector<uint8_t>& data, uint16_t load_addr) {
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
    if (data.size() != 49179) {
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
    for (size_t i = 0; i < 49152; ++i)
        cpu.get_bus()->write(0x4000 + i, data[27 + i]);
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
    if (byte12 == 0xFF)
        byte12 = 1;
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
                if (data_ptr + 3 >= data.size())
                    break; // Corrupted sequence
                uint8_t count = data[data_ptr + 2];
            uint8_t value = data[data_ptr + 3];
            data_ptr += 4;
                for (int i = 0; i < count && mem_addr < 0xFFFF; ++i)
                    cpu.get_bus()->write(mem_addr++, value);
            } else
                cpu.get_bus()->write(mem_addr++, data[data_ptr++]);
        }
    } else {
        if (data.size() - 30 != 49152) {
            std::cerr << "Error: Invalid uncompressed 48K Z80 file size." << std::endl;
            return false;
        }
        for (int i = 0; i < 3; ++i) {
            uint16_t mem_addr;
            switch (i) {
            case 0:
                mem_addr = 0x4000;
                break;
            case 1:
                mem_addr = 0x8000;
                break;
            case 2:
                mem_addr = 0xC000;
                break;
            }
            for (int j = 0; j < 16384; ++j)
                cpu.get_bus()->write(mem_addr + j, data[data_ptr++]);
        }
    }
    return true;
}

uint16_t resolve_address(const std::string& addr_str, const Z80<>& cpu) {
    if (addr_str.empty())
        throw std::runtime_error("Address argument is empty.");
    try {
        std::string upper_str = addr_str;
        std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
        if (upper_str.size() > 2 && upper_str.substr(0, 2) == "0X")
            return std::stoul(upper_str.substr(2), nullptr, 16);
        else if (upper_str.back() == 'H')
            return std::stoul(upper_str.substr(0, upper_str.length() - 1), nullptr, 16);
        else
            return std::stoul(addr_str, nullptr, 10);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid address format: " + addr_str);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Address value out of range: " + addr_str);
    }
}

std::string format_bytes_str(const std::vector<uint8_t>& bytes, bool hex) {
    std::stringstream ss;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (hex)
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
        else
            ss << std::dec << static_cast<int>(bytes[i]);
        if (i < bytes.size() - 1)
            ss << " ";
    }
    return ss.str();
}
class DumpLabelHandler : public ILabels {
public:
    void load_map(const std::string& content) {
        std::stringstream file(content);
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            uint16_t address;
            std::string label;
            ss >> std::hex >> address >> std::dec >> label;
            if (!ss.fail() && !label.empty())
                add_label(address, label);
        }
    }
    std::string get_label(uint16_t address) const override { auto it = m_labels.find(address); return (it != m_labels.end()) ? it->second : ""; }
    void add_label(uint16_t address, const std::string& label) { m_labels[address] = label; }
private:
    std::map<uint16_t, std::string> m_labels;
};

using Decoder = Z80Decoder<Z80StandardBus>;

std::string format_operands(const std::vector<Decoder::CodeLine::Operand>& operands) {
    if (operands.empty())
        return "";
    std::stringstream ss;
    for (size_t i = 0; i < operands.size(); ++i) {
        const auto& op = operands[i];
        switch (op.type) {
            case Decoder::CodeLine::Operand::REG8:
            case Decoder::CodeLine::Operand::REG16:
            case Decoder::CodeLine::Operand::CONDITION:
                ss << op.s_val;
                break;
            case Decoder::CodeLine::Operand::IMM8:
                ss << format_hex(static_cast<uint8_t>(op.num_val), 2);
                break;
            case Decoder::CodeLine::Operand::IMM16:
            case Decoder::CodeLine::Operand::MEM_IMM16: {
                std::string formatted_addr = op.label.empty() ? format_hex(static_cast<uint16_t>(op.num_val), 4) : op.label;
                if (op.type == Decoder::CodeLine::Operand::MEM_IMM16) ss << "(" << formatted_addr << ")";
                else ss << formatted_addr;
                break;
            }
            case Decoder::CodeLine::Operand::MEM_REG16:
                ss << "(" << op.s_val << ")";
                break;
            case Decoder::CodeLine::Operand::MEM_INDEXED:
                ss << "(" << op.base_reg << (op.offset >= 0 ? "+" : "") << static_cast<int>(op.offset) << ")";
                break;
            case Decoder::CodeLine::Operand::PORT_IMM8:
                ss << "(" << format_hex(static_cast<uint8_t>(op.num_val), 2) << ")";
                break;
            case Decoder::CodeLine::Operand::STRING:
                ss << "\"" << op.s_val << "\"";
                break;
            case Decoder::CodeLine::Operand::CHAR_LITERAL:
                ss << "'" << static_cast<char>(op.num_val) << "'";
                break;
            case Decoder::CodeLine::Operand::UNKNOWN:
                ss << "?";
                break;
        }
        if (i < operands.size() - 1)
            ss << ", ";
    }
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    std::string file_path = argv[1];
    std::string mem_dump_addr_str, disasm_addr_str;
    size_t mem_dump_size = 0, disasm_lines = 0;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-mem" && i + 2 < argc) {
            mem_dump_addr_str = argv[++i];
            mem_dump_size = std::stoul(argv[++i], nullptr, 10);
        } else if (arg == "-dasm") {
            if (i + 2 >= argc) {
                std::cerr << "Error: -dasm requires at least <address> and <lines>." << std::endl;
                return 1;
            }
            disasm_addr_str = argv[++i];
            disasm_lines = std::stoul(argv[++i], nullptr, 10);
        }
        else {
            bool is_known_arg_with_missing_value =
                (arg == "-mem" && i + 2 >= argc);
            if (is_known_arg_with_missing_value)
                std::cerr << "Error: Incomplete argument for '" << arg << "'. Expected two values." << std::endl;
            else
                std::cerr << "Error: Unknown or incomplete argument '" << arg << "'." << std::endl;
            print_usage();
            return 1;
        }
    }    
    std::string ext = get_file_extension(file_path);
    std::vector<uint8_t> file_data_bytes = read_file(file_path);
    if (file_data_bytes.empty()) {
        std::cerr << "Error: Could not read file or file is empty '" << file_path << "'." << std::endl;
        return 1;
    }
    Z80<> cpu;
    DumpLabelHandler label_handler;
    Decoder decoder(cpu.get_bus(), &label_handler);
    std::string map_file_path = file_path;
    size_t dot_pos = map_file_path.rfind('.');
    if (dot_pos != std::string::npos)
        map_file_path.replace(dot_pos, std::string::npos, ".map");
    try {
        std::ifstream map_file(map_file_path);
        if (map_file) {
            std::stringstream buffer;
            buffer << map_file.rdbuf();
            label_handler.load_map(buffer.str());
            std::cout << "Loaded labels from " << map_file_path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading map file: " << e.what() << std::endl;
    }
    bool loaded = false;
    std::cout << "Loading file: " << file_path << " (type: " << (ext.empty() ? "bin" : ext) << ")" << std::endl;
    if (ext == "sna")
        loaded = load_sna_file(cpu, file_data_bytes);
    else if (ext == "z80")
        loaded = load_z80_file(cpu, file_data_bytes);
    else if (ext == "bin" || ext.empty()) {
        loaded = load_bin_file(*cpu.get_bus(), file_data_bytes, 0x0000);
        cpu.set_PC(0x0000);
    } else {
        std::cerr << "Error: Unsupported file extension '" << ext << "'." << std::endl;
        return 1;
    }
    if (!loaded) {
        std::cerr << "Error: Failed to load file content into emulator." << std::endl;
        return 1;
    }
    std::cout << "File loaded successfully." << std::endl;
    if (mem_dump_size > 0) {
        uint16_t mem_dump_addr = resolve_address(mem_dump_addr_str, cpu);
        std::cout << "--- Memory Dump from " << format_hex(mem_dump_addr, 4) << " (" << mem_dump_size
                  << " bytes) ---\n";
        uint16_t current_addr = mem_dump_addr;
        size_t bytes_remaining = mem_dump_size;
        const size_t cols = 16;
        for (size_t i = 0; i < mem_dump_size; i += cols) {
            std::cout << format_hex(current_addr, 4) << ": ";
            char original_fill = std::cout.fill(); // Save original fill character
            std::string ascii_chars;
            for (size_t j = 0; j < cols; ++j) {
                if (i + j < mem_dump_size) {
                    uint8_t byte = cpu.get_bus()->peek(current_addr + j);
                    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    ascii_chars += (isprint(byte) ? (char)byte : '.');
                } else
                    std::cout << "   ";
            }
            std::cout << " " << ascii_chars << std::endl;
            current_addr += cols;
        }
    }
    if (disasm_lines > 0) {
        uint16_t disasm_addr = resolve_address(disasm_addr_str, cpu);
        std::cout << "--- Disassembly from " << format_hex(disasm_addr, 4) << " (" << std::dec << disasm_lines << " lines) ---\n";
        uint16_t pc = disasm_addr;
        std::cout << std::setfill(' ');
        for (size_t i = 0; i < disasm_lines; ++i) {
            auto line_info = decoder.parse_instruction(pc);
            uint16_t start_pc = line_info.address;
            std::string ticks_str;
            if (line_info.ticks > 0)
                ticks_str = "(" + std::to_string(line_info.ticks) + (line_info.ticks_alt > 0 ? "/" + std::to_string(line_info.ticks_alt) : "") + "T)";
            if (!line_info.label.empty()) {
                std::cout << line_info.label << ":" << std::endl;
            }
            std::cout << "\t";
            std::cout << std::left << format_hex(start_pc, 4) << "  " << std::setw(24) << format_bytes_str(line_info.bytes, true) << " "
                      << std::setw(10) << ticks_str << " " << std::setw(7) << line_info.mnemonic << " " << std::setw(18) << format_operands(line_info.operands) << std::endl;
            pc += line_info.bytes.size();
            if (line_info.bytes.empty()) pc++;
        }
    }
    if (mem_dump_size == 0 && disasm_lines == 0) {
        std::cout << "\nNo action specified. Use -mem, or -dasm to see output."
                  << std::endl;
    }
    return 0;
}
