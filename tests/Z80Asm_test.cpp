//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm_test.cpp
// Verson: 1.0.5
//
// This file contains unit tests for the Z80Asm tool,
// specifically for the source file preprocessing logic.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "tools/Z80Asm.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <stdexcept>

int tests_passed_asm = 0;
int tests_failed_asm = 0;

#define TEST_CASE_ASM(name)                                       \
    void test_asm_##name();                                       \
    struct TestRegisterer_##name {                                \
        TestRegisterer_##name() {                                 \
            std::cout << "--- Running test: " #name " ---\n";     \
            try {                                                 \
                test_asm_##name();                                \
            } catch (const std::exception& e) {                   \
                std::cerr << "ERROR: " << e.what() << std::endl;  \
                tests_failed_asm++;                               \
            }                                                     \
        }                                                         \
    } test_registerer_##name;                                     \
    void test_asm_##name()

void write_file(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

TEST_CASE_ASM(IncludeDirective_Basic) {
    write_file("main.asm", "LD A, 5\nINCLUDE \"included.asm\"\nADD A, B");
    write_file("included.asm", "LD B, 10\n");

    std::string result = read_source_file("main.asm");
    std::string expected = "LD A, 5\nLD B, 10\n\nADD A, B\n";
    
    if (result == expected) {
        tests_passed_asm++;
    } else {
        tests_failed_asm++;
        std::cerr << "Assertion failed: Basic include failed.\n";
        std::cerr << "  Expected: " << expected << "\n";
        std::cerr << "  Got:      " << result << "\n";
    }
    std::filesystem::remove("main.asm");
    std::filesystem::remove("included.asm");
}

TEST_CASE_ASM(IncludeDirective_Nested) {
    write_file("main.asm", "INCLUDE \"level1.asm\"");
    write_file("level1.asm", "LD A, 1\nINCLUDE \"level2.asm\"");
    write_file("level2.asm", "LD B, 2\n");

    std::string result = read_source_file("main.asm");
    std::string expected = "LD A, 1\nLD B, 2\n\n";

    if (result == expected) {
        tests_passed_asm++;
    } else {
        tests_failed_asm++;
        std::cerr << "Assertion failed: Nested include failed.\n";
        std::cerr << "  Expected: " << expected << "\n";
        std::cerr << "  Got:      " << result << "\n";
    }
    std::filesystem::remove("main.asm");
    std::filesystem::remove("level1.asm");
    std::filesystem::remove("level2.asm");
}

TEST_CASE_ASM(IncludeDirective_CircularDependency) {
    write_file("a.asm", "INCLUDE \"b.asm\"");
    write_file("b.asm", "INCLUDE \"a.asm\"");

    try {
        read_source_file("a.asm");
        tests_failed_asm++;
        std::cerr << "Assertion failed: Circular dependency did not throw an exception.\n";
    } catch (const std::runtime_error& e) {
        tests_passed_asm++;
    }
    std::filesystem::remove("a.asm");
    std::filesystem::remove("b.asm");
}

TEST_CASE_ASM(IncludeDirective_FileNotFound) {
    write_file("main.asm", "INCLUDE \"nonexistent.asm\"");

    try {
        read_source_file("main.asm");
        tests_failed_asm++;
        std::cerr << "Assertion failed: Including a non-existent file did not throw an exception.\n";
    } catch (const std::exception& e) {
        tests_passed_asm++;
    }
    std::filesystem::remove("main.asm");
}

void run_asm_tests() {
    std::cout << "\n=============================\n";
    std::cout << "  Running Z80Asm Tool Tests  \n";
    std::cout << "=============================\n";

    // Test cases are run automatically by static initializers

    std::cout << "\n=============================\n";
    std::cout << "Z80Asm Test summary:\n";
    std::cout << "  Passed: " << tests_passed_asm << "\n";
    std::cout << "  Failed: " << tests_failed_asm << "\n";
    std::cout << "=============================\n";
}