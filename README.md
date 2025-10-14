# RPascal - Turbo Pascal 7 Compatible Compiler

A modern Pascal-to-C++ transpiler that aims for Turbo Pascal 7 compatibility. RPascal converts Pascal source code to C++ and compiles it to native executables using MSVC or MinGW.

## Features

### ✅ **Implemented Language Features**
- **Variables**: All basic types (integer, real, boolean, char, string)
- **Constants**: Compile-time constant declarations
- **Type Definitions**: Custom types, records (structs), arrays with ranges
- **Enumeration Types**: Full enum support with C++ enum class generation
- **Functions & Procedures**: Parameters, return values, local variables
- **Parameter Passing**: By value and by reference (`var` parameters)
- **Control Flow**: if-then-else, while loops, repeat-until loops, case statements
- **Case Statements**: Multi-value cases, else clause, character and integer expressions
- **Expressions**: Arithmetic, boolean, string operations
- **Record Access**: Field access via dot notation (`record.field`)
- **Array Indexing**: Array element access (`array[index]`)
- **Built-in Functions**: `writeln`, `readln`, `length`, `chr`, `ord`

### 🎯 **Code Generation**
- **Modern C++17 Output**: Clean, readable C++ code generation
- **Standard Library**: Uses `std::string`, `std::array`, `std::cout`
- **Type Safety**: Proper type mapping and validation
- **Memory Safe**: No manual memory management needed

### 🔧 **Compiler Pipeline**
- **Lexical Analysis**: Complete tokenization of Pascal source
- **Parsing**: Full AST generation with visitor pattern
- **Semantic Analysis**: Type checking and symbol resolution
- **Code Generation**: C++ output with proper includes
- **Native Compilation**: MSVC (primary) or MinGW compilation

## Installation & Usage

### Prerequisites
- **Windows** with Visual Studio Build Tools or Visual Studio
- **CMake** 3.15 or later
- **MSVC** compiler (recommended) or MinGW-w64

### Building
```bash
git clone https://github.com/daver64/rpascal.git
cd rpascal
cmake -B build -S .
cmake --build build --config Release
```

### Usage
```bash
# Compile to executable (default behavior)
bin/rpascal hello.pas

# Specify output executable name
bin/rpascal hello.pas -o myprogram.exe

# Keep intermediate C++ file
bin/rpascal hello.pas --keep-cpp

# Show debug information
bin/rpascal --tokens --ast -v hello.pas
```

### Command Line Options
- `-o <file>`: Specify output executable (default: `<input>.exe`)
- `--keep-cpp`: Keep intermediate C++ file after compilation
- `-v`: Verbose output
- `--tokens`: Show tokenization output
- `--ast`: Show Abstract Syntax Tree
- `-h, --help`: Show help message

## Examples

### Hello World
```pascal
program HelloWorld;
begin
    writeln('Hello, World!');
end.
```

### Advanced Features
```pascal
program Demo;

const
    MaxSize = 100;
    PI = 3.14159;

type
    TPoint = record
        x, y: integer;
    end;
    TArray = array[1..5] of integer;
    TColor = (Red, Green, Blue, Yellow);

var
    point: TPoint;
    numbers: TArray;
    color: TColor;
    grade: char;
    i: integer;

procedure ShowPoint(p: TPoint);
begin
    writeln('Point: (', p.x, ', ', p.y, ')');
end;

function Add(a, b: integer): integer;
begin
    Add := a + b;
end;

begin
    { Records and arrays }
    point.x := 10;
    point.y := 20;
    ShowPoint(point);
    
    numbers[1] := 100;
    writeln('First element: ', numbers[1]);
    writeln('Sum: ', Add(5, 3));
    
    { Enumeration types }
    color := Red;
    writeln('Color value: ', ord(color));
    
    { Case statements }
    grade := 'B';
    case grade of
        'A': writeln('Excellent');
        'B': writeln('Good');
        'C': writeln('Average');
        'D', 'F': writeln('Poor')
    else
        writeln('Invalid grade')
    end;
    
    { Repeat-until loops }
    i := 1;
    repeat
        writeln('Count: ', i);
        i := i + 1;
    until i > 3;
end.
```

## Project Structure
```
rpascal/
├── src/           # Source code
│   ├── lexer/     # Tokenization
│   ├── parser/    # AST generation
│   ├── semantic/  # Type checking
│   └── codegen/   # C++ generation
├── include/       # Header files
├── examples/      # Example Pascal programs
├── tests/         # Test suite
└── bin/           # Compiled executable
```

## Current Limitations

### ⚠️ **Not Yet Implemented**
- **Advanced Control Flow**: for loops
- **Advanced Types**: Sets, files, pointers, variant records, advanced array types
- **Units/Modules**: Uses clause, unit system
- **Advanced Functions**: Function overloading, nested functions
- **Object-Oriented**: Classes, inheritance (Turbo Pascal 5.5+ features)
- **Inline Assembly**: asm blocks
- **Advanced I/O**: File operations, formatted I/O

### 🔍 **Known Issues**
- Limited error reporting detail
- Some edge cases in type conversion
- Arrays are 0-based in C++ vs 1-based in Pascal (runtime offset)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new features
4. Ensure all tests pass
5. Submit a pull request

## License

See [LICENSE](LICENSE) file for details.

## Roadmap

- [ ] Enhanced error reporting with line numbers
- [ ] Array types with proper bounds checking
- [ ] For loops and additional control structures
- [ ] File I/O operations
- [ ] Set types and operations
- [ ] Unit system support
- [ ] More comprehensive test suite
- [ ] Performance optimizations

---

**Note**: This is an experimental compiler focused on core Pascal language features. Full Turbo Pascal 7 compatibility is a work in progress.
