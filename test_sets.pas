program TestSets;program TestSets;



{ Test comprehensive set operations in Pascal }type

  TCharSet = set of char;

var  TIntSet = set of integer;

    digits: set of 1..10;

    evens: set of 1..10;var

    odds: set of 1..10;  vowels: TCharSet;

    primes: set of 1..10;  numbers: TIntSet;

    result: set of 1..10;  ch: char;

    i: integer;

begin

begin  writeln('Testing set types:');

    { Initialize sets }  

    digits := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];  { Test set literals }

    evens := [2, 4, 6, 8, 10];  vowels := ['a', 'e', 'i', 'o', 'u'];

    odds := [1, 3, 5, 7, 9];  numbers := [1, 3, 5, 7, 9];

    primes := [2, 3, 5, 7];  

      { Test membership testing }

    writeln('Testing Set Operations');  ch := 'a';

    writeln('=====================');  if ch in vowels then

        writeln('a is a vowel')

    { Test set union with + operator }  else

    writeln('Testing set union (+):');    writeln('a is not a vowel');

    result := evens + odds;    

    writeln('evens + odds should equal digits (1-10)');  ch := 'b';

      if ch in vowels then

    { Test set intersection with * operator }    writeln('b is a vowel')

    writeln('Testing set intersection (*):');  else

    result := digits * primes;    writeln('b is not a vowel');

    writeln('digits * primes should be [2, 3, 5, 7]');    

      if 5 in numbers then

    { Test set difference with - operator }    writeln('5 is in the set')

    writeln('Testing set difference (-):');  else

    result := digits - evens;    writeln('5 is not in the set');

    writeln('digits - evens should be [1, 3, 5, 7, 9]');    

      writeln('Set test complete');

    { Test set membership with in operator }end.
    writeln('Testing set membership (in):');
    if 3 in primes then
        writeln('3 is in primes - correct!')
    else
        writeln('3 is NOT in primes - ERROR!');
        
    if 4 in primes then
        writeln('4 is in primes - ERROR!')
    else
        writeln('4 is NOT in primes - correct!');
    
    { Test empty set operations }
    writeln('Testing empty set:');
    result := [];
    result := result + [5];
    writeln('Empty set + [5] should be [5]');
    
    { Test single element sets }
    writeln('Testing single element operations:');
    result := [1] + [2];
    writeln('[1] + [2] should be [1, 2]');
    
    result := [1, 2, 3] * [2];
    writeln('[1, 2, 3] * [2] should be [2]');
    
    writeln('Set operations test complete!');
end.