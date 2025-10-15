#pragma once

#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace rpascal {

// Unit loader for loading and parsing Pascal unit files
class UnitLoader {
public:
    UnitLoader();
    
    // Load a unit by name, returns nullptr if not found or parse error
    std::unique_ptr<Unit> loadUnit(const std::string& unitName);
    
    // Check if a unit is already loaded
    bool isUnitLoaded(const std::string& unitName) const;
    
    // Get a loaded unit
    Unit* getLoadedUnit(const std::string& unitName) const;
    
    // Add search path for unit files
    void addSearchPath(const std::string& path);
    
    // Clear all loaded units
    void clearUnits();
    
private:
    // Find unit file in search paths
    std::string findUnitFile(const std::string& unitName);
    
    // Load unit file content
    std::string loadFileContent(const std::string& filePath);
    
    // Cache of loaded units
    std::unordered_map<std::string, std::unique_ptr<Unit>> loadedUnits_;
    
    // Search paths for unit files
    std::vector<std::string> searchPaths_;
};

} // namespace rpascal