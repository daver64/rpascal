#include "../include/lexer.h"
#include <cctype>
#include <sstream>

namespace rpascal {

Lexer::Lexer(const std::string& source) 
    : source_(source), current_(0), line_(1), column_(1), lineStart_(0) {}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeToken(TokenType::EOF_TOKEN);
    }
    
    SourceLocation startLocation = makeLocation();
    char c = advance();
    
    // Identifiers and keywords
    if (isAlpha(c) || c == '_') {
        current_--; // Back up to re-read the character
        column_--;
        return parseIdentifier();
    }
    
    // Numbers
    if (isDigit(c)) {
        current_--; // Back up to re-read the character
        column_--;
        return parseNumber();
    }
    
    // String literals
    if (c == '\'') {
        return parseString();
    }
    
    // Character literals (single quoted, single character)
    if (c == '#') {
        return parseCharLiteral();
    }
    
    // Comments
    if (c == '{') {
        return parseComment();
    }
    if (c == '(' && peek() == '*') {
        advance(); // consume '*'
        skipBlockComment();
        return nextToken(); // Get the next real token
    }
    if (c == '/' && peek() == '/') {
        skipLineComment();
        return nextToken(); // Get the next real token
    }
    
    // Two-character operators
    if (c == ':' && peek() == '=') {
        advance();
        return makeToken(TokenType::ASSIGN, ":=");
    }
    if (c == '<' && peek() == '=') {
        advance();
        return makeToken(TokenType::LESS_EQUAL, "<=");
    }
    if (c == '>' && peek() == '=') {
        advance();
        return makeToken(TokenType::GREATER_EQUAL, ">=");
    }
    if (c == '<' && peek() == '>') {
        advance();
        return makeToken(TokenType::NOT_EQUAL, "<>");
    }
    if (c == '.' && peek() == '.') {
        advance();
        return makeToken(TokenType::RANGE, "..");
    }
    
    // Single-character tokens
    switch (c) {
        case '+': return makeToken(TokenType::PLUS, "+");
        case '-': return makeToken(TokenType::MINUS, "-");
        case '*': return makeToken(TokenType::MULTIPLY, "*");
        case '/': return makeToken(TokenType::DIVIDE, "/");
        case '=': return makeToken(TokenType::EQUAL, "=");
        case '<': return makeToken(TokenType::LESS_THAN, "<");
        case '>': return makeToken(TokenType::GREATER_THAN, ">");
        case ';': return makeToken(TokenType::SEMICOLON, ";");
        case ',': return makeToken(TokenType::COMMA, ",");
        case '.': return makeToken(TokenType::PERIOD, ".");
        case ':': return makeToken(TokenType::COLON, ":");
        case '(': return makeToken(TokenType::LEFT_PAREN, "(");
        case ')': return makeToken(TokenType::RIGHT_PAREN, ")");
        case '[': return makeToken(TokenType::LEFT_BRACKET, "[");
        case ']': return makeToken(TokenType::RIGHT_BRACKET, "]");
        case '^': return makeToken(TokenType::CARET, "^");
        case '@': return makeToken(TokenType::AT, "@");
        default:
            addError("Unexpected character: '" + std::string(1, c) + "'");
            return makeErrorToken("Unexpected character");
    }
}

Token Lexer::peekToken() {
    size_t savedCurrent = current_;
    size_t savedLine = line_;
    size_t savedColumn = column_;
    size_t savedLineStart = lineStart_;
    
    Token token = nextToken();
    
    current_ = savedCurrent;
    line_ = savedLine;
    column_ = savedColumn;
    lineStart_ = savedLineStart;
    
    return token;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

SourceLocation Lexer::getCurrentLocation() const {
    return makeLocation();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        lineStart_ = current_;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

bool Lexer::isAtEnd(size_t position) const {
    return position >= source_.length();
}

bool Lexer::isAlpha(char c) const {
    return std::isalpha(c) || c == '_';
}

bool Lexer::isDigit(char c) const {
    return std::isdigit(c);
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

Token Lexer::makeToken(TokenType type, const std::string& value) const {
    return Token(type, value.empty() ? std::string(1, source_[current_ - 1]) : value, makeLocation());
}

Token Lexer::makeErrorToken(const std::string& message) const {
    return Token(TokenType::INVALID, message, makeLocation());
}

Token Lexer::parseIdentifier() {
    SourceLocation startLocation = makeLocation();
    std::string value;
    
    while (!isAtEnd() && isAlphaNumeric(peek())) {
        value += advance();
    }
    
    // Check if it's a keyword
    TokenType type = Keywords::getKeywordType(value);
    return Token(type, value, startLocation);
}

Token Lexer::parseNumber() {
    SourceLocation startLocation = makeLocation();
    std::string value;
    bool hasDecimal = false;
    
    // Parse integer part
    while (!isAtEnd() && isDigit(peek())) {
        value += advance();
    }
    
    // Check for decimal point
    if (!isAtEnd() && peek() == '.' && peekNext() != '.') {
        hasDecimal = true;
        value += advance(); // consume '.'
        
        // Parse fractional part
        while (!isAtEnd() && isDigit(peek())) {
            value += advance();
        }
        
        // Check for scientific notation
        if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) {
            value += advance();
            if (!isAtEnd() && (peek() == '+' || peek() == '-')) {
                value += advance();
            }
            while (!isAtEnd() && isDigit(peek())) {
                value += advance();
            }
        }
    }
    
    TokenType type = hasDecimal ? TokenType::REAL_LITERAL : TokenType::INTEGER_LITERAL;
    return Token(type, value, startLocation);
}

Token Lexer::parseString() {
    SourceLocation startLocation = makeLocation();
    std::string value;
    
    // Skip opening quote
    while (!isAtEnd() && peek() != '\'') {
        if (peek() == '\n') {
            addError("Unterminated string literal");
            break;
        }
        
        char c = advance();
        if (c == '\'' && peek() == '\'') {
            // Escaped quote in Pascal
            value += '\'';
            advance(); // consume second quote
        } else {
            value += c;
        }
    }
    
    if (isAtEnd()) {
        addError("Unterminated string literal");
        return makeErrorToken("Unterminated string");
    }
    
    // Consume closing quote
    advance();
    
    // In Pascal, single quotes with one character are CHAR_LITERAL, 
    // multiple characters (or empty) are STRING_LITERAL
    if (value.length() == 1) {
        return Token(TokenType::CHAR_LITERAL, value, startLocation);
    } else {
        return Token(TokenType::STRING_LITERAL, value, startLocation);
    }
}

Token Lexer::parseCharLiteral() {
    SourceLocation startLocation = makeLocation();
    std::string value;
    
    // Parse character code after #
    while (!isAtEnd() && isDigit(peek())) {
        value += advance();
    }
    
    if (value.empty()) {
        addError("Invalid character literal");
        return makeErrorToken("Invalid character literal");
    }
    
    return Token(TokenType::CHAR_LITERAL, "#" + value, startLocation);
}

Token Lexer::parseComment() {
    skipBlockComment();
    return nextToken(); // Get the next real token
}

Token Lexer::parseOperator() {
    // This is handled in nextToken() for now
    return makeErrorToken("Should not reach parseOperator");
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && isWhitespace(peek())) {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    int nesting = 1;
    
    while (!isAtEnd() && nesting > 0) {
        char c = advance();
        
        if (c == '*' && peek() == ')') {
            advance(); // consume ')'
            nesting--;
        } else if (c == '(' && peek() == '*') {
            advance(); // consume '*'
            nesting++;
        } else if (c == '}') {
            nesting--;
        } else if (c == '{') {
            nesting++;
        }
    }
    
    if (nesting > 0) {
        addError("Unterminated comment");
    }
}

void Lexer::addError(const std::string& message) {
    std::ostringstream oss;
    oss << "Line " << line_ << ", Column " << column_ << ": " << message;
    errors_.push_back(oss.str());
}

SourceLocation Lexer::makeLocation() const {
    return SourceLocation(line_, column_, current_);
}

} // namespace rpascal