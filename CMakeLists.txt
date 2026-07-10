cmake_minimum_required(VERSION 4.3.4)

project(rados-drip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
add_subdirectory(cli)

add_executable(rados-drip
    app/main.cc
    app/drip_options.cc
    client/client.cc
    client/object.cc
    config/config.cc
)
target_include_directories(rados-drip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rados-drip PRIVATE
    cli-parser
    rados
)

add_executable(rados-delete
    app/delete.cc
    app/delete_options.cc
    client/client.cc
    client/object.cc
    config/config.cc
)
target_include_directories(rados-delete PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rados-delete PRIVATE
    cli-parser
    rados
)
