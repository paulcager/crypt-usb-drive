#include "pico/cyw43_arch.h"
#include "bsp/board.h"
#include "lwip/apps/http_client.h"
#include "pico/unique_id.h"

#include "state_machine.h"
#include "image.h"
#include "debug.h"
#include "secrets.h"

#pragma GCC diagnostic ignored "-Wswitch"

extern void fetchKey();

enum states state;

uint32_t lastSuccessfulFetch;

char *state_name(enum states state) {
    switch (state) {
        case STARTING: return "STARTING";
        case CONNECTING: return "CONNECTING";
        case FETCHING_KEY: return "FETCHING_KEY";
        case READY: return "READY";
        case REFRESHING_KEY: return "REFRESHING_KEY";
        case EJECTED: return "EJECTED";
        case DEAD: return "DEAD";
        default: return "unknown";
    };
}

void got_fetch_key_reply(const struct pbuf *buf) {
    uint8_t buff[PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 6];

    DEBUG_LOG("got_fetch_key_reply(state=%s, len=%d)", state_name(state), buf->tot_len);

    switch (state) {
        case FETCHING_KEY:
        case REFRESHING_KEY:
            pico_get_unique_board_id((pico_unique_board_id_t *) buff);
            cyw43_hal_get_mac(0, buff + PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
            memcpy(file_contents, buff, sizeof(buff));
            memcpy(file_contents + sizeof(buff), key_file_prefix, FILE_PREFIX_LEN);
            int offset = sizeof(buff) + FILE_PREFIX_LEN;
            pbuf_copy_partial(buf, file_contents + offset, BLOCK_SIZE - offset, 0);
            state = READY;
            lastSuccessfulFetch = board_millis();
            break;

        default:
            printf("Unexpected fetch_key_reply in state %d", state);
            state = DEAD;
    }
}

void handle_state() {
    switch (state) {
        case STARTING:
            state = CONNECTING;

            int result = cyw43_arch_wifi_connect_async(MY_WIFI_SSID, MY_WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
            if (result) {
                printf("Failed to connect: %d\n", result);
                state = DEAD;
                return;
            }
            break;

        case CONNECTING:
            (void) NULL;
            int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

            if (link_status < 0) {
                // An error
                printf("cyw43_tcpip_link_status is %d\n", link_status);
                state = DEAD;
                return;
            }

            if (link_status == CYW43_LINK_UP) {
                state = FETCHING_KEY;
                fetchKey();
                return;
            }

            // Otherwise, still in progress.

            return;

        case FETCHING_KEY:
            // No action necessary - state will be updated when HTTP GET finishes.
            return;

        case READY:
            if (board_millis() >= lastSuccessfulFetch + REFRESH_INTERVAL) {
                state = REFRESHING_KEY;
                fetchKey();
            }

            return;

        case EJECTED:
            // No action necessary.
            return;
    }
}


static struct pbuf *http_body;

void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    DEBUG_LOG("transfer complete, httpc_result=%d, HTTP result=%d, content-len=%d", httpc_result, (int) srv_res, (int) rx_content_len);

    switch (state) {
        case FETCHING_KEY:
        case REFRESHING_KEY:
            if (httpc_result != HTTPC_RESULT_OK ||
                rx_content_len == 0 ||
                srv_res != 200 ||
                http_body == NULL) {
                DEBUG_LOG("Set state to DEAD(%d)", DEAD);
                state = DEAD;
                return;
            }

            got_fetch_key_reply(http_body);
            return;
    }

    pbuf_free(http_body);
    http_body = NULL;
}


err_t body(void *arg, struct tcp_pcb *conn, struct pbuf *p, err_t err) {
    DEBUG_LOG("body: err=%d, len=%d", err, p->tot_len);
    if (http_body != NULL)
        pbuf_free(http_body);

    http_body = p;

    return ERR_OK;
}

static httpc_connection_t settings;

void fetchKey() {
    settings.result_fn = http_result;
    err_t err = httpc_get_file_dns(
            KEY_SERVER,
            80,
            KEY_FILE,
            &settings,
            body,
            NULL,
            NULL
    );

    DEBUG_LOG("httpc_get_file_dns: %d", err);

    if (err) {
        state = DEAD;
    }
}