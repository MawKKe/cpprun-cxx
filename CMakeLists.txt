cmake_minimum_required(VERSION 3.20)

project(cpprun-cxx)

add_executable(cpprun cpprun.cpp)

target_compile_features(cpprun
    PRIVATE
        cxx_std_17
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(cpprun
        PRIVATE
            -Wall -Wextra -pedantic
    )
endif()
