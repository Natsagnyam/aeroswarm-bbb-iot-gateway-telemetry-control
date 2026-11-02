#ifndef GPIO_CONTROLLER_HPP
#define GPIO_CONTROLLER_HPP

#include <fstream>
#include <cstdint>

namespace aeroswarm {
class GpioController {
public:
    GpioController() {
        // Initialize GPIO 60 (P9_12) sysfs export
        std::ofstream exp("/sys/class/gpio/export");
        if (exp.is_open()) {
            exp << "60";
        }
        std::ofstream dir("/sys/class/gpio/gpio60/direction");
        if (dir.is_open()) {
            dir << "out";
        }
    }

    ~GpioController() {}

    bool write_pins(uint32_t mask, int val) {
        // 1. Control LED3 (USR3)
        std::ofstream led_file("/sys/class/leds/beaglebone:green:usr3/brightness");
        if (led_file.is_open()) {
            led_file << (val ? "1" : "0");
        }

        // 2. Control physical pin P9_12 (GPIO 60)
        std::ofstream p9_file("/sys/class/gpio/gpio60/value");
        if (p9_file.is_open()) {
            p9_file << (val ? "1" : "0");
        }
        return true;
    }

    uint32_t read_pins() {
        std::ifstream led_file("/sys/class/leds/beaglebone:green:usr3/brightness");
        int val = 0;
        if (led_file.is_open()) {
            led_file >> val;
        }
        return val ? 1 : 0;
    }
};
}

#endif
