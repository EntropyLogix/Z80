//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm.cpp
// Verson: 1.1.1
//
// This file contains a command-line utility for assembling Z80 code.
// It serves as an example of how to use the Z80Assembler class.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Assemble.h"
#include "Z80Analyze.h"
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

template <typename T> std::string format_hex(T value, int width) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << static_cast<int>(value);
    return ss.str();
}

void print_usage() {
    std::cerr << "Usage: Z80Asm <input_file>\n"
              << "Generates <input_file>.bin, <input_file>.map, and <input_file>.lst\n";
}

class FileSystemSourceProvider : public IFileProvider {
public:
    bool read_file(const std::string& identifier, std::vector<uint8_t>& data) override {
        std::filesystem::path file_path = m_current_path_stack.empty() ? 
            std::filesystem::path(identifier) : 
            m_current_path_stack.back().parent_path() / identifier;

        try {
            file_path = std::filesystem::canonical(file_path);
        } catch (const std::filesystem::filesystem_error& e) {
            if (e.code() == std::errc::no_such_file_or_directory) {
                throw std::runtime_error("File not found: " + identifier);
            }
            throw;
        }

        m_current_path_stack.push_back(file_path);
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) {
            m_current_path_stack.pop_back();
            return false;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        data.resize(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        m_current_path_stack.pop_back();
        return true;
    }
    bool exists(const std::string& identifier) override {
        if (m_current_path_stack.empty())
            return std::filesystem::exists(identifier);
        else
            return std::filesystem::exists(m_current_path_stack.back().parent_path() / identifier);
    }
    size_t file_size(const std::string& identifier) override {
        if (m_current_path_stack.empty())
            return std::filesystem::file_size(identifier);
        else
            return std::filesystem::file_size(m_current_path_stack.back().parent_path() / identifier);
    }
private:
    std::vector<std::filesystem::path> m_current_path_stack;
};

void write_map_file(const std::string& file_path, const std::map<std::string, Z80Assembler<Z80DefaultBus>::SymbolInfo>& symbols) {
    std::ofstream file(file_path);
    if (!file)
        throw std::runtime_error("Cannot open map file for writing: " + file_path);
    for (const auto& symbol : symbols) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<uint16_t>(symbol.second.value);
        file << ss.str()
             << " " // Add a space separator
             << std::setw(16) << std::left << std::setfill(' ') << symbol.first
             << "; " << (symbol.second.label ? "label" : "equ")
             << std::endl;
    }
}

void write_bin_file(const std::string& file_path, const Z80DefaultBus& bus, const std::vector<Z80Assembler<Z80DefaultBus>::BlockInfo>& blocks) {
    if (blocks.empty())
        return;
    uint16_t min_addr = blocks[0].start_address;
    uint16_t max_addr = blocks[0].start_address + blocks[0].size -1;
    for (const auto& block : blocks) {
        if (block.start_address < min_addr)
            min_addr = block.start_address;
        uint16_t block_end_addr = block.start_address + block.size -1;
        if (block_end_addr > max_addr)
            max_addr = block_end_addr;
    }
    size_t total_size = max_addr - min_addr + 1;
    std::vector<uint8_t> image(total_size, 0x00);
    for (const auto& block : blocks) {
        for (uint16_t i = 0; i < block.size; ++i)
            image[block.start_address - min_addr + i] = bus.peek(block.start_address + i);
    }
    std::ofstream file(file_path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(image.data()), image.size());
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

void write_lst_file(const std::string& file_path, const std::vector<Z80Assembler<Z80DefaultBus>::ListingLine>& listing) {
    std::ofstream file(file_path);
    if (!file)
        throw std::runtime_error("Cannot open listing file for writing: " + file_path);
    file << std::left << std::setw(7) << "Line" << std::setw(7) << "Addr" << std::setw(18) << "Hex Code" << "Source Code\n";
    file << std::string(80, '-') << '\n';
    for (const auto& line : listing) {
        file << std::setw(5) << std::left << line.source_line.line_number << "  ";
        bool is_label_only = line.bytes.empty() && !line.source_line.content.empty() && std::all_of(line.source_line.content.begin(), line.source_line.content.end(), [](char c){ return isspace(c) || c == ':'; });
        bool has_address = !line.bytes.empty() || is_label_only || (line.source_line.content.find("PROC") != std::string::npos);
        if (has_address) {
            std::stringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << line.address;
            file << std::setw(7) << std::left << addr_ss.str();
        } else
            file << std::setw(7) << " ";
        if (!line.bytes.empty())
            file << std::setw(18) << std::left << format_bytes_str(line.bytes, true);
        else
            file << std::setw(18) << " ";
        file << line.source_line.content << '\n';
    }
}

#ifndef Z80ASM_TEST_BUILD

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    std::string input_file = argv[1];
    if (argc > 2) {
        std::cerr << "Error: Too many arguments." << std::endl;
        print_usage();
        return 1;
    }
    std::filesystem::path input_path(input_file);
    std::string output_bin_file = input_path.replace_extension(".bin").string();
    std::string output_map_file = input_path.replace_extension(".map").string();
    std::string output_lst_file = input_path.replace_extension(".lst").string();
    Z80<> cpu;
    Z80DefaultBus bus;
    FileSystemSourceProvider source_provider;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);
    try {
        std::cout << "Assembling source code from: " << input_file << std::endl;
        if (assembler.compile(input_file, 0x0000)) {
            std::cout << "\n--- Assembly Successful ---\n" << std::endl;
            const auto& symbols = assembler.get_symbols();
            std::cout << "--- Calculated Symbols ---" << std::endl;
            for (const auto& symbol : symbols) {
                std::stringstream hex_val;
                hex_val << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<uint16_t>(symbol.second.value);
                std::cout << std::setw(20) << std::left << std::setfill(' ') << symbol.first << " = " << hex_val.str() << " (" << std::dec << symbol.second.value << ")" << std::endl;
            }
            std::cout << std::endl;
            const auto& blocks = assembler.get_blocks();
            const auto& listing = assembler.get_listing();
            write_bin_file(output_bin_file, bus, blocks);
            std::cout << "Binary code written to " << output_bin_file << std::endl;
            write_map_file(output_map_file, symbols);
            std::cout << "Symbols written to " << output_map_file << std::endl;
            write_lst_file(output_lst_file, listing);
            std::cout << "Listing written to " << output_lst_file << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }
    return 0;
}
#endif//Z80ASM_TEST_BUILD
