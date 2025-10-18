program TestFileOperations;

{
  Simplified test for file operations (text files only)
  Tests: text files, append mode, IOResult() error checking
}

var
  textFile: text;
  i: integer;
  fileName: string;
  line: string;
  
  { Test result tracking }
  textFileTestsPassed: boolean;
  errorCheckingPassed: boolean;

begin
  writeln('=== File Operations Test (Text Files Only) ===');
  
  { Initialize test results }
  textFileTestsPassed := true;
  errorCheckingPassed := true;
  
  { === TEXT FILE OPERATIONS === }
  writeln('--- Text File Operations ---');
  
  { Write to text file }
  fileName := 'test_output.txt';
  assign(textFile, fileName);
  rewrite(textFile);
  if IOResult() <> 0 then
  begin
    writeln('ERROR: Could not create text file - Code: ', IOResult());
    textFileTestsPassed := false;
  end
  else
  begin
    writeln(textFile, 'This is a test file');
    writeln(textFile, 'Line 2: Numbers and text');
    writeln(textFile, 'Line 3: ', 123, ' and ', 45.67:0:2);
    writeln(textFile, 'Line 4: Boolean value: ', true);
    
    for i := 1 to 5 do
      writeln(textFile, 'Loop line ', i, ': value = ', i * i);
    
    close(textFile);
    if IOResult() = 0 then
      writeln('Written text file: ', fileName)
    else
    begin
      writeln('ERROR: Could not write to text file - Code: ', IOResult());
      textFileTestsPassed := false;
    end;
  end;
  
  { Read from text file }
  assign(textFile, fileName);
  reset(textFile);
  if IOResult() <> 0 then
  begin
    writeln('ERROR: Could not open text file for reading - Code: ', IOResult());
    textFileTestsPassed := false;
  end
  else
  begin
    writeln('Reading text file contents:');
    i := 1;
    while not eof(textFile) do
    begin
      readln(textFile, line);
      if IOResult() <> 0 then
      begin
        writeln('ERROR: Could not read from text file - Code: ', IOResult());
        textFileTestsPassed := false;
        break;
      end;
      writeln('  Line ', i, ': ', line);
      i := i + 1;
    end;
    
    close(textFile);
  end;
  
  { === APPEND MODE OPERATIONS === }
  writeln();
  writeln('--- Append Mode Operations ---');
  
  { Create initial file }
  fileName := 'test_append.txt';
  assign(textFile, fileName);
  rewrite(textFile);
  if IOResult() <> 0 then
  begin
    writeln('ERROR: Could not create append test file - Code: ', IOResult());
    textFileTestsPassed := false;
  end
  else
  begin
    writeln(textFile, 'Initial content');
    writeln(textFile, 'Second line');
    close(textFile);
    
    { Append to existing file }
    writeln('Appending to text file...');
    assign(textFile, fileName);
    append(textFile);
    if IOResult() <> 0 then
    begin
      writeln('ERROR: Could not open file for append - Code: ', IOResult());
      textFileTestsPassed := false;
    end
    else
    begin
      writeln(textFile, 'Appended line 1');
      writeln(textFile, 'Appended line 2');
      close(textFile);
      if IOResult() <> 0 then
      begin
        writeln('ERROR: Could not close append file - Code: ', IOResult());
        textFileTestsPassed := false;
      end;
    end;
  end;
  
  { Read complete file }
  assign(textFile, fileName);
  reset(textFile);
  writeln('Complete file contents:');
  i := 1;
  while not eof(textFile) do
  begin
    readln(textFile, line);
    writeln('  ', i, ': ', line);
    i := i + 1;
  end;
  close(textFile);
  
  { === ERROR CHECKING === }
  writeln();
  writeln('--- I/O Error Checking ---');
  
  { Test error handling with non-existent file }
  assign(textFile, 'nonexistent_file.txt');
  reset(textFile);
  if IOResult() <> 0 then
  begin
    writeln('Reset non-existent file: ERROR (expected) - Code: ', IOResult());
  end
  else
  begin
    writeln('Reset non-existent file: UNEXPECTED SUCCESS');
    errorCheckingPassed := false;
  end;
  
  { Test successful operation }
  assign(textFile, 'test_output.txt'); { This file should exist from earlier }
  reset(textFile);
  if IOResult() = 0 then
  begin
    writeln('Reset existing file: SUCCESS');
    close(textFile);
  end
  else
  begin
    writeln('Reset existing file: ERROR - Code: ', IOResult());
    errorCheckingPassed := false;
  end;
  
  writeln();
  writeln('--- File Operations Summary ---');
  if textFileTestsPassed then
    writeln('Text file operations: PASSED')
  else
    writeln('Text file operations: FAILED');
    
  if errorCheckingPassed then
    writeln('I/O error checking (IOResult): PASSED')
  else
    writeln('I/O error checking (IOResult): FAILED');
  
  writeln();
  writeln('Note: Test files created:');
  writeln('  - test_output.txt (text file)');
  writeln('  - test_append.txt (append test file)');
  
  writeln();
  if textFileTestsPassed and errorCheckingPassed then
    writeln('=== File Operations Tests Completed Successfully - ALL TESTS PASSED ===')
  else
    writeln('=== File Operations Tests Completed - SOME TESTS FAILED ===');
end.