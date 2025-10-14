#pragma once

#include "ast.h"
#include "symbol_table.h"
#include <memory>
#include <string>
#include <sstream>
#include <vector>

namespace rpascal {

// C++ code generator that converts Pascal AST to C++ code
class CppGenerator : public ASTVisitor {
public:
    explicit CppGenerator(std::shared_ptr<SymbolTable> symbolTable);
    
    // Generate C++ code for the entire program
    std::string generate(Program& program);
    
    // Visitor pattern implementation
    void visit(LiteralExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(FieldAccessExpression& node) override;
    void visit(ArrayIndexExpression& node) override;
    
    void visit(ExpressionStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;
    
    void visit(ConstantDeclaration& node) override;
    void visit(TypeDefinition& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ProcedureDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    
    void visit(Program& node) override;
    
private:
    std::shared_ptr<SymbolTable> symbolTable_;
    std::ostringstream output_;
    int indentLevel_;
    std::string currentFunction_;
    
    // Helper methods
    void emit(const std::string& code);
    void emitLine(const std::string& line);
    void emitIndent();
    void increaseIndent();
    void decreaseIndent();
    
    // Code generation helpers
    std::string generateHeaders();
    std::string generateRuntimeIncludes();
    std::string generateForwardDeclarations(const std::vector<std::unique_ptr<Declaration>>& declarations);
    std::string mapPascalOperatorToCpp(TokenType operator_);
    std::string mapPascalTypeToCpp(const std::string& pascalType);
    std::string mapPascalFunctionToCpp(const std::string& functionName);
    void generateRecordDefinition(const std::string& typeName, const std::string& definition);
    void generateArrayDefinition(const std::string& typeName, const std::string& definition);
    void generateRangeDefinition(const std::string& typeName, const std::string& definition);
    void generateEnumDefinition(const std::string& typeName, const std::string& definition);
    
    // Expression and statement helpers
    void generateExpression(Expression* expr);
    void generateStatement(Statement* stmt);
    void generateFunctionCall(CallExpression& node);
    void generateBuiltinCall(CallExpression& node, const std::string& functionName);
    
    // Variable and function management
    std::string generateVariableDeclaration(const std::string& name, const std::string& type, Expression* initializer = nullptr);
    std::string generateParameterList(const std::vector<std::unique_ptr<VariableDeclaration>>& parameters);
    
    // Utility methods
    bool isBuiltinFunction(const std::string& name);
    std::string escapeCppString(const std::string& str);
};

} // namespace rpascal