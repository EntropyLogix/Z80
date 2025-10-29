//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm.cpp
// Verson: 1.0.5
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
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void print_usage() {
    std::cerr << "Usage: Z80Asm <input_file> [options]\n"
              << "Options:\n"
              << "  -o <output_bin_file>   Specify the output binary file path.\n"
              << "                         Default: <input_file_base>.bin\n"
              << "  -m <output_map_file>   Specify the output map file path.\n"
              << "                         Default: <input_file_base>.map\n";
}

std::string read_source_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        throw std::runtime_error("Cannot open source file: " + file_path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_map_file(const std::string& file_path, const std::map<std::string, int32_t>& symbols) {
    std::ofstream file(file_path);
    if (!file) {
        throw std::runtime_error("Cannot open map file for writing: " + file_path);
    }
    for (const auto& symbol : symbols) {
        file << std::setw(20) << std::left << std::setfill(' ') << symbol.first
             << " EQU $" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
             << static_cast<uint16_t>(symbol.second) << std::endl;
    }
}

void write_bin_file(const std::string& file_path, const Z80DefaultBus& bus, const std::vector<std::pair<uint16_t, uint16_t>>& blocks) {
    if (blocks.empty()) {
        return;
    }

    // Find the overall memory range
    uint16_t min_addr = blocks[0].first;
    uint16_t max_addr = blocks[0].first + blocks[0].second -1;

    for (const auto& block : blocks) {
        if (block.first < min_addr) {
            min_addr = block.first;
        }
        uint16_t block_end_addr = block.first + block.second -1;
        if (block_end_addr > max_addr) {
            max_addr = block_end_addr;
        }
    }

    size_t total_size = max_addr - min_addr + 1;
    std::vector<uint8_t> image(total_size, 0x00); // Fill with 0x00 by default

    // Copy the data from each block into the image
    for (const auto& block : blocks) {
        for (uint16_t i = 0; i < block.second; ++i) {
            image[block.first - min_addr + i] = bus.peek(block.first + i);
        }
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open binary file for writing: " + file_path);
    }
    file.write(reinterpret_cast<const char*>(image.data()), image.size());
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string input_file;
    std::string output_bin_file;
    std::string output_map_file;

    input_file = argv[1];
    size_t dot_pos = input_file.rfind('.');
    std::string base_name = (dot_pos == std::string::npos) ? input_file : input_file.substr(0, dot_pos);

    output_bin_file = base_name + ".bin";
    output_map_file = base_name + ".map";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            output_bin_file = argv[++i];
        } else if (arg == "-m" && i + 1 < argc) {
            output_map_file = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    Z80<> cpu;
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);

    try {
        std::string source_code = read_source_file(input_file);
        std::cout << "Assembling source code from: " << input_file << std::endl;

        if (assembler.compile(source_code)) {
            std::cout << "\n--- Assembly Successful ---\n" << std::endl;

            // Print symbols
            auto symbols = assembler.get_symbols();
            std::cout << "--- Calculated Symbols ---" << std::endl;
            for (const auto& symbol : symbols) {
                std::stringstream hex_val;
                hex_val << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<uint16_t>(symbol.second);

                std::cout << std::setw(20) << std::left << std::setfill(' ') << symbol.first << " = " << hex_val.str() << " (" << std::dec << symbol.second << ")" << std::endl;
            }
            std::cout << std::endl;

            // Print memory map of code blocks
            Z80Analyzer<Z80DefaultBus, Z80<>, void> analyzer(&bus, &cpu, nullptr);
            auto blocks = assembler.get_blocks();
            std::cout << "--- Code Blocks ---" << std::endl;
            for (size_t i = 0; i < blocks.size(); ++i) {
                const auto& block = blocks[i];
                uint16_t start_addr = block.first;
                uint16_t len = block.second;

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
                        std::cout << analyzer.disassemble(disasm_addr, "%a: %-12b %-15m") << std::endl;
                    }
                }
                std::cout << std::endl;
            }

            // Write output files
            write_bin_file(output_bin_file, bus, blocks);
            std::cout << "Binary code written to " << output_bin_file << std::endl;

            write_map_file(output_map_file, symbols);
            std::cout << "Symbols written to " << output_map_file << std::endl;

        }
    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
