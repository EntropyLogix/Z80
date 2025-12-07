# Z80 Assembler Documentation (`Z80Assemble.h`)

This document provides a detailed description of the features and syntax of the `Z80Assembler`, provided within the `Z80Assemble.h` header file. This assembler is capable of compiling Z80 source code into machine code, supporting standard mnemonics, advanced expressions, macros, and a rich set of directives.

## Syntax

Each line of code can contain a label, an instruction (mnemonic with operands), and a comment.

```z80
LABEL: MNEMONIC OPERAND1, OPERAND2 ; This is a comment
```

### Labels

Labels are used to mark memory addresses, making them easier to reference.

*   **Global Labels**: Start with a letter or `_`. They can optionally end with a colon (`:`). Their scope is global.
*   **Local Labels**: Start with a dot (`.`). Their scope is limited to the last defined global label or the current procedure (`PROC`).
*   A label must start with a letter, underscore (`_`), dot (`.`), at sign (`@`), or question mark (`?`).
*   Subsequent characters can also include digits.
*   Labels cannot be the same as keywords (mnemonics, directives, register names).

**Example:**
```z80
GlobalLabel:
    NOP
    JR .local_jump ; Jumps to the address GlobalLabel.local_jump

.local_jump:
    HALT

AnotherGlobal:
.local_jump: ; A different local label with the same name
    RET
```

### Instructions

An instruction consists of a mnemonic (e.g., `LD`, `ADD`) and zero or more operands (e.g., `A`, `5`). Operands are separated by commas.

### Comments

The assembler supports three types of comments:

*   **Single-line**: Starts with a semicolon (`;`).
*   **Single-line (C++ style)**: Starts with `//`.
*   **Block (C style)**: Starts with `/*` and ends with `*/`.

**Example:**
```z80
LD A, 5     ; This is a comment to the end of the line.
// This is also a comment.
/*
  This is a
  multi-line
  block comment.
*/
```

## Registers

The assembler supports all standard Z80 registers and their parts.

| Type | Registers |
|:---|:---|
| 8-bit | `A`, `B`, `C`, `D`, `E`, `H`, `L`, `I`, `R` |
| 16-bit | `AF`, `BC`, `DE`, `HL`, `SP`, `IX`, `IY` |
| Index Register Parts | `IXH`, `IXL`, `IYH`, `IYL` |
| Register Pairs (for `PUSH`/`POP`) | `AF`, `BC`, `DE`, `HL`, `IX`, `IY` |
| Special | `AF'` (alternate register) |

## Expressions

The assembler features an advanced expression evaluator that calculates values at compile time. Expressions can be used wherever a numeric value is expected.

### Operators

Arithmetic, bitwise, and logical operators are supported, following standard operator precedence. Both symbols and keywords can be used.

| Category | Operators (Symbol) | Operators (Keyword) | Description |
|:---|:---|:---|:---|
| Arithmetic | `+`, `-`, `*`, `/`, `%` | `MOD` | Addition, subtraction, etc. |
| Bitwise | `&`, `\|`, `^`, `~`, `<<`, `>>` | `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR` | Bitwise operations. |
| Logical | `!`, `&&`, `\|\|` | | Logical NOT, AND, OR. |
| Comparison | `==`, `!=`, `>`, `<`, `>=`, `<=` | `EQ`, `NE`, `GT`, `LT`, `GE`, `LE` | Comparison operators. |
| Unary | `+`, `-` (sign), `DEFINED` | | Sign operators and symbol definition check. |
| Conditional | `? :` | | Ternary operator (e.g., `condition ? val1 : val2`). |

### Functions

The assembler supports a wide range of built-in functions for compile-time calculations.

**String and Type Conversion:**

| Function | Description |
|:---|:---|
| `ISSTRING(val)` | Returns `TRUE` if the argument is a string. |
| `ISNUMBER(val)` | Returns `TRUE` if the argument is a number or a string that can be converted to a number. |
| `STR(num)` | Converts a number to its string representation. |
| `VAL(str)` | Converts a string representation of a number to a numeric value. |
| `CHR(num)` | Returns a single-character string from an ASCII code. |
| `ASC(str)` | Returns the ASCII code of the first character of a string. |
| `CHARS(str)` | Converts a string of up to 4 characters into a little-endian integer value. |
| `STRLEN(str)` | Returns the length of a string. |
| `SUBSTR(str, pos, len)` | Extracts a substring of a given length, starting from a specified position (0-indexed). |
| `STRIN(str, sub)` | Finds the starting position (1-indexed) of a substring within a string. Returns 0 if not found. |
| `REPLACE(str, old, new)` | Replaces all occurrences of a substring with a new string. |
| `LCASE(str)` | Converts a string to lowercase. |
| `UCASE(str)` | Converts a string to uppercase. |

**Bit, Byte, and Memory Operations:**

| Function | Description |
|:---|:---|
| `{addr}` | Reads a byte from memory at the specified address during compilation. |
| `HIGH(val)` | Returns the high byte of a 16-bit value. |
| `LOW(val)` | Returns the low byte of a 16-bit value. |
| `MEM(addr)` | Reads a byte from memory at the specified address during the final assembly pass. |

**Mathematical Functions:**

| Function | Description |
|:---|:---|
| `MIN(n1, n2,...)`, `MAX(n1, n2,...)` | Returns the minimum/maximum value from a list of numbers. |
| `ABS(x)` | Returns the absolute value of a number. |
| `SGN(n)` | Returns the sign of a number (-1 for negative, 0 for zero, 1 for positive). |
| `POW(base, exp)` | Calculates `base` to the power of `exp`. |
| `SQRT(x)` | Calculates the square root of a number. |
| `HYPOT(x, y)` | Calculates the hypotenuse of a right-angled triangle (`sqrt(x^2 + y^2)`). |
| `FMOD(x, y)` | Returns the floating-point remainder of `x/y`. |
| `LOG(x)`, `LOG10(x)`, `LOG2(x)` | Calculates the natural, base-10, and base-2 logarithm. |
| `ROUND(n)`, `FLOOR(n)`, `CEIL(n)` | Rounds a number to the nearest integer, down, or up. |
| `TRUNC(n)` | Truncates the fractional part of a number (rounds towards zero). |
| `SIN(n)`, `COS(n)`, `TAN(n)` | Trigonometric functions (angle in radians). |
| `ASIN(n)`, `ACOS(n)`, `ATAN(n)` | Inverse trigonometric functions. |
| `ATAN2(y, x)` | Arc tangent of `y/x`, using the signs of the arguments to determine the quadrant. |
| `SINH(n)`, `COSH(n)`, `TANH(n)` | Hyperbolic functions. |
| `ASINH(n)`, `ACOSH(n)`, `ATANH(n)` | Inverse hyperbolic functions. |
| `RAND(min, max)` | Returns a pseudo-random integer in the range `[min, max]`. |
| `RRND(min, max)` | Returns a pseudo-random integer from a generator with seed 0. |
| `RND()` | Returns a pseudo-random float in the range `[0.0, 1.0]` from a generator with seed 1. |

### Special Variables

| Variable | Description |
|:---|:---|
| `$`, `@` | The current logical address. |
| `$PASS` | The number of the current assembly pass (starting from 1). |
| `$$` | The current physical address (useful in `PHASE`/`DEPHASE` blocks). |

### Constants

| Constant | Description |
|:---|:---|
| `TRUE` | The value 1. |
| `FALSE` | The value 0. |
| `MATH_PI` | The constant Pi (≈3.14159). |
| `MATH_E` | Euler's number (≈2.71828). |
| `MATH_PI_2` | Pi / 2. |
| `MATH_PI_4` | Pi / 4. |
| `MATH_LN2` | Natural logarithm of 2. |
| `MATH_LN10` | Natural logarithm of 10. |
| `MATH_LOG2E` | Base-2 logarithm of E. |
| `MATH_LOG10E` | Base-10 logarithm of E. |
| `MATH_SQRT2` | Square root of 2. |
| `MATH_SQRT1_2` | Square root of 1/2. |

### Expression Examples

```z80
; --- Simple Arithmetic ---
InitialValue EQU 100
LD A, InitialValue + 5 * 2 ; A = 110 (operator precedence)
LD B, (InitialValue + 5) * 2 ; B = 210 (parentheses)

; --- Using Labels to Calculate Size ---
DataTable:
    DB "This is a string", 0
TableEnd:
DataTableSize EQU TableEnd - DataTable

LD A, DataTableSize ; Load the size of the table into A

; --- Bitwise and Logical Operations ---
Bitmask EQU %11110000
LD A, Bitmask AND $F0
LD B, (1 << 7) | (1 << 1) ; B = %10000010 (130)

; --- Conditional Expressions and Functions ---
DEBUG_MODE EQU 1
TimeoutValue EQU (DEBUG_MODE == 1) ? 100 : 2000
LD BC, TimeoutValue

SineTable: DB ROUND(SIN(MATH_PI / 4) * 255) ; Using trig functions and rounding
```

## Assembler Directives

Directives are commands for the assembler that control the compilation process.

### Data Definition

| Directive | Aliases | Syntax | Example |
|:---|:---|:---|:---|
| `DB` | `DEFB`, `BYTE`, `DM` | `DB <expression>, <string>, ...` | `DB 10, 0xFF, "Hello", 'A'` |
| `DW` | `DEFW`, `WORD` | `DW <expression>, <label>, ...` | `DW 0x1234, MyLabel` |
| `DS` | `DEFS`, `BLOCK` | `DS <count> [, <fill_byte>]` | `DS 10, 0xFF` |
| `DZ` | `ASCIZ` | `DZ <string>, <expression>, ...` | `DZ "Game Over"` |
| `DH` | `HEX`, `DEFH` | `DH <hex_string>, ...` | `DH "DEADBEEF"` |
| `DG` | `DEFG` | `DG <bit_string>, ...` | `DG "11110000", "XXXX...."` |

### Symbol Definition

| Directive | Syntax | Description | Example |
|:---|:---|:---|:---|
| `EQU` | `<label> EQU <expression>` | Assigns a constant value. Redefinition causes an error. | `PORTA EQU 0x80` |
| `SET` | `<label> SET <expression>` | Assigns a numeric value. The symbol can be redefined. | `Counter SET 0` |
| `DEFINE` | `DEFINE <label> <expression>` | Alias for `SET`. | `DEBUG DEFINE 1` |
| `=` | `<label> = <expression>` | By default, acts like `EQU`. Can be configured to act like `SET`. | `PORTA = 0x80` |

### Address and Structure Control

| Directive | Syntax | Description |
|:---|:---|:---|
| `ORG` | `ORG <address>` | Sets the starting address for the subsequent code. |
| `ALIGN` | `ALIGN <boundary>` | Aligns the current address to the specified boundary, filling gaps with zeros. |
| `PHASE` | `PHASE <address>` | Sets the logical address without changing the physical address. |
| `DEPHASE` | `DEPHASE` | Ends a `PHASE` block, synchronizing the logical address with the physical one. |
| `PROC` | `<name> PROC` | Starts a procedure, creating a new namespace for local labels. |
| `ENDP` | `ENDP` | Ends a procedure block. |
| `LOCAL` | `LOCAL <sym1>, ...` | Declares symbols as local within a macro or procedure. |

**`PROC` Example:**
```z80
; A procedure to clear a 6KB screen memory area
ClearScreen PROC
    LD   HL, 0x4000      ; Start of video memory
    LD   (HL), 0         ; Clear the first byte
    LD   DE, 0x4001      ; Destination is the next byte
    LD   BC, 6143        ; Number of remaining bytes (6*1024 - 1)
    LDIR                 ; Copy the zeroed byte across the screen
    RET

.some_local: ; This label is only visible as ClearScreen.some_local
    NOP
ENDP

Main:
    CALL ClearScreen
    ; JR .some_local ; This would cause an error, label is not in scope
```

### Conditional Compilation

| Directive | Syntax | Description |
|:---|:---|:---|
| `IF` | `IF <expression>` | Starts a conditional block if the expression is non-zero. |
| `ELSE` | `ELSE` | Executes code if the `IF` condition was false. |
| `ENDIF` | `ENDIF` | Ends a conditional block. |
| `IFDEF` | `IFDEF <symbol>` | Executes code if the symbol is defined. |
| `IFNDEF` | `IFNDEF <symbol>` | Executes code if the symbol is not defined. |
| `IFNB` | `IFNB <argument>` | Executes code if the macro argument is not blank. |
| `IFIDN` | `IFIDN <arg1>, <arg2>` | Executes code if two text arguments are identical. |

### Macros

Macros allow you to define reusable code templates.

*   `MACRO`/`ENDM`: Defines a macro.
*   `SHIFT`: Shifts positional parameters (`\2` becomes `\1`, etc.).
*   `EXITM`: Exits the current macro expansion.
*   Parameters: `{name}` (named), `\1` (positional), `\0` (number of arguments).

**Example 1: Variable Arguments**
```z80
; Macro that defines a series of bytes based on its arguments
WRITE_BYTES MACRO
    REPT \0      ; Repeat for the number of arguments
        DB \1    ; Define the CURRENT first argument
        SHIFT    ; Shift the argument queue: \2 becomes \1, etc.
    ENDR
ENDM

; Usage:
WRITE_BYTES 10, 20, 30 ; Generates: DB 10, DB 20, DB 30
```

**Example 2: Named Arguments and Local Labels**
```z80
; Macro to add a 16-bit value to HL
ADD_HL MACRO {value}
    LOCAL .add_value
    LD   DE, .add_value
    ADD  HL, DE
    EXITM ; Exit macro

.add_value:
    DW {value}
ENDM

; Usage:
ADD_HL 1000 ; Generates LD DE, <addr>; ADD HL, DE; DW 1000
```

### Repetitions (Loops)

| Directive | Aliases | Syntax | Description |
|:---|:---|:---|:---|
| `REPT` | `DUP` | `REPT <count>` | Repeats a block of code a specified number of times. |
| `ENDR` | `EDUP` | `ENDR` | Ends a `REPT` block. |
| `WHILE` | | `WHILE <expression>` | Repeats a block of code as long as the expression is true. |
| `ENDW` | | `ENDW` | Ends a `WHILE` block. |
| `EXITR` | | `EXITR` | Exits the current `REPT` loop. |

Inside a `REPT` loop, the special symbol `\@` represents the current iteration (starting from 1).

**`REPT` Example:**
```z80
; Generate a table of squares
SquaresTable:
REPT 8
    DB \@ * \@ ; Generates: DB 1, DB 4, DB 9, ..., DB 64
ENDR
```

**`WHILE` Example:**
```z80
; Generate a powers-of-two table
PowerOfTwo SET 1
WHILE PowerOfTwo <= 128
    DB PowerOfTwo
    PowerOfTwo SET PowerOfTwo * 2
ENDW
; Generates: DB 1, DB 2, DB 4, DB 8, DB 16, DB 32, DB 64, DB 128
```

### File Inclusion

| Directive | Aliases | Syntax | Description |
|:---|:---|:---|:---|
| `INCLUDE` | | `INCLUDE "<filename>"` | Includes the content of another source file. |
| `INCBIN` | `BINARY` | `INCBIN "<filename>"` | Includes a binary file into the output code. |

### Other Directives

| Directive | Syntax | Description |
|:---|:---|:---|
| `DISPLAY` | `DISPLAY <message>, <expression>...` | Displays a message or value on the console during compilation (alias: `ECHO`). |
| `ERROR` | `ERROR "<message>"` | Stops compilation and displays an error message. |
| `ASSERT` | `ASSERT <expression>` | Stops compilation if the expression evaluates to false (zero). |
| `END` | `END` | Ends the assembly process. |

## Supported Instructions (Mnemonics)

The assembler supports the full standard and most of the undocumented Z80 instruction sets.

### 8-bit Load
*   **Mnemonics**: `LD`
*   **Description**: Loads data into 8-bit registers from various sources (immediate values, other registers, memory).
```z80
LoadExamples:
    LD A, 0x5A        ; Load A with immediate value 90
    LD B, A           ; Copy value from A to B
    LD C, (HL)        ; Load C with byte from address in HL
    LD (0x1234), A    ; Store A at memory address 0x1234
    LD D, (IX+5)      ; Load D from address IX + 5
```

### 16-bit Load
*   **Mnemonics**: `LD`, `PUSH`, `POP`
*   **Description**: Loads data into 16-bit register pairs, pushes them onto the stack, or pops them off the stack.
```z80
Load16BitExamples:
    LD BC, 0xABCD      ; Load BC with immediate 16-bit value
    LD HL, (DataTable) ; Load HL with the word at address DataTable
    LD SP, 0xF000      ; Set the Stack Pointer

    PUSH BC            ; Push BC onto the stack
    PUSH IX
    POP  DE            ; Pop value from stack into DE (DE = IX)
    POP  AF            ; Pop value from stack into AF (AF = BC)
```

### Exchange, Block Transfer, and Search
*   **Mnemonics**: `EX`, `EXX`, `LDI`, `LDD`, `LDIR`, `LDDR`, `CPI`, `CPD`, `CPIR`, `CPDR`
*   **Description**: Instructions for swapping register contents and performing block memory operations.
```z80
BlockOpsExamples:
    EX DE, HL          ; Swap the contents of DE and HL
    EXX                ; Swap BC, DE, HL with BC', DE', HL'

    LDIR               ; Copy BC bytes from (HL) to (DE), increment HL, DE, decrement BC
    LDDR               ; Same as LDIR, but decrements pointers
    CPIR               ; Compare A with (HL), increment HL, decrement BC until match or BC=0
```

### 8-bit Arithmetic
*   **Mnemonics**: `ADD`, `ADC`, `SUB`, `SBC`, `AND`, `OR`, `XOR`, `CP`, `INC`, `DEC`
*   **Description**: Perform arithmetic and logical operations on the accumulator (A) or other 8-bit registers.
```z80
Arith8BitExamples:
    ADD A, B           ; A = A + B
    ADC A, (HL)        ; A = A + (HL) + Carry Flag
    SUB 10             ; A = A - 10
    SBC A, C           ; A = A - C - Carry Flag
    AND %11110000      ; Mask lower 4 bits of A
    XOR A              ; A = A XOR A (clears A)
    CP (HL)            ; Compare A with (HL), set flags
    INC B              ; Increment B
    DEC (IX-1)         ; Decrement byte at address IX - 1
```

### General Arithmetic and CPU Control
*   **Mnemonics**: `DAA`, `CPL`, `NEG`, `CCF`, `SCF`, `NOP`, `HALT`, `DI`, `EI`, `IM`
*   **Description**: Miscellaneous CPU control and arithmetic adjustment instructions.
```z80
CpuControlExamples:
    DAA                ; Decimal Adjust Accumulator after BCD arithmetic
    CPL                ; Complement A (A = ~A)
    NEG                ; Negate A (2's complement)
    CCF                ; Complement Carry Flag
    DI                 ; Disable interrupts
    EI                 ; Enable interrupts
    IM 1               ; Set Interrupt Mode 1
    HALT               ; Halt CPU until an interrupt occurs
```

### 16-bit Arithmetic
*   **Mnemonics**: `ADD`, `ADC`, `SBC`, `INC`, `DEC`
*   **Description**: Perform arithmetic on 16-bit register pairs.
```z80
Arith16BitExamples:
    ADD HL, BC         ; HL = HL + BC
    ADD IX, DE         ; IX = IX + DE
    ADC HL, DE         ; HL = HL + DE + Carry Flag
    SBC HL, BC         ; HL = HL - BC - Carry Flag
    INC SP             ; Increment Stack Pointer
    DEC IY             ; Decrement IY
```

### Rotations and Shifts
*   **Mnemonics**: `RLCA`, `RLA`, `RRCA`, `RRA`, `RLC`, `RL`, `RRC`, `RR`, `SLA`, `SRA`, `SRL`, `RLD`, `RRD`
*   **Description**: Bitwise rotation and shift operations on registers or memory.
```z80
RotateShiftExamples:
    RLA                ; Rotate A left through carry
    RRC B              ; Rotate B right, bit 0 to bit 7 and carry
    SLA (HL)           ; Shift byte at (HL) left, bit 0 = 0
    SRL C              ; Shift C right, bit 7 = 0
    RLD                ; Rotate digit left between A and (HL)
```

### Bit Set, Reset, and Test
*   **Mnemonics**: `BIT`, `SET`, `RES`
*   **Description**: Manipulate individual bits within an 8-bit register or memory location.
```z80
BitManipulationExamples:
    BIT 7, A           ; Test bit 7 of A, set Z flag if 0
    SET 0, (HL)        ; Set bit 0 of the byte at address HL
    RES 4, B           ; Reset bit 4 of register B
    SET 2, (IY+10)     ; Set bit 2 of the byte at address IY + 10
```

### Jumps
*   **Mnemonics**: `JP`, `JR`, `DJNZ`
*   **Description**: Control program flow by jumping to different addresses, conditionally or unconditionally.
```z80
JumpExamples:
    JP MainLoop        ; Unconditional jump to MainLoop
    JP (HL)            ; Jump to address contained in HL
    JR NZ, .skip      ; Relative jump if Not Zero
    DJNZ LoopCounter   ; Decrement B and jump if B is not zero
.skip:
    NOP
```

### Calls and Returns
*   **Mnemonics**: `CALL`, `RET`, `RETI`, `RETN`, `RST`
*   **Description**: Subroutine call and return instructions.
```z80
CallReturnExamples:
    CALL PrintMessage  ; Call subroutine at PrintMessage
    CALL C, IsCarry    ; Call if Carry flag is set
    RET                ; Return from subroutine
    RET Z              ; Return if Zero flag is set
    RST 0x28           ; Restart - fast call to address 0x0028
```

### Input and Output
*   **Mnemonics**: `IN`, `INI`, `INIR`, `IND`, `INDR`, `OUT`, `OUTI`, `OTIR`, `OUTD`, `OTDR`
*   **Description**: Instructions for communicating with I/O ports.
```z80
IOExamples:
    IN A, (0xFE)       ; Read from port 0xFE into A
    OUT (C), A         ; Write value of A to port specified by C
    INIR               ; Input from port C, store at (HL), inc HL, dec B, repeat if B!=0
    OTDR               ; Output from (HL) to port C, dec HL, dec B, repeat if B!=0
```

### Undocumented Instructions
*   **Mnemonics**: `SLL` (alias `SLI`), `OUT (C)`, etc.
*   **Description**: The assembler also supports many "undocumented" or "illegal" instructions that perform useful operations.
```z80
UndocumentedExamples:
    SLL B              ; Shift B left, bit 0 is set to 1 (alias for SLI)
    OUT (C), 0         ; Some assemblers support this form
```
