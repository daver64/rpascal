program TestSets;

{
  Comprehensive test for set operations
  Tests: declarations, literals, union, intersection, difference, membership, comparisons
}

type
  { Enumeration types for set testing }
  TColor = (Red, Green, Blue, Yellow, Purple, Orange, Black, White);
  TDay = (Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday);
  TDigit = 0..9;
  TLetter = 'A'..'Z';
  
  { Set type declarations }
  TColorSet = set of TColor;
  TDaySet = set of TDay;
  TDigitSet = set of TDigit;
  TCharSet = set of char;
  TLetterSet = set of TLetter;

var
  { Set variables }
  colors1, colors2, colors3: TColorSet;
  workdays, weekend, alldays: TDaySet;
  digits1, digits2: TDigitSet;
  vowels, consonants, letters: TCharSet;
  upperCase: TLetterSet;
  
  { Individual elements for testing }
  color: TColor;
  day: TDay;
  digit: TDigit;
  ch: char;
  letter: TLetter;
  
  { Helper variables }
  i: integer;
  found: boolean;

begin
  writeln('=== Set Operations Test ===');
  
  { === BASIC SET DECLARATIONS AND LITERALS === }
  writeln('--- Basic Set Declarations and Literals ---');
  
  { Initialize basic sets }
  colors1 := [Red, Green, Blue];
  colors2 := [Yellow, Purple];
  colors3 := [];  { Empty set }
  
  writeln('colors1 contains Red, Green, Blue');
  writeln('colors2 contains Yellow, Purple');
  writeln('colors3 is empty');
  writeln;
  
  { === SET MEMBERSHIP TESTING === }
  writeln('--- Set Membership Testing ---');
  
  { Test membership with in operator }
  if Red in colors1 then
    writeln('Red is in colors1')
  else
    writeln('Red is NOT in colors1');
  
  if Yellow in colors1 then
    writeln('Yellow is in colors1')
  else
    writeln('Yellow is NOT in colors1');
  
  if Purple in colors2 then
    writeln('Purple is in colors2')
  else
    writeln('Purple is NOT in colors2');
  
  { Test with enumeration loop }
  write('colors1 contains: ');
  for color := Red to White do
  begin
    if color in colors1 then
      write(ord(color), ' ');
  end;
  writeln;
  
  write('colors2 contains: ');
  for color := Red to White do
  begin
    if color in colors2 then
      write(ord(color), ' ');
  end;
  writeln;
  writeln;
  
  { === SET UNION OPERATIONS === }
  writeln('--- Set Union Operations ---');
  
  { Test union with + operator }
  colors3 := colors1 + colors2;
  writeln('colors3 := colors1 + colors2');
  
  write('colors3 contains: ');
  for color := Red to White do
  begin
    if color in colors3 then
      write(ord(color), ' ');
  end;
  writeln;
  
  { Union with individual elements }
  colors3 := colors1 + [Orange];
  writeln('colors1 + [Orange] contains: ');
  write('  ');
  for color := Red to White do
  begin
    if color in colors3 then
      write(ord(color), ' ');
  end;
  writeln;
  writeln;
  
  { === SET INTERSECTION OPERATIONS === }
  writeln('--- Set Intersection Operations ---');
  
  { Create overlapping sets }
  colors1 := [Red, Green, Blue, Yellow];
  colors2 := [Yellow, Purple, Red];
  
  colors3 := colors1 * colors2;
  writeln('colors1 = [Red, Green, Blue, Yellow]');
  writeln('colors2 = [Yellow, Purple, Red]');
  writeln('colors1 * colors2 contains: ');
  write('  ');
  for color := Red to White do
  begin
    if color in colors3 then
      write(ord(color), ' ');
  end;
  writeln;
  
  { Test intersection with no overlap }
  colors1 := [Red, Green];
  colors2 := [Blue, Yellow];
  colors3 := colors1 * colors2;
  
  writeln('Intersection of [Red, Green] and [Blue, Yellow]:');
  if colors3 = [] then
    writeln('  Empty set (no common elements)')
  else
  begin
    write('  ');
    for color := Red to White do
    begin
      if color in colors3 then
        write(ord(color), ' ');
    end;
    writeln;
  end;
  writeln;
  
  { === SET DIFFERENCE OPERATIONS === }
  writeln('--- Set Difference Operations ---');
  
  colors1 := [Red, Green, Blue, Yellow];
  colors2 := [Green, Yellow];
  
  colors3 := colors1 - colors2;
  writeln('colors1 = [Red, Green, Blue, Yellow]');
  writeln('colors2 = [Green, Yellow]');
  writeln('colors1 - colors2 contains: ');
  write('  ');
  for color := Red to White do
  begin
    if color in colors3 then
      write(ord(color), ' ');
  end;
  writeln;
  
  { Reverse difference }
  colors3 := colors2 - colors1;
  writeln('colors2 - colors1 contains: ');
  if colors3 = [] then
    writeln('  Empty set')
  else
  begin
    write('  ');
    for color := Red to White do
    begin
      if color in colors3 then
        write(ord(color), ' ');
    end;
    writeln;
  end;
  writeln;
  
  { === SET COMPARISONS === }
  writeln('--- Set Comparisons ---');
  
  colors1 := [Red, Green];
  colors2 := [Red, Green];
  colors3 := [Green, Red];  { Same elements, different order }
  
  writeln('colors1 = [Red, Green]');
  writeln('colors2 = [Red, Green]');
  writeln('colors3 = [Green, Red]');
  
  writeln('colors1 = colors2: ', colors1 = colors2);
  writeln('colors1 = colors3: ', colors1 = colors3);
  writeln('colors1 <> colors2: ', colors1 <> colors2);
  
  colors2 := [Red, Green, Blue];
  writeln('After colors2 := [Red, Green, Blue]:');
  writeln('colors1 = colors2: ', colors1 = colors2);
  writeln;
  
  { === CHARACTER SETS === }
  writeln('--- Character Sets ---');
  
  vowels := ['a', 'e', 'i', 'o', 'u', 'A', 'E', 'I', 'O', 'U'];
  consonants := ['b'..'z', 'B'..'Z'] - vowels;
  
  writeln('Testing character "e":');
  writeln('  "e" in vowels: ', 'e' in vowels);
  writeln('  "e" in consonants: ', 'e' in consonants);
  
  writeln('Testing character "x":');
  writeln('  "x" in vowels: ', 'x' in vowels);
  writeln('  "x" in consonants: ', 'x' in consonants);
  
  { Count vowels in a word }
  writeln('Vowels in "hello": ');
  write('  ');
  if 'h' in vowels then write('h ');
  if 'e' in vowels then write('e ');
  if 'l' in vowels then write('l ');
  if 'l' in vowels then write('l ');
  if 'o' in vowels then write('o ');
  writeln;
  writeln;
  
  { === DIGIT SETS === }
  writeln('--- Digit Sets ---');
  
  digits1 := [0, 2, 4, 6, 8];
  digits2 := [1, 3, 5, 7, 9];
  
  writeln('Even digits: ');
  write('  ');
  for i := 0 to 9 do
  begin
    if i in digits1 then
      write(i, ' ');
  end;
  writeln;
  
  writeln('Odd digits: ');
  write('  ');
  for i := 0 to 9 do
  begin
    if i in digits2 then
      write(i, ' ');
  end;
  writeln;
  
  { Test if sets are complementary }
  if (digits1 + digits2) = [0..9] then
    writeln('Even and odd digits form complete set')
  else
    writeln('Even and odd digits do NOT form complete set');
  
  if (digits1 * digits2) = [] then
    writeln('Even and odd digits have no intersection')
  else
    writeln('Even and odd digits have intersection');
  writeln;
  
  { === DAY SETS === }
  writeln('--- Day Sets ---');
  
  workdays := [Monday, Tuesday, Wednesday, Thursday, Friday];
  weekend := [Saturday, Sunday];
  alldays := workdays + weekend;
  
  writeln('Workdays: Monday through Friday');
  writeln('Weekend: Saturday and Sunday');
  
  writeln('Is Wednesday a workday? ', Wednesday in workdays);
  writeln('Is Sunday a workday? ', Sunday in workdays);
  writeln('Is Saturday a weekend day? ', Saturday in weekend);
  
  if alldays = [Monday..Sunday] then
    writeln('All days of week are accounted for')
  else
    writeln('Missing some days of week');
  writeln;
  
  { === COMPLEX SET OPERATIONS === }
  writeln('--- Complex Set Operations ---');
  
  { Multiple operations in one expression }
  colors1 := [Red, Green, Blue];
  colors2 := [Green, Yellow, Purple];
  colors3 := [Blue, Purple, Orange];
  
  { Find colors that are in colors1 but not in the union of colors2 and colors3 }
  colors1 := colors1 - (colors2 + colors3);
  writeln('Complex operation result:');
  write('  ');
  for color := Red to White do
  begin
    if color in colors1 then
      write(ord(color), ' ');
  end;
  writeln;
  
  { Find common elements across three sets }
  colors1 := [Red, Green, Blue, Yellow];
  colors2 := [Green, Yellow, Purple, Blue];
  colors3 := [Blue, Yellow, Orange, Green];
  
  colors1 := colors1 * colors2 * colors3;
  writeln('Common to all three sets:');
  write('  ');
  for color := Red to White do
  begin
    if color in colors1 then
      write(ord(color), ' ');
  end;
  writeln;
  writeln;
  
  { === EMPTY SET TESTS === }
  writeln('--- Empty Set Tests ---');
  
  colors1 := [];
  colors2 := [Red, Green];
  
  writeln('colors1 is empty');
  writeln('colors1 = []: ', colors1 = []);
  writeln('colors1 + colors2 = colors2: ', (colors1 + colors2) = colors2);
  writeln('colors1 * colors2 = []: ', (colors1 * colors2) = []);
  writeln('colors2 - colors1 = colors2: ', (colors2 - colors1) = colors2);
  
  writeln;
  writeln('=== All Set Tests Completed Successfully ===');
end.