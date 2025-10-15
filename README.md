# RPascal - Turbo Pascal 7 Compatible Compiler

RPascal is a modern compiler that converts Turbo Pascal 7 source code to C++ and compiles it to native executables. It aims to provide excellent compatibility with classic Pascal programs while leveraging modern C++ performance.

## What RPascal Does

- **Transpiles Pascal to C++**: Converts your .pas files to optimized C++ code
- **Native Compilation**: Produces fast executables using MSVC or GCC
- **TP7 Compatibility**: Supports most Turbo Pascal 7 language features
- **Cross-Platform**: Works on Windows and Linux

## Current Language Support

RPascal currently supports about 75% of Turbo Pascal 7 features, including:

- All basic data types (integer, real, boolean, char, string)
- Arrays, records, sets, and enumerations
- Procedures and functions with parameter passing
- Control flow (if-then-else, while, for, repeat-until, case)
- 25+ built-in functions (writeln, readln, length, ord, chr, mathematical functions, etc.)
- String operations and 1-based indexing (S[1])
- Pointer operations (new, dispose, @, ^)

**Not yet implemented**: Variant records, labels/goto, function overloading, nested procedures, DOS/CRT units, object-oriented features.

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
program Example;
type
  TPoint = record
    x, y: integer;
  end;
var
  point: TPoint;
  numbers: array[1..5] of integer;
  i: integer;
begin
  point.x := 10;
  point.y := 20;
  
  for i := 1 to 5 do
    numbers[i] := i * i;
    
  writeln('Point: (', point.x, ', ', point.y, ')');
  writeln('Square of 3: ', numbers[3]);
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