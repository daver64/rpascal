#include "../include/unit_loader.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace rpascal {

UnitLoader::UnitLoader() {
    // Add current directory as default search path
    addSearchPath(".");
    addSearchPath("./units");
    addSearchPath("../");  // Parent directory
    addSearchPath("../units");  // Parent units directory
}

std::unique_ptr<Unit> UnitLoader::loadUnit(const std::string& unitName) {
    // Check if already loaded
    if (isUnitLoaded(unitName)) {
        // Return a copy or reference - for now, reload to keep it simple
        // In a real implementation, we'd return cached unit
    }
    
    // Find unit file
    std::string unitFile = findUnitFile(unitName);
    if (unitFile.empty()) {
        std::cerr << "Unit file not found: " << unitName << "\n";
        return nullptr;
    }
    
    // Load file content
    std::string content = loadFileContent(unitFile);
    if (content.empty()) {
        std::cerr << "Failed to load unit file: " << unitFile << "\n";
        return nullptr;
    }
    
    // Parse unit
    auto lexer = std::make_unique<Lexer>(content);
    auto parser = std::make_unique<Parser>(std::move(lexer));
    
    auto unit = parser->parseUnit();
    if (parser->hasErrors()) {
        std::cerr << "Parse errors in unit " << unitName << ":\n";
        for (const auto& error : parser->getErrors()) {
            std::cerr << "  " << error << "\n";
        }
        return nullptr;
    }
    
    // Store in cache  
    loadedUnits_[unitName] = std::move(unit);
    
    // Return nullptr since we moved to cache - caller should check isUnitLoaded()
    return nullptr;
}

bool UnitLoader::isUnitLoaded(const std::string& unitName) const {
    return loadedUnits_.find(unitName) != loadedUnits_.end();
}

Unit* UnitLoader::getLoadedUnit(const std::string& unitName) const {
    auto it = loadedUnits_.find(unitName);
    return it != loadedUnits_.end() ? it->second.get() : nullptr;
}

void UnitLoader::addSearchPath(const std::string& path) {
    searchPaths_.push_back(path);
}

void UnitLoader::clearUnits() {
    loadedUnits_.clear();
}

std::string UnitLoader::findUnitFile(const std::string& unitName) {
    // Try different file extensions and paths
    std::vector<std::string> extensions = {".pas", ".pp", ".p"};
    
    for (const auto& searchPath : searchPaths_) {
        for (const auto& ext : extensions) {
            std::filesystem::path fullPath = std::filesystem::path(searchPath) / (unitName + ext);
            if (std::filesystem::exists(fullPath)) {
                return fullPath.string();
            }
            
            // Also try with lowercase unit name
            std::string lowerUnitName = unitName;
            std::transform(lowerUnitName.begin(), lowerUnitName.end(), lowerUnitName.begin(), 
                          [](char c) { return static_cast<char>(std::tolower(c)); });
            std::filesystem::path lowerPath = std::filesystem::path(searchPath) / (lowerUnitName + ext);
            if (std::filesystem::exists(lowerPath)) {
                return lowerPath.string();
            }
        }
    }
    
    return ""; // Not found
}

std::string UnitLoader::loadFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace rpascal