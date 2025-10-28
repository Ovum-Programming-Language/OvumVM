add_library(runtime STATIC
        VirtualTable.cpp
)

target_include_directories(runtime PUBLIC ${PROJECT_SOURCE_DIR})
