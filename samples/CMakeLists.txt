function(add_sample name)
    add_executable(${name}
        ${name}.cpp
    )

    target_link_libraries(${name}
        smidi
        smidi_ext
    )

    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 17
    )
endfunction()

add_sample("list_devices")
add_sample("simple_output")
add_sample("simple_input")
