program TestStrings;
uses strings;

{
  Test program for the Strings unit built-in functions
  Demonstrates string manipulation functions from Turbo Pascal 7
}

var
  str1, str2: array[0..255] of char;
  result: ^char;
  len: integer;
  
begin
  writeln('=== RPascal Strings Unit Test ===');
  writeln();
  
  // Initialize test strings
  strcopy(str1, 'Hello World');
  strcopy(str2, ' - Pascal Programming');
  
  writeln('Initial strings:');
  writeln('str1: ', str1);
  writeln('str2: ', str2);
  writeln();
  
  // Test strlen
  len := strlen(str1);
  writeln('strlen test:');
  writeln('Length of "', str1, '" = ', len);
  writeln();
  
  // Test strcat (concatenation)
  writeln('strcat test:');
  writeln('Before: str1 = "', str1, '"');
  strcat(str1, str2);
  writeln('After strcat: str1 = "', str1, '"');
  writeln();
  
  // Test strcmp (comparison)
  writeln('strcomp test:');
  strcopy(str1, 'Apple');
  strcopy(str2, 'apple');
  writeln('Comparing "', str1, '" with "', str2, '"');
  writeln('strcomp result: ', strcomp(str1, str2));
  writeln('stricomp result: ', stricomp(str1, str2));
  writeln();
  
  // Test strpos (find substring)
  writeln('strpos test:');
  strcopy(str1, 'Hello Pascal World');
  result := strpos(str1, 'Pascal');
  if result <> nil then
    writeln('Found "Pascal" in "', str1, '"')
  else
    writeln('Substring not found');
  writeln();
  
  // Test case conversion
  writeln('Case conversion test:');
  strcopy(str1, 'Hello World');
  writeln('Original: "', str1, '"');
  strupper(str1);
  writeln('strupper: "', str1, '"');
  strlower(str1);
  writeln('strlower: "', str1, '"');
  writeln();
  
  // Test strscan (find character)
  writeln('strscan test:');
  strcopy(str1, 'Hello World');
  result := strscan(str1, 'o');
  if result <> nil then
    writeln('Found ''o'' in "', str1, '"')
  else
    writeln('Character not found');
  writeln();
  
  // Test strrscan (find last occurrence)
  writeln('strrscan test:');
  result := strrscan(str1, 'o');
  if result <> nil then
    writeln('Last ''o'' found in "', str1, '"')
  else
    writeln('Character not found');
  writeln();
  
  writeln('=== Strings Unit Test Complete ===');
end.