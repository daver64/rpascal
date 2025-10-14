program Calculator;
var
  a: integer;
  b: integer;
  result: integer;

procedure add(x: integer; y: integer);
begin
  result := x + y;
end;

function multiply(x: integer; y: integer): integer;
begin
  multiply := x * y;
end;

begin
  a := 10;
  b := 20;
  add(a, b);
  writeln('Addition: ', result);
  
  result := multiply(a, b);
  writeln('Multiplication: ', result);
end.