# crappola
A C compiler for macOS and Linux

## Overview

Crappola is a minimal C compiler written in C that demonstrates all the essential components of a modern compiler:

- **Preprocessor**: Handles `#define` macros and directive processing
- **Lexer**: Tokenizes the source code into meaningful tokens
- **Parser**: Builds an Abstract Syntax Tree (AST) from tokens
- **Code Generator**: Generates x86_64 assembly code
- **Linker**: Links object files into executable binaries

The compiler targets macOS (primary target) and also supports Linux for testing.

## Features

- Basic C language support (subset):
  - Function definitions (`int main()`)
  - Variable declarations and assignments
  - Arithmetic operations (`+`, `-`, `*`, `/`)
  - Comparison operators (`<`, `>`, `<=`, `>=`, `==`, `!=`)
  - Control flow (`if`/`else`, `while`)
  - Return statements
- Preprocessor features:
  - Macros (`#define`)
  - File includes (`#include`)
  - Nested includes with cycle detection
  - Line directives for accurate error reporting
- x86_64 assembly generation
- Automatic linking with system libraries

## Building

Requirements:
- CMake 3.10 or later
- C11 compatible C compiler
- System assembler (`as`)
- System linker (`ld`)

Build steps:

```bash
mkdir build
cd build
cmake ..
make
```

The compiler executable will be created as `build/crappola`.

## Usage

Compile a C source file:

```bash
./build/crappola input.c -o output
```

If no output file is specified, the default is `a.out`:

```bash
./build/crappola input.c
```

## Examples

The `examples/` directory contains sample programs:

- `hello.c` - Demonstrates preprocessor with `#define`
- `variables.c` - Shows variable declarations and arithmetic
- `arithmetic.c` - Complex arithmetic expressions
- `include_test.c` - Demonstrates `#include` directive
- `nested_include.c` - Shows nested includes
- `cycle_test.c` - Demonstrates cycle detection in includes

Compile and run an example:

```bash
./build/crappola examples/hello.c -o hello
./hello
echo $?  # Prints the exit code
```

## Architecture

The compiler follows a traditional multi-pass architecture:

1. **Preprocessing** (`preprocessor.c`): Expands macros and processes directives
2. **Lexical Analysis** (`lexer.c`): Converts source code into tokens
3. **Parsing** (`parser.c`): Builds an Abstract Syntax Tree
4. **Code Generation** (`codegen.c`): Generates x86_64 assembly code
5. **Linking** (`linker.c`): Assembles and links the final executable

### Directory Structure

```
crappola/
├── CMakeLists.txt          # Build configuration
├── include/
│   └── crappola.h         # Main header file
├── src/
│   ├── main.c             # Compiler driver
│   ├── preprocessor.c     # Preprocessor implementation
│   ├── lexer.c            # Lexical analyzer
│   ├── parser.c           # Syntax parser
│   ├── codegen.c          # Code generator
│   └── linker.c           # Linker integration
└── examples/              # Sample programs
```

## Limitations

As a minimal compiler, Crappola has several limitations:

- Only supports `int` type
- No function parameters or multiple functions
- No arrays, pointers, or structs
- Limited preprocessor (no `#ifdef`, `#ifndef`, etc.)
- No optimization passes
- Basic error reporting

## License

This project is a demonstration compiler for educational purposes.
