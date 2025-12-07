add_library(bytecode_lexer STATIC
        BytecodeSourceWrapper.cpp
        BytecodeLexer.cpp
        handlers/WhitespaceHandler.cpp
        handlers/PunctHandler.cpp
        handlers/IdentifierHandler.cpp
        handlers/StringHandler.cpp
        handlers/DefaultHandler.cpp
        handlers/NumberHandler.cpp
)

target_include_directories(bytecode_lexer PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(bytecode_lexer PUBLIC tokens)
