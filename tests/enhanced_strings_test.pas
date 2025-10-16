program EnhancedStringsTest;

{ Test program for enhanced string operations }

var
  s, result: string;
  ch: char;
  i: integer;

begin
  writeln('=== Enhanced String Functions Test ===');
  
  { Test trim functions }
  s := '  Hello World  ';
  writeln('Original: "', s, '"');
  writeln('Trim: "', trim(s), '"');
  writeln('TrimLeft: "', trimleft(s), '"');
  writeln('TrimRight: "', trimright(s), '"');
  writeln;
  
  { Test stringofchar }
  ch := '*';
  result := stringofchar(ch, 10);
  writeln('StringOfChar(''*'', 10): "', result, '"');
  writeln;
  
  { Test case conversion }
  s := 'Hello World';
  writeln('Original: "', s, '"');
  writeln('Lowercase: "', lowercase(s), '"');
  writeln('Uppercase: "', uppercase(s), '"');
  writeln;
  
  { Test left/right string functions }
  s := 'Programming';
  writeln('Original: "', s, '"');
  writeln('LeftStr(5): "', leftstr(s, 5), '"');
  writeln('RightStr(4): "', rightstr(s, 4), '"');
  writeln;
  
  { Test padding functions }
  s := 'Test';
  writeln('Original: "', s, '"');
  writeln('PadLeft(10): "', padleft(s, 10), '"');
  writeln('PadRight(10): "', padright(s, 10), '"');
  writeln('PadLeft(10, ''.'') : "', padleft(s, 10, '.'), '"');
  writeln('PadRight(10, ''-''): "', padright(s, 10, '-'), '"');
  writeln;
  
  { Test with existing functions }
  s := 'Pascal Programming';
  writeln('Combined operations:');
  writeln('Original: "', s, '"');
  result := uppercase(leftstr(trim(s), 6));
  writeln('uppercase(leftstr(trim(s), 6)): "', result, '"');
  
  writeln('=== Test Complete ===');
end.