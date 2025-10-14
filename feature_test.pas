program FeatureTest;

{ Test what we have so far }
type
  TRange = 1..10;
  TCharRange = 'A'..'Z';

var
  i: integer;
  r: TRange;
  ch: char;

begin
  { Test range types - should work }
  r := 5;
  
  { Test for loops - should work }
  for i := 1 to 3 do
    writeln('Count: ', i);
    
  for ch := 'A' to 'C' do
    writeln('Char: ', ch);
    
  writeln('Feature test complete');
end.