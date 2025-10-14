#include "../include/symbol_table.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace rpascal {

// Symbol implementation
std::string Symbol::toString() const {
    std::ostringstream oss;
    oss << name_ << " (";
    
    switch (symbolType_) {
        case SymbolType::VARIABLE: oss << "VAR"; break;
        case SymbolType::PROCEDURE: oss << "PROC"; break;
        case SymbolType::FUNCTION: oss << "FUNC"; break;
        case SymbolType::PARAMETER: oss << "PARAM"; break;
        case SymbolType::CONSTANT: oss << "CONST"; break;
        case SymbolType::TYPE_DEF: oss << "TYPE"; break;
    }
    
    oss << ", " << SymbolTable::dataTypeToString(dataType_);
    
    if (symbolType_ == SymbolType::FUNCTION || symbolType_ == SymbolType::PROCEDURE) {
        oss << "(";
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << parameters_[i].first << ": " << SymbolTable::dataTypeToString(parameters_[i].second);
        }
        oss << ")";
        
        if (symbolType_ == SymbolType::FUNCTION) {
            oss << " -> " << SymbolTable::dataTypeToString(returnType_);
        }
    }
    
    oss << ", scope=" << scopeLevel_ << ")";
    return oss.str();
}

// Scope implementation
void Scope::define(const std::string& name, std::shared_ptr<Symbol> symbol) {
    symbols_[name] = symbol;
}

std::shared_ptr<Symbol> Scope::lookup(const std::string& name) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    
    // Look in parent scope
    if (parent_) {
        return parent_->lookup(name);
    }
    
    return nullptr;
}

std::shared_ptr<Symbol> Scope::lookupLocal(const std::string& name) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    return nullptr;
}

// SymbolTable implementation
SymbolTable::SymbolTable() {
    // Create global scope
    scopes_.push_back(std::make_unique<Scope>(0));
    currentScope_ = scopes_[0].get();
    
    // Initialize built-in symbols
    initializeBuiltinSymbols();
}

void SymbolTable::enterScope() {
    int newLevel = currentScope_->getLevel() + 1;
    scopes_.push_back(std::make_unique<Scope>(newLevel, currentScope_));
    currentScope_ = scopes_.back().get();
}

void SymbolTable::exitScope() {
    if (currentScope_->getParent()) {
        currentScope_ = currentScope_->getParent();
        // Remove the last scope
        scopes_.pop_back();
    }
}

int SymbolTable::getCurrentScopeLevel() const {
    return currentScope_->getLevel();
}

void SymbolTable::define(const std::string& name, SymbolType symbolType, DataType dataType) {
    auto symbol = std::make_shared<Symbol>(name, symbolType, dataType, getCurrentScopeLevel());
    define(name, symbol);
}

void SymbolTable::define(const std::string& name, std::shared_ptr<Symbol> symbol) {
    // Check if symbol already exists in current scope
    if (currentScope_->lookupLocal(name)) {
        addError("Symbol '" + name + "' already defined in current scope");
        return;
    }
    
    currentScope_->define(name, symbol);
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string& name) {
    return currentScope_->lookup(name);
}

std::shared_ptr<Symbol> SymbolTable::lookupLocal(const std::string& name) {
    return currentScope_->lookupLocal(name);
}

DataType SymbolTable::stringToDataType(const std::string& typeStr) {
    std::string lowerType = typeStr;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    if (lowerType == "integer") return DataType::INTEGER;
    if (lowerType == "real") return DataType::REAL;
    if (lowerType == "boolean") return DataType::BOOLEAN;
    if (lowerType == "char") return DataType::CHAR;
    if (lowerType == "string") return DataType::STRING;
    if (lowerType == "void") return DataType::VOID;
    
    return DataType::UNKNOWN;
}

DataType SymbolTable::resolveDataType(const std::string& typeStr) {
    // First check for built-in types
    DataType builtinType = stringToDataType(typeStr);
    if (builtinType != DataType::UNKNOWN) {
        return builtinType;
    }
    
    // Check for custom type definitions
    auto symbol = lookup(typeStr);
    if (symbol && symbol->getSymbolType() == SymbolType::TYPE_DEF) {
        return DataType::CUSTOM;
    }
    
    return DataType::UNKNOWN;
}

std::string SymbolTable::dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INTEGER: return "integer";
        case DataType::REAL: return "real";
        case DataType::BOOLEAN: return "boolean";
        case DataType::CHAR: return "char";
        case DataType::STRING: return "string";
        case DataType::VOID: return "void";
        case DataType::CUSTOM: return "custom";
        case DataType::UNKNOWN: return "unknown";
        default: return "unknown";
    }
}

std::string SymbolTable::dataTypeToCppType(DataType type) {
    switch (type) {
        case DataType::INTEGER: return "int32_t";
        case DataType::REAL: return "double";
        case DataType::BOOLEAN: return "bool";
        case DataType::CHAR: return "char";
        case DataType::STRING: return "std::string";
        case DataType::VOID: return "void";
        case DataType::CUSTOM: return "auto";  // For now, let C++ compiler deduce custom types
        case DataType::UNKNOWN: return "auto";
        default: return "auto";
    }
}

void SymbolTable::addError(const std::string& error) {
    errors_.push_back(error);
}

void SymbolTable::printCurrentScope() const {
    std::cout << "=== Current Scope (Level " << currentScope_->getLevel() << ") ===\n";
    for (const auto& pair : currentScope_->getSymbols()) {
        std::cout << "  " << pair.second->toString() << "\n";
    }
    std::cout << "=== End Scope ===\n";
}

void SymbolTable::printAllScopes() const {
    std::cout << "=== All Scopes ===\n";
    for (const auto& scope : scopes_) {
        std::cout << "Scope Level " << scope->getLevel() << ":\n";
        for (const auto& pair : scope->getSymbols()) {
            std::cout << "  " << pair.second->toString() << "\n";
        }
    }
    std::cout << "=== End All Scopes ===\n";
}

void SymbolTable::initializeBuiltinSymbols() {
    // Built-in procedures
    auto writeln = std::make_shared<Symbol>("writeln", SymbolType::PROCEDURE, DataType::VOID, 0);
    // writeln can take variable arguments, we'll handle this specially in semantic analysis
    define("writeln", writeln);
    
    auto readln = std::make_shared<Symbol>("readln", SymbolType::PROCEDURE, DataType::VOID, 0);
    define("readln", readln);
    
    // Built-in functions
    auto length = std::make_shared<Symbol>("length", SymbolType::FUNCTION, DataType::INTEGER, 0);
    length->addParameter("s", DataType::STRING);
    length->setReturnType(DataType::INTEGER);
    define("length", length);
    
    auto chr = std::make_shared<Symbol>("chr", SymbolType::FUNCTION, DataType::CHAR, 0);
    chr->addParameter("code", DataType::INTEGER);
    chr->setReturnType(DataType::CHAR);
    define("chr", chr);
    
    auto ord = std::make_shared<Symbol>("ord", SymbolType::FUNCTION, DataType::INTEGER, 0);
    ord->addParameter("c", DataType::CHAR);
    ord->setReturnType(DataType::INTEGER);
    define("ord", ord);
}

} // namespace rpascal