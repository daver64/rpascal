#include "../include/token.h"
#include <algorithm>
#include <cctype>

namespace rpascal {

// Keywords mapping (case-insensitive)
const std::unordered_map<std::string, TokenType> Keywords::keywords_ = {
    {"program", TokenType::PROGRAM},
    {"unit", TokenType::UNIT},
    {"uses", TokenType::USES},
    {"const", TokenType::CONST},
    {"type", TokenType::TYPE},
    {"var", TokenType::VAR},
    {"procedure", TokenType::PROCEDURE},
    {"function", TokenType::FUNCTION},
    {"begin", TokenType::BEGIN},
    {"end", TokenType::END},
    {"if", TokenType::IF},
    {"then", TokenType::THEN},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"do", TokenType::DO},
    {"for", TokenType::FOR},
    {"to", TokenType::TO},
    {"downto", TokenType::DOWNTO},
    {"repeat", TokenType::REPEAT},
    {"until", TokenType::UNTIL},
    {"case", TokenType::CASE},
    {"of", TokenType::OF},
    {"with", TokenType::WITH},
    {"goto", TokenType::GOTO},
    {"label", TokenType::LABEL},
    {"record", TokenType::RECORD},
    {"array", TokenType::ARRAY},
    {"set", TokenType::SET},
    {"file", TokenType::FILE},
    {"packed", TokenType::PACKED},
    {"forward", TokenType::FORWARD},
    {"external", TokenType::EXTERNAL},
    {"integer", TokenType::INTEGER},
    {"real", TokenType::REAL},
    {"boolean", TokenType::BOOLEAN},
    {"char", TokenType::CHAR},
    {"string", TokenType::STRING},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"div", TokenType::DIV},
    {"mod", TokenType::MOD},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    {"xor", TokenType::XOR},
    {"shl", TokenType::SHL},
    {"shr", TokenType::SHR}
};

Token::Token(TokenType type, const std::string& value, const SourceLocation& location)
    : type_(type), value_(value), location_(location) {}

Token::Token() : type_(TokenType::INVALID), value_(""), location_(SourceLocation()) {}

bool Token::isKeyword() const {
    return type_ >= TokenType::PROGRAM && type_ <= TokenType::SHR;
}

bool Token::isOperator() const {
    return type_ >= TokenType::PLUS && type_ <= TokenType::SHR;
}

bool Token::isLiteral() const {
    return (type_ >= TokenType::IDENTIFIER && type_ <= TokenType::CHAR_LITERAL) ||
           type_ == TokenType::TRUE || type_ == TokenType::FALSE;
}

bool Token::isPunctuation() const {
    return type_ >= TokenType::SEMICOLON && type_ <= TokenType::RANGE;
}

std::string Token::toString() const {
    return tokenTypeToString(type_) + " '" + value_ + "' at line " + 
           std::to_string(location_.line) + ", column " + std::to_string(location_.column);
}

std::string Token::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::REAL_LITERAL: return "REAL_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::PROGRAM: return "PROGRAM";
        case TokenType::UNIT: return "UNIT";
        case TokenType::USES: return "USES";
        case TokenType::CONST: return "CONST";
        case TokenType::TYPE: return "TYPE";
        case TokenType::VAR: return "VAR";
        case TokenType::PROCEDURE: return "PROCEDURE";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::BEGIN: return "BEGIN";
        case TokenType::END: return "END";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::FOR: return "FOR";
        case TokenType::TO: return "TO";
        case TokenType::DOWNTO: return "DOWNTO";
        case TokenType::REPEAT: return "REPEAT";
        case TokenType::UNTIL: return "UNTIL";
        case TokenType::CASE: return "CASE";
        case TokenType::OF: return "OF";
        case TokenType::WITH: return "WITH";
        case TokenType::GOTO: return "GOTO";
        case TokenType::LABEL: return "LABEL";
        case TokenType::RECORD: return "RECORD";
        case TokenType::ARRAY: return "ARRAY";
        case TokenType::SET: return "SET";
        case TokenType::FILE: return "FILE";
        case TokenType::PACKED: return "PACKED";
        case TokenType::FORWARD: return "FORWARD";
        case TokenType::EXTERNAL: return "EXTERNAL";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::REAL: return "REAL";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::CHAR: return "CHAR";
        case TokenType::STRING: return "STRING";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::DIV: return "DIV";
        case TokenType::MOD: return "MOD";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::XOR: return "XOR";
        case TokenType::SHL: return "SHL";
        case TokenType::SHR: return "SHR";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::PERIOD: return "PERIOD";
        case TokenType::COLON: return "COLON";
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenType::CARET: return "CARET";
        case TokenType::AT: return "AT";
        case TokenType::RANGE: return "RANGE";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::WHITESPACE: return "WHITESPACE";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

TokenType Keywords::getKeywordType(const std::string& word) {
    std::string lowerWord = word;
    std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    auto it = keywords_.find(lowerWord);
    if (it != keywords_.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

bool Keywords::isKeyword(const std::string& word) {
    std::string lowerWord = word;
    std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return keywords_.find(lowerWord) != keywords_.end();
}

} // namespace rpascal