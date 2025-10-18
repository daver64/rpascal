#include "../include/cpp_generator.h"
#include <algorithm>
#include <iostream>

namespace rpascal {

CppGenerator::CppGenerator(std::shared_ptr<SymbolTable> symbolTable, UnitLoader* unitLoader)
    : symbolTable_(symbolTable), unitLoader_(unitLoader), indentLevel_(0) {}

std::string CppGenerator::generate(Program& program) {
    output_.str("");
    output_.clear();
    indentLevel_ = 0;
    
    program.accept(*this);
    return output_.str();
}

void CppGenerator::visit(LiteralExpression& node) {
    const Token& token = node.getToken();
    
    switch (token.getType()) {
        case TokenType::INTEGER_LITERAL:
        case TokenType::REAL_LITERAL:
            emit(token.getValue());
            break;
        case TokenType::STRING_LITERAL:
            emit("\"" + escapeCppString(token.getValue()) + "\"");
            break;
        case TokenType::CHAR_LITERAL:
            // Handle Pascal char literals like #65
            if (token.getValue().front() == '#') {
                std::string numStr = token.getValue().substr(1);
                emit("static_cast<char>(" + numStr + ")");
            } else {
                emit("'" + escapeCppString(token.getValue()) + "'");
            }
            break;
        case TokenType::TRUE:
            emit("true");
            break;
        case TokenType::FALSE:
            emit("false");
            break;
        case TokenType::NIL:
            emit("nullptr");
            break;
        default:
            emit(token.getValue());
    }
}

void CppGenerator::visit(IdentifierExpression& node) {
    std::string name = node.getName();
    
    // Check if this is a user-defined enumeration constant first
    if (symbolTable_) {
        auto symbol = symbolTable_->lookup(name);
        if (symbol && symbol->getSymbolType() == SymbolType::CONSTANT && 
            symbol->getDataType() == DataType::CUSTOM) {
            // This is a user-defined enumeration constant, emit as-is
            emit(name);
            return;
        }
    }
    
    // Check if this is a built-in constant
    if (isBuiltinConstant(name)) {
        emit(std::to_string(getBuiltinConstantValue(name)));
        return;
    }
    
    // Check if this is a with field access
    if (node.isWithFieldAccess()) {
        emit(node.getWithVariable() + "." + name);
        return;
    }
    
    // Check if this is a procedure call without parentheses
    if (symbolTable_) {
        auto symbol = symbolTable_->lookup(name);
        if (symbol && symbol->getSymbolType() == SymbolType::PROCEDURE) {
            // Check if this is a builtin procedure and handle it specially
            if (isBuiltinFunction(name)) {
                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                
                if (lowerName == "randomize") {
                    emit("std::srand(static_cast<unsigned int>(std::time(nullptr)))");
                    return;
                } else if (lowerName == "exit") {
                    emit("return");
                    return;
                } else if (lowerName == "clrscr") {
                    emit("#ifdef _WIN32\n    system(\"cls\");\n#else\n    system(\"clear\");\n#endif");
                    return;
                } else if (lowerName == "clreol") {
                    emit("std::cout << \"\\033[K\"");
                    return;
                } else if (lowerName == "lowvideo" || lowerName == "highvideo" || lowerName == "normvideo" ||
                           lowerName == "cursoron" || lowerName == "cursoroff" || lowerName == "nosound") {
                    emit("/* " + name + " not implemented */");
                    return;
                }
            }
            // For procedures called without parameters, add parentheses for C++
            emit(name + "()");
            return;
        } else if (symbol && symbol->getSymbolType() == SymbolType::FUNCTION) {
            // Check if this is a builtin function without parameters
            if (isBuiltinFunction(name)) {
                if (name == "paramcount") {
                    emit("(pascal_argc - 1)");
                    return;
                } else if (name == "random") {
                    emit("(static_cast<double>(std::rand()) / RAND_MAX)");
                    return;
                }
            }
            // For user-defined functions, we might need to add () for C++
            // For now, just emit the name for functions
            emit(name);
            return;
        }
    }
    
    // Regular variable or function reference
    emit(name);
}

void CppGenerator::visit(BinaryExpression& node) {
    // Handle range operator in case statements (1..10 means values 1 through 10)
    if (node.getOperator().getType() == TokenType::RANGE) {
        // For now, emit a comment indicating this is a range
        // In a full implementation, we'd need to detect case statements and generate multiple case labels
        emit("/* RANGE: ");
        node.getLeft()->accept(*this);
        emit(" to ");
        node.getRight()->accept(*this);
        emit(" */");
        // We'll need to handle this properly in case statements later
        return;
    }
    
    // Handle special operators
    if (node.getOperator().getType() == TokenType::IN) {
        // Pascal 'in' operator: element in set
        // Use lambda to avoid duplicate set creation: [&](){ auto s = set; return s.find(element) != s.end(); }()
        emit("([&](){ auto temp_set = ");
        node.getRight()->accept(*this); // set
        emit("; return temp_set.find(static_cast<int>(");
        node.getLeft()->accept(*this);  // element - cast to int for enum compatibility
        emit(")) != temp_set.end(); })()");
        return;
    }
    
    // Handle set operations (union, intersection, difference)
    TokenType op = node.getOperator().getType();
    if (op == TokenType::PLUS || op == TokenType::MULTIPLY || op == TokenType::MINUS) {
        // Check if we're dealing with sets by looking for set types
        bool mightBeSetOperation = false;
        
        // Helper function to check if an expression is definitely a set
        auto isSetExpression = [this](Expression* expr) -> bool {
            // Check for set literals
            if (dynamic_cast<SetLiteralExpression*>(expr)) {
                return true;
            }
            
            // Check for set variables
            if (auto identExpr = dynamic_cast<IdentifierExpression*>(expr)) {
                auto symbol = symbolTable_->lookup(identExpr->getName());
                if (symbol && symbol->getDataType() == DataType::CUSTOM) {
                    std::string typeName = symbol->getTypeName();
                    if (typeName.find("Set") != std::string::npos || typeName.find("set") != std::string::npos) {
                        return true;
                    }
                }
            }
            
            // Don't assume nested binary expressions are sets - let them be handled by the caller
            
            return false;
        };
        
        // Check if either operand is a set expression
        if (isSetExpression(node.getLeft()) || isSetExpression(node.getRight())) {
            mightBeSetOperation = true;
        }
        
        if (mightBeSetOperation) {
            // Generate set operations using simpler C++ approach
            if (op == TokenType::PLUS) {
                // Set union: use std::set_union algorithm
                emit("([&](){ auto left = ");
                node.getLeft()->accept(*this);
                emit("; auto right = ");
                node.getRight()->accept(*this);
                emit("; std::remove_reference_t<decltype(left)> result; std::set_union(left.begin(), left.end(), right.begin(), right.end(), std::inserter(result, result.end())); return result; })()");
                return;
            } else if (op == TokenType::MULTIPLY) {
                // Set intersection: use std::set_intersection algorithm
                emit("([&](){ auto left = ");
                node.getLeft()->accept(*this);
                emit("; auto right = ");
                node.getRight()->accept(*this);
                emit("; std::remove_reference_t<decltype(left)> result; std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::inserter(result, result.end())); return result; })()");
                return;
            } else if (op == TokenType::MINUS) {
                // Set difference: use std::set_difference algorithm
                emit("([&](){ ");
                
                // For set literals, we need to wrap them with std::set constructor
                if (dynamic_cast<SetLiteralExpression*>(node.getLeft())) {
                    emit("std::set<char> left");
                    node.getLeft()->accept(*this);
                } else {
                    emit("auto left = ");
                    node.getLeft()->accept(*this);
                }
                
                emit("; ");
                
                if (dynamic_cast<SetLiteralExpression*>(node.getRight())) {
                    emit("std::set<char> right");
                    node.getRight()->accept(*this);
                } else {
                    emit("auto right = ");
                    node.getRight()->accept(*this);
                }
                
                emit("; std::remove_reference_t<decltype(left)> result; std::set_difference(left.begin(), left.end(), right.begin(), right.end(), std::inserter(result, result.end())); return result; })()");
                return;
            }
        }
    }
    
    // Handle set comparisons (== and !=)
    if (node.getOperator().getType() == TokenType::EQUAL || node.getOperator().getType() == TokenType::NOT_EQUAL) {
        // Check if we're comparing sets
        bool mightBeSetComparison = false;
        
        // Check if operands look like set operations (similar to above heuristic)
        if (auto leftIdent = dynamic_cast<IdentifierExpression*>(node.getLeft())) {
            auto symbol = symbolTable_->lookup(leftIdent->getName());
            if (symbol && symbol->getDataType() == DataType::CUSTOM) {
                std::string typeName = symbol->getTypeName();
                if (typeName.find("Set") != std::string::npos || typeName.find("set") != std::string::npos) {
                    mightBeSetComparison = true;
                }
            }
        }
        
        // Also check for set literals
        if (dynamic_cast<SetLiteralExpression*>(node.getLeft()) || 
            dynamic_cast<SetLiteralExpression*>(node.getRight())) {
            mightBeSetComparison = true;
        }
        
        if (mightBeSetComparison) {
            emit("(");
            node.getLeft()->accept(*this);
            emit(" " + mapPascalOperatorToCpp(node.getOperator().getType()) + " ");
            
            // If right side is a set literal, we need to wrap it in a set constructor
            if (dynamic_cast<SetLiteralExpression*>(node.getRight())) {
                emit("std::set<int>");
                node.getRight()->accept(*this);  // This will emit the {...} part
            } else {
                node.getRight()->accept(*this);
            }
            emit(")");
            return;
        }
    }
    
    // Handle string concatenation vs numeric addition
    if (node.getOperator().getType() == TokenType::PLUS) {
        // Only use string concatenation if at least one operand is actually a string
        if (isStringExpression(node.getLeft()) || isStringExpression(node.getRight())) {
            // Use std::string constructor to ensure at least one operand is a std::string
            emit("(std::string(");
            node.getLeft()->accept(*this);
            emit(") + ");
            node.getRight()->accept(*this);
            emit(")");
            return;
        }
        // Otherwise fall through to standard numeric addition
    }
    
    // Standard binary operators
    emit("(");
    node.getLeft()->accept(*this);
    emit(" " + mapPascalOperatorToCpp(node.getOperator().getType()) + " ");
    node.getRight()->accept(*this);
    emit(")");
}

void CppGenerator::visit(UnaryExpression& node) {
    std::string op = mapPascalOperatorToCpp(node.getOperator().getType());
    emit(op + "(");
    node.getOperand()->accept(*this);
    emit(")");
}

void CppGenerator::visit(AddressOfExpression& node) {
    emit("&(");
    node.getOperand()->accept(*this);
    emit(")");
}

void CppGenerator::visit(DereferenceExpression& node) {
    emit("*(");
    node.getOperand()->accept(*this);
    emit(")");
}

void CppGenerator::visit(CallExpression& node) {
    generateFunctionCall(node);
}

void CppGenerator::visit(FieldAccessExpression& node) {
    // Check if the object is a dereference expression
    if (auto dereferenceExpr = dynamic_cast<DereferenceExpression*>(node.getObject())) {
        // Use -> syntax instead of *(ptr).field
        dereferenceExpr->getOperand()->accept(*this);
        emit("->");
        emit(node.getFieldName());
    } else {
        // Regular field access
        node.getObject()->accept(*this);
        emit(".");
        emit(node.getFieldName());
    }
}

void CppGenerator::visit(ArrayIndexExpression& node) {
    // Get the array variable name to look up its type
    std::string arrayVarName;
    if (auto* identifierExpr = dynamic_cast<IdentifierExpression*>(node.getArray())) {
        arrayVarName = identifierExpr->getName();
    }
    
    // Look up the variable's type from symbol table
    std::string arrayTypeName;
    DataType arrayDataType = DataType::UNKNOWN;
    if (!arrayVarName.empty() && symbolTable_) {
        auto symbol = symbolTable_->lookup(arrayVarName);
        if (symbol && symbol->getSymbolType() == SymbolType::VARIABLE) {
            arrayTypeName = symbol->getTypeName();
            arrayDataType = symbol->getDataType();
        }
    }
    
    const std::vector<std::unique_ptr<Expression>>& indices = node.getIndices();
    
    // Special handling for string indexing
    if (arrayDataType == DataType::STRING && indices.size() == 1) {
        // Pascal strings use 1-based indexing, C++ uses 0-based
        node.getArray()->accept(*this);
        emit("[");
        emit("(");
        indices[0]->accept(*this);
        emit(") - 1");
        emit("]");
        return;
    }
    
    // Check if we have array type information
    auto arrayTypeIt = arrayTypes_.find(arrayTypeName);
    if (arrayTypeIt != arrayTypes_.end()) {
        // Use stored array type info (for complex multi-dimensional arrays)
        const ArrayTypeInfo& info = arrayTypeIt->second;
        
        if (info.dimensions.size() > 1 && indices.size() == info.dimensions.size()) {
            // Multi-dimensional array - use flattened index calculation
            node.getArray()->accept(*this);
            emit("[");
            
            // Calculate flattened index: index = (i1-start1)*size2*size3 + (i2-start2)*size3 + (i3-start3)
            for (size_t i = 0; i < indices.size(); ++i) {
                if (i > 0) emit(" + ");
                
                emit("(");
                
                // Handle enum indices differently
                if (info.dimensions[i].isEnumRange) {
                    emit("static_cast<int>(");
                    indices[i]->accept(*this);
                    emit(")");
                } else {
                    indices[i]->accept(*this);
                    emit(" - " + std::to_string(info.dimensions[i].startIndex));
                }
                
                emit(")");
                
                // Multiply by the size of all subsequent dimensions
                for (size_t j = i + 1; j < info.dimensions.size(); ++j) {
                    int dimSize;
                    if (info.dimensions[j].isEnumRange) {
                        auto enumIt = enumTypes_.find(info.dimensions[j].enumTypeName);
                        dimSize = enumIt != enumTypes_.end() ? enumIt->second.size() : 1;
                    } else {
                        dimSize = info.dimensions[j].endIndex - info.dimensions[j].startIndex + 1;
                    }
                    emit(" * " + std::to_string(dimSize));
                }
            }
            
            emit("]");
        } else if (indices.size() == 1) {
            // Single dimension array
            node.getArray()->accept(*this);
            emit("[");
            
            if (!info.dimensions.empty()) {
                if (info.dimensions[0].isEnumRange) {
                    emit("static_cast<int>(");
                    indices[0]->accept(*this);
                    emit(")");
                } else {
                    emit("(");
                    indices[0]->accept(*this);
                    emit(") - " + std::to_string(info.dimensions[0].startIndex));
                }
            } else {
                // Legacy single dimension support
                emit("(");
                indices[0]->accept(*this);
                emit(") - " + std::to_string(info.startIndex));
            }
            
            emit("]");
        } else {
            // Dimension mismatch - fallback
            node.getArray()->accept(*this);
            emit("[");
            indices[0]->accept(*this);
            emit(" - 1]");
        }
    } else {
        // No type information - parse array bounds from type name
        node.getArray()->accept(*this);
        emit("[");
        
        int startIndex = 1; // Default to 1-based indexing (Pascal default)
        
        // Try to extract start index from array type name like "array[0..2] of Byte"
        if (!arrayTypeName.empty() && arrayTypeName.find("array[") == 0) {
            size_t bracketStart = arrayTypeName.find('[');
            size_t rangePos = arrayTypeName.find("..");
            if (bracketStart != std::string::npos && rangePos != std::string::npos) {
                std::string startStr = arrayTypeName.substr(bracketStart + 1, rangePos - bracketStart - 1);
                try {
                    startIndex = std::stoi(startStr);
                } catch (const std::exception&) {
                    startIndex = 1; // Fallback to 1 if parsing fails
                }
            }
        }
        
        if (indices.size() == 1) {
            if (startIndex == 0) {
                // 0-based array - no offset needed
                indices[0]->accept(*this);
            } else {
                // N-based array - subtract start index
                emit("(");
                indices[0]->accept(*this);
                emit(") - " + std::to_string(startIndex));
            }
        } else {
            // Multi-dimensional but no type info - just use first index with offset
            if (startIndex == 0) {
                indices[0]->accept(*this);
            } else {
                emit("(");
                indices[0]->accept(*this);
                emit(") - " + std::to_string(startIndex));
            }
        }
        
        emit("]");
    }
}

void CppGenerator::visit(SetLiteralExpression& node) {
    // For now, generate a universal set literal that can be assigned to different set types
    // We'll use brace initialization which can be converted to the target type
    emit("{");
    const auto& elements = node.getElements();
    bool first = true;
    
    for (const auto& element : elements) {
        // Check if this element is a range expression that needs expansion
        if (auto rangeExpr = dynamic_cast<RangeExpression*>(element.get())) {
            // For ranges like 0..9 or 'a'..'z', we need to expand them
            // For simplicity, we'll generate code that works for integer ranges
            // A full implementation would handle char ranges and enum ranges too
            
            // Expand integer ranges at compile time
            // Try to get literal values for start and end
            auto startLit = dynamic_cast<const LiteralExpression*>(rangeExpr->getStart());
            auto endLit = dynamic_cast<const LiteralExpression*>(rangeExpr->getEnd());
            
            if (startLit && endLit) {
                // Check if these are integer literals
                std::string startStr = startLit->getToken().getValue();
                std::string endStr = endLit->getToken().getValue();
                
                // Try to parse as integers - if it fails, check for character literals
                bool isIntegerRange = true;
                int startVal = 0, endVal = 0;
                try {
                    startVal = std::stoi(startStr);
                    endVal = std::stoi(endStr);
                } catch (const std::exception&) {
                    isIntegerRange = false;
                }
                
                if (isIntegerRange) {
                    // Expand the integer range
                    for (int i = startVal; i <= endVal; ++i) {
                        if (!first) emit(", ");
                        emit(std::to_string(i));
                        first = false;
                    }
                } else if (startStr.length() == 3 && startStr[0] == '\'' && startStr[2] == '\'' &&
                          endStr.length() == 3 && endStr[0] == '\'' && endStr[2] == '\'') {
                    // Character range like 'a'..'z'
                    char startChar = startStr[1];
                    char endChar = endStr[1];
                    for (char c = startChar; c <= endChar; ++c) {
                        if (!first) emit(", ");
                        emit("'");
                        emit(std::string(1, c));
                        emit("'");
                        first = false;
                    }
                } else if (startStr.length() == 1 && endStr.length() == 1 &&
                          std::isalpha(startStr[0]) && std::isalpha(endStr[0])) {
                    // Single character tokens (without quotes) - assume they're characters
                    char startChar = startStr[0];
                    char endChar = endStr[0];
                    for (char c = startChar; c <= endChar; ++c) {
                        if (!first) emit(", ");
                        emit("'");
                        emit(std::string(1, c));
                        emit("'");
                        first = false;
                    }
                } else {
                    // For enum ranges, we need to expand based on enum definition
                    std::vector<std::string> enumValues = expandEnumRange(startStr, endStr);
                    if (!enumValues.empty()) {
                        for (size_t i = 0; i < enumValues.size(); ++i) {
                            if (!first) emit(", ");
                            // Cast enum values to int for set compatibility
                            emit("static_cast<int>(");
                            emit(enumValues[i]);
                            emit(")");
                            first = false;
                        }
                    } else {
                        // Fallback: cast enum values to int
                        if (!first) emit(", ");
                        emit("static_cast<int>(");
                        emit(startStr);
                        emit("), static_cast<int>(");
                        emit(endStr);
                        emit(")");
                        first = false;
                    }
                }
            } else {
                // Try to get identifier names for enum range expansion
                auto startIdent = dynamic_cast<const IdentifierExpression*>(rangeExpr->getStart());
                auto endIdent = dynamic_cast<const IdentifierExpression*>(rangeExpr->getEnd());
                
                if (startIdent && endIdent) {
                    std::string startName = startIdent->getName();
                    std::string endName = endIdent->getName();
                    
                    // Try enum range expansion
                    std::vector<std::string> enumValues = expandEnumRange(startName, endName);
                    if (!enumValues.empty()) {
                        for (size_t i = 0; i < enumValues.size(); ++i) {
                            if (!first) emit(", ");
                            emit("static_cast<int>(");
                            emit(enumValues[i]);
                            emit(")");
                            first = false;
                        }
                    } else {
                        // Fallback: emit start and end with proper enum casting
                        if (!first) emit(", ");
                        
                        // Cast enum constants to int for set compatibility
                        if (startIdent && symbolTable_) {
                            auto symbol = symbolTable_->lookup(startIdent->getName());
                            if (symbol && symbol->getSymbolType() == SymbolType::CONSTANT && 
                                symbol->getDataType() == DataType::CUSTOM) {
                                emit("static_cast<int>(");
                                const_cast<Expression*>(rangeExpr->getStart())->accept(*this);
                                emit(")");
                            } else {
                                const_cast<Expression*>(rangeExpr->getStart())->accept(*this);
                            }
                        } else {
                            const_cast<Expression*>(rangeExpr->getStart())->accept(*this);
                        }
                        
                        emit(", ");
                        
                        if (endIdent && symbolTable_) {
                            auto symbol = symbolTable_->lookup(endIdent->getName());
                            if (symbol && symbol->getSymbolType() == SymbolType::CONSTANT && 
                                symbol->getDataType() == DataType::CUSTOM) {
                                emit("static_cast<int>(");
                                const_cast<Expression*>(rangeExpr->getEnd())->accept(*this);
                                emit(")");
                            } else {
                                const_cast<Expression*>(rangeExpr->getEnd())->accept(*this);
                            }
                        } else {
                            const_cast<Expression*>(rangeExpr->getEnd())->accept(*this);
                        }
                        
                        first = false;
                    }
                } else {
                    // Not identifier expressions, use original fallback
                    if (!first) emit(", ");
                    const_cast<Expression*>(rangeExpr->getStart())->accept(*this);
                    emit(", ");
                    const_cast<Expression*>(rangeExpr->getEnd())->accept(*this);
                    first = false;
                }
            }
            
            first = false;
        } else {
            if (!first) emit(", ");
            
            // Check if this element is an enum constant that needs casting to int
            if (auto identExpr = dynamic_cast<IdentifierExpression*>(element.get())) {
                std::string elemName = identExpr->getName();
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(elemName);
                    if (symbol && symbol->getSymbolType() == SymbolType::CONSTANT && 
                        symbol->getDataType() == DataType::CUSTOM) {
                        // This is an enum constant, cast it to int for set compatibility
                        emit("static_cast<int>(");
                        element->accept(*this);
                        emit(")");
                    } else {
                        element->accept(*this);
                    }
                } else {
                    element->accept(*this);
                }
            } else {
                element->accept(*this);
            }
            first = false;
        }
    }
    emit("}");
}

void CppGenerator::visit(RangeExpression& node) {
    // In C++, we need to expand ranges to individual elements
    // This is typically handled in the context where the range is used (like set literals)
    // For now, we'll generate a placeholder that indicates this should be expanded
    emit("/* range: ");
    const_cast<Expression*>(node.getStart())->accept(*this);
    emit(" .. ");
    const_cast<Expression*>(node.getEnd())->accept(*this);
    emit(" */");
    // Note: Range expansion will be handled by the parent context (e.g., SetLiteralExpression)
}

void CppGenerator::visit(FormattedExpression& node) {
    // For formatted output in C++, we'll use the main expression
    // Width and precision will be handled by the output stream
    // For now, just output the expression (formatting will be done at runtime)
    const_cast<Expression*>(node.getExpression())->accept(*this);
    
    // TODO: In a full implementation, we could generate proper formatting code
    // For Pascal format "expr:width:precision", we could generate:
    // std::setw(width) << std::setprecision(precision) << expr
}

void CppGenerator::visit(ExpressionStatement& node) {
    emitIndent();
    node.getExpression()->accept(*this);
    emitLine(";");
}

void CppGenerator::visit(CompoundStatement& node) {
    for (const auto& stmt : node.getStatements()) {
        stmt->accept(*this);
    }
}

void CppGenerator::visit(AssignmentStatement& node) {
    emitIndent();
    
    // Special handling for Pascal function return value assignment
    auto targetId = dynamic_cast<IdentifierExpression*>(node.getTarget());
    if (targetId && !currentFunctionOriginalName_.empty() && targetId->getName() == currentFunctionOriginalName_) {
        // This is an assignment to the function name (return value)
        emit(currentFunctionOriginalName_ + "_result = ");
        node.getValue()->accept(*this);
    } else {
        node.getTarget()->accept(*this);
        emit(" = ");
        
        // Check if we need type conversion for char to string assignment
        if (needsCharToStringConversion(node)) {
            emit("std::string(1, ");
            node.getValue()->accept(*this);
            emit(")");
        } else {
            node.getValue()->accept(*this);
        }
    }
    
    emitLine(";");
}

void CppGenerator::visit(IfStatement& node) {
    emitIndent();
    emit("if (");
    node.getCondition()->accept(*this);
    emitLine(") {");
    
    increaseIndent();
    node.getThenStatement()->accept(*this);
    decreaseIndent();
    
    if (node.getElseStatement()) {
        emitLine("} else {");
        increaseIndent();
        node.getElseStatement()->accept(*this);
        decreaseIndent();
    }
    
    emitLine("}");
}

void CppGenerator::visit(WhileStatement& node) {
    emitIndent();
    emit("while (");
    node.getCondition()->accept(*this);
    emitLine(") {");
    
    increaseIndent();
    node.getBody()->accept(*this);
    decreaseIndent();
    
    emitLine("}");
}

void CppGenerator::visit(ForStatement& node) {
    emitIndent();
    if (node.isDownto()) {
        // For downto loops: for (var = start; var >= end; var--)
        // Special handling for enum types
        emit("for (" + node.getVariable() + " = ");
        node.getStart()->accept(*this);
        emit("; " + node.getVariable() + " >= ");
        node.getEnd()->accept(*this);
        emit("; " + node.getVariable() + " = static_cast<decltype(" + node.getVariable() + ")>(static_cast<int>(" + node.getVariable() + ") - 1)");
        emitLine(") {");
    } else {
        // For to loops: for (var = start; var <= end; var++)
        // Special handling for enum types
        emit("for (" + node.getVariable() + " = ");
        node.getStart()->accept(*this);
        emit("; " + node.getVariable() + " <= ");
        node.getEnd()->accept(*this);
        emit("; " + node.getVariable() + " = static_cast<decltype(" + node.getVariable() + ")>(static_cast<int>(" + node.getVariable() + ") + 1)");
        emitLine(") {");
    }
    
    increaseIndent();
    node.getBody()->accept(*this);
    decreaseIndent();
    
    emitLine("}");
}

void CppGenerator::visit(RepeatStatement& node) {
    emitIndent();
    emitLine("do {");
    
    increaseIndent();
    node.getBody()->accept(*this);
    decreaseIndent();
    
    emitIndent();
    emit("} while (!(");
    node.getCondition()->accept(*this);
    emitLine("));");
}

void CppGenerator::visit(CaseStatement& node) {
    emitIndent();
    emit("switch (");
    node.getExpression()->accept(*this);
    emitLine(") {");
    
    increaseIndent();
    
    // Generate case branches
    for (const auto& branch : node.getBranches()) {
        for (const auto& value : branch->getValues()) {
            // Check if this is a range expression
            if (auto binaryExpr = dynamic_cast<BinaryExpression*>(value.get())) {
                if (binaryExpr->getOperator().getType() == TokenType::RANGE) {
                    // Handle range: generate multiple case labels
                    // For simplicity, assuming integer ranges for now
                    
                    // Get start and end values (assuming they are integer literals)
                    auto leftLiteral = dynamic_cast<LiteralExpression*>(binaryExpr->getLeft());
                    auto rightLiteral = dynamic_cast<LiteralExpression*>(binaryExpr->getRight());
                    
                    if (leftLiteral && rightLiteral) {
                        int start = std::stoi(leftLiteral->getToken().getValue());
                        int end = std::stoi(rightLiteral->getToken().getValue());
                        
                        // Generate case labels for each value in range
                        for (int i = start; i <= end; ++i) {
                            emitIndent();
                            emitLine("case " + std::to_string(i) + ":");
                        }
                    } else {
                        // Fallback: emit comment
                        emitIndent();
                        emit("/* case ");
                        value->accept(*this);
                        emitLine(": */");
                    }
                    continue;
                }
            }
            
            // Regular case value
            emitIndent();
            emit("case ");
            value->accept(*this);
            emitLine(":");
        }
        increaseIndent();
        branch->getStatement()->accept(*this);
        emitIndent();
        emitLine("break;");
        decreaseIndent();
    }
    
    // Generate default case if else clause exists
    if (node.getElseClause()) {
        emitIndent();
        emitLine("default:");
        increaseIndent();
        node.getElseClause()->accept(*this);
        emitIndent();
        emitLine("break;");
        decreaseIndent();
    }
    
    decreaseIndent();
    emitIndent();
    emitLine("}");
}

void CppGenerator::visit(WithStatement& node) {
    // Generate nested scopes with reference aliases for each with expression
    // Example: with point, person.address do x := 10;
    // Becomes: { auto& __with_0 = point; { auto& __with_1 = person.address; x = 10; } }
    
    for (size_t i = 0; i < node.getWithExpressions().size(); ++i) {
        emitIndent();
        emit("{ auto& __with_" + std::to_string(i) + " = ");
        node.getWithExpressions()[i]->accept(*this);
        emitLine(";");
        increaseIndent();
    }
    
    // Generate the body
    node.getBody()->accept(*this);
    
    // Close all scopes
    for (size_t i = 0; i < node.getWithExpressions().size(); ++i) {
        decreaseIndent();
        emitIndent();
        emitLine("}");
    }
}

void CppGenerator::visit(LabelStatement& node) {
    // Generate C++ label
    emitIndent();
    emitLine("label_" + node.getLabel() + ":;");
}

void CppGenerator::visit(GotoStatement& node) {
    // Generate C++ goto statement
    emitIndent();
    emitLine("goto label_" + node.getTarget() + ";");
}

void CppGenerator::visit(BreakStatement& node) {
    (void)node; // Suppress unused parameter warning
    // Generate C++ break statement
    emitIndent();
    emitLine("break;");
}

void CppGenerator::visit(ContinueStatement& node) {
    (void)node; // Suppress unused parameter warning
    // Generate C++ continue statement
    emitIndent();
    emitLine("continue;");
}

void CppGenerator::visit(ConstantDeclaration& node) {
    emitIndent();
    emit("const auto " + node.getName() + " = ");
    node.getValue()->accept(*this);
    emitLine(";");
}

void CppGenerator::visit(LabelDeclaration& node) {
    // Label declarations don't generate any C++ code directly
    // The labels themselves are generated when referenced in LabelStatement
    // This is just for Pascal syntax compliance
    (void)node; // Suppress unused parameter warning
}

void CppGenerator::visit(TypeDefinition& node) {
    const std::string& definition = node.getDefinition();
    
    // Handle enumeration types
    if (definition.length() > 2 && definition[0] == '(' && definition.back() == ')') {
        generateEnumDefinition(node.getName(), definition);
    }
    // Handle record types
    else if (definition.find("record") != std::string::npos) {
        generateRecordDefinition(node.getName(), definition);
    } 
    // Handle array types
    else if (definition.find("array[") != std::string::npos) {
        generateArrayDefinition(node.getName(), definition);
    } 
    // Handle set types
    else if (definition.find("set of") != std::string::npos) {
        generateSetDefinition(node.getName(), definition);
    } 
    // Handle bounded string types
    else if (definition.find("string[") != std::string::npos) {
        generateBoundedStringDefinition(node.getName(), definition);
    }
    // Handle range types  
    else if (definition.find("..") != std::string::npos) {
        generateRangeDefinition(node.getName(), definition);
    }
    // Handle pointer types
    else if (!definition.empty() && definition[0] == '^') {
        generatePointerDefinition(node.getName(), definition);
    }
    // Handle file types
    else if (definition.find("file of") != std::string::npos) {
        generateFileDefinition(node.getName(), definition);
    }
    // Handle text files
    else if (definition == "text") {
        emitLine("using " + node.getName() + " = PascalFile;");
    }
    // Handle untyped files
    else if (definition == "file") {
        emitLine("using " + node.getName() + " = PascalFile;");
    }
    else {
        // For other types, generate a comment for now
        emitLine("// Type definition: " + node.getName() + " = " + definition);
        emitLine("using " + node.getName() + " = int; // TODO: implement proper type");
    }
}

void CppGenerator::visit(RecordTypeDefinition& node) {
    // Generate C++ struct definition from RecordTypeDefinition AST node
    emitLine("struct " + node.getName() + " {");
    increaseIndent();
    
    // Generate field declarations from the AST fields
    for (const auto& field : node.getFields()) {
        emitIndent();
        emitLine(mapPascalTypeToCpp(field.getType()) + " " + field.getName() + ";");
    }
    
    // Generate variant part if present
    if (node.hasVariantPart()) {
        const VariantPart* variantPart = node.getVariantPart();
        
        // Check if the selector field is already defined in regular fields
        bool selectorAlreadyDefined = false;
        for (const auto& field : node.getFields()) {
            if (field.getName() == variantPart->getSelectorName()) {
                selectorAlreadyDefined = true;
                break;
            }
        }
        
        // Generate the selector field only if not already defined
        if (!selectorAlreadyDefined) {
            emitIndent();
            emitLine(mapPascalTypeToCpp(variantPart->getSelectorType()) + " " + variantPart->getSelectorName() + ";");
        }
        
        // For variant records, generate all variant fields as regular fields
        // This simplifies the implementation and matches Pascal's behavior where
        // all variant fields are accessible (with programmer responsibility for correctness)
        for (const auto& variantCase : variantPart->getCases()) {
            for (const auto& field : variantCase->getFields()) {
                emitIndent();
                emitLine(mapPascalTypeToCpp(field.getType()) + " " + field.getName() + ";");
            }
        }
        
        // Add constructor to handle initialization
        emitLine("");
        emitIndent();
        emitLine("// Default constructor");
        emitIndent();
        emit(node.getName() + "() : ");
        
        // Initialize all fields
        bool first = true;
        for (const auto& field : node.getFields()) {
            if (!first) emit(", ");
            emit(field.getName() + "()");
            first = false;
        }
        
        if (variantPart) {
            // Only initialize selector if not already initialized in regular fields
            if (!selectorAlreadyDefined) {
                if (!first) emit(", ");
                emit(variantPart->getSelectorName() + "()");
                first = false;
            }
            
            for (const auto& variantCase : variantPart->getCases()) {
                for (const auto& field : variantCase->getFields()) {
                    if (!first) emit(", ");
                    emit(field.getName() + "()");
                    first = false;
                }
            }
        }
        
        emitLine(" {}");
    }
    
    decreaseIndent();
    emitLine("};");
    emitLine("");
}

void CppGenerator::visit(VariableDeclaration& node) {
    std::string cppType = mapPascalTypeToCpp(node.getType());
    emitIndent();
    emit(cppType + " " + node.getName());
    
    // Register variable in symbol table for proper lookups
    if (symbolTable_) {
        DataType dataType = symbolTable_->resolveDataType(node.getType());
        auto symbol = std::make_shared<Symbol>(node.getName(), SymbolType::VARIABLE, dataType);
        symbol->setTypeName(node.getType()); // Use original Pascal type name, not C++ mapped type
        
        // Handle pointer types - set pointee information
        if (dataType == DataType::POINTER && !node.getType().empty() && node.getType()[0] == '^') {
            std::string pointeeTypeName = node.getType().substr(1); // Remove '^' prefix
            DataType pointeeType = symbolTable_->resolveDataType(pointeeTypeName);
            symbol->setPointeeType(pointeeType);
            symbol->setPointeeTypeName(pointeeTypeName);
        }
        
        symbolTable_->define(node.getName(), symbol);
    }
    
    if (node.getInitializer()) {
        emit(" = ");
        node.getInitializer()->accept(*this);
    }
    
    emitLine(";");
}

void CppGenerator::visit(ProcedureDeclaration& node) {
    // Skip forward declarations - they're handled in generateForwardDeclarations
    if (node.isForward()) {
        return;
    }
    
    // Register this procedure in the symbol table so it can be found for calls
    if (symbolTable_) {
        symbolTable_->define(node.getName(), SymbolType::PROCEDURE, DataType::UNKNOWN);
    }
    
    // Check for nested procedures/functions and emit error message
    if (!node.getNestedDeclarations().empty()) {
        emitLine("// ERROR: Nested procedures/functions are not supported in RPascal");
        emitLine("// Please refactor '" + node.getName() + "' to use global procedures instead");
        emitLine("// Nested declarations found: ");
        for (const auto& nestedDecl : node.getNestedDeclarations()) {
            if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(nestedDecl.get())) {
                emitLine("//   - procedure " + procDecl->getName());
            } else if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(nestedDecl.get())) {
                emitLine("//   - function " + funcDecl->getName());
            }
        }
        emitLine("");
    }
    
    std::string mangledName = generateMangledFunctionName(node.getName(), node.getParameters());
    emitLine("void " + mangledName + "(" + generateParameterList(node.getParameters()) + ") {");
    
    increaseIndent();
    
    // Generate local variable declarations
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    // Enter procedure scope and add parameters for proper type resolution during code generation
    symbolTable_->enterScope();
    
    // Add parameters to current scope for type resolution during function call generation
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    node.getBody()->accept(*this);
    
    // Exit procedure scope
    symbolTable_->exitScope();
    
    decreaseIndent();
    
    emitLine("}");
    emitLine("");
}

void CppGenerator::visit(FunctionDeclaration& node) {
    // Skip forward declarations - they're handled in generateForwardDeclarations
    if (node.isForward()) {
        return;
    }
    
    // Register this function in the symbol table so it can be found for calls
    if (symbolTable_) {
        DataType returnType = symbolTable_->resolveDataType(node.getReturnType());
        symbolTable_->define(node.getName(), SymbolType::FUNCTION, returnType);
    }
    
    // Generate nested procedures and functions BEFORE the parent function
    // This ensures they are declared before being called
    for (const auto& nestedDecl : node.getNestedDeclarations()) {
        nestedDecl->accept(*this);
    }
    
    std::string returnType = mapPascalTypeToCpp(node.getReturnType());
    std::string mangledName = generateMangledFunctionName(node.getName(), node.getParameters());
    
    emitLine(returnType + " " + mangledName + "(" + generateParameterList(node.getParameters()) + ") {");
    
    increaseIndent();
    
    // Declare return variable (Pascal functions assign to function name)
    emitIndent();
    emitLine(returnType + " " + node.getName() + "_result;");
    
    // Generate local variable declarations
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    // Enter function scope and add parameters for proper type resolution during code generation
    symbolTable_->enterScope();
    
    // Add parameters to current scope for type resolution during function call generation
    for (const auto& param : node.getParameters()) {
        DataType paramType = symbolTable_->resolveDataType(param->getType());
        symbolTable_->define(param->getName(), SymbolType::PARAMETER, paramType);
    }
    
    currentFunction_ = mangledName;  // Use mangled name for internal tracking
    currentFunctionOriginalName_ = node.getName();  // Store original name for return assignments
    node.getBody()->accept(*this);
    currentFunction_ = "";
    currentFunctionOriginalName_ = "";
    
    // Exit function scope
    symbolTable_->exitScope();
    
    // Return the result
    emitIndent();
    emitLine("return " + node.getName() + "_result;");
    
    decreaseIndent();
    
    emitLine("}");
    emitLine("");
}

void CppGenerator::visit(Program& node) {
    // Generate headers
    emitLine(generateHeaders());
    emitLine(generateRuntimeIncludes());
    emitLine("");
    
    // Generate uses clause includes
    if (node.getUsesClause()) {
        node.getUsesClause()->accept(*this);
    }
    
    // Generate forward declarations
    std::string forwardDecls = generateForwardDeclarations(node.getDeclarations());
    if (!forwardDecls.empty()) {
        emit(forwardDecls);
        emitLine("");
    }
    
    // Generate global variables, constants, and types
    for (const auto& decl : node.getDeclarations()) {
        if (auto constDecl = dynamic_cast<ConstantDeclaration*>(decl.get())) {
            constDecl->accept(*this);
        } else if (auto typeDecl = dynamic_cast<TypeDefinition*>(decl.get())) {
            typeDecl->accept(*this);
        } else if (auto recordDecl = dynamic_cast<RecordTypeDefinition*>(decl.get())) {
            recordDecl->accept(*this);
        } else if (auto varDecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
            varDecl->accept(*this);
        }
    }
    
    if (!node.getDeclarations().empty()) {
        emitLine("");
    }
    
    // Generate procedures and functions
    for (const auto& decl : node.getDeclarations()) {
        if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(decl.get())) {
            procDecl->accept(*this);
        } else if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            funcDecl->accept(*this);
        }
    }
    
    // Global variables for Pascal command line arguments
    emitLine("// Global variables for Pascal system functions");
    emitLine("static int pascal_argc = 0;");
    emitLine("static char** pascal_argv = nullptr;");
    emitLine("");
    
    // Generate main function
    emitLine("int main(int argc, char* argv[]) {");
    increaseIndent();
    
    // Initialize global command line arguments
    emitIndent();
    emitLine("pascal_argc = argc;");
    emitIndent();
    emitLine("pascal_argv = argv;");
    emitLine("");
    
    node.getMainBlock()->accept(*this);
    
    emitIndent();
    emitLine("return 0;");
    
    decreaseIndent();
    emitLine("}");
}

bool CppGenerator::isStringExpression(Expression* expr) {
    if (!expr) return false;
    
    // Check if it's a string literal
    if (auto literal = dynamic_cast<LiteralExpression*>(expr)) {
        return literal->getToken().getType() == TokenType::STRING_LITERAL;
    }
    
    // Check if it's an identifier with string type
    if (auto ident = dynamic_cast<IdentifierExpression*>(expr)) {
        if (symbolTable_) {
            auto symbol = symbolTable_->lookup(ident->getName());
            if (symbol) {
                return symbol->getDataType() == DataType::STRING;
            }
        }
    }
    
    // Check if it's a function call that returns a string
    if (auto call = dynamic_cast<CallExpression*>(expr)) {
        // Get function name from the callee
        std::string funcName;
        if (auto ident = dynamic_cast<IdentifierExpression*>(call->getCallee())) {
            funcName = ident->getName();
        }
        
        // Check for known string-returning functions
        if (funcName == "concat" || funcName == "copy" || funcName == "chr" || 
            funcName == "upcase" || funcName == "lowercase") {
            return true;
        }
        
        // Look up function in symbol table
        if (!funcName.empty() && symbolTable_) {
            auto symbol = symbolTable_->lookup(funcName);
            if (symbol && symbol->getSymbolType() == SymbolType::FUNCTION) {
                return symbol->getDataType() == DataType::STRING;
            }
        }
    }
    
    // Check if it's array access on a string (string indexing)
    if (auto arrayAccess = dynamic_cast<ArrayIndexExpression*>(expr)) {
        return isStringExpression(arrayAccess->getArray());
    }
    
    return false;
}

bool CppGenerator::needsCharToStringConversion(AssignmentStatement& node) {
    // Check if target is a string variable and value is a char expression
    auto targetId = dynamic_cast<IdentifierExpression*>(node.getTarget());
    if (!targetId || !symbolTable_) return false;
    
    auto targetSymbol = symbolTable_->lookup(targetId->getName());
    if (!targetSymbol || targetSymbol->getDataType() != DataType::STRING) return false;
    
    // Check if value is a char literal
    if (auto literal = dynamic_cast<LiteralExpression*>(node.getValue())) {
        return literal->getToken().getType() == TokenType::CHAR_LITERAL;
    }
    
    // Check if value is a char variable
    if (auto valueId = dynamic_cast<IdentifierExpression*>(node.getValue())) {
        auto valueSymbol = symbolTable_->lookup(valueId->getName());
        return valueSymbol && valueSymbol->getDataType() == DataType::CHAR;
    }
    
    return false;
}

void CppGenerator::emit(const std::string& code) {
    output_ << code;
}

void CppGenerator::emitLine(const std::string& line) {
    output_ << line << "\n";
}

void CppGenerator::emitIndent() {
    for (int i = 0; i < indentLevel_; ++i) {
        output_ << "    ";
    }
}

void CppGenerator::increaseIndent() {
    indentLevel_++;
}

void CppGenerator::decreaseIndent() {
    if (indentLevel_ > 0) {
        indentLevel_--;
    }
}

std::string CppGenerator::generateHeaders() {
    return "// Generated by RPascal Compiler\n"
           "#include <iostream>\n"
           "#include <fstream>\n"
           "#include <string>\n"
           "#include <array>\n"
           "#include <set>\n"
           "#include <algorithm>\n"
           "#include <cstdint>\n"
           "#include <cmath>\n"
           "#include <cstdlib>\n"
           "#include <ctime>\n"
           "#include <cctype>\n"
           "#include <memory>\n"
           "#include <type_traits>\n"
           "#include <thread>\n"
           "#include <chrono>\n"
           "#include <filesystem>\n";
           // Note: Platform-specific includes (windows.h, conio.h) moved to 
           // unit-specific includes to avoid unnecessary Windows API conflicts
}

std::string CppGenerator::generateRuntimeIncludes() {
    return "// Using explicit std:: prefixes to avoid name conflicts\n\n"
           "// Global I/O error tracking\n"
           "static int g_last_io_error = 0;\n\n"
           "// Pascal string functions\n"
           "void Delete(std::string& s, int index, int count) {\n"
           "    if (index <= 0 || index > static_cast<int>(s.length())) return;\n"
           "    int startPos = index - 1;  // Convert to 0-based index\n"
           "    s.erase(startPos, count);\n"
           "}\n\n"
           "void Insert(const std::string& substr, std::string& s, int index) {\n"
           "    if (index <= 0) index = 1;\n"
           "    if (index > static_cast<int>(s.length()) + 1) index = s.length() + 1;\n"
           "    int insertPos = index - 1;  // Convert to 0-based index\n"
           "    s.insert(insertPos, substr);\n"
           "}\n\n"
           "// Pascal file wrapper class\n"
           "class PascalFile {\n"
           "private:\n"
           "    std::fstream stream_;\n"
           "    std::string filename_;\n"
           "    \n"
           "public:\n"
           "    PascalFile() = default;\n"
           "    ~PascalFile() { close(); }\n"
           "    \n"
           "    void assign(const std::string& filename) {\n"
           "        filename_ = filename;\n"
           "    }\n"
           "    \n"
           "    void reset() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::in);\n"
           "        g_last_io_error = stream_.good() ? 0 : 2; // 2 = file not found\n"
           "    }\n"
           "    \n"
           "    void rewrite() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::out);\n"
           "        g_last_io_error = stream_.good() ? 0 : 3; // 3 = path not found\n"
           "    }\n"
           "    \n"
           "    void append() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::out | std::ios::app);\n"
           "        g_last_io_error = stream_.good() ? 0 : 3; // 3 = path not found\n"
           "    }\n"
           "    \n"
           "    void close() {\n"
           "        if (stream_.is_open()) {\n"
           "            stream_.close();\n"
           "        }\n"
           "    }\n"
           "    \n"
           "    bool eof() const {\n"
           "        return stream_.eof();\n"
           "    }\n"
           "    \n"
           "    std::fstream& getStream() { return stream_; }\n"
           "    const std::string& getFilename() const { return filename_; }\n"
           "};\n\n"
           "// Pascal typed file wrapper class\n"
           "template<typename T>\n"
           "class PascalTypedFile {\n"
           "private:\n"
           "    std::fstream stream_;\n"
           "    std::string filename_;\n"
           "    \n"
           "public:\n"
           "    PascalTypedFile() = default;\n"
           "    ~PascalTypedFile() { close(); }\n"
           "    \n"
           "    void assign(const std::string& filename) {\n"
           "        filename_ = filename;\n"
           "    }\n"
           "    \n"
           "    void reset() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::in | std::ios::binary);\n"
           "    }\n"
           "    \n"
           "    void rewrite() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::out | std::ios::binary);\n"
           "    }\n"
           "    \n"
           "    void close() {\n"
           "        if (stream_.is_open()) {\n"
           "            stream_.close();\n"
           "        }\n"
           "    }\n"
           "    \n"
           "    bool eof() const {\n"
           "        return stream_.eof();\n"
           "    }\n"
           "    \n"
           "    void write(const T& data) {\n"
           "        stream_.write(reinterpret_cast<const char*>(&data), sizeof(T));\n"
           "    }\n"
           "    \n"
           "    void read(T& data) {\n"
           "        stream_.read(reinterpret_cast<char*>(&data), sizeof(T));\n"
           "    }\n"
           "    \n"
           "    std::fstream& getStream() { return stream_; }\n"
           "    const std::string& getFilename() const { return filename_; }\n"
           "};\n\n"
           "// I/O error checking function\n"
           "int pascal_ioresult() {\n"
           "    int result = g_last_io_error;\n"
           "    g_last_io_error = 0; // Clear error after reading (Pascal behavior)\n"
           "    return result;\n"
           "}";
}

std::string CppGenerator::generateForwardDeclarations(const std::vector<std::unique_ptr<Declaration>>& declarations) {
    std::ostringstream forward;
    
    for (const auto& decl : declarations) {
        if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(decl.get())) {
            if (procDecl->isForward()) {
                std::string mangledName = generateMangledFunctionName(procDecl->getName(), procDecl->getParameters());
                forward << "void " << mangledName << "(" << generateParameterList(procDecl->getParameters()) << ");\n";
            }
        } else if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            if (funcDecl->isForward()) {
                std::string returnType = mapPascalTypeToCpp(funcDecl->getReturnType());
                std::string mangledName = generateMangledFunctionName(funcDecl->getName(), funcDecl->getParameters());
                forward << returnType << " " << mangledName << "(" << generateParameterList(funcDecl->getParameters()) << ");\n";
            }
        }
    }
    
    return forward.str();
}

std::string CppGenerator::mapPascalOperatorToCpp(TokenType operator_) {
    switch (operator_) {
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::MULTIPLY: return "*";
        case TokenType::DIVIDE: return "/";
        case TokenType::DIV: return "/"; // Integer division
        case TokenType::MOD: return "%";
        case TokenType::EQUAL: return "==";
        case TokenType::NOT_EQUAL: return "!=";
        case TokenType::LESS_THAN: return "<";
        case TokenType::LESS_EQUAL: return "<=";
        case TokenType::GREATER_THAN: return ">";
        case TokenType::GREATER_EQUAL: return ">=";
        case TokenType::AND: return "&&";
        case TokenType::OR: return "||";
        case TokenType::NOT: return "!";
        case TokenType::XOR: return "^";
        default: return "/* UNKNOWN_OP */";
    }
}

std::string CppGenerator::mapPascalTypeToCpp(const std::string& pascalType) {
    std::string lowerType = pascalType;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Handle pointer types: ^Type -> Type*
    if (!lowerType.empty() && lowerType[0] == '^') {
        std::string pointeeType = pascalType.substr(1); // Use original case, not lowercase
        return mapPascalTypeToCpp(pointeeType) + "*";
    }
    
    // Handle open array types: array of Type -> std::vector<Type>&
    if (lowerType == "array of integer") {
        return "std::vector<int32_t>";
    }
    if (lowerType == "array of real") {
        return "std::vector<double>";
    }
    if (lowerType == "array of char") {
        return "std::vector<char>";
    }
    if (lowerType == "array of boolean") {
        return "std::vector<bool>";
    }
    if (lowerType == "array of string") {
        return "std::vector<std::string>";
    }
    
    // Handle generic open array types
    if (lowerType.find("array of ") == 0) {
        std::string elementType = pascalType.substr(9); // Skip "array of "
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r")); // Trim leading whitespace
        std::string cppElementType = mapPascalTypeToCpp(elementType);
        return "std::vector<" + cppElementType + ">";
    }
    
    // Handle array types: array[0..9] of Type -> std::array<Type, 10> (must be before subrange check)
    if (lowerType.find("array") == 0 && lowerType.find(" of ") != std::string::npos) {
        // Parse array[start..end] of ElementType
        size_t bracketStart = lowerType.find('[');
        size_t bracketEnd = lowerType.find(']');
        size_t ofPos = lowerType.find(" of ");
        
        // Also find positions in original string to preserve case
        size_t origOfPos = pascalType.find(" of ");
        
        if (bracketStart != std::string::npos && bracketEnd != std::string::npos && ofPos != std::string::npos && origOfPos != std::string::npos) {
            std::string bounds = lowerType.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
            // Extract element type from original string to preserve case
            std::string elementType = pascalType.substr(origOfPos + 4);
            elementType.erase(0, elementType.find_first_not_of(" \t\n\r")); // Trim leading whitespace
            
            // Parse the bounds: start..end
            size_t rangePos = bounds.find("..");
            if (rangePos != std::string::npos) {
                std::string startStr = bounds.substr(0, rangePos);
                std::string endStr = bounds.substr(rangePos + 2);
                
                // Convert to integers
                int start = std::stoi(startStr);
                int end = std::stoi(endStr);
                int size = end - start + 1;
                
                std::string cppElementType = mapPascalTypeToCpp(elementType);
                return "std::array<" + cppElementType + ", " + std::to_string(size) + ">";
            }
        }
    }
    
    // Handle subrange types: 0..9 -> int, 'A'..'Z' -> char
    if (lowerType.find("..") != std::string::npos) {
        // Check if it's a character range 'A'..'Z'
        if (lowerType.find("'") != std::string::npos) {
            return "char";
        } else {
            // Numeric range like 0..9
            return "int";
        }
    }
    
    if (lowerType == "integer") return "int32_t";
    if (lowerType == "real") return "double";
    if (lowerType == "boolean") return "bool";
    if (lowerType == "char") return "char";
    if (lowerType == "byte") return "uint8_t";
    if (lowerType == "string") return "std::string";
    if (lowerType == "text") return "PascalFile";
    if (lowerType == "file") return "PascalFile"; // Untyped file
    
    // Handle bounded string types: string[30] -> std::string
    if (lowerType.find("string[") == 0 && lowerType.find(']') != std::string::npos) {
        return "std::string"; // For now, map bounded strings to std::string
    }
    
    // Handle typed files: file of T
    if (lowerType.find("file of") == 0) {
        std::string elementType = lowerType.substr(8); // Skip "file of "
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r")); // Trim leading whitespace
        return "PascalTypedFile<" + mapPascalTypeToCpp(elementType) + ">";
    }
    
    return pascalType; // fallback
}

std::string CppGenerator::mapPascalFunctionToCpp(const std::string& functionName) {
    if (functionName == "writeln") return "std::cout";
    if (functionName == "readln") return "std::cin";
    if (functionName == "length") return "length";
    if (functionName == "chr") return "static_cast<char>";
    if (functionName == "ord") return "static_cast<int>";
    
    return functionName;
}

void CppGenerator::generateExpression(Expression* expr) {
    expr->accept(*this);
}

void CppGenerator::generateStatement(Statement* stmt) {
    stmt->accept(*this);
}

void CppGenerator::generateFunctionCall(CallExpression& node) {
    auto calleeExpr = dynamic_cast<IdentifierExpression*>(node.getCallee());
    if (!calleeExpr) {
        emit("/* Invalid function call */");
        return;
    }
    
    std::string functionName = calleeExpr->getName();
    
    if (isBuiltinFunction(functionName)) {
        generateBuiltinCall(node, functionName);
    } else {
        // Check if this is a recursive call to the current function
        if (!currentFunction_.empty() && functionName == currentFunctionOriginalName_) {
            // Use the mangled name for recursive calls
            emit(currentFunction_ + "(");
            for (size_t i = 0; i < node.getArguments().size(); ++i) {
                if (i > 0) emit(", ");
                node.getArguments()[i]->accept(*this);
            }
            emit(")");
            return;
        }
        // For overloaded functions, we need to resolve which one to call
        // Build argument types for proper overload resolution
        std::vector<DataType> argTypes;
        for (const auto& arg : node.getArguments()) {
            DataType argType = DataType::UNKNOWN;
            
            // Try to determine argument type
            if (auto literal = dynamic_cast<LiteralExpression*>(arg.get())) {
                // Handle literals
                if (literal->getToken().getType() == TokenType::INTEGER_LITERAL) {
                    argType = DataType::INTEGER;
                } else if (literal->getToken().getType() == TokenType::REAL_LITERAL) {
                    argType = DataType::REAL;
                } else if (literal->getToken().getType() == TokenType::STRING_LITERAL) {
                    argType = DataType::STRING;
                } else if (literal->getToken().getType() == TokenType::CHAR_LITERAL) {
                    argType = DataType::CHAR;
                }
            } else if (auto identifier = dynamic_cast<IdentifierExpression*>(arg.get())) {
                // Look up variable in symbol table
                auto symbol = symbolTable_->lookup(identifier->getName());
                if (symbol) {
                    argType = symbol->getDataType();
                    // For CUSTOM types, check if it's a known enum type
                    if (argType == DataType::CUSTOM && !symbol->getTypeName().empty()) {
                        // Keep CUSTOM but the type name will be used in mangling
                        argType = DataType::CUSTOM;
                    }
                } else {
                    // Check if it's a known enum constant
                    if (isBuiltinConstant(identifier->getName())) {
                        argType = DataType::INTEGER; // Enum constants are treated as integers
                    }
                }
            }
            // TODO: Handle other expression types (array access, field access, etc.)
            
            argTypes.push_back(argType);
        }
        
        // Try to find the matching function overload
        auto functionSymbol = symbolTable_->lookupFunction(functionName, argTypes);
        if (functionSymbol) {
            // Build proper mangled name using argument type information
            std::vector<std::unique_ptr<VariableDeclaration>> dummyParams;
            
            // Create dummy parameters based on the argument types and symbol information
            for (size_t i = 0; i < argTypes.size(); ++i) {
                std::string paramType;
                
                // Try to get the actual type name from the argument if possible
                if (i < node.getArguments().size()) {
                    if (auto identifier = dynamic_cast<IdentifierExpression*>(node.getArguments()[i].get())) {
                        auto symbol = symbolTable_->lookup(identifier->getName());
                        if (symbol && !symbol->getTypeName().empty()) {
                            paramType = symbol->getTypeName();
                        }
                    }
                }
                
                // Fallback to basic type mapping if we don't have the type name
                if (paramType.empty()) {
                    switch (argTypes[i]) {
                        case DataType::INTEGER: paramType = "integer"; break;
                        case DataType::REAL: paramType = "real"; break;
                        case DataType::BOOLEAN: paramType = "boolean"; break;
                        case DataType::CHAR: paramType = "char"; break;
                        case DataType::STRING: paramType = "string"; break;
                        case DataType::CUSTOM: paramType = "custom"; break;
                        default: paramType = "unknown"; break;
                    }
                }
                
                auto param = std::make_unique<VariableDeclaration>("dummy" + std::to_string(i), paramType, nullptr);
                dummyParams.push_back(std::move(param));
            }
            
            // Generate the proper mangled name using the same algorithm as function definitions
            std::string mangledName = generateMangledFunctionName(functionName, dummyParams);
            emit(mangledName + "(");
        } else {
            // Fallback to original name if no overload found
            emit(functionName + "(");
        }
        
        // Generate arguments
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
    }
}

void CppGenerator::generateBuiltinCall(CallExpression& node, const std::string& functionName) {
    // Convert to lowercase for comparison
    std::string lowerName = functionName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Delegate to category-specific helper methods
    if (generateBasicIOCall(node, lowerName)) return;
    if (generateMathFunctionCall(node, lowerName)) return;
    if (generateStringFunctionCall(node, lowerName)) return;
    if (generateConversionFunctionCall(node, lowerName)) return;
    if (generateCharacterFunctionCall(node, lowerName)) return;
    if (generateDateTimeFunctionCall(node, lowerName)) return;
    if (generateSystemFunctionCall(node, lowerName)) return;
    if (generateMemoryFunctionCall(node, lowerName)) return;
    if (generateFileFunctionCall(node, lowerName)) return;
    
    // Default function call for unrecognized functions
    emit(functionName + "(");
    for (size_t i = 0; i < node.getArguments().size(); ++i) {
        if (i > 0) emit(", ");
        node.getArguments()[i]->accept(*this);
    }
    emit(")");
}

// Helper method implementations

bool CppGenerator::generateBasicIOCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "writeln") {
        if (node.getArguments().empty()) {
            // writeln() with no arguments just prints a newline
            emit("std::cout << std::endl");
        } else {
            // Check if the first argument is a file variable
            bool isFileOutput = false;
            std::string outputTarget = "std::cout";
            
            if (auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get())) {
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(firstArg->getName());
                    if (symbol && (symbol->getDataType() == DataType::FILE_TYPE || 
                                  (symbol->getDataType() == DataType::CUSTOM && 
                                   symbol->getTypeName().find("File") != std::string::npos))) {
                        isFileOutput = true;
                        // Check if this is a typed file (PascalTypedFile)
                        if (symbol->getDataType() == DataType::CUSTOM && 
                            symbol->getTypeName().find("PascalTypedFile") != std::string::npos) {
                            // For typed files, use direct write method
                            emit(firstArg->getName() + ".write(");
                            if (node.getArguments().size() > 1) {
                                node.getArguments()[1]->accept(*this);
                            }
                            emit(")");
                            return true;
                        } else {
                            outputTarget = firstArg->getName() + ".getStream()";
                        }
                    }
                }
            }
            
            emit(outputTarget);
            size_t startIdx = isFileOutput ? 1 : 0; // Skip file argument if present
            for (size_t i = startIdx; i < node.getArguments().size(); ++i) {
                const auto& arg = node.getArguments()[i];
                emit(" << ");
                
                // Check if this is a Byte type that needs casting for proper display
                bool needsCast = false;
                if (auto identifier = dynamic_cast<IdentifierExpression*>(arg.get())) {
                    if (symbolTable_) {
                        auto symbol = symbolTable_->lookup(identifier->getName());
                        if (symbol && symbol->getDataType() == DataType::BYTE) {
                            needsCast = true;
                        }
                    }
                } else if (auto arrayAccess = dynamic_cast<ArrayIndexExpression*>(arg.get())) {
                    // Check if array element type is Byte
                    if (auto arrayIdent = dynamic_cast<IdentifierExpression*>(arrayAccess->getArray())) {
                        if (symbolTable_) {
                            auto symbol = symbolTable_->lookup(arrayIdent->getName());
                            if (symbol) {
                                // Check for named types in arrayTypes_ (for custom types)
                                if (symbol->getDataType() == DataType::CUSTOM) {
                                    std::string typeName = symbol->getTypeName();
                                    auto arrayTypeIt = arrayTypes_.find(typeName);
                                    if (arrayTypeIt != arrayTypes_.end()) {
                                        const ArrayTypeInfo& info = arrayTypeIt->second;
                                        std::string lowerElementType = info.elementType;
                                        std::transform(lowerElementType.begin(), lowerElementType.end(), lowerElementType.begin(), 
                                                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                                        if (lowerElementType == "byte") {
                                            needsCast = true;
                                        }
                                    } else {
                                        // For inline arrays, check the type name directly for "of byte"
                                        std::string lowerTypeName = typeName;
                                        std::transform(lowerTypeName.begin(), lowerTypeName.end(), lowerTypeName.begin(), 
                                                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                                        if (lowerTypeName.find("of byte") != std::string::npos) {
                                            needsCast = true;
                                        }
                                    }
                                } else {
                                    // Check any symbol with uint8_t in type name (handles inline arrays with different DataType)
                                    std::string typeName = symbol->getTypeName();
                                    std::string lowerTypeName = typeName;
                                    std::transform(lowerTypeName.begin(), lowerTypeName.end(), lowerTypeName.begin(), 
                                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                                    if (lowerTypeName.find("uint8_t") != std::string::npos) {
                                        needsCast = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (needsCast) {
                    emit("static_cast<int>(");
                    arg->accept(*this);
                    emit(")");
                } else {
                    arg->accept(*this);
                }
            }
            // Add newline for writeln
            emit(" << std::endl");
        }
        return true;
    } else if (lowerName == "write") {
        if (node.getArguments().empty()) {
            // write() with no arguments does nothing (but is valid)
            emit("// write() with no arguments");
        } else {
            // Check if the first argument is a file variable
            bool isFileOutput = false;
            std::string outputTarget = "std::cout";
            
            if (auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get())) {
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(firstArg->getName());
                    if (symbol && (symbol->getDataType() == DataType::FILE_TYPE || 
                                  (symbol->getDataType() == DataType::CUSTOM && 
                                   symbol->getTypeName().find("File") != std::string::npos))) {
                        isFileOutput = true;
                        // Check if this is a typed file (PascalTypedFile)
                        if (symbol->getDataType() == DataType::CUSTOM && 
                            symbol->getTypeName().find("PascalTypedFile") != std::string::npos) {
                            // For typed files, use direct write method
                            emit(firstArg->getName() + ".write(");
                            if (node.getArguments().size() > 1) {
                                node.getArguments()[1]->accept(*this);
                            }
                            emit(")");
                            return true;
                        } else {
                            outputTarget = firstArg->getName() + ".getStream()";
                        }
                    }
                }
            }
            
            emit(outputTarget);
            size_t startIdx = isFileOutput ? 1 : 0; // Skip file argument if present
            for (size_t i = startIdx; i < node.getArguments().size(); ++i) {
                const auto& arg = node.getArguments()[i];
                emit(" << ");
                
                // Check if this is a Byte type that needs casting for proper display
                bool needsCast = false;
                if (auto identifier = dynamic_cast<IdentifierExpression*>(arg.get())) {
                    if (symbolTable_) {
                        auto symbol = symbolTable_->lookup(identifier->getName());
                        if (symbol && symbol->getDataType() == DataType::BYTE) {
                            needsCast = true;
                        }
                    }
                } else if (auto arrayAccess = dynamic_cast<ArrayIndexExpression*>(arg.get())) {
                // Check if array element type is Byte  
                if (auto arrayIdent = dynamic_cast<IdentifierExpression*>(arrayAccess->getArray())) {
                    if (symbolTable_) {
                        auto symbol = symbolTable_->lookup(arrayIdent->getName());
                        if (symbol) {
                            if (symbol->getDataType() == DataType::CUSTOM) {
                                std::string typeName = symbol->getTypeName();
                                auto arrayTypeIt = arrayTypes_.find(typeName);
                                if (arrayTypeIt != arrayTypes_.end()) {
                                    const ArrayTypeInfo& info = arrayTypeIt->second;
                                    std::string lowerElementType = info.elementType;
                                    std::transform(lowerElementType.begin(), lowerElementType.end(), lowerElementType.begin(), 
                                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                                    if (lowerElementType == "byte") {
                                        needsCast = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            if (needsCast) {
                emit("static_cast<int>(");
                arg->accept(*this);
                emit(")");
            } else {
                arg->accept(*this);
            }
        }
        }
        return true;
    } else if (lowerName == "readln") {
        // Check if the first argument is a file variable
        bool isFileInput = false;
        std::string inputSource = "std::cin";
        
        if (!node.getArguments().empty()) {
            if (auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get())) {
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(firstArg->getName());
                    if (symbol && (symbol->getDataType() == DataType::FILE_TYPE || 
                                  (symbol->getDataType() == DataType::CUSTOM && 
                                   symbol->getTypeName().find("File") != std::string::npos))) {
                        isFileInput = true;
                        inputSource = firstArg->getName() + ".getStream()";
                    }
                }
            }
        }
        
        emit(inputSource);
        size_t startIdx = isFileInput ? 1 : 0; // Skip file argument if present
        for (size_t i = startIdx; i < node.getArguments().size(); ++i) {
            const auto& arg = node.getArguments()[i];
            emit(" >> ");
            
            // Check if this is a Byte type that needs casting for proper input
            bool needsCast = false;
            if (auto identifier = dynamic_cast<IdentifierExpression*>(arg.get())) {
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(identifier->getName());
                    if (symbol && symbol->getDataType() == DataType::BYTE) {
                        needsCast = true;
                    }
                }
            }
            
            if (needsCast) {
                emit("reinterpret_cast<int&>(");
                arg->accept(*this);
                emit(")");
            } else {
                arg->accept(*this);
            }
        }
        return true;
    } else if (lowerName == "read") {
        emit("std::cin");
        for (const auto& arg : node.getArguments()) {
            emit(" >> ");
            
            // Check if this is a Byte type that needs casting for proper input
            bool needsCast = false;
            if (auto identifier = dynamic_cast<IdentifierExpression*>(arg.get())) {
                if (symbolTable_) {
                    auto symbol = symbolTable_->lookup(identifier->getName());
                    if (symbol && symbol->getDataType() == DataType::BYTE) {
                        needsCast = true;
                    }
                }
            }
            
            if (needsCast) {
                emit("reinterpret_cast<int&>(");
                arg->accept(*this);
                emit(")");
            } else {
                arg->accept(*this);
            }
        }
        return true;
    }
    return false;
}

bool CppGenerator::generateMathFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "abs") {
        emit("std::abs(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "sqr") {
        emit("(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(" * ");
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "sqrt") {
        emit("std::sqrt(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "sin") {
        emit("std::sin(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "cos") {
        emit("std::cos(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "tan") {
        emit("std::tan(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "arctan") {
        emit("std::atan(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "ln") {
        emit("std::log(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "exp") {
        emit("std::exp(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "power") {
        emit("std::pow(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this);
            emit(", ");
            node.getArguments()[1]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "round") {
        emit("static_cast<int>(std::round(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("))");
        return true;
    } else if (lowerName == "trunc") {
        emit("static_cast<int>(std::trunc(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("))");
        return true;
    }
    return false;
}

bool CppGenerator::generateStringFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "length") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".length()");
        }
        return true;
    } else if (lowerName == "pos") {
        emit("(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[1]->accept(*this);
            emit(".find(");
            node.getArguments()[0]->accept(*this);
            emit(") != std::string::npos ? ");
            node.getArguments()[1]->accept(*this);
            emit(".find(");
            node.getArguments()[0]->accept(*this);
            emit(") + 1 : 0)");
        }
        return true;
    } else if (lowerName == "copy") {
        emit("(");
        if (node.getArguments().size() >= 3) {
            node.getArguments()[0]->accept(*this);
            emit(".substr(");
            node.getArguments()[1]->accept(*this);
            emit(" - 1, ");
            node.getArguments()[2]->accept(*this);
            emit(")");
        } else if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this);
            emit(".substr(");
            node.getArguments()[1]->accept(*this);
            emit(" - 1)");
        }
        emit(")");
        return true;
    } else if (lowerName == "concat") {
        emit("(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(" + ");
            if (i == 0) emit("std::string(");
            node.getArguments()[i]->accept(*this);
            if (i == 0) emit(")");
        }
        emit(")");
        return true;
    } else if (lowerName == "trim") {
        emit("[](std::string s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); })); s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end()); return s; }(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "lowercase") {
        emit("[](std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); }); return s; }(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "uppercase") {
        emit("[](std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); }); return s; }(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "trimleft") {
        emit("[](std::string s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); })); return s; }(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "trimright") {
        emit("[](std::string s) { s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end()); return s; }(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "stringofchar") {
        emit("std::string(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[1]->accept(*this); // count
            emit(", ");
            node.getArguments()[0]->accept(*this); // char
        }
        emit(")");
        return true;
    } else if (lowerName == "leftstr") {
        emit("(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // string
            emit(".substr(0, ");
            node.getArguments()[1]->accept(*this); // count
            emit(")");
        }
        emit(")");
        return true;
    } else if (lowerName == "rightstr") {
        emit("[](const std::string& s, int count) { return count >= static_cast<int>(s.length()) ? s : s.substr(s.length() - count); }(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // string
            emit(", ");
            node.getArguments()[1]->accept(*this); // count
        }
        emit(")");
        return true;
    } else if (lowerName == "padleft") {
        emit("[](const std::string& s, int width, char pad = ' ') { return width <= static_cast<int>(s.length()) ? s : std::string(width - s.length(), pad) + s; }(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // string
            emit(", ");
            node.getArguments()[1]->accept(*this); // width
            if (node.getArguments().size() >= 3) {
                emit(", ");
                node.getArguments()[2]->accept(*this); // padding char
            }
        }
        emit(")");
        return true;
    } else if (lowerName == "padright") {
        emit("[](const std::string& s, int width, char pad = ' ') { return width <= static_cast<int>(s.length()) ? s : s + std::string(width - s.length(), pad); }(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // string
            emit(", ");
            node.getArguments()[1]->accept(*this); // width
            if (node.getArguments().size() >= 3) {
                emit(", ");
                node.getArguments()[2]->accept(*this); // padding char
            }
        }
        emit(")");
        return true;
    } else if (lowerName == "delete") {
        // Delete(s, index, count) - modifies string in place
        emit("Delete(");
        if (node.getArguments().size() >= 3) {
            node.getArguments()[0]->accept(*this); // string (by reference)
            emit(", ");
            node.getArguments()[1]->accept(*this); // index
            emit(", ");
            node.getArguments()[2]->accept(*this); // count
        }
        emit(")");
        return true;
    } else if (lowerName == "insert") {
        // Insert(substring, s, index) - modifies string in place
        emit("Insert(");
        if (node.getArguments().size() >= 3) {
            node.getArguments()[0]->accept(*this); // substring
            emit(", ");
            node.getArguments()[1]->accept(*this); // string (by reference)
            emit(", ");
            node.getArguments()[2]->accept(*this); // index
        }
        emit(")");
        return true;
    }
    return false;
}

bool CppGenerator::generateConversionFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "inttostr") {
        emit("std::to_string(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "floattostr") {
        emit("std::to_string(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "strtoint") {
        emit("std::stoi(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "strtofloat") {
        emit("std::stod(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "str") {
        if (node.getArguments().size() >= 2) {
            node.getArguments()[1]->accept(*this);
            emit(" = std::to_string(");
            node.getArguments()[0]->accept(*this);
            emit(")");
        }
        return true;
    }
    return false;
}

bool CppGenerator::generateCharacterFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "chr") {
        emit("static_cast<char>(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "ord") {
        emit("static_cast<int>(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "upcase") {
        emit("static_cast<char>(std::toupper(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("))");
        return true;
    }
    return false;
}

bool CppGenerator::generateDateTimeFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "dayofweek") {
        emit("pascal_dayofweek(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "datetostr") {
        emit("pascal_datetostr(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
        return true;
    } else if (lowerName == "timetostr") {
        emit("pascal_timetostr(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
        return true;
    }
    return false;
}

bool CppGenerator::generateSystemFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "halt") {
        emit("std::exit(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        } else {
            emit("0");
        }
        emit(")");
        return true;
    } else if (lowerName == "exit") {
        emit("return");
        return true;
    } else if (lowerName == "random") {
        emit("(static_cast<double>(std::rand()) / RAND_MAX)");
        return true;
    } else if (lowerName == "randomize") {
        emit("std::srand(static_cast<unsigned int>(std::time(nullptr)))");
        return true;
    } else if (lowerName == "ioresult") {
        emit("pascal_ioresult()");
        return true;
    } else if (lowerName == "paramcount") {
        emit("(pascal_argc - 1)");
        return true;
    } else if (lowerName == "paramstr") {
        emit("std::string(pascal_argv[");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("])");
        return true;
    } else if (lowerName == "inc") {
        // Handle inc(var) and inc(var, amount)
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            if (node.getArguments().size() > 1) {
                emit(" += ");
                node.getArguments()[1]->accept(*this);
            } else {
                emit("++");
            }
        }
        return true;
    } else if (lowerName == "dec") {
        // Handle dec(var) and dec(var, amount)
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            if (node.getArguments().size() > 1) {
                emit(" -= ");
                node.getArguments()[1]->accept(*this);
            } else {
                emit("--");
            }
        }
        return true;
    }
    return false;
}

bool CppGenerator::generateMemoryFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "new") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(" = std::make_unique<std::remove_pointer_t<decltype(");
            node.getArguments()[0]->accept(*this);
            emit(")>>().release()");
        }
        return true;
    } else if (lowerName == "dispose") {
        if (!node.getArguments().empty()) {
            emit("delete ");
            node.getArguments()[0]->accept(*this);
            emit("; ");
            node.getArguments()[0]->accept(*this);
            emit(" = nullptr");
        }
        return true;
    } else if (lowerName == "getmem") {
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this);
            emit(" = std::make_unique<uint8_t[]>(");
            node.getArguments()[1]->accept(*this);
            emit(").release()");
        }
        return true;
    } else if (lowerName == "freemem") {
        if (!node.getArguments().empty()) {
            emit("delete[] ");
            node.getArguments()[0]->accept(*this);
            emit("; ");
            node.getArguments()[0]->accept(*this);
            emit(" = nullptr");
        }
        return true;
    }
    return false;
}

bool CppGenerator::generateFileFunctionCall(CallExpression& node, const std::string& lowerName) {
    if (lowerName == "assign") {
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this);
            emit(".assign(");
            node.getArguments()[1]->accept(*this);
            emit(")");
        }
        return true;
    } else if (lowerName == "reset") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".reset()");
        }
        return true;
    } else if (lowerName == "rewrite") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".rewrite()");
        }
        return true;
    } else if (lowerName == "append") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".append()");
        }
        return true;
    } else if (lowerName == "close") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".close()");
        }
        return true;
    } else if (lowerName == "eof") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".eof()");
        }
        return true;
    }
    return false;
}

std::string CppGenerator::generateVariableDeclaration(const std::string& name, const std::string& type, Expression* initializer) {
    std::ostringstream decl;
    decl << mapPascalTypeToCpp(type) << " " << name;
    
    if (initializer) {
        decl << " = ";
        // Note: This is a simplified approach. In a complete implementation,
        // we'd need to generate the initializer expression properly.
    }
    
    decl << ";";
    return decl.str();
}

std::string CppGenerator::generateParameterList(const std::vector<std::unique_ptr<VariableDeclaration>>& parameters) {
    std::ostringstream params;
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) params << ", ";
        
        std::string cppType = mapPascalTypeToCpp(parameters[i]->getType());
        
        // Handle parameter modes
        switch (parameters[i]->getParameterMode()) {
            case ParameterMode::VAR:
                // var parameters are passed by reference
                params << cppType << "& " << parameters[i]->getName();
                break;
            case ParameterMode::CONST:
                // const parameters are passed by const reference for efficiency
                params << "const " << cppType << "& " << parameters[i]->getName();
                break;
            case ParameterMode::VALUE:
            default:
                // value parameters are passed by value
                params << cppType << " " << parameters[i]->getName();
                break;
        }
    }
    
    return params.str();
}

std::string CppGenerator::generateMangledFunctionName(const std::string& functionName, const std::vector<std::unique_ptr<VariableDeclaration>>& parameters) {
    // Generate a unique C++ function name based on Pascal function name and parameter types
    std::ostringstream mangledName;
    mangledName << functionName;
    
    // Only add mangling if there are parameters (to avoid mangling simple functions)
    if (!parameters.empty()) {
        mangledName << "_";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) mangledName << "_";
            
            std::string paramType = parameters[i]->getType();
            // Convert Pascal type names to safe C++ identifier parts
            std::string lowerType = paramType;
            std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), 
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            if (lowerType == "integer") mangledName << "int";
            else if (lowerType == "real") mangledName << "real";
            else if (lowerType == "boolean") mangledName << "bool";
            else if (lowerType == "char") mangledName << "char";
            else if (lowerType == "string") mangledName << "str";
            else if (lowerType == "array of integer") mangledName << "arrayofint";
            else if (lowerType == "array of real") mangledName << "arrayofreal";
            else if (lowerType == "array of char") mangledName << "arrayofchar";
            else if (lowerType == "array of boolean") mangledName << "arrayofbool";
            else if (lowerType == "array of string") mangledName << "arrayofstr";
            else if (lowerType.find("array of ") == 0) {
                // Generic array of Type -> arrayofType
                std::string elementType = paramType.substr(9);
                elementType.erase(0, elementType.find_first_not_of(" \t\n\r"));
                mangledName << "arrayof";
                // Recursively mangle the element type
                std::vector<std::unique_ptr<VariableDeclaration>> dummyParams;
                auto dummyParam = std::make_unique<VariableDeclaration>("dummy", elementType, nullptr);
                dummyParams.push_back(std::move(dummyParam));
                std::string elementMangle = generateMangledFunctionName("", dummyParams);
                // Extract just the type part (remove the trailing "_")
                if (elementMangle.size() > 1 && elementMangle[0] == '_') {
                    mangledName << elementMangle.substr(1);
                } else {
                    mangledName << "custom";
                }
            }
            else {
                // For custom types (including enums), use the type name directly
                // but sanitize it to be a valid C++ identifier
                std::string sanitized;
                for (char c : paramType) {
                    if (std::isalnum(c)) {
                        sanitized += c;
                    } else if (c == ' ') {
                        sanitized += "_";
                    }
                }
                if (!sanitized.empty()) {
                    mangledName << sanitized;
                } else {
                    mangledName << "custom";
                }
            }
        }
    }
    
    return mangledName.str();
}

bool CppGenerator::isBuiltinFunction(const std::string& name) {
    // Convert to lowercase for comparison
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    return lowerName == "writeln" || lowerName == "write" || lowerName == "readln" || lowerName == "read" || lowerName == "length" || 
           lowerName == "chr" || lowerName == "ord" || lowerName == "pos" || lowerName == "copy" ||
           lowerName == "concat" || lowerName == "insert" || lowerName == "delete" ||
           lowerName == "assign" || lowerName == "reset" || lowerName == "rewrite" ||
           lowerName == "append" || lowerName == "close" || lowerName == "eof" || lowerName == "ioresult" ||
           lowerName == "new" || lowerName == "dispose" ||
           // File operations
           lowerName == "blockread" || lowerName == "blockwrite" || 
           lowerName == "filepos" || lowerName == "filesize" || lowerName == "seek" ||
           // System unit mathematical functions
           lowerName == "abs" || lowerName == "sqr" || lowerName == "sqrt" || lowerName == "sin" ||
           lowerName == "cos" || lowerName == "arctan" || lowerName == "ln" || lowerName == "exp" ||
           lowerName == "power" || lowerName == "tan" || lowerName == "round" || lowerName == "trunc" ||
           // System unit conversion functions  
           lowerName == "val" || lowerName == "str" ||
           lowerName == "inttostr" || lowerName == "floattostr" || 
           lowerName == "strtoint" || lowerName == "strtofloat" ||
           // System unit string functions
           lowerName == "upcase" || lowerName == "trim" || lowerName == "trimleft" ||
           lowerName == "trimright" || lowerName == "stringofchar" || lowerName == "lowercase" ||
           lowerName == "uppercase" || lowerName == "leftstr" || lowerName == "rightstr" ||
           lowerName == "padleft" || lowerName == "padright" ||
           // System unit I/O functions
           lowerName == "paramcount" || lowerName == "paramstr" ||
           // System unit system functions
           lowerName == "halt" || lowerName == "exit" || lowerName == "random" || lowerName == "randomize" ||
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

bool CppGenerator::isBuiltinConstant(const std::string& name) {
    // Convert to lowercase for comparison
    std::string lowerName = name;
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

int CppGenerator::getBuiltinConstantValue(const std::string& name) {
    // Convert to lowercase for comparison
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // CRT color constants (matching Turbo Pascal 7 values)
    if (lowerName == "black") return 0;
    if (lowerName == "blue") return 1;
    if (lowerName == "green") return 2;
    if (lowerName == "cyan") return 3;
    if (lowerName == "red") return 4;
    if (lowerName == "magenta") return 5;
    if (lowerName == "brown") return 6;
    if (lowerName == "lightgray") return 7;
    if (lowerName == "darkgray") return 8;
    if (lowerName == "lightblue") return 9;
    if (lowerName == "lightgreen") return 10;
    if (lowerName == "lightcyan") return 11;
    if (lowerName == "lightred") return 12;
    if (lowerName == "lightmagenta") return 13;
    if (lowerName == "yellow") return 14;
    if (lowerName == "white") return 15;
    if (lowerName == "blink") return 128;
    
    return 0; // Default
}

std::string CppGenerator::escapeCppString(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            case '\r': escaped += "\\r"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

void CppGenerator::generateRecordDefinition(const std::string& typeName, const std::string& definition) {
    // Parse the record definition to extract field names and types
    // Example: "record x , y : integer ; end" -> struct with x, y fields
    
    emitLine("struct " + typeName + " {");
    increaseIndent();
    
    // Simple parsing: look for field declarations between 'record' and 'end'
    size_t recordPos = definition.find("record");
    size_t endPos = definition.find("end");
    
    if (recordPos != std::string::npos && endPos != std::string::npos) {
        std::string fieldsSection = definition.substr(recordPos + 6, endPos - recordPos - 6);
        
        // Parse field declarations (simplified parsing)
        // Split by semicolons to get individual field declarations
        std::stringstream ss(fieldsSection);
        std::string fieldDecl;
        
        while (std::getline(ss, fieldDecl, ';')) {
            // Trim whitespace
            fieldDecl.erase(0, fieldDecl.find_first_not_of(" \t\n\r"));
            fieldDecl.erase(fieldDecl.find_last_not_of(" \t\n\r") + 1);
            
            if (fieldDecl.empty()) continue;
            
            // Find the colon separator
            size_t colonPos = fieldDecl.find(':');
            if (colonPos != std::string::npos) {
                std::string fieldNames = fieldDecl.substr(0, colonPos);
                std::string fieldType = fieldDecl.substr(colonPos + 1);
                
                // Trim field names and type
                fieldNames.erase(0, fieldNames.find_first_not_of(" \t\n\r"));
                fieldNames.erase(fieldNames.find_last_not_of(" \t\n\r") + 1);
                fieldType.erase(0, fieldType.find_first_not_of(" \t\n\r"));
                fieldType.erase(fieldType.find_last_not_of(" \t\n\r") + 1);
                
                // Handle multiple field names (e.g., "x, y")
                std::stringstream nameStream(fieldNames);
                std::string fieldName;
                while (std::getline(nameStream, fieldName, ',')) {
                    // Trim individual field name
                    fieldName.erase(0, fieldName.find_first_not_of(" \t\n\r"));
                    fieldName.erase(fieldName.find_last_not_of(" \t\n\r") + 1);
                    
                    if (!fieldName.empty()) {
                        emitIndent();
                        emitLine(mapPascalTypeToCpp(fieldType) + " " + fieldName + ";");
                    }
                }
            }
        }
    }
    
    decreaseIndent();
    emitLine("};");
    emitLine("");
}

void CppGenerator::generateArrayDefinition(const std::string& typeName, const std::string& definition) {
    // Parse array definition: "array[1..5] of integer" or "array[1..3, 1..3] of real"
    
    // Extract range and element type
    size_t arrayPos = definition.find("array[");
    size_t ofPos = definition.find("] of ");
    
    if (arrayPos != std::string::npos && ofPos != std::string::npos) {
        std::string rangeSpec = definition.substr(arrayPos + 6, ofPos - arrayPos - 6);
        std::string elementType = definition.substr(ofPos + 5);
        
        // Trim whitespace
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r"));
        elementType.erase(elementType.find_last_not_of(" \t\n\r") + 1);
        
        // Parse multiple dimensions separated by commas
        std::vector<std::string> dimensions;
        std::stringstream ss(rangeSpec);
        std::string dim;
        
        while (std::getline(ss, dim, ',')) {
            // Trim whitespace
            dim.erase(0, dim.find_first_not_of(" \t\n\r"));
            dim.erase(dim.find_last_not_of(" \t\n\r") + 1);
            dimensions.push_back(dim);
        }
        
        ArrayTypeInfo info;
        info.elementType = elementType;
        int totalSize = 1;
        bool allParsed = true;
        
        // Parse each dimension
        for (const std::string& dimRange : dimensions) {
            size_t dotdotPos = dimRange.find("..");
            
            // Check if it's an enum type (no .. found)
            if (dotdotPos == std::string::npos) {
                // Single enum type like "TColor"
                std::string enumTypeName = dimRange;
                enumTypeName.erase(0, enumTypeName.find_first_not_of(" \t\n\r"));
                enumTypeName.erase(enumTypeName.find_last_not_of(" \t\n\r") + 1);
                
                auto enumIt = enumTypes_.find(enumTypeName);
                if (enumIt != enumTypes_.end()) {
                    ArrayDimension dimension;
                    dimension.startIndex = 0; // Enums start at 0
                    dimension.endIndex = enumIt->second.size() - 1;
                    dimension.isCharacterRange = false;
                    dimension.isEnumRange = true;
                    dimension.enumTypeName = enumTypeName;
                    
                    int dimSize = enumIt->second.size();
                    totalSize *= dimSize;
                    info.dimensions.push_back(dimension);
                    
                    // Legacy support for single dimension
                    if (info.dimensions.size() == 1) {
                        info.startIndex = dimension.startIndex;
                        info.endIndex = dimension.endIndex;
                        info.isCharacterArray = false;
                    }
                } else {
                    allParsed = false;
                    break;
                }
            } else {
                // Range like "1..5" or "'A'..'D'"
                std::string startStr = dimRange.substr(0, dotdotPos);
                std::string endStr = dimRange.substr(dotdotPos + 2);
                
                // Trim
                startStr.erase(0, startStr.find_first_not_of(" \t\n\r"));
                startStr.erase(startStr.find_last_not_of(" \t\n\r") + 1);
                endStr.erase(0, endStr.find_first_not_of(" \t\n\r"));
                endStr.erase(endStr.find_last_not_of(" \t\n\r") + 1);
                
                try {
                    ArrayDimension dimension;
                    dimension.isEnumRange = false;
                    
                    // Handle character ranges like 'A'..'D'
                    if (startStr.length() == 3 && startStr[0] == '\'' && startStr[2] == '\'' &&
                        endStr.length() == 3 && endStr[0] == '\'' && endStr[2] == '\'') {
                        dimension.startIndex = static_cast<int>(startStr[1]);
                        dimension.endIndex = static_cast<int>(endStr[1]);
                        dimension.isCharacterRange = true;
                    } else {
                        // Handle numeric ranges
                        dimension.startIndex = std::stoi(startStr);
                        dimension.endIndex = std::stoi(endStr);
                        dimension.isCharacterRange = false;
                    }
                    
                    int dimSize = dimension.endIndex - dimension.startIndex + 1;
                    totalSize *= dimSize;
                    info.dimensions.push_back(dimension);
                    
                    // Legacy support for single dimension
                    if (info.dimensions.size() == 1) {
                        info.startIndex = dimension.startIndex;
                        info.endIndex = dimension.endIndex;
                        info.isCharacterArray = dimension.isCharacterRange;
                    }
                } catch (const std::exception&) {
                    allParsed = false;
                    break;
                }
            }
        }
        
        if (allParsed && !info.dimensions.empty()) {
            // Store array type information
            arrayTypes_[typeName] = info;
            
            std::string cppElementType = mapPascalTypeToCpp(elementType);
            emitLine("using " + typeName + " = std::array<" + cppElementType + ", " + std::to_string(totalSize) + ">;");
            emitLine("");
        } else {
            // Fallback for unparseable dimensions
            emitLine("// Array definition: " + typeName + " = " + definition);
            emitLine("using " + typeName + " = int; // TODO: implement proper array type");
        }
    } else {
        // Malformed array definition
        emitLine("// Array definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = int; // TODO: implement proper array type");
    }
}

void CppGenerator::generateRangeDefinition(const std::string& typeName, const std::string& definition) {
    // Parse range definition: "1..10" or "'A'..'Z'" 
    
    size_t dotdotPos = definition.find("..");
    if (dotdotPos != std::string::npos) {
        std::string startStr = definition.substr(0, dotdotPos);
        std::string endStr = definition.substr(dotdotPos + 2);
        
        // Trim whitespace
        startStr.erase(0, startStr.find_first_not_of(" \t\n\r"));
        startStr.erase(startStr.find_last_not_of(" \t\n\r") + 1);
        endStr.erase(0, endStr.find_first_not_of(" \t\n\r"));
        endStr.erase(endStr.find_last_not_of(" \t\n\r") + 1);
        
        // Check if it's a character range (if the definition contains single quotes)
        if (definition.find("'") != std::string::npos) {
            // Character range: 'A'..'Z' - extract characters from quoted strings
            char startChar = startStr[1]; // Skip the quote
            char endChar = endStr[1];     // Skip the quote
            
            emitLine("// Character range: " + typeName + " = " + definition);
            emitLine("using " + typeName + " = char;");
            emitLine("const char " + typeName + "_MIN = '" + std::string(1, startChar) + "';");
            emitLine("const char " + typeName + "_MAX = '" + std::string(1, endChar) + "';");
        } else {
            // Numeric range: 1..10
            try {
                int start = std::stoi(startStr);
                int end = std::stoi(endStr);
                
                emitLine("// Numeric range: " + typeName + " = " + definition);
                emitLine("using " + typeName + " = int;");
                emitLine("const int " + typeName + "_MIN = " + std::to_string(start) + ";");
                emitLine("const int " + typeName + "_MAX = " + std::to_string(end) + ";");
            } catch (const std::exception&) {
                // Fallback for non-numeric ranges
                emitLine("// Range definition: " + typeName + " = " + definition);
                emitLine("using " + typeName + " = int; // TODO: implement proper range type");
            }
        }
    } else {
        // Malformed range definition
        emitLine("// Range definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = int; // TODO: implement proper range type");
    }
    emitLine("");
}

void CppGenerator::generateBoundedStringDefinition(const std::string& typeName, const std::string& definition) {
    // Parse bounded string definition: "string[10]" -> "using TShortString = BoundedString<10>;"
    
    size_t bracketPos = definition.find("[");
    size_t endBracketPos = definition.find("]");
    
    if (bracketPos != std::string::npos && endBracketPos != std::string::npos) {
        std::string sizeStr = definition.substr(bracketPos + 1, endBracketPos - bracketPos - 1);
        
        // Trim whitespace
        sizeStr.erase(0, sizeStr.find_first_not_of(" \t\n\r"));
        sizeStr.erase(sizeStr.find_last_not_of(" \t\n\r") + 1);
        
        try {
            int size = std::stoi(sizeStr);
            
            emitLine("// Bounded string: " + typeName + " = " + definition);
            emitLine("class " + typeName + " {");
            increaseIndent();
            emitLine("private:");
            increaseIndent();
            emitLine("std::string data_;");
            emitLine("static const size_t MAX_LENGTH = " + std::to_string(size) + ";");
            decreaseIndent();
            emitLine("public:");
            increaseIndent();
            emitLine(typeName + "() = default;");
            emitLine(typeName + "(const std::string& s) : data_(s.length() > MAX_LENGTH ? s.substr(0, MAX_LENGTH) : s) {}");
            emitLine(typeName + "(const char* s) : data_(std::string(s).length() > MAX_LENGTH ? std::string(s).substr(0, MAX_LENGTH) : std::string(s)) {}");
            emitLine("");
            emitLine("operator std::string() const { return data_; }");
            emitLine("const std::string& str() const { return data_; }");
            emitLine("size_t length() const { return data_.length(); }");
            emitLine("");
            emitLine(typeName + "& operator=(const std::string& s) {");
            increaseIndent();
            emitLine("data_ = s.length() > MAX_LENGTH ? s.substr(0, MAX_LENGTH) : s;");
            emitLine("return *this;");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine(typeName + "& operator=(const char* s) {");
            increaseIndent();
            emitLine("return *this = std::string(s);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("// Concatenation operators");
            emitLine("friend " + typeName + " operator+(const " + typeName + "& lhs, const " + typeName + "& rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs.data_ + rhs.data_);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("friend " + typeName + " operator+(const " + typeName + "& lhs, const std::string& rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs.data_ + rhs);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("friend " + typeName + " operator+(const std::string& lhs, const " + typeName + "& rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs + rhs.data_);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("friend " + typeName + " operator+(const " + typeName + "& lhs, const char* rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs.data_ + std::string(rhs));");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("friend " + typeName + " operator+(const char* lhs, const " + typeName + "& rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(std::string(lhs) + rhs.data_);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("// Character concatenation operators");
            emitLine("friend " + typeName + " operator+(const " + typeName + "& lhs, char rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs.data_ + rhs);");
            decreaseIndent();
            emitLine("}");
            emitLine("");
            emitLine("friend " + typeName + " operator+(char lhs, const " + typeName + "& rhs) {");
            increaseIndent();
            emitLine("return " + typeName + "(lhs + rhs.data_);");
            decreaseIndent();
            emitLine("}");
            decreaseIndent();
            emitLine("};");
            emitLine("");
            emitLine("// Stream output operator for " + typeName);
            emitLine("inline std::ostream& operator<<(std::ostream& os, const " + typeName + "& obj) {");
            increaseIndent();
            emitLine("return os << obj.str();");
            decreaseIndent();
            emitLine("}");
        } catch (const std::exception&) {
            // Fallback for non-numeric sizes
            emitLine("// Bounded string definition: " + typeName + " = " + definition);
            emitLine("using " + typeName + " = std::string; // TODO: implement proper bounded string");
        }
    } else {
        // Malformed bounded string definition
        emitLine("// Bounded string definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = std::string; // TODO: implement proper bounded string");
    }
    emitLine("");
}

void CppGenerator::generatePointerDefinition(const std::string& typeName, const std::string& definition) {
    // Parse pointer definition: "^integer" -> "using PInteger = int32_t*;"
    if (!definition.empty() && definition[0] == '^') {
        std::string pointeeType = definition.substr(1); // Remove the '^' prefix
        
        // Trim whitespace
        size_t start = pointeeType.find_first_not_of(" \t");
        size_t end = pointeeType.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            pointeeType = pointeeType.substr(start, end - start + 1);
        }
        
        // Map the pointee type to C++ type
        std::string cppPointeeType = mapPascalTypeToCpp(pointeeType);
        
        emitLine("// Pointer type definition: " + typeName + " = " + definition);
        
        // Check if this pointee type looks like a Pascal record type (starts with capital)
        // and hasn't been mapped to a basic C++ type
        bool needsForwardDecl = !pointeeType.empty() && std::isupper(pointeeType[0]) && 
                               cppPointeeType == pointeeType &&
                               pointeeType.find("Node") != std::string::npos; // Heuristic for record types
        
        if (needsForwardDecl) {
            emitLine("struct " + pointeeType + "; // Forward declaration");
            emitLine("using " + typeName + " = " + pointeeType + "*;");
        } else {
            emitLine("using " + typeName + " = " + cppPointeeType + "*;");
        }
    } else {
        emitLine("// Invalid pointer definition: " + definition);
        emitLine("using " + typeName + " = void*;");
    }
    emitLine("");
}

void CppGenerator::generateSetDefinition(const std::string& typeName, const std::string& definition) {
    // Parse set definition: "set of char" -> "using TCharSet = std::set<char>;"
    
    size_t setOfPos = definition.find("set of");
    if (setOfPos != std::string::npos) {
        std::string elementType = definition.substr(setOfPos + 7); // Skip "set of "
        
        // Trim whitespace
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r"));
        elementType.erase(elementType.find_last_not_of(" \t\n\r") + 1);
        
        // Check if the element type is an enum - if so, use int for internal representation
        if (enumTypes_.find(elementType) != enumTypes_.end()) {
            emitLine("using " + typeName + " = std::set<int>; // Set of enum " + elementType);
        } else {
            std::string cppElementType = mapPascalTypeToCpp(elementType);
            emitLine("using " + typeName + " = std::set<" + cppElementType + ">;");
        }
        emitLine("");
    } else {
        // Malformed set definition
        emitLine("// Set definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = std::set<int>; // TODO: implement proper set type");
        emitLine("");
    }
}

void CppGenerator::generateEnumDefinition(const std::string& typeName, const std::string& definition) {
    // Parse enumeration definition: "(Red, Green, Blue)"
    
    if (definition.length() > 2 && definition[0] == '(' && definition.back() == ')') {
        std::string enumValues = definition.substr(1, definition.length() - 2); // Remove parentheses
        
        // Store enum information for array indexing
        EnumTypeInfo enumInfo;
        
        emitLine("// Enumeration: " + typeName + " = " + definition);
        emitLine("enum class " + typeName + " {");
        increaseIndent();
        
        // Parse individual enum values
        size_t pos = 0;
        int enumOrdinal = 0;
        bool firstValue = true;
        
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
                
                // Store in enum info
                enumInfo.values.push_back(enumValue);
                
                if (!firstValue) {
                    emit(",");
                    emitLine("");
                }
                emitIndent();
                emit(enumValue + " = " + std::to_string(enumOrdinal));
                firstValue = false;
                enumOrdinal++;
            }
            
            pos = commaPos + 1;
        }
        
        emitLine("");
        decreaseIndent();
        emitLine("};");
        
        // Store enum information
        enumTypes_[typeName] = enumInfo;
        
        // Generate constants for the enum values to use in Pascal code
        emitLine("");
        emitLine("// Enum value constants for Pascal compatibility");
        for (size_t i = 0; i < enumInfo.values.size(); ++i) {
            const std::string& enumValue = enumInfo.values[i];
            // Add conflict resolution for Windows API functions
            if (enumValue == "Rectangle") {
                emitLine("#ifdef Rectangle");
                emitLine("#undef Rectangle");
                emitLine("#endif");
            }
            emitLine("const " + typeName + " " + enumValue + " = " + typeName + "::" + enumValue + ";");
        }
    } else {
        // Malformed enum definition
        emitLine("// Enum definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = int; // TODO: implement proper enum type");
    }
    emitLine("");
}

void CppGenerator::generateFileDefinition(const std::string& typeName, const std::string& definition) {
    // Parse file definition: "file of integer", "file of real", etc.
    
    if (definition.find("file of") == 0) {
        std::string elementType = definition.substr(8); // Skip "file of "
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r")); // Trim leading whitespace
        
        std::string cppElementType = mapPascalTypeToCpp(elementType);
        
        emitLine("// File type: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = PascalTypedFile<" + cppElementType + ">;");
    } else {
        // Fallback for malformed file definitions
        emitLine("// File definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = PascalFile; // Fallback to untyped file");
    }
    emitLine("");
}

void CppGenerator::visit(UsesClause& node) {
    // Generate include statements for units
    emitLine("// Uses clause");
    for (const std::string& unitName : node.getUnits()) {
        if (unitName == "System") {
            // System unit is automatically included via our built-in functions
            emitLine("// System unit functions automatically available");
        } else if (unitName == "Dos") {
            emitLine("#include <filesystem>  // DOS unit support");
            emitLine("#include <chrono>      // Date/time functions");
        } else if (unitName == "Crt") {
            emitLine("#ifdef _WIN32");
            emitLine("#include <conio.h>     // CRT unit support (Windows)");
            // Only include windows.h if we need console functions that aren't in conio.h
            emitLine("#include <windows.h>   // Console API");
            emitLine("#ifdef Rectangle");
            emitLine("#undef Rectangle       // Avoid conflict with Pascal Rectangle identifier");
            emitLine("#endif");
            emitLine("#else");
            emitLine("#include <unistd.h>");
            emitLine("#include <termios.h>");
            emitLine("#endif");
        } else {
            // Generate C++ code for custom units
            if (unitLoader_ && unitLoader_->isUnitLoaded(unitName)) {
                emitLine("// Unit: " + unitName);
                Unit* loadedUnit = unitLoader_->getLoadedUnit(unitName);
                if (loadedUnit) {
                    emitLine("// Interface declarations");
                    // Generate interface declarations
                    for (const auto& decl : loadedUnit->getInterfaceDeclarations()) {
                        // For function/procedure declarations in interfaces, generate prototypes
                        if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                            std::string returnType = mapPascalTypeToCpp(funcDecl->getReturnType());
                            emitLine(returnType + " " + funcDecl->getName() + "(" + generateParameterList(funcDecl->getParameters()) + ");");
                        } else if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(decl.get())) {
                            emitLine("void " + procDecl->getName() + "(" + generateParameterList(procDecl->getParameters()) + ");");
                        } else {
                            // For other declarations (types, constants, variables), use normal generation
                            decl->accept(*this);
                        }
                    }
                    
                    emitLine("// Implementation");
                    // Generate implementation declarations (function/procedure bodies)
                    for (const auto& decl : loadedUnit->getImplementationDeclarations()) {
                        decl->accept(*this);
                    }
                }
            } else {
                emitLine("// TODO: Include unit " + unitName);
            }
        }
    }
    emitLine("");
}

void CppGenerator::visit(Unit& node) {
    // Generate C++ header and implementation for the unit
    
    // Generate header section from interface declarations
    emitLine("// Unit: " + node.getName());
    emitLine("// Interface declarations");
    
    for (const auto& decl : node.getInterfaceDeclarations()) {
        decl->accept(*this);
    }
    
    emitLine("");
    emitLine("// Implementation declarations");
    
    // Generate implementation section
    for (const auto& decl : node.getImplementationDeclarations()) {
        decl->accept(*this);
    }
    
    // Generate initialization code if present
    if (node.getInitializationBlock()) {
        emitLine("");
        emitLine("// Unit initialization");
        emitLine("class " + node.getName() + "_Initializer {");
        emitLine("public:");
        emitLine("    " + node.getName() + "_Initializer() {");
        
        // Generate initialization code
        for (const auto& stmt : node.getInitializationBlock()->getStatements()) {
            emitLine("        ");
            stmt->accept(*this);
        }
        
        emitLine("    }");
        emitLine("};");
        emitLine("static " + node.getName() + "_Initializer " + node.getName() + "_init;");
    }
}

std::vector<std::string> CppGenerator::expandEnumRange(const std::string& startName, const std::string& endName) {
    std::vector<std::string> result;
    
    if (!symbolTable_) {
        return result;
    }
    
    // Look up the start element to find its enum type
    auto startSymbol = symbolTable_->lookup(startName);
    if (!startSymbol || startSymbol->getSymbolType() != SymbolType::CONSTANT || 
        startSymbol->getDataType() != DataType::CUSTOM) {
        return result;
    }
    
    std::string enumTypeName = startSymbol->getTypeName();
    
    // Look up the enum type definition
    auto enumTypeSymbol = symbolTable_->lookup(enumTypeName);
    if (!enumTypeSymbol || enumTypeSymbol->getSymbolType() != SymbolType::TYPE_DEF) {
        return result;
    }
    
    std::string enumDef = enumTypeSymbol->getTypeDefinition();
    
    // Parse the enum definition to extract all values
    // Format: (Value1, Value2, Value3, ...)
    if (enumDef.empty() || enumDef[0] != '(' || enumDef.back() != ')') {
        return result;
    }
    
    std::string enumValues = enumDef.substr(1, enumDef.length() - 2); // Remove parentheses
    std::vector<std::string> allValues;
    
    // Split by commas
    std::stringstream ss(enumValues);
    std::string value;
    while (std::getline(ss, value, ',')) {
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        if (!value.empty()) {
            allValues.push_back(value);
        }
    }
    
    // Find the indices of start and end values
    int startIndex = -1, endIndex = -1;
    for (size_t i = 0; i < allValues.size(); ++i) {
        if (allValues[i] == startName) {
            startIndex = static_cast<int>(i);
        }
        if (allValues[i] == endName) {
            endIndex = static_cast<int>(i);
        }
    }
    
    // Extract the range
    if (startIndex >= 0 && endIndex >= 0 && startIndex <= endIndex) {
        for (int i = startIndex; i <= endIndex; ++i) {
            result.push_back(allValues[i]);
        }
    }
    
    return result;
}

} // namespace rpascal