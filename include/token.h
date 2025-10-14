#pragma once

#include <string>
#include <unordered_map>

namespace rpascal {

enum class TokenType {
    // Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    REAL_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    
    // Keywords
    PROGRAM,
    UNIT,
    USES,
    CONST,
    TYPE,
    VAR,
    PROCEDURE,
    FUNCTION,
    BEGIN,
    END,
    IF,
    THEN,
    ELSE,
    WHILE,
    DO,
    FOR,
    TO,
    DOWNTO,
    REPEAT,
    UNTIL,
    CASE,
    OF,
    WITH,
    GOTO,
    LABEL,
    RECORD,
    ARRAY,
    SET,
    FILE,
    PACKED,
    FORWARD,
    EXTERNAL,
    
    // Data types
    INTEGER,
    REAL,
    BOOLEAN,
    CHAR,
    STRING,
    
    // Boolean literals
    TRUE,
    FALSE,
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    MULTIPLY,       // *
    DIVIDE,         // /
    DIV,            // div
    MOD,            // mod
    ASSIGN,         // :=
    EQUAL,          // =
    NOT_EQUAL,      // <>
    LESS_THAN,      // <
    LESS_EQUAL,     // <=
    GREATER_THAN,   // >
    GREATER_EQUAL,  // >=
    AND,            // and
    OR,             // or
    NOT,            // not
    XOR,            // xor
    IN,             // in
    SHL,            // shl
    SHR,            // shr
    
    // Punctuation
    SEMICOLON,      // ;
    COMMA,          // ,
    PERIOD,         // .
    COLON,          // :
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    CARET,          // ^
    AT,             // @
    RANGE,          // ..
    
    // Special
    EOF_TOKEN,
    NEWLINE,
    WHITESPACE,
    COMMENT,
    INVALID
};

struct SourceLocation {
    size_t line;
    size_t column;
    size_t position;
    
    SourceLocation(size_t l = 1, size_t c = 1, size_t p = 0) 
        : line(l), column(c), position(p) {}
};

class Token {
public:
    Token(TokenType type, const std::string& value, const SourceLocation& location);
    Token();
    
    TokenType getType() const { return type_; }
    const std::string& getValue() const { return value_; }
    const SourceLocation& getLocation() const { return location_; }
    
    bool isKeyword() const;
    bool isOperator() const;
    bool isLiteral() const;
    bool isPunctuation() const;
    
    std::string toString() const;
    static std::string tokenTypeToString(TokenType type);
    
private:
    TokenType type_;
    std::string value_;
    SourceLocation location_;
};

// Helper class for keyword lookup
class Keywords {
public:
    static TokenType getKeywordType(const std::string& word);
    static bool isKeyword(const std::string& word);
    
private:
    static const std::unordered_map<std::string, TokenType> keywords_;
};

} // namespace rpascal