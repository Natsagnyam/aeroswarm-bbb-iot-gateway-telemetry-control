# Define a standalone test executable
add_executable(unit_tests test_gpio_controller.cpp)

# Link include paths
target_include_directories(unit_tests PRIVATE ../include)

# Register the test binary with CTest
add_test(NAME GpioControllerTest COMMAND unit_tests)
