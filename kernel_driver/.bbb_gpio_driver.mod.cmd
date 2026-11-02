savedcmd_bbb_gpio_driver.mod := printf '%s\n'   bbb_gpio_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > bbb_gpio_driver.mod
