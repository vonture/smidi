set(smidi_include_dir ../../include)
set(smidi_sources
    smidi.cpp
    ${smidi_include_dir}/smidi/smidi.h
)

if(SMIDI_DLL)
    add_library(smidi SHARED
        ${smidi_sources}
    )
    target_compile_definitions(smidi PRIVATE
        _SMIDI_DLL_IMPLEMENTATION
    )
    target_compile_definitions(smidi PUBLIC
        SMIDI_DLL
    )
else()
    add_library(smidi
        ${smidi_sources}
    )
endif()

if(WIN32)
    target_sources(smidi PRIVATE
        winmm/winmm_device.cpp
    )
    target_link_libraries(smidi PRIVATE
        winmm.lib
    )
endif()

set_target_properties(smidi PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(smidi PRIVATE
    .
)
target_include_directories(smidi PUBLIC
    ${smidi_include_dir}
)
