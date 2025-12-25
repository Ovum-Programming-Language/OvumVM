enable_testing()

add_executable(
        ${PROJECT_NAME}_tests
        main_test.cpp
        unit_tests.cpp
        test_functions.cpp
        test_suites/ProjectIntegrationTestSuite.cpp
        test_suites/BytecodeLexerTestSuite.cpp
        test_suites/BytecodeParserTestSuite.cpp
        test_suites/BuiltinTestSuite.cpp
        bytecode_lexer_tests.cpp
        bytecode_parser_tests.cpp
        bytecode_commands_tests.cpp
        builtin_functions_tests.cpp
)

target_link_libraries(
        ${PROJECT_NAME}_tests
        bytecode_lexer
        bytecode_parser
        execution_tree
        runtime
        executor
        vm_ui
        GTest::gtest_main
)

message(STATUS "Tests build type: ${CMAKE_BUILD_TYPE}")

if (WITH_VALGRIND)
        target_compile_definitions(${PROJECT_NAME}_tests PUBLIC WITH_VALGRIND)
endif ()

target_include_directories(${PROJECT_NAME}_tests PUBLIC ${PROJECT_SOURCE_DIR})

add_custom_command(
    TARGET ${PROJECT_NAME}_tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Updating examples submodule..."
    COMMAND git -C ${CMAKE_SOURCE_DIR} submodule update --init --recursive tests/test_data/examples
    COMMAND git -C ${CMAKE_SOURCE_DIR}/tests/test_data/examples fetch origin main
    COMMAND git -C ${CMAKE_SOURCE_DIR}/tests/test_data/examples checkout main
    COMMAND git -C ${CMAKE_SOURCE_DIR}/tests/test_data/examples pull origin main
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning previous test_data..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/test_data
    COMMAND ${CMAKE_COMMAND} -E echo "Copying test_data..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test_data ${CMAKE_CURRENT_BINARY_DIR}/test_data
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling C++ files from deps directory..."
    COMMAND ${CMAKE_COMMAND} 
        -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        -P ${CMAKE_SOURCE_DIR}/cmake/CompileDeps.cmake
    COMMENT "Updating git submodule and preparing test_data"
    VERBATIM
)

include(GoogleTest)

gtest_discover_tests(${PROJECT_NAME}_tests)
