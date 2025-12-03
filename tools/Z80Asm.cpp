//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm.cpp
// Verson: 1.1.0
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

void print_usage() {
    std::cerr << "Usage: Z80Asm <input_file> [options]\n"
              << "Options:\n"
              << "  --bin <output_bin_file>  Specify the output binary file path.\n"
              << "  --hex <output_hex_file>  Specify the output Intel HEX file path.\n"
              << "  --map <output_map_file>  Specify the output map file path.\n"
              << "If no output options are provided, the result is printed to the screen only.\n";
}

class FileSystemSourceProvider : public IFileProvider {
public:
    bool read_file(const std::string& identifier, std::vector<uint8_t>& data) override {
        std::filesystem::path file_path;
        if (m_current_path_stack.empty())
            file_path = std::filesystem::canonical(identifier);
        else
            file_path = std::filesystem::canonical(m_current_path_stack.back().parent_path() / identifier);
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
        file << std::setw(20) << std::left << std::setfill(' ') << symbol.first
             << " EQU $" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
             << symbol.second.value << std::endl;
    }
}

void write_hex_file(const std::string& file_path, const Z80DefaultBus& bus, const std::vector<Z80Assembler<Z80DefaultBus>::BlockInfo>& blocks) {
    std::ofstream file(file_path);
    const size_t bytes_per_line = 16;
    for (const auto& block : blocks) {
        uint16_t current_addr = block.start_address;
        uint16_t remaining_len = block.size;

        while (remaining_len > 0) {
            uint8_t line_len = std::min((size_t)remaining_len, bytes_per_line);
            uint8_t checksum = 0;

            file << ":" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)line_len;
            checksum += line_len;

            file << std::setw(4) << std::setfill('0') << current_addr;
            checksum += (current_addr >> 8) & 0xFF;
            checksum += current_addr & 0xFF;

            file << "00"; // Record type 00 (Data)

            for (uint8_t i = 0; i < line_len; ++i) {
                uint8_t byte = bus.peek(current_addr + i);
                file << std::setw(2) << std::setfill('0') << (int)byte;
                checksum += byte;
            }
            file << std::setw(2) << std::setfill('0') << (int)((-checksum) & 0xFF) << std::endl;
            current_addr += line_len;
            remaining_len -= line_len;
        }
    }
    file << ":00000001FF" << std::endl; // End of File record
}

void write_bin_file(const std::string& file_path, const Z80DefaultBus& bus, const std::vector<Z80Assembler<Z80DefaultBus>::BlockInfo>& blocks) {
    if (blocks.empty())
        return;
    // Find the overall memory range
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
    std::vector<uint8_t> image(total_size, 0x00); // Fill with 0x00 by default

    // Copy the data from each block into the image
    for (const auto& block : blocks) {
        for (uint16_t i = 0; i < block.size; ++i)
            image[block.start_address - min_addr + i] = bus.peek(block.start_address + i);
    }

    std::ofstream file(file_path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(image.data()), image.size());
}

#ifndef Z80ASM_TEST_BUILD
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string input_file;
    std::string output_bin_file, output_hex_file;
    std::string output_map_file;

    input_file = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--bin" && i + 1 < argc) {
            output_bin_file = argv[++i];
        } else if (arg == "--hex" && i + 1 < argc) {
            output_hex_file = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            output_map_file = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    Z80<> cpu;
    Z80DefaultBus bus;
    FileSystemSourceProvider source_provider;
    Z80Assembler<Z80DefaultBus> assembler(&bus, &source_provider);

    try {
        std::cout << "Assembling source code from: " << input_file << std::endl;

        if (assembler.compile(input_file, 0x0000)) {
            std::cout << "\n--- Assembly Successful ---\n" << std::endl;

            // Print symbols
            const auto& symbols = assembler.get_symbols();
            std::cout << "--- Calculated Symbols ---" << std::endl;
            for (const auto& symbol : symbols) {
                std::stringstream hex_val;
                hex_val << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<uint16_t>(symbol.second.value);

                std::cout << std::setw(20) << std::left << std::setfill(' ') << symbol.first << " = " << hex_val.str() << " (" << std::dec << symbol.second.value << ")" << std::endl;
            }
            std::cout << std::endl;

            // Print memory map of code blocks
            Z80Analyzer<Z80DefaultBus, Z80<>, Z80DefaultLabels> analyzer(&bus, &cpu, nullptr);
            auto blocks = assembler.get_blocks();
            std::cout << "--- Code Blocks ---" << std::endl;
            for (size_t i = 0; i < blocks.size(); ++i) {
                const auto& block = blocks[i];
                uint16_t start_addr = block.start_address;
                uint16_t len = block.size;

                std::cout << "--- Block #" << i << ": Address=0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << start_addr << ", Size=" << std::dec << len << " bytes ---\n";

                if (len > 0) {
                    uint16_t dump_addr = start_addr;
                    auto dump = analyzer.dump_memory(dump_addr, (len + 15) / 16, 16);
                    for (const auto& line : dump) {
                        std::cout << line << std::endl;
                    }
                    std::cout << "\n--- Disassembly for Block #" << i << " ---\n";
                    uint16_t disasm_addr = start_addr;
                    while (disasm_addr < start_addr + len) {
                        auto listing = analyzer.disassemble(disasm_addr, 1);
                        for (const auto& line : listing) {
                            std::cout << line << std::endl;
                        }
                    }
                }
                std::cout << std::endl;
            }

            // Write output files
            if (!output_bin_file.empty()) {
                write_bin_file(output_bin_file, bus, blocks);
                std::cout << "Binary code written to " << output_bin_file << std::endl;
            }
            if (!output_hex_file.empty()) {
                write_hex_file(output_hex_file, bus, blocks);
                std::cout << "Intel HEX code written to " << output_hex_file << std::endl;
            }
            if (!output_map_file.empty()) {
                write_map_file(output_map_file, symbols);
                std::cout << "Symbols written to " << output_map_file << std::endl;
            }

        }
    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
#endif
