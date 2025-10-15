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
        case TokenType::NIL:
            emit("nullptr");
            break;
        default:
            emit(token.getValue());
    }
}

void CppGenerator::visit(IdentifierExpression& node) {
    std::string name = node.getName();
    
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
                if (name == "randomize") {
                    emit("std::srand(static_cast<unsigned int>(std::time(nullptr)))");
                    return;
                } else if (name == "exit") {
                    emit("return");
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
    // Handle special operators
    if (node.getOperator().getType() == TokenType::IN) {
        // Pascal 'in' operator: element in set
        // Use lambda to avoid duplicate set creation: [&](){ auto s = set; return s.find(element) != s.end(); }()
        emit("([&](){ auto temp_set = ");
        node.getRight()->accept(*this); // set
        emit("; return temp_set.find(");
        node.getLeft()->accept(*this);  // element
        emit(") != temp_set.end(); })()");
        return;
    }
    
    // Handle string concatenation
    if (node.getOperator().getType() == TokenType::PLUS) {
        // Use std::string constructor to ensure at least one operand is a std::string
        emit("(std::string(");
        node.getLeft()->accept(*this);
        emit(") + ");
        node.getRight()->accept(*this);
        emit(")");
        return;
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
    node.getObject()->accept(*this);
    emit(".");
    emit(node.getFieldName());
}

void CppGenerator::visit(ArrayIndexExpression& node) {
    // Get the array variable name to look up its type
    std::string arrayVarName;
    if (auto* identifierExpr = dynamic_cast<IdentifierExpression*>(node.getArray())) {
        arrayVarName = identifierExpr->getName();
    }
    
    // Look up the variable's type from symbol table
    std::string arrayTypeName;
    if (!arrayVarName.empty() && symbolTable_) {
        auto symbol = symbolTable_->lookup(arrayVarName);
        if (symbol && symbol->getSymbolType() == SymbolType::VARIABLE) {
            arrayTypeName = symbol->getTypeName();
        }
    }
    
    const std::vector<std::unique_ptr<Expression>>& indices = node.getIndices();
    
    // Check if we have array type information
    auto arrayTypeIt = arrayTypes_.find(arrayTypeName);
    if (arrayTypeIt != arrayTypes_.end()) {
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
        // No type information - use legacy behavior
        node.getArray()->accept(*this);
        emit("[");
        
        if (indices.size() == 1) {
            emit("(");
            indices[0]->accept(*this);
            emit(") - 1");
        } else {
            // Multi-dimensional but no type info - just use first index
            indices[0]->accept(*this);
            emit(" - 1");
        }
        
        emit("]");
    }
}

void CppGenerator::visit(SetLiteralExpression& node) {
    // Generate C++ set literal with proper type inference
    // For now, determine type from first element - could be improved with full type analysis
    std::string elementType = "int"; // default to int
    
    const auto& elements = node.getElements();
    if (!elements.empty()) {
        // Check if first element is a character literal
        if (auto* literal = dynamic_cast<LiteralExpression*>(elements[0].get())) {
            if (literal->getToken().getType() == TokenType::CHAR_LITERAL) {
                elementType = "char";
            } else if (literal->getToken().getType() == TokenType::INTEGER_LITERAL) {
                elementType = "int";
            }
        }
    }
    
    emit("std::set<" + elementType + ">{");
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) emit(", ");
        elements[i]->accept(*this);
    }
    emit("}");
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
    
    decreaseIndent();
    emitLine("};");
    emitLine("");
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
    // Skip forward declarations - they're handled in generateForwardDeclarations
    if (node.isForward()) {
        return;
    }
    
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
    // Skip forward declarations - they're handled in generateForwardDeclarations
    if (node.isForward()) {
        return;
    }
    
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
           "#include <cstdint>\n"
           "#include <cmath>\n"
           "#include <cstdlib>\n"
           "#include <ctime>\n"
           "#include <cctype>";
}

std::string CppGenerator::generateRuntimeIncludes() {
    return "// Using explicit std:: prefixes to avoid name conflicts\n\n"
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
           "    }\n"
           "    \n"
           "    void rewrite() {\n"
           "        close();\n"
           "        stream_.open(filename_, std::ios::out);\n"
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
           "};";
}

std::string CppGenerator::generateForwardDeclarations(const std::vector<std::unique_ptr<Declaration>>& declarations) {
    std::ostringstream forward;
    
    for (const auto& decl : declarations) {
        if (auto procDecl = dynamic_cast<ProcedureDeclaration*>(decl.get())) {
            if (procDecl->isForward()) {
                forward << "void " << procDecl->getName() << "(" << generateParameterList(procDecl->getParameters()) << ");\n";
            }
        } else if (auto funcDecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            if (funcDecl->isForward()) {
                std::string returnType = mapPascalTypeToCpp(funcDecl->getReturnType());
                forward << returnType << " " << funcDecl->getName() << "(" << generateParameterList(funcDecl->getParameters()) << ");\n";
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
        std::string pointeeType = lowerType.substr(1);
        return mapPascalTypeToCpp(pointeeType) + "*";
    }
    
    if (lowerType == "integer") return "int32_t";
    if (lowerType == "real") return "double";
    if (lowerType == "boolean") return "bool";
    if (lowerType == "char") return "char";
    if (lowerType == "string") return "std::string";
    if (lowerType == "text" || lowerType == "file") return "PascalFile";
    
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
        // Check if first argument is a file variable
        if (!node.getArguments().empty()) {
            // Check if the first argument looks like a file (simple heuristic)
            auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get());
            if (firstArg && (firstArg->getName() == "f" || 
                           firstArg->getName().find("file") != std::string::npos ||
                           firstArg->getName().find("File") != std::string::npos)) {
                // File output: writeln(f, args...) -> f.getStream() << args... << std::endl
                node.getArguments()[0]->accept(*this);
                emit(".getStream()");
                for (size_t i = 1; i < node.getArguments().size(); ++i) {
                    emit(" << ");
                    node.getArguments()[i]->accept(*this);
                }
                emit(" << std::endl");
                return;
            }
        }
        
        // Console output: writeln(args...) -> std::cout << args... << std::endl
        emit("std::cout");
        for (const auto& arg : node.getArguments()) {
            emit(" << ");
            arg->accept(*this);
        }
        emit(" << std::endl");
    } else if (functionName == "readln") {
        // Check if first argument is a file variable
        if (!node.getArguments().empty()) {
            auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get());
            if (firstArg && (firstArg->getName() == "f" || 
                           firstArg->getName().find("file") != std::string::npos ||
                           firstArg->getName().find("File") != std::string::npos)) {
                // File input: readln(f, var) -> f.getStream() >> var
                node.getArguments()[0]->accept(*this);
                emit(".getStream()");
                for (size_t i = 1; i < node.getArguments().size(); ++i) {
                    emit(" >> ");
                    node.getArguments()[i]->accept(*this);
                }
                return;
            }
        }
        
        // Console input: readln(var) -> std::cin >> var
        emit("std::cin");
        for (const auto& arg : node.getArguments()) {
            emit(" >> ");
            arg->accept(*this);
        }
    } else if (functionName == "read") {
        // read(f, var) -> f.getStream() >> var
        if (node.getArguments().size() >= 2) {
            // Check if first argument is a file variable
            auto firstArg = dynamic_cast<IdentifierExpression*>(node.getArguments()[0].get());
            if (firstArg && (firstArg->getName() == "f" || 
                           firstArg->getName().find("file") != std::string::npos ||
                           firstArg->getName().find("File") != std::string::npos)) {
                // File input: read(f, var) -> f.getStream() >> var
                node.getArguments()[0]->accept(*this);
                emit(".getStream()");
                for (size_t i = 1; i < node.getArguments().size(); ++i) {
                    emit(" >> ");
                    node.getArguments()[i]->accept(*this);
                }
                return;
            }
        }
        
        // Console input: read(var) -> std::cin >> var
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
    } else if (functionName == "pos") {
        // pos(substr, str) -> str.find(substr) + 1 (Pascal is 1-indexed)
        emit("(");
        if (node.getArguments().size() >= 2) {
            node.getArguments()[1]->accept(*this); // str
            emit(".find(");
            node.getArguments()[0]->accept(*this); // substr
            emit(") != std::string::npos ? ");
            node.getArguments()[1]->accept(*this); // str
            emit(".find(");
            node.getArguments()[0]->accept(*this); // substr
            emit(") + 1 : 0)");
        }
    } else if (functionName == "copy") {
        // copy(str, start, length) -> str.substr(start-1, length)
        emit("(");
        if (node.getArguments().size() >= 3) {
            node.getArguments()[0]->accept(*this); // str
            emit(".substr(");
            node.getArguments()[1]->accept(*this); // start
            emit(" - 1, ");
            node.getArguments()[2]->accept(*this); // length
            emit(")");
        } else if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // str
            emit(".substr(");
            node.getArguments()[1]->accept(*this); // start
            emit(" - 1)");
        }
        emit(")");
    } else if (functionName == "concat") {
        // concat(str1, str2, ...) -> std::string(str1) + str2 + ...
        emit("(");
        for (size_t i = 0; i < node.getArguments().size(); ++i) {
            if (i > 0) emit(" + ");
            if (i == 0) emit("std::string("); // Wrap first argument
            node.getArguments()[i]->accept(*this);
            if (i == 0) emit(")"); // Close wrapper for first argument
        }
        emit(")");
    } else if (functionName == "insert") {
        // insert(substr, str, pos) - Pascal modifies string in place
        // Generate: str.insert(pos-1, substr)
        if (node.getArguments().size() >= 3) {
            node.getArguments()[1]->accept(*this); // str
            emit(".insert(");
            node.getArguments()[2]->accept(*this); // pos
            emit(" - 1, ");
            node.getArguments()[0]->accept(*this); // substr
            emit(")");
        }
    } else if (functionName == "delete") {
        // delete(str, pos, length) - Pascal modifies string in place
        // Generate: str.erase(pos-1, length)
        if (node.getArguments().size() >= 3) {
            node.getArguments()[0]->accept(*this); // str
            emit(".erase(");
            node.getArguments()[1]->accept(*this); // pos
            emit(" - 1, ");
            node.getArguments()[2]->accept(*this); // length
            emit(")");
        };
    } else if (functionName == "assign") {
        // assign(f, filename) -> f.assign(filename)
        if (node.getArguments().size() >= 2) {
            node.getArguments()[0]->accept(*this); // file variable
            emit(".assign(");
            node.getArguments()[1]->accept(*this); // filename
            emit(")");
        }
    } else if (functionName == "reset") {
        // reset(f) -> f.reset()
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this); // file variable
            emit(".reset()");
        }
    } else if (functionName == "rewrite") {
        // rewrite(f) -> f.rewrite()
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this); // file variable
            emit(".rewrite()");
        }
    } else if (functionName == "close") {
        // close(f) -> f.close()
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this); // file variable
            emit(".close()");
        }
    } else if (functionName == "eof") {
        // eof(f) -> f.eof()
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this); // file variable
            emit(".eof()");
        }
    } else if (functionName == "new") {
        // new(ptr) -> ptr = new <type>
        // For simplicity, we'll generate a basic new for integer pointers
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this); // pointer variable
            emit(" = new int32_t()");
        }
    } else if (functionName == "dispose") {
        // dispose(ptr) -> delete ptr; ptr = nullptr
        if (!node.getArguments().empty()) {
            emit("delete ");
            node.getArguments()[0]->accept(*this); // pointer variable
            emit("; ");
            node.getArguments()[0]->accept(*this); // pointer variable
            emit(" = nullptr");
        }
    
    // === SYSTEM UNIT FUNCTIONS ===
    
    // Mathematical functions
    } else if (functionName == "abs") {
        // abs(x) -> std::abs(x)
        emit("std::abs(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "sqr") {
        // sqr(x) -> (x * x)
        emit("(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
            emit(" * ");
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "sqrt") {
        // sqrt(x) -> std::sqrt(x)
        emit("std::sqrt(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "sin") {
        // sin(x) -> std::sin(x)
        emit("std::sin(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "cos") {
        // cos(x) -> std::cos(x)
        emit("std::cos(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "arctan") {
        // arctan(x) -> std::atan(x)
        emit("std::atan(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "ln") {
        // ln(x) -> std::log(x)
        emit("std::log(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    } else if (functionName == "exp") {
        // exp(x) -> std::exp(x)
        emit("std::exp(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit(")");
    
    // Conversion functions
    } else if (functionName == "val") {
        // val(s, result, code) -> Custom implementation needed
        // For now, generate a simple conversion
        if (node.getArguments().size() >= 2) {
            emit("/* val not fully implemented */ ");
            node.getArguments()[1]->accept(*this); // result variable
            emit(" = std::stoi(");
            node.getArguments()[0]->accept(*this); // string
            emit(")");
        }
    } else if (functionName == "str") {
        // str(x, s) -> s = std::to_string(x)
        if (node.getArguments().size() >= 2) {
            node.getArguments()[1]->accept(*this); // string variable
            emit(" = std::to_string(");
            node.getArguments()[0]->accept(*this); // value
            emit(")");
        }
    
    // String functions
    } else if (functionName == "upcase") {
        // upcase(c) -> static_cast<char>(std::toupper(c))
        emit("static_cast<char>(std::toupper(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("))");
    
    // I/O functions
    } else if (functionName == "paramcount") {
        // paramcount -> (argc - 1) // assuming global argc available
        emit("(pascal_argc - 1)");
    } else if (functionName == "paramstr") {
        // paramstr(i) -> pascal_argv[i] // assuming global argv available
        emit("std::string(pascal_argv[");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        }
        emit("])");
    
    // System functions
    } else if (functionName == "halt") {
        // halt(code) -> std::exit(code)
        emit("std::exit(");
        if (!node.getArguments().empty()) {
            node.getArguments()[0]->accept(*this);
        } else {
            emit("0");
        }
        emit(")");
    } else if (functionName == "exit") {
        // exit -> return (from current function)
        emit("return");
    } else if (functionName == "random") {
        // random -> (static_cast<double>(std::rand()) / RAND_MAX)
        emit("(static_cast<double>(std::rand()) / RAND_MAX)");
    } else if (functionName == "randomize") {
        // randomize -> std::srand(std::time(nullptr))
        emit("std::srand(static_cast<unsigned int>(std::time(nullptr)))");
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

bool CppGenerator::isBuiltinFunction(const std::string& name) {
    return name == "writeln" || name == "readln" || name == "read" || name == "length" || 
           name == "chr" || name == "ord" || name == "pos" || name == "copy" ||
           name == "concat" || name == "insert" || name == "delete" ||
           name == "assign" || name == "reset" || name == "rewrite" ||
           name == "close" || name == "eof" || name == "new" || name == "dispose" ||
           // System unit mathematical functions
           name == "abs" || name == "sqr" || name == "sqrt" || name == "sin" ||
           name == "cos" || name == "arctan" || name == "ln" || name == "exp" ||
           // System unit conversion functions  
           name == "val" || name == "str" ||
           // System unit string functions
           name == "upcase" ||
           // System unit I/O functions
           name == "paramcount" || name == "paramstr" ||
           // System unit system functions
           name == "halt" || name == "exit" || name == "random" || name == "randomize";
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

void CppGenerator::generateSetDefinition(const std::string& typeName, const std::string& definition) {
    // Parse set definition: "set of char" -> "using TCharSet = std::set<char>;"
    
    size_t setOfPos = definition.find("set of");
    if (setOfPos != std::string::npos) {
        std::string elementType = definition.substr(setOfPos + 7); // Skip "set of "
        
        // Trim whitespace
        elementType.erase(0, elementType.find_first_not_of(" \t\n\r"));
        elementType.erase(elementType.find_last_not_of(" \t\n\r") + 1);
        
        std::string cppElementType = mapPascalTypeToCpp(elementType);
        emitLine("using " + typeName + " = std::set<" + cppElementType + ">;");
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
            emitLine("const " + typeName + " " + enumValue + " = " + typeName + "::" + enumValue + ";");
        }
    } else {
        // Malformed enum definition
        emitLine("// Enum definition: " + typeName + " = " + definition);
        emitLine("using " + typeName + " = int; // TODO: implement proper enum type");
    }
    emitLine("");
}

} // namespace rpascal