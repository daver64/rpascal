#include "../include/type_checker.h"
#include <iostream>

namespace rpascal {

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<SymbolTable> symbolTable)
    : symbolTable_(symbolTable), currentExpressionType_(DataType::UNKNOWN) {}

bool SemanticAnalyzer::analyze(Program& program) {
    errors_.clear();
    program.accept(*this);
    
    // Combine errors from symbol table
    if (symbolTable_->hasErrors()) {
        for (const auto& error : symbolTable_->getErrors()) {
            errors_.push_back(error);
        }
    }
    
    return !hasErrors();
}

bool SemanticAnalyzer::hasErrors() const {
    return !errors_.empty() || symbolTable_->hasErrors();
}

const std::vector<std::string>& SemanticAnalyzer::getErrors() const {
    return errors_;
}

void SemanticAnalyzer::visit(LiteralExpression& node) {
    const Token& token = node.getToken();
    
    switch (token.getType()) {
        case TokenType::INTEGER_LITERAL:
            currentExpressionType_ = DataType::INTEGER;
            break;
        case TokenType::REAL_LITERAL:
            currentExpressionType_ = DataType::REAL;
            break;
        case TokenType::STRING_LITERAL:
            currentExpressionType_ = DataType::STRING;
            break;
        case TokenType::CHAR_LITERAL:
            currentExpressionType_ = DataType::CHAR;
            break;
        case TokenType::TRUE:
        case TokenType::FALSE:
            currentExpressionType_ = DataType::BOOLEAN;
            break;
        default:
            currentExpressionType_ = DataType::UNKNOWN;
            addError("Unknown literal type: " + token.getValue());
    }
}

void SemanticAnalyzer::visit(IdentifierExpression& node) {
    auto symbol = symbolTable_->lookup(node.getName());
    if (!symbol) {
        addError("Undefined identifier: " + node.getName());
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    if (symbol->getSymbolType() == SymbolType::FUNCTION) {
        // Function name used as expression (for return value assignment)
        currentExpressionType_ = symbol->getReturnType();
    } else {
        currentExpressionType_ = symbol->getDataType();
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node) {
    // Analyze operands
    node.getLeft()->accept(*this);
    DataType leftType = currentExpressionType_;
    
    node.getRight()->accept(*this);
    DataType rightType = currentExpressionType_;
    
    TokenType operator_ = node.getOperator().getType();
    
    if (!isValidBinaryOperation(leftType, rightType, operator_)) {
        addError("Invalid binary operation: " + 
                SymbolTable::dataTypeToString(leftType) + " " + 
                node.getOperator().getValue() + " " + 
                SymbolTable::dataTypeToString(rightType));
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    currentExpressionType_ = getResultType(leftType, rightType, operator_);
}

void SemanticAnalyzer::visit(UnaryExpression& node) {
    node.getOperand()->accept(*this);
    DataType operandType = currentExpressionType_;
    TokenType operator_ = node.getOperator().getType();
    
    if (!isValidUnaryOperation(operandType, operator_)) {
        addError("Invalid unary operation: " + node.getOperator().getValue() + 
                SymbolTable::dataTypeToString(operandType));
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Result type is usually the same as operand type for unary operations
    // except for 'not' which always returns boolean
    if (operator_ == TokenType::NOT) {
        currentExpressionType_ = DataType::BOOLEAN;
    } else {
        currentExpressionType_ = operandType;
    }
}

void SemanticAnalyzer::visit(CallExpression& node) {
    checkFunctionCall(node);
}

void SemanticAnalyzer::visit(FieldAccessExpression& node) {
    // Visit the object expression to ensure it's valid
    node.getObject()->accept(*this);
    
    // For now, just accept any field access on custom types
    // TODO: Add proper record field validation by storing record structure
}

void SemanticAnalyzer::visit(ArrayIndexExpression& node) {
    // Visit both expressions to ensure they're valid
    node.getArray()->accept(*this);
    node.getIndex()->accept(*this);
    
    // TODO: Add proper array bounds checking and ensure index is integer type
}

void SemanticAnalyzer::visit(ExpressionStatement& node) {
    node.getExpression()->accept(*this);
}

void SemanticAnalyzer::visit(CompoundStatement& node) {
    for (const auto& stmt : node.getStatements()) {
        stmt->accept(*this);
    }
}

void SemanticAnalyzer::visit(AssignmentStatement& node) {
    checkAssignment(node.getTarget(), node.getValue());
}

void SemanticAnalyzer::visit(IfStatement& node) {
    // Check condition
    node.getCondition()->accept(*this);
    if (currentExpressionType_ != DataType::BOOLEAN && currentExpressionType_ != DataType::UNKNOWN) {
        addError("If condition must be boolean, got " + SymbolTable::dataTypeToString(currentExpressionType_));
    }
    
    // Check then statement
    node.getThenStatement()->accept(*this);
    
    // Check else statement if present
    if (node.getElseStatement()) {
        node.getElseStatement()->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStatement& node) {
    // Check condition
    node.getCondition()->accept(*this);
    if (currentExpressionType_ != DataType::BOOLEAN && currentExpressionType_ != DataType::UNKNOWN) {
        addError("While condition must be boolean, got " + SymbolTable::dataTypeToString(currentExpressionType_));
    }
    
    // Check body
    node.getBody()->accept(*this);
}

void SemanticAnalyzer::visit(ConstantDeclaration& node) {
    // Analyze the constant value expression
    node.getValue()->accept(*this);
    
    // Register the constant in the symbol table
    symbolTable_->define(node.getName(), SymbolType::CONSTANT, currentExpressionType_);
}

void SemanticAnalyzer::visit(TypeDefinition& node) {
    // Register the type definition in the symbol table as a custom type
    symbolTable_->define(node.getName(), SymbolType::TYPE_DEF, DataType::CUSTOM);
}

void SemanticAnalyzer::visit(VariableDeclaration& node) {
    DataType dataType = symbolTable_->resolveDataType(node.getType());
    if (dataType == DataType::UNKNOWN) {
        addError("Unknown data type: " + node.getType());
        return;
    }
    
    symbolTable_->define(node.getName(), SymbolType::VARIABLE, dataType);
    
    // Check initializer if present
    if (node.getInitializer()) {
        node.getInitializer()->accept(*this);
        if (!areTypesCompatible(dataType, currentExpressionType_)) {
            addError("Type mismatch in variable initialization: cannot assign " +
                    SymbolTable::dataTypeToString(currentExpressionType_) + " to " +
                    SymbolTable::dataTypeToString(dataType));
        }
    }
}

void SemanticAnalyzer::visit(ProcedureDeclaration& node) {
    // Enter new scope for procedure
    symbolTable_->enterScope();
    
    // Create procedure symbol
    auto procedureSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::PROCEDURE, DataType::VOID, 
                                                   symbolTable_->getCurrentScopeLevel() - 1);
    
    // Add parameters to procedure symbol and current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = SymbolTable::stringToDataType(param->getType());
        procedureSymbol->addParameter(param->getName(), paramType);
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    // Define procedure in parent scope
    symbolTable_->exitScope();
    symbolTable_->define(node.getName(), procedureSymbol);
    symbolTable_->enterScope();
    
    // Re-add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = SymbolTable::stringToDataType(param->getType());
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    // Add local variables to current scope
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    // Analyze procedure body
    node.getBody()->accept(*this);
    
    // Exit procedure scope
    symbolTable_->exitScope();
}

void SemanticAnalyzer::visit(FunctionDeclaration& node) {
    // Enter new scope for function
    symbolTable_->enterScope();
    
    DataType returnType = SymbolTable::stringToDataType(node.getReturnType());
    if (returnType == DataType::UNKNOWN) {
        addError("Unknown return type: " + node.getReturnType());
        returnType = DataType::VOID;
    }
    
    // Create function symbol
    auto functionSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::FUNCTION, returnType,
                                                  symbolTable_->getCurrentScopeLevel() - 1);
    functionSymbol->setReturnType(returnType);
    
    // Add parameters to function symbol and current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = SymbolTable::stringToDataType(param->getType());
        functionSymbol->addParameter(param->getName(), paramType);
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    // Define function in parent scope
    symbolTable_->exitScope();
    symbolTable_->define(node.getName(), functionSymbol);
    symbolTable_->enterScope();
    
    // Re-add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = SymbolTable::stringToDataType(param->getType());
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    // Add function name as a variable for return value assignment
    symbolTable_->define(node.getName(), SymbolType::VARIABLE, returnType);
    
    // Add local variables to current scope
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    currentFunctionName_ = node.getName();
    
    // Analyze function body
    node.getBody()->accept(*this);
    
    currentFunctionName_ = "";
    
    // Exit function scope
    symbolTable_->exitScope();
}

void SemanticAnalyzer::visit(Program& node) {
    // Analyze declarations
    for (const auto& decl : node.getDeclarations()) {
        decl->accept(*this);
    }
    
    // Analyze main block
    node.getMainBlock()->accept(*this);
}

void SemanticAnalyzer::addError(const std::string& message) {
    errors_.push_back("Semantic error: " + message);
}

DataType SemanticAnalyzer::getExpressionType(Expression* expr) {
    expr->accept(*this);
    return currentExpressionType_;
}

bool SemanticAnalyzer::areTypesCompatible(DataType left, DataType right) {
    if (left == right) return true;
    
    // Integer and real are compatible
    if ((left == DataType::INTEGER && right == DataType::REAL) ||
        (left == DataType::REAL && right == DataType::INTEGER)) {
        return true;
    }
    
    return false;
}

DataType SemanticAnalyzer::getResultType(DataType left, DataType right, TokenType operator_) {
    // Comparison operators always return boolean
    if (operator_ == TokenType::EQUAL || operator_ == TokenType::NOT_EQUAL ||
        operator_ == TokenType::LESS_THAN || operator_ == TokenType::LESS_EQUAL ||
        operator_ == TokenType::GREATER_THAN || operator_ == TokenType::GREATER_EQUAL) {
        return DataType::BOOLEAN;
    }
    
    // Logical operators
    if (operator_ == TokenType::AND || operator_ == TokenType::OR || operator_ == TokenType::XOR) {
        return DataType::BOOLEAN;
    }
    
    // Arithmetic operators
    if (operator_ == TokenType::PLUS || operator_ == TokenType::MINUS ||
        operator_ == TokenType::MULTIPLY || operator_ == TokenType::DIVIDE) {
        // If either operand is real, result is real
        if (left == DataType::REAL || right == DataType::REAL) {
            return DataType::REAL;
        }
        return DataType::INTEGER;
    }
    
    // Integer division operators
    if (operator_ == TokenType::DIV || operator_ == TokenType::MOD) {
        return DataType::INTEGER;
    }
    
    return left; // Default to left operand type
}

bool SemanticAnalyzer::isValidUnaryOperation(DataType operandType, TokenType operator_) {
    switch (operator_) {
        case TokenType::PLUS:
        case TokenType::MINUS:
            return operandType == DataType::INTEGER || operandType == DataType::REAL;
        case TokenType::NOT:
            return operandType == DataType::BOOLEAN;
        default:
            return false;
    }
}

bool SemanticAnalyzer::isValidBinaryOperation(DataType leftType, DataType rightType, TokenType operator_) {
    // Skip checks for unknown types (errors already reported)
    if (leftType == DataType::UNKNOWN || rightType == DataType::UNKNOWN) {
        return true;
    }
    
    switch (operator_) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
            return (leftType == DataType::INTEGER || leftType == DataType::REAL) &&
                   (rightType == DataType::INTEGER || rightType == DataType::REAL);
                   
        case TokenType::DIV:
        case TokenType::MOD:
            return leftType == DataType::INTEGER && rightType == DataType::INTEGER;
            
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
            return areTypesCompatible(leftType, rightType);
            
        case TokenType::LESS_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_THAN:
        case TokenType::GREATER_EQUAL:
            return (leftType == DataType::INTEGER || leftType == DataType::REAL) &&
                   (rightType == DataType::INTEGER || rightType == DataType::REAL);
                   
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::XOR:
            return leftType == DataType::BOOLEAN && rightType == DataType::BOOLEAN;
            
        default:
            return false;
    }
}

void SemanticAnalyzer::checkFunctionCall(CallExpression& node) {
    // Get callee name
    auto calleeExpr = dynamic_cast<IdentifierExpression*>(node.getCallee());
    if (!calleeExpr) {
        addError("Invalid function call");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    std::string functionName = calleeExpr->getName();
    auto symbol = symbolTable_->lookup(functionName);
    
    if (!symbol) {
        addError("Undefined function: " + functionName);
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    if (symbol->getSymbolType() != SymbolType::FUNCTION && 
        symbol->getSymbolType() != SymbolType::PROCEDURE) {
        addError("'" + functionName + "' is not a function or procedure");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Special handling for built-in functions like writeln (variable arguments)
    if (functionName == "writeln" || functionName == "readln") {
        // Accept any number and type of arguments for now
        for (const auto& arg : node.getArguments()) {
            arg->accept(*this);
        }
        currentExpressionType_ = DataType::VOID;
        return;
    }
    
    // Check parameter count
    const auto& expectedParams = symbol->getParameters();
    const auto& actualArgs = node.getArguments();
    
    if (expectedParams.size() != actualArgs.size()) {
        addError("Function '" + functionName + "' expects " + 
                std::to_string(expectedParams.size()) + " arguments, got " + 
                std::to_string(actualArgs.size()));
    }
    
    // Check parameter types
    size_t minParams = std::min(expectedParams.size(), actualArgs.size());
    for (size_t i = 0; i < minParams; ++i) {
        DataType actualType = getExpressionType(actualArgs[i].get());
        DataType expectedType = expectedParams[i].second;
        
        if (!areTypesCompatible(expectedType, actualType)) {
            addError("Argument " + std::to_string(i + 1) + " type mismatch: expected " +
                    SymbolTable::dataTypeToString(expectedType) + ", got " +
                    SymbolTable::dataTypeToString(actualType));
        }
    }
    
    currentExpressionType_ = symbol->getReturnType();
}

void SemanticAnalyzer::checkAssignment(Expression* target, Expression* value) {
    // Check if target is assignable
    auto targetId = dynamic_cast<IdentifierExpression*>(target);
    auto targetField = dynamic_cast<FieldAccessExpression*>(target);
    auto targetArray = dynamic_cast<ArrayIndexExpression*>(target);
    
    std::shared_ptr<Symbol> targetSymbol = nullptr;
    
    if (targetId) {
        // Simple variable assignment
        targetSymbol = symbolTable_->lookup(targetId->getName());
        if (!targetSymbol) {
            addError("Undefined variable: " + targetId->getName());
            return;
        }
        
        if (targetSymbol->getSymbolType() != SymbolType::VARIABLE &&
            targetSymbol->getSymbolType() != SymbolType::PARAMETER &&
            targetSymbol->getSymbolType() != SymbolType::FUNCTION) {
            addError("Cannot assign to " + targetId->getName());
            return;
        }
    } else if (targetField || targetArray) {
        // Field access or array indexing - these are valid assignment targets
        // For now, skip detailed type checking and allow the assignment
        // TODO: Add more detailed validation for field/array types
        return; // Skip type compatibility check for now
    } else {
        addError("Invalid assignment target");
        return;
    }
    
    // Get types (only for simple variable assignments)
    DataType targetType = targetSymbol->getDataType();
    DataType valueType = getExpressionType(value);
    
    if (!areTypesCompatible(targetType, valueType)) {
        addError("Type mismatch in assignment: cannot assign " +
                SymbolTable::dataTypeToString(valueType) + " to " +
                SymbolTable::dataTypeToString(targetType));
    }
}

} // namespace rpascal