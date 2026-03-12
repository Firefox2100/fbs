#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <bt/bt_service/bt.h>
#include <gui/gui.h>
#include <gui/view.h>

#include "ble_serial.h"

#define TAG "FlipperBTSerial"
#define BT_SERIAL_BUFFER_SIZE 128
#define FBS_DISPLAY_TEXT_SIZE 64
#define FBS_RX_BUFFER_SIZE BT_SERIAL_BUFFER_SIZE
#define FBS_ECHO_BUFFER_SIZE BLE_PROFILE_SERIAL_PACKET_SIZE_MAX

typedef enum {
    FbsEventTypeInput,
    FbsEventTypeBleRx,
} FbsEventType;

typedef struct {
    FbsEventType type;
    union {
        InputEvent input;
        struct {
            uint16_t size;
            uint8_t data[FBS_RX_BUFFER_SIZE];
        } ble_rx;
    };
} FbsEvent;

typedef struct {
    Bt* bt;
    FuriHalBleProfileBase* bt_serial_profile;
    bool bt_connected;

    char display_text[FBS_DISPLAY_TEXT_SIZE];
    uint8_t echo_buffer[FBS_ECHO_BUFFER_SIZE];

    ViewPort* view_port;
    Gui* gui;

    FuriMutex* app_mutex;
    FuriMessageQueue* event_queue;
} FBS;
