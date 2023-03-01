#ifndef CRYPTUSBDRIVE_SECRETS_H
#define CRYPTUSBDRIVE_SECRETS_H

#include "stdint.h"

//
//  Modify this file to match your requirements, and save as "secrets.h"
//

#define MY_WIFI_SSID "MySSID"
#define MY_WIFI_PASSWORD "MySecretPassword"


#define KEY_SERVER "my-server.example.org"
#define KEY_FILE "/my-key-file.bin"

#define FILE_PREFIX {0xdc, 0x5d, 0xbc, 0x4c, 0xc4, 0x6e, 0x4d, 0x39, 0x7f, 0x4b, 0x18, 0x69, 0x29, 0x7f, 0x3f, 0x51}
#define FILE_PREFIX_LEN 16

extern uint8_t key_file_prefix[];

#define REFRESH_INTERVAL 55000
#define FETCH_TIMEOUT    30000

#define DEBUG_HOST "192.168.99.99"
#define DEBUG_PORT 2020

#endif //CRYPTUSBDRIVE_SECRETS_H
