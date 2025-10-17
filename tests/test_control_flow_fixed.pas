program TestControlFlow;

{
  Comprehensive test for all Pascal control flow constructs
  Tests: if-then-else, while-do, for-to/downto, repeat-until, case statements
}

type
  TGrade = (A, B, C, D, F);
  TColor = (Red, Green, Blue, Yellow, Purple);

var
  i, j, count: integer;
  flag, done: boolean;
  grade: TGrade;
  color: TColor;
  ch: char;

begin
  writeln('=== Control Flow Test ===');
  
  { === IF-THEN-ELSE TESTS === }
  writeln('--- If-Then-Else Tests ---');
  
  { Simple if-then }
  i := 10;
  if i > 5 then
    writeln('i is greater than 5');
  
  { If-then-else }
  if i < 5 then
    writeln('i is less than 5')
  else
    writeln('i is not less than 5');
  
  { Nested if-then-else }
  j := 15;
  if i > j then
    writeln('i > j')
  else if i < j then
    writeln('i < j')
  else
    writeln('i = j');
  
  { Complex boolean expressions }
  if (i > 5) and (j > 10) then
    writeln('Both conditions are true');
  
  if (i < 5) or (j > 10) then
    writeln('At least one condition is true');
  
  if not (i = j) then
    writeln('i and j are not equal');
  writeln();
  
  { === WHILE-DO TESTS === }
  writeln('--- While-Do Tests ---');
  
  { Basic while loop }
  count := 1;
  writeln('Counting from 1 to 5:');
  while count <= 5 do
  begin
    write(count, ' ');
    count := count + 1;
  end;
  writeln();
  
  { While with boolean condition }
  done := false;
  i := 1;
  writeln('Powers of 2 less than 100:');
  while not done do
  begin
    write(i, ' ');
    i := i * 2;
    if i >= 100 then
      done := true;
  end;
  writeln();
  writeln();
  
  { === FOR-TO/DOWNTO TESTS === }
  writeln('--- For-To/Downto Tests ---');
  
  { Basic for-to loop }
  writeln('For i := 1 to 5:');
  for i := 1 to 5 do
    write(i, ' ');
  writeln();
  
  { Basic for-downto loop }
  writeln('For i := 5 downto 1:');
  for i := 5 downto 1 do
    write(i, ' ');
  writeln();
  
  { For loop with enumeration }
  writeln('For grade := A to F:');
  for grade := A to F do
    write(ord(grade), ' ');
  writeln();
  
  { For loop with character range }
  writeln('For ch := ''A'' to ''E'':');
  for ch := 'A' to 'E' do
    write(ch, ' ');
  writeln();
  
  { Nested for loops }
  writeln('Multiplication table (3x3):');
  for i := 1 to 3 do
  begin
    for j := 1 to 3 do
      write(i * j:3);
    writeln();
  end;
  writeln();
  
  { === REPEAT-UNTIL TESTS === }
  writeln('--- Repeat-Until Tests ---');
  
  { Basic repeat-until }
  i := 1;
  writeln('Repeat-until counting to 5:');
  repeat
    write(i, ' ');
    i := i + 1;
  until i > 5;
  writeln();
  
  { Repeat-until with complex condition }
  i := 2;
  writeln('Powers of 2 using repeat-until:');
  repeat
    write(i, ' ');
    i := i * 2;
  until (i > 50) or (i < 0);  { Test for overflow }
  writeln();
  writeln();
  
  { === CASE STATEMENT TESTS === }
  writeln('--- Case Statement Tests ---');
  
  { Case with integer }
  i := 3;
  write('Case ', i, ': ');
  case i of
    1: writeln('One');
    2: writeln('Two');
    3: writeln('Three');
    4, 5: writeln('Four or Five');
  end;
  
  { Case with character }
  ch := 'B';
  write('Grade ', ch, ': ');
  case ch of
    'A': writeln('Excellent');
    'B': writeln('Good');
    'C': writeln('Average');
    'D': writeln('Poor');
    'F': writeln('Fail');
  else
    writeln('Invalid grade');
  end;
  
  { Case with enumeration }
  color := Blue;
  write('Color ', ord(color), ': ');
  case color of
    Red: writeln('Red color');
    Green: writeln('Green color');
    Blue: writeln('Blue color');
    Yellow, Purple: writeln('Yellow or Purple color');
  end;
  
  { Case with ranges }
  i := 15;
  write('Number ', i, ' is ');
  case i of
    1..10: writeln('between 1 and 10');
    11..20: writeln('between 11 and 20');
    21..30: writeln('between 21 and 30');
  else
    writeln('outside range 1-30');
  end;
  writeln();
  
  { === COMPLEX CONTROL FLOW === }
  writeln('--- Complex Control Flow ---');
  
  { Nested loops with exit conditions }
  writeln('Finding first number divisible by 7 in range 10-50:');
  done := false;
  i := 10;
  while (i <= 50) and not done do
  begin
    if i mod 7 = 0 then
    begin
      writeln('Found: ', i);
      done := true;
    end;
    if not done then
      i := i + 1;
  end;
  
  { Complex nested structure }
  writeln('Complex nested example:');
  for i := 1 to 3 do
  begin
    writeln('Outer loop i = ', i);
    j := 1;
    while j <= 3 do
    begin
      if (i + j) mod 2 = 0 then
      begin
        write('  Even sum: ', i, '+', j, '=', i+j);
        case i + j of
          2: writeln(' (smallest even)');
          4: writeln(' (middle even)');
          6: writeln(' (largest even)');
        else
          writeln();
        end;
      end
      else
        writeln('  Odd sum: ', i, '+', j, '=', i+j);
      j := j + 1;
    end;
  end;
  
  writeln();
  writeln('=== All Control Flow Tests Completed Successfully ===');
end.
