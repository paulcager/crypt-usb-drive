#include <stdlib.h>
#include <memory.h>
#include "image.h"
#include "bsp/board.h"
#include "tusb.h"
#include "state_machine.h"
#include "debug.h"

extern uint32_t start_time;

int32_t copy_block(const int lba, const uint8_t* block, uint32_t offset, void* buffer, uint32_t bufsize);



// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  if ( lba >= NUM_BLOCKS ) {
      return -1;
  }

  if (state == EJECTED) {
      return -1;
  }

  return copy_block(lba, disk_blocks[lba], offset, buffer, bufsize);
}

int32_t copy_block(const int lba, const uint8_t* block, uint32_t offset, void* buffer, uint32_t bufsize) {
    if (offset + bufsize > BLOCK_SIZE) {
        bufsize = BLOCK_SIZE - offset;
    }

    memcpy(buffer, block + offset, bufsize);
    return (int32_t)bufsize;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    DEBUG_LOG("Mounted %s", state_name(state));
    if (state == EJECTED) {
        state = CONNECTING;
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


// Invoked to determine max LUN
uint8_t tud_msc_get_maxlun_cb(void)
{
    return 1;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void) lun;

    const char vid[] = "cager";
    const char pid[] = "Mass Storage";
    const char rev[] = "1.0";

    memcpy(vendor_id  , vid, strlen(vid));
    memcpy(product_id , pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    return state == READY || state == REFRESHING_KEY;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    (void) lun;

    *block_count = NUM_BLOCKS;
    *block_size  = BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    DEBUG_LOG("tud_msc_start_stop_cb: power=%d, start=%d, load=%d", (int)power_condition, (int)start, (int)load_eject);

    if ( load_eject ) {
        if (start) {
            // Let state machine notice we are connected, and need to fetch key.
            state = CONNECTING;
        } else {
            state = EJECTED;
        }
    }

    return true;
}


bool tud_msc_is_writable_cb (uint8_t lun)
{
    (void) lun;

    return false;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    (void) lun; (void) lba; (void) offset; (void) buffer;
    return -1;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    // read10 & write10 has their own callback and MUST not be handled here

    DEBUG_LOG("tud_msc_scsi_cb: %d [%d] - state=%s", (int)scsi_cmd[0], (int)scsi_cmd[4], state_name(state));

    void const* response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0])
    {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            resplen = 0;
//            if ((scsi_cmd[4] & 0x03) == 0) {
//                tud_msc_set_sense(0, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
//                state = EJECTED;
//            } else if (state == EJECTED) {
//                state = CONNECTING;
//            }

            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }

    // return resplen must not larger than bufsize
    if ( resplen > bufsize ) resplen = bufsize;

    if ( response && (resplen > 0) )
    {
        if(in_xfer)
        {
            memcpy(buffer, response, (size_t) resplen);
        }else
        {
            // SCSI output
        }
    }

    return resplen;
}