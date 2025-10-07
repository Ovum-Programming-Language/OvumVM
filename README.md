# Ovum Virtual Machine

The Ovum Virtual Machine is the runtime engine for the [Ovum Programming Language](https://github.com/Ovum-Programming-Language/OvumLanguage). It provides a garbage-collected, JIT-compiled execution environment for Ovum bytecode, focusing on memory safety, performance, and single-threaded execution.

## Overview

The Ovum VM is designed to execute Ovum bytecode with the following key features:

- **Memory Safety**: Garbage collection eliminates manual memory management
- **Performance**: JIT compilation optimizes hot code paths
- **Single-threaded**: Predictable execution model without concurrency complexity
- **Cross-platform**: Supports amd64 and arm64 architectures
- **Type Safety**: Enforces Ovum's strong static typing at runtime

## Architecture

The VM consists of several core components:

- **Bytecode Interpreter**: Executes Ovum bytecode instructions
- **Garbage Collector**: Manages memory allocation and deallocation
- **JIT Compiler**: Compiles frequently executed code to native instructions
- **Type System**: Enforces runtime type checking and safety
- **System Interface**: Provides access to system functions and FFI

> **Note**: Due to the architecture-dependent nature of JIT compilation, JIT implementations are located in separate repositories for each target architecture.

## Quick Start

### Prerequisites

- CMake 3.12 or later
- C++20 compatible compiler (GCC, Clang, or MSVC)
- Git

### Building

1. Clone the repository:
```bash
git clone https://github.com/Ovum-Programming-Language/OvumVM.git
cd OvumVM
```

2. Create build directory and configure:
```bash
cmake -S . -B build
```

3. Build the VM:
```bash
cmake --build build --target ovum-vm
```

4. Run the VM:
```bash
# Windows
.\build\bin\ovum-vm.exe

# Unix/Linux/macOS
./build/bin/ovum-vm
```

### Running Tests

```bash
# Build tests
cmake --build build --target ovum-vm_tests

# Run tests
# Windows
.\build\tests\ovum-vm_tests.exe

# Unix/Linux/macOS
./build/tests/ovum-vm_tests
```

## Development

### Building from Source

The VM uses CMake for build configuration. Key build options:

```bash
# Debug build with symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Enable additional warnings
cmake -S . -B build -DENABLE_WARNINGS=ON
```

### Code Style

The project follows C++ best practices and uses:
- Clang-format for code formatting (`.clang-format`)
- Clang-tidy for static analysis (`.clang-tidy`)
- Google Test framework for unit testing

### Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Make your changes following the coding guidelines
4. Add tests for new functionality
5. Ensure all tests pass: `cmake --build build --target ovum-vm_tests`
6. Submit a pull request

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.

## Related Projects

- **[Ovum Language](https://github.com/Ovum-Programming-Language/OvumLanguage)**: The main Ovum programming language repository
- **[Ovum Documentation](https://ovum-programming-language.github.io/OvumDocs/)**: Complete language and VM documentation
- **JIT Implementations**: Architecture-specific JIT compilers (separate repositories)

## Language Features Supported

The VM implements the core runtime features of the Ovum language:

- **Memory Management**: Automatic garbage collection
- **Type System**: Runtime type checking and safety
- **Function Calls**: Support for pure functions and caching
- **Object Model**: Interface-based polymorphism
- **System Integration**: FFI and system function calls
- **Error Handling**: Safe null handling and type casting

## Performance Characteristics

- **Startup Time**: Fast VM initialization
- **Memory Usage**: Efficient garbage collection with low overhead
- **Execution Speed**: JIT compilation provides near-native performance for hot code
- **Memory Safety**: Zero-cost abstractions with guaranteed memory safety

## Supported Platforms

- **Operating Systems**: Windows, Linux, macOS
- **Architectures**: x86_64 (amd64), ARM64
- **Compilers**: GCC 7+, Clang 6+, MSVC 2019+

> **Note**:  Some of these platforms or architectures may not have a JIT compiler implementation.

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## Community

- **Documentation**: [Ovum Docs](https://ovum-programming-language.github.io/OvumDocs/)
- **Issues**: [GitHub Issues](https://github.com/Ovum-Programming-Language/OvumVM/issues)

**Note**: This VM is under active development. The current implementation provides the foundation for executing Ovum bytecode, with JIT compilation capabilities being developed in architecture-specific repositories.