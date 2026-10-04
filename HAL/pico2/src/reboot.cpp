#include "reboot.hpp"

#include "stdlib.hpp"

void reboot_firmware() {
    rp2350.reboot();
}

void reboot_bootloader() {
    rp2350.rebootToBootloader();
}
