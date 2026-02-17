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

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    # Old GCC versions require manual linking with stdc++fs for std::filesystem support
    target_link_libraries(cpprun
        PRIVATE
            stdc++fs
    )
endif()


if(ENABLE_TESTING)
    include(CTest)
    add_test(NAME cpprun_test
        COMMAND cpprun ${CMAKE_CURRENT_SOURCE_DIR}/hello.cpp -- foo bar baz
    )
    set_tests_properties(cpprun_test
        PROPERTIES
            PASS_REGULAR_EXPRESSION
                "Hello World!\nargv\\[1\\]: foo\nargv\\[2\\]: bar\nargv\\[3\\]: baz\n"
    )
endif()
