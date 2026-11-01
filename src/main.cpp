#include <iostream>
#include <thread>
#include <chrono>
#include "gpio_controller.hpp"

int main() {
    try {
        aeroswarm::GpioController gpio;
        std::cout << "GPIO Gateway Controller initialized successfully.\n";

        // Example: Set pin 23 high
        uint32_t pin_mask = (1 << 23);
        if (gpio.write_pins(pin_mask, 1)) {
            std::cout << "Pin 23 set HIGH.\n";
        }

        // Polling loop example
        for (int i = 0; i < 5; ++i) {
            uint32_t current_state = gpio.read_pins();
            std::cout << "Current GPIO0 Data In: 0x" << std::hex << current_state << std::dec << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Set pin 23 low
        gpio.write_pins(pin_mask, 0);
        std::cout << "Pin 23 set LOW.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
