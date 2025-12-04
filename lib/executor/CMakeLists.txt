add_library(executor STATIC
        PlaceholderJitExecutor.cpp
        BuiltinFunctions.cpp
        builtin_factory.cpp
)

target_include_directories(executor PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(executor PUBLIC runtime execution_tree)
