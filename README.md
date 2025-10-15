# RPascal - Turbo Pascal 7 Compatible Compiler

A modern Pascal-to-C++ transpiler that aims for Turbo Pascal 7 compatibility. RPascal converts Pascal source code to C++ and compiles it to native executables using MSVC or MinGW.

## 🎉 **Recent Major Updates** (October 2025)

### ✅ **Complete Record System**
- **Full record type definitions** with proper field parsing
- **Field access expressions** with complete type resolution
- **Nested field access** for complex record structures (`obj.field1.field2`)
- **Record assignment and field assignment** with type checking
- **Mixed field types** (integer, string, real, boolean) fully supported

### ✅ **System Unit Functions**
- **Mathematical functions**: `abs`, `sqr`, `sqrt`, `sin`, `cos`, `arctan`, `ln`, `exp`
- **Type conversion**: `val`, `str` for string/number conversion  
- **String functions**: `upcase` for case conversion
- **Program control**: `halt`, `exit` for program termination
- **Random numbers**: `random`, `randomize` for random number generation
- **Command line**: `paramcount`, `paramstr` for command line access

## Features

### ✅ **Implemented Language Features**
- **Variables**: All basic types (integer, real, boolean, char, string)
- **Constants**: Compile-time constant declarations
- **Type Definitions**: Custom types, records (structs), arrays with ranges
- **Record Types**: **Complete implementation** with field access, nested records, assignment
- **Enumeration Types**: Full enum support with C++ enum class generation
- **Functions & Procedures**: Parameters, return values, local variables, System unit functions
- **Parameter Passing**: By value and by reference (`var` parameters)
- **Control Flow**: if-then-else, while loops, repeat-until loops, for loops, case statements
- **Case Statements**: Multi-value cases, else clause, character and integer expressions
- **Expressions**: Arithmetic, boolean, string operations with **full type checking**
- **Record Access**: **Advanced field access** via dot notation (`record.field`, `obj.field1.field2`)
- **Array Indexing**: Array element access (`array[index]`)
- **Built-in Functions**: `writeln`, `readln`, `length`, `chr`, `ord`, **plus 16 System unit functions**

### 🎯 **Code Generation**
- **Modern C++17 Output**: Clean, readable C++ code generation
- **Standard Library**: Uses `std::string`, `std::array`, `std::cout`, `<cmath>` integration
- **Type Safety**: **Enhanced type mapping and validation** with proper field type resolution
- **Memory Safe**: No manual memory management needed
- **Struct Generation**: **Automatic C++ struct creation** from Pascal record types

### 🔧 **Compiler Pipeline**
- **Lexical Analysis**: Complete tokenization of Pascal source
- **Parsing**: Full AST generation with visitor pattern, **advanced record parsing**
- **Semantic Analysis**: **Robust type checking** and symbol resolution with field validation
- **Code Generation**: C++ output with proper includes and System function mapping
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

### Advanced Record Features
```pascal
program AdvancedRecords;

type
    Point = record
        x, y: integer;
    end;
    
    Employee = record
        id: integer;
        name: string;
        salary: real;
        active: boolean;
    end;
    
    Circle = record
        center: Point;
        radius: real;
    end;

var
    p1, p2: Point;
    emp: Employee;
    c: Circle;

begin
    { Record field assignment }
    p1.x := 5;
    p1.y := 10;
    
    { Record assignment }
    p2 := p1;
    p2.x := p2.x * 2;  { Field access in expressions }
    
    { Mixed field types }
    emp.id := 101;
    emp.name := 'Alice Smith';
    emp.salary := 75000.50;
    emp.active := true;
    
    { Nested record access }
    c.center.x := 100;
    c.center.y := 200;
    c.radius := 15.5;
    
    { System unit functions }
    writeln('Square root of 16: ', sqrt(16));
    writeln('Absolute value: ', abs(-42));
    writeln('Uppercase: ', upcase('hello'));
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

## Current Status & Roadmap

### 🚀 **Current Turbo Pascal 7 Compatibility: ~40-45%**

**Recently Completed (Major milestones)**:
- ✅ **Complete Record System** - Field access, nested records, assignments
- ✅ **System Unit Functions** - 16 core mathematical and utility functions
- ✅ **Advanced Type Checking** - Proper field type resolution and validation

### ⚠️ **Next Priority Features for Full TP7 Compatibility**

#### **Phase 1 - Critical Core Features** (High Priority)
- **Units/Uses System** - Essential for TP7 program compatibility
- **Set Type Operations** - Complete set algebra (union +, intersection *, membership in)
- **String Indexing** - s[1] syntax for character access (very common in TP7)

#### **Phase 2 - Enhanced Type System** (Medium Priority)  
- **Enhanced Enumeration Types** - Complete enum value resolution
- **Subrange Types** - Bounds checking and type validation
- **Enhanced File I/O** - Typed files and random access operations

#### **Phase 3 - Standard Units** (Lower Priority)
- **DOS Unit Functions** - File system and date/time operations
- **CRT Unit Functions** - Screen and keyboard control
- **Advanced String Functions** - Complete string manipulation library

### 🔍 **Current Limitations**

#### **Missing Critical Features**
- **Units/Uses**: No separate compilation or unit dependencies
- **Set Operations**: Can parse sets but no operations (+, -, *, in)
- **String Indexing**: Cannot access individual characters (s[1])
- **Advanced I/O**: Only basic text file operations

#### **Known Issues**
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

### **Immediate Next Steps** (Phase 1)
- [ ] **Units/Uses System** - Separate compilation and unit dependencies
- [ ] **Set Type Operations** - Complete set algebra (+, -, *, in operations)
- [ ] **String Indexing** - Character access via s[1] syntax

### **Medium Term** (Phase 2)
- [ ] **Enhanced Enumeration Types** - Complete enum value resolution
- [ ] **Subrange Types** - Bounds checking and validation
- [ ] **Enhanced File I/O** - Typed files and random access

### **Long Term** (Phase 3)
- [ ] **DOS Unit** - File system operations, date/time functions
- [ ] **CRT Unit** - Screen control and keyboard input
- [ ] **Advanced String Functions** - Complete string manipulation
- [ ] **Enhanced error reporting** with line numbers and context
- [ ] **Performance optimizations** and code generation improvements

---

**Note**: RPascal now provides solid Turbo Pascal 7 compatibility with complete record support and System unit functions. The compiler successfully handles real-world Pascal programs that use records and mathematical operations.
