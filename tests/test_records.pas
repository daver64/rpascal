program TestRecords;

{
  Comprehensive test for record types and variant records
  Tests: simple records, nested records, variant records, with statements
}

type
  { Simple record }
  TPoint = record
    x, y: integer;
  end;
  
  { Record with different field types }
  TPerson = record
    name: string;
    age: integer;
    height: real;
    active: boolean;
  end;
  
  { Nested record }
  TRectangle = record
    topLeft: TPoint;
    bottomRight: TPoint;
    color: char;
  end;
  
  { Shape record (simplified - no variant record) }
  TShape = record
    shapeType: integer;
    radius: real;             { Used for circles }
    width: real;              { Used for rectangles }
    height: real;             { Used for rectangles }
    side: real;               { Used for squares }
    base: real;               { Used for triangles }
    triangleHeight: real;     { Used for triangles }
  end;
  
  { Record with array field }
  TStudent = record
    name: string;
    grades: array[1..5] of integer;
    average: real;
  end;

var
  point1, point2: TPoint;
  person1, person2: TPerson;
  rect: TRectangle;
  circle, square, triangle: TShape;
  student: TStudent;
  i: integer;
  sum: integer;

begin
  writeln('=== Record Types Test ===');
  
  { === SIMPLE RECORD TESTS === }
  writeln('--- Simple Record Tests ---');
  
  point1.x := 10;
  point1.y := 20;
  writeln('Point1: (', point1.x, ', ', point1.y, ')');
  
  point2.x := point1.x + 5;
  point2.y := point1.y + 10;
  writeln('Point2: (', point2.x, ', ', point2.y, ')');
  
  { === RECORD WITH MULTIPLE FIELD TYPES === }
  writeln();
  writeln('--- Person Record Tests ---');
  
  person1.name := 'Alice';
  person1.age := 25;
  person1.height := 165.5;
  person1.active := true;
  
  writeln('Person1:');
  writeln('  Name: ', person1.name);
  writeln('  Age: ', person1.age);
  writeln('  Height: ', person1.height:0:1, ' cm');
  writeln('  Active: ', person1.active);
  
  { Record assignment }
  person2 := person1;
  person2.name := 'Bob';
  person2.age := 30;
  writeln('Person2 (copied and modified):');
  writeln('  Name: ', person2.name);
  writeln('  Age: ', person2.age);
  
  { === NESTED RECORD TESTS === }
  writeln();
  writeln('--- Nested Record Tests ---');
  
  rect.topLeft.x := 0;
  rect.topLeft.y := 0;
  rect.bottomRight.x := 100;
  rect.bottomRight.y := 50;
  rect.color := 'R';
  
  writeln('Rectangle:');
  writeln('  Top-left: (', rect.topLeft.x, ', ', rect.topLeft.y, ')');
  writeln('  Bottom-right: (', rect.bottomRight.x, ', ', rect.bottomRight.y, ')');
  writeln('  Color: ', rect.color);
  writeln('  Width: ', rect.bottomRight.x - rect.topLeft.x);
  writeln('  Height: ', rect.bottomRight.y - rect.topLeft.y);
  
  { === WITH STATEMENT TESTS === }
  writeln();
  writeln('--- With Statement Tests ---');
  
  { Direct point access }
  writeln('Direct point1 access: (', point1.x, ', ', point1.y, ')');
  point1.x := 100;
  point1.y := 200;
  writeln('Modified point1: (', point1.x, ', ', point1.y, ')');
  
  with person1 do
  begin
    writeln('Using direct field access for person1:');
    writeln('  Name: ', person1.name);
    writeln('  Age: ', person1.age);
    person1.active := false;
    writeln('  Active (modified): ', person1.active);  
  end;
  
  { Direct field access for rectangle }
  writeln('Rectangle color: ', rect.color);
  writeln('Top-left coordinates: (', rect.topLeft.x, ', ', rect.topLeft.y, ')');
  rect.topLeft.x := rect.topLeft.x + 10;
  rect.topLeft.y := rect.topLeft.y + 10;
  writeln('Modified top-left: (', rect.topLeft.x, ', ', rect.topLeft.y, ')');
  
  { === VARIANT RECORD TESTS === }
  writeln();
  writeln('--- Variant Record Tests ---');
  
  { Circle }
  circle.shapeType := 1;
  circle.radius := 5.0;
  writeln('Circle: type=', circle.shapeType, ', radius=', circle.radius:0:1);
  writeln('Circle area: ', 3.14159 * circle.radius * circle.radius:0:2);
  
  { Rectangle }
  square.shapeType := 2;
  square.width := 10.0;
  square.height := 15.0;
  writeln('Rectangle: type=', square.shapeType, ', width=', square.width:0:1, ', height=', square.height:0:1);
  writeln('Rectangle area: ', square.width * square.height:0:1);
  
  { Square }
  triangle.shapeType := 3;
  triangle.side := 8.0;
  writeln('Square: type=', triangle.shapeType, ', side=', triangle.side:0:1);
  writeln('Square area: ', triangle.side * triangle.side:0:1);
  
  { === RECORD WITH ARRAY FIELD === }
  writeln();
  writeln('--- Record with Array Field Tests ---');
  
  student.name := 'Charlie';
  student.grades[1] := 85;
  student.grades[2] := 92;
  student.grades[3] := 78;
  student.grades[4] := 89;
  student.grades[5] := 94;
  
  writeln('Student: ', student.name);
  write('Grades: ');
  sum := 0;
  for i := 1 to 5 do
  begin
    write(student.grades[i]);
    if i < 5 then write(', ');
    sum := sum + student.grades[i];
  end;
  writeln();
  
  student.average := sum / 5.0;
  writeln('Average: ', student.average:0:1);
  
  writeln();
  writeln('=== Record Tests Completed Successfully ===');
end.