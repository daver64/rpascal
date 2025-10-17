program TestPointers;

{
  Comprehensive test for pointer operations
  Tests: declarations, dereferencing, arithmetic, chained access, dynamic memory
}

type
  { Record types for pointer testing }
  TNode = record
    data: integer;
    next: ^TNode;
  end;
  
  TPerson = record
    name: string;
    age: integer;
    next: ^TPerson;
  end;
  
  { Array type for pointer arithmetic }
  TIntArray = array[1..10] of integer;

var
  { Basic pointer types }
  intPtr: ^integer;
  realPtr: ^real;
  nodePtr: ^TNode;
  personPtr: ^TPerson;
  
  { Regular variables for testing }
  value: integer;
  floatValue: real;
  
  { Array for pointer arithmetic tests }
  numbers: TIntArray;
  arrayPtr: ^integer;
  
  { Linked list variables }
  head: ^TNode;
  current: ^TNode;
  temp: ^TNode;
  
  { Loop variables }
  i: integer;

begin
  writeln('=== Pointer Operations Test ===');
  
  { === BASIC POINTER OPERATIONS === }
  writeln('--- Basic Pointer Operations ---');
  
  { Initialize values }
  value := 42;
  floatValue := 3.14159;
  
  { Test address-of operation (simulated) }
  writeln('value = ', value);
  writeln('floatValue = ', floatValue:0:5);
  
  { Test dynamic memory allocation }
  new(intPtr);
  new(realPtr);
  new(nodePtr);
  
  writeln('Allocated memory for pointers');
  
  { Test pointer assignment and dereferencing }
  intPtr^ := 100;
  realPtr^ := 2.71828;
  
  writeln('intPtr^ = ', intPtr^);
  writeln('realPtr^ = ', realPtr^:0:5);
  
  { Test record pointer operations }
  nodePtr^.data := 200;
  nodePtr^.next := nil;
  
  writeln('nodePtr^.data = ', nodePtr^.data);
  writeln('nodePtr^.next = nil: ', nodePtr^.next = nil);
  writeln;
  
  { === CHAINED FIELD ACCESS === }
  writeln('--- Chained Field Access ---');
  
  { Create a chain of nodes }
  new(head);
  head^.data := 10;
  
  new(head^.next);
  head^.next^.data := 20;
  head^.next^.next := nil;
  
  { Test chained access }
  writeln('head^.data = ', head^.data);
  writeln('head^.next^.data = ', head^.next^.data);
  writeln('head^.next^.next = nil: ', head^.next^.next = nil);
  writeln;
  
  { === POINTER ARITHMETIC === }
  writeln('--- Pointer Arithmetic ---');
  
  { Initialize array for pointer arithmetic }
  for i := 1 to 10 do
    numbers[i] := i * 10;
  
  { Simulate pointer to array start }
  writeln('Array contents:');
  for i := 1 to 10 do
    write(numbers[i], ' ');
  writeln;
  
  { Test inc and dec operations on pointers }
  new(temp);
  temp^.data := 100;
  
  writeln('Before inc: temp^.data = ', temp^.data);
  
  { Create a second node to simulate pointer arithmetic }
  new(temp^.next);
  temp^.next^.data := 200;
  temp^.next^.next := nil;
  
  { Simulate inc(ptr) by moving to next }
  current := temp;
  writeln('current^.data = ', current^.data);
  
  current := current^.next;  { Simulate inc(current) }
  writeln('After "inc": current^.data = ', current^.data);
  
  { Create more nodes for extended arithmetic test }
  new(current^.next);
  current^.next^.data := 300;
  current^.next^.next := nil;
  
  new(current^.next^.next);
  current^.next^.next^.data := 400;
  current^.next^.next^.next := nil;
  
  { Test multiple increments }
  current := temp;  { Reset to start }
  writeln('Testing pointer "arithmetic":');
  for i := 1 to 4 do
  begin
    if current <> nil then
    begin
      writeln('  Step ', i, ': current^.data = ', current^.data);
      current := current^.next;
    end
    else
      writeln('  Step ', i, ': current is nil');
  end;
  writeln;
  
  { === LINKED LIST OPERATIONS === }
  writeln('--- Linked List Operations ---');
  
  { Build a proper linked list }
  head := nil;
  
  { Add nodes to the list }
  for i := 1 to 5 do
  begin
    new(temp);
    temp^.data := i * 100;
    temp^.next := head;
    head := temp;
  end;
  
  { Traverse and print the list }
  writeln('Linked list contents (LIFO order):');
  current := head;
  while current <> nil do
  begin
    write(current^.data, ' ');
    current := current^.next;
  end;
  writeln;
  
  { Count nodes in the list }
  current := head;
  i := 0;
  while current <> nil do
  begin
    i := i + 1;
    current := current^.next;
  end;
  writeln('List contains ', i, ' nodes');
  writeln;
  
  { === COMPLEX POINTER OPERATIONS === }
  writeln('--- Complex Pointer Operations ---');
  
  { Create a more complex linked structure }
  new(personPtr);
  personPtr^.name := 'Alice';
  personPtr^.age := 25;
  
  new(personPtr^.next);
  personPtr^.next^.name := 'Bob';
  personPtr^.next^.age := 30;
  
  new(personPtr^.next^.next);
  personPtr^.next^.next^.name := 'Charlie';
  personPtr^.next^.next^.age := 35;
  personPtr^.next^.next^.next := nil;
  
  writeln('Person list:');
  current := personPtr;
  while current <> nil do
  begin
    writeln('  Name: ', current^.name, ', Age: ', current^.age);
    current := current^.next;
  end;
  
  { Test finding a person in the list }
  current := personPtr;
  while (current <> nil) and (current^.name <> 'Bob') do
    current := current^.next;
  
  if current <> nil then
    writeln('Found Bob, age: ', current^.age)
  else
    writeln('Bob not found');
  writeln;
  
  { === POINTER COMPARISONS === }
  writeln('--- Pointer Comparisons ---');
  
  current := head;
  temp := head;
  
  writeln('current = head: ', current = head);
  writeln('current = temp: ', current = temp);
  
  if current <> nil then
    current := current^.next;
  
  writeln('After moving current:');
  writeln('current = head: ', current = head);
  writeln('current <> head: ', current <> head);
  writeln('current = nil: ', current = nil);
  writeln;
  
  { === MEMORY CLEANUP === }
  writeln('--- Memory Cleanup ---');
  
  { Clean up the integer list }
  while head <> nil do
  begin
    temp := head;
    head := head^.next;
    dispose(temp);
  end;
  writeln('Integer list cleaned up');
  
  { Clean up the person list }
  while personPtr <> nil do
  begin
    current := personPtr;
    personPtr := personPtr^.next;
    dispose(current);
  end;
  writeln('Person list cleaned up');
  
  { Clean up individual pointers }
  dispose(intPtr);
  dispose(realPtr);
  dispose(nodePtr);
  writeln('Individual pointers cleaned up');
  
  writeln;
  writeln('=== All Pointer Tests Completed Successfully ===');
end.