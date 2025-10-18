program TestArrays;

{
  Comprehensive test for array types
  Tests: single-dimensional arrays, multi-dimensional arrays, different bounds, array assignment
}

type
  { Single-dimensional arrays }
  TIntArray = array[1..10] of integer;
  TCharArray = array[0..9] of char;
  TRealArray = array[-5..5] of real;
  
  { Multi-dimensional arrays }
  TMatrix = array[1..3, 1..3] of integer;
  TCube = array[0..2, 0..2, 0..2] of real;
  TBoard = array[1..8, 1..8] of char;
  
  { Array of records }
  TPoint = record
    x, y: integer;
  end;
  TPointArray = array[1..5] of TPoint;

var
  { Single-dimensional arrays }
  intArr: TIntArray;
  charArr: TCharArray;
  realArr: TRealArray;
  
  { Multi-dimensional arrays }
  matrix: TMatrix;
  cube: TCube;
  board: TBoard;
  
  { Array of records }
  points: TPointArray;
  tempPoint: TPoint;
  
  { Loop variables }
  i, j, k: integer;
  sum: integer;
  avg: real;
  ch: char;

begin
  writeln('=== Array Types Test ===');
  
  { === SINGLE-DIMENSIONAL ARRAY TESTS === }
  writeln('--- Single-Dimensional Array Tests ---');
  
  { Initialize integer array [1..10] }
  writeln('Initializing integer array [1..10]...');
  for i := 1 to 10 do
    intArr[i] := i * i;
  
  write('Integer array: ');
  for i := 1 to 10 do
  begin
    write(intArr[i]);
    if i < 10 then write(', ');
  end;
  writeln();
  
  { Calculate sum }
  sum := 0;
  for i := 1 to 10 do
    sum := sum + intArr[i];
  writeln('Sum of squares 1-10: ', sum);
  
  { Initialize character array [0..9] }
  writeln();
  writeln('Initializing character array [0..9]...');
  charArr[0] := 'A';
  charArr[1] := 'B';
  charArr[2] := 'C';
  charArr[3] := 'D';
  charArr[4] := 'E';
  charArr[5] := 'F';
  charArr[6] := 'G';
  charArr[7] := 'H';
  charArr[8] := 'I';
  charArr[9] := 'J';
  
  write('Character array: ');
  for i := 0 to 9 do
  begin
    write(charArr[i]);
    if i < 9 then write(', ');
  end;
  writeln();
  
  { Initialize real array [-5..5] }
  writeln();
  writeln('Initializing real array [-5..5]...');
  for i := -5 to 5 do
    realArr[i] := i * 0.5;
  
  write('Real array: ');
  for i := -5 to 5 do
  begin
    write(realArr[i]:0:1);
    if i < 5 then write(', ');
  end;
  writeln();
  
  { === MULTI-DIMENSIONAL ARRAY TESTS === }
  writeln();
  writeln('--- Multi-Dimensional Array Tests ---');
  
  { Initialize 3x3 matrix }
  writeln('Initializing 3x3 matrix...');
  for i := 1 to 3 do
    for j := 1 to 3 do
      matrix[i, j] := i * 10 + j;
  
  writeln('Matrix:');
  for i := 1 to 3 do
  begin
    write('  Row ', i, ': ');
    for j := 1 to 3 do
    begin
      write(matrix[i, j]:4);
    end;
    writeln();
  end;
  
  { Matrix operations }
  sum := 0;
  for i := 1 to 3 do
    for j := 1 to 3 do
      sum := sum + matrix[i, j];
  writeln('Sum of all matrix elements: ', sum);
  
  { Initialize 3x3x3 cube }
  writeln();
  writeln('Initializing 3x3x3 cube...');
  for i := 0 to 2 do
    for j := 0 to 2 do
      for k := 0 to 2 do
        cube[i, j, k] := i + j * 0.1 + k * 0.01;
  
  writeln('Cube layers:');
  for i := 0 to 2 do
  begin
    writeln('Layer ', i, ':');
    for j := 0 to 2 do
    begin
      write('  ');
      for k := 0 to 2 do
        write(cube[i, j, k]:6:2);
      writeln();
    end;
  end;
  
  { Calculate cube average }
  avg := 0.0;
  for i := 0 to 2 do
    for j := 0 to 2 do
      for k := 0 to 2 do
        avg := avg + cube[i, j, k];
  avg := avg / 27.0;
  writeln('Average of all cube elements: ', avg:0:4);
  
  { === CHESS BOARD PATTERN === }
  writeln();
  writeln('--- Chess Board Pattern (8x8) ---');
  
  for i := 1 to 8 do
    for j := 1 to 8 do
    begin
      if (i + j) mod 2 = 0 then
        board[i, j] := '#'
      else
        board[i, j] := '.';
    end;
  
  writeln('Chess board pattern:');
  for i := 1 to 8 do
  begin
    write('  ');
    for j := 1 to 8 do
      write(board[i, j], ' ');
    writeln();
  end;
  
  { === ARRAY OF RECORDS === }
  writeln();
  writeln('--- Array of Records Tests ---');
  
  writeln('Initializing array of points...');
  { Use temporary variable to work around array-of-record field access issues }
  for i := 1 to 5 do
  begin
    tempPoint.x := i * 10;
    tempPoint.y := i * 20;
    points[i] := tempPoint;
  end;
  
  writeln('Points array:');
  for i := 1 to 5 do
  begin
    tempPoint := points[i];
    writeln('  Point[', i, ']: (', tempPoint.x, ', ', tempPoint.y, ')');
  end;
  
  { === ARRAY ASSIGNMENT TESTS === }
  writeln();
  writeln('--- Array Assignment Tests ---');
  
  { Test array bounds }
  writeln('Testing array bounds...');
  writeln('intArr bounds: 1 to 10');
  writeln('  intArr[1] = ', intArr[1]);
  writeln('  intArr[10] = ', intArr[10]);
  
  writeln('charArr bounds: 0 to 9');
  writeln('  charArr[0] = ', charArr[0]);
  writeln('  charArr[9] = ', charArr[9]);
  
  writeln('realArr bounds: -5 to 5');
  writeln('  realArr[-5] = ', realArr[-5]:0:1);
  writeln('  realArr[5] = ', realArr[5]:0:1);
  
  { === ARRAY SEARCH AND MANIPULATION === }
  writeln();
  writeln('--- Array Search and Manipulation ---');
  
  { Find maximum in integer array }
  j := intArr[1];
  k := 1;
  for i := 2 to 10 do
  begin
    if intArr[i] > j then
    begin
      j := intArr[i];
      k := i;
    end;
  end;
  writeln('Maximum value in intArr: ', j, ' at index ', k);
  
  { Reverse character array }
  write('Original charArr: ');
  for i := 0 to 9 do
    write(charArr[i]);
  writeln();
  
  for i := 0 to 4 do
  begin
    ch := charArr[i];
    charArr[i] := charArr[9 - i];
    charArr[9 - i] := ch;
  end;
  
  write('Reversed charArr: ');
  for i := 0 to 9 do
    write(charArr[i]);
  writeln();
  
  writeln();
  writeln('=== Array Tests Completed Successfully ===');
end.