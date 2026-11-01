# Aeroswarm BBB IoT Gateway: Telemetry & Control

A high-performance, low-latency embedded Linux gateway designed for the **BeagleBone Black (AM335x ARM architecture)**. This project bridges kernel-space hardware register memory-mapping with a zero-allocation C++ user-space controller for real-time GPIO telemetry and actuator control.

## Architecture & Design

- **Kernel Driver (`kernel_driver/`)**: An out-of-tree character device driver (`bbb_custom_gpio`) utilizing standard `ioremap` to map the AM335x GPIO0 register space (`0x44e07000`) safely without resource conflicts. It supports direct 32-bit hardware register reads (`GPIO_DATAIN`) and atomic bitmask writes via `GPIO_SETDATAOUT` and `GPIO_CLEARDATAOUT`.
- **C++ Controller (`include/gpio_controller.hpp`)**: A zero-allocation, hot-path safe C++ wrapper utilizing direct file descriptor operations (`pread`/`write`) and an 8-byte command payload struct (`mask` + `value`).
- **Build System**: Lightweight `Makefile` compiling with `-O3` optimization standards for bare-metal performance.

## Repository Structure

text
aeroswarm-bbb-iot-gateway-telemetry-control/
├── include/
│   └── gpio_controller.hpp   # Zero-allocation C++ GPIO controller
├── src/
│   └── main.cpp              # Gateway test runner & execution loop
├── kernel_driver/            # Out-of-tree AM335x GPIO kernel module
├── Makefile                  # Build automation
└── README.md


## Quick Start

### 1. Build the Gateway Node
bash
make clean && make


### 2. Run the Gateway Application
bash
sudo ./gateway_node


## License
GPL v2 / MIT
