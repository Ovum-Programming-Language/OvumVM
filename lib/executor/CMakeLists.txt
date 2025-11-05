add_library(executor STATIC
        PlaceholderJitExecutor.cpp
)

target_include_directories(executor PUBLIC ${PROJECT_SOURCE_DIR})
