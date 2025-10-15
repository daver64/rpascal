#include "../include/pascal_runtime.h"
#include <ios>

namespace rpascal {

// PascalFile implementation
void PascalFile::assign(const std::string& filename) {
    filename_ = filename;
    // Just store the filename, don't open yet (like Pascal)
}

void PascalFile::reset() {
    close(); // Close if already open
    stream_.open(filename_, std::ios::in);
}

void PascalFile::rewrite() {
    close(); // Close if already open  
    stream_.open(filename_, std::ios::out);
}

void PascalFile::close() {
    if (stream_.is_open()) {
        stream_.close();
    }
}

bool PascalFile::eof() const {
    return stream_.eof();
}

// Pascal string manipulation functions
void pascal_insert(const std::string& substr, std::string& str, int pos) {
    // Pascal positions are 1-indexed, C++ are 0-indexed
    if (pos < 1) pos = 1;
    if (pos > static_cast<int>(str.length()) + 1) pos = static_cast<int>(str.length()) + 1;
    
    str.insert(static_cast<size_t>(pos - 1), substr);
}

void pascal_delete(std::string& str, int pos, int length) {
    // Pascal positions are 1-indexed, C++ are 0-indexed
    if (pos < 1 || pos > static_cast<int>(str.length())) return;
    if (length < 0) return;
    
    str.erase(static_cast<size_t>(pos - 1), static_cast<size_t>(length));
}

// Placeholder implementation

} // namespace rpascal