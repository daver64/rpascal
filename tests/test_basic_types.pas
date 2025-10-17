program TestBasicTypes;

{
  Comprehensive test for all basic Pascal data types
  Tests: integer, real, boolean, char, byte, string
}

var
  { Integer types }
  i1, i2, i3: integer;
  
  { Real types }
  r1, r2, r3: real;
  
  { Boolean types }
  b1, b2, b3: boolean;
  
  { Character types }
  c1, c2: char;
  
  { Byte types }
  bt1, bt2, bt3: byte;
  
  { String types }
  s1, s2, s3: string;

begin
  writeln('=== Basic Data Types Test ===');
  
  { === INTEGER TESTS === }
  writeln('--- Integer Tests ---');
  i1 := 42;
  i2 := -17;
  i3 := 0;
  
  writeln('i1 = ', i1);
  writeln('i2 = ', i2);
  writeln('i3 = ', i3);
  
  { Integer arithmetic }
  writeln('i1 + i2 = ', i1 + i2);
  writeln('i1 - i2 = ', i1 - i2);
  writeln('i1 * i2 = ', i1 * i2);
  writeln('i1 div i2 = ', i1 div i2);
  writeln('i1 mod 5 = ', i1 mod 5);
  
  { Integer comparisons }
  writeln('i1 > i2: ', i1 > i2);
  writeln('i1 < i2: ', i1 < i2);
  writeln('i1 = i2: ', i1 = i2);
  writeln('i1 <> i2: ', i1 <> i2);
  writeln();
  
  { === REAL TESTS === }
  writeln('--- Real Tests ---');
  r1 := 3.14159;
  r2 := -2.71828;
  r3 := 0.0;
  
  writeln('r1 = ', r1:0:5);
  writeln('r2 = ', r2:0:5);
  writeln('r3 = ', r3:0:1);
  
  { Real arithmetic }
  writeln('r1 + r2 = ', (r1 + r2):0:5);
  writeln('r1 - r2 = ', (r1 - r2):0:5);
  writeln('r1 * r2 = ', (r1 * r2):0:5);
  writeln('r1 / r2 = ', (r1 / r2):0:5);
  
  { Real comparisons }
  writeln('r1 > r2: ', r1 > r2);
  writeln('r1 < r2: ', r1 < r2);
  writeln('r3 = 0.0: ', r3 = 0.0);
  writeln();
  
  { === BOOLEAN TESTS === }
  writeln('--- Boolean Tests ---');
  b1 := true;
  b2 := false;
  
  writeln('b1 = ', b1);
  writeln('b2 = ', b2);
  
  { Boolean operations }
  b3 := b1 and b2;
  writeln('b1 and b2 = ', b3);
  
  b3 := b1 or b2;
  writeln('b1 or b2 = ', b3);
  
  b3 := not b1;
  writeln('not b1 = ', b3);
  
  b3 := b1 xor b2;
  writeln('b1 xor b2 = ', b3);
  
  { Boolean comparisons }
  writeln('b1 = b2: ', b1 = b2);
  writeln('b1 <> b2: ', b1 <> b2);
  writeln();
  
  { === CHARACTER TESTS === }
  writeln('--- Character Tests ---');
  c1 := 'A';
  c2 := 'z';
  
  writeln('c1 = ', c1);
  writeln('c2 = ', c2);
  
  { Character operations }
  writeln('ord(c1) = ', ord(c1));
  writeln('ord(c2) = ', ord(c2));
  writeln('chr(65) = ', chr(65));
  writeln('chr(97) = ', chr(97));
  
  { Character comparisons }
  writeln('c1 < c2: ', c1 < c2);
  writeln('c1 > c2: ', c1 > c2);
  writeln('c1 = ''A'': ', c1 = 'A');
  writeln();
  
  { === BYTE TESTS === }
  writeln('--- Byte Tests ---');
  bt1 := 255;
  bt2 := 0;
  bt3 := 128;
  
  writeln('bt1 = ', bt1);
  writeln('bt2 = ', bt2);
  writeln('bt3 = ', bt3);
  
  { Byte arithmetic }
  writeln('bt1 - bt3 = ', bt1 - bt3);
  writeln('bt3 + bt3 = ', bt3 + bt3);
  writeln('bt1 div 2 = ', bt1 div 2);
  writeln('bt1 mod 10 = ', bt1 mod 10);
  
  { Byte comparisons }
  writeln('bt1 > bt3: ', bt1 > bt3);
  writeln('bt2 = 0: ', bt2 = 0);
  writeln('bt1 = 255: ', bt1 = 255);
  writeln();
  
  { === STRING TESTS === }
  writeln('--- String Tests ---');
  s1 := 'Hello';
  s2 := 'World';
  s3 := '';
  
  writeln('s1 = "', s1, '"');
  writeln('s2 = "', s2, '"');
  writeln('s3 = "', s3, '"');
  
  { String operations }
  writeln('length(s1) = ', length(s1));
  writeln('length(s3) = ', length(s3));
  
  { String concatenation }
  s3 := s1 + ' ' + s2;
  writeln('s1 + " " + s2 = "', s3, '"');
  
  { String indexing }
  writeln('s1[1] = ', s1[1]);
  writeln('s1[length(s1)] = ', s1[length(s1)]);
  
  { String comparisons }
  writeln('s1 = "Hello": ', s1 = 'Hello');
  writeln('s1 < s2: ', s1 < s2);
  writeln('s1 > s2: ', s1 > s2);
  writeln();
  
  { === MIXED TYPE OPERATIONS === }
  writeln('--- Mixed Type Operations ---');
  
  { Integer to Real }
  r1 := i1;  { Implicit conversion }
  writeln('r1 := i1; r1 = ', r1:0:1);
  
  { Real to Integer (explicit) }
  i3 := round(r2);
  writeln('round(r2) = ', i3);
  
  i3 := trunc(r1);
  writeln('trunc(r1) = ', i3);
  
  { Character to Integer }
  i3 := ord(c1);
  writeln('ord(''A'') = ', i3);
  
  { Integer to Character }
  c1 := chr(66);
  writeln('chr(66) = ', c1);
  
  { Byte and Integer interactions }
  i3 := bt1;  { Implicit conversion }
  writeln('i3 := bt1; i3 = ', i3);
  
  bt3 := 100;  { Direct assignment }
  writeln('bt3 := 100; bt3 = ', bt3);
  
  writeln('=== All Basic Types Tests Completed Successfully ===');
end.