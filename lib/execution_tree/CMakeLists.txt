add_library(execution_tree STATIC
        Block.cpp
        ConditionalExecution.cpp
        Function.cpp
        IfMultibranch.cpp
        WhileExecution.cpp
)

target_include_directories(execution_tree PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(execution_tree PUBLIC runtime)
