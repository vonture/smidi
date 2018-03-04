cmake_minimum_required(VERSION 3.1)

project(smidi)

option(SMIDI_BUILD_SAMPLES "Build samples" ON)
option(SMIDI_BUILD_TESTS "Build tests" ON)

add_subdirectory(src)
if(SMIDI_BUILD_TESTS)
    add_subdirectory(tests)
endif()
if(SMIDI_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
