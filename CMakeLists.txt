cmake_minimum_required(VERSION 3.16)

project(cpp_structure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

function(cpp_structure_enable_tidy target_name)
    if(CLANG_TIDY_EXE)
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--header-filter=^${PROJECT_SOURCE_DIR}/(app|simple)/"
        )
    endif()
endfunction()

include(FetchContent)

add_library(simple
    simple/simple.cc
)
target_include_directories(simple PUBLIC
    "${PROJECT_SOURCE_DIR}"
)
target_compile_features(simple PUBLIC cxx_std_23)
cpp_structure_enable_tidy(simple)

add_executable(cpp_structure
    app/main.cc
)
target_link_libraries(cpp_structure PRIVATE simple)
cpp_structure_enable_tidy(cpp_structure)

include(CTest)

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(simple_test
        simple/simple_test.cc
    )
    target_link_libraries(simple_test PRIVATE
        simple
        GTest::gtest_main
    )
    cpp_structure_enable_tidy(simple_test)

    include(GoogleTest)
    gtest_discover_tests(simple_test)
endif()

option(CPP_STRUCTURE_BUILD_BENCHMARKS "Build Google Benchmark examples" ON)

if(CPP_STRUCTURE_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.5
    )
    FetchContent_MakeAvailable(benchmark)

    add_executable(simple_benchmark
        simple/simple_benchmark.cc
    )
    target_link_libraries(simple_benchmark PRIVATE
        simple
        benchmark::benchmark_main
    )
    cpp_structure_enable_tidy(simple_benchmark)
endif()
