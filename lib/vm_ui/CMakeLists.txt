add_library(vm_ui STATIC
        vm_ui_functions.cpp
)

target_link_libraries(vm_ui PUBLIC
        mylib
)

target_include_directories(vm_ui PUBLIC ${PROJECT_SOURCE_DIR})
