# Script to compile all C++ files in deps directory as dynamic libraries

set(DEPS_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_data/examples/deps")

if(WIN32)
    set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# Use compiler from CMake if available
if(CMAKE_CXX_COMPILER)
    set(COMPILER_FROM_CMAKE "${CMAKE_CXX_COMPILER}")
else()
    set(COMPILER_FROM_CMAKE "")
endif()

# Check if deps directory exists
if(NOT EXISTS "${DEPS_DIR}")
    message(WARNING "Deps directory does not exist: ${DEPS_DIR}")
    return()
endif()

# Find all C++ files
file(GLOB CPP_FILES "${DEPS_DIR}/*.cpp")

if(NOT CPP_FILES)
    message(STATUS "No C++ files found in ${DEPS_DIR}")
    return()
endif()

list(LENGTH CPP_FILES FILE_COUNT)
message(STATUS "Found ${FILE_COUNT} C++ file(s) to compile")

# Determine compiler and flags based on platform
if(WIN32)
    # For Windows/MSVC, find vcvarsall.bat to set up environment
    # Use execute_process to get environment variables with parentheses in their names
    execute_process(
        COMMAND cmd /c "echo %ProgramFiles(x86)%"
        OUTPUT_VARIABLE PROGRAM_FILES_X86
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND cmd /c "echo %ProgramFiles%"
        OUTPUT_VARIABLE PROGRAM_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Try common locations for Visual Studio
    set(VCVARSALL_PATHS
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2019/Professional/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES}/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES}/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES}/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"
        "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"
    )

    set(VCVARSALL_FOUND "")
    foreach(VCVARSALL_PATH ${VCVARSALL_PATHS})
        if(EXISTS "${VCVARSALL_PATH}")
            set(VCVARSALL_FOUND "${VCVARSALL_PATH}")
            break()
        endif()
    endforeach()

    # Use compiler from CMake if available
    if(COMPILER_FROM_CMAKE)
        set(COMPILER "${COMPILER_FROM_CMAKE}")
    else()
        set(COMPILER "cl")
    endif()
    set(SHARED_FLAG "/LD")
    set(OUTPUT_FLAG "/Fe:")
    set(EXTENSION ".dll")
    # Use C++ standard from CMake if available, otherwise default to C++20
    if(CMAKE_CXX_STANDARD)
        if(CMAKE_CXX_STANDARD VERSION_GREATER_EQUAL 23)
            set(FLAG_STD "/std:c++latest")
        else()
            set(FLAG_STD "/std:c++${CMAKE_CXX_STANDARD}")
        endif()
    else()
        set(FLAG_STD "/std:c++20")
    endif()
    set(FLAG_OPT "/O2")
elseif(APPLE)
    if(COMPILER_FROM_CMAKE)
        set(COMPILER "${COMPILER_FROM_CMAKE}")
    else()
        set(COMPILER "clang++")
    endif()
    set(SHARED_FLAG "-shared")
    set(OUTPUT_FLAG "-o")
    set(EXTENSION ".so")
    set(FLAG_FPIC "-fPIC")
    if(CMAKE_CXX_STANDARD)
        set(FLAG_STD "-std=c++${CMAKE_CXX_STANDARD}")
    else()
        set(FLAG_STD "-std=c++23")
    endif()
    set(FLAG_OPT "-O2")
elseif(UNIX)
    if(COMPILER_FROM_CMAKE)
        set(COMPILER "${COMPILER_FROM_CMAKE}")
    else()
        set(COMPILER "g++")
    endif()
    set(SHARED_FLAG "-shared")
    set(OUTPUT_FLAG "-o")
    set(EXTENSION ".so")
    set(FLAG_FPIC "-fPIC")
    if(CMAKE_CXX_STANDARD)
        set(FLAG_STD "-std=c++${CMAKE_CXX_STANDARD}")
    else()
        set(FLAG_STD "-std=c++23")
    endif()
    set(FLAG_OPT "-O2")
endif()

# Compile each C++ file
foreach(CPP_FILE ${CPP_FILES})
    get_filename_component(FILE_NAME ${CPP_FILE} NAME_WE)
    set(OUTPUT_FILE "${OUTPUT_DIR}/${FILE_NAME}${EXTENSION}")

    message(STATUS "Compiling ${CPP_FILE} -> ${OUTPUT_FILE}")

    if(WIN32)
        # MSVC command - /LD creates DLL, /Fe: specifies output file
        # Use vcvarsall.bat to set up environment if found, otherwise use compiler directly
        if(VCVARSALL_FOUND)
            # Create a temporary batch file to compile with proper environment
            set(BATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/compile_${FILE_NAME}.bat")
            # Convert paths to native Windows format
            file(TO_NATIVE_PATH "${VCVARSALL_FOUND}" VCVARSALL_NATIVE)
            file(TO_NATIVE_PATH "cl.exe" COMPILER_NATIVE)
            file(TO_NATIVE_PATH "${CPP_FILE}" CPP_FILE_NATIVE)
            file(TO_NATIVE_PATH "${OUTPUT_FILE}" OUTPUT_FILE_NATIVE)

            file(WRITE "${BATCH_FILE}" "@echo off\n")
            file(APPEND "${BATCH_FILE}" "call \"${VCVARSALL_NATIVE}\"\n")
            # Ensure all paths with spaces are properly quoted
            file(APPEND "${BATCH_FILE}" "\"${COMPILER_NATIVE}\" ${SHARED_FLAG} ${FLAG_STD} ${FLAG_OPT} \"${CPP_FILE_NATIVE}\" ${OUTPUT_FLAG}\"${OUTPUT_FILE_NATIVE}\"\n")
            file(APPEND "${BATCH_FILE}" "exit /b %ERRORLEVEL%\n")

            execute_process(
                COMMAND cmd /c "${BATCH_FILE}"
                WORKING_DIRECTORY ${OUTPUT_DIR}
                RESULT_VARIABLE RESULT
                ERROR_VARIABLE ERROR_OUT
                OUTPUT_VARIABLE OUTPUT
            )

            # Clean up temporary batch file
            file(REMOVE "${BATCH_FILE}")
        else()
            # Fallback: try to use compiler directly (may fail if environment not set)
            message(WARNING "vcvarsall.bat not found, trying G++")
            set(COMPILER "g++")
            set(SHARED_FLAG "-shared")
            set(OUTPUT_FLAG "-o")
            set(EXTENSION ".dll")
            set(FLAG_FPIC "-fPIC")
            if(CMAKE_CXX_STANDARD)
                set(FLAG_STD "-std=c++${CMAKE_CXX_STANDARD}")
            else()
                set(FLAG_STD "-std=c++23")
            endif()
            set(FLAG_OPT "-O2")
            set(OUTPUT_FILE "${OUTPUT_DIR}/${FILE_NAME}${EXTENSION}")
            execute_process(
                COMMAND ${COMPILER} ${SHARED_FLAG} ${FLAG_FPIC} ${FLAG_STD} ${FLAG_OPT} ${CPP_FILE} ${OUTPUT_FLAG} ${OUTPUT_FILE}
                WORKING_DIRECTORY ${OUTPUT_DIR}
                RESULT_VARIABLE RESULT
                ERROR_VARIABLE ERROR_OUT
                OUTPUT_VARIABLE OUTPUT
            )
        endif()
    else()
        # Unix-like (clang++/g++) command
        execute_process(
            COMMAND ${COMPILER} ${SHARED_FLAG} ${FLAG_FPIC} ${FLAG_STD} ${FLAG_OPT} ${CPP_FILE} ${OUTPUT_FLAG} ${OUTPUT_FILE}
            WORKING_DIRECTORY ${DEPS_DIR}
            RESULT_VARIABLE RESULT
            ERROR_VARIABLE ERROR_OUT
            OUTPUT_VARIABLE OUTPUT
        )
    endif()

    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to compile ${CPP_FILE}: ${ERROR_OUT}")
    endif()

    message(STATUS "Successfully compiled ${OUTPUT_FILE}")
endforeach()

message(STATUS "Finished compiling all C++ files from deps directory")

