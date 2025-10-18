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
    currentExpressionTypeName_ = ""; // Literals don't have custom type names
}

void SemanticAnalyzer::visit(IdentifierExpression& node) {
    auto symbol = symbolTable_->lookup(node.getName());
    if (!symbol) {
        
        // Check if this is a built-in constant (CRT colors, etc.)
        if (isBuiltinConstant(node.getName())) {
            currentExpressionType_ = DataType::INTEGER;
            currentExpressionTypeName_ = "";
            return;
        }
        
        // Check if this is a built-in function/procedure without parentheses
        if (isBuiltinFunction(node.getName())) {
            std::string lowerName = node.getName();
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            // Most CRT procedures return void
            if (lowerName == "clrscr" || lowerName == "clreol" || lowerName == "textcolor" ||
                lowerName == "textbackground" || lowerName == "lowvideo" || lowerName == "highvideo" ||
                lowerName == "normvideo" || lowerName == "cursoron" || lowerName == "cursoroff" ||
                lowerName == "sound" || lowerName == "nosound" || lowerName == "delay") {
                currentExpressionType_ = DataType::VOID;
            } else if (lowerName == "wherex" || lowerName == "wherey") {
                currentExpressionType_ = DataType::INTEGER;
            } else if (lowerName == "keypressed") {
                currentExpressionType_ = DataType::BOOLEAN;
            } else if (lowerName == "readkey") {
                currentExpressionType_ = DataType::CHAR;
            } else {
                currentExpressionType_ = DataType::VOID; // Default for procedures
            }
            currentExpressionTypeName_ = "";
            return;
        }
        
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
            // Check overloaded symbols (procedures/functions defined with defineOverloaded)
            auto allOverloads = symbolTable_->lookupAllOverloads(node.getName());
            if (!allOverloads.empty()) {
                // For a bare identifier (no parentheses), we treat it as a procedure call with no arguments
                // or as a function reference
                auto overloadSymbol = allOverloads[0]; // Take the first overload for parameterless calls
                
                // Check if any overload has no parameters
                for (const auto& overload : allOverloads) {
                    if (overload->getParameters().empty()) {
                        overloadSymbol = overload;
                        break;
                    }
                }
                
                if (overloadSymbol->getSymbolType() == SymbolType::PROCEDURE) {
                    if (overloadSymbol->getParameters().empty()) {
                        // This is a parameterless procedure call
                        currentExpressionType_ = DataType::VOID;
                        currentExpressionTypeName_ = "";
                        return;
                    } else {
                        addError("Procedure '" + node.getName() + "' requires parameters");
                        currentExpressionType_ = DataType::UNKNOWN;
                        currentExpressionTypeName_ = "";
                        return;
                    }
                } else if (overloadSymbol->getSymbolType() == SymbolType::FUNCTION) {
                    if (overloadSymbol->getParameters().empty()) {
                        // This is a parameterless function call
                        currentExpressionType_ = overloadSymbol->getReturnType();
                        currentExpressionTypeName_ = "";
                        return;
                    } else {
                        addError("Function '" + node.getName() + "' requires parameters");
                        currentExpressionType_ = DataType::UNKNOWN;
                        currentExpressionTypeName_ = "";
                        return;
                    }
                }
            }
            
            addError("Undefined identifier: " + node.getName());
            currentExpressionType_ = DataType::UNKNOWN;
            currentExpressionTypeName_ = "";
        }
        return;
    }
    
    if (symbol->getSymbolType() == SymbolType::FUNCTION) {
        // Function name used as expression (for return value assignment)
        currentExpressionType_ = symbol->getReturnType();
        currentExpressionTypeName_ = "";
    } else {
        currentExpressionType_ = symbol->getDataType();
        // For custom types and pointer types, also store the type name
        if (symbol->getDataType() == DataType::CUSTOM || symbol->getDataType() == DataType::POINTER) {
            currentExpressionTypeName_ = symbol->getTypeName();
        } else {
            currentExpressionTypeName_ = "";
        }
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node) {
    // Analyze operands
    node.getLeft()->accept(*this);
    DataType leftType = currentExpressionType_;
    std::string leftTypeName = currentExpressionTypeName_;
    
    node.getRight()->accept(*this);
    DataType rightType = currentExpressionType_;
    std::string rightTypeName = currentExpressionTypeName_;
    
    TokenType operator_ = node.getOperator().getType();
    
    // Check type compatibility for comparison operators
    if (operator_ == TokenType::EQUAL || operator_ == TokenType::NOT_EQUAL ||
        operator_ == TokenType::LESS_THAN || operator_ == TokenType::LESS_EQUAL ||
        operator_ == TokenType::GREATER_THAN || operator_ == TokenType::GREATER_EQUAL) {
        if (!areTypesCompatible(leftType, rightType, leftTypeName, rightTypeName)) {
            // Use the operator location since it's more accurate
            SourceLocation loc = node.getOperator().getLocation();
            
            addError("Invalid binary operation: " + 
                    SymbolTable::dataTypeToString(leftType) + " (" + leftTypeName + ") " + 
                    node.getOperator().getValue() + " " + 
                    SymbolTable::dataTypeToString(rightType) + " (" + rightTypeName + ")", loc);
            currentExpressionType_ = DataType::UNKNOWN;
            currentExpressionTypeName_ = "";
            return;
        }
        currentExpressionType_ = DataType::BOOLEAN;
        currentExpressionTypeName_ = "";
        return;
    }
    
    // For other operators, use the existing validation
    if (!isValidBinaryOperation(leftType, rightType, operator_)) {
        // Use the operator location since it's more accurate
        SourceLocation loc = node.getOperator().getLocation();
        addError("Invalid binary operation: " + 
                SymbolTable::dataTypeToString(leftType) + " (" + leftTypeName + ") " + 
                node.getOperator().getValue() + " " + 
                SymbolTable::dataTypeToString(rightType) + " (" + rightTypeName + ")", loc);
        currentExpressionType_ = DataType::UNKNOWN;
        currentExpressionTypeName_ = "";
        return;
    }
    
    currentExpressionType_ = getResultType(leftType, rightType, operator_);
    
    // Handle type name preservation for custom types
    if (currentExpressionType_ == DataType::CUSTOM) {
        if (operator_ == TokenType::PLUS || operator_ == TokenType::MINUS || operator_ == TokenType::MULTIPLY) {
            // For set operations, both operands should have the same set type
            if (!leftTypeName.empty() && !rightTypeName.empty() && leftTypeName == rightTypeName) {
                currentExpressionTypeName_ = leftTypeName;
            } else if (!leftTypeName.empty()) {
                currentExpressionTypeName_ = leftTypeName;
            } else if (!rightTypeName.empty()) {
                currentExpressionTypeName_ = rightTypeName;
            } else {
                currentExpressionTypeName_ = "";
            }
        }
        
        // Special case for bounded string + char operations
        if (operator_ == TokenType::PLUS) {
            if (leftType == DataType::CUSTOM && rightType == DataType::CHAR && !leftTypeName.empty()) {
                // Check if left is a bounded string
                auto leftTypeSymbol = symbolTable_->lookup(leftTypeName);
                if (leftTypeSymbol && leftTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string leftDef = leftTypeSymbol->getTypeDefinition();
                    if (leftDef.find("string[") == 0) {
                        currentExpressionType_ = DataType::CUSTOM;
                        currentExpressionTypeName_ = leftTypeName;
                    }
                }
            } else if ((leftType == DataType::CHAR || leftType == DataType::STRING) && rightType == DataType::CUSTOM && !rightTypeName.empty()) {
                // Check if right is a bounded string
                auto rightTypeSymbol = symbolTable_->lookup(rightTypeName);
                if (rightTypeSymbol && rightTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string rightDef = rightTypeSymbol->getTypeDefinition();
                    if (rightDef.find("string[") == 0) {
                        currentExpressionType_ = DataType::CUSTOM;
                        currentExpressionTypeName_ = rightTypeName;
                    }
                }
            } else if (leftType == DataType::CUSTOM && (rightType == DataType::STRING || rightType == DataType::CHAR) && !leftTypeName.empty()) {
                // Check if left is a bounded string
                auto leftTypeSymbol = symbolTable_->lookup(leftTypeName);
                if (leftTypeSymbol && leftTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string leftDef = leftTypeSymbol->getTypeDefinition();
                    if (leftDef.find("string[") == 0) {
                        currentExpressionType_ = DataType::CUSTOM;
                        currentExpressionTypeName_ = leftTypeName;
                    }
                }
            }
        }
    } else {
        currentExpressionTypeName_ = ""; // Result types are usually built-in types
    }
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
                SymbolTable::dataTypeToString(operandType), node.getLocation());
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Try to determine the pointee type from the operand
    // If the operand is an identifier, look up its pointee type
    if (auto identExpr = dynamic_cast<IdentifierExpression*>(node.getOperand())) {
        auto symbol = symbolTable_->lookup(identExpr->getName());
        if (symbol && symbol->getDataType() == DataType::POINTER) {
            currentExpressionType_ = symbol->getPointeeType();
            currentExpressionTypeName_ = symbol->getPointeeTypeName();
            return;
        }
    }
    
    // If the operand is a field access or other expression that results in a pointer,
    // we need to determine the pointee type from the type name
    if (currentExpressionType_ == DataType::POINTER && !currentExpressionTypeName_.empty()) {
        // First try to resolve as a named pointer type
        auto pointerTypeSymbol = symbolTable_->lookup(currentExpressionTypeName_);
        if (pointerTypeSymbol && pointerTypeSymbol->getSymbolType() == SymbolType::TYPE_DEF && 
            pointerTypeSymbol->getDataType() == DataType::POINTER) {
            currentExpressionType_ = pointerTypeSymbol->getPointeeType();
            currentExpressionTypeName_ = pointerTypeSymbol->getPointeeTypeName();
            return;
        }
        
        // Handle direct pointer syntax like ^TNode
        if (!currentExpressionTypeName_.empty() && currentExpressionTypeName_[0] == '^') {
            std::string pointeeTypeName = currentExpressionTypeName_.substr(1); // Remove the '^'
            DataType pointeeType = symbolTable_->resolveDataType(pointeeTypeName);
            currentExpressionType_ = pointeeType;
            currentExpressionTypeName_ = (pointeeType == DataType::CUSTOM) ? pointeeTypeName : "";
            return;
        }
    }
    
    // For basic testing, assume integer pointers if we can't determine the type
    currentExpressionType_ = DataType::INTEGER;
    currentExpressionTypeName_ = "";
}

void SemanticAnalyzer::visit(CallExpression& node) {
    checkFunctionCall(node);
}

void SemanticAnalyzer::visit(FieldAccessExpression& node) {
    // Visit the object expression to ensure it's valid
    node.getObject()->accept(*this);
    
    // Save the object's type before we potentially overwrite currentExpressionType_
    DataType objectType = currentExpressionType_;
    
    if (objectType == DataType::CUSTOM || (objectType == DataType::UNKNOWN && !currentExpressionTypeName_.empty())) {
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
        // Case 3: Object is a dereference expression (e.g., ptr^.data)
        else if (auto dereferenceExpr = dynamic_cast<DereferenceExpression*>(node.getObject())) {
            // The dereference expression should have already been processed and 
            // currentExpressionType_ should be set to the pointee type
            // For record pointers, currentExpressionTypeName_ should contain the record type name
            recordTypeName = currentExpressionTypeName_;
        }
        // Case 4: Object is an array index expression (e.g., nodes[1].data)
        else if (auto arrayIndexExpr = dynamic_cast<ArrayIndexExpression*>(node.getObject())) {
            // For array indexing, we need to find the element type of the array
            if (auto arrayIdent = dynamic_cast<IdentifierExpression*>(arrayIndexExpr->getArray())) {
                auto arraySymbol = symbolTable_->lookup(arrayIdent->getName());
                if (arraySymbol && arraySymbol->getSymbolType() == SymbolType::VARIABLE) {
                    // For arrays of records, the element type is the record type
                    // The currentExpressionType_ should already be set to the element type
                    // and currentExpressionTypeName_ should contain the type name
                    recordTypeName = currentExpressionTypeName_;
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
                    
                    // For pointer field types, store the type name properly
                    if (fieldType == DataType::POINTER && !fieldTypeName.empty() && fieldTypeName[0] == '^') {
                        // Store the complete pointer type for later resolution
                        currentExpressionTypeName_ = fieldTypeName;
                    } else if (fieldType == DataType::CUSTOM) {
                        // For custom types, store the type name
                        currentExpressionTypeName_ = fieldTypeName;
                    } else {
                        currentExpressionTypeName_ = "";
                    }
                    return;
                } else {
                    addError("Field '" + node.getFieldName() + "' not found in record type '" + recordTypeName + "'", node.getLocation());
                    currentExpressionType_ = DataType::UNKNOWN;
                    return;
                }
            }
        }
    }
    
    // If we get here, field access failed
    addError("Invalid field access: object is not a record type", node.getLocation());
    currentExpressionType_ = DataType::UNKNOWN;
}

void SemanticAnalyzer::visit(ArrayIndexExpression& node) {
    // Visit both expressions to ensure they're valid
    node.getArray()->accept(*this);
    DataType arrayType = currentExpressionType_;
    std::string arrayTypeName = currentExpressionTypeName_;
    
    node.getIndex()->accept(*this);
    DataType indexType = currentExpressionType_;
    
    // Validate index type (should be integer)
    if (indexType != DataType::INTEGER) {
        addError("Array index must be an integer");
        currentExpressionType_ = DataType::UNKNOWN;
        currentExpressionTypeName_ = "";
        return;
    }
    
    // Determine result type based on array type
    if (arrayType == DataType::STRING) {
        // String indexing returns a character
        currentExpressionType_ = DataType::CHAR;
        currentExpressionTypeName_ = "";
    } else if (arrayType == DataType::CUSTOM) {
        // For arrays, extract the element type from the array type definition
        // Array type names can be "array[start..end] of ElementType" or "array of ElementType"
        if (!arrayTypeName.empty() && arrayTypeName.find("array") == 0 && arrayTypeName.find(" of ") != std::string::npos) {
            size_t ofPos = arrayTypeName.find(" of ");
            std::string elementTypeName = arrayTypeName.substr(ofPos + 4); // Skip " of "
            
            // Trim whitespace
            size_t start = elementTypeName.find_first_not_of(" \t");
            if (start != std::string::npos) {
                elementTypeName = elementTypeName.substr(start);
                size_t end = elementTypeName.find_last_not_of(" \t");
                if (end != std::string::npos) {
                    elementTypeName = elementTypeName.substr(0, end + 1);
                }
            }
            
            // Set the element type as the result
            DataType elementDataType = symbolTable_->resolveDataType(elementTypeName);
            currentExpressionType_ = elementDataType;
            currentExpressionTypeName_ = (elementDataType == DataType::CUSTOM) ? elementTypeName : "";
        } else {
            // For open arrays and other custom types without proper type name, 
            // we need to handle this case. For now, assume integer elements for open arrays
            // This is a temporary fix - the real issue is that the type name isn't being stored properly
            currentExpressionType_ = DataType::INTEGER;  // Assume integer for open arrays
            currentExpressionTypeName_ = "";
        }
    } else {
        addError("Cannot index into non-array type: " + SymbolTable::dataTypeToString(arrayType));
        currentExpressionType_ = DataType::UNKNOWN;
        currentExpressionTypeName_ = "";
    }
}

void SemanticAnalyzer::visit(SetLiteralExpression& node) {
    // Visit all elements to ensure they're valid
    DataType elementType = DataType::UNKNOWN;
    std::string elementTypeName = "";
    
    for (const auto& element : node.getElements()) {
        element->accept(*this);
        
        // Determine element type from first valid element
        if (elementType == DataType::UNKNOWN && currentExpressionType_ != DataType::UNKNOWN) {
            elementType = currentExpressionType_;
            elementTypeName = currentExpressionTypeName_;
        }
        // TODO: Validate all elements have same type
    }
    
    // Set the result type as a set type
    currentExpressionType_ = DataType::CUSTOM;
    
    // Create a set type name based on the element type
    if (elementType != DataType::UNKNOWN) {
        if (!elementTypeName.empty()) {
            // Use the specific type name (could be range like "0..9", enum like "TColor", etc.)
            currentExpressionTypeName_ = "set of " + elementTypeName;
        } else {
            // For built-in types without specific type name
            currentExpressionTypeName_ = "set of " + SymbolTable::dataTypeToString(elementType);
        }
    } else {
        currentExpressionTypeName_ = "set";
    }
}

void SemanticAnalyzer::visit(RangeExpression& node) {
    // Visit start and end expressions
    const_cast<Expression*>(node.getStart())->accept(*this);
    DataType startType = currentExpressionType_;
    std::string startTypeName = currentExpressionTypeName_;
    
    const_cast<Expression*>(node.getEnd())->accept(*this);
    DataType endType = currentExpressionType_;
    std::string endTypeName = currentExpressionTypeName_;
    
    // Both start and end should be the same ordinal type (integer, char, enum)
    if (startType != endType) {
        addError("Range start and end must be the same type");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // For custom types, also check type names are compatible
    if (startType == DataType::CUSTOM && startTypeName != endTypeName) {
        addError("Range start and end must be the same type");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Ranges are valid for ordinal types: integer, char, boolean, enums
    if (startType != DataType::INTEGER && startType != DataType::CHAR && 
        startType != DataType::BOOLEAN && startType != DataType::CUSTOM) {
        addError("Range can only be used with ordinal types (integer, char, boolean, enum)");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Range expressions represent a subrange type
    // For sets, we need to preserve the range information for type compatibility
    currentExpressionType_ = startType;
    
    // Create a range type name for type compatibility checking
    if (startType == DataType::INTEGER) {
        // Try to extract literal values for the range
        auto startLit = dynamic_cast<const LiteralExpression*>(node.getStart());
        auto endLit = dynamic_cast<const LiteralExpression*>(node.getEnd());
        if (startLit && endLit) {
            currentExpressionTypeName_ = startLit->getToken().getValue() + ".." + endLit->getToken().getValue();
            return;
        }
    } else if (startType == DataType::CUSTOM && !startTypeName.empty()) {
        // For enum ranges like Monday..Sunday, preserve the enum type name
        currentExpressionTypeName_ = startTypeName;
        return;
    }
    
    // For other cases, just use the element type
    currentExpressionTypeName_ = "";
}

void SemanticAnalyzer::visit(FormattedExpression& node) {
    // Visit the main expression to get its type
    const_cast<Expression*>(node.getExpression())->accept(*this);
    DataType expressionType = currentExpressionType_;
    
    // Visit width specifier if present
    if (node.getWidth()) {
        const_cast<Expression*>(node.getWidth())->accept(*this);
        if (currentExpressionType_ != DataType::INTEGER) {
            addError("Width specifier must be an integer");
        }
    }
    
    // Visit precision specifier if present
    if (node.getPrecision()) {
        const_cast<Expression*>(node.getPrecision())->accept(*this);
        if (currentExpressionType_ != DataType::INTEGER) {
            addError("Precision specifier must be an integer");
        }
    }
    
    // Result type is the same as the expression being formatted
    currentExpressionType_ = expressionType;
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
    std::string varTypeName = variable->getTypeName();
    if (varType != DataType::INTEGER && varType != DataType::CHAR && 
        varType != DataType::CUSTOM && varType != DataType::UNKNOWN) {
        addError("For loop variable must be an ordinal type, got " + SymbolTable::dataTypeToString(varType));
    }
    
    // Check start expression
    node.getStart()->accept(*this);
    DataType startType = currentExpressionType_;
    std::string startTypeName = currentExpressionTypeName_;
    
    // Check end expression  
    node.getEnd()->accept(*this);
    DataType endType = currentExpressionType_;
    std::string endTypeName = currentExpressionTypeName_;
    
    // Helper lambda to check type compatibility for for loops
    auto isCompatibleForLoopType = [this](DataType varType, const std::string& varTypeName, DataType exprType, const std::string& exprTypeName) -> bool {
        if (varType == exprType) {
            if (varType == DataType::CUSTOM) {
                return varTypeName == exprTypeName;
            }
            return true;
        }
        
        // Check if variable type is a subrange type compatible with expression type
        if (varType == DataType::CUSTOM && !varTypeName.empty()) {
            auto typeSymbol = symbolTable_->lookup(varTypeName);
            if (typeSymbol && typeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
                std::string typeDef = typeSymbol->getTypeDefinition();
                // Check if it's a numeric subrange (e.g., "0..9")
                if (typeDef.find("..") != std::string::npos && typeDef.find("'") == std::string::npos) {
                    return exprType == DataType::INTEGER;
                }
                // Check if it's a character subrange (e.g., "'A'..'Z'")
                if (typeDef.find("..") != std::string::npos && typeDef.find("'") != std::string::npos) {
                    return exprType == DataType::CHAR;
                }
            }
        }
        
        return false;
    };
    
    // Check that start and end expressions are compatible with loop variable
    if (varType != DataType::UNKNOWN && startType != DataType::UNKNOWN && 
        !isCompatibleForLoopType(varType, varTypeName, startType, startTypeName)) {
        addError("For loop start expression type doesn't match variable type");
    }
    if (varType != DataType::UNKNOWN && endType != DataType::UNKNOWN && 
        !isCompatibleForLoopType(varType, varTypeName, endType, endTypeName)) {
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

void SemanticAnalyzer::visit(LabelStatement& node) {
    // Check if the label was declared
    const std::string& label = node.getLabel();
    if (declaredLabels_.find(label) == declaredLabels_.end()) {
        addError("Undeclared label: " + label);
    }
}

void SemanticAnalyzer::visit(GotoStatement& node) {
    // Track referenced labels for validation
    const std::string& target = node.getTarget();
    referencedLabels_.push_back(target);
    
    // Check if the label was declared (this will be validated at the end of scope)
    if (declaredLabels_.find(target) == declaredLabels_.end()) {
        addError("Goto references undeclared label: " + target);
    }
}

void SemanticAnalyzer::visit(BreakStatement& node) {
    (void)node; // Suppress unused parameter warning
    // Break is only valid inside loops
    // For now, we'll allow it - validation could be added later
}

void SemanticAnalyzer::visit(ContinueStatement& node) {
    (void)node; // Suppress unused parameter warning
    // Continue is only valid inside loops  
    // For now, we'll allow it - validation could be added later
}

void SemanticAnalyzer::visit(ConstantDeclaration& node) {
    // Analyze the constant value expression
    node.getValue()->accept(*this);
    
    // Register the constant in the symbol table
    symbolTable_->define(node.getName(), SymbolType::CONSTANT, currentExpressionType_);
}

void SemanticAnalyzer::visit(LabelDeclaration& node) {
    // Register all declared labels
    for (const std::string& label : node.getLabels()) {
        if (declaredLabels_.find(label) != declaredLabels_.end()) {
            addError("Label already declared: " + label);
        } else {
            declaredLabels_.insert(label);
        }
    }
}

void SemanticAnalyzer::visit(TypeDefinition& node) {
    // Check if this is a pointer type definition (starts with ^)
    std::string definition = node.getDefinition();
    if (!definition.empty() && definition[0] == '^') {
        // This is a pointer type
        std::string pointeeTypeName = definition.substr(1); // Remove the ^
        
        // Register as a pointer type
        auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::TYPE_DEF, DataType::POINTER);
        symbol->setTypeDefinition(definition);
        symbol->setPointeeTypeName(pointeeTypeName);
        
        // Try to resolve the pointee type
        DataType pointeeType = symbolTable_->stringToDataType(pointeeTypeName);
        if (pointeeType == DataType::UNKNOWN) {
            pointeeType = symbolTable_->resolveDataType(pointeeTypeName);
        }
        symbol->setPointeeType(pointeeType);
        
        symbolTable_->define(node.getName(), symbol);
        return;
    }
    
    // Register the type definition in the symbol table as a custom type
    auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::TYPE_DEF, DataType::CUSTOM);
    symbol->setTypeDefinition(definition);
    symbolTable_->define(node.getName(), symbol);
    
    // Check if this is an enumeration type and register enum values
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
                
                // Register enum value as a constant with the enum type
                auto enumSymbol = std::make_shared<Symbol>(enumValue, SymbolType::CONSTANT, DataType::CUSTOM);
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
    
    // Add variant part fields if present
    if (node.hasVariantPart()) {
        const VariantPart* variantPart = node.getVariantPart();
        
        // Add the selector field
        recordDef += variantPart->getSelectorName() + ":" + variantPart->getSelectorType() + "; ";
        
        // Add all variant case fields (all fields from all cases are accessible)
        for (const auto& variantCase : variantPart->getCases()) {
            for (const auto& field : variantCase->getFields()) {
                recordDef += field.getName() + ":" + field.getType() + "; ";
            }
        }
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
    
    // Validate variant part field types if present
    if (node.hasVariantPart()) {
        const VariantPart* variantPart = node.getVariantPart();
        
        // Validate selector type
        DataType selectorType = symbolTable_->resolveDataType(variantPart->getSelectorType());
        if (selectorType == DataType::UNKNOWN) {
            addError("Unknown selector type '" + variantPart->getSelectorType() + "' in variant record '" + node.getName() + "'");
        }
        
        // Validate all variant case field types
        for (const auto& variantCase : variantPart->getCases()) {
            for (const auto& field : variantCase->getFields()) {
                DataType fieldType = symbolTable_->resolveDataType(field.getType());
                if (fieldType == DataType::UNKNOWN) {
                    addError("Unknown field type '" + field.getType() + "' in variant record '" + node.getName() + "'");
                }
            }
        }
    }
}

void SemanticAnalyzer::visit(VariableDeclaration& node) {
    DataType dataType = symbolTable_->resolveDataType(node.getType());
    
    // If not a built-in type, check if it's a user-defined type
    if (dataType == DataType::UNKNOWN) {
        auto typeSymbol = symbolTable_->lookup(node.getType());
        if (typeSymbol && typeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
            dataType = typeSymbol->getDataType();
        }
    }
    
    if (dataType == DataType::UNKNOWN) {
        addError("Unknown data type: " + node.getType());
        return;
    }
    
    auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::VARIABLE, dataType);
    symbol->setTypeName(node.getType()); // Store the original type name
    
    // Handle pointer types - extract and store pointee type information
    if (dataType == DataType::POINTER) {
        // First check if this is a user-defined pointer type
        auto typeSymbol = symbolTable_->lookup(node.getType());
        if (typeSymbol && typeSymbol->getSymbolType() == SymbolType::TYPE_DEF) {
            symbol->setPointeeType(typeSymbol->getPointeeType());
            symbol->setPointeeTypeName(typeSymbol->getPointeeTypeName());
        } else {
            // Handle direct pointer syntax like ^Integer
            std::string typeStr = node.getType();
            if (!typeStr.empty() && typeStr[0] == '^') {
                std::string pointeeTypeName = typeStr.substr(1); // Remove the '^' prefix
                DataType pointeeType = symbolTable_->resolveDataType(pointeeTypeName);
                symbol->setPointeeType(pointeeType);
                symbol->setPointeeTypeName(pointeeTypeName);
            }
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
    // Build parameter types vector for overload resolution
    std::vector<DataType> paramTypes;
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        paramTypes.push_back(paramType);
    }
    
    // Check for existing procedure with same signature
    auto existingProcedure = symbolTable_->lookupFunction(node.getName(), paramTypes);
    
    if (node.isForward()) {
        // This is a forward declaration
        if (existingProcedure) {
            addError("Procedure '" + node.getName() + "' with this signature already declared");
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
        
        // Define procedure using overloaded symbol table
        symbolTable_->defineOverloaded(node.getName(), procedureSymbol);
        return;
    }
    
    // This is an implementation - check if it matches a forward declaration
    if (existingProcedure && existingProcedure->getSymbolType() == SymbolType::PROCEDURE) {
        // Forward declaration exists with same signature - verified by lookup
    } else if (!existingProcedure) {
        // No forward declaration with this signature - define new overloaded procedure
        auto procedureSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::PROCEDURE, DataType::VOID, 
                                                       symbolTable_->getCurrentScopeLevel());
        
        // Add parameters to procedure symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            procedureSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define procedure in symbol table using overloaded table for consistency with forward declarations
        symbolTable_->defineOverloaded(node.getName(), procedureSymbol);
    }
    
    // Enter new scope for procedure body
    symbolTable_->enterScope();
    
    // Add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        // Create parameter symbol and preserve the original Pascal type name
        auto paramSymbol = std::make_shared<Symbol>(param->getName(), SymbolType::PARAMETER, paramType, symbolTable_->getCurrentScopeLevel());
        paramSymbol->setTypeName(param->getType());
        symbolTable_->define(param->getName(), paramSymbol);
    }
    
    // Add local variables to current scope
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    // Process nested declarations in the current scope
    // This allows nested procedures to access the parent procedure's variables
    // and be visible within this procedure
    for (const auto& nestedDecl : node.getNestedDeclarations()) {
        nestedDecl->accept(*this);
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
    
    // Build parameter types vector for overload resolution
    std::vector<DataType> paramTypes;
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        paramTypes.push_back(paramType);
    }
    
    // Check for existing function with same signature
    auto existingFunction = symbolTable_->lookupFunction(node.getName(), paramTypes);
    
    if (node.isForward()) {
        // This is a forward declaration
        if (existingFunction) {
            addError("Function '" + node.getName() + "' with this signature already declared");
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
        
        // Define function using overloaded symbol table
        symbolTable_->defineOverloaded(node.getName(), functionSymbol);
        return;
    }
    
    // This is an implementation - check if it matches a forward declaration
    if (existingFunction && existingFunction->getSymbolType() == SymbolType::FUNCTION) {
        // Forward declaration exists with same signature - verify compatibility
        if (existingFunction->getReturnType() != returnType) {
            addError("Function '" + node.getName() + "' implementation return type doesn't match forward declaration");
            return;
        }
        // Note: Parameter types already match since we found by signature
    } else if (!existingFunction) {
        // No forward declaration with this signature - define new overloaded function
        auto functionSymbol = std::make_shared<Symbol>(node.getName(), SymbolType::FUNCTION, returnType,
                                                      symbolTable_->getCurrentScopeLevel());
        functionSymbol->setReturnType(returnType);
        
        // Add parameters to function symbol
        for (const auto& param : node.getParameters()) {
            DataType paramType = symbolTable_->resolveDataType(param->getType());
            functionSymbol->addParameter(param->getName(), paramType);
        }
        
        // Define function in symbol table using overloaded table for proper overload support
        symbolTable_->defineOverloaded(node.getName(), functionSymbol);
    }
    
    // Enter new scope for function body
    symbolTable_->enterScope();
    
    // Add parameters to current scope
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        auto paramSymbol = std::make_shared<Symbol>(param->getName(), SymbolType::PARAMETER, paramType, symbolTable_->getCurrentScopeLevel());
        paramSymbol->setTypeName(param->getType());
        symbolTable_->define(param->getName(), paramSymbol);
    }
    
    // Add function name as a variable for return value assignment
    symbolTable_->define(node.getName(), SymbolType::VARIABLE, returnType);
    
    // Add local variables to current scope
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    // Process nested declarations in the current scope
    // This allows nested procedures/functions to access the parent function's variables
    // and be visible within this function
    for (const auto& nestedDecl : node.getNestedDeclarations()) {
        nestedDecl->accept(*this);
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

void SemanticAnalyzer::addError(const std::string& message, const SourceLocation& location) {
    errors_.push_back("Semantic error at line " + std::to_string(location.line) + 
                     ", column " + std::to_string(location.column) + ": " + message);
}

DataType SemanticAnalyzer::getExpressionType(Expression* expr) {
    expr->accept(*this);
    return currentExpressionType_;
}

bool SemanticAnalyzer::areTypesCompatible(DataType left, DataType right, const std::string& leftTypeName, const std::string& rightTypeName) {
    if (left == right) {
        // For custom types, also check type names
        if (left == DataType::CUSTOM) {
            if (leftTypeName == rightTypeName) {
                return true;
            }
            
            // Special case for set types: check if one is a named set type and the other is a set literal
            // For example: TColorSet vs "set of TColor"
            if (leftTypeName.find("set of ") == 0 && rightTypeName.find("set of ") == 0) {
                // Both are set types, check element types
                std::string leftElement = leftTypeName.substr(7); // Remove "set of "
                std::string rightElement = rightTypeName.substr(7);
                return leftElement == rightElement;
            }
            
            // Check if one is a named set type and the other is a literal set type
            auto leftSetType = symbolTable_->lookup(leftTypeName);
            if (leftSetType && leftSetType->getSymbolType() == SymbolType::TYPE_DEF) {
                std::string leftDef = leftSetType->getTypeDefinition();
                if (leftDef == rightTypeName) {
                    return true;
                }
                
                // If leftDef is "set of SomeType", check if SomeType resolves to the right element type
                if (leftDef.find("set of ") == 0 && rightTypeName.find("set of ") == 0) {
                    std::string leftElement = leftDef.substr(7); // Remove "set of "
                    std::string rightElement = rightTypeName.substr(7);
                    
                    // Check if leftElement is a type alias that resolves to rightElement
                    auto leftElementType = symbolTable_->lookup(leftElement);
                    if (leftElementType && leftElementType->getSymbolType() == SymbolType::TYPE_DEF) {
                        std::string leftElementDef = leftElementType->getTypeDefinition();
                        if (leftElementDef == rightElement) {
                            return true;
                        }
                    }
                }
            }
            
            auto rightSetType = symbolTable_->lookup(rightTypeName);
            if (rightSetType && rightSetType->getSymbolType() == SymbolType::TYPE_DEF) {
                std::string rightDef = rightSetType->getTypeDefinition();
                if (rightDef == leftTypeName) {
                    return true;
                }
                
                // If rightDef is "set of SomeType", check if SomeType resolves to the left element type
                if (rightDef.find("set of ") == 0 && leftTypeName.find("set of ") == 0) {
                    std::string rightElement = rightDef.substr(7); // Remove "set of "
                    std::string leftElement = leftTypeName.substr(7);
                    
                    // Check if rightElement is a type alias that resolves to leftElement
                    auto rightElementType = symbolTable_->lookup(rightElement);
                    if (rightElementType && rightElementType->getSymbolType() == SymbolType::TYPE_DEF) {
                        std::string rightElementDef = rightElementType->getTypeDefinition();
                        if (rightElementDef == leftElement) {
                            return true;
                        }
                    }
                }
            }
            
            // Special case: empty set "set" is compatible with any named set type
            if (leftTypeName == "set" && rightTypeName != "set") {
                // Left is empty set, right is a specific set type - they're compatible
                auto rightType = symbolTable_->lookup(rightTypeName);
                if (rightType && rightType->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string rightDef = rightType->getTypeDefinition();
                    if (rightDef.find("set of ") == 0) {
                        return true;
                    }
                }
            }
            
            if (rightTypeName == "set" && leftTypeName != "set") {
                // Right is empty set, left is a specific set type - they're compatible
                auto leftType = symbolTable_->lookup(leftTypeName);
                if (leftType && leftType->getSymbolType() == SymbolType::TYPE_DEF) {
                    std::string leftDef = leftType->getTypeDefinition();
                    if (leftDef.find("set of ") == 0) {
                        return true;
                    }
                }
            }
            
            return false;
        }
        return true;
    }
    
    // Integer and real are compatible
    if ((left == DataType::INTEGER && right == DataType::REAL) ||
        (left == DataType::REAL && right == DataType::INTEGER)) {
        return true;
    }
    
    // Byte is compatible with integer (common in Pascal)
    if ((left == DataType::BYTE && right == DataType::INTEGER) ||
        (left == DataType::INTEGER && right == DataType::BYTE)) {
        return true;
    }
    
    // Char is compatible with string (for assignments)
    if (left == DataType::STRING && right == DataType::CHAR) {
        return true;
    }
    
    // Handle bounded string compatibility with char and string
    if ((left == DataType::CUSTOM && (right == DataType::CHAR || right == DataType::STRING)) && !leftTypeName.empty()) {
        // Check if left is a bounded string type
        auto leftType = symbolTable_->lookup(leftTypeName);
        if (leftType && leftType->getSymbolType() == SymbolType::TYPE_DEF) {
            std::string leftDef = leftType->getTypeDefinition();
            // Check if it's a bounded string like "string[10]"
            if (leftDef.find("string[") == 0) {
                return true;
            }
        }
    }
    
    if (((left == DataType::CHAR || left == DataType::STRING) && right == DataType::CUSTOM) && !rightTypeName.empty()) {
        // Check if right is a bounded string type
        auto rightType = symbolTable_->lookup(rightTypeName);
        if (rightType && rightType->getSymbolType() == SymbolType::TYPE_DEF) {
            std::string rightDef = rightType->getTypeDefinition();
            // Check if it's a bounded string like "string[10]"
            if (rightDef.find("string[") == 0) {
                return true;
            }
        }
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
    
    // Addition can be arithmetic, string concatenation, set union, or pointer arithmetic
    if (operator_ == TokenType::PLUS) {
        // Pointer arithmetic: pointer + integer = pointer
        if (left == DataType::POINTER && right == DataType::INTEGER) {
            return DataType::POINTER;
        }
        if (left == DataType::INTEGER && right == DataType::POINTER) {
            return DataType::POINTER;
        }
        // Set union and bounded string operations
        if (left == DataType::CUSTOM && right == DataType::CUSTOM) {
            return DataType::CUSTOM;
        }
        // Bounded string operations
        if ((left == DataType::CUSTOM && right == DataType::CHAR) ||
            (left == DataType::CHAR && right == DataType::CUSTOM) ||
            (left == DataType::CUSTOM && right == DataType::STRING) ||
            (left == DataType::STRING && right == DataType::CUSTOM)) {
            return DataType::CUSTOM;
        }
        // String concatenation: string + string, string + char, char + string all return string
        if ((left == DataType::STRING && right == DataType::STRING) ||
            (left == DataType::STRING && right == DataType::CHAR) ||
            (left == DataType::CHAR && right == DataType::STRING)) {
            return DataType::STRING;
        }
        // Arithmetic addition
        if (left == DataType::REAL || right == DataType::REAL) {
            return DataType::REAL;
        }
        return DataType::INTEGER;
    }
    
    // Other arithmetic operators, set operations, or pointer arithmetic
    if (operator_ == TokenType::MINUS || operator_ == TokenType::MULTIPLY || 
        operator_ == TokenType::DIVIDE) {
        // Pointer arithmetic: pointer - integer = pointer, pointer - pointer = integer
        if (operator_ == TokenType::MINUS) {
            if (left == DataType::POINTER && right == DataType::INTEGER) {
                return DataType::POINTER;
            }
            if (left == DataType::POINTER && right == DataType::POINTER) {
                return DataType::INTEGER; // difference between pointers
            }
        }
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
    
    // Range operator: returns the same type as operands (used in case statements, sets)
    if (operator_ == TokenType::RANGE) {
        // For integer ranges, return integer type
        if ((left == DataType::INTEGER || left == DataType::BYTE) &&
            (right == DataType::INTEGER || right == DataType::BYTE)) {
            return DataType::INTEGER;
        }
        // For character ranges, return character type
        if (left == DataType::CHAR && right == DataType::CHAR) {
            return DataType::CHAR;
        }
        return left; // Default to left operand type
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
            // Allow numeric addition, string concatenation, set union, and pointer arithmetic
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL || leftType == DataType::BYTE) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL || rightType == DataType::BYTE)) ||
                   // String concatenation: string + string, string + char, char + string
                   (leftType == DataType::STRING && rightType == DataType::STRING) ||
                   (leftType == DataType::STRING && rightType == DataType::CHAR) ||
                   (leftType == DataType::CHAR && rightType == DataType::STRING) ||
                   // Set operations and bounded string operations
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CHAR) ||
                   (leftType == DataType::CHAR && rightType == DataType::CUSTOM) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::STRING) ||
                   (leftType == DataType::STRING && rightType == DataType::CUSTOM) ||
                   // Pointer arithmetic: pointer + integer
                   (leftType == DataType::POINTER && rightType == DataType::INTEGER) ||
                   (leftType == DataType::INTEGER && rightType == DataType::POINTER);
        case TokenType::MINUS:
            // Allow numeric subtraction, set difference, and pointer arithmetic
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL || leftType == DataType::BYTE) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL || rightType == DataType::BYTE)) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM) ||
                   // Pointer arithmetic: pointer - integer or pointer - pointer
                   (leftType == DataType::POINTER && rightType == DataType::INTEGER) ||
                   (leftType == DataType::POINTER && rightType == DataType::POINTER);
        case TokenType::MULTIPLY:
            // Allow numeric multiplication and set intersection
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL || leftType == DataType::BYTE) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL || rightType == DataType::BYTE)) ||
                   (leftType == DataType::CUSTOM && rightType == DataType::CUSTOM);
        case TokenType::DIVIDE:
            return (leftType == DataType::INTEGER || leftType == DataType::REAL || leftType == DataType::BYTE) &&
                   (rightType == DataType::INTEGER || rightType == DataType::REAL || rightType == DataType::BYTE);
                   
        case TokenType::DIV:
        case TokenType::MOD:
            return (leftType == DataType::INTEGER || leftType == DataType::BYTE) && 
                   (rightType == DataType::INTEGER || rightType == DataType::BYTE);
            
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
            return areTypesCompatible(leftType, rightType);
            
        case TokenType::LESS_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_THAN:
        case TokenType::GREATER_EQUAL:
            return ((leftType == DataType::INTEGER || leftType == DataType::REAL || leftType == DataType::BYTE || leftType == DataType::CHAR) &&
                    (rightType == DataType::INTEGER || rightType == DataType::REAL || rightType == DataType::BYTE || rightType == DataType::CHAR)) ||
                   // Pointer comparisons
                   (leftType == DataType::POINTER && rightType == DataType::POINTER);
                   
        case TokenType::IN:
            // Set membership: element in set
            return (leftType == DataType::INTEGER || leftType == DataType::CHAR || leftType == DataType::CUSTOM) &&
                   (rightType == DataType::CUSTOM);
                   
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::XOR:
            return leftType == DataType::BOOLEAN && rightType == DataType::BOOLEAN;
            
        case TokenType::RANGE:
            // Range operator: integer..integer or char..char (for case statements, sets, etc.)
            return ((leftType == DataType::INTEGER || leftType == DataType::BYTE) &&
                    (rightType == DataType::INTEGER || rightType == DataType::BYTE)) ||
                   (leftType == DataType::CHAR && rightType == DataType::CHAR);
            
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
    
    // Check for built-in functions first
    if (isBuiltinFunction(functionName)) {
        handleBuiltinFunction(functionName, node);
        return;
    }
    
    // Build argument types for overload resolution
    std::vector<DataType> argTypes;
    for (const auto& arg : node.getArguments()) {
        DataType argType = getExpressionType(arg.get());
        argTypes.push_back(argType);
    }
    
    // Try to find function with matching signature
    auto symbol = symbolTable_->lookupFunction(functionName, argTypes);
    
    if (!symbol) {
        // Try regular lookup for non-overloaded functions
        symbol = symbolTable_->lookup(functionName);
    }
    
    if (!symbol) {
        // Check if any overloads exist and try to find a match
        auto allOverloads = symbolTable_->lookupAllOverloads(functionName);
        if (!allOverloads.empty()) {
            // Try to find an exact parameter match
            for (const auto& overload : allOverloads) {
                if (overload->matchesSignature(argTypes)) {
                    symbol = overload;
                    break;
                }
            }
            
            if (!symbol) {
                addError("No matching overload for function '" + functionName + "' with given argument types");
            }
        } else {
            addError("Undefined function: " + functionName);
        }
        
        if (!symbol) {
            currentExpressionType_ = DataType::UNKNOWN;
            return;
        }
    }
    
    if (symbol->getSymbolType() != SymbolType::FUNCTION && 
        symbol->getSymbolType() != SymbolType::PROCEDURE) {
        addError("'" + functionName + "' is not a function or procedure");
        currentExpressionType_ = DataType::UNKNOWN;
        return;
    }
    
    // Special handling for built-in functions like writeln (variable arguments)
    if (functionName == "writeln" || functionName == "write" || functionName == "readln") {
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
        // Check if unit is a standard unit (System, Dos, Crt)
        if (unitName == "System" || unitName == "Dos" || unitName == "Crt") {
            // Built-in units are handled automatically
            continue;
        }
        
        // Try to load custom unit
        if (!unitLoader_->isUnitLoaded(unitName)) {
            unitLoader_->loadUnit(unitName);
            
            // Check if loading succeeded
            if (!unitLoader_->isUnitLoaded(unitName)) {
                addError("Failed to load unit: " + unitName);
                continue;
            }
        }
        
        // Get the loaded unit and process its interface declarations
        Unit* loadedUnit = unitLoader_->getLoadedUnit(unitName);
        if (loadedUnit) {
            // Import symbols from the unit's interface section
            for (const auto& decl : loadedUnit->getInterfaceDeclarations()) {
                // Process the declaration to add symbols to our symbol table
                decl->accept(*this);
            }
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

bool SemanticAnalyzer::isBuiltinFunction(const std::string& functionName) {
    // Convert to lowercase for comparison
    std::string lowerName = functionName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    return lowerName == "writeln" || lowerName == "write" || lowerName == "readln" || lowerName == "read" ||
           lowerName == "length" || lowerName == "chr" || lowerName == "ord" || 
           lowerName == "pos" || lowerName == "copy" || lowerName == "concat" ||
           lowerName == "insert" || lowerName == "delete" || lowerName == "assign" ||
           lowerName == "reset" || lowerName == "rewrite" || lowerName == "close" ||
           lowerName == "eof" || lowerName == "new" || lowerName == "dispose" ||
           // File operations
           lowerName == "blockread" || lowerName == "blockwrite" || 
           lowerName == "filepos" || lowerName == "filesize" || lowerName == "seek" ||
           // System unit mathematical functions
           lowerName == "abs" || lowerName == "sqr" || lowerName == "sqrt" ||
           lowerName == "sin" || lowerName == "cos" || lowerName == "arctan" ||
           lowerName == "ln" || lowerName == "exp" || lowerName == "power" ||
           lowerName == "round" || lowerName == "trunc" || lowerName == "tan" ||
           // System unit conversion functions  
           lowerName == "val" || lowerName == "str" || lowerName == "inttostr" ||
           lowerName == "floattostr" || lowerName == "strtoint" || lowerName == "strtofloat" ||
           // System unit string functions
           lowerName == "upcase" || lowerName == "trim" || lowerName == "trimleft" ||
           lowerName == "trimright" || lowerName == "stringofchar" || lowerName == "lowercase" ||
           lowerName == "uppercase" || lowerName == "leftstr" || lowerName == "rightstr" ||
           lowerName == "padleft" || lowerName == "padright" ||
           // System unit I/O functions
           lowerName == "paramcount" || lowerName == "paramstr" ||
           // System unit system functions
           lowerName == "halt" || lowerName == "exit" || lowerName == "random" || 
           lowerName == "randomize" ||
           // Pointer arithmetic functions
           lowerName == "inc" || lowerName == "dec" ||
           // Dynamic memory allocation functions
           lowerName == "getmem" || lowerName == "freemem" || lowerName == "mark" || lowerName == "release" ||
           // CRT unit functions
           lowerName == "clrscr" || lowerName == "clreol" || lowerName == "gotoxy" ||
           lowerName == "wherex" || lowerName == "wherey" || lowerName == "textcolor" ||
           lowerName == "textbackground" || lowerName == "lowvideo" || lowerName == "highvideo" ||
           lowerName == "normvideo" || lowerName == "window" || lowerName == "keypressed" ||
           lowerName == "readkey" || lowerName == "sound" || lowerName == "nosound" ||
           lowerName == "delay" || lowerName == "cursoron" || lowerName == "cursoroff" ||
           // DOS unit functions
           lowerName == "fileexists" || lowerName == "filesize" || lowerName == "findfirst" ||
           lowerName == "findnext" || lowerName == "findclose" || lowerName == "getcurrentdir" ||
           lowerName == "setcurrentdir" || lowerName == "directoryexists" || lowerName == "mkdir" ||
           lowerName == "rmdir" || lowerName == "getdate" || lowerName == "gettime" ||
           lowerName == "getdatetime" || lowerName == "getenv" || lowerName == "exec";
}

bool SemanticAnalyzer::isBuiltinConstant(const std::string& constantName) {
    // Convert to lowercase for comparison
    std::string lowerName = constantName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // CRT color constants
    return lowerName == "black" || lowerName == "blue" || lowerName == "green" ||
           lowerName == "cyan" || lowerName == "red" || lowerName == "magenta" ||
           lowerName == "brown" || lowerName == "lightgray" || lowerName == "darkgray" ||
           lowerName == "lightblue" || lowerName == "lightgreen" || lowerName == "lightcyan" ||
           lowerName == "lightred" || lowerName == "lightmagenta" || lowerName == "yellow" ||
           lowerName == "white" || lowerName == "blink";
}

void SemanticAnalyzer::handleBuiltinFunction(const std::string& functionName, CallExpression& node) {
    // Process arguments for type checking
    for (const auto& arg : node.getArguments()) {
        arg->accept(*this);
    }
    
    // Convert to lowercase for comparison
    std::string lowerName = functionName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Set return type based on function
    if (lowerName == "writeln" || lowerName == "write" || lowerName == "readln" || lowerName == "read" ||
        lowerName == "insert" || lowerName == "delete" || lowerName == "assign" ||
        lowerName == "reset" || lowerName == "rewrite" || lowerName == "close" ||
        lowerName == "new" || lowerName == "dispose" || lowerName == "halt" ||
        lowerName == "exit" || lowerName == "randomize" ||
        // CRT procedures (no return value)
        lowerName == "clrscr" || lowerName == "clreol" || lowerName == "gotoxy" ||
        lowerName == "textcolor" || lowerName == "textbackground" || lowerName == "lowvideo" ||
        lowerName == "highvideo" || lowerName == "normvideo" || lowerName == "window" ||
        lowerName == "sound" || lowerName == "nosound" || lowerName == "delay" ||
        lowerName == "cursoron" || lowerName == "cursoroff" ||
        // DOS procedures (no return value)
        lowerName == "findfirst" || lowerName == "findnext" || lowerName == "findclose" ||
        lowerName == "setcurrentdir" || lowerName == "mkdir" || lowerName == "rmdir" ||
        lowerName == "getdatetime") {
        currentExpressionType_ = DataType::VOID;
    } else if (lowerName == "length" || lowerName == "ord" || lowerName == "pos" ||
               lowerName == "paramcount" || lowerName == "abs" ||
               // CRT functions returning integer
               lowerName == "wherex" || lowerName == "wherey" ||
               // Mathematical functions returning integer
               lowerName == "round" || lowerName == "trunc" ||
               // Conversion functions returning integer/real
               lowerName == "strtoint" ||
               // DOS functions returning integer
               lowerName == "filesize" || lowerName == "getdate" || lowerName == "gettime" ||
               lowerName == "exec") {
        currentExpressionType_ = DataType::INTEGER;
    } else if (lowerName == "chr" ||
               // CRT functions returning char
               lowerName == "readkey") {
        currentExpressionType_ = DataType::CHAR;
    } else if (lowerName == "copy" || lowerName == "concat" || lowerName == "upcase" ||
               lowerName == "str" || lowerName == "paramstr" ||
               // Additional string functions returning string
               lowerName == "trim" || lowerName == "trimleft" || lowerName == "trimright" ||
               lowerName == "stringofchar" || lowerName == "lowercase" || lowerName == "uppercase" ||
               lowerName == "leftstr" || lowerName == "rightstr" || lowerName == "padleft" ||
               lowerName == "padright" ||
               // Conversion functions returning string
               lowerName == "inttostr" || lowerName == "floattostr" ||
               // DOS functions returning string
               lowerName == "getcurrentdir" || lowerName == "getenv") {
        currentExpressionType_ = DataType::STRING;
    } else if (lowerName == "sqr" || lowerName == "sqrt" || lowerName == "sin" ||
               lowerName == "cos" || lowerName == "arctan" || lowerName == "ln" ||
               lowerName == "exp" || lowerName == "random" ||
               // Additional mathematical functions returning real
               lowerName == "power" || lowerName == "tan" ||
               // Conversion functions returning real
               lowerName == "strtofloat") {
        currentExpressionType_ = DataType::REAL;
    } else if (lowerName == "eof" ||
               // CRT functions returning boolean
               lowerName == "keypressed" ||
               // DOS functions returning boolean
               lowerName == "fileexists" || lowerName == "directoryexists") {
        currentExpressionType_ = DataType::BOOLEAN;
    } else if (lowerName == "filepos" || lowerName == "filesize") {
        // File position functions return integer
        currentExpressionType_ = DataType::INTEGER;
    } else if (lowerName == "blockread" || lowerName == "blockwrite" || lowerName == "seek") {
        // File operations are procedures (void)
        currentExpressionType_ = DataType::VOID;
    } else {
        currentExpressionType_ = DataType::UNKNOWN;
    }
    currentExpressionTypeName_ = ""; // Built-in functions return built-in types
}

} // namespace rpascal