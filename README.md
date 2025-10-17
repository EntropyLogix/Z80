# **Z80 CPU Emulator Core**

This repository contains a high-performance, **header-only C++ template** implementation of the **Zilog Z80 microprocessor** emulator core. 

The design emphasizes decoupling the CPU logic from external components (Bus, Events, and Debugger) using C++ templates, making it highly flexible and easy to integrate into various retro-computing projects or emulators (e.g., ZX Spectrum, Amstrad CPC, TRS-80, MSX, Sega Master System, or CP/M-based machines).

---

## ✨ **Features**

* **Header-Only Design:** Easy inclusion and integration into any C++ project.  
* **Template-Based Architecture:** The core logic (`Z80<TBus, TEvents, TDebugger>`) is completely decoupled from system specifics, promoting clean separation of concerns.  
* **Cycle-Accurate Timing:** Includes granular cycle tracking and integration with a user-defined event system (TEvents).  
* **Comprehensive Instruction Set:** Implements the core Z80 instruction set, including standard opcodes and the extended instruction sets (prefixed opcodes like CB, ED, DD, and FD).  
* **State Management:** Provides utility functions (save\_state, load\_state) for easy serialization and save-state functionality.

---

## 🧩 **Usage and Integration**

The `Z80` class is a template that can be configured with up to three external interface classes to handle system interactions: `TBus`, `TEvents`, and `TDebugger`. This design allows for maximum flexibility by separating the CPU core logic from the system-specific hardware it interacts with.

```cpp
template <
    typename TBus = Z80DefaultBus, 
    typename TEvents = Z80DefaultEvents, 
    typename TDebugger = Z80DefaultDebugger
>
class Z80 {
public:
    Z80(TBus* bus = nullptr, TEvents* events = nullptr, TDebugger* debugger = nullptr);
    
    // ... public methods like run(), step(), reset() ...
};
```

### Constructor and Ownership

The `Z80` constructor manages the lifecycle of its `TBus`, `TEvents`, and `TDebugger` dependencies through a flexible ownership model.

```cpp
Z80(TBus* bus = nullptr, TEvents* events = nullptr, TDebugger* debugger = nullptr);
```

*   **Passing a Pointer (External Ownership):** If you provide a valid pointer to an existing object, the `Z80` core will use that object. In this case, the `Z80` instance does **not** take ownership, and you are responsible for managing the object's memory (i.e., creating and deleting it). This is useful for sharing a single bus or event system across multiple components.

*   **Passing `nullptr` (Internal Ownership):** If you pass `nullptr` or omit the argument, the `Z80` core will internally create a new instance of the corresponding template type (e.g., `new TBus()`). The `Z80` core then **takes ownership** and will automatically `delete` the object in its destructor. This is convenient for self-contained setups.

This model gives you the choice between dependency injection (you control the lifetime) and composition (the `Z80` object controls the lifetime).

### **Public API Reference**

This section details the primary public methods for controlling and interacting with the Z80 CPU core.

#### **Core Execution and Control**

| Method | Description |
| :--- | :--- |
| `long long run(long long ticks_limit)` | Runs the emulation until the internal T-state counter (`m_ticks`) reaches `ticks_limit`. Returns the number of ticks executed. |
| `int step()` | Executes a single Z80 instruction and returns the number of T-states it took. |
| `void reset()` | Resets all CPU registers, flags, and internal state to their power-on defaults (e.g., PC to `0x0000`, SP to `0xFFFF`). |
| `void request_interrupt(uint8_t data)` | Signals a maskable interrupt (IRQ) request. The `data` byte is used for interrupt mode 0. The interrupt will be handled on the next instruction cycle if IFF1 is enabled. |
| `void request_NMI()` | Signals a non-maskable interrupt (NMI) request. This interrupt is always handled, regardless of the IFF1 flag state. |

#### **State Management**

These methods allow for saving and restoring the complete state of the CPU, which is essential for implementing features like save states.

| Method | Description |
| :--- | :--- |
| `State save_state() const` | Captures the current state of all CPU registers and internal flags into a `State` struct and returns it. |
| `void restore_state(const State& state)` | Restores the CPU's state from a given `State` struct, overwriting all current register and flag values. |

#### **Register and Flag Access**

The API provides a comprehensive set of getter and setter methods for all Z80 registers.

##### **16-bit Registers**

*   `get_AF()`, `set_AF(uint16_t value)`
*   `get_BC()`, `set_BC(uint16_t value)`
*   `get_DE()`, `set_DE(uint16_t value)`
*   `get_HL()`, `set_HL(uint16_t value)`
*   `get_IX()`, `set_IX(uint16_t value)`
*   `get_IY()`, `set_IY(uint16_t value)`
*   `get_SP()`, `set_SP(uint16_t value)`
*   `get_PC()`, `set_PC(uint16_t value)`
*   Alternate registers: `get_AFp()`, `get_BCp()`, `get_DEp()`, `get_HLp()`, etc.

##### **8-bit Registers**

*   `get_A()`, `set_A(uint8_t value)`
*   `get_B()`, `set_B(uint8_t value)`
*   `get_C()`, `set_C(uint8_t value)`
*   ...and so on for `D`, `E`, `H`, `L`, `I`, `R`.
*   Indexed register parts: `get_IXH()`, `get_IXL()`, `get_IYH()`, `get_IYL()`.

##### **Flags Register (F)**

The `F` register is accessed via a helper class `Z80::Flags` that provides a convenient interface for flag manipulation.

*   `Flags get_F() const`: Returns the `Flags` object.
*   `void set_F(Flags value)`: Sets the entire flags register.
*   `flags.is_set(Flags::Z)`: Checks if the Zero flag is set.
*   `flags.set(Flags::C)`: Sets the Carry flag.
*   `flags.clear(Flags::N)`: Clears the Subtract flag.

#### **Direct Instruction Execution (`exec_*` API)**

When compiled with `Z80_ENABLE_EXEC_API`, the emulator exposes a set of public methods for executing individual instructions directly. This is useful for unit testing or building specialized tools. The methods follow the Z80 instruction naming convention.

**Note:** These methods do not perform a fetch-decode cycle. They execute the corresponding instruction's logic directly but will still perform memory reads/writes via the bus if the instruction requires an operand (e.g., `LD A, n`).

**Example Methods:**

*   `exec_NOP()`
*   `exec_LD_BC_nn()`
*   `exec_LD_BC_ptr_A()`
*   `exec_INC_BC()`
*   `exec_INC_B()`
*   `...`
*   `exec_SET_7_IY_d_ptr_H(int8_t offset)`
*   `exec_SET_7_IY_d_ptr_L(int8_t offset)`
*   `exec_SET_7_IY_d_ptr(int8_t offset)`
*   `exec_SET_7_IY_d_ptr_A(int8_t offset)`

### **Core Interfaces (TBus, TEvents, TDebugger)**

The power of the emulator comes from its template-based design, which allows you to customize its interaction with the outside world by providing your own implementations for three core interfaces: `TBus`, `TEvents`, and `TDebugger`.

For each interface, you can either use the provided default for simplicity or pass a pointer to your own custom class in the `Z80` constructor to achieve specialized behavior.

#### `TBus` Interface
Responsible for all communication with memory and I/O ports.

| Method | Description |
| :--- | :--- |
| `void connect(Z80<...>* cpu)` | Called by the Z80 constructor to pass a pointer to the CPU instance. This allows the bus to have backward access to the processor, e.g., to trigger an interrupt. |
| `uint8_t read(uint16_t address)` | Reads a single byte from the 16-bit memory address space. This can have side effects (e.g., handling memory-mapped I/O). |
| `void write(uint16_t address, uint8_t value)` | Writes a single byte to the 16-bit memory address space. |
| `uint8_t peek(uint16_t address) const` | Reads a single byte from memory without any side effects. This is primarily used by tools like the disassembler. |
| `uint8_t in(uint16_t port)` | Reads a single byte from the 16-bit I/O port address space. |
| `void out(uint16_t port, uint8_t value)` | Writes a single byte to the 16-bit I/O port address space. |
| `void reset()` | Resets the bus state (e.g., clears RAM, resets connected devices). |

**Default Implementation (`Z80DefaultBus`):**
Provides a simple 64KB RAM space (`std::vector<uint8_t>`). `read`/`write` operations access this internal RAM, `in` operations always return `0xFF`, and `out` operations do nothing. This is useful for basic testing.

#### `TEvents` Interface
Manages cycle-dependent events, which are crucial for precise, hardware-accurate timing (e.g., synchronizing with a video controller).

| Method | Description |
| :--- | :--- |
| `void connect(const Z80<...>* cpu)` | Called by the Z80 constructor. It passes a constant pointer to the CPU, allowing the event system to read its state (like the cycle counter) without modifying it. |
| `long long get_event_limit() const` | Returns the absolute cycle count (`m_ticks`) at which the next event should occur. The CPU core compares its internal cycle counter against this value. |
| `void handle_event(long long tick)` | A callback function invoked by the CPU when its cycle counter reaches the value returned by `get_event_limit()`. Used to handle timing-based events (e.g., video frame interrupts). |
| `void reset()` | Resets the state of the event system. |

**Default Implementation (`Z80DefaultEvents`):**
A minimal event handler that effectively disables the event system for maximum performance. The next event is scheduled for the maximum possible cycle count (`LLONG_MAX`), so `handle_event` is never called.

#### `TDebugger` Interface
Provides hooks that allow an external tool to be attached to monitor and trace code execution.

| Method | Description |
| :--- | :--- |
| `void connect(const Z80<...>* cpu)` | Called by the Z80 constructor. It passes a constant pointer to the CPU to allow the debugger to inspect registers and processor state. |
| `before_step(...)` / `after_step(...)` | Hooks called before and after each instruction is executed. The method signature depends on the `Z80_DEBUGGER_OPCODES` macro.<br>• **Without macro:** `void before_step()`<br>• **With macro:** `void before_step(const std::vector<uint8_t>& opcodes)`<br>The `opcodes` vector contains the full byte sequence of the instruction (opcode + operands). |
| `void before_IRQ()` / `void after_IRQ()` | Hooks called just before and after handling a maskable interrupt (IRQ). |
| `void before_NMI()` / `void after_NMI()` | Hooks called just before and after handling a non-maskable interrupt (NMI). |
| `void reset()` | Resets the internal state of the debugger. |

**Default Implementation (`Z80DefaultDebugger`):**
A stub implementation with empty methods. All debugger hooks are no-ops and will be optimized out by the compiler, ensuring zero performance overhead when debugging is not needed.

### **Example Implementation Snippets**

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

#### 5. Using the `exec_*` API for Direct Instruction Execution
This advanced example demonstrates how to execute single instructions directly without a full fetch-decode-execute cycle from memory. This is useful for unit testing instruction implementations or for building custom tools.

**Note:** This requires the `Z80_ENABLE_EXEC_API` macro to be defined during compilation (e.g., `g++ -DZ80_ENABLE_EXEC_API ...`).

```cpp
#include "Z80.h"
#include <iostream>
#include <iomanip>

int main() {
    Z80<> cpu;

    // The exec_* API still uses the bus to fetch operands if needed.
    // Let's prepare memory for LD A, 0x12 and LD B, 0x34
    cpu.get_bus()->write(0x0000, 0x12); // Operand for LD A, n
    cpu.get_bus()->write(0x0001, 0x34); // Operand for LD B, n
    cpu.set_PC(0x0000);

    // Execute instructions directly
    cpu.exec_LD_A_n();  // Executes LD A, n. Fetches 0x12 from PC=0. PC becomes 1.
    cpu.exec_LD_B_n();  // Executes LD B, n. Fetches 0x34 from PC=1. PC becomes 2.
    cpu.exec_ADD_A_B(); // Executes ADD A, B. A = A + B = 0x12 + 0x34 = 0x46.

    // Verify the result
    std::cout << "Register A: 0x" << std::hex << (int)cpu.get_A() << std::endl; // Should be 0x46
    std::cout << "Register B: 0x" << std::hex << (int)cpu.get_B() << std::endl; // Should be 0x34
    std::cout << "PC: 0x" << std::hex << std::setw(4) << std::setfill('0') << cpu.get_PC() << std::endl; // Should be 0x0002

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

The library is fully **header-only**, which means it does not require compilation or linking. You just need to add its directory to the include paths in your project.

Below are examples of `CMakeLists.txt` configurations for your project.

---

#### Example 1: Library Downloaded Manually

In this scenario, we assume you have downloaded the library and placed it in a `lib/Z80` subdirectory within your project.

Project structure:
```
my_project/
├── lib/
│   └── Z80/
│       ├── Z80.h
│       └── Z80Analyze.h
├── CMakeLists.txt
└── main.cpp
```

Your `CMakeLists.txt` file could look like this:

```cmake
# Required minimum version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(MyZ80Project)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the Z80 library directory to the include paths
# This tells the compiler where to find the header files.
target_include_directories(my_project PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Add the executable
add_executable(my_project main.cpp)

# No linking is needed because the library is header-only.
```

In your `main.cpp` file, you can now include the library like this:
```cpp
#include "Z80/Z80.h"
```

---

#### Example 2: Using `FetchContent` to Automatically Download the Library

If you prefer CMake to automatically download the library from its GitHub repository, you can use the `FetchContent` module.

Your `CMakeLists.txt` file could look like this:

```cmake
# Required minimum version of CMake
cmake_minimum_required(VERSION 3.15)

# Project name
project(MyZ80Project)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the FetchContent module
include(FetchContent)

# Declare the Z80 library dependency
FetchContent_Declare(
  Z80
  GIT_REPOSITORY https://github.com/EntropyLogix/Z80
  GIT_TAG        v1.0.3 # You can use a tag, branch, or a specific commit
)

# Download and make the library available
# The library will be available as the "Z80" target
FetchContent_MakeAvailable(Z80)

# Add the executable
add_executable(my_project main.cpp)

# Add the include directories from the fetched library to your target
# The "Z80" target is created by FetchContent_MakeAvailable
target_include_directories(my_project PRIVATE ${Z80_SOURCE_DIR})
```

In your `main.cpp` file, you include the headers in the same way:
```cpp
#include "Z80/Z80.h"
```

---

## 🛠️ **Tools**

This repository also includes utility classes that can be used alongside the Z80 core for debugging and analysis.

### **Disassembler (`Z80Analyzer`)**

The `Z80Analyzer` class provides a powerful tool for disassembling Z80 machine code. It can convert raw instruction bytes from a bus into human-readable assembly mnemonics.

#### **Usage**

To use the analyzer, instantiate it by passing a reference to a bus object (which provides memory access). You can then call the `disassemble` method.

```cpp
#include "Z80.h"
#include "Z80Analyze.h"
#include <iostream>

// Assuming a Bus class that implements read() and peek()
Bus bus; 
Z80Analyzer<Bus> analyzer(bus);

// Write some example code to memory
bus.write(0x0000, 0x21); // LD HL, 0x1234
bus.write(0x0001, 0x34);
bus.write(0x0002, 0x12);

uint16_t pc = 0x0000;
std::string disassembly = analyzer.disassemble(pc);

std::cout << disassembly << std::endl;
```

#### **Output Formatting**

The `disassemble` method accepts an optional format string that allows you to customize the output's appearance, enabling flexible and clean presentation of disassembled code.

`std::string disassemble(uint16_t& address, const std::string& format)`

**Format Specifiers:**

| Specifier | Description |
| :--- | :--- |
| `%a` | Instruction address (hexadecimal). |
| `%A` | Instruction address (decimal). |
| `%b` | Instruction bytes (hexadecimal). |
| `%B` | Instruction bytes (decimal). |
| `%m` | Instruction mnemonic. |

The format string also supports standard `printf`-style modifiers for width, alignment, and fill character:
*   **Width**: `%10b` will set the field width to 10 characters.
*   **Alignment**: `%-10b` will left-align the output. Right-alignment is the default.
*   **Fill Character**: `%-15.B` will use a dot `.` as the fill character instead of a space.

#### **Formatting Examples**

The following code, taken from `main.cpp`, demonstrates the use of different format strings:

```cpp
uint16_t pc = 0x0000;
bus.write(0x0000, 0x21); bus.write(0x0001, 0x34); bus.write(0x0002, 0x12); // LD HL, 0x1234
bus.write(0x0003, 0x7E); // LD A, (HL)
bus.write(0x0004, 0x18); bus.write(0x0005, 0xFB); // JR -5

std::cout << analyzer.disassemble(pc, "%a: %-12b %m") << std::endl;
std::cout << analyzer.disassemble(pc, "%A: %-15.B %m") << std::endl;
std::cout << analyzer.disassemble(pc, "%m") << std::endl;
```

**Output:**

```
0x0000: 21 34 12     LD HL, 0x1234
3: 126............. LD A, (HL)
JR 0x0000
```

---

## 📜 **License**

This project is licensed under the MIT License. See the LICENSE file for details.
