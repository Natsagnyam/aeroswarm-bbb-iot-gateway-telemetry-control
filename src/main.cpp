#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>

#define CONTROL_MODULE_PHY 0x44E10000
#define CONF_GPMC_BE1N_OFFSET 0x878  // Pad control offset for P9_12

#define GPIO1_PHY 0x4804C000
#define GPIO_OE        0x134
#define GPIO_DATAIN    0x138
#define INPUT_PIN      (1 << 28)     // P9_12 is GPIO1_28

volatile uint32_t *gpio1_addr = nullptr;

int init_hardware() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/mem" << std::endl;
        return -1;
    }

    // 1. Map Control Module to enable RXACTIVE for P9_12
    volatile void *ctrl_mapped = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CONTROL_MODULE_PHY);
    if (ctrl_mapped != MAP_FAILED) {
        volatile uint32_t *p9_12_reg = (volatile uint32_t *)((char *)ctrl_mapped + CONF_GPMC_BE1N_OFFSET);
        *p9_12_reg = 0x37; // Mode 7, RXACTIVE enabled, Pull-up enabled
        munmap((void *)ctrl_mapped, 0x1000);
    }

    // 2. Map GPIO1 module for reading data
    volatile void *gpio1_mapped = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO1_PHY);
    close(fd);

    if (gpio1_mapped == MAP_FAILED) {
        std::cerr << "Failed to map GPIO1" << std::endl;
        return -1;
    }

    gpio1_addr = (volatile uint32_t *)gpio1_mapped;

    // 3. Set GPIO1_28 as Input (clear bit or set OE depending on your convention; 1 = input for OE on AM335x)
    gpio1_addr[GPIO_OE / 4] |= INPUT_PIN;

    return 0;
}

int read_digital_input() {
    if (!gpio1_addr) return 0;
    uint32_t datain = gpio1_addr[GPIO_DATAIN / 4];
    return (datain & INPUT_PIN) ? 1 : 0;
}