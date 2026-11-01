CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Iinclude

TARGET = gateway_node
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/gpio_controller.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
