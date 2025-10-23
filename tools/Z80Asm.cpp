//  ▄▄▄▄▄▄▄▄    ▄▄▄▄      ▄▄▄▄
//  ▀▀▀▀▀███  ▄██▀▀██▄   ██▀▀██
//      ██▀   ██▄  ▄██  ██    ██
//    ▄██▀     ██████   ██ ██ ██
//   ▄██      ██▀  ▀██  ██    ██
//  ███▄▄▄▄▄  ▀██▄▄██▀   ██▄▄██
//  ▀▀▀▀▀▀▀▀    ▀▀▀▀      ▀▀▀▀   Asm.cpp
// Verson: 1.0.4
//
// This file contains a command-line utility for assembling Z80 code.
// It serves as an example of how to use the Z80Assembler class.
//
// Copyright (c) 2025 Adam Szulc
// MIT License

#include "Z80Assemble.h"
#include <iomanip>
#include <iostream>

void print_bytes(const std::vector<uint8_t>& bytes) {
    for (uint8_t byte : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::endl;
}

int main() {
    // The assembler needs a bus to interact with memory, even if it's just for calculating sizes.
    Z80DefaultBus bus;
    Z80Assembler<Z80DefaultBus> assembler(&bus);

    // Ten kod źródłowy demonstruje szeroki zakres możliwości asemblera,
    // w tym dyrektywy, etykiety, wyrażenia, różne tryby adresowania
    // i instrukcje, w tym te z prefiksami CB, ED, DD, FD.
    std::string source_code = R"(
; ============================================================================
; Kompleksowy przykład dla Z80Asm
; Demonstruje etykiety, dyrektywy, wyrażenia, instrukcje z prefiksami
; i różne tryby adresowania.
; ============================================================================

        ORG 0x8000          ; Ustaw adres początkowy programu

; --- Stałe zdefiniowane za pomocą EQU ---
MAX_RETRIES     EQU 5
VIDEO_RAM      EQU 0x4000
IO_PORT        EQU 0x38

; --- Główny program ---
START:
        DI                  ; Wyłącz przerwania
        LD SP, STACK_TOP    ; Ustaw wskaźnik stosu

        ; Ładowanie rejestrów wartościami natychmiastowymi i stałymi
        LD A, MAX_RETRIES
        LD BC, 0x1234
        LD HL, MESSAGE      ; Załaduj adres etykiety (odwołanie w przód)

        CALL CLEAR_SCREEN   ; Wywołaj podprogram

; --- Główna pętla ---
MAIN_LOOP:
        LD A, (COUNTER)     ; Załaduj wartość ze zmiennej
        CP MAX_RETRIES      ; Porównaj z maksymalną wartością
        JP Z, FINISH        ; Jeśli równe, skocz na koniec

        INC A
        LD (COUNTER), A     ; Zapisz nową wartość

        ; Przykłady instrukcji z prefiksami ED (I/O)
        IN A, (IO_PORT)     ; Odczyt z portu I/O
        OUT (IO_PORT), A    ; Zapis do portu I/O

        ; Przykłady instrukcji z prefiksami CB (operacje na bitach)
        SET 7, B            ; Ustaw bit 7 w rejestrze B
        RES 0, C            ; Zresetuj bit 0 w rejestrze C
        BIT 4, D            ; Przetestuj bit 4 w rejestrze D

        JP MAIN_LOOP        ; Powtórz pętlę

; --- Koniec programu ---
FINISH:
        LD HL, MSG_FINISH   ; Załaduj adres końcowej wiadomości
        HALT                ; Zatrzymaj procesor
        JR $                 ; Nieskończona pętla (skok do samego siebie)

; --- Podprogramy ---
CLEAR_SCREEN:
        LD HL, VIDEO_RAM    ; Adres początku pamięci wideo
        LD DE, VIDEO_RAM + 1
        LD BC, 767          ; 32*24 - 1
        LD (HL), ' '        ; Wyczyść pierwszy bajt
        LDIR                ; Skopiuj spację na cały ekran
        RET                 ; Powrót z podprogramu

; ============================================================================
; Sekcja danych i zmiennych
; ============================================================================
MESSAGE:
        DB "Program started!", 0x0D, 0x0A, 0

MSG_FINISH:
        DB "Loop finished. End.", 0

; --- Zmienne w RAM ---
COUNTER:
        DB 0                ; 8-bitowy licznik

; --- Przykłady użycia DW i wyrażeń ---
POINTER_TO_START:
        DW START            ; 16-bitowy wskaźnik do etykiety START

LABEL_A:    DS 4, 0xAA      ; Zarezerwuj 4 bajty wypełnione 0xAA
LABEL_B:    DS 4, 0xBB      ; Zarezerwuj 4 bajty wypełnione 0xBB

; Obliczanie różnicy adresów między etykietami
DISTANCE:
        DW LABEL_B - LABEL_A ; Powinno dać w wyniku 4

; --- Przykłady instrukcji z prefiksami DD/FD (rejestry IX/IY) ---
INDEXED_OPS:
        LD IX, TABLE_DATA   ; Załaduj adres tabeli do IX
        LD IY, VIDEO_RAM    ; Załaduj adres VRAM do IY

        ; Dostęp do pamięci z przesunięciem
        LD A, (IX+3)        ; Załaduj A z TABLE_DATA+3 (wartość 0xDD)
        LD (IY+10), A       ; Zapisz A w VIDEO_RAM+10

        ; Operacje arytmetyczne z użyciem rejestrów indeksowych
        ADD A, (IX+1)       ; A = A + TABLE_DATA[1]
        SUB (IX+2)          ; A = A - TABLE_DATA[2]

        ; Użycie części rejestrów IX/IY
        LD IXH, 0x80
        LD IXL, 0x00
        LD A, IXH
        ADD A, IXL

        ; Instrukcje z prefiksami DDCB/FDCB
        SET 1, (IX+0)       ; Ustaw bit 1 w TABLE_DATA[0]
        RES 2, (IY+0)       ; Zresetuj bit 2 w VIDEO_RAM[0]
        BIT 7, (IX+3)       ; Przetestuj bit 7 w TABLE_DATA[3]
        RET

TABLE_DATA:
        DB 0xAA, 0xBB, 0xCC, 0xDD, 0xEE

; --- Definicja stosu ---
        DS 255, 0           ; Zarezerwuj 255 bajtów na stos
STACK_TOP:                  ; Etykieta wskazująca na szczyt stosu
    )";
    try {
        std::cout << "Assembling source code:" << std::endl;
        std::cout << source_code << std::endl;
        if (assembler.assemble(source_code)) {
            std::cout << "Machine code -> ";
            // Read back from the bus to display the assembled code
            std::vector<uint8_t> machine_code;
            // The start address is defined by ORG. The end address is at STACK_TOP.
            // A proper implementation would expose the symbol table from the assembler
            // to get this value. For this simple example, we can calculate it.
            uint16_t start_addr = 0x8000; 
            uint16_t end_addr = 0x8000 + 115 + 255; // 115 bytes of code/data before the stack
            for (uint16_t addr = start_addr; addr < end_addr; ++addr) {
                machine_code.push_back(bus.peek(addr));
            }
            print_bytes(machine_code);
            std::cout << "Assembly successful. Code written to bus memory." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Assembly error: " << e.what() << std::endl;
    }

    return 0;
}
