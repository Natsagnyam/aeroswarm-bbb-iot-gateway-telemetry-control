#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "bbb_custom_gpio"
#define CLASS_NAME  "bbb_gpio"
#define GPIO0_PHYS_BASE 0x44e07000
#define GPIO_MAP_SIZE   0x1000

/* AM335x GPIO Register Offsets */
#define GPIO_OE           0x134
#define GPIO_DATAIN       0x138
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT   0x194

static int major_number;
static struct class* gpio_class = NULL;
static struct device* gpio_device = NULL;
static void __iomem *gpio_base;
static struct platform_device *custom_pdev = NULL;

/* User-space command structure: 8 bytes total */
struct gpio_cmd {
    u32 mask;   /* Bitmask of GPIO pins to modify */
    u32 value;  /* 1 = Set high (SETDATAOUT), 0 = Set low (CLEARDATAOUT) */
};

static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    u32 data_in;
    if (*offset > 0) return 0;

    data_in = ioread32(gpio_base + GPIO_DATAIN);

    if (copy_to_user(buffer, &data_in, sizeof(u32))) {
        return -EFAULT;
    }

    *offset += sizeof(u32);
    return sizeof(u32);
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    struct gpio_cmd cmd;

    if (len < sizeof(struct gpio_cmd))
        return -EINVAL;

    if (copy_from_user(&cmd, buffer, sizeof(struct gpio_cmd)))
        return -EFAULT;

    if (cmd.value) {
        /* Set specified pins high */
        iowrite32(cmd.mask, gpio_base + GPIO_SETDATAOUT);
    } else {
        /* Set specified pins low */
        iowrite32(cmd.mask, gpio_base + GPIO_CLEARDATAOUT);
    }

    return sizeof(struct gpio_cmd);
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int bbb_gpio_probe(struct platform_device *pdev) {
    pr_info("%s: Probing Custom GPIO Driver with Read/Write\n", DRIVER_NAME);

    gpio_base = ioremap(GPIO0_PHYS_BASE, GPIO_MAP_SIZE);
    if (!gpio_base) {
        pr_err("%s: Failed to ioremap register space\n", DRIVER_NAME);
        return -ENOMEM;
    }

    major_number = register_chrdev(0, DRIVER_NAME, &fops);
    if (major_number < 0) {
        pr_err("%s: Failed to register major number\n", DRIVER_NAME);
        iounmap(gpio_base);
        return major_number;
    }

    gpio_class = class_create(CLASS_NAME);
    if (IS_ERR(gpio_class)) {
        unregister_chrdev(major_number, DRIVER_NAME);
        iounmap(gpio_base);
        return PTR_ERR(gpio_class);
    }

    gpio_device = device_create(gpio_class, NULL, MKDEV(major_number, 0), NULL, DRIVER_NAME);
    if (IS_ERR(gpio_device)) {
        class_destroy(gpio_class);
        unregister_chrdev(major_number, DRIVER_NAME);
        iounmap(gpio_base);
        return PTR_ERR(gpio_device);
    }

    pr_info("%s: Driver loaded successfully at virtual address 0x%p\n", DRIVER_NAME, gpio_base);
    return 0;
}

static void bbb_gpio_remove(struct platform_device *pdev) {
    if (gpio_device) device_destroy(gpio_class, MKDEV(major_number, 0));
    if (gpio_class) {
        class_unregister(gpio_class);
        class_destroy(gpio_class);
    }
    unregister_chrdev(major_number, DRIVER_NAME);
    if (gpio_base) iounmap(gpio_base);
    pr_info("%s: Driver unloaded successfully\n", DRIVER_NAME);
}

static struct platform_driver bbb_gpio_driver = {
    .probe = bbb_gpio_probe,
    .remove = bbb_gpio_remove,
    .driver = {
        .name = DRIVER_NAME,
    },
};

static int __init bbb_gpio_init(void) {
    int ret;
    ret = platform_driver_register(&bbb_gpio_driver);
    if (ret) return ret;

    custom_pdev = platform_device_register_simple(DRIVER_NAME, 0, NULL, 0);
    if (IS_ERR(custom_pdev)) {
        platform_driver_unregister(&bbb_gpio_driver);
        return PTR_ERR(custom_pdev);
    }
    return 0;
}

static void __exit bbb_gpio_exit(void) {
    if (custom_pdev) platform_device_unregister(custom_pdev);
    platform_driver_unregister(&bbb_gpio_driver);
}

module_init(bbb_gpio_init);
module_exit(bbb_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Natsagnyam Namkhai");
MODULE_DESCRIPTION("Standalone Memory-Mapped GPIO Driver with Read/Write for BBB");
MODULE_VERSION("2.2");
