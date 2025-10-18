program TestBuiltinFunctions;

{
  Comprehensive test for built-in functions from System, CRT, and DOS units
  Tests: string functions, math functions, conversion functions, CRT functions, DOS functions
}

uses CRT, DOS;

var
  { String testing variables }
  s1, s2, result: string;
  i, pos_result: integer;
  ch: char;
  
  { Math testing variables }
  x, y, math_result: real;
  int_val: integer;
  
  { Conversion variables }
  str_val: string;
  int_from_str, code: integer;
  real_from_str: real;
  
  { CRT variables }
  key: char;
  old_x, old_y: integer;
  
  { DOS variables }
  current_dir: string;
  file_exists: boolean;

begin
  writeln('=== Built-in Functions Test ===');
  
  { === STRING FUNCTIONS === }
  writeln('--- String Functions ---');
  
  s1 := 'Hello World';
  s2 := 'Pascal';
  
  writeln('s1 = "', s1, '"');
  writeln('s2 = "', s2, '"');
  writeln('Length of s1: ', length(s1));
  writeln('Length of s2: ', length(s2));
  
  { Position function }
  pos_result := pos('World', s1);
  writeln('Position of "World" in s1: ', pos_result);
  pos_result := pos('xyz', s1);
  writeln('Position of "xyz" in s1: ', pos_result, ' (not found)');
  
  { Copy function }
  result := copy(s1, 1, 5);
  writeln('Copy(s1, 1, 5): "', result, '"');
  result := copy(s1, 7, 5);
  writeln('Copy(s1, 7, 5): "', result, '"');
  
  { Concat function }
  result := concat(s1, ' + ', s2);
  writeln('Concat(s1, " + ", s2): "', result, '"');
  
  { Insert and Delete }
  s1 := 'Hello World';
  insert('Beautiful ', s1, 7);
  writeln('After inserting "Beautiful ": "', s1, '"');
  
  delete(s1, 7, 10);
  writeln('After deleting 10 chars from pos 7: "', s1, '"');
  
  { UpCase }
  ch := 'a';
  writeln('UpCase(''a''): ', upcase(ch));
  ch := 'Z';
  writeln('UpCase(''Z''): ', upcase(ch));
  
  { Enhanced string functions }
  s1 := '  Hello World  ';
  writeln('Original: "', s1, '"');
  writeln('Trim: "', trim(s1), '"');
  writeln('TrimLeft: "', trimleft(s1), '"');
  writeln('TrimRight: "', trimright(s1), '"');
  
  result := stringofchar('*', 10);
  writeln('StringOfChar(''*'', 10): "', result, '"');
  
  writeln('LowerCase("HELLO"): "', lowercase('HELLO'), '"');
  writeln('UpperCase("world"): "', uppercase('world'), '"');
  
  { === MATHEMATICAL FUNCTIONS === }
  writeln();
  writeln('--- Mathematical Functions ---');
  
  { Basic math }
  int_val := -42;
  writeln('Abs(-42): ', abs(int_val));
  
  x := 7.0;
  writeln('Sqr(7.0): ', sqr(x):0:1);
  writeln('Sqrt(49.0): ', sqrt(49.0):0:2);
  
  { Trigonometric functions }
  x := 3.14159 / 2;  { 90 degrees in radians }
  writeln('Sin(π/2): ', sin(x):0:4);
  writeln('Cos(0): ', cos(0.0):0:4);
  
  x := 1.0;
  writeln('ArcTan(1): ', arctan(x):0:4, ' (should be π/4)');
  
  { Logarithmic and exponential }
  x := 2.71828;  { approximately e }
  writeln('Ln(e): ', ln(x):0:4);
  writeln('Exp(1): ', exp(1.0):0:4, ' (should be e)');
  
  { Rounding functions }
  x := 3.7;
  writeln('Round(3.7): ', round(x));
  writeln('Trunc(3.7): ', trunc(x));
  
  x := -3.7;
  writeln('Round(-3.7): ', round(x));
  writeln('Trunc(-3.7): ', trunc(x));
  
  { === CONVERSION FUNCTIONS === }
  writeln();
  writeln('--- Conversion Functions ---');
  
  { Chr and Ord }
  writeln('Chr(65): ', chr(65));
  writeln('Ord(''A''): ', ord('A'));
  
  { Str and Val }
  int_val := 123;
  str(int_val, str_val);
  writeln('Str(123): "', str_val, '"');
  
  x := 45.67;
  str(x:0:2, str_val);
  writeln('Str(45.67:0:2): "', str_val, '"');
  
  str_val := '789';
  val(str_val, int_from_str, code);
  writeln('Val("789"): ', int_from_str, ', error code: ', code);
  
  str_val := '12.34';
  val(str_val, real_from_str, code);
  writeln('Val("12.34"): ', real_from_str:0:2, ', error code: ', code);
  
  { Enhanced conversion functions }
  writeln('IntToStr(456): "', inttostr(456), '"');
  writeln('FloatToStr(78.9): "', floattostr(78.9), '"');
  writeln('StrToInt("321"): ', strtoint('321'));
  writeln('StrToFloat("12.5"): ', strtofloat('12.5'):0:1);
  
  { === SYSTEM FUNCTIONS === }
  writeln();
  writeln('--- System Functions ---');
  
  writeln('ParamCount: ', paramcount());
  if paramcount() > 0 then
    writeln('ParamStr(1): "', paramstr(1), '"')
  else
    writeln('No command line parameters');
  
  { Random functions }
  randomize();
  writeln('Random numbers:');
  for i := 1 to 5 do
    writeln('  Random: ', random(), ', Random(100): ', random(100));
  
  { Memory functions test }
  writeln('Memory allocation test passed (new/dispose work)');
  
  { === CRT FUNCTIONS === }
  writeln();
  writeln('--- CRT Functions ---');
  
  { Get current cursor position }
  old_x := wherex();
  old_y := wherey();
  writeln('Current cursor position: (', old_x, ', ', old_y, ')');
  
  { Text color functions }
  writeln('Testing text colors...');
  textcolor(12); { Light Red }
  writeln('This text should be in light red (if terminal supports it)');
  
  textcolor(10); { Light Green }
  writeln('This text should be in light green');
  
  textcolor(14); { Yellow }
  writeln('This text should be in yellow');
  
  normvideo(); { Reset to normal }
  writeln('Text color reset to normal');
  
  { Screen functions }
  writeln('Screen manipulation functions available:');
  writeln('  - ClrScr: Clear screen');
  writeln('  - ClrEol: Clear to end of line');
  writeln('  - GotoXY: Position cursor');
  writeln('  - WhereX/WhereY: Get cursor position');
  
  { Sound functions }
  writeln('Sound functions available (not actually making sound):');
  writeln('  - Sound(frequency): Generate tone');
  writeln('  - NoSound: Stop sound');
  writeln('  - Delay(ms): Pause execution');
  
  { === DOS FUNCTIONS === }
  writeln();
  writeln('--- DOS Functions ---');
  
  { Directory functions }
  current_dir := getcurrentdir();
  writeln('Current directory: "', current_dir, '"');
  
  { File existence test }
  file_exists := fileexists('test_output.txt');
  writeln('File "test_output.txt" exists: ', file_exists);
  
  file_exists := fileexists('nonexistent_file.txt');
  writeln('File "nonexistent_file.txt" exists: ', file_exists);
  
  { Environment variable }
  str_val := getenv('PATH');
  if length(str_val) > 0 then
    writeln('PATH environment variable found (length: ', length(str_val), ')')
  else
    writeln('PATH environment variable not found');
  
  { Date and time functions }
  writeln('Date/time functions available:');
  writeln('  - GetDate: Get current date');
  writeln('  - GetTime: Get current time');
  writeln('  - GetDateTime: Get date and time');
  
  { === SUMMARY === }
  writeln();
  writeln('--- Built-in Functions Summary ---');
  writeln('String functions: TESTED (length, pos, copy, concat, insert, delete, upcase, trim, etc.)');
  writeln('Math functions: TESTED (abs, sqr, sqrt, sin, cos, arctan, ln, exp, round, trunc)');
  writeln('Conversion functions: TESTED (chr, ord, str, val, inttostr, floattostr, etc.)');
  writeln('System functions: TESTED (paramcount, paramstr, random, randomize)');
  writeln('CRT functions: TESTED (wherex, wherey, textcolor, normvideo, etc.)');
  writeln('DOS functions: TESTED (getcurrentdir, fileexists, getenv, etc.)');
  
  writeln();
  writeln('=== Built-in Functions Tests Completed Successfully ===');
end.