#pragma once

#include "token.h"
#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>
#include <string>

namespace rpascal {

class Parser {
public:
    explicit Parser(std::unique_ptr<Lexer> lexer);
    
    // Parse the entire program
    std::unique_ptr<Program> parseProgram();
    
    // Error handling
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    
private:
    std::unique_ptr<Lexer> lexer_;
    Token currentToken_;
    std::vector<std::string> errors_;
    
    // Token management
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& message);
    bool isAtEnd() const;
    
    // Error handling
    void addError(const std::string& message);
    void synchronize();
    
    // Parsing methods
    std::unique_ptr<Declaration> parseDeclaration();
    std::unique_ptr<ConstantDeclaration> parseConstantDeclaration();
    std::unique_ptr<VariableDeclaration> parseVariableDeclaration();
    std::vector<std::unique_ptr<VariableDeclaration>> parseLocalVariables();
    std::unique_ptr<ProcedureDeclaration> parseProcedureDeclaration();
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<CompoundStatement> parseCompoundStatement();
    std::unique_ptr<AssignmentStatement> parseAssignmentStatement(std::unique_ptr<Expression> target);
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<ForStatement> parseForStatement();
    std::unique_ptr<RepeatStatement> parseRepeatStatement();
    std::unique_ptr<ExpressionStatement> parseExpressionStatement();
    
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseOrExpression();
    std::unique_ptr<Expression> parseAndExpression();
    std::unique_ptr<Expression> parseEqualityExpression();
    std::unique_ptr<Expression> parseRelationalExpression();
    std::unique_ptr<Expression> parseAdditiveExpression();
    std::unique_ptr<Expression> parseMultiplicativeExpression();
    std::unique_ptr<Expression> parseUnaryExpression();
    std::unique_ptr<Expression> parsePrimaryExpression();
    std::unique_ptr<Expression> parseCallExpression(std::unique_ptr<Expression> callee);
    
    // Helper methods
    std::string parseTypeName();
    std::string parseTypeDefinition();
    std::vector<std::unique_ptr<VariableDeclaration>> parseParameterList();
    std::vector<std::unique_ptr<Expression>> parseArgumentList();
    
    // Precedence helpers
    bool isAssignmentOperator() const;
    bool isBinaryOperator() const;
    int getOperatorPrecedence(TokenType type) const;
    bool isRightAssociative(TokenType type) const;
};

} // namespace rpascal