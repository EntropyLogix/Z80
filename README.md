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

The `Z80` class is a template that can be configured with up to three external interface classes to handle system interactions: `TBus`, `TEvents`, and `TDebugger`. Default implementations are provided for each, allowing for quick setup and testing.

### Default Implementations

If you don't provide custom classes when instantiating the `Z80` template, the emulator will use the following defaults, which are defined at the end of `Z80.h`:

*   **`Z80DefaultBus`**: Implements a simple memory and I/O bus.
    *   Provides a 64KB RAM space (`std::vector<uint8_t>`).
    *   `read`/`write` operations access this internal RAM.
    *   `in` operations always return `0xFF`.
    *   `out` operations do nothing.
*   **`Z80DefaultEvents`**: A minimal event handler that effectively disables the event system for maximum performance when timing is not critical.
    *   The next event is scheduled for the maximum possible cycle count (`LLONG_MAX`), so `handle_event` is never called.
*   **`Z80DefaultDebugger`**: A stub implementation with empty methods.
    *   All debugger hooks (`before_step`, `after_step`, etc.) are no-ops and will be optimized out by the compiler, ensuring zero overhead when not in use.

This allows you to instantiate the CPU with minimal boilerplate, for example `Z80<> cpu;`, which is useful for simple test cases or scenarios where you only need the core CPU logic.

### **Required Interfaces**

Your custom classes must implement the methods detailed below to interact with the CPU core.

#### `TBus` Interface
Responsible for communication with memory and I/O ports.

| Method | Description |
| :--- | :--- |
| `void connect(Z80<...>* cpu)` | Called by the Z80 constructor to pass a pointer to the CPU instance. This allows the bus to have backward access to the processor, e.g., to trigger an interrupt. |
| `uint8_t read(uint16_t address)` | Reads a single byte from the 16-bit memory address space. |
| `void write(uint16_t address, uint8_t value)` | Writes a single byte to the 16-bit memory address space. |
| `uint8_t in(uint16_t port)` | Reads a single byte from the 16-bit I/O port address space. |
| `void out(uint16_t port, uint8_t value)` | Writes a single byte to the 16-bit I/O port address space. |
| `void reset()` | Resets the bus state (e.g., clears RAM, resets connected devices). |

#### `TEvents` Interface
Manages cycle-dependent events, which are crucial for precise timing.

| Method | Description |
| :--- | :--- |
| `void connect(const Z80<...>* cpu)` | Called by the Z80 constructor. It passes a constant pointer to the CPU, allowing the event system to read its state (like the cycle counter) without modifying it. |
| `long long get_event_limit() const` | Returns the absolute cycle count (`m_ticks`) at which the next event should occur. The CPU core compares its internal cycle counter against this value. |
| `void handle_event(long long tick)` | A callback function invoked by the CPU when its cycle counter reaches the value returned by `get_event_limit()`. Used to handle timing-based events (e.g., video frame interrupts). |
| `void reset()` | Resets the state of the event system. |

#### `TDebugger` Interface
Allows an external debugger to be attached to trace code execution.

| Method | Description |
| :--- | :--- |
| `void connect(const Z80<...>* cpu)` | Called by the Z80 constructor. It passes a constant pointer to the CPU to allow the debugger to inspect registers and processor state. |
| `before_step(...)` / `after_step(...)` | Hooks called before and after each instruction is executed. The method signature depends on the `Z80_DEBUGGER_OPCODES` macro.<br>• **Without macro:** `void before_step()`<br>• **With macro:** `void before_step(const std::vector<uint8_t>& opcodes)`<br>The `opcodes` vector contains the full byte sequence of the instruction (opcode + operands). |
| `void before_IRQ()` / `void after_IRQ()` | Hooks called just before and after handling a maskable interrupt (IRQ). |
| `void before_NMI()` / `void after_NMI()` | Hooks called just before and after handling a non-maskable interrupt (NMI). |
| `void reset()` | Resets the internal state of the debugger. |

### **Example Implementation Snippet**

The following snippets demonstrate how to initialize and use the Z80 core in various configurations.

#### 1. Using Default Implementations
The simplest way to get the CPU running, perfect for basic tests. It uses `Z80DefaultBus` (with 64KB RAM), `Z80DefaultEvents` (no events), and `Z80DefaultDebugger` (no debugging).

```cpp
#include "Z80.h"
#include <iostream>

int main() {
    // Initializes the CPU with default components.
    Z80<> cpu;

    // Write a simple program to memory (LD A, 0x42; HALT)
    cpu.get_bus()->write(0x0000, 0x3E);
    cpu.get_bus()->write(0x0001, 0x42);
    cpu.get_bus()->write(0x0002, 0x76);

    // Run the emulation
    cpu.run(100);

    // Check the state of register A
    // std::cout << "Register A: " << (int)cpu.get_A() << std::endl; // Should print 66
    return 0;
}
```

#### 2. Using a Custom `TBus`
This example shows how to integrate a custom bus, for instance, to handle a specific memory map or I/O devices.

```cpp
#include "Z80.h"

class CustomBus {
    // ... implement read, write, in, out, reset, connect ...
public:
    void connect(Z80<CustomBus>* cpu) { /* ... */ }
    void reset() { /* ... */ }
    uint8_t read(uint16_t address) { return m_ram[address]; }
    void write(uint16_t address, uint8_t value) { m_ram[address] = value; }
    uint8_t in(uint16_t port) { return 0xFF; }
    void out(uint16_t port, uint8_t value) { /* ... */ }
private:
    uint8_t m_ram[0x10000];
};

int main() {
    CustomBus my_bus;
    Z80<CustomBus> cpu(&my_bus); // Pass a pointer to the custom bus
    cpu.run(100);
    return 0;
}
```

#### 3. Using a Custom `TEvents`
Ideal when you need precise timing, such as for synchronizing with a video chip emulation.

```cpp
#include "Z80.h"
#include <iostream>

class CustomEvents {
    // ... implement get_event_limit, handle_event, reset, connect ...
public:
    void connect(const Z80<Z80DefaultBus, CustomEvents>* cpu) { /* ... */ }
    void reset() { /* ... */ }
    long long get_event_limit() const { return 20000; } // Trigger event every 20000 ticks
    void handle_event(long long tick) {
        // std::cout << "Event at tick: " << tick << std::endl;
        // A V-blank interrupt could be triggered here.
    }
};

int main() {
    CustomEvents my_events;
    // Use the default bus (nullptr) and a custom event system
    Z80<Z80DefaultBus, CustomEvents> cpu(nullptr, &my_events);
    cpu.run(100000);
    return 0;
}
```

#### 4. Using All Custom Classes
A full-featured setup for a complex project, like a complete computer emulator.

```cpp
#include "Z80.h"

class MyBus { /* ... */ };
class MyEvents { /* ... */ };
class MyDebugger { /* ... */ };

int main() {
    // 1. Instantiate all custom components
    MyBus bus;
    MyEvents events;
    MyDebugger debugger;

    // 2. Initialize the Z80 core, passing pointers to all components
    Z80<MyBus, MyEvents, MyDebugger> cpu(&bus, &events, &debugger);

    // 3. Run the emulation
    long long ticks_per_frame = 4000000;
    cpu.run(cpu.get_ticks() + ticks_per_frame);

    return 0;
}
```

## **⚙️ Configuration**

The emulator's behavior and performance can be customized using several preprocessor directives and CMake build options.

### Compile-Time Definitions (`#define`)

The following macros can be added to your C++ compiler flags in your build system (e.g., using `-D<MACRO_NAME>`).

| Macro | Description |
| :--- | :--- |
| `Z80_BIG_ENDIAN` | By default, the emulator assumes the host system is little-endian for register pairs (AF, BC, etc.). Define this macro if you are compiling on a big-endian architecture to ensure correct mapping of 8-bit to 16-bit registers. |
| `Z80_DEBUGGER_OPCODES` | Enables collecting instruction bytes (opcode and operands) and passing them to the `TDebugger` implementation in the `before_step` and `after_step` methods. Useful for creating detailed debugging and tracing tools. |
| `Z80_ENABLE_EXEC_API` | Exposes the public `exec_*` API, which allows executing individual Z80 instructions by calling dedicated methods (e.g., `cpu.exec_NOP()`, `cpu.exec_LD_A_n()`). This can be useful for testing or specific scenarios but is disabled by default to keep the public interface clean. |

### Build Options (CMake)

These options can be configured when generating the project with CMake (e.g., `cmake -D<OPTION>=ON ..`).

| Option | Default | Description |
| :--- | :--- | :--- |
| `ENABLE_PGO` | `OFF` | Enables Profile-Guided Optimization (PGO). This requires a two-stage build process and is primarily supported by GCC and Clang compilers. |
| `PGO_MODE` | - | Used when `ENABLE_PGO` is on. Set to `GENERATE` to build an instrumented version for collecting a performance profile. Then, set to `USE` to compile the final, optimized binary using the collected data. |

**Example of using PGO:**

1.  **Generate Profile:**
    ```bash
    cmake -B build -DENABLE_PGO=ON -DPGO_MODE=GENERATE
    cmake --build build
    ./build/Z80 zexdoc.com # Run the program to generate profile data (*.gcda)
    ```
2.  **Build with Optimization:**
    ```bash
    cmake -B build -DENABLE_PGO=ON -DPGO_MODE=USE
    cmake --build build # The compiler will use the profile data to optimize
    ```
