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

std::string Symbol::getSignature() const {
    std::ostringstream oss;
    oss << name_ << "(";
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (i > 0) oss << ",";
        oss << SymbolTable::dataTypeToString(parameters_[i].second);
    }
    oss << ")";
    return oss.str();
}

bool Symbol::matchesSignature(const std::vector<DataType>& paramTypes) const {
    if (parameters_.size() != paramTypes.size()) {
        return false;
    }
    
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i].second != paramTypes[i]) {
            return false;
        }
    }
    
    return true;
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

void Scope::defineOverloaded(const std::string& name, std::shared_ptr<Symbol> symbol) {
    overloadedSymbols_[name].push_back(symbol);
}

std::shared_ptr<Symbol> Scope::lookupFunction(const std::string& name, const std::vector<DataType>& paramTypes) {
    // First check overloaded functions
    auto it = overloadedSymbols_.find(name);
    if (it != overloadedSymbols_.end()) {
        for (const auto& symbol : it->second) {
            if (symbol->matchesSignature(paramTypes)) {
                return symbol;
            }
        }
    }
    
    // Also check regular symbols for non-overloaded functions and procedures
    auto regularIt = symbols_.find(name);
    if (regularIt != symbols_.end()) {
        auto symbol = regularIt->second;
        if ((symbol->getSymbolType() == SymbolType::FUNCTION || symbol->getSymbolType() == SymbolType::PROCEDURE) 
            && symbol->matchesSignature(paramTypes)) {
            return symbol;
        }
    }
    
    // Check parent scope
    if (parent_) {
        return parent_->lookupFunction(name, paramTypes);
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<Symbol>> Scope::lookupAllOverloads(const std::string& name) {
    std::vector<std::shared_ptr<Symbol>> result;
    
    // Get overloads from this scope
    auto it = overloadedSymbols_.find(name);
    if (it != overloadedSymbols_.end()) {
        result = it->second;
    }
    
    // Add overloads from parent scopes
    if (parent_) {
        auto parentOverloads = parent_->lookupAllOverloads(name);
        result.insert(result.end(), parentOverloads.begin(), parentOverloads.end());
    }
    
    return result;
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

void SymbolTable::defineOverloaded(const std::string& name, std::shared_ptr<Symbol> symbol) {
    currentScope_->defineOverloaded(name, symbol);
}

std::shared_ptr<Symbol> SymbolTable::lookupFunction(const std::string& name, const std::vector<DataType>& paramTypes) {
    return currentScope_->lookupFunction(name, paramTypes);
}

std::vector<std::shared_ptr<Symbol>> SymbolTable::lookupAllOverloads(const std::string& name) {
    return currentScope_->lookupAllOverloads(name);
}

DataType SymbolTable::stringToDataType(const std::string& typeStr) {
    std::string lowerType = typeStr;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Handle pointer types: ^TypeName
    if (!lowerType.empty() && lowerType[0] == '^') {
        return DataType::POINTER;
    }
    
    // Handle file types: file, file of T
    if (lowerType == "file" || lowerType.find("file of") == 0) {
        return DataType::FILE_TYPE;
    }
    
    if (lowerType == "integer") return DataType::INTEGER;
    if (lowerType == "real") return DataType::REAL;
    if (lowerType == "boolean") return DataType::BOOLEAN;
    if (lowerType == "char") return DataType::CHAR;
    if (lowerType == "byte") return DataType::BYTE;
    if (lowerType == "string") return DataType::STRING;
    
    // Handle bounded strings: string[N]
    if (lowerType.find("string[") == 0 && lowerType.back() == ']') {
        return DataType::STRING;
    }
    
    if (lowerType == "void") return DataType::VOID;
    if (lowerType == "text") return DataType::FILE_TYPE;
    
    return DataType::UNKNOWN;
}

DataType SymbolTable::resolveDataType(const std::string& typeStr) {
    // First check for built-in types
    DataType builtinType = stringToDataType(typeStr);
    if (builtinType != DataType::UNKNOWN) {
        return builtinType;
    }
    
    // Check for array types: array[...] of Type
    if (typeStr.find("array") == 0 && typeStr.find(" of ") != std::string::npos) {
        return DataType::CUSTOM; // Arrays are treated as custom types
    }
    
    // Check for custom type definitions
    auto symbol = lookup(typeStr);
    if (symbol && symbol->getSymbolType() == SymbolType::TYPE_DEF) {
        // If it's a pointer type definition, return POINTER, not CUSTOM
        if (symbol->getDataType() == DataType::POINTER) {
            return DataType::POINTER;
        }
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
        case DataType::BYTE: return "byte";
        case DataType::STRING: return "string";
        case DataType::VOID: return "void";
        case DataType::CUSTOM: return "custom";
        case DataType::POINTER: return "pointer";
        case DataType::FILE_TYPE: return "file";
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
        case DataType::POINTER: return "void*";  // Generic pointer for now
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
    
    auto write = std::make_shared<Symbol>("write", SymbolType::PROCEDURE, DataType::VOID, 0);
    // write can take variable arguments, we'll handle this specially in semantic analysis
    define("write", write);
    
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
    
    // Additional string functions
    auto pos = std::make_shared<Symbol>("pos", SymbolType::FUNCTION, DataType::INTEGER, 0);
    pos->addParameter("substr", DataType::STRING);
    pos->addParameter("str", DataType::STRING);
    pos->setReturnType(DataType::INTEGER);
    define("pos", pos);
    
    auto copy = std::make_shared<Symbol>("copy", SymbolType::FUNCTION, DataType::STRING, 0);
    copy->addParameter("str", DataType::STRING);
    copy->addParameter("start", DataType::INTEGER);
    copy->addParameter("length", DataType::INTEGER);
    copy->setReturnType(DataType::STRING);
    define("copy", copy);
    
    auto concat = std::make_shared<Symbol>("concat", SymbolType::FUNCTION, DataType::STRING, 0);
    // concat can take variable arguments, add a few generic parameters
    concat->addParameter("str1", DataType::STRING);
    concat->addParameter("str2", DataType::STRING);
    concat->addParameter("str3", DataType::STRING); // optional
    concat->setReturnType(DataType::STRING);
    define("concat", concat);
    
    auto insert = std::make_shared<Symbol>("insert", SymbolType::PROCEDURE, DataType::VOID, 0);
    insert->addParameter("substr", DataType::STRING);
    insert->addParameter("str", DataType::STRING); // this would be var parameter in reality
    insert->addParameter("pos", DataType::INTEGER);
    define("insert", insert);
    
    auto deleteFunc = std::make_shared<Symbol>("delete", SymbolType::PROCEDURE, DataType::VOID, 0);
    deleteFunc->addParameter("str", DataType::STRING); // this would be var parameter in reality
    deleteFunc->addParameter("pos", DataType::INTEGER);
    deleteFunc->addParameter("length", DataType::INTEGER);
    define("delete", deleteFunc);
    
    // File operation functions
    auto assignFile = std::make_shared<Symbol>("assign", SymbolType::PROCEDURE, DataType::VOID, 0);
    assignFile->addParameter("f", DataType::FILE_TYPE); // var parameter
    assignFile->addParameter("filename", DataType::STRING);
    define("assign", assignFile);
    
    auto resetFile = std::make_shared<Symbol>("reset", SymbolType::PROCEDURE, DataType::VOID, 0);
    resetFile->addParameter("f", DataType::FILE_TYPE); // var parameter
    define("reset", resetFile);
    
    auto rewriteFile = std::make_shared<Symbol>("rewrite", SymbolType::PROCEDURE, DataType::VOID, 0);
    rewriteFile->addParameter("f", DataType::FILE_TYPE); // var parameter
    define("rewrite", rewriteFile);
    
    auto appendFile = std::make_shared<Symbol>("append", SymbolType::PROCEDURE, DataType::VOID, 0);
    appendFile->addParameter("f", DataType::FILE_TYPE); // var parameter
    define("append", appendFile);
    
    auto closeFile = std::make_shared<Symbol>("close", SymbolType::PROCEDURE, DataType::VOID, 0);
    closeFile->addParameter("f", DataType::FILE_TYPE); // var parameter
    define("close", closeFile);
    
    auto eofFunc = std::make_shared<Symbol>("eof", SymbolType::FUNCTION, DataType::BOOLEAN, 0);
    eofFunc->addParameter("f", DataType::FILE_TYPE);
    eofFunc->setReturnType(DataType::BOOLEAN);
    define("eof", eofFunc);
    
    auto ioresultFunc = std::make_shared<Symbol>("ioresult", SymbolType::FUNCTION, DataType::INTEGER, 0);
    ioresultFunc->setReturnType(DataType::INTEGER);
    define("ioresult", ioresultFunc);
    
    // File position and block operation functions
    auto blockreadFunc = std::make_shared<Symbol>("blockread", SymbolType::PROCEDURE, DataType::VOID, 0);
    blockreadFunc->addParameter("f", DataType::FILE_TYPE); // var parameter
    blockreadFunc->addParameter("buffer", DataType::POINTER); // var parameter (array)
    blockreadFunc->addParameter("count", DataType::INTEGER);
    blockreadFunc->addParameter("result", DataType::INTEGER); // var parameter
    define("blockread", blockreadFunc);
    
    auto blockwriteFunc = std::make_shared<Symbol>("blockwrite", SymbolType::PROCEDURE, DataType::VOID, 0);
    blockwriteFunc->addParameter("f", DataType::FILE_TYPE); // var parameter
    blockwriteFunc->addParameter("buffer", DataType::POINTER); // const parameter (array)
    blockwriteFunc->addParameter("count", DataType::INTEGER);
    blockwriteFunc->addParameter("result", DataType::INTEGER); // var parameter
    define("blockwrite", blockwriteFunc);
    
    auto fileposFunc = std::make_shared<Symbol>("filepos", SymbolType::FUNCTION, DataType::INTEGER, 0);
    fileposFunc->addParameter("f", DataType::FILE_TYPE);
    fileposFunc->setReturnType(DataType::INTEGER);
    define("filepos", fileposFunc);
    
    auto filesizeFunc = std::make_shared<Symbol>("filesize", SymbolType::FUNCTION, DataType::INTEGER, 0);
    filesizeFunc->addParameter("f", DataType::FILE_TYPE);
    filesizeFunc->setReturnType(DataType::INTEGER);
    define("filesize", filesizeFunc);
    
    auto seekFunc = std::make_shared<Symbol>("seek", SymbolType::PROCEDURE, DataType::VOID, 0);
    seekFunc->addParameter("f", DataType::FILE_TYPE); // var parameter
    seekFunc->addParameter("position", DataType::INTEGER);
    define("seek", seekFunc);
    
    // Pointer allocation functions
    auto newFunc = std::make_shared<Symbol>("new", SymbolType::PROCEDURE, DataType::VOID, 0);
    newFunc->addParameter("ptr", DataType::POINTER); // var parameter in reality
    define("new", newFunc);
    
    auto disposeFunc = std::make_shared<Symbol>("dispose", SymbolType::PROCEDURE, DataType::VOID, 0);
    disposeFunc->addParameter("ptr", DataType::POINTER); // var parameter in reality
    define("dispose", disposeFunc);
    
    // === SYSTEM UNIT FUNCTIONS ===
    
    // Mathematical functions
    auto absFunc = std::make_shared<Symbol>("abs", SymbolType::FUNCTION, DataType::INTEGER, 0);
    absFunc->addParameter("x", DataType::INTEGER);
    absFunc->setReturnType(DataType::INTEGER);
    define("abs", absFunc);
    
    auto sqrFunc = std::make_shared<Symbol>("sqr", SymbolType::FUNCTION, DataType::INTEGER, 0);
    sqrFunc->addParameter("x", DataType::INTEGER);
    sqrFunc->setReturnType(DataType::INTEGER);
    define("sqr", sqrFunc);
    
    auto sqrtFunc = std::make_shared<Symbol>("sqrt", SymbolType::FUNCTION, DataType::REAL, 0);
    sqrtFunc->addParameter("x", DataType::REAL);
    sqrtFunc->setReturnType(DataType::REAL);
    define("sqrt", sqrtFunc);
    
    auto sinFunc = std::make_shared<Symbol>("sin", SymbolType::FUNCTION, DataType::REAL, 0);
    sinFunc->addParameter("x", DataType::REAL);
    sinFunc->setReturnType(DataType::REAL);
    define("sin", sinFunc);
    
    auto cosFunc = std::make_shared<Symbol>("cos", SymbolType::FUNCTION, DataType::REAL, 0);
    cosFunc->addParameter("x", DataType::REAL);
    cosFunc->setReturnType(DataType::REAL);
    define("cos", cosFunc);
    
    auto arctanFunc = std::make_shared<Symbol>("arctan", SymbolType::FUNCTION, DataType::REAL, 0);
    arctanFunc->addParameter("x", DataType::REAL);
    arctanFunc->setReturnType(DataType::REAL);
    define("arctan", arctanFunc);
    
    auto lnFunc = std::make_shared<Symbol>("ln", SymbolType::FUNCTION, DataType::REAL, 0);
    lnFunc->addParameter("x", DataType::REAL);
    lnFunc->setReturnType(DataType::REAL);
    define("ln", lnFunc);
    
    auto expFunc = std::make_shared<Symbol>("exp", SymbolType::FUNCTION, DataType::REAL, 0);
    expFunc->addParameter("x", DataType::REAL);
    expFunc->setReturnType(DataType::REAL);
    define("exp", expFunc);
    
    // Conversion functions
    auto valFunc = std::make_shared<Symbol>("val", SymbolType::PROCEDURE, DataType::VOID, 0);
    valFunc->addParameter("s", DataType::STRING);
    valFunc->addParameter("result", DataType::INTEGER); // var parameter
    valFunc->addParameter("code", DataType::INTEGER);   // var parameter
    define("val", valFunc);
    
    auto strFunc = std::make_shared<Symbol>("str", SymbolType::PROCEDURE, DataType::VOID, 0);
    strFunc->addParameter("x", DataType::INTEGER);
    strFunc->addParameter("s", DataType::STRING); // var parameter
    define("str", strFunc);
    
    // String functions
    auto upcaseFunc = std::make_shared<Symbol>("upcase", SymbolType::FUNCTION, DataType::CHAR, 0);
    upcaseFunc->addParameter("c", DataType::CHAR);
    upcaseFunc->setReturnType(DataType::CHAR);
    define("upcase", upcaseFunc);
    
    // Additional string functions
    auto trimFunc = std::make_shared<Symbol>("trim", SymbolType::FUNCTION, DataType::STRING, 0);
    trimFunc->addParameter("str", DataType::STRING);
    trimFunc->setReturnType(DataType::STRING);
    define("trim", trimFunc);
    
    auto trimleftFunc = std::make_shared<Symbol>("trimleft", SymbolType::FUNCTION, DataType::STRING, 0);
    trimleftFunc->addParameter("str", DataType::STRING);
    trimleftFunc->setReturnType(DataType::STRING);
    define("trimleft", trimleftFunc);
    
    auto trimrightFunc = std::make_shared<Symbol>("trimright", SymbolType::FUNCTION, DataType::STRING, 0);
    trimrightFunc->addParameter("str", DataType::STRING);
    trimrightFunc->setReturnType(DataType::STRING);
    define("trimright", trimrightFunc);
    
    auto stringofcharFunc = std::make_shared<Symbol>("stringofchar", SymbolType::FUNCTION, DataType::STRING, 0);
    stringofcharFunc->addParameter("ch", DataType::CHAR);
    stringofcharFunc->addParameter("count", DataType::INTEGER);
    stringofcharFunc->setReturnType(DataType::STRING);
    define("stringofchar", stringofcharFunc);
    
    auto lowercaseFunc = std::make_shared<Symbol>("lowercase", SymbolType::FUNCTION, DataType::STRING, 0);
    lowercaseFunc->addParameter("str", DataType::STRING);
    lowercaseFunc->setReturnType(DataType::STRING);
    define("lowercase", lowercaseFunc);
    
    auto uppercaseFunc = std::make_shared<Symbol>("uppercase", SymbolType::FUNCTION, DataType::STRING, 0);
    uppercaseFunc->addParameter("str", DataType::STRING);
    uppercaseFunc->setReturnType(DataType::STRING);
    define("uppercase", uppercaseFunc);
    
    auto leftstrFunc = std::make_shared<Symbol>("leftstr", SymbolType::FUNCTION, DataType::STRING, 0);
    leftstrFunc->addParameter("str", DataType::STRING);
    leftstrFunc->addParameter("count", DataType::INTEGER);
    leftstrFunc->setReturnType(DataType::STRING);
    define("leftstr", leftstrFunc);
    
    auto rightstrFunc = std::make_shared<Symbol>("rightstr", SymbolType::FUNCTION, DataType::STRING, 0);
    rightstrFunc->addParameter("str", DataType::STRING);
    rightstrFunc->addParameter("count", DataType::INTEGER);
    rightstrFunc->setReturnType(DataType::STRING);
    define("rightstr", rightstrFunc);
    
    auto padleftFunc = std::make_shared<Symbol>("padleft", SymbolType::FUNCTION, DataType::STRING, 0);
    padleftFunc->addParameter("str", DataType::STRING);
    padleftFunc->addParameter("totalWidth", DataType::INTEGER);
    padleftFunc->addParameter("paddingChar", DataType::CHAR); // optional
    padleftFunc->setReturnType(DataType::STRING);
    define("padleft", padleftFunc);
    
    auto padrightFunc = std::make_shared<Symbol>("padright", SymbolType::FUNCTION, DataType::STRING, 0);
    padrightFunc->addParameter("str", DataType::STRING);
    padrightFunc->addParameter("totalWidth", DataType::INTEGER);
    padrightFunc->addParameter("paddingChar", DataType::CHAR); // optional
    padrightFunc->setReturnType(DataType::STRING);
    define("padright", padrightFunc);
    
    // I/O functions
    auto paramCountFunc = std::make_shared<Symbol>("paramcount", SymbolType::FUNCTION, DataType::INTEGER, 0);
    paramCountFunc->setReturnType(DataType::INTEGER);
    define("paramcount", paramCountFunc);
    
    auto paramStrFunc = std::make_shared<Symbol>("paramstr", SymbolType::FUNCTION, DataType::STRING, 0);
    paramStrFunc->addParameter("index", DataType::INTEGER);
    paramStrFunc->setReturnType(DataType::STRING);
    define("paramstr", paramStrFunc);
    
    // System functions
    auto haltFunc = std::make_shared<Symbol>("halt", SymbolType::PROCEDURE, DataType::VOID, 0);
    haltFunc->addParameter("exitcode", DataType::INTEGER); // optional parameter
    define("halt", haltFunc);
    
    auto exitFunc = std::make_shared<Symbol>("exit", SymbolType::PROCEDURE, DataType::VOID, 0);
    define("exit", exitFunc);
    
    auto randomFunc = std::make_shared<Symbol>("random", SymbolType::FUNCTION, DataType::REAL, 0);
    randomFunc->setReturnType(DataType::REAL);
    define("random", randomFunc);
    
    auto randomizeFunc = std::make_shared<Symbol>("randomize", SymbolType::PROCEDURE, DataType::VOID, 0);
    define("randomize", randomizeFunc);
    
    // === POINTER ARITHMETIC FUNCTIONS ===
    
    // Inc procedure - increment a variable
    auto incFunc = std::make_shared<Symbol>("inc", SymbolType::PROCEDURE, DataType::VOID, 0);
    incFunc->addParameter("var", DataType::INTEGER); // var parameter - can be integer or pointer
    incFunc->addParameter("amount", DataType::INTEGER); // optional amount parameter
    define("inc", incFunc);
    
    // Dec procedure - decrement a variable
    auto decFunc = std::make_shared<Symbol>("dec", SymbolType::PROCEDURE, DataType::VOID, 0);
    decFunc->addParameter("var", DataType::INTEGER); // var parameter - can be integer or pointer
    decFunc->addParameter("amount", DataType::INTEGER); // optional amount parameter
    define("dec", decFunc);
    
    // === DYNAMIC MEMORY ALLOCATION FUNCTIONS ===
    
    // GetMem procedure - allocate memory
    auto getmemFunc = std::make_shared<Symbol>("getmem", SymbolType::PROCEDURE, DataType::VOID, 0);
    getmemFunc->addParameter("ptr", DataType::POINTER); // var parameter
    getmemFunc->addParameter("size", DataType::INTEGER);
    define("getmem", getmemFunc);
    
    // FreeMem procedure - deallocate memory
    auto freememFunc = std::make_shared<Symbol>("freemem", SymbolType::PROCEDURE, DataType::VOID, 0);
    freememFunc->addParameter("ptr", DataType::POINTER); // var parameter
    freememFunc->addParameter("size", DataType::INTEGER); // optional size parameter
    define("freemem", freememFunc);
    
    // Mark procedure - mark memory allocation point
    auto markFunc = std::make_shared<Symbol>("mark", SymbolType::PROCEDURE, DataType::VOID, 0);
    markFunc->addParameter("ptr", DataType::POINTER); // var parameter
    define("mark", markFunc);
    
    // Release procedure - release memory to mark point
    auto releaseFunc = std::make_shared<Symbol>("release", SymbolType::PROCEDURE, DataType::VOID, 0);
    releaseFunc->addParameter("ptr", DataType::POINTER);
    define("release", releaseFunc);
}

} // namespace rpascal