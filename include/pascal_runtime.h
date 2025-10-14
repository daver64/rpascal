#pragma once
#include <string>

namespace rpascal {

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