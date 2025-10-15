program AdvancedRecords;

type
  { Simple record }
  Point = record
    x, y: integer;
  end;
  
  { Record with multiple field types }
  Employee = record
    id: integer;
    name: string;
    salary: real;
    active: boolean;
  end;
  
  { Nested record (using already defined type) }
  Circle = record
    center: Point;
    radius: real;
  end;

var
  p1, p2: Point;
  emp: Employee;
  c: Circle;

begin
  writeln('=== Advanced Record Type Test ===');
  
  { Test basic record operations }
  p1.x := 5;
  p1.y := 10;
  writeln('Point 1: (', p1.x, ', ', p1.y, ')');
  
  { Test record assignment }
  p2 := p1;
  p2.x := p2.x * 2;
  writeln('Point 2: (', p2.x, ', ', p2.y, ')');
  
  { Test record with mixed field types }
  emp.id := 101;
  emp.name := 'Alice Smith';
  emp.salary := 75000.50;
  emp.active := true;
  
  writeln('Employee ID: ', emp.id);
  writeln('Employee Name: ', emp.name);
  writeln('Employee Salary: ', emp.salary);
  writeln('Employee Active: ', emp.active);
  
  { Test nested record }
  c.center.x := 100;
  c.center.y := 200;
  c.radius := 15.5;
  
  writeln('Circle center: (', c.center.x, ', ', c.center.y, ')');
  writeln('Circle radius: ', c.radius);
  
  writeln('=== All record tests passed! ===');
end.