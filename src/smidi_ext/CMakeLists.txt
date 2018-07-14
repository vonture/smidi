set(smidi_include_dir ../../include)

add_library(smidi_ext
    smidi_messages.cpp
    ${smidi_include_dir}/smidi_ext/smidi_messages.h
)

target_link_libraries(smidi_ext
    smidi
)

set_target_properties(smidi_ext PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(smidi_ext PRIVATE
    .
)

target_include_directories(smidi_ext PUBLIC
    ${smidi_include_dir}
)
