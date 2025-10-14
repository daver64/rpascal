#include "../include/ast.h"
#include <sstream>

namespace rpascal {

// LiteralExpression
void LiteralExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string LiteralExpression::toString() const {
    return "LiteralExpression(" + token_.getValue() + ")";
}

// IdentifierExpression
void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string IdentifierExpression::toString() const {
    return "IdentifierExpression(" + name_ + ")";
}

// BinaryExpression
void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string BinaryExpression::toString() const {
    return "BinaryExpression(" + left_->toString() + " " + 
           operator_.getValue() + " " + right_->toString() + ")";
}

// UnaryExpression
void UnaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string UnaryExpression::toString() const {
    return "UnaryExpression(" + operator_.getValue() + " " + operand_->toString() + ")";
}

// CallExpression
void CallExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string CallExpression::toString() const {
    std::ostringstream oss;
    oss << "CallExpression(" << callee_->toString() << "(";
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << arguments_[i]->toString();
    }
    oss << "))";
    return oss.str();
}

// FieldAccessExpression
void FieldAccessExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string FieldAccessExpression::toString() const {
    return "FieldAccessExpression(" + object_->toString() + "." + fieldName_ + ")";
}

// ArrayIndexExpression
void ArrayIndexExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ArrayIndexExpression::toString() const {
    return "ArrayIndexExpression(" + array_->toString() + "[" + index_->toString() + "])";
}

// ExpressionStatement
void ExpressionStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ExpressionStatement::toString() const {
    return "ExpressionStatement(" + expression_->toString() + ")";
}

// CompoundStatement
void CompoundStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string CompoundStatement::toString() const {
    std::ostringstream oss;
    oss << "CompoundStatement(";
    for (size_t i = 0; i < statements_.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << statements_[i]->toString();
    }
    oss << ")";
    return oss.str();
}

// AssignmentStatement
void AssignmentStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string AssignmentStatement::toString() const {
    return "AssignmentStatement(" + target_->toString() + " := " + value_->toString() + ")";
}

// IfStatement
void IfStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string IfStatement::toString() const {
    std::string result = "IfStatement(" + condition_->toString() + " then " + thenStatement_->toString();
    if (elseStatement_) {
        result += " else " + elseStatement_->toString();
    }
    result += ")";
    return result;
}

// WhileStatement
void WhileStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string WhileStatement::toString() const {
    return "WhileStatement(" + condition_->toString() + " do " + body_->toString() + ")";
}

// ForStatement
void ForStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ForStatement::toString() const {
    return "ForStatement(" + variable_ + " := " + start_->toString() + 
           (isDownto_ ? " downto " : " to ") + end_->toString() + 
           " do " + body_->toString() + ")";
}

// RepeatStatement
void RepeatStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string RepeatStatement::toString() const {
    return "RepeatStatement(repeat " + body_->toString() + " until " + 
           condition_->toString() + ")";
}

// CaseStatement
void CaseStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string CaseStatement::toString() const {
    std::string result = "CaseStatement(" + expression_->toString() + " of ";
    for (size_t i = 0; i < branches_.size(); ++i) {
        if (i > 0) result += "; ";
        const auto& branch = branches_[i];
        for (size_t j = 0; j < branch->getValues().size(); ++j) {
            if (j > 0) result += ", ";
            result += branch->getValues()[j]->toString();
        }
        result += ": " + branch->getStatement()->toString();
    }
    if (elseClause_) {
        result += " else " + elseClause_->toString();
    }
    result += ")";
    return result;
}

// ConstantDeclaration
void ConstantDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ConstantDeclaration::toString() const {
    return "ConstantDeclaration(" + name_ + " = " + value_->toString() + ")";
}

// TypeDefinition
void TypeDefinition::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string TypeDefinition::toString() const {
    return "TypeDefinition(" + name_ + " = " + definition_ + ")";
}

// VariableDeclaration
void VariableDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string VariableDeclaration::toString() const {
    std::string result = "VariableDeclaration(" + name_ + ": " + type_;
    if (initializer_) {
        result += " = " + initializer_->toString();
    }
    result += ")";
    return result;
}

// ProcedureDeclaration
void ProcedureDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ProcedureDeclaration::toString() const {
    std::ostringstream oss;
    oss << "ProcedureDeclaration(" << name_ << "(";
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters_[i]->toString();
    }
    oss << ")";
    if (!localVariables_.empty()) {
        oss << " var ";
        for (size_t i = 0; i < localVariables_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << localVariables_[i]->toString();
        }
    }
    oss << " " << body_->toString() << ")";
    return oss.str();
}

// FunctionDeclaration
void FunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string FunctionDeclaration::toString() const {
    std::ostringstream oss;
    oss << "FunctionDeclaration(" << name_ << "(";
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters_[i]->toString();
    }
    oss << "): " << returnType_;
    if (!localVariables_.empty()) {
        oss << " var ";
        for (size_t i = 0; i < localVariables_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << localVariables_[i]->toString();
        }
    }
    oss << " " << body_->toString() << ")";
    return oss.str();
}

// Program
void Program::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string Program::toString() const {
    std::ostringstream oss;
    oss << "Program(" << name_ << " [";
    for (size_t i = 0; i < declarations_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << declarations_[i]->toString();
    }
    oss << "] " << mainBlock_->toString() << ")";
    return oss.str();
}

// ArrayType
int ArrayType::calculateSize() const {
    // For now, handle simple integer ranges
    if (auto startLit = dynamic_cast<LiteralExpression*>(startIndex_.get())) {
        if (auto endLit = dynamic_cast<LiteralExpression*>(endIndex_.get())) {
            try {
                int start = std::stoi(startLit->getToken().getValue());
                int end = std::stoi(endLit->getToken().getValue());
                return end - start + 1;
            } catch (const std::exception&) {
                // Non-numeric bounds
                return 1; // Default size
            }
        }
    }
    return 1; // Default size for unknown bounds
}

int ArrayType::getStartOffset() const {
    // For now, handle simple integer ranges
    if (auto startLit = dynamic_cast<LiteralExpression*>(startIndex_.get())) {
        try {
            return std::stoi(startLit->getToken().getValue());
        } catch (const std::exception&) {
            // Non-numeric start
            return 0; // Default to 0-based
        }
    }
    return 0; // Default to 0-based
}

} // namespace rpascal