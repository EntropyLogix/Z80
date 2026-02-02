//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Next_test.cpp
// Version: 1.0.0
//
// This file contains unit tests for the Z80 processor emulation class.
//
// Copyright (c) 2025-2026 Adam Szulc
// MIT License

#define Z80_ENABLE_NEXT
#include "Z80.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>
#include <map>
#include <sstream>

// Simple Bus for testing
class TestBus : public Z80::StandardBus {
public:
    struct IOWrite {
        uint16_t port;
        uint8_t value;
    };
    std::vector<IOWrite> io_writes;
    
    // Mocking Next registers
    uint8_t next_reg_select = 0;
    std::map<uint8_t, uint8_t> next_regs;

    void out(uint16_t port, uint8_t value) {
        io_writes.push_back({port, value});
        if (port == 0x243B) {
            next_reg_select = value;
        } else if (port == 0x253B) {
            next_regs[next_reg_select] = value;
        }
    }
    
    uint8_t in(uint16_t port) {
        if (port == 0x253B) {
            return next_regs[next_reg_select];
        }
        return 0xFF;
    }

    void clear_io() {
        io_writes.clear();
        next_regs.clear();
        next_reg_select = 0;
    }
};

int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        tests_passed++;
    } else {
        std::cout << "[FAIL] " << name << "\n";
        tests_failed++;
    }
}

void test_z80n_swapnib() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();
    
    // SWAPNIB opcode: ED 23
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x23);
    
    cpu.set_A(0x12);
    cpu.step(); // Execute ED 23
    
    check(cpu.get_A() == 0x21, "SWAPNIB 0x12 -> 0x21");
    
    cpu.set_PC(0x0000);
    cpu.set_A(0xF0);
    cpu.step();
    check(cpu.get_A() == 0x0F, "SWAPNIB 0xF0 -> 0x0F");

    // Additional tests for nibble swapping
    cpu.set_PC(0x0000);
    cpu.set_A(0x00);
    cpu.step();
    check(cpu.get_A() == 0x00, "SWAPNIB 0x00 -> 0x00");

    cpu.set_PC(0x0000);
    cpu.set_A(0xFF);
    cpu.step();
    check(cpu.get_A() == 0xFF, "SWAPNIB 0xFF -> 0xFF");

    cpu.set_PC(0x0000);
    cpu.set_A(0xA5);
    cpu.step();
    check(cpu.get_A() == 0x5A, "SWAPNIB 0xA5 -> 0x5A");
}

void test_z80n_mirror() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // MIRROR opcode: ED 24
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x24);

    cpu.set_A(0b10000001); // 0x81
    cpu.step();
    check(cpu.get_A() == 0b10000001, "MIRROR 0x81 -> 0x81");

    cpu.set_PC(0x0000);
    cpu.set_A(0b11000000); // 0xC0
    cpu.step();
    check(cpu.get_A() == 0b00000011, "MIRROR 0xC0 -> 0x03");

    // Additional tests for bit reversal
    cpu.set_PC(0x0000);
    cpu.set_A(0xAA); // 10101010
    cpu.step();
    check(cpu.get_A() == 0x55, "MIRROR 0xAA -> 0x55");

    cpu.set_PC(0x0000);
    cpu.set_A(0x01); // 00000001
    cpu.step();
    check(cpu.get_A() == 0x80, "MIRROR 0x01 -> 0x80");

    cpu.set_PC(0x0000);
    cpu.set_A(0x12); // 00010010
    cpu.step();
    check(cpu.get_A() == 0x48, "MIRROR 0x12 -> 0x48");
}

void test_z80n_mul() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // MUL D, E opcode: ED 30
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x30);

    cpu.set_D(10);
    cpu.set_E(20);
    cpu.step();
    check(cpu.get_DE() == 200, "MUL D, E (10 * 20 = 200)");
    
    cpu.set_PC(0x0000);
    cpu.set_D(0xFF);
    cpu.set_E(0xFF);
    cpu.step();
    check(cpu.get_DE() == 0xFE01, "MUL D, E (255 * 255 = 65025)");
}

void test_z80n_add_hl_a() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // ADD HL, A opcode: ED 31
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x31);

    cpu.set_HL(0x1000);
    cpu.set_A(0x20);
    cpu.step();
    check(cpu.get_HL() == 0x1020, "ADD HL, A");
}

void test_z80n_bsla() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // BSLA DE, B opcode: ED 28
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x28);

    cpu.set_DE(0x0001);
    cpu.set_B(4);
    cpu.step();
    check(cpu.get_DE() == 0x0010, "BSLA DE, B (1 << 4 = 16)");
}

void test_z80n_nextreg() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();
    
    // NEXTREG n, n (ED 91 reg val)
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x91);
    cpu.get_bus()->write(0x0002, 0x10); // Register 0x10
    cpu.get_bus()->write(0x0003, 0x55); // Value 0x55
    
    cpu.step();
    
    const auto& writes = cpu.get_bus()->io_writes;
    check(writes.size() == 2, "NEXTREG n, n writes count");
    if (writes.size() >= 2) {
        check(writes[0].port == 0x243B && writes[0].value == 0x10, "NEXTREG select port (0x243B) = 0x10");
        check(writes[1].port == 0x253B && writes[1].value == 0x55, "NEXTREG data port (0x253B) = 0x55");
    }
    
    cpu.get_bus()->clear_io();
    
    // NEXTREG n, A (ED 92 reg)
    cpu.set_PC(0x0004);
    cpu.get_bus()->write(0x0004, 0xED);
    cpu.get_bus()->write(0x0005, 0x92);
    cpu.get_bus()->write(0x0006, 0x20); // Register 0x20
    cpu.set_A(0xAA); // Value 0xAA
    
    cpu.step();
    
    check(writes.size() == 2, "NEXTREG n, A writes count");
    if (writes.size() >= 2) {
        check(writes[0].port == 0x243B && writes[0].value == 0x20, "NEXTREG select port (0x243B) = 0x20");
        check(writes[1].port == 0x253B && writes[1].value == 0xAA, "NEXTREG data port (0x253B) = 0xAA");
    }
}

void test_z80n_nextreg_readback() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // 1. Write to Next Register using NEXTREG instruction
    // NEXTREG 0x15, 0x99 (ED 91 15 99)
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x91);
    cpu.get_bus()->write(0x0002, 0x15); // Register 0x15
    cpu.get_bus()->write(0x0003, 0x99); // Value 0x99
    cpu.step();

    // 2. Read back using standard IO
    // We assume NEXTREG left 0x243B selected to 0x15.
    // LD BC, 0x253B
    // IN A, (C)
    
    cpu.set_PC(0x0004);
    cpu.set_BC(0x253B);
    // IN A, (C) -> ED 78
    cpu.get_bus()->write(0x0004, 0xED);
    cpu.get_bus()->write(0x0005, 0x78);
    
    cpu.step();
    
    check(cpu.get_A() == 0x99, "Read back NEXTREG value (0x99) via port 0x253B");
}

void test_z80n_ldix() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();
    
    // LDIX opcode: ED A4
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xA4);
    
    uint16_t src = 0x1000;
    uint16_t dst = 0x2000;
    cpu.set_HL(src);
    cpu.set_DE(dst);
    cpu.set_BC(1);
    cpu.get_bus()->write(src, 0x55);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    
    cpu.step();
    
    check(cpu.get_bus()->read(dst) == 0x55, "LDIX copy byte");
    check(cpu.get_HL() == src + 1, "LDIX HL increment");
    check(cpu.get_DE() == dst + 1, "LDIX DE increment");
    check(cpu.get_BC() == 0, "LDIX BC decrement");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDIX PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDIX clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDIX clears H");
}

void test_z80n_ldws() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDWS opcode: ED A5
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xA5);

    // Setup HL so L wraps (0x10FF -> 0x1000)
    // Setup DE so D increments (0x2000 -> 0x2100)
    uint16_t src = 0x10FF;
    uint16_t dst = 0x2000;
    cpu.set_HL(src);
    cpu.set_DE(dst);
    cpu.get_bus()->write(src, 0xAA);

    cpu.step();

    check(cpu.get_bus()->read(dst) == 0xAA, "LDWS copy byte");
    check(cpu.get_HL() == 0x1000, "LDWS L increment (wrap)");
    check(cpu.get_DE() == 0x2100, "LDWS D increment");
}

void test_z80n_ldirx() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDIRX opcode: ED B4
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xB4);

    uint16_t src = 0x1000;
    uint16_t dst = 0x2000;
    cpu.set_HL(src);
    cpu.set_DE(dst);
    cpu.set_BC(3);
    cpu.get_bus()->write(src + 0, 0x11);
    cpu.get_bus()->write(src + 1, 0x22);
    cpu.get_bus()->write(src + 2, 0x33);
    cpu.set_F(Z80::CPU<TestBus>::Flags::H); // Set H to ensure it gets cleared

    // Run until PC moves past the instruction (0x0002)
    int steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }

    check(steps == 3, "LDIRX steps count");
    check(cpu.get_bus()->read(dst + 0) == 0x11, "LDIRX byte 0");
    check(cpu.get_bus()->read(dst + 1) == 0x22, "LDIRX byte 1");
    check(cpu.get_bus()->read(dst + 2) == 0x33, "LDIRX byte 2");
    check(cpu.get_BC() == 0, "LDIRX BC=0");
    check(cpu.get_HL() == src + 3, "LDIRX HL updated");
    check(cpu.get_DE() == dst + 3, "LDIRX DE updated");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDIRX PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDIRX clears H");

    // Test PV flag when BC != 0 (single step)
    cpu.set_PC(0);
    cpu.set_BC(2);
    cpu.set_F(Z80::CPU<TestBus>::Flags::H); // Set H again
    cpu.step();
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDIRX PV flag (BC!=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDIRX step clears H");
}

void test_z80n_shifts() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // BSRA DE, B (ED 29)
    // Arithmetic shift right. 0x8000 >> 1 -> 0xC000
    cpu.set_DE(0x8000);
    cpu.set_B(1);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x29);
    cpu.step();
    check(cpu.get_DE() == 0xC000, "BSRA DE, B (0x8000 >> 1)");

    // BSRL DE, B (ED 2A)
    // Logical shift right. 0x8000 >> 1 -> 0x4000
    cpu.set_PC(0);
    cpu.set_DE(0x8000);
    cpu.set_B(1);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2A);
    cpu.step();
    check(cpu.get_DE() == 0x4000, "BSRL DE, B (0x8000 >> 1)");

    // BSRF DE, B (ED 2B)
    // Shift right fill ones. 0x0000 >> 4 -> 0xF000
    cpu.set_PC(0);
    cpu.set_DE(0x0000);
    cpu.set_B(4);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2B);
    cpu.step();
    check(cpu.get_DE() == 0xF000, "BSRF DE, B (0x0000 >> 4)");

    // BRLC DE, B (ED 2C)
    // Rotate left. 0x8000 rot 1 -> 0x0001
    cpu.set_PC(0);
    cpu.set_DE(0x8000);
    cpu.set_B(1);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2C);
    cpu.step();
    check(cpu.get_DE() == 0x0001, "BRLC DE, B (0x8000 rot 1)");
}

void test_z80n_alu_misc() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // TEST n (ED 27 n)
    // AND n, update flags, don't change A
    cpu.set_A(0xFF);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x27); cpu.get_bus()->write(0x0002, 0x00);
    cpu.step();
    check(cpu.get_A() == 0xFF, "TEST n preserves A");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "TEST 0 sets Z flag");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "TEST 0 sets H flag");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "TEST 0 clears N flag");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "TEST 0 clears C flag");

    // ADD DE, A (ED 32)
    cpu.set_PC(0);
    cpu.set_DE(0x1000);
    cpu.set_A(0x20);
    cpu.set_F(Z80::CPU<TestBus>::Flags::C); // Set C to check preservation
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x32);
    cpu.step();
    check(cpu.get_DE() == 0x1020, "ADD DE, A");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "ADD DE, A updates C flag (clears)");

    // ADD HL, nn (ED 34)
    cpu.set_PC(0);
    cpu.set_HL(0x1000);
    cpu.set_F(Z80::CPU<TestBus>::Flags::Z); // Set Z to check preservation
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x34);
    cpu.get_bus()->write(0x0002, 0x55); cpu.get_bus()->write(0x0003, 0x00); // 0x0055
    cpu.step();
    check(cpu.get_HL() == 0x1055, "ADD HL, nn");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "ADD HL, nn preserves Z flag");

    // ADD BC, A (ED 33)
    cpu.set_PC(0);
    cpu.set_BC(0x2000);
    cpu.set_A(0x10);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x33);
    cpu.step();
    check(cpu.get_BC() == 0x2010, "ADD BC, A");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) == 0, "ADD BC, A updates Z flag (clears)");

    // ADD DE, nn (ED 35)
    cpu.set_PC(0);
    cpu.set_DE(0x3000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x35);
    cpu.get_bus()->write(0x0002, 0x22); cpu.get_bus()->write(0x0003, 0x11); // 0x1122
    cpu.step();
    check(cpu.get_DE() == 0x4122, "ADD DE, nn");

    // ADD BC, nn (ED 36)
    cpu.set_PC(0);
    cpu.set_BC(0x4000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x36);
    cpu.get_bus()->write(0x0002, 0x33); cpu.get_bus()->write(0x0003, 0x22); // 0x2233
    cpu.step();
    check(cpu.get_BC() == 0x6233, "ADD BC, nn");

    // ADD BC, A (ED 33)
    cpu.set_PC(0);
    cpu.set_BC(0x2000);
    cpu.set_A(0x10);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x33);
    cpu.step();
    check(cpu.get_BC() == 0x2010, "ADD BC, A");

    // ADD DE, nn (ED 35)
    cpu.set_PC(0);
    cpu.set_DE(0x3000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x35);
    cpu.get_bus()->write(0x0002, 0x22); cpu.get_bus()->write(0x0003, 0x11); // 0x1122
    cpu.step();
    check(cpu.get_DE() == 0x4122, "ADD DE, nn");

    // ADD BC, nn (ED 36)
    cpu.set_PC(0);
    cpu.set_BC(0x4000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x36);
    cpu.get_bus()->write(0x0002, 0x33); cpu.get_bus()->write(0x0003, 0x22); // 0x2233
    cpu.step();
    check(cpu.get_BC() == 0x6233, "ADD BC, nn");
}

void test_z80n_test_flags() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // TEST n (ED 27 n)
    // Performs A AND n, updates flags, preserves A.
    // Flags: S, Z, P/V from result. H=1, N=0, C=0.

    // Case 1: Result 0 (Z set, P/V set (even parity))
    cpu.set_PC(0x0000);
    cpu.set_A(0xFF);
    cpu.set_F(0); // Clear flags
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x27);
    cpu.get_bus()->write(0x0002, 0x00); // n = 0
    cpu.step();

    check(cpu.get_A() == 0xFF, "TEST 0 preserves A");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "TEST 0 sets Z");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::S) == 0, "TEST 0 clears S");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "TEST 0 sets P/V (parity even)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "TEST 0 sets H");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "TEST 0 clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "TEST 0 clears C");

    // Case 2: Result 0x80 (S set, Z clear, P/V clear (odd parity))
    cpu.set_PC(0x0000);
    cpu.set_A(0xFF);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0002, 0x80); // n = 0x80
    cpu.step();

    check(cpu.get_A() == 0xFF, "TEST 0x80 preserves A");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) == 0, "TEST 0x80 clears Z");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::S) != 0, "TEST 0x80 sets S");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "TEST 0x80 clears P/V (parity odd)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "TEST 0x80 sets H");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "TEST 0x80 clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "TEST 0x80 clears C");

    // Case 3: Result 0x03 (S clear, Z clear, P/V set (even parity))
    cpu.set_PC(0x0000);
    cpu.set_A(0xFF);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0002, 0x03); // n = 0x03
    cpu.step();

    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) == 0, "TEST 0x03 clears Z");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::S) == 0, "TEST 0x03 clears S");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "TEST 0x03 sets P/V (parity even)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "TEST 0x03 sets H");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "TEST 0x03 clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "TEST 0x03 clears C");
}

void test_z80n_stack_jump() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // PUSH nn (ED 8A h l) - Big Endian in instruction!
    cpu.set_SP(0x0000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x8A);
    cpu.get_bus()->write(0x0002, 0x12); cpu.get_bus()->write(0x0003, 0x34);
    cpu.step();
    uint16_t sp = cpu.get_SP();
    check(sp == 0xFFFE, "PUSH nn SP decrement");
    uint16_t val = cpu.get_bus()->read(sp) | (cpu.get_bus()->read(sp+1) << 8);
    check(val == 0x1234, "PUSH nn value on stack");

    // JP (C) (ED 98)
    // PC = (PC & 0xC000) | (C << 6)
    cpu.set_PC(0x8000); // 0xC000 mask -> 0x8000
    cpu.set_C(0x01);    // 0x01 << 6 -> 0x0040
    // Target -> 0x8040
    cpu.get_bus()->write(0x8000, 0xED); cpu.get_bus()->write(0x8001, 0x98);
    cpu.step();
    check(cpu.get_PC() == 0x8040, "JP (C)");
}

void test_z80n_io_misc() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // OUTINB (ED 90)
    // OUT (C), (HL); HL++
    cpu.set_BC(0x1234);
    cpu.set_HL(0x2000);
    cpu.get_bus()->write(0x2000, 0x55);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x90);
    cpu.step();
    check(cpu.get_bus()->io_writes.size() == 1, "OUTINB writes");
    if (cpu.get_bus()->io_writes.size() > 0) {
        check(cpu.get_bus()->io_writes[0].port == 0x1234 && cpu.get_bus()->io_writes[0].value == 0x55, "OUTINB value");
    }
    check(cpu.get_HL() == 0x2001, "OUTINB HL increment");

    // PIXELAD (ED 93)
    cpu.set_PC(0);
    cpu.set_DE(0x0000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x93);
    cpu.step();
    check(cpu.get_HL() == 0x4000, "PIXELAD (0,0)");

    // PIXELDN (ED 94)
    cpu.set_PC(0);
    cpu.set_HL(0x4000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x94);
    cpu.step();
    check(cpu.get_HL() == 0x4100, "PIXELDN (0x4000 -> 0x4100)");
}

void test_z80n_ldirscale_scaling() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDIRSCALE (ED B6)
    // HL += BC' after each byte.
    // Repeats until BC=0.

    // Case 1: Downscaling (Step = 2)
    // Source: 0x1000 [0x11, 0x22, 0x33, 0x44, 0x55]
    // Dest:   0x2000
    // Count:  3
    // Step:   2
    // Expected: 0x11, 0x33, 0x55
    
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(3);
    cpu.set_BCp(2); // Step
    
    cpu.get_bus()->write(0x1000, 0x11);
    cpu.get_bus()->write(0x1001, 0x22);
    cpu.get_bus()->write(0x1002, 0x33);
    cpu.get_bus()->write(0x1003, 0x44);
    cpu.get_bus()->write(0x1004, 0x55);
    
    // Instruction at 0x0000
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xB6);
    cpu.set_PC(0x0000);
    
    // Execute until PC advances
    int steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }
    
    check(steps == 3, "LDIRSCALE downscale steps");
    check(cpu.get_bus()->read(0x2000) == 0x11, "LDIRSCALE downscale byte 0");
    check(cpu.get_bus()->read(0x2001) == 0x33, "LDIRSCALE downscale byte 1");
    check(cpu.get_bus()->read(0x2002) == 0x55, "LDIRSCALE downscale byte 2");
    check(cpu.get_HL() == 0x1006, "LDIRSCALE downscale HL final"); // 1000 + 2*3 = 1006
    
    // Case 2: Upscaling/Smear (Step = 0)
    // Source: 0x3000 [0xAA]
    // Dest:   0x4000
    // Count:  3
    // Step:   0
    // Expected: 0xAA, 0xAA, 0xAA
    
    cpu.set_HL(0x3000);
    cpu.set_DE(0x4000);
    cpu.set_BC(3);
    cpu.set_BCp(0);
    
    cpu.get_bus()->write(0x3000, 0xAA);
    cpu.get_bus()->write(0x3001, 0xBB); // Should not be read
    
    cpu.set_PC(0x0000); // Re-run instruction
    
    steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }
    
    check(steps == 3, "LDIRSCALE upscale steps");
    check(cpu.get_bus()->read(0x4000) == 0xAA, "LDIRSCALE upscale byte 0");
    check(cpu.get_bus()->read(0x4001) == 0xAA, "LDIRSCALE upscale byte 1");
    check(cpu.get_bus()->read(0x4002) == 0xAA, "LDIRSCALE upscale byte 2");
    check(cpu.get_HL() == 0x3000, "LDIRSCALE upscale HL final"); // 3000 + 0*3 = 3000
}

void test_z80n_ldirscale_z_flag() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDIRSCALE (ED B6)
    // Verify Z flag behavior.
    
    // Case 1: Finish (BC becomes 0)
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(1);
    cpu.set_BCp(1);
    cpu.get_bus()->write(0x1000, 0x55);
    cpu.set_F(0); // Clear flags
    
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xB6);
    cpu.set_PC(0x0000);
    
    cpu.step();
    
    check(cpu.get_BC() == 0, "LDIRSCALE BC=0");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "LDIRSCALE sets Z flag when finished");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDIRSCALE clears P/V flag when finished");
    
    // Case 2: Continue (BC != 0)
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(2);
    cpu.set_BCp(1);
    cpu.set_F(Z80::CPU<TestBus>::Flags::Z); // Set Z initially to see if it gets cleared
    cpu.set_PC(0);
    
    cpu.step();
    
    check(cpu.get_BC() == 1, "LDIRSCALE BC=1");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) == 0, "LDIRSCALE clears Z flag when not finished");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDIRSCALE sets P/V flag when not finished");
}

void test_z80n_ldpirx_masking() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDPIRX (ED B7)
    // Copies (HL) to (DE), increments HL, DE, decrements BC.
    // If (HL) == A, the byte is NOT copied (transparent).
    // Repeats until BC=0.

    uint16_t src = 0x1000;
    uint16_t dst = 0x2000;
    uint8_t mask = 0xFF; // The transparent color

    cpu.set_HL(src);
    cpu.set_DE(dst);
    cpu.set_BC(4);
    cpu.set_A(mask);

    // Source data: [0x11, 0xFF, 0x22, 0xFF]
    cpu.get_bus()->write(src + 0, 0x11);
    cpu.get_bus()->write(src + 1, 0xFF); // Should be skipped
    cpu.get_bus()->write(src + 2, 0x22);
    cpu.get_bus()->write(src + 3, 0xFF); // Should be skipped

    // Destination data (initial): [0xAA, 0xAA, 0xAA, 0xAA]
    cpu.get_bus()->write(dst + 0, 0xAA);
    cpu.get_bus()->write(dst + 1, 0xAA);
    cpu.get_bus()->write(dst + 2, 0xAA);
    cpu.get_bus()->write(dst + 3, 0xAA);

    // Instruction at 0x0000
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xB7);
    cpu.set_PC(0x0000);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared

    // Execute
    int steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }

    check(steps == 4, "LDPIRX masking steps");
    check(cpu.get_bus()->read(dst + 0) == 0x11, "LDPIRX byte 0 copied (0x11)");
    check(cpu.get_bus()->read(dst + 1) == 0xAA, "LDPIRX byte 1 skipped (mask match)");
    check(cpu.get_bus()->read(dst + 2) == 0x22, "LDPIRX byte 2 copied (0x22)");
    check(cpu.get_bus()->read(dst + 3) == 0xAA, "LDPIRX byte 3 skipped (mask match)");
}

void test_z80n_lddx_decrement() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDDX (ED AC)
    // Copies (HL) to (DE), decrements HL, DE, decrements BC.
    // Flags: P/V set if BC!=0, else reset. N=0, H=0.

    uint16_t src = 0x1002;
    uint16_t dst = 0x2002;
    
    cpu.set_HL(src);
    cpu.set_DE(dst);
    cpu.set_BC(3);
    
    // Data at source (backwards)
    cpu.get_bus()->write(0x1002, 0xAA);
    cpu.get_bus()->write(0x1001, 0xBB);
    cpu.get_bus()->write(0x1000, 0xCC);
    
    // Instruction at 0x0000
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xAC);
    cpu.set_PC(0x0000);
    
    // Step 1
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    cpu.step();
    check(cpu.get_bus()->read(0x2002) == 0xAA, "LDDX step 1 copy");
    check(cpu.get_HL() == 0x1001, "LDDX step 1 HL dec");
    check(cpu.get_DE() == 0x2001, "LDDX step 1 DE dec");
    check(cpu.get_BC() == 2, "LDDX step 1 BC dec");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDDX step 1 PV set (BC!=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDDX step 1 clears N");
    
    // Step 2
    cpu.set_PC(0x0000); // Reset PC to re-execute instruction
    cpu.step();
    check(cpu.get_bus()->read(0x2001) == 0xBB, "LDDX step 2 copy");
    check(cpu.get_HL() == 0x1000, "LDDX step 2 HL dec");
    check(cpu.get_DE() == 0x2000, "LDDX step 2 DE dec");
    check(cpu.get_BC() == 1, "LDDX step 2 BC dec");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDDX step 2 PV set (BC!=0)");

    // Step 3
    cpu.set_PC(0x0000);
    cpu.step();
    check(cpu.get_bus()->read(0x2000) == 0xCC, "LDDX step 3 copy");
    check(cpu.get_HL() == 0x0FFF, "LDDX step 3 HL dec");
    check(cpu.get_DE() == 0x1FFF, "LDDX step 3 DE dec");
    check(cpu.get_BC() == 0, "LDDX step 3 BC dec");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDDX step 3 PV clear (BC=0)");
}

void test_z80n_block_ext() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDDX (ED AC)
    cpu.set_HL(0x1001);
    cpu.set_DE(0x2001);
    cpu.set_BC(1);
    cpu.get_bus()->write(0x1001, 0x88);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xAC);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    cpu.step();
    check(cpu.get_bus()->read(0x2001) == 0x88, "LDDX copy");
    check(cpu.get_HL() == 0x1000, "LDDX HL dec");
    check(cpu.get_DE() == 0x2000, "LDDX DE dec");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDDX PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDDX clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDDX clears H");

    // LDIRSCALE (ED B6)
    cpu.set_BCp(0x0010);
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(1);
    cpu.get_bus()->write(0x1000, 0x99);
    cpu.set_PC(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xB6);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    cpu.step();
    check(cpu.get_bus()->read(0x2000) == 0x99, "LDIRSCALE copy");
    check(cpu.get_DE() == 0x2001, "LDIRSCALE DE inc");
    check(cpu.get_HL() == 0x1010, "LDIRSCALE HL += BC'");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDIRSCALE PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDIRSCALE clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDIRSCALE clears H");

    // LDDRX (ED BC)
    cpu.set_HL(0x1002);
    cpu.set_DE(0x2002);
    cpu.set_BC(3);
    cpu.get_bus()->write(0x1002, 0x11);
    cpu.get_bus()->write(0x1001, 0x22);
    cpu.get_bus()->write(0x1000, 0x33);
    cpu.set_PC(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xBC);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    
    int steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }
    check(steps == 3, "LDDRX steps");
    check(cpu.get_bus()->read(0x2002) == 0x11, "LDDRX byte 0");
    check(cpu.get_bus()->read(0x2001) == 0x22, "LDDRX byte 1");
    check(cpu.get_bus()->read(0x2000) == 0x33, "LDDRX byte 2");
    check(cpu.get_BC() == 0, "LDDRX BC=0");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDDRX PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDDRX clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDDRX clears H");

    // Test PV flag when BC != 0
    cpu.set_PC(0);
    cpu.set_BC(2);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N again
    cpu.step();
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDDRX PV flag (BC!=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDDRX step clears N");

    // LDPIRX (ED B7)
    cpu.set_HL(0x3000);
    cpu.set_DE(0x4000);
    cpu.set_BC(3);
    cpu.set_A(0x55);
    cpu.get_bus()->write(0x3000, 0x11);
    cpu.get_bus()->write(0x3001, 0x55);
    cpu.get_bus()->write(0x3002, 0x33);
    cpu.get_bus()->write(0x4000, 0);
    cpu.get_bus()->write(0x4001, 0);
    cpu.get_bus()->write(0x4002, 0);

    cpu.set_PC(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xB7);

    steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }
    check(steps == 3, "LDPIRX steps");
    check(cpu.get_bus()->read(0x4000) == 0x11, "LDPIRX byte 0 (copy)");
    check(cpu.get_bus()->read(0x4001) == 0x00, "LDPIRX byte 1 (skip)");
    check(cpu.get_bus()->read(0x4002) == 0x33, "LDPIRX byte 2 (copy)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) == 0, "LDPIRX PV flag (BC=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDPIRX clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) == 0, "LDPIRX clears H");

    // Test PV flag when BC != 0
    cpu.set_PC(0);
    cpu.set_BC(2);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N again
    cpu.step();
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "LDPIRX PV flag (BC!=0)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "LDPIRX step clears N");
}

void test_z80n_pixel_ops() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // PIXELAD (ED 93)
    // HL = Pixel Address for D(Y) and E(X)
    
    // Case 1: (0,0) -> 0x4000
    cpu.set_PC(0);
    cpu.set_D(0); cpu.set_E(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x93);
    cpu.step();
    check(cpu.get_HL() == 0x4000, "PIXELAD (0,0)");

    // Case 2: (255, 191) -> 0x57FF
    // D=191, E=255
    cpu.set_PC(0);
    cpu.set_D(191); cpu.set_E(255);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x93);
    cpu.step();
    check(cpu.get_HL() == 0x57FF, "PIXELAD (255,191)");

    // Case 3: (0, 8) -> 0x4020 (Line 8)
    // D=8, E=0
    cpu.set_PC(0);
    cpu.set_D(8); cpu.set_E(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x93);
    cpu.step();
    check(cpu.get_HL() == 0x4020, "PIXELAD (0,8)");

    // PIXELDN (ED 94)
    // Moves HL down one pixel line
    
    // Case 1: 0x4000 -> 0x4100
    cpu.set_PC(0);
    cpu.set_HL(0x4000);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x94);
    cpu.step();
    check(cpu.get_HL() == 0x4100, "PIXELDN 0x4000 -> 0x4100");

    // Case 2: 0x4700 (Line 7) -> 0x4020 (Line 8)
    cpu.set_PC(0);
    cpu.set_HL(0x4700);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x94);
    cpu.step();
    check(cpu.get_HL() == 0x4020, "PIXELDN 0x4700 -> 0x4020");

    // Case 3: 0x57FF (Line 191) -> 0x581F (Attribute area, preserving column)
    cpu.set_PC(0);
    cpu.set_HL(0x57FF);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x94);
    cpu.step();
    check(cpu.get_HL() == 0x581F, "PIXELDN 0x57FF -> 0x581F");
}

void test_z80n_setae_mask() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // SETAE (ED 95)
    // Generates pixel mask in A based on lower 3 bits of E.
    // A = 1 << (7 - (E & 7))

    uint8_t expected_masks[] = {
        0x80, // 0 -> 10000000
        0x40, // 1 -> 01000000
        0x20, // 2 -> 00100000
        0x10, // 3 -> 00010000
        0x08, // 4 -> 00001000
        0x04, // 5 -> 00000100
        0x02, // 6 -> 00000010
        0x01  // 7 -> 00000001
    };

    for (int i = 0; i < 8; ++i) {
        cpu.set_PC(0);
        cpu.set_E(i);
        cpu.get_bus()->write(0x0000, 0xED);
        cpu.get_bus()->write(0x0001, 0x95);
        cpu.step();
        
        std::stringstream ss;
        ss << "SETAE E=" << i << " -> A=0x" << std::hex << (int)expected_masks[i];
        check(cpu.get_A() == expected_masks[i], ss.str());
    }

    // Test with higher bits set in E (should be ignored)
    cpu.set_PC(0);
    cpu.set_E(0xF3); // 0xF3 & 7 = 3 -> 0x10
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x95);
    cpu.step();
    check(cpu.get_A() == 0x10, "SETAE E=0xF3 (mask 3) -> A=0x10");
}

void test_z80n_add_rr_a_flags() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // ADD DE, A (ED 32)
    // Should update C, Z, H, N=0
    cpu.set_DE(0xFFFF);
    cpu.set_A(1);
    cpu.set_F(Z80::CPU<TestBus>::Flags::N); // Set N to ensure it gets cleared
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x32);
    cpu.step();
    check(cpu.get_DE() == 0x0000, "ADD DE, A result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) != 0, "ADD DE, A sets Carry");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "ADD DE, A sets Zero");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "ADD DE, A clears N");

    // ADD BC, A (ED 33)
    cpu.set_PC(0);
    cpu.set_BC(0x0FFF);
    cpu.set_A(1);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x33);
    cpu.step();
    check(cpu.get_BC() == 0x1000, "ADD BC, A result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "ADD BC, A sets Half Carry");
}

void test_z80n_add_rr_nn_flags() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // ADD HL, nn (ED 34)
    // Should behave like ADD HL, BC (updates C, N=0, H; preserves Z, S, PV)
    cpu.set_HL(0xFFFF);
    cpu.set_F(Z80::CPU<TestBus>::Flags::Z | Z80::CPU<TestBus>::Flags::N); // Set Z and N
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x34);
    cpu.get_bus()->write(0x0002, 0x01); cpu.get_bus()->write(0x0003, 0x00); // nn = 1
    cpu.step();
    check(cpu.get_HL() == 0x0000, "ADD HL, nn result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) != 0, "ADD HL, nn sets Carry");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::N) == 0, "ADD HL, nn clears N");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "ADD HL, nn preserves Z");

    // ADD DE, nn (ED 35)
    cpu.set_PC(0);
    cpu.set_DE(0x0FFF);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x35);
    cpu.get_bus()->write(0x0002, 0x01); cpu.get_bus()->write(0x0003, 0x00); // nn = 1
    cpu.step();
    check(cpu.get_DE() == 0x1000, "ADD DE, nn result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::H) != 0, "ADD DE, nn sets Half Carry");

    // ADD BC, nn (ED 36)
    cpu.set_PC(0);
    cpu.set_BC(0x1000);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x36);
    cpu.get_bus()->write(0x0002, 0x01); cpu.get_bus()->write(0x0003, 0x00); // nn = 1
    cpu.step();
    check(cpu.get_BC() == 0x1001, "ADD BC, nn result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "ADD BC, nn clears Carry");
}

void test_z80n_shifts_edge_cases() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // BSRA DE, B (ED 29) - Shift by 16
    cpu.set_DE(0xFFFF); // -1
    cpu.set_B(16);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x29);
    cpu.step();
    // Arithmetic shift right by 16 on 0xFFFF should remain 0xFFFF (-1)
    check(cpu.get_DE() == 0xFFFF, "BSRA DE, B (shift 16 of -1)");

    // BSRL DE, B (ED 2A) - Shift by 16
    cpu.set_PC(0);
    cpu.set_DE(0xFFFF);
    cpu.set_B(16);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2A);
    cpu.step();
    check(cpu.get_DE() == 0x0000, "BSRL DE, B (shift 16)");

    // BSRF DE, B (ED 2B) - Shift by 16
    // Shift right and fill with 1s. 0x0000 >> 16 | 0xFFFF -> 0xFFFF
    cpu.set_PC(0);
    cpu.set_DE(0x0000);
    cpu.set_B(16);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2B);
    cpu.step();
    check(cpu.get_DE() == 0xFFFF, "BSRF DE, B (shift 16)");

    // BRLC DE, B (ED 2C) - Rotate by 16 (should be no change)
    cpu.set_PC(0);
    cpu.set_DE(0x1234);
    cpu.set_B(16);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2C);
    cpu.step();
    check(cpu.get_DE() == 0x1234, "BRLC DE, B (rotate 16)");

    // BRLC DE, B (ED 2C) - Rotate by 17 (should be rotate 1)
    cpu.set_PC(0);
    cpu.set_DE(0x8000);
    cpu.set_B(17);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x2C);
    cpu.step();
    check(cpu.get_DE() == 0x0001, "BRLC DE, B (rotate 17)");
}

void test_z80n_extended_cases() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // 1. MUL D, E - Zero and Carry
    cpu.set_D(0);
    cpu.set_E(50);
    cpu.set_F(Z80::CPU<TestBus>::Flags::C); // Set Carry
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x30);
    cpu.step();
    check(cpu.get_DE() == 0, "MUL D, E (0 * 50 = 0)");
    // Note: Z80N MUL usually clears C.
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) == 0, "MUL D, E clears Carry");

    // MUL D, E - Carry set (Result > 255)
    cpu.set_PC(0);
    cpu.set_D(2);
    cpu.set_E(200); // 400
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x30);
    cpu.step();
    check(cpu.get_DE() == 400, "MUL D, E (2 * 200 = 400)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) != 0, "MUL D, E sets Carry if > 255");

    // 2. ADD HL, A - Overflow
    cpu.set_PC(0);
    cpu.set_HL(0xFFFF);
    cpu.set_A(1);
    cpu.set_F(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x31);
    cpu.step();
    check(cpu.get_HL() == 0x0000, "ADD HL, A (0xFFFF + 1 = 0x0000)");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) != 0, "ADD HL, A sets Carry on overflow");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "ADD HL, A sets Zero");

    // 3. BSLA DE, B - Edge cases
    // Shift by 0
    cpu.set_PC(0);
    cpu.set_DE(0x1234);
    cpu.set_B(0);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x28);
    cpu.step();
    check(cpu.get_DE() == 0x1234, "BSLA DE, B (shift by 0)");

    // Shift by 16 (should be 0)
    cpu.set_PC(0);
    cpu.set_DE(0xFFFF);
    cpu.set_B(16);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x28);
    cpu.step();
    check(cpu.get_DE() == 0x0000, "BSLA DE, B (shift by 16)");

    // 4. LDIRSCALE - Step 1 (Normal copy)
    cpu.set_PC(0);
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(2);
    cpu.set_BCp(1); // Step 1
    cpu.get_bus()->write(0x1000, 0xAA);
    cpu.get_bus()->write(0x1001, 0xBB);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xB6);
    
    int steps = 0;
    while (cpu.get_PC() == 0x0000 && steps < 100) {
        cpu.step();
        steps++;
    }
    check(steps == 2, "LDIRSCALE step=1 steps");
    check(cpu.get_bus()->read(0x2000) == 0xAA, "LDIRSCALE step=1 byte 0");
    check(cpu.get_bus()->read(0x2001) == 0xBB, "LDIRSCALE step=1 byte 1");

    // 5. OUTINB - Check B preservation
    cpu.set_PC(0);
    cpu.set_BC(0x0510); // B=5, C=0x10
    cpu.set_HL(0x3000);
    cpu.get_bus()->write(0x3000, 0xFF);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x90);
    cpu.step();
    check(cpu.get_B() == 0x05, "OUTINB preserves B");
    check(cpu.get_HL() == 0x3001, "OUTINB increments HL");
}

void test_z80n_flags_preservation() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    // LDIX (ED A4)
    // Should preserve C, Z, S.
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(1);
    cpu.get_bus()->write(0x1000, 0x55);
    // Set flags to check preservation
    cpu.set_F(Z80::CPU<TestBus>::Flags::C | Z80::CPU<TestBus>::Flags::Z | Z80::CPU<TestBus>::Flags::S);
    
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0xA4);
    cpu.step();
    
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::C) != 0, "LDIX preserves C");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::Z) != 0, "LDIX preserves Z");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::S) != 0, "LDIX preserves S");

    // ADD HL, A (ED 31)
    // Updates C, Z, H, N. Should preserve S, PV.
    cpu.set_PC(0);
    cpu.set_HL(0x1000);
    cpu.set_A(0x01);
    cpu.set_F(Z80::CPU<TestBus>::Flags::S | Z80::CPU<TestBus>::Flags::PV);
    
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x31);
    cpu.step();
    
    check(cpu.get_HL() == 0x1001, "ADD HL, A result");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::S) != 0, "ADD HL, A preserves S");
    check((cpu.get_F() & Z80::CPU<TestBus>::Flags::PV) != 0, "ADD HL, A preserves PV");
}

void test_z80n_no_flag_changes() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();

    auto run_check = [&](const std::string& name, const std::vector<uint8_t>& opcodes) {
        cpu.set_PC(0x0000);
        cpu.set_F(0xAA); // Test pattern (S=1, Z=0, H=1, PV=0, N=1, C=0)
        cpu.set_BC(0x0101);
        cpu.set_DE(0x0101);
        cpu.set_HL(0x1000);
        cpu.set_SP(0x2000);
        cpu.set_A(0x11);
        
        for(size_t i=0; i<opcodes.size(); ++i) {
            cpu.get_bus()->write(i, opcodes[i]);
        }
        
        cpu.step();
        
        if (cpu.get_F() == 0xAA) {
            tests_passed++;
        } else {
            std::cout << "[FAIL] " << name << " changed flags: 0x" << std::hex << (int)cpu.get_F() << " expected 0xAA\n";
            tests_failed++;
        }
    };

    run_check("SWAPNIB", {0xED, 0x23});
    run_check("MIRROR", {0xED, 0x24});
    run_check("BSLA DE, B", {0xED, 0x28});
    run_check("BSRA DE, B", {0xED, 0x29});
    run_check("BSRL DE, B", {0xED, 0x2A});
    run_check("BSRF DE, B", {0xED, 0x2B});
    run_check("BRLC DE, B", {0xED, 0x2C});
    run_check("OUTINB", {0xED, 0x90});
    run_check("NEXTREG n, n", {0xED, 0x91, 0x10, 0x00});
    run_check("NEXTREG n, A", {0xED, 0x92, 0x10});
    run_check("PIXELAD", {0xED, 0x93});
    run_check("PIXELDN", {0xED, 0x94});
    run_check("SETAE", {0xED, 0x95});
    
    // JP (C) - ensure target is safe
    cpu.set_C(1);
    cpu.get_bus()->write(0x0040, 0x00); // NOP at target
    run_check("JP (C)", {0xED, 0x98});
    
    run_check("LDWS", {0xED, 0xA5});
    run_check("PUSH nn", {0xED, 0x8A, 0x12, 0x34});
}

void test_z80n_mul_flags() {
    Z80::CPU<TestBus> cpu;
    cpu.reset();
    
    // MUL D, E (ED 30)
    // Updates C, clears N. Preserves S, Z, H, P/V.
    cpu.set_D(2); cpu.set_E(10);
    cpu.set_F(Z80::CPU<TestBus>::Flags::S | Z80::CPU<TestBus>::Flags::Z | Z80::CPU<TestBus>::Flags::H | Z80::CPU<TestBus>::Flags::PV | Z80::CPU<TestBus>::Flags::N);
    cpu.get_bus()->write(0x0000, 0xED); cpu.get_bus()->write(0x0001, 0x30);
    cpu.step();
    
    uint8_t f = cpu.get_F();
    check((f & Z80::CPU<TestBus>::Flags::C) == 0, "MUL D, E clears C");
    check((f & Z80::CPU<TestBus>::Flags::N) == 0, "MUL D, E clears N");
    check((f & (Z80::CPU<TestBus>::Flags::S | Z80::CPU<TestBus>::Flags::Z | Z80::CPU<TestBus>::Flags::H | Z80::CPU<TestBus>::Flags::PV)) == (Z80::CPU<TestBus>::Flags::S | Z80::CPU<TestBus>::Flags::Z | Z80::CPU<TestBus>::Flags::H | Z80::CPU<TestBus>::Flags::PV), "MUL D, E preserves S, Z, H, PV");
}

void test_z80n_disabled() {
    std::cout << "Running Z80N Disabled Tests...\n";
    // Explicitly disable Z80N via template parameter
    Z80::CPU<TestBus, Z80::StandardEvents, Z80::StandardDebugger, false> cpu;
    cpu.reset();

    // 1. SWAPNIB (ED 23) - Should be NOP
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x23);
    cpu.set_A(0x12);
    cpu.step();
    check(cpu.get_A() == 0x12, "Disabled SWAPNIB: A unchanged");
    check(cpu.get_PC() == 0x0002, "Disabled SWAPNIB: PC advanced by 2");
    check(cpu.get_ticks() == 8, "Disabled SWAPNIB: 8 T-states");

    // 2. MUL D, E (ED 30) - Should be NOP
    cpu.set_PC(0);
    cpu.set_ticks(0);
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x30);
    cpu.set_DE(0x0203);
    cpu.step();
    check(cpu.get_DE() == 0x0203, "Disabled MUL: DE unchanged");
    check(cpu.get_PC() == 0x0002, "Disabled MUL: PC advanced by 2");

    // 3. NEXTREG (ED 91 n n) - Should be 2-byte NOP, not consuming operands
    cpu.set_PC(0);
    cpu.set_ticks(0);
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0x91);
    cpu.get_bus()->write(0x0002, 0x00); // NOP
    cpu.get_bus()->write(0x0003, 0x00); // NOP
    cpu.step();
    check(cpu.get_PC() == 0x0002, "Disabled NEXTREG: PC advanced by 2 (operands not consumed)");
    
    // 4. LDIX (ED A4) - Should be NOP
    cpu.set_PC(0);
    cpu.set_ticks(0);
    cpu.get_bus()->write(0x0000, 0xED);
    cpu.get_bus()->write(0x0001, 0xA4);
    cpu.set_HL(0x1000);
    cpu.set_DE(0x2000);
    cpu.set_BC(1);
    cpu.get_bus()->write(0x1000, 0x55);
    cpu.get_bus()->write(0x2000, 0x00);
    cpu.step();
    check(cpu.get_bus()->read(0x2000) == 0x00, "Disabled LDIX: No memory write");
    check(cpu.get_HL() == 0x1000, "Disabled LDIX: HL unchanged");
    check(cpu.get_DE() == 0x2000, "Disabled LDIX: DE unchanged");
    check(cpu.get_BC() == 1, "Disabled LDIX: BC unchanged");
}

int main() {
    std::cout << "Running Z80 Execution Tests (Z80Next_test.cpp)...\n";
    
    test_z80n_swapnib();
    test_z80n_mirror();
    test_z80n_mul();
    test_z80n_add_hl_a();
    test_z80n_bsla();
    test_z80n_nextreg();
    test_z80n_nextreg_readback();
    test_z80n_ldix();
    test_z80n_ldws();
    test_z80n_ldirx();
    test_z80n_shifts();
    test_z80n_alu_misc();
    test_z80n_test_flags();
    test_z80n_stack_jump();
    test_z80n_io_misc();
    test_z80n_ldirscale_scaling();
    test_z80n_ldirscale_z_flag();
    test_z80n_ldpirx_masking();
    test_z80n_lddx_decrement();
    test_z80n_block_ext();
    test_z80n_extended_cases();
    test_z80n_pixel_ops();
    test_z80n_setae_mask();
    test_z80n_add_rr_a_flags();
    test_z80n_add_rr_nn_flags();
    test_z80n_shifts_edge_cases();
    test_z80n_flags_preservation();
    test_z80n_no_flag_changes();
    test_z80n_mul_flags();
    test_z80n_disabled();

    std::cout << "Tests passed: " << tests_passed << "\n";
    std::cout << "Tests failed: " << tests_failed << "\n";
    
    return tests_failed > 0 ? 1 : 0;
}