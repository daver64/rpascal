unit Foo;

interface

function Double(x: integer): integer;
procedure SayHello;

implementation

function Double(x: integer): integer;
begin
  Double := x * 2;
end;

procedure SayHello;
begin
  writeln('Hello from Foo unit!');
end;

end.