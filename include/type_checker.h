#pragma once

#include "ast.h"
#include "symbol_table.h"
#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "unit_loader.h"
#include <memory>
#include <vector>
#include <string>

namespace rpascal {

// Semantic analyzer that performs type checking and symbol resolution
class SemanticAnalyzer : public ASTVisitor {
public:
    explicit SemanticAnalyzer(std::shared_ptr<SymbolTable> symbolTable);
    
    // Analyze the entire program
    bool analyze(Program& program);
    
    // Error handling
    bool hasErrors() const;
    const std::vector<std::string>& getErrors() const;
    
    // Get symbol table for further use
    std::shared_ptr<SymbolTable> getSymbolTable() const { return symbolTable_; }
    
    // Get unit loader for further use
    UnitLoader* getUnitLoader() const { return unitLoader_.get(); }
    
    // Visitor pattern implementation
    void visit(LiteralExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AddressOfExpression& node) override;
    void visit(DereferenceExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(FieldAccessExpression& node) override;
    void visit(ArrayIndexExpression& node) override;
    void visit(SetLiteralExpression& node) override;
    
    void visit(ExpressionStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(WithStatement& node) override;
    
    void visit(ConstantDeclaration& node) override;
    void visit(TypeDefinition& node) override;
    void visit(RecordTypeDefinition& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ProcedureDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    
    void visit(UsesClause& node) override;
    void visit(Unit& node) override;
    void visit(Program& node) override;
    
private:
    std::shared_ptr<SymbolTable> symbolTable_;
    std::vector<std::string> errors_;
    DataType currentExpressionType_;
    DataType currentPointeeType_;
    std::string currentFunctionName_; // For return value checking
    
    // With statement context tracking
    struct WithContext {
        std::string withVariable;    // Name of the with variable (e.g., "person")
        std::string recordTypeName;  // Name of the record type
        DataType recordType;         // Type of the with variable
    };
    std::vector<WithContext> withContextStack_;
    
    // Unit loading system
    std::unique_ptr<UnitLoader> unitLoader_;
    
    // Helper methods
    void addError(const std::string& message);
    DataType getExpressionType(Expression* expr);
    bool areTypesCompatible(DataType left, DataType right);
    bool areArgumentTypesCompatible(DataType expectedType, DataType actualType, Expression* actualExpr);
    bool isBoundedStringType(DataType type);
    DataType getResultType(DataType left, DataType right, TokenType operator_);
    bool isValidUnaryOperation(DataType operandType, TokenType operator_);
    bool isValidBinaryOperation(DataType leftType, DataType rightType, TokenType operator_);
    void checkFunctionCall(CallExpression& node);
    void checkAssignment(Expression* target, Expression* value);
    bool isFieldInRecordDefinition(const std::string& fieldName, const std::string& recordDef);
    std::string getFieldTypeFromRecord(const std::string& fieldName, const std::string& recordDef);
};

} // namespace rpascal