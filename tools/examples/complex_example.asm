; ==============================================================================
; Complex Z80 Assembly Example
; Demonstrating advanced features of the Z80Assembler
; ==============================================================================

    ; --------------------------------------------------------------------------
    ; 1. Configuration and Constants
    ; --------------------------------------------------------------------------
    OPTION +Z80N            ; Enable ZX Spectrum Next instructions
    OPTION +UNDOC           ; Enable undocumented instructions
    OPTIMIZE SPEED          ; Enable speed optimizations

    ; Define constants
    SCREEN_WIDTH    EQU 256
    SCREEN_HEIGHT   EQU 192
    ATTR_START      EQU 0x5800
    BIG_VAL         EQU 0x1122334455667788
    DEFINE DEBUG_ENABLED 1  ; Define for conditional assembly

    ; --------------------------------------------------------------------------
    ; 2. Macros
    ; --------------------------------------------------------------------------
    
    ; Macro to wait for a number of NOPs
    WAIT_NOPS MACRO count
        REPT count
            NOP
        ENDR
    ENDM

    ; Macro to push multiple registers
    PUSH_ALL MACRO
        PUSH AF
        PUSH BC
        PUSH DE
        PUSH HL
        PUSH IX
        PUSH IY
    ENDM

    ; Macro to pop multiple registers (reverse order)
    POP_ALL MACRO
        POP IY
        POP IX
        POP HL
        POP DE
        POP BC
        POP AF
    ENDM

    ; Variadic macro to sum bytes into A
    SUM_BYTES MACRO
        XOR A               ; Clear A
        REPT \0             ; Repeat for number of arguments
            ADD A, \1       ; Add current argument
            SHIFT           ; Shift arguments
        ENDR
    ENDM

    ; --------------------------------------------------------------------------
    ; 3. Main Program Entry
    ; --------------------------------------------------------------------------
    ORG 0x8000              ; Program start address

Start:
    DI
    LD SP, StackTop

    ; Display build info
    IF $PHASE == 2
        DISPLAY "Assembling at address: ", $
        DISPLAY "Pass number: ", $PASS
    ENDIF

    ; --------------------------------------------------------------------------
    ; 4. Conditional Assembly
    ; --------------------------------------------------------------------------
    IFDEF DEBUG_ENABLED
        LD A, 2             ; Red border
        OUT (0xFE), A
    ELSE
        LD A, 0             ; Black border
        OUT (0xFE), A
    ENDIF

    ; --------------------------------------------------------------------------
    ; 5. Z80N and Undocumented Instructions
    ; --------------------------------------------------------------------------
    ; Z80N specific
    NEXTREG 0x07, 0x03      ; Set Turbo mode
    SWAPNIB                 ; Swap nibbles of A
    MIRROR                  ; Mirror bits of A
    BSLA DE, B              ; Barrel shift left DE by B
    LDIRSCALE               ; LDIR with scaling
    PUSH 0x1234             ; Push immediate 16-bit value (Z80N)

    ; Undocumented Z80
    LD A, IXH               ; Load high byte of IX
    ADD A, IYL              ; Add low byte of IY
    SLL B                   ; Shift Left Logical (undocumented)
    SLI A                   ; Alias for SLL

    ; --------------------------------------------------------------------------
    ; 6. Advanced Expressions and Functions
    ; --------------------------------------------------------------------------
    ; Calculate sine table value at compile time
    LD A, SIN(MATH_PI / 2) * 127 + 128
    
    ; Check if a value is a string
    IF ISSTRING("Test")
        NOP
    ENDIF

    ; --------------------------------------------------------------------------
    ; 7. Procedures and Local Labels
    ; --------------------------------------------------------------------------
ProcessData PROC
    LOCAL loop              ; Local label definition
    
    LD HL, DataBlock
    LD BC, 16
    
loop:
    LD A, (HL)
    XOR 0xFF                ; Invert bits
    LD (HL), A
    INC HL
    DEC BC
    LD A, B
    OR C
    JR NZ, loop
    
    RET
ProcessData ENDP

    ; --------------------------------------------------------------------------
    ; 8. Phase / Dephase (Relocatable Code)
    ; --------------------------------------------------------------------------
    ; This code is stored here but designed to run at 0xC000
    PHASE 0xC000
RelocatedRoutine:
    LD A, 0xAA
    LD (0x4000), A
    RET
RelocatedEnd:
    DEPHASE

    ; Copy the relocated routine
    LD HL, RelocatedRoutine ; Source (physical address)
    LD DE, 0xC000           ; Destination (logical address)
    LD BC, RelocatedEnd - RelocatedRoutine
    LDIR

    ; --------------------------------------------------------------------------
    ; 9. Data Definitions
    ; --------------------------------------------------------------------------
    ALIGN 256               ; Align to page boundary

DataBlock:
    DB 1, 2, 3, 4           ; Bytes
    DW 0x1234, 0x5678       ; Words
    D24 0x123456            ; 24-bit value
    DWORD 0xDEADBEEF        ; 32-bit value
    DQ 0x1122334455667788   ; 64-bit value
    DQ BIG_VAL


    ; String formats
    DZ "Null Terminated"    ; ASCIZ
    DC "Last Char Set"      ; Last character has bit 7 set

    ; Hex and Binary helpers
    DH "01020304"           ; Hex string
    DG "11110000"           ; Binary grid (0xF0)
    DG "....XXXX"           ; Binary grid (0x0F)

    ; --------------------------------------------------------------------------
    ; 10. Advanced Directives (WHILE, DEFL, UNDEFINE)
    ; --------------------------------------------------------------------------
    
    ; Generate a sequence using WHILE
    COUNTER SET 0
    WHILE COUNTER < 4
        DB 0xAA + COUNTER
        COUNTER SET COUNTER + 1
    ENDW

    ; Redefinable label using DEFL (same as SET)
    VAR_DEFL DEFL 10
    VAR_DEFL DEFL 20
    DB VAR_DEFL

    ; Undefine a symbol (DEFINE based)
    DEFINE TEMP_DEF 1
    UNDEFINE TEMP_DEF
    IFNDEF TEMP_DEF
        DB 0x00 ; Should be assembled
    ENDIF

    ; --------------------------------------------------------------------------
    ; 11. String Manipulation and Display
    ; --------------------------------------------------------------------------
    
    DEFINE MY_STR "Hello World"
    
    ; Display message during assembly
    IF $PHASE == 2
        DISPLAY "Current Address: ", $
        DISPLAY "String Length: ", STRLEN(MY_STR)
    ENDIF
    
    ; Store substring
    DZ SUBSTR(MY_STR, 0, 5) ; "Hello"
    
    ; Replace in string
    DZ REPLACE("1-2-3", "-", ":") ; "1:2:3"

    ; --------------------------------------------------------------------------
    ; 12. Advanced Macros
    ; --------------------------------------------------------------------------
    
    ; Macro checking arguments
    SAFE_LD MACRO reg, val
        IFIDN <{reg}>, <A>
            LD A, {val}
        ELSE
            IFNB {reg}
                LD {reg}, {val}
            ELSE
                NOP ; No register provided
            ENDIF
        ENDIF
    ENDM
    
SAFE_LD A, 10
    SAFE_LD B, 20
    SAFE_LD , 30 ; Empty reg

    ; --------------------------------------------------------------------------
    ; 13. Struct Simulation
    ; --------------------------------------------------------------------------
    
    ; Define offsets
    ST_X    EQU 0
    ST_Y    EQU 1
    ST_HP   EQU 2
    ST_SIZE EQU 3
    
    ; Access struct fields
    LD IX, PlayerObj
    LD A, (IX + ST_HP)
    
    ; Define instance
PlayerObj:
    DB 10, 20, 100

    ; --------------------------------------------------------------------------
    ; 14. Assertions
    ; --------------------------------------------------------------------------
    ASSERT $ < 0xFFFF       ; Ensure we fit in memory

    ; --------------------------------------------------------------------------
    ; 15. Stack
    ; --------------------------------------------------------------------------
    DS 256, 0xAA            ; Stack space filled with 0xAA
StackTop:

    ; --------------------------------------------------------------------------
    ; 16. File Inclusion and Binary Data
    ; --------------------------------------------------------------------------
    INCLUDE "other_file.asm"  ; Include another source file
    INCBIN "sprite.bin"       ; Include raw binary data
    
    ; --------------------------------------------------------------------------
    ; 17. Error Handling
    ; --------------------------------------------------------------------------
    ; IF SCREEN_WIDTH > 256
    ;     ERROR "Screen width too large"
    ; ENDIF

    END