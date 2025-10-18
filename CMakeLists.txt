cmake_minimum_required(VERSION 3.15)
project(rpascal VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories for all configurations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Override per-configuration output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin)
endforeach()

# Set build directory
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Compiler-specific settings
if(MSVC)
    # MSVC specific settings
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    message(STATUS "Using MSVC compiler")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC specific settings
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    # Disable specific warnings that are problematic for our codebase
    add_compile_options(-Wno-unused-but-set-variable -Wno-unused-parameter)
    message(STATUS "Using GCC compiler")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang specific settings
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    # Disable specific warnings that may be problematic
    add_compile_options(-Wno-unused-parameter)
    message(STATUS "Using Clang compiler")
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-g -O0)
    endif()
else()
    add_compile_definitions(NDEBUG)
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
    endif()
endif()

# Source files
set(LEXER_SOURCES
    src/lexer/token.cpp
    src/lexer/lexer.cpp
)

set(PARSER_SOURCES
    src/parser/ast.cpp
    src/parser/parser.cpp
)

set(SEMANTIC_SOURCES
    src/semantic/symbol_table.cpp
    src/semantic/type_checker.cpp
    src/unit_loader.cpp
)

set(CODEGEN_SOURCES
    src/codegen/cpp_generator.cpp
)

set(RUNTIME_SOURCES
    src/runtime/pascal_runtime.cpp
)

set(MAIN_SOURCES
    src/main.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${SEMANTIC_SOURCES}
    ${CODEGEN_SOURCES}
    ${RUNTIME_SOURCES}
    ${MAIN_SOURCES}
)

# Create the main executable
add_executable(rpascal ${ALL_SOURCES})

# Set target properties
set_target_properties(rpascal PROPERTIES
    OUTPUT_NAME "rpascal"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Optional: Add testing support
enable_testing()

# Optional: Add subdirectories for tests
# add_subdirectory(tests)

# Print configuration summary
message(STATUS "")
message(STATUS "=== RPascal Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "=====================================")
message(STATUS "")