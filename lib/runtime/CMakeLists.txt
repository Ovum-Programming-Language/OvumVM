add_library(runtime STATIC
        VirtualTable.cpp
        VirtualTableRepository.cpp
        ObjectRepository.cpp
        ByteArray.cpp
        MarkAndSweepGC.cpp
        MemoryManager.cpp
)

target_include_directories(runtime PUBLIC ${PROJECT_SOURCE_DIR})
