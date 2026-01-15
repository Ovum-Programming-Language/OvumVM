set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_definitions("_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH")


if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Detect target architecture for JIT support
# x86_64 is reported as "AMD64" on Windows, "x86_64" on Linux/Mac
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(OVUM_JIT_X64_AVAILABLE ON CACHE BOOL "JIT compiler available for x86_64 architecture")
    set(JIT_PROVIDED ON CACHE BOOL "JIT compiler provided")
    add_compile_definitions(JIT_PROVIDED)
    message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR} - JIT support available")
else()
    set(OVUM_JIT_X64_AVAILABLE OFF CACHE BOOL "JIT compiler available for x86_64 architecture")
    message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR} - JIT support not available")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Compiler flags debug: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Compiler flags release: ${CMAKE_CXX_FLAGS_RELEASE}")

# Windows: place runtime artifacts next to the executable
if(WIN32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
