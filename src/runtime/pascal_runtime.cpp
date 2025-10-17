#include "../include/pascal_runtime.h"
#include <ios>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>
#include <stdexcept>

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

// Additional string functions
std::string pascal_trim(const std::string& str) {
    if (str.empty()) return str;
    
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string pascal_trimleft(const std::string& str) {
    if (str.empty()) return str;
    
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    
    return str.substr(start);
}

std::string pascal_trimright(const std::string& str) {
    if (str.empty()) return str;
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) return "";
    
    return str.substr(0, end + 1);
}

std::string pascal_stringofchar(char ch, int count) {
    if (count <= 0) return "";
    return std::string(static_cast<size_t>(count), ch);
}

std::string pascal_lowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string pascal_uppercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string pascal_leftstr(const std::string& str, int count) {
    if (count <= 0) return "";
    if (count >= static_cast<int>(str.length())) return str;
    return str.substr(0, static_cast<size_t>(count));
}

std::string pascal_rightstr(const std::string& str, int count) {
    if (count <= 0) return "";
    if (count >= static_cast<int>(str.length())) return str;
    return str.substr(str.length() - static_cast<size_t>(count));
}

std::string pascal_padleft(const std::string& str, int totalWidth, char paddingChar) {
    if (totalWidth <= static_cast<int>(str.length())) return str;
    
    int padCount = totalWidth - static_cast<int>(str.length());
    return std::string(static_cast<size_t>(padCount), paddingChar) + str;
}

std::string pascal_padright(const std::string& str, int totalWidth, char paddingChar) {
    if (totalWidth <= static_cast<int>(str.length())) return str;
    
    int padCount = totalWidth - static_cast<int>(str.length());
    return str + std::string(static_cast<size_t>(padCount), paddingChar);
}

// Mathematical functions
double pascal_power(double base, double exponent) {
    return std::pow(base, exponent);
}

int pascal_round(double value) {
    return static_cast<int>(std::round(value));
}

int pascal_trunc(double value) {
    return static_cast<int>(std::trunc(value));
}

// String conversion functions
std::string pascal_inttostr(int value) {
    return std::to_string(value);
}

std::string pascal_floattostr(double value) {
    return std::to_string(value);
}

int pascal_strtoint(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return 0; // Pascal behavior on invalid conversion
    }
}

double pascal_strtofloat(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return 0.0; // Pascal behavior on invalid conversion
    }
}

// Format function (simplified implementation)
std::string pascal_format(const std::string& format, const std::vector<std::string>& args) {
    std::string result = format;
    size_t argIndex = 0;
    
    // Simple placeholder replacement for %s, %d, %f
    size_t pos = 0;
    while ((pos = result.find('%', pos)) != std::string::npos && argIndex < args.size()) {
        if (pos + 1 < result.length()) {
            char specifier = result[pos + 1];
            if (specifier == 's' || specifier == 'd' || specifier == 'f') {
                result.replace(pos, 2, args[argIndex++]);
            } else {
                pos++;
            }
        } else {
            break;
        }
    }
    
    return result;
}

// Date/time functions (simplified)
int pascal_dayofweek(int year, int month, int day) {
    // Simplified day of week calculation (Zeller's congruence)
    if (month < 3) {
        month += 12;
        year--;
    }
    
    int k = year % 100;
    int j = year / 100;
    
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    
    // Convert to Pascal day of week (1=Sunday, 2=Monday, etc.)
    return ((h + 5) % 7) + 1;
}

std::string pascal_datetostr(int year, int month, int day) {
    char buffer[11]; // "MM/DD/YYYY"
    std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", month, day, year);
    return std::string(buffer);
}

std::string pascal_timetostr(int hour, int minute, int second) {
    char buffer[9]; // "HH:MM:SS"
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour, minute, second);
    return std::string(buffer);
}

// Global variables for command-line arguments
int pascal_argc = 0;
char** pascal_argv = nullptr;

} // namespace rpascal