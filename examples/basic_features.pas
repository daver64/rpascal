{ Test 1: Basic features we know work }
program BasicTest;
var
  a, b: integer;
  message: string;

procedure Test1;
begin
  writeln('Test 1: Basic procedure');
end;

function Test2(x: integer): integer;
begin
  Test2 := x * 2;
end;

begin
  a := 5;
  b := 10;
  writeln('Basic arithmetic: ', a + b);
  Test1;
  writeln('Function result: ', Test2(a));
end.