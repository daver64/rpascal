unit DOS;

interface

type
  { File attributes }
  TSearchRec = record
    Fill: array[1..21] of integer;  { Simplified for now }
    Attr: integer;
    Time: integer;
    Size: integer;
    Name: string;
  end;

const
  { File attribute constants }
  faReadOnly = 1;
  faHidden = 2;
  faSysFile = 4;
  faVolumeID = 8;
  faDirectory = 16;
  faArchive = 32;
  faAnyFile = 63;

{ File operations }
function FileExists(const FileName: string): boolean;
function FileSize(const FileName: string): integer;
procedure FindFirst(const Path: string; Attr: integer; var F: TSearchRec);
procedure FindNext(var F: TSearchRec);
procedure FindClose(var F: TSearchRec);

{ Directory operations }
function GetCurrentDir: string;
procedure SetCurrentDir(const Dir: string);
function DirectoryExists(const Dir: string): boolean;
procedure MkDir(const Dir: string);
procedure RmDir(const Dir: string);

{ Date and time functions }
function GetDate: integer;
function GetTime: integer;
procedure GetDateTime(var Year, Month, Day, Hour, Min, Sec: integer);

{ Environment functions }
function GetEnv(const EnvVar: string): string;

{ Program execution }
function Exec(const Path, CmdLine: string): integer;

implementation

{ Implementations will be handled by the compiler as built-in functions }

end.