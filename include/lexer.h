#pragma once

#include "token.h"
#include <string>
#include <vector>
#include <memory>

namespace rpascal {

class Lexer {
public:
    explicit Lexer(const std::string& source);
    
    // Get the next token from the source
    Token nextToken();
    
    // Peek at the next token without consuming it
    Token peekToken();
    
    // Check if we've reached the end of the source
    bool isAtEnd() const;
    
    // Get current position info
    SourceLocation getCurrentLocation() const;
    
    // Error handling
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    
private:
    // Source code and position tracking
    std::string source_;
    size_t current_;
    size_t line_;
    size_t column_;
    size_t lineStart_;
    
    // Error collection
    std::vector<std::string> errors_;
    
    // Character examination and consumption
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd(size_t position) const;
    
    // Character classification
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isWhitespace(char c) const;
    
    // Token creation helpers
    Token makeToken(TokenType type, const std::string& value = "") const;
    Token makeErrorToken(const std::string& message) const;
    
    // Specific token parsing
    Token parseIdentifier();
    Token parseNumber();
    Token parseString();
    Token parseCharLiteral();
    Token parseComment();
    Token parseOperator();
    
    // Skip whitespace and comments
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    
    // Error reporting
    void addError(const std::string& message);
    
    // Utility functions
    SourceLocation makeLocation() const;
};

} // namespace rpascal