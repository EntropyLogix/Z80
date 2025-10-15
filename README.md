# **Z80 CPU Emulator Core (C++ Template)**

This repository contains a high-performance, header-only C++ template implementation of the **Zilog Z80 microprocessor** emulator core. 

The design emphasizes decoupling the CPU logic from external components (Bus, Events, and Debugger) using C++ templates, making it highly flexible and easy to integrate into various retro-computing projects or emulators (e.g., ZX Spectrum, CP/M, MSX).

## **✨ Features**

* **Header-Only Design:** Easy inclusion and integration into any C++ project.  
* **Template-Based Architecture:** The core logic (`Z80<TBus, TEvents, TDebugger>`) is completely decoupled from system specifics, promoting clean separation of concerns.  
* **Cycle-Accurate Timing:** Includes granular cycle tracking and integration with a user-defined event system (TEvents).  
* **Comprehensive Instruction Set:** Implements the core Z80 instruction set, including standard opcodes and the extended instruction sets (prefixed opcodes like CB, ED, DD, and FD).  
* **State Management:** Provides utility functions (save\_state, load\_state) for easy serialization and save-state functionality.

## **🚀 Usage and Integration**

The `Z80` class is a template that can be configured with up to three external interface classes to handle system interactions: `TBus`, `TEvents`, and `TDebugger`. Default implementations are provided for each.

### **Required Interfaces**

Your custom classes must provide the methods detailed below (based on the conceptual examples provided in Z80.h):

| Interface | Required Methods | Description |
| :--- | :--- | :--- |
| **TBus** | `void connect(Z80<...>* cpu)` | Connects the bus to the CPU instance. |
| **TBus** | `uint8_t read(uint16_t address)` | Reads a single byte from the specified memory address. |
| **TBus** | `void write(uint16_t address, uint8_t value)` | Writes a single byte to the specified memory address. |
| **TBus** | `uint8_t in(uint16_t port)` | Reads a byte from the specified I/O port. |
| **TBus** | `void out(uint16_t port, uint8_t value)` | Writes a byte to the specified I/O port. |
| **TBus** | `void reset()` | Resets the bus state (e.g., clears RAM). |
| **TEvents** | `void connect(const Z80<...>* cpu)` | Connects the event system to the CPU instance. |
| **TEvents** | `long long get_event_limit() const` | Returns the next scheduled cycle for an event. |
| **TEvents** | `void handle_event(long long tick)` | Called when the CPU reaches an event threshold. |
| **TEvents** | `void reset()` | Resets event tracking. |
| **TDebugger** | `void connect(const Z80<...>* cpu)` | Connects the debugger to the CPU instance. |
| **TDebugger** | `void before_step(...)` / `void after_step(...)` | Called before/after executing an instruction. |
| **TDebugger** | `void before_IRQ()` / `void after_IRQ()` | Called when handling an IRQ. |
| **TDebugger** | `void before_NMI()` / `void after_NMI()` | Called when handling an NMI. |
| **TDebugger** | `void reset()` | Resets the debugger state. |

### **Example Implementation Snippet**

// (Assumes you have defined MyBus, MyEvents, and MyDebugger classes)

\#include "Z80.h"

// 1\. Instantiate the interfaces  
MyBus bus;  
MyEvents events;

// 2\. Initialize the Z80 core  
Z80<MyBus, MyEvents> cpu(&bus, &events);

// 3\. Run the emulation  
long long ticks\_to\_execute \= 10000;  
long long executed\_ticks \= cpu.run(cpu.get\_ticks() \+ ticks\_to\_execute);

// Or step one instruction at a time  
int instruction\_cycles \= cpu.step();

## **⚙️ Configuration**

The core uses a preprocessor macro to configure register endianness, which is crucial for the internal Register union structure:

Little Endian (Default): Little Endian order is used by default if no macro is defined.

Big Endian: To enable Big Endian, the macro Z80_BIG_ENDIAN must be defined during compilation.
