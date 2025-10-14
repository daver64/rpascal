#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/symbol_table.h"
#include "../include/type_checker.h"
#include "../include/cpp_generator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using namespace rpascal;

// Command line argument structure
struct CompilerOptions {
    std::string inputFile;
    std::string outputFile;      // Executable output file (default)
    std::string cppFile;         // C++ intermediate file
    bool verbose = false;
    bool showTokens = false;
    bool showAST = false;
    bool helpRequested = false;
    bool keepCpp = false;        // Keep C++ file after compilation
};

// Function to display help information
void showHelp(const std::string& programName) {
    std::cout << "RPascal - Turbo Pascal 7 Compatible Compiler\n";
    std::cout << "Usage: " << programName << " [options] <input_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>     Specify output executable (default: <input>.exe)\n";
    std::cout << "  --keep-cpp    Keep intermediate C++ file after compilation\n";
    std::cout << "  -v            Verbose output\n";
    std::cout << "  --tokens      Show tokenization output\n";
    std::cout << "  --ast         Show Abstract Syntax Tree\n";
    std::cout << "  -h, --help    Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " hello.pas                    # Generates hello.exe\n";
    std::cout << "  " << programName << " -o myprogram.exe hello.pas   # Generates myprogram.exe\n";
    std::cout << "  " << programName << " --keep-cpp hello.pas         # Generates hello.exe and keeps hello.cpp\n";
    std::cout << "  " << programName << " --tokens --ast -v hello.pas  # Show debug output\n";
}

// Parse command line arguments
CompilerOptions parseArguments(int argc, char* argv[]) {
    CompilerOptions options;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            options.helpRequested = true;
            return options;
        } else if (arg == "-v") {
            options.verbose = true;
        } else if (arg == "--keep-cpp") {
            options.keepCpp = true;
        } else if (arg == "--tokens") {
            options.showTokens = true;
        } else if (arg == "--ast") {
            options.showAST = true;
        } else if (arg == "-o" && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (arg[0] != '-') {
            if (options.inputFile.empty()) {
                options.inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files specified\n";
                options.helpRequested = true;
                return options;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            options.helpRequested = true;
            return options;
        }
    }
    
    if (options.inputFile.empty() && !options.helpRequested) {
        std::cerr << "Error: No input file specified\n";
        options.helpRequested = true;
    }
    
    // Set default output file if not specified
    if (options.outputFile.empty() && !options.inputFile.empty()) {
        size_t lastDot = options.inputFile.find_last_of('.');
        if (lastDot != std::string::npos) {
            options.outputFile = options.inputFile.substr(0, lastDot) + ".exe";
        } else {
            options.outputFile = options.inputFile + ".exe";
        }
    }
    
    // Set C++ intermediate file name
    if (!options.inputFile.empty()) {
        size_t lastDot = options.inputFile.find_last_of('.');
        if (lastDot != std::string::npos) {
            options.cppFile = options.inputFile.substr(0, lastDot) + ".cpp";
        } else {
            options.cppFile = options.inputFile + ".cpp";
        }
    }
    
    return options;
}

// Read file contents
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Tokenize and optionally display tokens
std::unique_ptr<Lexer> tokenizeFile(const std::string& source, bool showTokens, bool verbose) {
    if (verbose) {
        std::cout << "Tokenizing source code...\n";
    }
    
    auto lexer = std::make_unique<Lexer>(source);
    
    if (showTokens) {
        std::cout << "\n=== TOKENS ===\n";
        auto tempLexer = std::make_unique<Lexer>(source);
        Token token;
        do {
            token = tempLexer->nextToken();
            std::cout << token.toString() << "\n";
        } while (token.getType() != TokenType::EOF_TOKEN);
        std::cout << "=== END TOKENS ===\n\n";
    }
    
    return lexer;
}

// Parse and optionally display AST
std::unique_ptr<Program> parseSource(std::unique_ptr<Lexer> lexer, bool showAST, bool verbose) {
    if (verbose) {
        std::cout << "Parsing source code...\n";
    }
    
    auto parser = std::make_unique<Parser>(std::move(lexer));
    auto program = parser->parseProgram();
    
    if (parser->hasErrors()) {
        std::cerr << "Parse errors:\n";
        for (const auto& error : parser->getErrors()) {
            std::cerr << "  " << error << "\n";
        }
        return nullptr;
    }
    
    if (showAST && program) {
        std::cout << "\n=== ABSTRACT SYNTAX TREE ===\n";
        std::cout << program->toString() << "\n";
        std::cout << "=== END AST ===\n\n";
    }
    
    return program;
}

// Perform semantic analysis
bool performSemanticAnalysis(std::unique_ptr<Program>& program, bool verbose, std::shared_ptr<SymbolTable>& symbolTable) {
    if (verbose) {
        std::cout << "Performing semantic analysis...\n";
    }
    
    symbolTable = std::make_shared<SymbolTable>();
    auto analyzer = std::make_unique<SemanticAnalyzer>(symbolTable);
    
    bool success = analyzer->analyze(*program);
    
    if (analyzer->hasErrors()) {
        std::cerr << "Semantic errors:\n";
        for (const auto& error : analyzer->getErrors()) {
            std::cerr << "  " << error << "\n";
        }
        return false;
    }
    
    if (verbose) {
        std::cout << "Semantic analysis completed successfully.\n";
    }
    
    return success;
}

// Generate C++ code
std::string generateCppCode(std::unique_ptr<Program>& program, std::shared_ptr<SymbolTable> symbolTable, bool verbose) {
    if (verbose) {
        std::cout << "Generating C++ code...\n";
    }
    
    auto generator = std::make_unique<CppGenerator>(symbolTable);
    std::string cppCode = generator->generate(*program);
    
    if (verbose) {
        std::cout << "C++ code generation completed.\n";
    }
    
    return cppCode;
}

// Compile C++ code to executable
// Execute a process and wait for completion
bool executeProcess(const std::string& executable, const std::vector<std::string>& args, bool verbose) {
#ifdef _WIN32
    // Build command line
    std::string cmdLine = "\"" + executable + "\"";
    for (const auto& arg : args) {
        cmdLine += " \"" + arg + "\"";
    }
    
    if (verbose) {
        std::cout << "Executing: " << cmdLine << std::endl;
        std::cout << "Working directory: " << std::filesystem::current_path() << std::endl;
    }
    
    // Create process
    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    
    // Create the process
    std::string currentDirStr = std::filesystem::current_path().string();
    BOOL success = CreateProcessA(
        executable.c_str(),      // Application name
        const_cast<char*>(cmdLine.c_str()), // Command line
        nullptr,                 // Process security attributes
        nullptr,                 // Thread security attributes
        FALSE,                   // Inherit handles
        0,                       // Creation flags
        nullptr,                 // Environment
        currentDirStr.c_str(),   // Current directory
        &si,                     // Startup info
        &pi                      // Process info
    );
    
    if (!success) {
        if (verbose) {
            DWORD error = GetLastError();
            std::cerr << "Failed to create process. Error code: " << error << std::endl;
        }
        return false;
    }
    
    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // Get exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    // Clean up
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return exitCode == 0;
#else
    // Unix/Linux implementation using execve would go here
    std::cerr << "Process execution not implemented for this platform" << std::endl;
    return false;
#endif
}

// CommandBuilder implementation based on rbasic
class CommandBuilder {
private:
    std::string compiler_;
    std::vector<std::string> compileFlags_;
    std::vector<std::string> linkFlags_;
    std::string inputFile_;
    std::string outputFile_;
    std::vector<std::string> libraries_;

public:
    CommandBuilder& compiler(const std::string& compiler) {
        compiler_ = compiler;
        return *this;
    }
    
    CommandBuilder& compileFlag(const std::string& flag) {
        compileFlags_.push_back(flag);
        return *this;
    }
    
    CommandBuilder& compileFlags(const std::vector<std::string>& flags) {
        compileFlags_.insert(compileFlags_.end(), flags.begin(), flags.end());
        return *this;
    }
    
    CommandBuilder& linkFlag(const std::string& flag) {
        linkFlags_.push_back(flag);
        return *this;
    }
    
    CommandBuilder& linkFlags(const std::vector<std::string>& flags) {
        linkFlags_.insert(linkFlags_.end(), flags.begin(), flags.end());
        return *this;
    }
    
    CommandBuilder& input(const std::string& inputFile) {
        inputFile_ = inputFile;
        return *this;
    }
    
    CommandBuilder& output(const std::string& outputFile) {
        outputFile_ = outputFile;
        return *this;
    }
    
    CommandBuilder& library(const std::string& library) {
        libraries_.push_back(library);
        return *this;
    }
    
    std::string build() const {
        if (compiler_.empty()) {
            throw std::runtime_error("Compiler not specified");
        }
        
        std::ostringstream cmd;
        cmd << escapeArgument(compiler_);
        
        // Add compile flags
        for (const auto& flag : compileFlags_) {
            cmd << " " << escapeArgument(flag);
        }
        
        // Add input file
        if (!inputFile_.empty()) {
            cmd << " " << escapeArgument(inputFile_);
        }
        
        // Add output specification
        if (!outputFile_.empty()) {
            if (isGccLikeCompiler()) {
                cmd << " -o " << escapeArgument(outputFile_);
            } else {
#ifdef _WIN32
                cmd << " /Fe:" << escapeArgument(outputFile_);
#else
                cmd << " -o " << escapeArgument(outputFile_);
#endif
            }
        }
        
        // Add libraries
        for (const auto& lib : libraries_) {
            cmd << " " << escapeArgument(lib);
        }
        
        // Add link flags
        if (!linkFlags_.empty()) {
            if (isGccLikeCompiler()) {
                for (const auto& flag : linkFlags_) {
                    cmd << " " << escapeArgument(flag);
                }
            }
        }
        
        return cmd.str();
    }
    
    int execute() const {
        std::string command = build();
        std::cout << "Executing: " << command << std::endl;
        
        // Use system() directly now that environment is fixed
        int result = std::system(command.c_str());
        
        if (result != 0 && result != -1) {
            std::cout << "Command exit code: " << result << std::endl;
        }
        
        return result;
        
#ifdef UNUSED_CREATEPROCESS_CODE
        // CreateProcess version - disabled due to WinMain vs main linking issues
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        
        std::string mutableCommand = command;
        
        BOOL success = CreateProcessA(
            nullptr,
            &mutableCommand[0],
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );
        
        if (!success) {
            return -1;
        }
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return static_cast<int>(exitCode);
#endif
    }

private:
    std::string escapeArgument(const std::string& arg) const {
        if (arg.find(' ') != std::string::npos || arg.find('"') != std::string::npos) {
            return "\"" + arg + "\"";
        }
        return arg;
    }
    
    bool isValidPath(const std::string& path) const {
        const std::string dangerous = "|&;()<>`$";
        return path.find_first_of(dangerous) == std::string::npos;
    }
    
    bool isGccLikeCompiler() const {
        std::string compilerName = compiler_;
        std::transform(compilerName.begin(), compilerName.end(), compilerName.begin(), 
                      [](char c) { return static_cast<char>(std::tolower(c)); });
        
        return compilerName.find("gcc") != std::string::npos ||
               compilerName.find("g++") != std::string::npos ||
               compilerName.find("clang") != std::string::npos ||
               compilerName.find("mingw") != std::string::npos;
    }
};

bool compileToExecutable(const std::string& cppFile, const std::string& exeFile, bool verbose) {
    if (!std::filesystem::exists(cppFile)) {
        std::cerr << "Error: C++ file does not exist: " << cppFile << std::endl;
        return false;
    }

    CommandBuilder builder;

    // Try MSVC first (cl.exe), then fallback to MinGW
    bool useMSVC = false;
    std::string compilerPath;
    
    // Check if cl.exe is available
    int result = std::system("cl >nul 2>&1");
    if (result == 0) {
        useMSVC = true;
        compilerPath = "cl";
        if (verbose) {
            std::cout << "Using MSVC compiler (cl.exe)" << std::endl;
        }
    } else {
        // Fallback to MinGW
        std::filesystem::path currentDir = std::filesystem::current_path();
        std::filesystem::path mingwPath = currentDir / "mingw64" / "bin" / "g++.exe";
        
        if (std::filesystem::exists(mingwPath)) {
            compilerPath = mingwPath.string();
        } else {
            // Try system MinGW
            compilerPath = "C:\\mingw64\\bin\\g++.exe";
            if (!std::filesystem::exists(compilerPath)) {
                std::cerr << "Error: Neither MSVC nor MinGW g++ found. Please install Visual Studio or MinGW64." << std::endl;
                return false;
            }
        }
        if (verbose) {
            std::cout << "Using MinGW compiler: " << compilerPath << std::endl;
        }
    }

    // Configure compiler based on type
    if (useMSVC) {
        // MSVC configuration
        builder.compiler(compilerPath)
               .compileFlags({"/std:c++17", "/O2", "/EHsc"})
               .input(cppFile)
               .output(exeFile);
    } else {
        // MinGW configuration with static linking
        builder.compiler(compilerPath)
               .compileFlags({"-std=c++17", "-O2", "-static-libgcc", "-static-libstdc++", "-static"})
               .input(cppFile)
               .output(exeFile);
    }

    if (verbose) {
        std::cout << "Compilation command: " << builder.build() << std::endl;
    }

    int exitCode = builder.execute();

    if (exitCode != 0) {
        std::cerr << "Error: Compilation failed with exit code " << exitCode << std::endl;
        return false;
    }

    if (verbose) {
        std::cout << "Successfully compiled to: " << exeFile << std::endl;
    }

    return true;
}

// Main compilation function
int compile(const CompilerOptions& options) {
    try {
        if (options.verbose) {
            std::cout << "RPascal Compiler v1.0.0\n";
            std::cout << "Input file: " << options.inputFile << "\n";
            std::cout << "Output file: " << options.outputFile << "\n\n";
        }
        
        // Read source file
        std::string source = readFile(options.inputFile);
        
        // Tokenize
        auto lexer = tokenizeFile(source, options.showTokens, options.verbose);
        
        // Check for lexer errors
        if (lexer->hasErrors()) {
            std::cerr << "Lexer errors:\n";
            for (const auto& error : lexer->getErrors()) {
                std::cerr << "  " << error << "\n";
            }
            return 1;
        }
        
        // Parse
        auto program = parseSource(std::move(lexer), options.showAST, options.verbose);
        if (!program) {
            return 1;
        }
        
        // Semantic Analysis
        std::shared_ptr<SymbolTable> symbolTable;
        if (!performSemanticAnalysis(program, options.verbose, symbolTable)) {
            return 1;
        }
        
        // Generate C++ Code
        std::string cppCode = generateCppCode(program, symbolTable, options.verbose);
        
        if (options.verbose) {
            std::cout << "Compilation successful!\n";
            std::cout << "Program name: " << program->getName() << "\n";
            std::cout << "Declarations: " << program->getDeclarations().size() << "\n";
        }
        
        // Write C++ code to intermediate file
        std::ofstream outFile(options.cppFile);
        if (!outFile.is_open()) {
            throw std::runtime_error("Could not create C++ file: " + options.cppFile);
        }
        
        outFile << cppCode;
        outFile.close();
        
        if (options.verbose) {
            std::cout << "C++ code generated: " << options.cppFile << "\n";
        }
        
        // Allow file system to settle before compilation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Compile the C++ code to executable
        if (!compileToExecutable(options.cppFile, options.outputFile, options.verbose)) {
            return 1;
        }
        if (options.verbose) {
            std::cout << "Executable created: " << options.outputFile << "\n";
        }
        
        // Remove C++ file unless user wants to keep it
        if (!options.keepCpp) {
            try {
                std::filesystem::remove(options.cppFile);
                if (options.verbose) {
                    std::cout << "Removed intermediate C++ file: " << options.cppFile << "\n";
                }
            } catch (const std::exception& e) {
                if (options.verbose) {
                    std::cout << "Warning: Could not remove C++ file: " << e.what() << "\n";
                }
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    CompilerOptions options = parseArguments(argc, argv);
    
    if (options.helpRequested) {
        showHelp(argv[0]);
        return options.inputFile.empty() ? 1 : 0;
    }
    
    return compile(options);
}