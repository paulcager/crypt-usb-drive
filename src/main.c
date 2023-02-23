/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/watchdog.h"
#include "lwip/apps/http_client.h"

#include "image.h"
#include "state_machine.h"

void main_loop();

int main(void) {
    watchdog_enable(1000, 1);

    stdio_init_all();

    state = STARTING;

    board_init(); // TinyUSB init.

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        state = DEAD;
    }

    cyw43_arch_enable_sta_mode();

    main_loop();

    cyw43_arch_deinit();

    for(;;);    // Wait for watchdog to kill us & restart.
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    printf("Mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    printf("Unmounted\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

uint32_t start_time;

bool connected = false;

void main_loop() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    while (state != DEAD) {
        watchdog_update();
        tud_task(); // tinyusb device task
        cyw43_arch_poll();
        handle_state();

        if (state == READY || state == REFRESHING_KEY) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            if (!connected) {
                tud_connect();
                connected = true;
            }
        } else {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            if (connected) {
                tud_disconnect();
                connected = false;
            }
        }
    }
}
