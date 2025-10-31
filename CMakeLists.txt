cmake_minimum_required(VERSION 3.25)

project(
        ovum-vm
        VERSION 0.1
        DESCRIPTION "Ovum Virtual Machine that interprets Ovum bytecode"
        LANGUAGES CXX
)

include(cmake/SetCompilerOptions.cmake)
include(cmake/IncludeExternalLibraries.cmake)

add_subdirectory(lib)
add_subdirectory(bin)


enable_testing()
add_subdirectory(tests)
