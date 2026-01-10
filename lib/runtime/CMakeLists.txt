add_library(runtime STATIC
        VirtualTable.cpp
        VirtualTableRepository.cpp
        ObjectRepository.cpp
        ByteArray.cpp
        MemoryManager.cpp
        gc/MarkAndSweepGC.cpp
        gc/reference_scanners/ArrayReferenceScanner.cpp
        gc/reference_scanners/DefaultReferenceScanner.cpp
)

target_include_directories(runtime PUBLIC ${PROJECT_SOURCE_DIR})
