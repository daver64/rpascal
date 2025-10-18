program TestSuccPred;

type
  TDayOfWeek = (Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday);
  TDigit = 0..9;

var
  day: TDayOfWeek;
  nextDay: TDayOfWeek;
  prevDay: TDayOfWeek;
  digit: TDigit;
  nextDigit: TDigit;
  prevDigit: TDigit;
  ch: char;
  nextChar: char;
  prevChar: char;

begin
  writeln('Testing succ and pred functions:');
  writeln();
  
  // Test with enumeration
  day := Wednesday;
  nextDay := succ(day);
  prevDay := pred(day);
  writeln('Current day: ', ord(day));
  writeln('Next day: ', ord(nextDay));
  writeln('Previous day: ', ord(prevDay));
  writeln();
  
  // Test with subrange
  digit := 5;
  nextDigit := succ(digit);
  prevDigit := pred(digit);
  writeln('Current digit: ', digit);
  writeln('Next digit: ', nextDigit);
  writeln('Previous digit: ', prevDigit);
  writeln();
  
  // Test with character
  ch := 'M';
  nextChar := succ(ch);
  prevChar := pred(ch);
  writeln('Current char: ', ch);
  writeln('Next char: ', nextChar);
  writeln('Previous char: ', prevChar);
  writeln();
  
  writeln('All tests completed successfully!');
end.