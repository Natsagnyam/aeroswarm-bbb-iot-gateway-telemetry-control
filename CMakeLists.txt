cmake_minimum_required(VERSION 3.16)
project(aeroswarm_edge_gateway VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gateway_node
    src/main.cpp
)

target_include_directories(gateway_node PRIVATE
    include
)
