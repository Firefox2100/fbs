#include "fbs.h"

static const uint8_t FBS_ECHO_PADDING[] = {0x0F, 0xF0};

static void fbs_set_display_text(FBS* app, const char* text) {
    furi_check(furi_mutex_acquire(app->app_mutex, FuriWaitForever) == FuriStatusOk);
    snprintf(app->display_text, FBS_DISPLAY_TEXT_SIZE, "%s", text);
    furi_mutex_release(app->app_mutex);
}

void draw_callback(Canvas* canvas, void* ctx) {
    FBS* app = ctx;
    furi_check(furi_mutex_acquire(app->app_mutex, FuriWaitForever) == FuriStatusOk);

    canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, app->display_text);

    furi_mutex_release(app->app_mutex);
}

void input_callback(InputEvent* input, void* ctx) {
    FBS* app = ctx;
    FbsEvent event = {
        .type = FbsEventTypeInput,
        .input = *input,
    };
    furi_message_queue_put(app->event_queue, &event, FuriWaitForever);
}

static void fbs_bt_status_changed_callback(BtStatus status, void* context) {
    FBS* app = context;

    app->bt_connected = (status == BtStatusConnected);
    switch(status) {
    case BtStatusConnected:
        FURI_LOG_D(TAG, "BT status: connected");
        fbs_set_display_text(app, "BLE connected");
        break;
    case BtStatusAdvertising:
        FURI_LOG_D(TAG, "BT status: advertising");
        fbs_set_display_text(app, "Advertising BLE serial");
        break;
    case BtStatusOff:
        FURI_LOG_D(TAG, "BT status: off");
        fbs_set_display_text(app, "Bluetooth off");
        break;
    case BtStatusUnavailable:
    default:
        FURI_LOG_D(TAG, "BT status: unavailable");
        fbs_set_display_text(app, "Bluetooth unavailable");
        break;
    }
}

static bool fbs_send_echo(FBS* app, const uint8_t* data, size_t size) {
    furi_check(app);
    furi_check(data);

    if(!app->bt_serial_profile || !app->bt_connected) {
        return false;
    }

    if(size > (BLE_PROFILE_SERIAL_PACKET_SIZE_MAX - sizeof(FBS_ECHO_PADDING) * 2U)) {
        FURI_LOG_W(TAG, "RX packet too large to echo: %u", size);
        fbs_set_display_text(app, "RX too large to echo");
        return false;
    }

    size_t offset = 0;

    memcpy(app->echo_buffer + offset, FBS_ECHO_PADDING, sizeof(FBS_ECHO_PADDING));
    offset += sizeof(FBS_ECHO_PADDING);
    memcpy(app->echo_buffer + offset, data, size);
    offset += size;
    memcpy(app->echo_buffer + offset, FBS_ECHO_PADDING, sizeof(FBS_ECHO_PADDING));
    offset += sizeof(FBS_ECHO_PADDING);

    bool sent = ble_profile_serial_tx(app->bt_serial_profile, app->echo_buffer, offset);
    FURI_LOG_D(TAG, "Echoed %u bytes, sent=%d", offset, sent);
    return sent;
}

static void fbs_handle_rx(FBS* app, const uint8_t* data, size_t size) {
    char data_text[FBS_DISPLAY_TEXT_SIZE];
    size_t offset = snprintf(data_text, sizeof(data_text), "RX:");

    FURI_LOG_D(TAG, "Handling RX packet of %u bytes", size);
    for(size_t i = 0; i < size; i++) {
        printf("%X ", data[i]);
        if(offset < sizeof(data_text)) {
            offset += snprintf(data_text + offset, sizeof(data_text) - offset, " %02X", data[i]);
        }
    }
    printf("\r\n");

    fbs_set_display_text(app, data_text);
    fbs_send_echo(app, data, size);
    if(app->bt_serial_profile) {
        ble_profile_serial_notify_buffer_is_empty(app->bt_serial_profile);
    }
}

FBS* fbs_alloc() {
    FBS* app = malloc(sizeof(FBS));
    app->app_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->event_queue = furi_message_queue_alloc(8, sizeof(FbsEvent));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->bt_connected = false;
    app->bt = furi_record_open(RECORD_BT);
    app->bt_serial_profile = NULL;
    snprintf(app->display_text, FBS_DISPLAY_TEXT_SIZE, "Waiting for BLE data");
    return app;
}

void fbs_free(FBS* app) {
    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    app->gui = NULL;
    view_port_free(app->view_port);

    furi_mutex_free(app->app_mutex);
    furi_message_queue_free(app->event_queue);

    furi_record_close(RECORD_BT);
    app->bt = NULL;

    free(app);
}

static uint16_t bt_serial_event_callback(SerialServiceEvent event, void* context) {
    furi_assert(context);
    FBS* app = context;
    uint16_t ret = BT_SERIAL_BUFFER_SIZE;

    if(event.event == SerialServiceEventTypeDataReceived) {
        FURI_LOG_D(TAG, "SerialServiceEventTypeDataReceived");
        FURI_LOG_D(TAG, "Size: %u", event.data.size);
        if(event.data.size <= FBS_RX_BUFFER_SIZE) {
            FbsEvent queue_event = {
                .type = FbsEventTypeBleRx,
            };
            queue_event.ble_rx.size = event.data.size;
            memcpy(queue_event.ble_rx.data, event.data.buffer, event.data.size);
            if(furi_message_queue_put(app->event_queue, &queue_event, 0) != FuriStatusOk) {
                FURI_LOG_W(TAG, "Dropping RX packet: app queue full");
            }
        } else {
            FURI_LOG_W(TAG, "Dropping RX packet: %u exceeds queue buffer", event.data.size);
        }
    } else if(event.event == SerialServiceEventTypeDataSent) {
        FURI_LOG_D(TAG, "SerialServiceEventTypeDataSent");
    } else if(event.event == SerialServiceEventTypesBleResetRequest) {
        FURI_LOG_D(TAG, "SerialServiceEventTypesBleResetRequest");
        fbs_set_display_text(app, "BLE reset requested");
    }
    return ret;
}

int32_t fbs_app(void* p) {
    UNUSED(p);
    FBS* app = fbs_alloc();

    FURI_LOG_D(TAG, "Switching to the serial profile...");
    app->bt_serial_profile = bt_profile_start(app->bt, ble_profile_serial, NULL);
    if(app->bt_serial_profile) {
        bt_set_status_changed_callback(app->bt, fbs_bt_status_changed_callback, app);
        ble_profile_serial_set_event_callback(
            app->bt_serial_profile, BT_SERIAL_BUFFER_SIZE, bt_serial_event_callback, app);
        furi_hal_bt_start_advertising();
        fbs_set_display_text(app, "Advertising BLE serial");
    } else {
        FURI_LOG_E(TAG, "Failed to start BLE serial profile");
        fbs_set_display_text(app, "Serial profile failed");
    }
    
    FbsEvent event;
    for(bool processing = true; processing;) {
        int status = furi_message_queue_get(app->event_queue, &event, 100);
        if(status == FuriStatusOk) {
            if(event.type == FbsEventTypeInput) {
                furi_check(furi_mutex_acquire(app->app_mutex, FuriWaitForever) == FuriStatusOk);
                if(event.input.type == InputTypePress && event.input.key == InputKeyBack) {
                    processing = false;
                }
                furi_mutex_release(app->app_mutex);
            } else if(event.type == FbsEventTypeBleRx) {
                fbs_handle_rx(app, event.ble_rx.data, event.ble_rx.size);
            }
        }
        view_port_update(app->view_port);
    }

    if(app->bt_serial_profile) {
        ble_profile_serial_set_event_callback(app->bt_serial_profile, 0, NULL, NULL);
        bt_set_status_changed_callback(app->bt, NULL, NULL);
        bt_profile_restore_default(app->bt);
    }

    fbs_free(app);
    FURI_LOG_D(TAG, "Released everything");
    return 0;
}
