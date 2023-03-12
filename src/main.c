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

    // Let the last UDP packets get sent out.
    for (uint32_t begin = board_millis(); board_millis() < begin+50; ) {
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();

    for(;;);    // Wait for watchdog to kill us & restart.
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    DEBUG_LOG("Mounted %s", state_name(state));
    if (state == EJECTED) {
        state = FETCHING_KEY;
    }
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    DEBUG_LOG("Unounted", NULL);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    DEBUG_LOG("tud_suspend_cb(%d)", (int)remote_wakeup_en);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
    DEBUG_LOG("tud_resume_cb", NULL);
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
                //tud_connect();
                ready = true;
            }
        } else {
            //DEBUG_LOG("state: %s (tud_connected=%d)", state_name(state), (int)tud_connected());
            if (ready) {
                DEBUG_LOG("Setting disconnected: %s (tud_connected=%d)", state_name(state), (int)tud_connected());
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
//                if (tud_connected()) {
//                    tud_disconnect();
//                }
                ready = false;
                DEBUG_LOG("Disconnected done (tud_connected=%d)", (int)tud_connected());
            }
        }

        if (state != orig_state) {
            DEBUG_LOG("\nState changed from %s to %s, mounted=%d, connected=%d", state_name(orig_state), state_name(state), (int)tud_mounted(), (int)tud_connected());
        }
    }

    DEBUG_LOG("Exit %s", "main_loop");

//    if (tud_connected()) tud_disconnect();
    memset(file_contents, 0, BLOCK_SIZE);
}
