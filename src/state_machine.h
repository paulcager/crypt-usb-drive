#ifndef CRYPTUSBDRIVE_STATE_MACHINE_H
#define CRYPTUSBDRIVE_STATE_MACHINE_H

#include "lwip/pbuf.h"

enum states{
    STARTING,
    CONNECTING,
    FETCHING_KEY,
    READY,
    REFRESHING_KEY,
    EJECTED,
    DEAD
};

extern enum states state;

extern void got_fetch_key_reply(const struct pbuf *buf);
extern void handle_state();
extern char* state_name(enum states state);


#endif //CRYPTUSBDRIVE_STATE_MACHINE_H
