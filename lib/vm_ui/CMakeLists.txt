add_library(vm_ui STATIC
        vm_ui_functions.cpp
)

target_link_libraries(vm_ui PUBLIC
        tokens
        bytecode_lexer
        bytecode_parser
        execution_tree
        runtime
        executor
)

target_include_directories(vm_ui PUBLIC ${PROJECT_SOURCE_DIR})
