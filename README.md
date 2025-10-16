# RPascal - Turbo Pascal 7 Compatible Compiler

RPascal is a modern compiler that converts Turbo Pascal 7 source code to C++ and compiles it to native executables. It provides excellent compatibility with classic Pascal programs while leveraging modern C++ performance.

## What RPascal Does

- **Transpiles Pascal to C++**: Converts your .pas files to optimized C++ code
- **Native Compilation**: Produces fast executables using MSVC or GCC
- **TP7 Compatibility**: Supports ~95% of Turbo Pascal 7 language features
- **Cross-Platform**: Works on Windows and Linux

## Current Language Support

RPascal supports nearly all Turbo Pascal 7 features, including:

### Core Language Features ✅
- All basic data types (integer, real, boolean, char, string)
- Constants, variables, and type definitions
- Arrays, records, sets, enumerations, and range types
- **Variant records** with case statements
- Procedures and functions with all parameter modes (value, var, const)
- **Function overloading** based on parameter types
- **Nested procedures and functions** with proper scoping
- **Forward declarations**
- **Labels and goto statements**

### Control Flow ✅
- if-then-else, while, for (to/downto), repeat-until, case
- with statements for record field access
- Proper boolean evaluation and control structures

### Built-in Functions (30+) ✅
- **I/O**: writeln, readln, read
- **String operations**: length, chr, ord, pos, copy, concat, insert, delete, upcase
- **Mathematical**: abs, sqr, sqrt, sin, cos, arctan, ln, exp
- **Conversion**: val, str  
- **System**: halt, exit, random, randomize, paramcount, paramstr
- **File operations**: assign, reset, rewrite, close, eof
- **Memory management**: new, dispose
- **Pointer operations**: @, ^

### Advanced Features ✅
- String indexing with 1-based arrays (S[1])
- Record field access and nested records
- Multi-dimensional arrays
- Pointer arithmetic and memory management
- Complex expressions with proper operator precedence

**Not yet implemented**: Units (partial), object-oriented features, DOS/CRT units, inline assembly.

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

## Example

```pascal
program AdvancedExample;
type
  TPoint = record
    x, y: integer;
  end;
  
  TShape = record
    case kind: (Circle, Rectangle) of
      Circle: (radius: real);
      Rectangle: (width, height: real);
  end;

{ Function overloading }
function Distance(p1, p2: TPoint): real; 
begin
  Distance := sqrt(sqr(p1.x - p2.x) + sqr(p1.y - p2.y));
end;

function Distance(x1, y1, x2, y2: integer): real;
begin
  Distance := sqrt(sqr(x1 - x2) + sqr(y1 - y2));
end;

{ Nested procedures }
procedure TestNested;
var
  result: real;
  
  function InnerCalc(x: integer): real;
  begin
    InnerCalc := x * 3.14;
  end;
  
begin
  result := InnerCalc(5);
  writeln('Nested result: ', result);
end;

var
  p1, p2: TPoint;
  shape: TShape;
begin
  { Test records }
  p1.x := 0; p1.y := 0;
  p2.x := 3; p2.y := 4;
  
  { Test function overloading }
  writeln('Distance (points): ', Distance(p1, p2));
  writeln('Distance (coords): ', Distance(0, 0, 3, 4));
  
  { Test variant records }
  shape.kind := Circle;
  shape.radius := 5.0;
  writeln('Circle radius: ', shape.radius);
  
  { Test nested procedures }
  TestNested;
end.
```

## Command Line Usage

```bash
rpascal program.pas              # Creates program.exe
rpascal -o myapp.exe program.pas # Custom output name
rpascal --keep-cpp program.pas   # Keep intermediate C++ file
rpascal --help                   # Show all options
```

RPascal is perfect for running legacy Pascal code, educational use, or anyone who prefers Pascal's clear syntax but wants modern performance.

## License

MIT License - see [LICENSE](LICENSE) file for details.