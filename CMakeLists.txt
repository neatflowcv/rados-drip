cmake_minimum_required(VERSION 4.3.4)

project(cpp_structure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp_structure
    app/main.cc
)
target_link_libraries(cpp_structure PRIVATE
    rados
)
