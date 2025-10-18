# RPascal

A Pascal-to-C++ transpiler targeting Turbo Pascal 7 compatibility.

⚠️ **Alpha Software Notice**: RPascal is currently in alpha development. While it successfully compiles and runs many Pascal programs, it should not be considered production-ready or feature-complete.

## Overview

RPascal is a transpiler that converts Pascal source code to C++ and then compiles it using a native C++ compiler. The goal is to provide broad compatibility with Turbo Pascal 7 programs while generating efficient, self-contained C++ executables.

## Features

### Core Language Support
- **Data Types**: integer, real, boolean, char, byte, string, and user-defined types
- **Control Flow**: if/then/else, while, for, repeat/until, case statements
- **Procedures & Functions**: Parameter passing, local variables, recursion, overloading
- **Records**: Including variant records and WITH statements
- **Arrays**: Single and multi-dimensional arrays with proper bounds checking
- **Enumerations**: Full enum support including subrange types (e.g., `TDigit = 0..9`)
- **Pointers**: Dynamic memory allocation and pointer arithmetic
- **Sets**: Set operations and set types
- **File I/O**: Text files, typed files, and binary file operations

### Built-in Units
RPascal includes built-in implementations of standard Turbo Pascal units:
- **System**: Core runtime functions (`writeln`, `readln`, `new`, `dispose`, etc.)
- **CRT**: Console/screen functions (`clrscr`, `gotoxy`, `textcolor`, etc.)
- **DOS**: File system functions (`fileexists`, `findfirst`, `findnext`, etc.)
- **strings**: String manipulation functions (`strcat`, `strcopy`, `strcomp`, `strlen`, etc.) - lowercase as deliberate TP departure

### Advanced Features
- **Built-in Functions**: `succ()`, `pred()`, `ord()`, `chr()`, string functions
- **Labels & GOTO**: Full support for structured and unstructured control flow
- **Forward Declarations**: Procedure and function forward declarations
- **Nested Procedures**: Local procedure and function definitions
- **Parameter Types**: Value parameters, var parameters, const parameters

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler (Visual Studio 2019+, GCC 7+, Clang 5+)
- Windows: Visual Studio Build Tools or Visual Studio IDE

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/daver64/rpascal.git
cd rpascal

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release
```

The compiler executable will be created as `bin/rpascal.exe` on Windows, or `bin/rpascal` on Unix-like systems.

## Usage

### Basic Compilation
```bash
# Compile a Pascal program (Linux/macOS)
./bin/rpascal program.pas

# Compile a Pascal program (Windows - PowerShell or cmd)
bin\rpascal.exe program.pas

# Keep intermediate C++ file for debugging
./bin/rpascal --keep-cpp program.pas

# Specify custom output name (include .exe on Windows if desired)
./bin/rpascal -o myprogram program.pas
# or on Windows
bin\rpascal.exe -o myprogram.exe program.pas

# Verbose output
./bin/rpascal -v program.pas
```

### Command Line Options
- `-o <file>`: Specify output executable name
- `--keep-cpp`: Keep intermediate C++ file after compilation
- `-v`: Verbose output showing compilation steps
- `--tokens`: Show tokenization output (debug)
- `--ast`: Show Abstract Syntax Tree (debug)
- `-h, --help`: Show help message

## Testing

RPascal includes a comprehensive test suite covering major language features:

```bash
# Run individual tests (Linux/macOS)
./bin/rpascal tests/test_arrays.pas
./bin/rpascal tests/test_records.pas
./bin/rpascal tests/test_enumerations.pas

# Run individual tests (Windows - PowerShell or cmd)
bin\rpascal.exe tests\test_arrays.pas
bin\rpascal.exe tests\test_records.pas
bin\rpascal.exe tests\test_enumerations.pas

# Run the full cross-platform test runner included with the repo
# On Unix-like systems (Linux/macOS):
./run_tests.sh

# On Windows (cmd.exe / PowerShell):
run_tests.bat

# The test runner compiles and executes each test; when successful it
# will remove intermediate executables and print PASSED/FAILED status.
```

Current test coverage includes:
- Basic data types and operations
- Control flow constructs
- Procedure and function calls
- Record and array handling
- Enumeration and subrange types
- Pointer operations
- Set operations
- File I/O operations
- Built-in function library
- Memory management

## Architecture

RPascal follows a traditional compiler pipeline:

1. **Lexical Analysis**: Tokenizes Pascal source code
2. **Parsing**: Builds Abstract Syntax Tree (AST) from tokens
3. **Semantic Analysis**: Type checking, symbol table management, scope resolution
4. **Code Generation**: Generates self-contained C++ code with embedded runtime
5. **Native Compilation**: Uses system C++ compiler to produce executable

The generated C++ code is self-contained and includes all necessary Pascal runtime functions, requiring no external Pascal libraries or dependencies.

## Known Limitations

As alpha software, RPascal has several known limitations:

- **Incomplete Language Coverage**: Some advanced Pascal features are not yet implemented
- **Error Handling**: Error messages could be more descriptive and user-friendly  
- **Performance**: Generated code is functional but not yet optimized for performance
- **Platform Support**: Tested on Windows and Linux. The included test runner scripts (`run_tests.bat` and `run_tests.sh`) execute the full test suite; recent runs completed successfully on both platforms with no errors.
- **Memory Management**: Some edge cases in dynamic memory handling may exist
- **Complex Expressions**: Very complex nested expressions may not parse correctly
- **Unit System**: Only basic built-in units are supported; custom units have limitations

## Contributing

RPascal is in active development. Contributions are welcome, especially:

- Bug reports with minimal test cases
- Additional test programs covering edge cases
- Documentation improvements
- Platform compatibility fixes
- Performance optimizations

## License

This project is open source. See LICENSE file for details.

## Development Status

RPascal successfully compiles and runs a significant subset of Turbo Pascal 7 programs. While many core language features are working well, this remains alpha-quality software under active development. Use at your own risk and always test thoroughly before relying on compiled programs for important tasks.

The project aims to eventually provide a reliable, compatible alternative for running legacy Pascal code on modern systems, but is not yet ready for production use.