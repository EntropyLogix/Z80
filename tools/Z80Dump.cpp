
//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Dump.cpp
// Verson: 1.1.1
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
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << static_cast<int>(value);
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

enum class IntelHexRecordType : uint8_t {
    Data = 0x00,
    EndOfFile = 0x01,
    ExtendedLinearAddress = 0x04,
};

bool load_hex_file(Z80DefaultBus& bus, const std::string& content) {
    std::stringstream file_stream(content);
    std::string line;
    uint32_t extended_linear_address = 0;
    while (std::getline(file_stream, line)) {
        if (line.empty() || line[0] != ':')
            continue;
        try {
            uint8_t byte_count = std::stoul(line.substr(1, 2), nullptr, 16);
            uint16_t address = std::stoul(line.substr(3, 4), nullptr, 16);
            uint8_t record_type = std::stoul(line.substr(7, 2), nullptr, 16);
            if (line.length() < 11 + (size_t)byte_count * 2) {
                std::cerr << "Warning: Malformed HEX line (too short): " << line << std::endl;
                continue;
            }
            uint8_t checksum = byte_count + (address >> 8) + (address & 0xFF) + record_type;
            std::vector<uint8_t> data_bytes;
            data_bytes.reserve(byte_count);
            for (int i = 0; i < byte_count; ++i) {
                uint8_t byte = std::stoul(line.substr(9 + i * 2, 2), nullptr, 16);
                data_bytes.push_back(byte);
                checksum += byte;
            }
            uint8_t file_checksum = std::stoul(line.substr(9 + byte_count * 2, 2), nullptr, 16);
            if (((checksum + file_checksum) & 0xFF) != 0) {
                std::cerr << "Warning: Checksum error in HEX line: " << line << std::endl;
                continue;
            }
            switch (static_cast<IntelHexRecordType>(record_type)) {
            case IntelHexRecordType::Data: {
                uint32_t full_address = extended_linear_address + address;
                for (size_t i = 0; i < data_bytes.size(); ++i) {
                    if (full_address + i <= 0xFFFF)
                        bus.write(full_address + i, data_bytes[i]);
                }
                break;
            }
            case IntelHexRecordType::EndOfFile:
                return true;
            case IntelHexRecordType::ExtendedLinearAddress:
                if (byte_count == 2) 
                    extended_linear_address = (data_bytes[0] << 24) | (data_bytes[1] << 16);
                break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not parse HEX line: " << line << " (" << e.what() << ")" << std::endl;
        }
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
                uint8_t value = data[data_ptr + 2];
                data_ptr += 3;
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

std::string format_flags_string(const Z80<>& cpu) {
    std::stringstream ss;
    auto f = cpu.get_F();
    using Flags = Z80<>::Flags;
    ss << (f.is_set(Flags::S) ? 'S' : '-');
    ss << (f.is_set(Flags::Z) ? 'Z' : '-');
    ss << (f.is_set(Flags::Y) ? 'Y' : '-');
    ss << (f.is_set(Flags::H) ? 'H' : '-');
    ss << (f.is_set(Flags::X) ? 'X' : '-');
    ss << (f.is_set(Flags::PV) ? 'P' : '-');
    ss << (f.is_set(Flags::N) ? 'N' : '-');
    ss << (f.is_set(Flags::C) ? 'C' : '-');
    return ss.str();
}

std::string format_register_segment(const std::string& specifier, const Z80<>& cpu) {
    std::string s_lower = specifier;
    std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(), ::tolower);
    bool is_upper = (specifier.length() > 0 && isupper(specifier[0]));
    if (s_lower == "af")
        return is_upper ? std::to_string(cpu.get_AF()) : format_hex(cpu.get_AF(), 4);
    if (s_lower == "bc")
        return is_upper ? std::to_string(cpu.get_BC()) : format_hex(cpu.get_BC(), 4);
    if (s_lower == "de")
        return is_upper ? std::to_string(cpu.get_DE()) : format_hex(cpu.get_DE(), 4);
    if (s_lower == "hl")
        return is_upper ? std::to_string(cpu.get_HL()) : format_hex(cpu.get_HL(), 4);
    if (s_lower == "ix")
        return is_upper ? std::to_string(cpu.get_IX()) : format_hex(cpu.get_IX(), 4);
    if (s_lower == "iy")
        return is_upper ? std::to_string(cpu.get_IY()) : format_hex(cpu.get_IY(), 4);
    if (s_lower == "sp")
        return is_upper ? std::to_string(cpu.get_SP()) : format_hex(cpu.get_SP(), 4);
    if (s_lower == "pc")
        return is_upper ? std::to_string(cpu.get_PC()) : format_hex(cpu.get_PC(), 4);
    if (s_lower == "af'")
        return is_upper ? std::to_string(cpu.get_AFp()) : format_hex(cpu.get_AFp(), 4);
    if (s_lower == "bc'")
        return is_upper ? std::to_string(cpu.get_BCp()) : format_hex(cpu.get_BCp(), 4);
    if (s_lower == "de'")
        return is_upper ? std::to_string(cpu.get_DEp()) : format_hex(cpu.get_DEp(), 4);
    if (s_lower == "hl'")
        return is_upper ? std::to_string(cpu.get_HLp()) : format_hex(cpu.get_HLp(), 4);
    if (s_lower == "a")
        return is_upper ? std::to_string(cpu.get_A()) : format_hex(cpu.get_A(), 2);
    if (s_lower == "f")
        return is_upper ? std::to_string(cpu.get_F()) : format_hex(cpu.get_F(), 2);
    if (s_lower == "b")
        return is_upper ? std::to_string(cpu.get_B()) : format_hex(cpu.get_B(), 2);
    if (s_lower == "c")
        return is_upper ? std::to_string(cpu.get_C()) : format_hex(cpu.get_C(), 2);
    if (s_lower == "d")
        return is_upper ? std::to_string(cpu.get_D()) : format_hex(cpu.get_D(), 2);
    if (s_lower == "e")
        return is_upper ? std::to_string(cpu.get_E()) : format_hex(cpu.get_E(), 2);
    if (s_lower == "h")
        return is_upper ? std::to_string(cpu.get_H()) : format_hex(cpu.get_H(), 2);
    if (s_lower == "l")
        return is_upper ? std::to_string(cpu.get_L()) : format_hex(cpu.get_L(), 2);
    if (s_lower == "i")
        return is_upper ? std::to_string(cpu.get_I()) : format_hex(cpu.get_I(), 2);
    if (s_lower == "r")
        return is_upper ? std::to_string(cpu.get_R()) : format_hex(cpu.get_R(), 2);
    if (s_lower == "flags")
        return format_flags_string(cpu);
    return "%" + specifier;
}

std::string dump_registers(const std::string& format, const Z80<>& cpu) {
    std::stringstream ss;
    for (size_t i = 0; i < format.length(); ++i) {
        if (format[i] == '%' && i + 1 < format.length()) {
            std::string specifier;
            size_t j = i + 1;
            while (j < format.length() && (isalnum(format[j]) || format[j] == '\''))
                specifier += format[j++];
            if (!specifier.empty()) {
                ss << format_register_segment(specifier, cpu);
                i = j - 1;
            } else
                ss << format[i];
        } else if (format[i] == '\\' && i + 1 < format.length()) {
            switch (format[i + 1]) {
            case 'n': ss << '\n'; break;
            case 't': ss << '\t'; break;
            default: ss << format[i + 1]; break;
            }
            i++;
        } else
            ss << format[i];
    }
    return ss.str();
}

std::string format_bytes_str(const std::vector<uint8_t>& bytes, bool hex) {
    std::stringstream ss;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (hex)
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
        else
            ss << std::dec << static_cast<int>(bytes[i]);
        if (i < bytes.size() - 1) ss << " ";
    }
    return ss.str();
}

using Analyzer = Z80Analyzer<Z80DefaultBus, Z80<>, Z80DefaultLabels>;

std::string format_operands(const std::vector<Analyzer::Operand>& operands) {
    if (operands.empty()) return "";
    std::stringstream ss;
    for (size_t i = 0; i < operands.size(); ++i) {
        const auto& op = operands[i];
        switch (op.type) {
            case Analyzer::Operand::Type::REG8:
            case Analyzer::Operand::Type::REG16:
            case Analyzer::Operand::Type::CONDITION:
                ss << op.s_val;
                break;
            case Analyzer::Operand::Type::IMM8:
                ss << format_hex(static_cast<uint8_t>(op.num_val), 2);
                break;
            case Analyzer::Operand::Type::IMM16:
            case Analyzer::Operand::Type::MEM_IMM16: {
                std::string formatted_addr = op.label.empty() ? format_hex(op.num_val, 4) : op.label;
                if (op.type == Analyzer::Operand::Type::MEM_IMM16) ss << "(" << formatted_addr << ")";
                else ss << formatted_addr;
                break;
            }
            case Analyzer::Operand::Type::MEM_REG16:
                ss << "(" << op.s_val << ")";
                break;
            case Analyzer::Operand::Type::MEM_INDEXED:
                ss << "(" << op.s_val << (op.offset >= 0 ? "+" : "") << static_cast<int>(op.offset) << ")";
                break;
            case Analyzer::Operand::Type::PORT_IMM8:
                ss << "(" << format_hex(static_cast<uint8_t>(op.num_val), 2) << ")";
                break;
            case Analyzer::Operand::Type::STRING:
                ss << "\"" << op.s_val << "\"";
                break;
        }
        if (i < operands.size() - 1) ss << ", ";
    }
    return ss.str();
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
    Analyzer::DisassemblyMode disassembly_mode = Analyzer::DisassemblyMode::RAW;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--mem-dump" || arg == "-mem-dump") && i + 2 < argc) {
            mem_dump_addr_str = argv[++i];
            mem_dump_size = std::stoul(argv[++i], nullptr, 16);
        } else if ((arg == "--disassemble" || arg == "-disassemble")) {
            if (i + 2 >= argc) {
                std::cerr << "Error: --disassemble requires at least <address> and <lines>." << std::endl;
                return 1;
            }
            disasm_addr_str = argv[++i];
            disasm_lines = std::stoul(argv[++i], nullptr, 10);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                std::string mode_str = argv[++i];
                std::transform(mode_str.begin(), mode_str.end(), mode_str.begin(), ::tolower);
                if (mode_str == "heuristic") {
                    disassembly_mode = Analyzer::DisassemblyMode::HEURISTIC;
                } else if (mode_str != "raw") {
                    std::cerr << "Error: Invalid disassembly mode '" << mode_str << "'. Use 'raw' or 'heuristic'." << std::endl;
                    return 1;
                }
            }
        }
        else if ((arg == "--load-addr" || arg == "-load-addr") && i + 1 < argc)
            load_addr_str = argv[++i];
        else if ((arg == "--map" || arg == "-map") && i + 1 < argc)
            map_files.push_back(argv[++i]);
        else if ((arg == "--ctl" || arg == "-ctl") && i + 1 < argc)
            ctl_files.push_back(argv[++i]);
        else if (arg == "--reg-dump" || arg == "-reg-dump") {
            reg_dump_action = true;
            if (i + 1 < argc && argv[i + 1][0] != '-')
                reg_dump_format = argv[++i];
        } else if ((arg == "--run-ticks" || arg == "-run-ticks") && i + 1 < argc)
            run_ticks = std::stoll(argv[++i], nullptr, 10);
        else if ((arg == "--run-steps" || arg == "-run-steps") && i + 1 < argc)
            run_steps = std::stoll(argv[++i], nullptr, 10);
        else {
            bool is_known_arg_with_missing_value =
                ((arg == "--mem-dump" || arg == "-mem-dump") && i + 2 >= argc) ||
                ((arg == "--load-addr" || arg == "-load-addr" || arg == "--map" || arg == "-map" || arg == "--ctl" || arg == "-ctl" || arg == "--run-ticks" || arg == "-run-ticks" || arg == "--run-steps" || arg == "-run-steps") && i + 1 >= argc);

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
    Analyzer analyzer(cpu.get_bus(), &cpu, &label_handler);
    try {
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
    if (ext == "hex")
        loaded = load_hex_file(*cpu.get_bus(), file_data_str);
    else if (ext == "sna")
        loaded = load_sna_file(cpu, file_data_bytes);
    else if (ext == "z80")
        loaded = load_z80_file(cpu, file_data_bytes);
    else if (ext == "bin" || ext.empty()) {
        uint16_t load_addr = resolve_address(load_addr_str, cpu);
        loaded = load_bin_file(*cpu.get_bus(), file_data_bytes, load_addr);
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
        std::cout << dump_registers(format, cpu) << std::endl;
    }
    if (mem_dump_size > 0) {
        uint16_t mem_dump_addr = resolve_address(mem_dump_addr_str, cpu);
        std::cout << "\n--- Memory Dump from " << format_hex(mem_dump_addr, 4) << " (" << mem_dump_size
                  << " bytes) ---\n";
        uint16_t current_addr = mem_dump_addr;
        size_t bytes_remaining = mem_dump_size;
        const size_t cols = 16;
        for (size_t i = 0; i < mem_dump_size; i += cols) {
            std::cout << format_hex(current_addr, 4) << ": ";
            std::string ascii_chars;
            for (size_t j = 0; j < cols; ++j) {
                if (i + j < mem_dump_size) {
                    uint8_t byte = cpu.get_bus()->peek(current_addr + j);
                    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    ascii_chars += (isprint(byte) ? (char)byte : '.');
                } else {
                    std::cout << "   ";
                }
            }
            std::cout << " " << ascii_chars << std::endl;
            current_addr += cols;
        }
    }
    if (disasm_lines > 0) {
        uint16_t disasm_addr = resolve_address(disasm_addr_str, cpu);
        std::cout << "\n--- Disassembly from " << format_hex(disasm_addr, 4) << " (" << disasm_lines << " lines) ---\n";
        uint16_t pc = disasm_addr;
        auto listing = analyzer.disassemble(pc, disasm_lines, disassembly_mode); 
        for (const auto& line_info : listing) {
            uint16_t start_pc = line_info.address;
            std::string ticks_str;
            if (line_info.ticks > 0) {
                ticks_str = "(" + std::to_string(line_info.ticks) + (line_info.ticks_alt > 0 ? "/" + std::to_string(line_info.ticks_alt) : "") + "T)";
            }
            if (!line_info.label.empty()) {
                std::cout << line_info.label << ":\t";
            } else {
                std::cout << "\t";
            }
            std::cout << std::left << format_hex(start_pc, 4) << "  " << std::setw(12) << format_bytes_str(line_info.bytes, true) << " "
                      << std::setw(10) << ticks_str << " " << std::setw(7) << line_info.mnemonic << " " << std::setw(18) << format_operands(line_info.operands) << std::endl;
            std::cout << std::setfill(' '); // Reset fill character
        }
    }
    if (mem_dump_size == 0 && disasm_lines == 0 && !reg_dump_action) {
        std::cout << "\nNo action specified. Use --reg-dump, --mem-dump, or "
                     "--disassemble to see output."
                  << std::endl;
    }
    return 0;
}
