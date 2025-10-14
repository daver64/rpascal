program ComprehensiveTest;

{ Test 1: Constants }
const
  MaxItems = 100;
  PI = 3.14159;
  TestMessage = 'Hello from Constants';
  TestChar = 'A';

{ Test 2: Type definitions }
type
  TRange = 1..10;
  TColor = (Red, Green, Blue);
  TPoint = record
    x, y: integer;
  end;
  TArray = array[1..5] of integer;
  TStringArray = array[1..3] of string;

{ Test 3: Variables with different types }
var
  i, j, count: integer;
  x, y: real;
  flag, done: boolean;
  ch: char;
  message: string;
  point: TPoint;
  numbers: TArray;
  names: TStringArray;

{ Test 4: Simple procedure }
procedure ShowMessage(msg: string);
begin
  writeln('Message: ', msg);
end;

{ Test 5: Function with parameters }
function Add(a, b: integer): integer;
begin
  Add := a + b;
end;

{ Test 6: Function with local variables }
function Factorial(n: integer): integer;
var
  result: integer;
begin
  result := 1;
  while n > 1 do
  begin
    result := result * n;
    n := n - 1;
  end;
  Factorial := result;
end;

{ Test 7: Procedure with var parameters }
procedure Swap(var a, b: integer);
var
  temp: integer;
begin
  temp := a;
  a := b;
  b := temp;
end;

{ Main program }
begin
  { Test basic arithmetic }
  i := 10;
  j := 20;
  count := Add(i, j);
  writeln('10 + 20 = ', count);
  
  { Test real numbers }
  x := 3.14;
  y := 2.0;
  writeln('Real division: ', x / y);
  
  { Test boolean operations }
  flag := (i > j);
  done := not flag;
  writeln('10 > 20 is ', flag);
  writeln('Not (10 > 20) is ', done);
  
  { Test string operations }
  message := 'Pascal';
  writeln('String: ', message);
  writeln('Length: ', length(message));
  
  { Test character operations }
  ch := 'Z';
  writeln('Character: ', ch);
  writeln('ASCII value: ', ord(ch));
  
  { Test procedure call }
  ShowMessage('Testing procedures');
  
  { Test function call }
  writeln('Factorial of 5: ', Factorial(5));
  
  { Test if-then-else }
  if i < j then
    writeln('i is less than j')
  else
    writeln('i is not less than j');
  
  { Test while loop }
  count := 1;
  while count <= 3 do
  begin
    writeln('Count: ', count);
    count := count + 1;
  end;
  
  { Test record access }
  point.x := 100;
  point.y := 200;
  writeln('Point: (', point.x, ', ', point.y, ')');
  
  { Test array access }
  numbers[1] := 10;
  numbers[2] := 20;
  numbers[3] := 30;
  writeln('Array: ', numbers[1], ', ', numbers[2], ', ', numbers[3]);
  
  writeln('Comprehensive test completed!');
end.