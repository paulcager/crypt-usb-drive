#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/watchdog.h"

#include "image.h"
#include "state_machine.h"
#include "debug.h"

void main_loop();

int main(void) {
    watchdog_enable(1000, 1);

    stdio_init_all();

    state = STARTING;

    board_init(); // TinyUSB init.

    // NB: defer initialising tud, until we have a disk ready.

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        state = DEAD;
    }

    cyw43_arch_enable_sta_mode();

    main_loop();

    DEBUG_LOG("Exit at %ld", (long)board_millis());

    cyw43_arch_deinit();

    for(;;);    // Wait for watchdog to kill us & restart.
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    DEBUG_LOG("Mounted", NULL);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    DEBUG_LOG("Unounted", NULL);
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

void main_loop() {
    static bool ready;

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    ready = false;

    while (state != DEAD) {
        enum states orig_state = state;

        watchdog_update();
        tud_task();
        cyw43_arch_poll();
        handle_state();

        if (state == READY || state == REFRESHING_KEY) {
            // init device stack on configured roothub port
            tud_init(BOARD_TUD_RHPORT);

            if (!ready) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                tud_connect();
                ready = true;
            }
        } else {
            if (ready) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                tud_disconnect();
                ready = false;
            }
        }

        if (state != orig_state) {
            DEBUG_LOG("\nState changed from %d to %d, mounted=%d", orig_state, state, (int)tud_mounted());
        }
    }

    DEBUG_LOG("Exit %s", "main_loop");

    tud_disconnect();
    memset(file_contents, 0, BLOCK_SIZE);
}
