#include "../include/parser.h"
#include <sstream>

namespace rpascal {

Parser::Parser(std::unique_ptr<Lexer> lexer) : lexer_(std::move(lexer)) {
    advance(); // Load first token
}

std::unique_ptr<Program> Parser::parseProgram() {
    try {
        // Parse "program" keyword
        consume(TokenType::PROGRAM, "Expected 'program'");
        
        // Parse program name
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected program name");
        std::string programName = nameToken.getValue();
        
        // Parse semicolon
        consume(TokenType::SEMICOLON, "Expected ';' after program name");
        
        // Parse declarations
        std::vector<std::unique_ptr<Declaration>> declarations;
        while (!check(TokenType::BEGIN) && !isAtEnd()) {
            if (match(TokenType::CONST)) {
                // Handle multiple constant declarations after 'const'
                do {
                    Token constNameToken = consume(TokenType::IDENTIFIER, "Expected constant name");
                    consume(TokenType::EQUAL, "Expected '=' after constant name");
                    auto value = parseExpression();
                    consume(TokenType::SEMICOLON, "Expected ';' after constant declaration");
                    
                    auto constDecl = std::make_unique<ConstantDeclaration>(constNameToken.getValue(), std::move(value));
                    declarations.push_back(std::move(constDecl));
                    
                } while (check(TokenType::IDENTIFIER) && !isAtEnd());
            } else if (match(TokenType::TYPE)) {
                // Handle multiple type definitions after 'type'
                do {
                    Token typeNameToken = consume(TokenType::IDENTIFIER, "Expected type name");
                    consume(TokenType::EQUAL, "Expected '=' after type name");
                    std::string typeDefinition = parseTypeDefinition();
                    consume(TokenType::SEMICOLON, "Expected ';' after type definition");
                    
                    auto typeDecl = std::make_unique<TypeDefinition>(typeNameToken.getValue(), typeDefinition);
                    declarations.push_back(std::move(typeDecl));
                    
                } while (check(TokenType::IDENTIFIER) && !isAtEnd());
            } else if (match(TokenType::VAR)) {
                // Handle multiple variable declarations after 'var'
                do {
                    // Parse variable name(s) - Pascal allows multiple variables of same type
                    std::vector<std::string> varNames;
                    
                    Token varNameToken = consume(TokenType::IDENTIFIER, "Expected variable name");
                    varNames.push_back(varNameToken.getValue());
                    
                    // Handle multiple variables: a, b, c: integer;
                    while (match(TokenType::COMMA)) {
                        Token nextName = consume(TokenType::IDENTIFIER, "Expected variable name after ','");
                        varNames.push_back(nextName.getValue());
                    }
                    
                    consume(TokenType::COLON, "Expected ':' after variable name(s)");
                    std::string typeName = parseTypeName();
                    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
                    
                    // Create a separate declaration for each variable
                    for (const auto& varName : varNames) {
                        auto varDecl = std::make_unique<VariableDeclaration>(varName, typeName);
                        declarations.push_back(std::move(varDecl));
                    }
                    
                } while (check(TokenType::IDENTIFIER) && !isAtEnd());
            } else if (check(TokenType::PROCEDURE)) {
                advance(); // consume PROCEDURE
                auto procDecl = parseProcedureDeclaration();
                if (procDecl) {
                    declarations.push_back(std::move(procDecl));
                }
            } else if (check(TokenType::FUNCTION)) {
                advance(); // consume FUNCTION
                auto funcDecl = parseFunctionDeclaration();
                if (funcDecl) {
                    declarations.push_back(std::move(funcDecl));
                }
            } else {
                addError("Expected declaration");
                synchronize();
                break;
            }
        }
        
        // Parse main block
        auto mainBlock = parseCompoundStatement();
        
        // Parse final period
        consume(TokenType::PERIOD, "Expected '.' after program");
        
        return std::make_unique<Program>(programName, std::move(declarations), std::move(mainBlock));
        
    } catch (const std::exception& e) {
        addError("Failed to parse program: " + std::string(e.what()));
        return nullptr;
    }
}

void Parser::advance() {
    currentToken_ = lexer_->nextToken();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    return currentToken_.getType() == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        Token token = currentToken_;
        advance();
        return token;
    }
    
    addError(message + ". Got: " + currentToken_.toString());
    throw std::runtime_error(message);
}

bool Parser::isAtEnd() const {
    return currentToken_.getType() == TokenType::EOF_TOKEN;
}

void Parser::addError(const std::string& message) {
    std::ostringstream oss;
    oss << "Line " << currentToken_.getLocation().line 
        << ", Column " << currentToken_.getLocation().column 
        << ": " << message;
    errors_.push_back(oss.str());
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (currentToken_.getType() == TokenType::SEMICOLON) {
            advance();
            return;
        }
        
        switch (currentToken_.getType()) {
            case TokenType::VAR:
            case TokenType::PROCEDURE:
            case TokenType::FUNCTION:
            case TokenType::BEGIN:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::unique_ptr<Declaration> Parser::parseDeclaration() {
    try {
        if (match(TokenType::CONST)) {
            return parseConstantDeclaration();
        } else if (match(TokenType::VAR)) {
            return parseVariableDeclaration();
        } else if (check(TokenType::PROCEDURE)) {
            advance(); // consume PROCEDURE
            return parseProcedureDeclaration();
        } else if (check(TokenType::FUNCTION)) {
            advance(); // consume FUNCTION
            return parseFunctionDeclaration();
        } else {
            addError("Expected declaration");
            synchronize();
            return nullptr;
        }
    } catch (const std::exception&) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<ConstantDeclaration> Parser::parseConstantDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected constant name");
    consume(TokenType::EQUAL, "Expected '=' after constant name");
    auto value = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after constant declaration");
    
    return std::make_unique<ConstantDeclaration>(nameToken.getValue(), std::move(value));
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::COLON, "Expected ':' after variable name");
    std::string typeName = parseTypeName();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VariableDeclaration>(nameToken.getValue(), typeName);
}

std::vector<std::unique_ptr<VariableDeclaration>> Parser::parseLocalVariables() {
    std::vector<std::unique_ptr<VariableDeclaration>> localVars;
    
    if (match(TokenType::VAR)) {
        do {
            // Parse variable name(s) - Pascal allows multiple variables of same type
            std::vector<std::string> varNames;
            
            Token varNameToken = consume(TokenType::IDENTIFIER, "Expected variable name");
            varNames.push_back(varNameToken.getValue());
            
            // Handle multiple variables: a, b, c: integer;
            while (match(TokenType::COMMA)) {
                Token nextName = consume(TokenType::IDENTIFIER, "Expected variable name after ','");
                varNames.push_back(nextName.getValue());
            }
            
            consume(TokenType::COLON, "Expected ':' after variable name(s)");
            std::string typeName = parseTypeName();
            consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
            
            // Create a separate declaration for each variable
            for (const auto& varName : varNames) {
                auto varDecl = std::make_unique<VariableDeclaration>(varName, typeName);
                localVars.push_back(std::move(varDecl));
            }
            
        } while (check(TokenType::IDENTIFIER) && !isAtEnd());
    }
    
    return localVars;
}

std::unique_ptr<ProcedureDeclaration> Parser::parseProcedureDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected procedure name");
    
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    if (match(TokenType::LEFT_PAREN)) {
        parameters = parseParameterList();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after procedure header");
    
    // Check for forward declaration
    if (match(TokenType::FORWARD)) {
        consume(TokenType::SEMICOLON, "Expected ';' after 'forward'");
        // For forward declarations, create empty body and local variables
        std::vector<std::unique_ptr<VariableDeclaration>> localVariables;
        auto body = std::make_unique<CompoundStatement>(std::vector<std::unique_ptr<Statement>>());
        return std::make_unique<ProcedureDeclaration>(nameToken.getValue(), std::move(parameters), std::move(localVariables), std::move(body), true);
    }
    
    // Parse local variables (optional var section)
    auto localVariables = parseLocalVariables();
    
    auto body = parseCompoundStatement();
    consume(TokenType::SEMICOLON, "Expected ';' after procedure body");
    
    return std::make_unique<ProcedureDeclaration>(nameToken.getValue(), std::move(parameters), std::move(localVariables), std::move(body));
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name");
    
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    if (match(TokenType::LEFT_PAREN)) {
        parameters = parseParameterList();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    }
    
    consume(TokenType::COLON, "Expected ':' before return type");
    std::string returnType = parseTypeName();
    consume(TokenType::SEMICOLON, "Expected ';' after function header");
    
    // Check for forward declaration
    if (match(TokenType::FORWARD)) {
        consume(TokenType::SEMICOLON, "Expected ';' after 'forward'");
        // For forward declarations, create empty body and local variables
        std::vector<std::unique_ptr<VariableDeclaration>> localVariables;
        auto body = std::make_unique<CompoundStatement>(std::vector<std::unique_ptr<Statement>>());
        return std::make_unique<FunctionDeclaration>(nameToken.getValue(), std::move(parameters), returnType, std::move(localVariables), std::move(body), true);
    }
    
    // Parse local variables (optional var section)
    auto localVariables = parseLocalVariables();
    
    auto body = parseCompoundStatement();
    consume(TokenType::SEMICOLON, "Expected ';' after function body");
    
    return std::make_unique<FunctionDeclaration>(nameToken.getValue(), std::move(parameters), returnType, std::move(localVariables), std::move(body));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    try {
        if (check(TokenType::BEGIN)) {
            return parseCompoundStatement();
        } else if (match(TokenType::IF)) {
            return parseIfStatement();
        } else if (match(TokenType::WHILE)) {
            return parseWhileStatement();
        } else if (match(TokenType::FOR)) {
            return parseForStatement();
        } else if (match(TokenType::REPEAT)) {
            return parseRepeatStatement();
        } else if (match(TokenType::CASE)) {
            return parseCaseStatement();
        } else if (match(TokenType::WITH)) {
            return parseWithStatement();
        } else {
            // Try to parse as assignment or expression statement
            auto expr = parseExpression();
            if (match(TokenType::ASSIGN)) {
                auto value = parseExpression();
                return std::make_unique<AssignmentStatement>(std::move(expr), std::move(value));
            } else {
                return std::make_unique<ExpressionStatement>(std::move(expr));
            }
        }
    } catch (const std::exception&) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<CompoundStatement> Parser::parseCompoundStatement() {
    consume(TokenType::BEGIN, "Expected 'begin'");
    
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!check(TokenType::END) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
        
        // Optional semicolon between statements
        if (match(TokenType::SEMICOLON)) {
            // Continue parsing more statements
        } else if (check(TokenType::END)) {
            // End of compound statement
            break;
        } else {
            addError("Expected ';' or 'end'");
            break;
        }
    }
    
    consume(TokenType::END, "Expected 'end'");
    
    return std::make_unique<CompoundStatement>(std::move(statements));
}

std::unique_ptr<AssignmentStatement> Parser::parseAssignmentStatement(std::unique_ptr<Expression> target) {
    consume(TokenType::ASSIGN, "Expected ':='");
    auto value = parseExpression();
    return std::make_unique<AssignmentStatement>(std::move(target), std::move(value));
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    auto condition = parseExpression();
    consume(TokenType::THEN, "Expected 'then'");
    auto thenStmt = parseStatement();
    
    std::unique_ptr<Statement> elseStmt = nullptr;
    if (match(TokenType::ELSE)) {
        elseStmt = parseStatement();
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenStmt), std::move(elseStmt));
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    auto condition = parseExpression();
    consume(TokenType::DO, "Expected 'do'");
    auto body = parseStatement();
    
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<ForStatement> Parser::parseForStatement() {
    // Parse: for variable := start to/downto end do statement
    Token variableToken = consume(TokenType::IDENTIFIER, "Expected variable name");
    std::string variable = variableToken.getValue();
    
    consume(TokenType::ASSIGN, "Expected ':='");
    auto start = parseExpression();
    
    bool isDownto = false;
    if (match(TokenType::TO)) {
        isDownto = false;
    } else if (match(TokenType::DOWNTO)) {
        isDownto = true;
    } else {
        throw std::runtime_error("Expected 'to' or 'downto'");
    }
    
    auto end = parseExpression();
    consume(TokenType::DO, "Expected 'do'");
    auto body = parseStatement();
    
    return std::make_unique<ForStatement>(variable, std::move(start), std::move(end), 
                                        isDownto, std::move(body));
}

std::unique_ptr<RepeatStatement> Parser::parseRepeatStatement() {
    // Parse: repeat statements until condition
    // In Pascal, repeat-until can contain multiple statements without BEGIN/END
    
    std::vector<std::unique_ptr<Statement>> statements;
    
    // Parse statements until we see UNTIL
    while (!check(TokenType::UNTIL) && !isAtEnd()) {
        statements.push_back(parseStatement());
        
        // Consume optional semicolon between statements
        if (check(TokenType::SEMICOLON)) {
            advance();
        }
    }
    
    // Create a compound statement for the body if we have multiple statements
    std::unique_ptr<Statement> body;
    if (statements.size() == 1) {
        body = std::move(statements[0]);
    } else {
        body = std::make_unique<CompoundStatement>(std::move(statements));
    }
    
    consume(TokenType::UNTIL, "Expected 'until'");
    auto condition = parseExpression();
    
    return std::make_unique<RepeatStatement>(std::move(body), std::move(condition));
}

std::unique_ptr<CaseStatement> Parser::parseCaseStatement() {
    // Parse: case expression of value1: stmt1; value2: stmt2; else stmt3; end
    auto expression = parseExpression();
    consume(TokenType::OF, "Expected 'of'");
    
    std::vector<std::unique_ptr<CaseBranch>> branches;
    std::unique_ptr<Statement> elseClause = nullptr;
    
    // Parse case branches
    while (!check(TokenType::ELSE) && !check(TokenType::END) && !isAtEnd()) {
        // Parse case values (can be multiple: value1, value2, value3)
        std::vector<std::unique_ptr<Expression>> values;
        values.push_back(parseExpression());
        
        // Parse additional values separated by commas
        while (match(TokenType::COMMA)) {
            values.push_back(parseExpression());
        }
        
        consume(TokenType::COLON, "Expected ':' after case value");
        auto statement = parseStatement();
        
        branches.push_back(std::make_unique<CaseBranch>(std::move(values), std::move(statement)));
        
        // Consume optional semicolon
        if (check(TokenType::SEMICOLON)) {
            advance();
        }
    }
    
    // Parse optional else clause
    if (match(TokenType::ELSE)) {
        elseClause = parseStatement();
    }
    
    consume(TokenType::END, "Expected 'end'");
    
    return std::make_unique<CaseStatement>(std::move(expression), std::move(branches), std::move(elseClause));
}

std::unique_ptr<WithStatement> Parser::parseWithStatement() {
    // Parse: with expr1, expr2 do statement
    
    std::vector<std::unique_ptr<Expression>> withExpressions;
    
    // Parse first expression
    withExpressions.push_back(parseExpression());
    
    // Parse additional expressions separated by commas
    while (match(TokenType::COMMA)) {
        withExpressions.push_back(parseExpression());
    }
    
    consume(TokenType::DO, "Expected 'do' after with expressions");
    
    auto body = parseStatement();
    
    return std::make_unique<WithStatement>(std::move(withExpressions), std::move(body));
}

std::unique_ptr<ExpressionStatement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseOrExpression();
}

std::unique_ptr<Expression> Parser::parseOrExpression() {
    auto expr = parseAndExpression();
    
    while (check(TokenType::OR)) {
        Token op = currentToken_;
        advance();
        auto right = parseAndExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAndExpression() {
    auto expr = parseEqualityExpression();
    
    while (check(TokenType::AND)) {
        Token op = currentToken_;
        advance();
        auto right = parseEqualityExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseEqualityExpression() {
    auto expr = parseRelationalExpression();
    
    while (check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL)) {
        Token op = currentToken_;
        advance();
        auto right = parseRelationalExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseRelationalExpression() {
    auto expr = parseAdditiveExpression();
    
    while (check(TokenType::LESS_THAN) || check(TokenType::LESS_EQUAL) ||
           check(TokenType::GREATER_THAN) || check(TokenType::GREATER_EQUAL) ||
           check(TokenType::IN)) {
        Token op = currentToken_;
        advance();
        auto right = parseAdditiveExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAdditiveExpression() {
    auto expr = parseMultiplicativeExpression();
    
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = currentToken_;
        advance();
        auto right = parseMultiplicativeExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseMultiplicativeExpression() {
    auto expr = parseUnaryExpression();
    
    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE) || 
           check(TokenType::DIV) || check(TokenType::MOD)) {
        Token op = currentToken_;
        advance();
        auto right = parseUnaryExpression();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnaryExpression() {
    if (check(TokenType::NOT) || check(TokenType::MINUS) || check(TokenType::PLUS)) {
        Token op = currentToken_;
        advance();
        auto operand = parseUnaryExpression();
        return std::make_unique<UnaryExpression>(op, std::move(operand));
    }
    
    if (check(TokenType::AT)) {
        advance(); // consume '@'
        auto operand = parseUnaryExpression();
        return std::make_unique<AddressOfExpression>(std::move(operand));
    }
    
    return parsePrimaryExpression();
}

std::unique_ptr<Expression> Parser::parsePrimaryExpression() {
    if (check(TokenType::TRUE) || check(TokenType::FALSE) || check(TokenType::NIL) ||
        check(TokenType::INTEGER_LITERAL) || check(TokenType::REAL_LITERAL) ||
        check(TokenType::STRING_LITERAL) || check(TokenType::CHAR_LITERAL)) {
        Token token = currentToken_;
        advance();
        return std::make_unique<LiteralExpression>(token);
    }
    
    if (check(TokenType::IDENTIFIER)) {
        Token nameToken = currentToken_;
        advance();
        std::unique_ptr<Expression> expr = std::make_unique<IdentifierExpression>(nameToken.getValue());
        
        // Handle postfix operations (function calls, field access, array indexing)
        while (true) {
            if (check(TokenType::LEFT_PAREN)) {
                // Function call
                advance(); // consume '('
                auto args = parseArgumentList();
                consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
                expr = std::make_unique<CallExpression>(std::move(expr), std::move(args));
            } else if (check(TokenType::PERIOD)) {
                // Field access
                advance(); // consume '.'
                Token fieldToken = consume(TokenType::IDENTIFIER, "Expected field name after '.'");
                expr = std::make_unique<FieldAccessExpression>(std::move(expr), fieldToken.getValue());
            } else if (check(TokenType::LEFT_BRACKET)) {
                // Array indexing - handle multiple dimensions
                advance(); // consume '['
                
                std::vector<std::unique_ptr<Expression>> indices;
                indices.push_back(parseExpression());
                
                // Parse additional dimensions separated by commas
                while (check(TokenType::COMMA)) {
                    advance(); // consume ','
                    indices.push_back(parseExpression());
                }
                
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index");
                expr = std::make_unique<ArrayIndexExpression>(std::move(expr), std::move(indices));
            } else if (check(TokenType::CARET)) {
                // Pointer dereference (postfix)
                advance(); // consume '^'
                expr = std::make_unique<DereferenceExpression>(std::move(expr));
            } else {
                break; // No more postfix operations
            }
        }
        
        return std::move(expr);
    }
    
    if (check(TokenType::LEFT_PAREN)) {
        advance(); // consume '('
        auto expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (check(TokenType::LEFT_BRACKET)) {
        // Set literal: [element1, element2, ...]
        advance(); // consume '['
        std::vector<std::unique_ptr<Expression>> elements;
        
        if (!check(TokenType::RIGHT_BRACKET)) {
            elements.push_back(parseExpression());
            
            while (match(TokenType::COMMA)) {
                elements.push_back(parseExpression());
            }
        }
        
        consume(TokenType::RIGHT_BRACKET, "Expected ']' after set elements");
        return std::make_unique<SetLiteralExpression>(std::move(elements));
    }
    
    addError("Expected expression");
    throw std::runtime_error("Expected expression");
}

std::unique_ptr<Expression> Parser::parseCallExpression(std::unique_ptr<Expression> callee) {
    auto args = parseArgumentList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
    return std::make_unique<CallExpression>(std::move(callee), std::move(args));
}

std::string Parser::parseTypeName() {
    // Handle pointer types: ^Type
    if (check(TokenType::CARET)) {
        advance(); // consume '^'
        std::string pointeeType = parseTypeName(); // recursive call for the pointed-to type
        return "^" + pointeeType;
    }
    
    // Accept both IDENTIFIER and type keywords as type names
    if (check(TokenType::IDENTIFIER) || 
        check(TokenType::INTEGER) || check(TokenType::REAL) || 
        check(TokenType::BOOLEAN) || check(TokenType::CHAR) ||
        check(TokenType::TEXT) || check(TokenType::FILE)) {
        Token typeToken = currentToken_;
        advance();
        return typeToken.getValue();
    } else if (check(TokenType::STRING)) {
        advance(); // consume 'string'
        
        // Check for bounded string: string[N]
        if (check(TokenType::LEFT_BRACKET)) {
            advance(); // consume '['
            
            // Parse the size
            if (check(TokenType::INTEGER_LITERAL)) {
                std::string size = currentToken_.getValue();
                advance();
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after string size");
                return "string[" + size + "]";
            } else {
                addError("Expected integer size in bounded string");
                throw std::runtime_error("Expected integer size in bounded string");
            }
        } else {
            // Unbounded string
            return "string";
        }
    }
    
    addError("Expected type name");
    throw std::runtime_error("Expected type name");
}

std::string Parser::parseTypeDefinition() {
    // For now, implement a simple type definition parser
    // This will handle basic cases and can be expanded later
    
    if (check(TokenType::INTEGER_LITERAL) || check(TokenType::CHAR_LITERAL)) {
        // Range type: 1..10 or 'A'..'Z'
        Token startToken = currentToken_;
        advance();
        consume(TokenType::RANGE, "Expected '..' in range type");
        
        TokenType expectedEndType = startToken.getType();
        Token endToken = consume(expectedEndType, 
            expectedEndType == TokenType::INTEGER_LITERAL ? 
            "Expected integer end value in range type" : 
            "Expected character end value in range type");
            
        // For character literals, preserve the quotes in the definition
        if (startToken.getType() == TokenType::CHAR_LITERAL) {
            return "'" + startToken.getValue() + "'.." + "'" + endToken.getValue() + "'";
        } else {
            return startToken.getValue() + ".." + endToken.getValue();
        }
    } else if (check(TokenType::LEFT_PAREN)) {
        // Enumeration type: (Red, Green, Blue)
        advance(); // consume '('
        std::string enumDef = "(";
        
        do {
            Token enumValue = consume(TokenType::IDENTIFIER, "Expected enumeration value");
            enumDef += enumValue.getValue();
            
            if (check(TokenType::COMMA)) {
                advance();
                enumDef += ", ";
            }
        } while (check(TokenType::IDENTIFIER));
        
        consume(TokenType::RIGHT_PAREN, "Expected ')' after enumeration");
        enumDef += ")";
        return enumDef;
    } else if (check(TokenType::SET)) {
        // Set type: set of type
        advance(); // consume 'set'
        consume(TokenType::OF, "Expected 'of' after 'set'");
        
        std::string elementType = parseTypeName();
        return "set of " + elementType;
    } else if (check(TokenType::RECORD)) {
        // Record type: record ... end
        advance(); // consume 'record'
        std::string recordDef = "record ";
        
        // For now, just consume until 'end' - we'll implement proper parsing later
        while (!check(TokenType::END) && !isAtEnd()) {
            recordDef += currentToken_.getValue() + " ";
            advance();
        }
        
        consume(TokenType::END, "Expected 'end' after record definition");
        recordDef += "end";
        return recordDef;
    } else if (check(TokenType::ARRAY)) {
        // Array type: array[1..5] of integer
        advance(); // consume 'array'
        std::string arrayDef = "array";
        
        if (check(TokenType::LEFT_BRACKET)) {
            advance(); // consume '['
            arrayDef += "[";
            
            // Parse array bounds - preserve token format for proper parsing
            while (!check(TokenType::RIGHT_BRACKET) && !isAtEnd()) {
                if (currentToken_.getType() == TokenType::CHAR_LITERAL) {
                    arrayDef += "'" + currentToken_.getValue() + "'";
                } else {
                    arrayDef += currentToken_.getValue();
                }
                advance();
            }
            
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array bounds");
            arrayDef += "]";
        }
        
        consume(TokenType::OF, "Expected 'of' after array bounds");
        arrayDef += " of ";
        
        std::string elementType = parseTypeName();
        arrayDef += elementType;
        
        return arrayDef;
    } else {
        // Simple type alias
        return parseTypeName();
    }
}

std::vector<std::unique_ptr<VariableDeclaration>> Parser::parseParameterList() {
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            // Check for parameter mode: var, const, or value (default)
            ParameterMode paramMode = ParameterMode::VALUE;
            if (match(TokenType::VAR)) {
                paramMode = ParameterMode::VAR;
            } else if (match(TokenType::CONST)) {
                paramMode = ParameterMode::CONST;
            }
            
            // Parse parameter names - Pascal allows multiple parameters of same type: a, b, c: integer
            std::vector<std::string> paramNames;
            
            Token nameToken = consume(TokenType::IDENTIFIER, "Expected parameter name");
            paramNames.push_back(nameToken.getValue());
            
            // Handle multiple parameters with same type: a, b, c: integer
            while (match(TokenType::COMMA)) {
                Token nextName = consume(TokenType::IDENTIFIER, "Expected parameter name after ','");
                paramNames.push_back(nextName.getValue());
            }
            
            consume(TokenType::COLON, "Expected ':' after parameter name(s)");
            std::string typeName = parseTypeName();
            
            // Create a separate parameter declaration for each parameter name
            for (const auto& paramName : paramNames) {
                auto param = std::make_unique<VariableDeclaration>(paramName, typeName);
                param->setParameterMode(paramMode);
                parameters.push_back(std::move(param));
            }
            
        } while (match(TokenType::SEMICOLON));
    }
    
    return parameters;
}

std::vector<std::unique_ptr<Expression>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    return arguments;
}

bool Parser::isAssignmentOperator() const {
    return currentToken_.getType() == TokenType::ASSIGN;
}

bool Parser::isBinaryOperator() const {
    switch (currentToken_.getType()) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::DIV:
        case TokenType::MOD:
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_THAN:
        case TokenType::GREATER_EQUAL:
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::XOR:
            return true;
        default:
            return false;
    }
}

int Parser::getOperatorPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::OR:
            return 1;
        case TokenType::AND:
            return 2;
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_THAN:
        case TokenType::GREATER_EQUAL:
            return 3;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 4;
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::DIV:
        case TokenType::MOD:
            return 5;
        default:
            return 0;
    }
}

bool Parser::isRightAssociative(TokenType) const {
    // Most Pascal operators are left-associative
    return false;
}

} // namespace rpascal