/**
 * File / Файл: settings.h
 * Description: EEPROM storage and Menu logic (UP+DOWN combo)
 * Описание: Хранение в EEPROM и логика меню (комбинация UP+DOWN)
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

// --- STRUCTURE FOR SETTINGS / СТРУКТУРА НАСТРОЕК ---
typedef struct {
    uint8_t mode;        // 2T, 4T, Cold, Tack
    uint8_t pre_gas;     // 0.1 - 0.5 sec (values 1-5)
    uint8_t post_gas;    // 0.5 - 1.0 sec (values 5-10)
    uint8_t arc_init;    // HF time (0.1 - 3.0 sec)
    uint8_t pulse_time;  // Cold pause (0.1 - 0.5 sec)
} settings_t;

// --- MENU STATES / СОСТОЯНИЯ МЕНЮ ---
typedef enum {
    EDIT_OFF = 0,        // Normal operation / Рабочий режим
    EDIT_PRE_GAS,        // LED 1 / Настройка пред-газа
    EDIT_POST_GAS,       // LED 2 / Настройка пост-газа
    EDIT_ARC,            // LED 3 / Настройка поджига
    EDIT_COLD_PAUSE,     // LED 4 / Пауза холодного импульса
    EDIT_MAX_STATES
} edit_mode_t;

// External variables for other modules / Внешние переменные для других модулей
extern settings_t cfg;
extern edit_mode_t edit_state;

// Functions / Функции
void Settings_Load(void);
void Settings_Save(void);
void UI_HandleButtons(uint16_t ms_tick);

#endif // SETTINGS_H