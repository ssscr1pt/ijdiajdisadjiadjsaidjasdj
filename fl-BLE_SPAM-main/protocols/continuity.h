#pragma once

#include "_base.h"

// Объявляем все типы атак, которые требуются для массива attacks[] в ble_spam.c
typedef enum {
    ContinuityTypeProximityPair,
    ContinuityTypeAppleAction,
    ContinuityTypeCustomCrash,   // Добавлено для строки 228
    ContinuityTypeNearbyAction,  // Добавлено для строки 245
} ContinuityType;

// Создаем псевдоним типа, который требует линкер в _registry.h (строка 8)
typedef struct {
    ContinuityType type;
} ContinuityMsg;

extern const BleSpamProtocol ble_spam_protocol_continuity;
