cmake_minimum_required(VERSION 3.10)
project(aeroswarm_bbb_gateway CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable CTest infrastructure
enable_testing()

# Include directories
include_directories(include)

# Main console gateway node target
# add_executable(gateway_node src/main.cpp)
add_executable(gateway_node src/server.cpp)

# IoT Gateway Server target (expects src/server.cpp or similar)
add_executable(iot_gateway_server src/server.cpp)

# Add the tests subdirectory
add_subdirectory(tests)
