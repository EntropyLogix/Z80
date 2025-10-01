# **Z80 CPU Emulator Core (C++ Template)**

This repository contains a high-performance, header-only C++ template implementation of the **Zilog Z80 microprocessor** emulator core.

The design emphasizes decoupling the CPU logic from external components (Memory, I/O, and Timing/Events) using C++ templates, making it highly flexible and easy to integrate into various retro-computing projects or emulators (e.g., ZX Spectrum, Game Boy, MSX).

## **✨ Features**

* **Header-Only Design:** Easy inclusion and integration into any C++ project.  
* **Template-Based Architecture:** The core logic (Z80\<TMemory, TIO, TEvents\>) is completely decoupled from system specifics, promoting clean separation of concerns.  
* **Cycle-Accurate Timing:** Includes granular cycle tracking and integration with a user-defined event system (TEvents).  
* **Comprehensive Instruction Set:** Implements the core Z80 instruction set, including standard opcodes and the extended instruction sets (prefixed opcodes like CB, ED, DD, and FD).  
* **State Management:** Provides utility functions (save\_state, load\_state) for easy serialization and save-state functionality.

## **🚀 Usage and Integration**

The Z80 class is a template requiring three external interface classes to handle system interactions: TMemory, TIO, and TEvents.

### **Required Interfaces**

Your custom classes must provide the methods detailed below (based on the conceptual examples provided in Z80.h):

| Interface | Required Methods | Description |
| :---- | :---- | :---- |
| **TMemory** | uint8\_t read(uint16\_t address) | Reads a single byte from the specified memory address. |
| **TMemory** | void write(uint16\_t address, uint8\_t value) | Writes a single byte to the specified memory address. |
| **TMemory** | void reset() | Resets the memory state (e.g., clears RAM). |
| **TIO** | uint8\_t read(uint16\_t port) | Reads a byte from the specified I/O port. |
| **TIO** | void write(uint16\_t port, uint8\_t value) | Writes a byte to the specified I/O port. |
| **TIO** | void reset() | Resets the I/O state. |
| **TEvents** | static constexpr long long CYCLES\_PER\_EVENT | Defines the interval between automatic event checks. |
| **TEvents** | long long get\_event\_limit() const | Returns the next scheduled cycle for an event. |
| **TEvents** | void handle\_event(long long tick) | Called when the CPU reaches an event threshold to handle timing-based events. |
| **TEvents** | void reset() | Resets event tracking. |

### **Example Implementation Snippet**

// (Assumes you have defined Z80Memory, Z80IO, and Z80Events classes)

\#include "Z80.h"

// 1\. Instantiate the interfaces  
Z80Memory memory;  
Z80IO io;  
Z80Events events;

// 2\. Initialize the Z80 core  
Z80\<Z80Memory, Z80IO, Z80Events\> cpu(memory, io, events);

// 3\. Run the emulation  
long long ticks\_to\_execute \= 10000;  
long long executed\_ticks \= cpu.run(cpu.get\_ticks() \+ ticks\_to\_execute);

// Or step one instruction at a time  
int instruction\_cycles \= cpu.step();

## **⚙️ Configuration**

The core uses a preprocessor macro to configure register endianness, which is crucial for the internal Register union structure:

* **Little Endian (Domyślne):** Domyślnie używany jest porządek Little Endian, jeśli nie zdefiniowano żadnego makra.  
* **Big Endian:** Aby włączyć Big Endian, należy zdefiniować makro **Z80\_BIG\_ENDIAN** podczas kompilacji.

It also includes optimization hints for certain compilers:

* \#define EXPECT\_TRUE(expr): Uses compiler intrinsics (\_\_builtin\_expect) or C++20 attributes (\[\[likely\]\]) where available to optimize branch prediction for frequently taken paths (e.g., default index register being HL).