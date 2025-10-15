#include "../include/type_checker.h"
#include <iostream>

namespace rpascal {

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<SymbolTable> symbolTable)
    : symbolTable_(symbolTable), currentExpressionType_(DataType::UNKNOWN), 
      currentPointeeType_(DataType::UNKNOWN), unitLoader_(std::make_unique<UnitLoader>()) {}

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
        case TokenType::NIL:
            currentExpressionType_ = DataType::POINTER;
            break;
        default:
            currentExpressionType_ = DataType::UNKNOWN;
            addError("Unknown literal type: " + token.getValue());
    }
}

void SemanticAnalyzer::visit(IdentifierExpression& node) {
    auto symbol = symbolTable_->lookup(node.getName());
    if (!symbol) {
        // Check if this identifier could be a field in a with context
        bool foundInWithContext = false;
        
        for (auto it = withContextStack_.rbegin(); it != withContextStack_.rend(); ++it) {
            const WithContext& context = *it;
            
            if (context.recordType == DataType::CUSTOM && !context.recordTypeName.empty()) {
                // Look up the record type definition
                auto recordTypeSymbol = symbolTable_->lookup(context.recordTypeName);
                if (recordTypeSymbol && recordTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string recordDef = recordTypeSymbol->getTypeDefinition();
                    
                    // Check if the identifier is a field in this record
                    std::string fieldType = getFieldTypeFromRecord(node.getName(), recordDef);
                    if (!fieldType.empty()) {
                        // Resolve the field type
                        DataType resolvedType = symbolTable_->stringToDataType(fieldType);
                        if (resolvedType == DataType::UNKNOWN) {
                            resolvedType = symbolTable_->resolveDataType(fieldType);
                        }
                        
                        currentExpressionType_ = resolvedType;
                        foundInWithContext = true;
                        
                        // Mark that this identifier was resolved in with context
                        node.setWithVariable(context.withVariable);
                        break;
                    }
                }
            }
        }
        
        if (!foundInWithContext) {
            addError("Undefined identifier: " + node.getName());
            currentExpressionType_ = DataType::UNKNOWN;
        }
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

void SemanticAnalyzer::visit(AddressOfExpression& node) {
    node.getOperand()->accept(*this);
    DataType operandType = currentExpressionType_;
    
    // The address-of operator always returns a pointer to the operand's type
    // The result type is POINTER, and we store the pointee type information
    currentExpressionType_ = DataType::POINTER;
    currentPointeeType_ = operandType; // Store what we're taking address of
}

void SemanticAnalyzer::visit(DereferenceExpression& node) {
    node.getOperand()->accept(*this);
    DataType operandType = currentExpressionType_;
    
    if (operandType != DataType::POINTER) {
        addError("Cannot dereference non-pointer type: " + 
                SymbolTable::dataTypeToString(operandType));
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Dereferencing a pointer returns the pointee type
    // For now, we need to look up the symbol to get the pointee type
    // This is a simplified implementation - would need proper symbol lookup
    // For basic testing, assume integer pointers dereference to integers
    currentExpressionType_ = DataType::INTEGER; // Simplified for testing
}

void SemanticAnalyzer::visit(CallExpression& node) {
    checkFunctionCall(node);
}

void SemanticAnalyzer::visit(FieldAccessExpression& node) {
    // Visit the object expression to ensure it's valid
    node.getObject()->accept(*this);
    
    // Save the object's type before we potentially overwrite currentExpressionType_
    DataType objectType = currentExpressionType_;
    
    if (objectType == DataType::CUSTOM) {
        std::string recordTypeName;
        
        // Case 1: Object is an identifier (e.g., p.x)
        if (auto identifierExpr = dynamic_cast<IdentifierExpression*>(node.getObject())) {
            auto symbol = symbolTable_->lookup(identifierExpr->getName());
            if (symbol && symbol->getSymbolType() == SymbolType::VARIABLE) {
                recordTypeName = symbol->getTypeName();
            }
        }
        // Case 2: Object is another field access (e.g., c.center.x)
        else if (auto fieldAccessExpr = dynamic_cast<FieldAccessExpression*>(node.getObject())) {
            // For nested field access, we need to find the type of the field result
            // The object field access should have already been processed and currentExpressionType_ set
            // We need to find what record type this field belongs to
            // This is more complex - for now, let's look up the field type from the outer expression
            
            // We need to determine the record type name from the nested field access
            // This requires recursive type resolution, but for now, let's handle the common case
            // where we have obj.field1.field2 and field1 is a record type
            
            // First get the object's object (for c.center.x, this would be 'c')
            if (auto outerIdentifier = dynamic_cast<IdentifierExpression*>(fieldAccessExpr->getObject())) {
                auto outerSymbol = symbolTable_->lookup(outerIdentifier->getName());
                if (outerSymbol && outerSymbol->getSymbolType() == SymbolType::VARIABLE) {
                    std::string outerRecordTypeName = outerSymbol->getTypeName();
                    auto outerRecordTypeSymbol = symbolTable_->lookup(outerRecordTypeName);
                    
                    if (outerRecordTypeSymbol && outerRecordTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                        std::string outerRecordDef = outerRecordTypeSymbol->getTypeDefinition();
                        
                        // Get the type of the intermediate field (e.g., 'center' in c.center.x)
                        std::string intermediateFieldType = getFieldTypeFromRecord(fieldAccessExpr->getFieldName(), outerRecordDef);
                        if (!intermediateFieldType.empty()) {
                            recordTypeName = intermediateFieldType;
                        }
                    }
                }
            }
        }
        
        if (!recordTypeName.empty()) {
            auto recordTypeSymbol = symbolTable_->lookup(recordTypeName);
            
            if (recordTypeSymbol && recordTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                std::string recordDef = recordTypeSymbol->getTypeDefinition();
                
                // Check if the field exists in the record
                if (isFieldInRecordDefinition(node.getFieldName(), recordDef)) {
                    // Get the field's type
                    std::string fieldTypeName = getFieldTypeFromRecord(node.getFieldName(), recordDef);
                    DataType fieldType = symbolTable_->resolveDataType(fieldTypeName);
                    currentExpressionType_ = fieldType;
                    return;
                } else {
                    addError("Field '" + node.getFieldName() + "' not found in record type '" + recordTypeName + "'");
                    currentExpressionType_ = DataType::UNKNOWN;
                    return;
                }
            }
        }
    }
    
    // If we get here, field access failed
    addError("Invalid field access: object is not a record type");
    currentExpressionType_ = DataType::UNKNOWN;
}

void SemanticAnalyzer::visit(ArrayIndexExpression& node) {
    // Visit both expressions to ensure they're valid
    node.getArray()->accept(*this);
    DataType arrayType = currentExpressionType_;
    
    node.getIndex()->accept(*this);
    DataType indexType = currentExpressionType_;
    
    // Validate index type (should be integer)
    if (indexType != DataType::INTEGER) {
        addError("Array index must be an integer");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Determine result type based on array type
    if (arrayType == DataType::STRING) {
        // String indexing returns a character
        currentExpressionType_ = DataType::CHAR;
    } else if (arrayType == DataType::CUSTOM) {
        // For arrays or other custom types, we'd need more type information
        // For now, assume integer (this is a simplification)
        currentExpressionType_ = DataType::INTEGER;
    } else {
        addError("Cannot index into non-array type: " + SymbolTable::dataTypeToString(arrayType));
        currentExpressionType_ = DataType::UNKNOWN;
    }
}

void SemanticAnalyzer::visit(SetLiteralExpression& node) {
    // Visit all elements to ensure they're valid
    DataType elementType = DataType::UNKNOWN;
    
    for (const auto& element : node.getElements()) {
        element->accept(*this);
        
        // Determine element type from first valid element
        if (elementType == DataType::UNKNOWN && currentExpressionType_ != DataType::UNKNOWN) {
            elementType = currentExpressionType_;
        }
        // TODO: Validate all elements have same type
    }
    
    // Set the result type as a set type (for now, use CUSTOM)
    // In a full implementation, we'd have specific set types
    currentExpressionType_ = DataType::CUSTOM;
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
    // First visit the target to trigger with field resolution if needed
    node.getTarget()->accept(*this);
    
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

void SemanticAnalyzer::visit(ForStatement& node) {
    // Check if loop variable exists
    auto variable = symbolTable_->lookup(node.getVariable());
    if (!variable) {
        addError("Undefined variable: " + node.getVariable());
        return;
    }
    
    // Check that loop variable is an ordinal type (integer, char, enum, etc.)
    DataType varType = variable->getDataType();
    if (varType != DataType::INTEGER && varType != DataType::CHAR && varType != DataType::UNKNOWN) {
        addError("For loop variable must be an ordinal type, got " + SymbolTable::dataTypeToString(varType));
    }
    
    // Check start expression
    node.getStart()->accept(*this);
    DataType startType = currentExpressionType_;
    
    // Check end expression  
    node.getEnd()->accept(*this);
    DataType endType = currentExpressionType_;
    
    // Check that start and end expressions are compatible with loop variable
    if (varType != DataType::UNKNOWN && startType != DataType::UNKNOWN && startType != varType) {
        addError("For loop start expression type doesn't match variable type");
    }
    if (varType != DataType::UNKNOWN && endType != DataType::UNKNOWN && endType != varType) {
        addError("For loop end expression type doesn't match variable type");
    }
    
    // Check body
    node.getBody()->accept(*this);
}

void SemanticAnalyzer::visit(RepeatStatement& node) {
    // Check body
    node.getBody()->accept(*this);
    
    // Check condition - must be boolean
    node.getCondition()->accept(*this);
    if (currentExpressionType_ != DataType::BOOLEAN && currentExpressionType_ != DataType::UNKNOWN) {
        addError("Repeat-until condition must be boolean, got " + SymbolTable::dataTypeToString(currentExpressionType_));
    }
}

void SemanticAnalyzer::visit(CaseStatement& node) {
    // Check the case expression
    node.getExpression()->accept(*this);
    DataType caseExpressionType = currentExpressionType_;
    
    // Check all case branches
    for (const auto& branch : node.getBranches()) {
        // Check each value in the branch
        for (const auto& value : branch->getValues()) {
            value->accept(*this);
            DataType valueType = currentExpressionType_;
            
            // Check that the value is compatible with the case expression
            if (caseExpressionType != DataType::UNKNOWN && valueType != DataType::UNKNOWN && 
                caseExpressionType != valueType) {
                addError("Case value type doesn't match case expression type");
            }
        }
        
        // Check the branch statement
        branch->getStatement()->accept(*this);
    }
    
    // Check the else clause if present
    if (node.getElseClause()) {
        node.getElseClause()->accept(*this);
    }
}

void SemanticAnalyzer::visit(WithStatement& node) {
    // Check all with expressions and set up with contexts
    for (const auto& withExpr : node.getWithExpressions()) {
        withExpr->accept(*this);
        
        // Try to extract the with variable and its type
        if (auto identifierExpr = dynamic_cast<IdentifierExpression*>(withExpr.get())) {
            std::string varName = identifierExpr->getName();
            auto symbol = symbolTable_->lookup(varName);
            
            if (symbol) {
                WithContext context;
                context.withVariable = varName;
                context.recordType = symbol->getDataType();
                
                // For CUSTOM types, we need to find the record type name
                if (context.recordType == DataType::CUSTOM) {
                    context.recordTypeName = symbol->getTypeName();
                }
                
                withContextStack_.push_back(context);
            } else {
                addError("Undefined with variable: " + varName);
            }
        }
        // TODO: Handle more complex with expressions like field access
    }
    
    // Check the body statement with the with context active
    node.getBody()->accept(*this);
    
    // Remove with contexts when exiting
    for (size_t i = 0; i < node.getWithExpressions().size(); ++i) {
        if (!withContextStack_.empty()) {
            withContextStack_.pop_back();
        }
    }
}

void SemanticAnalyzer::visit(ConstantDeclaration& node) {
    // Analyze the constant value expression
    node.getValue()->accept(*this);
    
    // Register the constant in the symbol table
    symbolTable_->define(node.getName(), SymbolType::CONSTANT, currentExpressionType_);
}

void SemanticAnalyzer::visit(TypeDefinition& node) {
    // Register the type definition in the symbol table as a custom type
    auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::TYPE_DEF, DataType::CUSTOM);
    symbol->setTypeDefinition(node.getDefinition());
    symbolTable_->define(node.getName(), symbol);
    
    // Check if this is an enumeration type and register enum values
    std::string definition = node.getDefinition();
    if (definition.length() > 2 && definition[0] == '(' && definition.back() == ')') {
        // This is an enumeration type like "(Red, Green, Blue)"
        std::string enumValues = definition.substr(1, definition.length() - 2); // Remove parentheses
        
        // Parse individual enum values
        size_t pos = 0;
        int enumOrdinal = 0;
        while (pos < enumValues.length()) {
            // Find the next comma or end of string
            size_t commaPos = enumValues.find(',', pos);
            if (commaPos == std::string::npos) {
                commaPos = enumValues.length();
            }
            
            // Extract enum value name (trim whitespace)
            std::string enumValue = enumValues.substr(pos, commaPos - pos);
            size_t start = enumValue.find_first_not_of(" \t");
            size_t end = enumValue.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                enumValue = enumValue.substr(start, end - start + 1);
                
                // Register enum value as a constant
                auto enumSymbol = std::make_shared<Symbol>(enumValue, SymbolType::CONSTANT, DataType::INTEGER);
                enumSymbol->setTypeName(node.getName()); // Link back to enum type
                symbolTable_->define(enumValue, enumSymbol);
                enumOrdinal++;
            }
            
            pos = commaPos + 1;
        }
    }
}

void SemanticAnalyzer::visit(RecordTypeDefinition& node) {
    // Register the record type in the symbol table
    auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::TYPE_DEF, DataType::CUSTOM);
    
    // Build the record definition string from the fields
    std::string recordDef = "record ";
    for (const auto& field : node.getFields()) {
        recordDef += field.getName() + ":" + field.getType() + "; ";
    }
    recordDef += "end";
    
    symbol->setTypeDefinition(recordDef);
    symbolTable_->define(node.getName(), symbol);
    
    // Validate that all field types exist
    for (const auto& field : node.getFields()) {
        DataType fieldType = symbolTable_->resolveDataType(field.getType());
        if (fieldType == DataType::UNKNOWN) {
            addError("Unknown field type '" + field.getType() + "' in record '" + node.getName() + "'");
        }
    }
}

void SemanticAnalyzer::visit(VariableDeclaration& node) {
    DataType dataType = symbolTable_->resolveDataType(node.getType());
    if (dataType == DataType::UNKNOWN) {
        addError("Unknown data type: " + node.getType());
        return;
    }
    
    auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::VARIABLE, dataType);
    symbol->setTypeName(node.getType()); // Store the original type name
    
    // Handle pointer types - extract and store pointee type information
    if (dataType == DataType::POINTER) {
        std::string typeStr = node.getType();
        if (!typeStr.empty() && typeStr[0] == '^') {
            std::string pointeeTypeName = typeStr.substr(1); // Remove the '^' prefix
            DataType pointeeType = symbolTable_->resolveDataType(pointeeTypeName);
            symbol->setPointeeType(pointeeType);
            symbol->setPointeeTypeName(pointeeTypeName);
        }
    }
    
    symbolTable_->define(node.getName(), symbol);
    
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
    // Check if this is an implementation of a forward declaration
    auto existingSymbol = symbolTable_->lookup(node.getName());
    
    if (node.isForward()) {
        // This is a forward declaration
        if (existingSymbol) {
            addError("Symbol '" + node.getName() + "' already defined in current scope");
            return;
        }
        
        // Create procedure symbol for forward declaration
        auto procedureSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::PROCEDURE, DataType::VOID, 
                                                       symbolTable_->getCurrentScopeLevel());
        
        // Add parameters to procedure symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            procedureSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define procedure in current scope
        symbolTable_->define(node.getName(), procedureSymbol);
        return;
    }
    
    // This is an implementation - check if it matches a forward declaration
    if (existingSymbol && existingSymbol->getSymbolType() == SymbolType::PROCEDURE) {
        // Verify parameter compatibility (simplified check)
        if (existingSymbol->getParameters().size() != node.getParameters().size()) {
            addError("Procedure '" + node.getName() + "' implementation doesn't match forward declaration parameter count");
            return;
        }
        // Note: Could add more detailed parameter type checking here
    } else if (!existingSymbol) {
        // No forward declaration - define the procedure
        auto procedureSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::PROCEDURE, DataType::VOID, 
                                                       symbolTable_->getCurrentScopeLevel());
        
        // Add parameters to procedure symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            procedureSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define procedure in current scope
        symbolTable_->define(node.getName(), procedureSymbol);
    }
    
    // Enter new scope for procedure body
    symbolTable_->enterScope();
    
    // Add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
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
    DataType returnType = symbolTable_->resolveDataType(node.getReturnType());
    if (returnType == DataType::UNKNOWN) {
        addError("Unknown return type: " + node.getReturnType());
        returnType = DataType::VOID;
    }
    
    // Check if this is an implementation of a forward declaration
    auto existingSymbol = symbolTable_->lookup(node.getName());
    
    if (node.isForward()) {
        // This is a forward declaration
        if (existingSymbol) {
            addError("Symbol '" + node.getName() + "' already defined in current scope");
            return;
        }
        
        // Create function symbol for forward declaration
        auto functionSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::FUNCTION, returnType,
                                                      symbolTable_->getCurrentScopeLevel());
        functionSymbol->setReturnType(returnType);
        
        // Add parameters to function symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            functionSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define function in current scope
        symbolTable_->define(node.getName(), functionSymbol);
        return;
    }
    
    // This is an implementation - check if it matches a forward declaration
    if (existingSymbol && existingSymbol->getSymbolType() == SymbolType::FUNCTION) {
        // Verify parameter compatibility (simplified check)
        if (existingSymbol->getParameters().size() != node.getParameters().size()) {
            addError("Function '" + node.getName() + "' implementation doesn't match forward declaration parameter count");
            return;
        }
        // Verify return type compatibility
        if (existingSymbol->getReturnType() != returnType) {
            addError("Function '" + node.getName() + "' implementation return type doesn't match forward declaration");
            return;
        }
        // Note: Could add more detailed parameter type checking here
    } else if (!existingSymbol) {
        // No forward declaration - define the function
        auto functionSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::FUNCTION, returnType,
                                                      symbolTable_->getCurrentScopeLevel());
        functionSymbol->setReturnType(returnType);
        
        // Add parameters to function symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            functionSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define function in current scope
        symbolTable_->define(node.getName(), functionSymbol);
    }
    
    // Enter new scope for function body
    symbolTable_->enterScope();
    
    // Add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
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
    // Process uses clause first
    if (node.getUsesClause()) {
        node.getUsesClause()->accept(*this);
    }
    
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
        operator_ == TokenType::GREATER_THAN || operator_ == TokenType::GREATER_EQUAL ||
        operator_ == TokenType::IN) {
        return DataType::BOOLEAN;
    }
    
    // Logical operators
    if (operator_ == TokenType::AND || operator_ == TokenType::OR || operator_ == TokenType::XOR) {
        return DataType::BOOLEAN;
    }
    
    // Addition can be arithmetic, string concatenation, or set union
    if (operator_ == TokenType::PLUS) {
        // Set union
        if (left == DataType::CUSTOM && right == DataType::CUSTOM) {
            return DataType::CUSTOM;
        }
        // String concatenation
        if (left == DataType::STRING && right == DataType::STRING) {
            return DataType::STRING;
        }
        // Arithmetic addition
        if (left == DataType::REAL || right == DataType::REAL) {
            return DataType::REAL;
        }
        return DataType::INTEGER;
    }
    
    // Other arithmetic operators or set operations
    if (operator_ == TokenType::MINUS || operator_ == TokenType::MULTIPLY || 
        operator_ == TokenType::DIVIDE) {
        // Set operations
        if (left == DataType::CUSTOM && right == DataType::CUSTOM) {
            return DataType::CUSTOM;
        }
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
            // Allow numeric addition, string concatenation, and set union
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL)) ||
                   (leftType == DataType::STRING && rightType == DataType::STRING) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM);
        case TokenType::MINUS:
            // Allow numeric subtraction and set difference
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL)) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM);
        case TokenType::MULTIPLY:
            // Allow numeric multiplication and set intersection
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL)) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM);
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
                   
        case TokenType::IN:
            // Set membership: element in set
            return (leftType == DataType::INTEGER || leftType == DataType::CHAR) &&
                   (rightType == DataType::CUSTOM);
                   
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
    
    if (functionName == "concat") {
        // Accept any number and type of arguments for now
        for (const auto& arg : node.getArguments()) {
            arg->accept(*this);
        }
        currentExpressionType_ = DataType::STRING;
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
        
        if (!areArgumentTypesCompatible(expectedType, actualType, actualArgs[i].get())) {
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
    auto targetDeref = dynamic_cast<DereferenceExpression*>(target);
    
    std::shared_ptr<Symbol> targetSymbol = nullptr;
    
    if (targetId) {
        // Simple variable assignment
        targetSymbol = symbolTable_->lookup(targetId->getName());
        if (!targetSymbol) {
            // Check if this is a with field access
            if (targetId->isWithFieldAccess()) {
                // Allow assignment to with fields - assume they're valid
                // The with field resolution was already done in identifier resolution
                // Skip further validation for now
                value->accept(*this);
                return;
            }
            
            addError("Undefined variable: " + targetId->getName());
            return;
        }
        
        if (targetSymbol->getSymbolType() != SymbolType::VARIABLE &&
            targetSymbol->getSymbolType() != SymbolType::PARAMETER &&
            targetSymbol->getSymbolType() != SymbolType::FUNCTION) {
            addError("Cannot assign to " + targetId->getName());
            return;
        }
    } else if (targetField || targetArray || targetDeref) {
        // Field access, array indexing, or pointer dereference - these are valid assignment targets
        // For now, skip detailed type checking and allow the assignment
        // TODO: Add more detailed validation for field/array/pointer types
        value->accept(*this);
        return; // Skip type compatibility check for now
    } else {
        addError("Invalid assignment target");
        return;
    }
    
    // Get types (only for simple variable assignments)
    DataType targetType = targetSymbol->getDataType();
    DataType valueType = getExpressionType(value);
    
    // Special handling for custom types (ranges)
    if (targetType == DataType::CUSTOM) {
        // Get the type name and look up its definition
        std::string typeName = targetSymbol->getTypeName();
        auto typeDefSymbol = symbolTable_->lookup(typeName);
        
        if (typeDefSymbol && typeDefSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
            std::string definition = typeDefSymbol->getTypeDefinition();
            
            // Check if it's a range type
            if (definition.find("..") != std::string::npos) {
                // Determine if it's a character or numeric range
                if (definition.find("'") != std::string::npos) {
                    // Character range - compatible with char
                    if (valueType == DataType::CHAR) {
                        return; // Compatible
                    }
                } else {
                    // Numeric range - compatible with integer
                    if (valueType == DataType::INTEGER) {
                        return; // Compatible
                    }
                }
            }
            // Check if it's a bounded string type
            else if (definition.find("string[") != std::string::npos) {
                // Bounded string - compatible with string
                if (valueType == DataType::STRING) {
                    return; // Compatible
                }
            }
            // Check if it's an enumeration type
            else if (definition.length() > 2 && definition[0] == '(' && definition.back() == ')') {
                // Enumeration type - check if value is an enum constant
                if (valueType == DataType::INTEGER) {
                    // The value might be an enum constant
                    // For now, check if the value expression is an identifier that belongs to this enum
                    auto valueId = dynamic_cast<IdentifierExpression*>(value);
                    if (valueId) {
                        auto valueSymbol = symbolTable_->lookup(valueId->getName());
                        if (valueSymbol && valueSymbol->getSymbolType() == SymbolType::CONSTANT &&
                            valueSymbol->getTypeName() == typeName) {
                            return; // Compatible - enum constant assignment
                        }
                    }
                }
            }
        }
    }
    
    if (!areTypesCompatible(targetType, valueType)) {
        addError("Type mismatch in assignment: cannot assign " +
                SymbolTable::dataTypeToString(valueType) + " to " +
                SymbolTable::dataTypeToString(targetType));
    }
}

bool SemanticAnalyzer::isBoundedStringType(DataType type) {
    // This is a simplified check - we'll assume CUSTOM types in string contexts are bounded strings
    // A more thorough implementation would track the actual type definitions
    return type == DataType::CUSTOM;
}

bool SemanticAnalyzer::areArgumentTypesCompatible(DataType expectedType, DataType actualType, Expression* actualExpr) {
    // First try normal type compatibility
    if (areTypesCompatible(expectedType, actualType)) {
        return true;
    }
    
    // Special case: string parameter can accept bounded string (CUSTOM type)
    if (expectedType == DataType::STRING && actualType == DataType::CUSTOM) {
        // Check if the actual expression is a bounded string type
        if (auto identExpr = dynamic_cast<IdentifierExpression*>(actualExpr)) {
            auto symbol = symbolTable_->lookup(identExpr->getName());
            if (symbol) {
                std::string typeName = symbol->getTypeName();
                auto typeSymbol = symbolTable_->lookup(typeName);
                if (typeSymbol && typeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string definition = typeSymbol->getTypeDefinition();
                    // Check if it's a bounded string type
                    if (definition.find("string[") != std::string::npos) {
                        return true; // Bounded string is compatible with string parameter
                    }
                }
            }
        }
    }
    
    return false;
}

bool SemanticAnalyzer::isFieldInRecordDefinition(const std::string& fieldName, const std::string& recordDef) {
    // Parse record definition like "record name: string; age: integer; address: string; end"
    std::string def = recordDef;
    
    // Remove "record" and "end" keywords
    size_t recordPos = def.find("record");
    if (recordPos != std::string::npos) {
        def = def.substr(recordPos + 6); // Skip "record"
    }
    
    size_t endPos = def.find("end");
    if (endPos != std::string::npos) {
        def = def.substr(0, endPos);
    }
    
    // Split by semicolons to get field definitions
    size_t pos = 0;
    while (pos < def.length()) {
        size_t semicolonPos = def.find(';', pos);
        if (semicolonPos == std::string::npos) {
            semicolonPos = def.length();
        }
        
        std::string fieldDef = def.substr(pos, semicolonPos - pos);
        
        // Trim whitespace
        size_t start = fieldDef.find_first_not_of(" \t\n\r");
        size_t end = fieldDef.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            fieldDef = fieldDef.substr(start, end - start + 1);
            
            // Parse field definition like "name: string" or "x, y: integer"
            size_t colonPos = fieldDef.find(':');
            if (colonPos != std::string::npos) {
                std::string fieldNames = fieldDef.substr(0, colonPos);
                
                // Check if our field name is in the list
                size_t namePos = 0;
                while (namePos < fieldNames.length()) {
                    size_t commaPos = fieldNames.find(',', namePos);
                    if (commaPos == std::string::npos) {
                        commaPos = fieldNames.length();
                    }
                    
                    std::string name = fieldNames.substr(namePos, commaPos - namePos);
                    
                    // Trim whitespace
                    size_t nameStart = name.find_first_not_of(" \t\n\r");
                    size_t nameEnd = name.find_last_not_of(" \t\n\r");
                    if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                        name = name.substr(nameStart, nameEnd - nameStart + 1);
                        
                        if (name == fieldName) {
                            return true;
                        }
                    }
                    
                    namePos = commaPos + 1;
                }
            }
        }
        
        pos = semicolonPos + 1;
    }
    
    return false;
}

std::string SemanticAnalyzer::getFieldTypeFromRecord(const std::string& fieldName, const std::string& recordDef) {
    // Parse record definition like "record name: string; age: integer; address: string; end"
    std::string def = recordDef;
    
    // Remove "record" and "end" keywords
    size_t recordPos = def.find("record");
    if (recordPos != std::string::npos) {
        def = def.substr(recordPos + 6); // Skip "record"
    }
    
    size_t endPos = def.find("end");
    if (endPos != std::string::npos) {
        def = def.substr(0, endPos);
    }
    
    // Split by semicolons to get field definitions
    size_t pos = 0;
    while (pos < def.length()) {
        size_t semicolonPos = def.find(';', pos);
        if (semicolonPos == std::string::npos) {
            semicolonPos = def.length();
        }
        
        std::string fieldDef = def.substr(pos, semicolonPos - pos);
        
        // Trim whitespace
        size_t start = fieldDef.find_first_not_of(" \t\n\r");
        size_t end = fieldDef.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            fieldDef = fieldDef.substr(start, end - start + 1);
            
            // Parse field definition like "name: string" or "x, y: integer"
            size_t colonPos = fieldDef.find(':');
            if (colonPos != std::string::npos) {
                std::string fieldNames = fieldDef.substr(0, colonPos);
                std::string fieldType = fieldDef.substr(colonPos + 1);
                
                // Trim the type
                size_t typeStart = fieldType.find_first_not_of(" \t\n\r");
                size_t typeEnd = fieldType.find_last_not_of(" \t\n\r");
                if (typeStart != std::string::npos && typeEnd != std::string::npos) {
                    fieldType = fieldType.substr(typeStart, typeEnd - typeStart + 1);
                }
                
                // Check if our field name is in the list
                size_t namePos = 0;
                while (namePos < fieldNames.length()) {
                    size_t commaPos = fieldNames.find(',', namePos);
                    if (commaPos == std::string::npos) {
                        commaPos = fieldNames.length();
                    }
                    
                    std::string name = fieldNames.substr(namePos, commaPos - namePos);
                    
                    // Trim whitespace
                    size_t nameStart = name.find_first_not_of(" \t\n\r");
                    size_t nameEnd = name.find_last_not_of(" \t\n\r");
                    if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                        name = name.substr(nameStart, nameEnd - nameStart + 1);
                        
                        if (name == fieldName) {
                            return fieldType;
                        }
                    }
                    
                    namePos = commaPos + 1;
                }
            }
        }
        
        pos = semicolonPos + 1;
    }
    
    return ""; // Field not found
}

void SemanticAnalyzer::visit(UsesClause& node) {
    // Load and process units
    for (const std::string& unitName : node.getUnits()) {
        std::cout << "DEBUG: Processing unit: " << unitName << std::endl;
        
        // Check if unit is a standard unit (System, Dos, Crt)
        if (unitName == "System" || unitName == "Dos" || unitName == "Crt") {
            std::cout << "DEBUG: " << unitName << " is a built-in unit" << std::endl;
            // Built-in units are handled automatically
            continue;
        }
        
        std::cout << "DEBUG: Attempting to load custom unit: " << unitName << std::endl;
        
        // Try to load custom unit
        if (!unitLoader_->isUnitLoaded(unitName)) {
            std::cout << "DEBUG: Unit not loaded, loading now..." << std::endl;
            unitLoader_->loadUnit(unitName);
            
            // Check if loading succeeded
            if (!unitLoader_->isUnitLoaded(unitName)) {
                std::cout << "DEBUG: Failed to load unit " << unitName << std::endl;
                addError("Failed to load unit: " + unitName);
                continue;
            }
            std::cout << "DEBUG: Successfully loaded unit " << unitName << std::endl;
        }
        
        // Get the loaded unit and process its interface declarations
        Unit* loadedUnit = unitLoader_->getLoadedUnit(unitName);
        if (loadedUnit) {
            std::cout << "DEBUG: Processing interface declarations for " << unitName << std::endl;
            // Import symbols from the unit's interface section
            for (const auto& decl : loadedUnit->getInterfaceDeclarations()) {
                // Process the declaration to add symbols to our symbol table
                decl->accept(*this);
            }
        } else {
            std::cout << "DEBUG: Could not get loaded unit " << unitName << std::endl;
        }
    }
}

void SemanticAnalyzer::visit(Unit& node) {
    // Process interface declarations first
    for (const auto& decl : node.getInterfaceDeclarations()) {
        decl->accept(*this);
    }
    
    // Process uses clause if present - need to cast away const
    if (node.getUsesClause()) {
        const_cast<UsesClause*>(node.getUsesClause())->accept(*this);
    }
    
    // Process implementation declarations
    for (const auto& decl : node.getImplementationDeclarations()) {
        decl->accept(*this);
    }
    
    // Process initialization block if present - need to cast away const
    if (node.getInitializationBlock()) {
        const_cast<CompoundStatement*>(node.getInitializationBlock())->accept(*this);
    }
}

} // namespace rpascal