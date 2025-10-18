program FizzBuzz;
{ Classic FizzBuzz implementation in Pascal
  Prints numbers 1-100, but:
  - "Fizz" for multiples of 3
  - "Buzz" for multiples of 5  
  - "FizzBuzz" for multiples of both 3 and 5
}

var
  i: integer;

begin
  writeln('FizzBuzz - Numbers 1 to 100:');
  writeln();
  
  for i := 1 to 100 do
  begin
    if (i mod 15 = 0) then
      writeln('FizzBuzz')
    else if (i mod 3 = 0) then
      writeln('Fizz')
    else if (i mod 5 = 0) then
      writeln('Buzz')
    else
      writeln(i);
  end;
  
  writeln();
  writeln('Done!');
end.