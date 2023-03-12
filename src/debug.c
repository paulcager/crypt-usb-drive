#include <stdio.h>
#include <lwip/ip4_addr.h>
#include <lwip/udp.h>
#include <stdarg.h>
#include "debug.h"
#include "secrets.h"

#ifdef UDP_LOGGING_ON
static char buff[1024];
static struct udp_pcb* udp_pcb;

void __debug_log(const char *fmt, ...) {
    if (udp_pcb == NULL) {
        udp_pcb = udp_new();
        ip4_addr_t  debug_address;
        ip4addr_aton(DEBUG_HOST, &debug_address);
        udp_connect(udp_pcb, &debug_address, DEBUG_PORT);
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buff, sizeof(buff)-1, fmt, args);
    va_end(args);
    buff[n++] = '\n';

    struct pbuf* pb = pbuf_alloc(PBUF_TRANSPORT, n, PBUF_REF);
    pb->payload = buff;
    pb->len = pb->tot_len = n;
    udp_send(udp_pcb, pb);
    pbuf_free(pb);
}

#endif