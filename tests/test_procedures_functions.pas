program TestProceduresFunctions;

{
  Comprehensive test for procedures and functions
  Tests: declarations, parameters (value, var, const), overloading, forward declarations, recursion
}

type
  TOperation = (Add, Subtract, Multiply, Divide);

var
  globalVar: integer;
  result: real;

{ === FORWARD DECLARATIONS === }
function Factorial(n: integer): integer; forward;
procedure PrintSeparator(ch: char; count: integer); forward;

{ === SIMPLE PROCEDURES === }
procedure SimpleProc;
begin
  writeln('This is a simple procedure with no parameters');
end;

procedure ProcWithValueParams(x, y: integer);
begin
  writeln('Value parameters: x=', x, ', y=', y);
  x := x + 10;  { This won''t affect the caller }
  y := y + 20;
  writeln('Modified in procedure: x=', x, ', y=', y);
end;

procedure ProcWithVarParams(var x, y: integer);
begin
  writeln('Var parameters before: x=', x, ', y=', y);
  x := x + 10;  { This WILL affect the caller }
  y := y + 20;
  writeln('Var parameters after: x=', x, ', y=', y);
end;

procedure ProcWithConstParams(const x: integer; const s: string);
begin
  writeln('Const parameters: x=', x, ', s="', s, '"');
  { x := x + 1; -- This would cause a compile error }
end;

procedure ProcWithMixedParams(value: integer; var variable: integer; const constant: string);
begin
  writeln('Mixed params - value:', value, ' var:', variable, ' const:"', constant, '"');
  value := value * 2;      { Only local change }
  variable := variable * 3; { Changes caller's variable }
end;

{ === SIMPLE FUNCTIONS === }
function SimpleFunc: integer;
begin
  SimpleFunc := 42;
end;

function AddIntegers(a, b: integer): integer;
begin
  AddIntegers := a + b;
end;

function AddReals(a, b: real): real;
begin
  AddReals := a + b;
end;

function IsEven(n: integer): boolean;
begin
  IsEven := (n mod 2) = 0;
end;

function MaxOfThree(a, b, c: integer): integer;
var
  temp: integer;
begin
  temp := a;
  if b > temp then
    temp := b;
  if c > temp then
    temp := c;
  MaxOfThree := temp;
end;

{ === FUNCTION OVERLOADING === }
function Calculate(a, b: integer; op: TOperation): integer; overload;
begin
  case op of
    Add: Calculate := a + b;
    Subtract: Calculate := a - b;
    Multiply: Calculate := a * b;
    Divide: if b <> 0 then Calculate := a div b else Calculate := 0;
  end;
end;

function Calculate(a, b: real; op: TOperation): real; overload;
begin
  case op of
    Add: Calculate := a + b;
    Subtract: Calculate := a - b;
    Multiply: Calculate := a * b;
    Divide: if b <> 0 then Calculate := a / b else Calculate := 0.0;
  end;
end;

function Calculate(a, b: integer): integer; overload;
begin
  Calculate := a + b;  { Default to addition }
end;

{ === NESTED PROCEDURES === }
procedure OuterProcedure(value: integer);
var
  outerLocal: integer;
  
  procedure InnerProcedure(innerParam: integer);
  var
    innerLocal: integer;
  begin
    innerLocal := innerParam * 2;
    outerLocal := outerLocal + innerLocal;  { Access to outer scope }
    globalVar := globalVar + 1;            { Access to global scope }
    writeln('  Inner: param=', innerParam, ', local=', innerLocal, ', outerLocal=', outerLocal);
  end;
  
  function InnerFunction(x: integer): integer;
  begin
    InnerFunction := x * outerLocal;  { Access to outer scope }
  end;
  
begin
  outerLocal := value;
  writeln('Outer: value=', value, ', outerLocal=', outerLocal);
  InnerProcedure(5);
  writeln('After inner call: outerLocal=', outerLocal);
  writeln('Inner function result: ', InnerFunction(3));
end;

{ === RECURSIVE FUNCTIONS === }
function Factorial(n: integer): integer;
begin
  if n <= 1 then
    Factorial := 1
  else
    Factorial := n * Factorial(n - 1);
end;

function Fibonacci(n: integer): integer;
begin
  if n <= 2 then
    Fibonacci := 1
  else
    Fibonacci := Fibonacci(n - 1) + Fibonacci(n - 2);
end;

function GCD(a, b: integer): integer;
begin
  if b = 0 then
    GCD := a
  else
    GCD := GCD(b, a mod b);
end;

{ === PROCEDURES WITH COMPLEX PARAMETERS === }
procedure ProcessArray(var arr: array of integer; size: integer);
var
  i: integer;
begin
  writeln('Processing array of size ', size);
  for i := 0 to size - 1 do
  begin
    arr[i] := arr[i] * 2;
    write(arr[i], ' ');
  end;
  writeln;
end;

procedure PrintSeparator(ch: char; count: integer);
var
  i: integer;
begin
  for i := 1 to count do
    write(ch);
  writeln;
end;

{ === MAIN PROGRAM === }
var
  x, y, z: integer;
  a, b: real;
  numbers: array[1..5] of integer;
  i: integer;

begin
  writeln('=== Procedures and Functions Test ===');
  globalVar := 0;
  
  { === BASIC PROCEDURE TESTS === }
  writeln('--- Basic Procedure Tests ---');
  SimpleProc;
  writeln;
  
  { === PARAMETER PASSING TESTS === }
  writeln('--- Parameter Passing Tests ---');
  
  { Value parameters }
  x := 5;
  y := 10;
  writeln('Before value param call: x=', x, ', y=', y);
  ProcWithValueParams(x, y);
  writeln('After value param call: x=', x, ', y=', y);
  writeln;
  
  { Var parameters }
  x := 5;
  y := 10;
  writeln('Before var param call: x=', x, ', y=', y);
  ProcWithVarParams(x, y);
  writeln('After var param call: x=', x, ', y=', y);
  writeln;
  
  { Const parameters }
  ProcWithConstParams(100, 'Hello World');
  writeln;
  
  { Mixed parameters }
  x := 2;
  y := 3;
  writeln('Before mixed param call: x=', x, ', y=', y);
  ProcWithMixedParams(x, y, 'Test String');
  writeln('After mixed param call: x=', x, ', y=', y);
  writeln;
  
  { === BASIC FUNCTION TESTS === }
  writeln('--- Basic Function Tests ---');
  
  writeln('SimpleFunc() = ', SimpleFunc());
  writeln('AddIntegers(5, 7) = ', AddIntegers(5, 7));
  writeln('AddReals(3.14, 2.86) = ', AddReals(3.14, 2.86):0:2);
  writeln('IsEven(4) = ', IsEven(4));
  writeln('IsEven(7) = ', IsEven(7));
  writeln('MaxOfThree(5, 12, 8) = ', MaxOfThree(5, 12, 8));
  writeln;
  
  { === FUNCTION OVERLOADING TESTS === }
  writeln('--- Function Overloading Tests ---');
  
  writeln('Calculate(10, 5, Add) = ', Calculate(10, 5, Add));
  writeln('Calculate(10, 5, Multiply) = ', Calculate(10, 5, Multiply));
  writeln('Calculate(10, 5) = ', Calculate(10, 5));  { Default overload }
  
  writeln('Calculate(10.5, 3.2, Add) = ', Calculate(10.5, 3.2, Add):0:2);
  writeln('Calculate(10.5, 3.2, Divide) = ', Calculate(10.5, 3.2, Divide):0:2);
  writeln;
  
  { === NESTED PROCEDURE TESTS === }
  writeln('--- Nested Procedure Tests ---');
  writeln('Global var before: ', globalVar);
  OuterProcedure(10);
  writeln('Global var after: ', globalVar);
  writeln;
  
  { === RECURSION TESTS === }
  writeln('--- Recursion Tests ---');
  
  writeln('Factorial tests:');
  for i := 0 to 6 do
    writeln('  ', i, '! = ', Factorial(i));
  
  writeln('Fibonacci sequence:');
  write('  ');
  for i := 1 to 10 do
    write(Fibonacci(i), ' ');
  writeln;
  
  writeln('GCD tests:');
  writeln('  GCD(48, 18) = ', GCD(48, 18));
  writeln('  GCD(100, 25) = ', GCD(100, 25));
  writeln('  GCD(17, 13) = ', GCD(17, 13));
  writeln;
  
  { === COMPLEX PARAMETER TESTS === }
  writeln('--- Complex Parameter Tests ---');
  
  { Initialize array }
  for i := 1 to 5 do
    numbers[i] := i;
  
  write('Array before: ');
  for i := 1 to 5 do
    write(numbers[i], ' ');
  writeln;
  
  { Note: This simulates array parameter passing }
  writeln('Processing array...');
  for i := 1 to 5 do
    numbers[i] := numbers[i] * 2;
  
  write('Array after: ');
  for i := 1 to 5 do
    write(numbers[i], ' ');
  writeln;
  
  writeln;
  PrintSeparator('=', 50);
  writeln('All Procedures and Functions Tests Completed Successfully');
  PrintSeparator('=', 50);
end.