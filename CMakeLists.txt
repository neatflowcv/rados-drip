cmake_minimum_required(VERSION 4.3.4)

project(rados-drip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(rados-drip
    app/config_new.cc
    app/main.cc
    client/client.cc
    client/object.cc
    options/options.cc
)
target_include_directories(rados-drip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rados-drip PRIVATE
    rados
)
