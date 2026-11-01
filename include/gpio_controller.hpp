#pragma once

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace aeroswarm {

class GpioController {
public:
    explicit GpioController(const char* dev_path = "/dev/bbb_custom_gpio") {
        fd_ = open(dev_path, O_RDWR);
        if (fd_ < 0) {
            throw std::runtime_error("Failed to open GPIO device node");
        }
    }

    ~GpioController() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    // Delete copy constructors to prevent dangling file descriptor handles
    GpioController(const GpioController&) = delete;
    GpioController& operator=(const GpioController&) = delete;

    // Read 32-bit GPIO_DATAIN register (zero-allocation hot-path safe)
    inline uint32_t read_pins() const noexcept {
        uint32_t data = 0;
        if (pread(fd_, &data, sizeof(data), 0) < 0) {
            return 0;
        }
        return data;
    }

    // Write pin states using mask and value (1 = set high, 0 = set low)
    inline bool write_pins(uint32_t mask, uint32_t value) noexcept {
        struct GpioCmd {
            uint32_t mask;
            uint32_t value;
        } cmd{mask, value};

        return write(fd_, &cmd, sizeof(cmd)) == sizeof(cmd);
    }

private:
    int fd_{-1};
};

} // namespace aeroswarm
