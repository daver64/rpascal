program TurboPascal7CompatibilityTest;

{ 
  Comprehensive test for Turbo Pascal 7 compatibility
  This file tests ALL major Pascal language constructs
  to identify missing features and implementation gaps
}

{ === CONSTANTS === }
const
  MaxItems = 100;
  PI = 3.14159;
  TestString = 'Hello World';
  TestChar = 'A';
  
{ === TYPE DEFINITIONS === }
type
  { Range Types }
  TRange = 1..10;
  TCharRange = 'A'..'Z';
  
  { Enumeration Types }
  TColor = (Red, Green, Blue, Yellow);
  TDirection = (North, South, East, West);
  
  { Set Types }
  TCharSet = set of char;
  TColorSet = set of TColor;
  TRangeSet = set of 1..10;
  
  { Array Types }
  TIntArray = array[1..5] of integer;
  TMultiArray = array[1..3, 1..3] of real;
  TStringArray = array[TColor] of string;
  
  { Record Types }
  TPoint = record
    x, y: integer;
  end;
  
  TPerson = record
    name: string[30];
    age: integer;
    active: boolean;
  end;
  
  { Enumeration for variant records }
  TShapeKind = (Circle, Rectangle, Triangle);
  
  { Variant Records }
  TShape = record
    kind: TShapeKind;
    case kind: TShapeKind of
      Circle: (radius: real);
      Rectangle: (width, height: real);
      Triangle: (side1, side2, side3: real);
  end;
  
  { Pointer Types }
  PInteger = ^integer;
  PNode = ^TNode;
  TNode = record
    data: integer;
    next: PNode;
  end;
  
  { File Types }
  TTextFile = text;
  TIntFile = file of integer;
  TByteFile = file;
  
  { Procedure/Function Types - not yet supported }
  { TIntFunction = function(x: integer): integer; }
  { TProcedure = procedure(var x: integer); }

{ === VARIABLES === }
var
  { Basic types }
  i, j, k: integer;
  x, y, z: real;
  flag, done: boolean;
  ch: char;
  str: string;
  
  { Custom types }
  color: TColor;
  direction: TDirection;
  point: TPoint;
  person: TPerson;
  shape: TShape;
  
  { Arrays }
  numbers: TIntArray;
  matrix: TMultiArray;
  colorNames: TStringArray;
  
  { Sets }
  vowels: TCharSet;
  colors: TColorSet;
  digits: TRangeSet;
  
  { Pointers }
  ptr: PInteger;
  head: PNode;
  
  { Files }
  textFile: TTextFile;
  dataFile: TIntFile;

{ === FORWARD DECLARATIONS === }
procedure TestProcedure(x: integer); forward;
function TestFunction(a, b: integer): integer; forward;

{ === NESTED PROCEDURES/FUNCTIONS === }
procedure OuterProcedure;
var
  localVar: integer;
  
  procedure InnerProcedure;
  var
    innerVar: integer;
  begin
    innerVar := localVar + 1;
        writeln('Inner: ', innerVar);
  end;
  
begin
  localVar := 10;
  InnerProcedure;
end;

{ === PARAMETER PASSING TESTS === }
procedure TestVarParams(var x: integer; y: integer);
begin
  x := x + y;  { x is passed by reference }
end;

procedure TestConstParams(const x: string; var result: integer);
begin
  result := length(x);
end;

{ === FUNCTION OVERLOADING (TP 7.0+ feature) === }
function Add(a, b: integer): integer; overload;
begin
  Add := a + b;
end;

function Add(a, b: real): real; overload;
begin
  Add := a + b;
end;

{ === CONTROL FLOW TESTS === }
procedure TestForLoops;
var
  i: integer;
begin
  { Standard for loop }
  for i := 1 to 10 do
    writeln('Forward: ', i);
    
  { Downto for loop }
  for i := 10 downto 1 do
    writeln('Backward: ', i);
    
  { For loop with custom range }
  for color := Red to Blue do
    writeln('Color: ', ord(color));
end;

procedure TestRepeatUntil;
var
  i: integer;
begin
  i := 1;
  repeat
    writeln('Repeat: ', i);
    i := i + 1;
  until i > 5;
end;

procedure TestCaseStatement;
var
  grade: char;
begin
  grade := 'B';
  case grade of
    'A': writeln('Excellent');
    'B': writeln('Good');
    'C': writeln('Average');
    'D', 'F': writeln('Poor');
  else
    writeln('Invalid grade');
  end;
end;

procedure TestWithStatement;
begin
  with point do
  begin
    x := 10;
    y := 20;
  end;
  
  with person do
  begin
    name := 'John Doe';
    age := 30;
    active := true;
  end;
end;

{ === SET OPERATIONS === }
procedure TestSets;
begin
  vowels := ['a', 'e', 'i', 'o', 'u'];
  colors := [Red, Green];
  digits := [1, 3, 5, 7, 9];
  
  { Set operations }
  if 'a' in vowels then
    writeln('a is a vowel');
    
  colors := colors + [Blue];      { Union }
  colors := colors - [Red];       { Difference }
  colors := colors * [Green, Blue]; { Intersection }
  
  if colors = [Green, Blue] then
    writeln('Set comparison works');
end;

{ === STRING FUNCTIONS === }
procedure TestStringFunctions;
var
  s1, s2, result: string;
  pos: integer;
begin
  s1 := 'Hello';
  s2 := 'World';
  
  { Built-in string functions }
  result := concat(s1, ' ', s2);
  writeln('Concat: ', result);
  
  pos := Pos('ll', s1);
  writeln('Position of "ll": ', pos);
  
  result := Copy(s1, 1, 4);
  writeln('Copy: ', result);
  
  Delete(s1, 1, 6);
  writeln('After delete: ', s1);
  
  Insert('Hi ', s1, 1);
  writeln('After insert: ', s1);
end;

{ === POINTER OPERATIONS === }
procedure TestPointers;
var
  value: integer;
  newNode: PNode;
begin
  value := 42;
  ptr := @value;           { Address operator }
  writeln('Value: ', ptr^); { Dereference operator }
  
  { Dynamic memory allocation }
  new(newNode);
  newNode^.data := 100;
  newNode^.next := nil;
  
  { Linked list operations }
  if head = nil then
    head := newNode
  else
    newNode^.next := head;
    
  dispose(newNode);
end;

{ === FILE OPERATIONS === }
procedure TestFileOperations;
var
  f: text;
  line: string;
  num: integer;
begin
  { Text file operations }
  assign(f, 'test.txt');
  rewrite(f);
  writeln(f, 'Hello file!');
  close(f);
  
  reset(f);
  while not eof(f) do
  begin
    readln(f, line);
    writeln('Read: ', line);
  end;
  close(f);
  
  { Binary file operations }
  assign(dataFile, 'data.dat');
  rewrite(dataFile);
  for num := 1 to 10 do
    write(dataFile, num);
  close(dataFile);
end;

{ === LABELS AND GOTO === }
procedure TestLabelsGoto;
label
  Start, Error, Finish;
var
  i: integer;
begin
Start:
  i := 1;
  
  while i <= 10 do
  begin
    if i = 5 then
      goto Error;
    writeln(i);
    i := i + 1;
  end;
  
  goto Finish;
  
Error:
  writeln('Error at i = 5');
  
Finish:
  writeln('Finished');
end;

{ === IMPLEMENTATION OF FORWARD DECLARATIONS === }
procedure TestProcedure(x: integer);
begin
  writeln('Test procedure called with: ', x);
end;

function TestFunction(a, b: integer): integer;
begin
  TestFunction := a * b;
end;

{ === MAIN PROGRAM === }
begin
  writeln('=== Turbo Pascal 7 Compatibility Test ===');
  
  { Test basic operations }
  i := 10;
  j := 20;
  k := TestFunction(i, j);
  writeln('Function result: ', k);
  
  { Test var parameters }
  TestVarParams(i, j);
  writeln('After var param test: ', i);
  
  { Test control flow }
  writeln('--- Testing For Loops ---');
  TestForLoops;
  
  writeln('--- Testing Repeat-Until ---');
  TestRepeatUntil;
  
  writeln('--- Testing Case Statement ---');
  TestCaseStatement;
  
  writeln('--- Testing With Statement ---');
  TestWithStatement;
  
  writeln('--- Testing Sets ---');
  TestSets;
  
  writeln('--- Testing String Functions ---');
  TestStringFunctions;
  
  writeln('--- Testing Pointers ---');
  TestPointers;
  
  writeln('--- Testing File Operations ---');
  TestFileOperations;
  
  writeln('--- Testing Labels/Goto ---');
  TestLabelsGoto;
  
  writeln('--- Testing Nested Procedures ---');
  OuterProcedure;
  
  writeln('=== Test Complete ===');
end.