add_library(bytecode_parser STATIC
        BytecodeParser.cpp
        ParserContext.cpp
        scenarios/InitStaticParser.cpp
        scenarios/FunctionParser.cpp
        scenarios/IfParser.cpp
        scenarios/WhileParser.cpp
        scenarios/CommandParser.cpp
        scenarios/VtableParser.cpp
)


target_include_directories(bytecode_parser PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(bytecode_parser PUBLIC tokens)
