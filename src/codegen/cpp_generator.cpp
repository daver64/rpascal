#include "../include/cpp_generator.h"
#include <algorithm>

namespace rpascal {

CppGenerator::CppGenerator(std::shared_ptr<SymbolTable> symbolTable)
    : symbolTable_(symbolTable), indentLevel_(0) {}

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
        default:
            emit(token.getValue());
    }
}

void CppGenerator::visit(IdentifierExpression& node) {
    std::string name = node.getName();
    
    // Check if this is a procedure call without parentheses
    if (symbolTable_) {
        auto symbol = symbolTable_->lookup(name);
        if (symbol && symbol->getSymbolType() == SymbolType::PROCEDURE) {
            // For procedures called without parameters, add parentheses for C++
            emit(name + "()");
            return;
        }
    }
    
    // Regular variable or function reference
    emit(name);
}

void CppGenerator::visit(BinaryExpression& node) {
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

void CppGenerator::visit(CallExpression& node) {
    generateFunctionCall(node);
}

void CppGenerator::visit(FieldAccessExpression& node) {
    node.getObject()->accept(*this);
    emit(".");
    emit(node.getFieldName());
}

void CppGenerator::visit(ArrayIndexExpression& node) {
    node.getArray()->accept(*this);
    emit("[");
    node.getIndex()->accept(*this);
    emit("]");
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
    if (targetId && !currentFunction_.empty() && targetId->getName() == currentFunction_) {
        // This is an assignment to the function name (return value)
        emit(currentFunction_ + "_result = ");
    } else {
        node.getTarget()->accept(*this);
        emit(" = ");
    }
    
    node.getValue()->accept(*this);
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
        emit("for (" + node.getVariable() + " = ");
        node.getStart()->accept(*this);
        emit("; " + node.getVariable() + " >= ");
        node.getEnd()->accept(*this);
        emitLine("; " + node.getVariable() + "--) {");
    } else {
        // For to loops: for (var = start; var <= end; var++)
        emit("for (" + node.getVariable() + " = ");
        node.getStart()->accept(*this);
        emit("; " + node.getVariable() + " <= ");
        node.getEnd()->accept(*this);
        emitLine("; " + node.getVariable() + "++) {");
    }
    
    increaseIndent();
    node.getBody()->accept(*this);
    decreaseIndent();
    
    emitLine("}");
}

void CppGenerator::visit(ConstantDeclaration& node) {
    emitIndent();
    emit("const auto " + node.getName() + " = ");
    node.getValue()->accept(*this);
    emitLine(";");
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
    // Handle range types  
    else if (definition.find("..") != std::string::npos) {
        generateRangeDefinition(node.getName(), definition);
    }
    else {
        // For other types, generate a comment for now
        emitLine("// Type definition: " + node.getName() + " = " + definition);
        emitLine("using " + node.getName() + " = int; // TODO: implement proper type");
    }
}

void CppGenerator::visit(VariableDeclaration& node) {
    std::string cppType = mapPascalTypeToCpp(node.getType());
    emitIndent();
    emit(cppType + " " + node.getName());
    
    if (node.getInitializer()) {
        emit(" = ");
        node.getInitializer()->accept(*this);
    }
    
    emitLine(";");
}

void CppGenerator::visit(ProcedureDeclaration& node) {
    emitLine("void " + node.getName() + "(" + generateParameterList(node.getParameters()) + ") {");
    
    increaseIndent();
    
    // Generate local variable declarations
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    node.getBody()->accept(*this);
    decreaseIndent();
    
    emitLine("}");
    emitLine("");
}

void CppGenerator::visit(FunctionDeclaration& node) {
    std::string returnType = mapPascalTypeToCpp(node.getReturnType());
    emitLine(returnType + " " + node.getName() + "(" + generateParameterList(node.getParameters()) + ") {");
    
    increaseIndent();
    
    // Declare return variable (Pascal functions assign to function name)
    emitIndent();
    emitLine(returnType + " " + node.getName() + "_result;");
    
    // Generate local variable declarations
    for (const auto& localVar : node.getLocalVariables()) {
        localVar->accept(*this);
    }
    
    currentFunction_ = node.getName();
    node.getBody()->accept(*this);
    currentFunction_ = "";
    
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
    
    // Generate main function
    emitLine("int main() {");
    increaseIndent();
    
    node.getMainBlock()->accept(*this);
    
    emitIndent();
    emitLine("return 0;");
    
    decreaseIndent();
    emitLine("}");
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
           "#include <string>\n"
           "#include <array>\n"
           "#include <cstdint>";
}

std::string CppGenerator::generateRuntimeIncludes() {
    return "// Using explicit std:: prefixes to avoid name conflicts";
}

std::string CppGenerator::generateForwardDeclarations(const std::vector<std::unique_ptr<Declaration>>& declarations) {
    std::ostringstream forward;
    
    for (const auto& decl : declarations) {
        if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(decl.get())) {
            forward << "void " << procDecl->getName() << "(" << generateParameterList(procDecl->getParameters()) << ");\n";
        } else if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            std::string returnType = mapPascalTypeToCpp(funcDecl->getReturnType());
            forward << returnType << " " << funcDecl->getName() << "(" << generateParameterList(funcDecl->getParameters()) << ");\n";
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
    
    if (lowerType == "integer") return "int32_t";
    if (lowerType == "real") return "double";
    if (lowerType == "boolean") return "bool";
    if (lowerType == "char") return "char";
    if (lowerType == "string") return "std::string";
    
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
        // Regular function call
        emit(functionName + "(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
    }
}

void CppGenerator::generateBuiltinCall(CallExpression& node, const std::string& functionName) {
    if (functionName == "writeln") {
        emit("std::cout");
        for (const auto& arg : node.getArguments()) {
            emit(" << ");
            arg->accept(*this);
        }
        emit(" << std::endl");
    } else if (functionName == "readln") {
        emit("std::cin");
        for (const auto& arg : node.getArguments()) {
            emit(" >> ");
            arg->accept(*this);
        }
    } else if (functionName == "length") {
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(".length()");
        }
    } else if (functionName == "chr") {
        emit("static_cast<char>(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "ord") {
        emit("static_cast<int>(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else {
        // Default function call
        emit(functionName + "(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(", ");
            node.getArguments()[i]->accept(*this);
        }
        emit(")");
    }
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
        params << mapPascalTypeToCpp(parameters[i]->getType()) << " " << parameters[i]->getName();
    }
    
    return params.str();
}

bool CppGenerator::isBuiltinFunction(const std::string& name) {
    return name == "writeln" || name == "readln" || name == "length" || 
           name == "chr" || name == "ord";
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
    // Parse array definition: "array[1..5] of integer" -> "using TArray = std::array<int, 5>;"
    
    // Extract range and element type
    size_t arrayPos = definition.find("array[");
    size_t ofPos = definition.find("] of ");
    
    if (arrayPos != std::string::npos && ofPos != std::string::npos) {
        std::string rangeSpec = definition.substr(arrayPos + 6, ofPos - arrayPos - 6);
        std::string elementType = definition.substr(ofPos + 5);
        
        // Trim whitespace
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r"));
        elementType.erase(elementType.find_last_not_of(" \t\n\r") + 1);
        
        // Parse range: "1..5" -> size = 5 - 1 + 1 = 5
        size_t dotdotPos = rangeSpec.find("..");
        if (dotdotPos != std::string::npos) {
            std::string startStr = rangeSpec.substr(0, dotdotPos);
            std::string endStr = rangeSpec.substr(dotdotPos + 2);
            
            // Trim numbers
            startStr.erase(0, startStr.find_first_not_of(" \t\n\r"));
            startStr.erase(startStr.find_last_not_of(" \t\n\r") + 1);
            endStr.erase(0, endStr.find_first_not_of(" \t\n\r"));
            endStr.erase(endStr.find_last_not_of(" \t\n\r") + 1);
            
            try {
                int start = std::stoi(startStr);
                int end = std::stoi(endStr);
                int size = end - start + 1;
                
                std::string cppElementType = mapPascalTypeToCpp(elementType);
                emitLine("using " + typeName + " = std::array<" + cppElementType + ", " + std::to_string(size) + ">;");
                emitLine("");
            } catch (const std::exception&) {
                // Fallback for non-numeric ranges
                emitLine("// Array definition: " + typeName + " = " + definition);
                emitLine("using " + typeName + " = int; // TODO: implement proper array type");
            }
        } else {
            // Single index or unknown format
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

void CppGenerator::generateEnumDefinition(const std::string& typeName, const std::string& definition) {
    // Parse enumeration definition: "(Red, Green, Blue)"
    
    if (definition.length() > 2 && definition[0] == '(' && definition.back() == ')') {
        std::string enumValues = definition.substr(1, definition.length() - 2); // Remove parentheses
        
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
        
        // Generate constants for the enum values to use in Pascal code
        emitLine("");
        emitLine("// Enum value constants for Pascal compatibility");
        pos = 0;
        enumOrdinal = 0;
        while (pos < enumValues.length()) {
            size_t commaPos = enumValues.find(',', pos);
            if (commaPos == std::string::npos) {
                commaPos = enumValues.length();
            }
            
            std::string enumValue = enumValues.substr(pos, commaPos - pos);
            size_t start = enumValue.find_first_not_of(" \t");
            size_t end = enumValue.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                enumValue = enumValue.substr(start, end - start + 1);
                emitLine("const " + typeName + " " + enumValue + " = " + typeName + "::" + enumValue + ";");
                enumOrdinal++;
            }
            
            pos = commaPos + 1;
        }
    } else {
        // Malformed enum definition
        emitLine("// Enum definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = int; // TODO: implement proper enum type");
    }
    emitLine("");
}

} // namespace rpascal