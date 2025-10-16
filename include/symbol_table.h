#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace rpascal {

// Symbol types
enum class SymbolType {
    VARIABLE,
    PROCEDURE,
    FUNCTION,
    PARAMETER,
    CONSTANT,
    TYPE_DEF
};

// Data types
enum class DataType {
    INTEGER,
    REAL,
    BOOLEAN,
    CHAR,
    BYTE,
    STRING,
    VOID,
    CUSTOM,    // User-defined types (records, arrays, enums)
    POINTER,   // Pointer types (^T)
    FILE_TYPE, // File types (text, typed files)
    UNKNOWN
};

// Symbol information
class Symbol {
public:
    Symbol(const std::string& name, SymbolType symbolType, DataType dataType, int scopeLevel = 0)
        : name_(name), symbolType_(symbolType), dataType_(dataType), scopeLevel_(scopeLevel) {}
    
    const std::string& getName() const { return name_; }
    SymbolType getSymbolType() const { return symbolType_; }
    DataType getDataType() const { return dataType_; }
    int getScopeLevel() const { return scopeLevel_; }
    
    // For type definitions
    void setTypeDefinition(const std::string& definition) { typeDefinition_ = definition; }
    const std::string& getTypeDefinition() const { return typeDefinition_; }
    
    // For variables with custom types
    void setTypeName(const std::string& typeName) { typeName_ = typeName; }
    const std::string& getTypeName() const { return typeName_; }
    
    // For pointer types
    void setPointeeType(DataType pointeeType) { pointeeType_ = pointeeType; }
    DataType getPointeeType() const { return pointeeType_; }
    void setPointeeTypeName(const std::string& pointeeName) { pointeeTypeName_ = pointeeName; }
    const std::string& getPointeeTypeName() const { return pointeeTypeName_; }
    
    // For functions and procedures
    void addParameter(const std::string& paramName, DataType paramType) {
        parameters_.push_back({paramName, paramType});
    }
    
    const std::vector<std::pair<std::string, DataType>>& getParameters() const {
        return parameters_;
    }
    
    void setReturnType(DataType returnType) { returnType_ = returnType; }
    DataType getReturnType() const { return returnType_; }
    
    // Function signature for overloading
    std::string getSignature() const;
    bool matchesSignature(const std::vector<DataType>& paramTypes) const;
    
    std::string toString() const;
    
private:
    std::string name_;
    SymbolType symbolType_;
    DataType dataType_;
    int scopeLevel_;
    std::string typeDefinition_;  // For custom types, stores the definition string
    std::string typeName_;        // For variables, stores the original type name  
    DataType pointeeType_ = DataType::UNKNOWN;  // For pointer types, the pointed-to type
    std::string pointeeTypeName_; // For pointer types, the pointed-to type name
    std::vector<std::pair<std::string, DataType>> parameters_;
    DataType returnType_ = DataType::VOID;
};

// Scope for managing nested scopes
class Scope {
public:
    explicit Scope(int level, Scope* parent = nullptr) 
        : level_(level), parent_(parent) {}
    
    void define(const std::string& name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup(const std::string& name);
    std::shared_ptr<Symbol> lookupLocal(const std::string& name);
    
    // Overloaded function support
    void defineOverloaded(const std::string& name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookupFunction(const std::string& name, const std::vector<DataType>& paramTypes);
    std::vector<std::shared_ptr<Symbol>> lookupAllOverloads(const std::string& name);
    
    int getLevel() const { return level_; }
    Scope* getParent() const { return parent_; }
    
    const std::unordered_map<std::string, std::shared_ptr<Symbol>>& getSymbols() const {
        return symbols_;
    }
    
private:
    int level_;
    Scope* parent_;
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Symbol>>> overloadedSymbols_;
};

// Main symbol table
class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;
    
    // Scope management
    void enterScope();
    void exitScope();
    int getCurrentScopeLevel() const;
    
    // Symbol operations
    void define(const std::string& name, SymbolType symbolType, DataType dataType);
    void define(const std::string& name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup(const std::string& name);
    std::shared_ptr<Symbol> lookupLocal(const std::string& name);
    
    // Overloaded function support
    void defineOverloaded(const std::string& name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookupFunction(const std::string& name, const std::vector<DataType>& paramTypes);
    std::vector<std::shared_ptr<Symbol>> lookupAllOverloads(const std::string& name);
    
    // Type utilities
    static DataType stringToDataType(const std::string& typeStr);
    DataType resolveDataType(const std::string& typeStr);  // Instance method that can check custom types
    static std::string dataTypeToString(DataType type);
    static std::string dataTypeToCppType(DataType type);
    
    // Error handling
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    void addError(const std::string& error);
    
    // Debugging
    void printCurrentScope() const;
    void printAllScopes() const;
    
private:
    std::vector<std::unique_ptr<Scope>> scopes_;
    Scope* currentScope_;
    std::vector<std::string> errors_;
    
    void initializeBuiltinSymbols();
};

} // namespace rpascal