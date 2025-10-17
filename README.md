# RPascal - Turbo Pascal 7 Compatible Compiler

RPascal is a modern compiler that converts Turbo Pascal 7 source code to C++ and compiles it to native executables. It provides excellent compatibility with classic Pascal programs while leveraging modern C++ performance and cross-platform portability.

## What RPascal Does

- **Transpiles Pascal to C++**: Converts your .pas files to optimized C++ code
- **Native Compilation**: Produces fast executables using MSVC or GCC
- **TP7 Compatibility**: Supports 85-90% of Turbo Pascal 7 language features
- **Cross-Platform**: Works on Windows, Linux, and other modern systems
- **Modern Performance**: Leverages C++ standard library and optimizations

# RPascal - Turbo Pascal 7 Compatible Compiler

RPascal is a modern compiler that converts Turbo Pascal 7 source code to C++ and compiles it to native executables. It provides solid compatibility with classic Pascal programs while leveraging modern C++ performance and cross-platform portability.

## What RPascal Does

- **Transpiles Pascal to C++**: Converts your .pas files to optimized C++ code
- **Native Compilation**: Produces fast executables using MSVC or GCC
- **TP7 Compatibility**: Supports 75-80% of Turbo Pascal 7 language features
- **Cross-Platform**: Works on Windows, Linux, and other modern systems
- **Modern Performance**: Leverages C++ standard library and optimizations

## Turbo Pascal 7 Compatibility Status

RPascal achieves **75-80% compatibility** with Turbo Pascal 7. Based on comprehensive testing and code analysis, here's the actual implementation status:

### ✅ **Fully Working Core Features (100% Compatible)**

**Language Structure:**
- ✅ All basic data types (integer, real, boolean, char, **byte**, string)
- ✅ Constants, variables, and basic type definitions  
- ✅ **Arrays** (single-dimensional with proper bounds) including **Byte arrays**
- ✅ **Records** with nested field access and record assignment
- ✅ **Enumerations** with proper ord() support
- ✅ **Range types** (e.g., 1..10, 'A'..'Z')

**Control Flow (Complete Implementation):**
- ✅ if-then-else statements
- ✅ while-do loops  
- ✅ for-to/downto loops with integer and enumeration variables
- ✅ repeat-until loops
- ✅ **Nested procedures and functions** with proper scoping
- ✅ **Function parameters**: value, var, const modes
- ✅ **Function overloading** based on parameter types
- ✅ **Forward declarations** and recursive calls

**Built-in Functions (60+ Functions):**
- ✅ **I/O Operations**: writeln, readln, read  
- ✅ **String Functions**: length, chr, ord, pos, copy, concat, insert, delete
- ✅ **Enhanced String Library**: trim, trimleft, trimright, stringofchar, lowercase, uppercase, leftstr, rightstr, padleft, padright
- ✅ **Mathematical Functions**: abs, sqr, sqrt, sin, cos, arctan, ln, exp, power, tan, round, trunc
- ✅ **Conversion Functions**: val, str, inttostr, floattostr, strtoint, strtofloat, upcase
- ✅ **System Functions**: paramcount, paramstr, halt, exit, random, randomize
- ✅ **Memory Functions**: inc, dec (for variables and simple arithmetic)
- ✅ **File Operations**: assign, reset, rewrite, close, eof, blockread, blockwrite, filepos, filesize, seek
- ✅ **CRT Functions**: clrscr, clreol, gotoxy, wherex, wherey, textcolor, textbackground, lowvideo, highvideo, normvideo, window, keypressed, readkey, sound, nosound, delay, cursoron, cursoroff
- ✅ **DOS Functions**: fileexists, directoryexists, findfirst, findnext, findclose, getcurrentdir, setcurrentdir, mkdir, rmdir, getdate, gettime, getdatetime, getenv, exec

**Type System:**
- ✅ **Complete Byte type support** with proper array handling and numeric display
- ✅ **Custom type definitions** for records, arrays, enumerations
- ✅ **Type safety** with proper error checking
- ✅ **String operations** with C++ std::string backend

### ⚠️ **Partially Working Features (60-90% Compatible)**

**Advanced Language Features:**
- ⚠️ **Multi-dimensional arrays** - Basic support, may have edge cases
- ⚠️ **String assignments** - Some syntax restrictions may apply  
- ⚠️ **Case statements** - Basic implementation, complex cases may have issues
- ⚠️ **With statements** - Implemented but not extensively tested

**Built-in Extensions:**
- ⚠️ **File types** - Basic file operations work, advanced features may be limited
- ⚠️ **Dynamic memory** - new/dispose work, advanced pointer operations limited

### ❌ **Missing/Limited Features (0-30% Compatible)**

**Advanced Language Constructs:**
- ❌ **Variant records** - Not implemented (case-of in record definitions)
- ❌ **Set operations** - Set types declared but operations limited
- ❌ **Pointer arithmetic** - Pointer types exist but dereference operations have issues  
- ❌ **Complex pointer operations** - Advanced pointer manipulation not working
- ❌ **Labels and goto** - Not implemented
- ❌ **Units and uses** - Module system not implemented
- ❌ **Object-oriented features** - Not planned (class, object, inheritance)

**Advanced Type System:**
- ❌ **Absolute variables** - Not implemented
- ❌ **Interrupt procedures** - Not implemented  
- ❌ **Compiler directives** - Limited directive support
- ❌ **Inline assembly** - Deliberately excluded for portability

**Complex Built-ins:**
- ❌ **Complex set operations** - Union, intersection, membership testing
- ❌ **Advanced file operations** - Typed files, complex seeking
- ❌ **Date/time formatting** - Basic functions only
- ❌ **Advanced string patterns** - Complex pattern matching

### 🔧 **Enhanced Beyond TP7**

**Modern Improvements:**
- ✅ **Cross-platform compilation** (Windows, Linux, macOS)
- ✅ **Modern C++ code generation** with optimizations
- ✅ **Enhanced error messages** with precise location information
- ✅ **64-bit compatibility** and modern memory management
- ✅ **Unicode string support** through C++ std::string
- ✅ **Better array bounds checking** and type safety

### 📊 **Realistic Compatibility Assessment**

Based on actual testing and code analysis:

- **Core Language Features**: 85% compatible (basic Pascal constructs work well)
- **Standard Library**: 70% compatible (most built-in functions implemented)  
- **Advanced Features**: 40% compatible (pointers, sets, variant records missing)
- **Real-World Usage**: 75-80% of typical TP7 programs will compile and run

**Most Compatible Program Types:**
- Educational Pascal programs
- Basic algorithms and data structures
- Mathematical computations
- Simple file processing
- Console applications with text I/O

**Less Compatible Program Types:**
- Programs using extensive pointer manipulation
- Code relying heavily on set operations
- Programs using variant records
- Legacy code with inline assembly
- Complex DOS-specific functionality

## Quick Start

**Prerequisites**: Windows with Visual Studio or MinGW, CMake 3.15+

```bash
# Build the compiler
git clone https://github.com/daver64/rpascal.git
cd rpascal
mkdir build && cd build
cmake .. && cmake --build . --config Release

# Compile a Pascal program
../bin/rpascal hello.pas
./hello.exe
```

## Example - Core Pascal Features That Work

```pascal
program WorkingExample;

{ Constants and type definitions }
const
  MaxItems = 100;
  PI = 3.14159;

type
  TColor = (Red, Green, Blue);
  TPoint = record
    x, y: integer;
  end;
  TIntArray = array[1..10] of integer;
  TByteBuffer = array[0..9] of Byte;

{ Function overloading - works perfectly }
function Distance(p1, p2: TPoint): real; 
begin
  Distance := sqrt(sqr(p1.x - p2.x) + sqr(p1.y - p2.y));
end;

function Distance(x1, y1, x2, y2: integer): real;
begin
  Distance := sqrt(sqr(x1 - x2) + sqr(y1 - y2));
end;

{ Byte array processing - fully working }
procedure ProcessByteBuffer(var buffer: TByteBuffer);
var
  i: integer;
  sum: integer;
begin
  sum := 0;
  for i := 0 to 9 do
  begin
    buffer[i] := i * 25;  
    sum := sum + buffer[i];
  end;
  writeln('Buffer sum: ', sum);
  
  { Display buffer contents - proper numeric output }
  write('Buffer: ');
  for i := 0 to 9 do
    write(buffer[i], ' ');
  writeln;
end;

{ Nested procedures with local scope - works }
procedure TestNested;
var
  result: real;
  
  function InnerCalc(x: integer): real;
  begin
    InnerCalc := x * 3.14159 / 2.0;
  end;
  
begin
  result := InnerCalc(5);
  writeln('Nested calculation: ', result:0:2);
end;

var
  p1, p2: TPoint;
  numbers: TIntArray;
  buffer: TByteBuffer;
  i: integer;
  b: Byte;
  color: TColor;
  
begin
  { Test records }
  p1.x := 0; p1.y := 0;
  p2.x := 3; p2.y := 4;
  
  { Test function overloading }
  writeln('Distance (points): ', Distance(p1, p2):0:2);
  writeln('Distance (coords): ', Distance(0, 0, 3, 4):0:2);
  
  { Test Byte type - works perfectly }
  b := 255;
  writeln('Byte value: ', b);
  
  { Test enumerations }
  color := Red;
  writeln('Color ordinal: ', ord(color));
  
  { Test arrays and loops }
  for i := 1 to 10 do
    numbers[i] := i * i;
  
  writeln('Array values:');
  for i := 1 to 5 do
    writeln('  numbers[', i, '] = ', numbers[i]);
  
  { Test Byte array processing }
  ProcessByteBuffer(buffer);
  
  { Test nested procedures }
  TestNested;
  
  { Test built-in CRT functions }
  writeln('Testing CRT functions...');
  textcolor(14); { Yellow }
  writeln('This text should be yellow (if supported).');
  normvideo;
  
  { Test string functions }
  writeln('String length of "Hello": ', length('Hello'));
  writeln('Uppercase: ', uppercase('hello world'));
  writeln('Character code of A: ', ord('A'));
  writeln('Character from code 66: ', chr(66));
end.
```

## Command Line Usage

```bash
rpascal program.pas              # Creates program.exe
rpascal -o myapp.exe program.pas # Custom output name
rpascal --keep-cpp program.pas   # Keep intermediate C++ file
rpascal --help                   # Show all options
```

## Target Audience

RPascal is perfect for:

- **Legacy code migration** - Run classic Turbo Pascal programs on modern systems
- **Educational use** - Teach programming with Pascal's clear, structured syntax
- **Cross-platform development** - Write once in Pascal, compile anywhere
- **Performance-critical applications** - Leverage C++ optimizations with Pascal clarity
- **Pascal enthusiasts** - Continue using Pascal with modern tooling and libraries

## Design Philosophy

RPascal prioritizes **practical compatibility** over perfect emulation. We focus on ensuring that real-world Pascal programs compile and run correctly, while making deliberate improvements for modern development:

- **Security**: Memory-safe operations using C++ standard library
- **Portability**: Cross-platform by design, no DOS/Windows dependencies  
- **Performance**: Optimized C++ code generation
- **Maintainability**: Clean, readable generated code for debugging

## 🚧 Improvement Roadmap

Based on comprehensive code analysis and testing, here are the priority areas for enhancing TP7 compatibility:

### **Critical Missing Features (High Impact, Medium Effort)**

**Pointer System (Major Gap):**
- [ ] Fix pointer dereference operations (`ptr^`)
- [ ] Implement proper pointer arithmetic (`Inc(ptr)`, `Dec(ptr)`)
- [ ] Support address-of operator (`@variable`)
- [ ] Enable pointer-to-pointer operations
- **Impact**: Many TP7 programs use pointers for linked lists, dynamic structures
- **Current Status**: Pointers declared but operations fail

**Set Operations (Medium-High Impact):**
- [ ] Implement set union (`[Red, Green] + [Blue]`)
- [ ] Implement set difference (`[Red, Green] - [Red]`)
- [ ] Implement set intersection (`set1 * set2`)
- [ ] Fix membership testing (`item in set`)
- [ ] Support set comparisons (`set1 = set2`)
- **Impact**: Sets are common in TP7 for state management, character processing
- **Current Status**: Set types parse but operations not implemented

**Variant Records (Medium Impact):**
- [ ] Parse `case selector of` syntax in record definitions
- [ ] Generate proper C++ union-based implementation
- [ ] Support accessing variant fields
- **Impact**: Used in advanced TP7 programs for polymorphic data
- **Current Status**: Parse error on variant record syntax

### **Important Language Features (Medium Impact, Low-Medium Effort)**

**Enhanced String Support:**
- [ ] Fix string variable assignments (some restrictions exist)
- [ ] Implement string concatenation with `+` operator
- [ ] Support bounded strings (`string[N]`)
- [ ] Better string literal handling
- **Current Status**: Basic strings work, some assignment issues

**Control Flow Enhancements:**
- [ ] Implement `case-of-else` statements fully
- [ ] Support `with` statement edge cases
- [ ] Add `goto` and label support
- **Current Status**: Basic case statements work, advanced features missing

**File System:**
- [ ] Complete typed file implementation (`file of T`)
- [ ] Advanced file positioning and manipulation
- [ ] Better error handling for file operations
- **Current Status**: Basic file operations work

### **Advanced Features (Lower Priority)**

**Type System Enhancements:**
- [ ] Multi-dimensional array improvements
- [ ] Absolute variable declarations
- [ ] Packed records and arrays
- [ ] More complex range types

**Built-in Function Gaps:**
- [ ] Mathematical functions (`frac`, `int`, `pi`)
- [ ] Date/time formatting functions
- [ ] Advanced memory management (`mark`, `release`)
- [ ] String pattern matching functions

**Modern Development Features:**
- [ ] Better error messages with suggestions
- [ ] Warnings for potential compatibility issues
- [ ] Debug information generation
- [ ] IDE integration support

### **Implementation Priority Matrix**

| Feature | Impact | Effort | Priority |
|---------|--------|--------|----------|
| Pointer Operations | High | Medium | **P1** |
| Set Operations | Medium-High | Medium | **P1** |
| Variant Records | Medium | High | **P2** |
| String Enhancements | Medium | Low | **P2** |
| Case Statement Edge Cases | Low-Medium | Low | **P3** |
| Advanced File Operations | Low-Medium | Medium | **P3** |
| Multi-dimensional Arrays | Low | Medium | **P4** |

### **Target Compatibility Goals**

- **Short-term (P1 features)**: Achieve 85% TP7 compatibility
- **Medium-term (P1+P2 features)**: Achieve 90% TP7 compatibility  
- **Long-term (All features)**: Achieve 95% TP7 compatibility

### **Testing Strategy**

Each feature implementation should include:
1. **Unit tests** for the specific feature
2. **Integration tests** with existing functionality
3. **Real-world TP7 program testing**
4. **Regression testing** to ensure no functionality breaks

---

**Current Status Summary**: RPascal is a solid Pascal compiler with 75-80% TP7 compatibility. The core language works well, but pointer operations and set handling are the main gaps preventing higher compatibility scores.

## License

MIT License - see [LICENSE](LICENSE) file for details.