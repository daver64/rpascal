#pragma once
#include <string>
#include <fstream>

namespace rpascal {

// Pascal file wrapper that stores filename and stream
class PascalFile {
private:
    std::fstream stream_;
    std::string filename_;
    
public:
    PascalFile() = default;
    ~PascalFile() { close(); }
    
    // Pascal file operations
    void assign(const std::string& filename);
    void reset();   // open for reading
    void rewrite(); // open for writing
    void close();
    bool eof() const;
    
    // Stream access for read/write operations
    std::fstream& getStream() { return stream_; }
    const std::string& getFilename() const { return filename_; }
};

// Pascal string manipulation functions
void pascal_insert(const std::string& substr, std::string& str, int pos);
void pascal_delete(std::string& str, int pos, int length);

// Placeholder for Pascal runtime library
class PascalRuntime {
public:
    PascalRuntime() = default;
    ~PascalRuntime() = default;
    
    // TODO: Implement Pascal runtime functions (writeln, readln, etc.)
};

} // namespace rpascal