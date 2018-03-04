set(smidi_include_dir ../../include)

add_library(smidi
    smidi.cpp
    ${smidi_include_dir}/smidi/smidi.h
)

if(WIN32)
    target_sources(smidi PRIVATE
        winmm/winmm_device.cpp
    )
    target_link_libraries(smidi PRIVATE
        winmm.lib
    )
endif()

set_property(TARGET smidi PROPERTY CXX_STANDARD 14)

target_include_directories(smidi PRIVATE
    .
)
target_include_directories(smidi PUBLIC
    ${smidi_include_dir}
)
