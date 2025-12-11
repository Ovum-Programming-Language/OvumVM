add_library(runtime STATIC
        VirtualTable.cpp
        VirtualTableRepository.cpp
        ObjectRepository.cpp
        ByteArray.cpp
)

target_include_directories(runtime PUBLIC ${PROJECT_SOURCE_DIR})
