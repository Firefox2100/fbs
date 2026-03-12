#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <bt/bt_service/bt.h>
#include <profiles/serial_profile.h>
#include <gui/gui.h>
#include <gui/view.h>

#define TAG "FlipperBTSerial"
#define FBS_DISPLAY_TEXT_SIZE 64

typedef struct {
    Bt* bt;
    FuriHalBleProfileBase* bt_serial_profile;
    bool bt_connected;
    FuriTimer* tx_timer;
    uint8_t tx_counter;

    char display_text[FBS_DISPLAY_TEXT_SIZE];

    ViewPort* view_port;
    Gui* gui;

    FuriMutex* app_mutex;
    FuriMessageQueue* event_queue;
} FBS;
