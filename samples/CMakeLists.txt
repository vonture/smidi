add_executable(list_devices
    list_devices.cpp
)

target_link_libraries(list_devices
    smidi
)

add_executable(simple_output
    simple_output.cpp
)

target_link_libraries(simple_output
    smidi
)
