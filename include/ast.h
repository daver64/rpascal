#pragma once

#include "token.h"
#include <memory>
#include <vector>
#include <string>

namespace rpascal {

// Forward declarations
class ASTVisitor;

// Parameter passing modes
enum class ParameterMode {
    VALUE,      // Normal parameter (by value)
    VAR,        // var parameter (by reference)
    CONST       // const parameter (by const reference)
};

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual std::string toString() const = 0;
    
    SourceLocation getLocation() const { return location_; }
    void setLocation(const SourceLocation& loc) { location_ = loc; }
    
protected:
    SourceLocation location_;
};

// Expression nodes
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

class Declaration : public ASTNode {
public:
    virtual ~Declaration() = default;
};

// Specific Expression types
class LiteralExpression : public Expression {
public:
    explicit LiteralExpression(const Token& token) : token_(token) {}
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const Token& getToken() const { return token_; }
    
private:
    Token token_;
};

class IdentifierExpression : public Expression {
public:
    explicit IdentifierExpression(const std::string& name) : name_(name) {}
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    
    // With statement support
    void setWithVariable(const std::string& withVar) { withVariable_ = withVar; }
    const std::string& getWithVariable() const { return withVariable_; }
    bool isWithFieldAccess() const { return !withVariable_.empty(); }
    
private:
    std::string name_;
    std::string withVariable_; // If set, this identifier is a field of this with variable
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<Expression> left, const Token& op, std::unique_ptr<Expression> right)
        : left_(std::move(left)), operator_(op), right_(std::move(right)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getLeft() const { return left_.get(); }
    Expression* getRight() const { return right_.get(); }
    const Token& getOperator() const { return operator_; }
    
private:
    std::unique_ptr<Expression> left_;
    Token operator_;
    std::unique_ptr<Expression> right_;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(const Token& op, std::unique_ptr<Expression> operand)
        : operator_(op), operand_(std::move(operand)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getOperand() const { return operand_.get(); }
    const Token& getOperator() const { return operator_; }
    
private:
    Token operator_;
    std::unique_ptr<Expression> operand_;
};

class AddressOfExpression : public Expression {
public:
    explicit AddressOfExpression(std::unique_ptr<Expression> operand)
        : operand_(std::move(operand)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getOperand() const { return operand_.get(); }
    
private:
    std::unique_ptr<Expression> operand_;
};

class DereferenceExpression : public Expression {
public:
    explicit DereferenceExpression(std::unique_ptr<Expression> operand)
        : operand_(std::move(operand)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getOperand() const { return operand_.get(); }
    
private:
    std::unique_ptr<Expression> operand_;
};

class CallExpression : public Expression {
public:
    CallExpression(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> arguments)
        : callee_(std::move(callee)), arguments_(std::move(arguments)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getCallee() const { return callee_.get(); }
    const std::vector<std::unique_ptr<Expression>>& getArguments() const { return arguments_; }
    
private:
    std::unique_ptr<Expression> callee_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

class FieldAccessExpression : public Expression {
public:
    FieldAccessExpression(std::unique_ptr<Expression> object, const std::string& fieldName)
        : object_(std::move(object)), fieldName_(fieldName) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getObject() const { return object_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    
private:
    std::unique_ptr<Expression> object_;
    std::string fieldName_;
};

class ArrayIndexExpression : public Expression {
public:
    ArrayIndexExpression(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index)
        : array_(std::move(array)) {
        indices_.push_back(std::move(index));
    }
    
    ArrayIndexExpression(std::unique_ptr<Expression> array, std::vector<std::unique_ptr<Expression>> indices)
        : array_(std::move(array)), indices_(std::move(indices)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getArray() const { return array_.get(); }
    Expression* getIndex() const { return indices_.empty() ? nullptr : indices_[0].get(); }
    const std::vector<std::unique_ptr<Expression>>& getIndices() const { return indices_; }
    
private:
    std::unique_ptr<Expression> array_;
    std::vector<std::unique_ptr<Expression>> indices_;
};

class SetLiteralExpression : public Expression {
public:
    SetLiteralExpression(std::vector<std::unique_ptr<Expression>> elements)
        : elements_(std::move(elements)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::vector<std::unique_ptr<Expression>>& getElements() const { return elements_; }
    
private:
    std::vector<std::unique_ptr<Expression>> elements_;
};

// Statement types
class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expression)
        : expression_(std::move(expression)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getExpression() const { return expression_.get(); }
    
private:
    std::unique_ptr<Expression> expression_;
};

class CompoundStatement : public Statement {
public:
    explicit CompoundStatement(std::vector<std::unique_ptr<Statement>> statements)
        : statements_(std::move(statements)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::vector<std::unique_ptr<Statement>>& getStatements() const { return statements_; }
    
private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class AssignmentStatement : public Statement {
public:
    AssignmentStatement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value)
        : target_(std::move(target)), value_(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getTarget() const { return target_.get(); }
    Expression* getValue() const { return value_.get(); }
    
private:
    std::unique_ptr<Expression> target_;
    std::unique_ptr<Expression> value_;
};

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition, 
                std::unique_ptr<Statement> thenStmt,
                std::unique_ptr<Statement> elseStmt = nullptr)
        : condition_(std::move(condition)), 
          thenStatement_(std::move(thenStmt)), 
          elseStatement_(std::move(elseStmt)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getCondition() const { return condition_.get(); }
    Statement* getThenStatement() const { return thenStatement_.get(); }
    Statement* getElseStatement() const { return elseStatement_.get(); }
    
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> thenStatement_;
    std::unique_ptr<Statement> elseStatement_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
        : condition_(std::move(condition)), body_(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getCondition() const { return condition_.get(); }
    Statement* getBody() const { return body_.get(); }
    
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};

class ForStatement : public Statement {
public:
    ForStatement(const std::string& variable, std::unique_ptr<Expression> start, 
                std::unique_ptr<Expression> end, bool isDownto, std::unique_ptr<Statement> body)
        : variable_(variable), start_(std::move(start)), end_(std::move(end)), 
          isDownto_(isDownto), body_(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getVariable() const { return variable_; }
    Expression* getStart() const { return start_.get(); }
    Expression* getEnd() const { return end_.get(); }
    bool isDownto() const { return isDownto_; }
    Statement* getBody() const { return body_.get(); }
    
private:
    std::string variable_;
    std::unique_ptr<Expression> start_;
    std::unique_ptr<Expression> end_;
    bool isDownto_;
    std::unique_ptr<Statement> body_;
};

class RepeatStatement : public Statement {
public:
    RepeatStatement(std::unique_ptr<Statement> body, std::unique_ptr<Expression> condition)
        : body_(std::move(body)), condition_(std::move(condition)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Statement* getBody() const { return body_.get(); }
    Expression* getCondition() const { return condition_.get(); }
    
private:
    std::unique_ptr<Statement> body_;
    std::unique_ptr<Expression> condition_;
};

class CaseBranch {
public:
    CaseBranch(std::vector<std::unique_ptr<Expression>> values, std::unique_ptr<Statement> statement)
        : values_(std::move(values)), statement_(std::move(statement)) {}
    
    const std::vector<std::unique_ptr<Expression>>& getValues() const { return values_; }
    Statement* getStatement() const { return statement_.get(); }
    
private:
    std::vector<std::unique_ptr<Expression>> values_;
    std::unique_ptr<Statement> statement_;
};

class CaseStatement : public Statement {
public:
    CaseStatement(std::unique_ptr<Expression> expression, 
                 std::vector<std::unique_ptr<CaseBranch>> branches,
                 std::unique_ptr<Statement> elseClause = nullptr)
        : expression_(std::move(expression)), branches_(std::move(branches)), 
          elseClause_(std::move(elseClause)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    Expression* getExpression() const { return expression_.get(); }
    const std::vector<std::unique_ptr<CaseBranch>>& getBranches() const { return branches_; }
    Statement* getElseClause() const { return elseClause_.get(); }
    
private:
    std::unique_ptr<Expression> expression_;
    std::vector<std::unique_ptr<CaseBranch>> branches_;
    std::unique_ptr<Statement> elseClause_;
};

class WithStatement : public Statement {
public:
    WithStatement(std::vector<std::unique_ptr<Expression>> withExpressions, 
                 std::unique_ptr<Statement> body)
        : withExpressions_(std::move(withExpressions)), body_(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::vector<std::unique_ptr<Expression>>& getWithExpressions() const { return withExpressions_; }
    Statement* getBody() const { return body_.get(); }
    
private:
    std::vector<std::unique_ptr<Expression>> withExpressions_;
    std::unique_ptr<Statement> body_;
};

// Array Type representation
class ArrayType {
public:
    ArrayType(std::unique_ptr<Expression> startIndex, std::unique_ptr<Expression> endIndex, const std::string& elementType)
        : startIndex_(std::move(startIndex)), endIndex_(std::move(endIndex)), elementType_(elementType) {}
    
    Expression* getStartIndex() const { return startIndex_.get(); }
    Expression* getEndIndex() const { return endIndex_.get(); }
    const std::string& getElementType() const { return elementType_; }
    
    // Calculate size for C++ array
    int calculateSize() const;
    
    // Get the starting offset for indexing conversion
    int getStartOffset() const;
    
private:
    std::unique_ptr<Expression> startIndex_;
    std::unique_ptr<Expression> endIndex_;
    std::string elementType_;
};

// Declaration types
class ConstantDeclaration : public Declaration {
public:
    ConstantDeclaration(const std::string& name, std::unique_ptr<Expression> value)
        : name_(name), value_(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    Expression* getValue() const { return value_.get(); }
    
private:
    std::string name_;
    std::unique_ptr<Expression> value_;
};

class TypeDefinition : public Declaration {
public:
    TypeDefinition(const std::string& name, const std::string& definition)
        : name_(name), definition_(definition) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    const std::string& getDefinition() const { return definition_; }
    
private:
    std::string name_;
    std::string definition_;
};

class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(const std::string& name, const std::string& type, 
                       std::unique_ptr<Expression> initializer = nullptr,
                       ParameterMode paramMode = ParameterMode::VALUE)
        : name_(name), type_(type), initializer_(std::move(initializer)), parameterMode_(paramMode) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    const std::string& getType() const { return type_; }
    Expression* getInitializer() const { return initializer_.get(); }
    ParameterMode getParameterMode() const { return parameterMode_; }
    void setParameterMode(ParameterMode mode) { parameterMode_ = mode; }
    
private:
    std::string name_;
    std::string type_;
    std::unique_ptr<Expression> initializer_;
    ParameterMode parameterMode_;
};

class ProcedureDeclaration : public Declaration {
public:
    ProcedureDeclaration(const std::string& name,
                        std::vector<std::unique_ptr<VariableDeclaration>> parameters,
                        std::vector<std::unique_ptr<VariableDeclaration>> localVariables,
                        std::unique_ptr<CompoundStatement> body)
        : name_(name), parameters_(std::move(parameters)), 
          localVariables_(std::move(localVariables)), body_(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<VariableDeclaration>>& getParameters() const { return parameters_; }
    const std::vector<std::unique_ptr<VariableDeclaration>>& getLocalVariables() const { return localVariables_; }
    CompoundStatement* getBody() const { return body_.get(); }
    
private:
    std::string name_;
    std::vector<std::unique_ptr<VariableDeclaration>> parameters_;
    std::vector<std::unique_ptr<VariableDeclaration>> localVariables_;
    std::unique_ptr<CompoundStatement> body_;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(const std::string& name,
                       std::vector<std::unique_ptr<VariableDeclaration>> parameters,
                       const std::string& returnType,
                       std::vector<std::unique_ptr<VariableDeclaration>> localVariables,
                       std::unique_ptr<CompoundStatement> body)
        : name_(name), parameters_(std::move(parameters)), 
          returnType_(returnType), localVariables_(std::move(localVariables)), body_(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<VariableDeclaration>>& getParameters() const { return parameters_; }
    const std::string& getReturnType() const { return returnType_; }
    const std::vector<std::unique_ptr<VariableDeclaration>>& getLocalVariables() const { return localVariables_; }
    CompoundStatement* getBody() const { return body_.get(); }
    
private:
    std::string name_;
    std::vector<std::unique_ptr<VariableDeclaration>> parameters_;
    std::string returnType_;
    std::vector<std::unique_ptr<VariableDeclaration>> localVariables_;
    std::unique_ptr<CompoundStatement> body_;
};

// Program node (root of AST)
class Program : public ASTNode {
public:
    Program(const std::string& name, std::vector<std::unique_ptr<Declaration>> declarations,
            std::unique_ptr<CompoundStatement> mainBlock)
        : name_(name), declarations_(std::move(declarations)), mainBlock_(std::move(mainBlock)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
    
    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const { return declarations_; }
    CompoundStatement* getMainBlock() const { return mainBlock_.get(); }
    
private:
    std::string name_;
    std::vector<std::unique_ptr<Declaration>> declarations_;
    std::unique_ptr<CompoundStatement> mainBlock_;
};

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visit(LiteralExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(AddressOfExpression& node) = 0;
    virtual void visit(DereferenceExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(FieldAccessExpression& node) = 0;
    virtual void visit(ArrayIndexExpression& node) = 0;
    virtual void visit(SetLiteralExpression& node) = 0;
    
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(CompoundStatement& node) = 0;
    virtual void visit(AssignmentStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(RepeatStatement& node) = 0;
    virtual void visit(CaseStatement& node) = 0;
    virtual void visit(WithStatement& node) = 0;
    
    virtual void visit(ConstantDeclaration& node) = 0;
    virtual void visit(TypeDefinition& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ProcedureDeclaration& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    
    virtual void visit(Program& node) = 0;
};

} // namespace rpascal