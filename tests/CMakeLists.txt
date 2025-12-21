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

include(GoogleTest)

gtest_discover_tests(${PROJECT_NAME}_tests)
