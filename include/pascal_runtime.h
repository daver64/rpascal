#pragma once
#include <string>
#include <fstream>
#include <vector>

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

// Template class for typed files (file of T)
template<typename T>
class PascalTypedFile {
private:
    std::fstream stream_;
    std::string filename_;
    
public:
    PascalTypedFile() = default;
    ~PascalTypedFile() { close(); }
    
    // Pascal file operations
    void assign(const std::string& filename) {
        filename_ = filename;
    }
    
    void reset() {
        close();
        stream_.open(filename_, std::ios::in | std::ios::binary);
    }
    
    void rewrite() {
        close();
        stream_.open(filename_, std::ios::out | std::ios::binary);
    }
    
    void close() {
        if (stream_.is_open()) {
            stream_.close();
        }
    }
    
    bool eof() const {
        return stream_.eof();
    }
    
    // Typed file operations
    void write(const T& data) {
        stream_.write(reinterpret_cast<const char*>(&data), sizeof(T));
    }
    
    void read(T& data) {
        stream_.read(reinterpret_cast<char*>(&data), sizeof(T));
    }
    
    // Block operations
    void blockwrite(const T* buffer, size_t count, size_t& result) {
        stream_.write(reinterpret_cast<const char*>(buffer), count * sizeof(T));
        result = stream_.good() ? count : 0;
    }
    
    void blockread(T* buffer, size_t count, size_t& result) {
        stream_.read(reinterpret_cast<char*>(buffer), count * sizeof(T));
        result = stream_.gcount() / sizeof(T);
    }
    
    // File positioning
    long filepos() const {
        return stream_.tellg() / sizeof(T);
    }
    
    long filesize() const {
        auto pos = stream_.tellg();
        stream_.seekg(0, std::ios::end);
        auto size = stream_.tellg() / sizeof(T);
        stream_.seekg(pos);
        return size;
    }
    
    void seek(long position) {
        stream_.seekg(position * sizeof(T));
        stream_.seekp(position * sizeof(T));
    }
    
    std::fstream& getStream() { return stream_; }
    const std::string& getFilename() const { return filename_; }
};

// Pascal string manipulation functions
void pascal_insert(const std::string& substr, std::string& str, int pos);
void pascal_delete(std::string& str, int pos, int length);

// Additional string functions
std::string pascal_trim(const std::string& str);
std::string pascal_trimleft(const std::string& str);
std::string pascal_trimright(const std::string& str);
std::string pascal_stringofchar(char ch, int count);
std::string pascal_lowercase(const std::string& str);
std::string pascal_uppercase(const std::string& str);
std::string pascal_leftstr(const std::string& str, int count);
std::string pascal_rightstr(const std::string& str, int count);
std::string pascal_padleft(const std::string& str, int totalWidth, char paddingChar = ' ');
std::string pascal_padright(const std::string& str, int totalWidth, char paddingChar = ' ');

// File operation functions
template<typename T>
void pascal_blockwrite(PascalTypedFile<T>& file, const T* buffer, size_t count, size_t& result) {
    file.blockwrite(buffer, count, result);
}

template<typename T>
void pascal_blockread(PascalTypedFile<T>& file, T* buffer, size_t count, size_t& result) {
    file.blockread(buffer, count, result);
}

template<typename T>
long pascal_filepos(const PascalTypedFile<T>& file) {
    return file.filepos();
}

template<typename T>
long pascal_filesize(const PascalTypedFile<T>& file) {
    return file.filesize();
}

template<typename T>
void pascal_seek(PascalTypedFile<T>& file, long position) {
    file.seek(position);
}

// Placeholder for Pascal runtime library
class PascalRuntime {
public:
    PascalRuntime() = default;
    ~PascalRuntime() = default;
    
    // TODO: Implement Pascal runtime functions (writeln, readln, etc.)
};

} // namespace rpascal