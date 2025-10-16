program PointerArithmeticTest;

type
  PInteger = ^integer;
  PNode = ^TNode;
  TNode = record
    data: integer;
    next: PNode;
  end;

var
  value1, value2, value3: integer;
  ptr1, ptr2: PInteger;
  arr: array[0..4] of integer;
  nodePtr: PNode;
  i: integer;

begin
  writeln('=== Pointer Arithmetic Test ===');
  
  { Basic pointer operations }
  value1 := 10;
  value2 := 20;
  value3 := 30;
  
  ptr1 := @value1;
  ptr2 := @value2;
  
  writeln('Value1: ', ptr1^);
  writeln('Value2: ', ptr2^);
  
  { Test Inc and Dec with pointers }
  writeln();
  writeln('Testing Inc/Dec with integers:');
  i := 5;
  writeln('i = ', i);
  inc(i);
  writeln('After inc(i): ', i);
  inc(i, 3);
  writeln('After inc(i, 3): ', i);
  dec(i);
  writeln('After dec(i): ', i);
  dec(i, 2);
  writeln('After dec(i, 2): ', i);
  
  { Test Inc and Dec with simple variables }
  writeln();
  writeln('Testing Inc/Dec with variable references:');
  inc(value1);
  writeln('After inc(value1): ', value1);
  dec(value2);
  writeln('After dec(value2): ', value2);
  
  { Array indexing }
  writeln();
  writeln('Testing array initialization:');
  for i := 0 to 4 do
  begin
    arr[i] := i * 10;
    writeln('arr[', i, '] = ', arr[i]);
  end;
  
  { Dynamic memory allocation }
  writeln();
  writeln('Testing dynamic memory allocation:');
  new(nodePtr);
  nodePtr^.data := 100;
  nodePtr^.next := nil;
  writeln('Node data: ', nodePtr^.data);
  
  { Clean up }
  dispose(nodePtr);
  
  writeln();
  writeln('Pointer arithmetic test completed successfully!');
end.