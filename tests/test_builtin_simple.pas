program TestBuiltinFunctions;

{
  Simplified test for built-in functions (without units)
  Tests: System functions, mathematical functions, string functions
}

var
  i, j, result: integer;
  r1, r2: real;
  s1, s2, s3: string;
  b: boolean;
  c: char;

begin
  writeln('=== Built-in Functions Test (System Only) ===');
  
  { === MATHEMATICAL FUNCTIONS === }
  writeln('--- Mathematical Functions ---');
  
  { Integer functions }
  writeln('abs(-42) = ', abs(-42));
  writeln('sqr(7) = ', sqr(7));
  
  { Real functions }
  r1 := 3.14159;
  writeln('sqrt(16.0) = ', sqrt(16.0):0:2);
  writeln('sin(', r1:0:5, ') = ', sin(r1):0:5);
  writeln('cos(', r1:0:5, ') = ', cos(r1):0:5);
  writeln('ln(2.718) = ', ln(2.718):0:3);
  writeln('exp(1.0) = ', exp(1.0):0:3);
  
  { Conversion functions }
  writeln('round(3.7) = ', round(3.7));
  writeln('trunc(3.7) = ', trunc(3.7));
  
  { === STRING FUNCTIONS === }
  writeln();
  writeln('--- String Functions ---');
  
  s1 := 'Hello';
  s2 := 'World';
  
  writeln('s1 = "', s1, '"');
  writeln('s2 = "', s2, '"');
  writeln('length(s1) = ', length(s1));
  writeln('pos("ll", s1) = ', pos('ll', s1));
  writeln('copy(s1, 2, 3) = "', copy(s1, 2, 3), '"');
  writeln('concat(s1, " ", s2) = "', concat(s1, ' ', s2), '"');
  
  { Character functions }
  c := 'A';
  writeln('ord("', c, '") = ', ord(c));
  writeln('chr(66) = "', chr(66), '"');
  
  { === SYSTEM FUNCTIONS === }
  writeln();
  writeln('--- System Functions ---');
  
  { Random functions }
  randomize;
  writeln('Random numbers:');
  for i := 1 to 5 do
  begin
    r1 := random;
    writeln('  random() = ', r1:0:3);
  end;
  
  { Parameter functions }
  writeln('paramcount = ', paramcount);
  if paramcount > 0 then
    writeln('paramstr(0) = "', paramstr(0), '"');
  
  { === INCREMENT/DECREMENT === }
  writeln();
  writeln('--- Inc/Dec Functions ---');
  
  i := 10;
  writeln('i = ', i);
  inc(i);
  writeln('after inc(i): i = ', i);
  dec(i, 3);
  writeln('after dec(i, 3): i = ', i);
  
  { === MEMORY FUNCTIONS === }
  writeln();
  writeln('--- Memory Functions (Basic) ---');
  writeln('Note: new/dispose require pointers which need more complex testing');
  
  writeln();
  writeln('=== Built-in Functions Test Completed Successfully ===');
end.