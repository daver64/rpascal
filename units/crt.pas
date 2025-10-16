unit CRT;

interface

const Black = 0;
const Blue = 1;
const Green = 2;
const Cyan = 3;
const Red = 4;
const Magenta = 5;
const Brown = 6;
const LightGray = 7;
const DarkGray = 8;
const LightBlue = 9;
const LightGreen = 10;
const LightCyan = 11;
const LightRed = 12;
const LightMagenta = 13;
const Yellow = 14;
const White = 15;
const Blink = 128;

procedure ClrScr;
procedure ClrEol;
procedure GotoXY(X, Y: integer);
function WhereX: integer;
function WhereY: integer;
procedure TextColor(Color: integer);
procedure TextBackground(Color: integer);
procedure LowVideo;
procedure HighVideo;
procedure NormVideo;
procedure Window(X1, Y1, X2, Y2: integer);
function KeyPressed: boolean;
function ReadKey: char;
procedure Sound(Hz: integer);
procedure NoSound;
procedure Delay(MS: integer);
procedure CursorOn;
procedure CursorOff;

implementation

end.