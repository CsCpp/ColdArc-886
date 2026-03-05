/**
 * File / Файл: settings.c
 * Description: Implementation of settings logic and button combinations
 * Описание: Реализация логики настроек и комбинаций кнопок
 */

#include "settings.h"
#include "hardware.h"
#include "config.h"
#include <xc.h>

settings_t cfg;
edit_mode_t edit_state = EDIT_OFF;

static uint16_t combo_timer = 0;
static uint8_t combo_lock = 0;

void Settings_Load(void) {
    cfg.mode = eeprom_read(0x00);
    cfg.pre_gas = eeprom_read(0x01);
    cfg.post_gas = eeprom_read(0x02);
    cfg.arc_init = eeprom_read(0x03);
    cfg.pulse_time = eeprom_read(0x04);

    // Initial check (EEPROM is 0xFF when empty)
    // Проверка при первом запуске (если EEPROM пуст)
    if (cfg.mode > 3) cfg.mode = 0;
    if (cfg.pre_gas < 1 || cfg.pre_gas > 5) cfg.pre_gas = 2;   // Default 0.2s
    if (cfg.post_gas < 5 || cfg.post_gas > 10) cfg.post_gas = 7; // Default 0.7s
    if (cfg.arc_init > 30) cfg.arc_init = 10;                // Default 1.0s
    if (cfg.pulse_time > 5) cfg.pulse_time = 3;               // Default 0.3s
}

void Settings_Save(void) {
    if (eeprom_read(0x00) != cfg.mode) eeprom_write(0x00, cfg.mode);
    if (eeprom_read(0x01) != cfg.pre_gas) eeprom_write(0x01, cfg.pre_gas);
    if (eeprom_read(0x02) != cfg.post_gas) eeprom_write(0x02, cfg.post_gas);
    if (eeprom_read(0x03) != cfg.arc_init) eeprom_write(0x03, cfg.arc_init);
    if (eeprom_read(0x04) != cfg.pulse_time) eeprom_write(0x04, cfg.pulse_time);
}

void UI_HandleButtons(uint16_t ms_tick) {
    // --- COMBO LOGIC (UP + DOWN) / ЛОГИКА КОМБИНАЦИИ ---
    if (!BTN_UP && !BTN_DOWN) {
        combo_timer++;
        // Long press (>1s): Enter or Save & Exit
        // Длинное нажатие (>1с): Вход или Сохранение и Выход
        if (combo_timer > 100 && combo_lock == 0) {
            if (edit_state == EDIT_OFF) edit_state = EDIT_PRE_GAS;
            else {
                edit_state = EDIT_OFF;
                Settings_Save();
            }
            combo_lock = 1;
        }
    } else {
        // Short press (<0.8s): Navigation in Menu
        // Короткое нажатие (<0.8с): Переход по пунктам меню
        if (combo_timer > 5 && combo_timer < 80 && edit_state != EDIT_OFF) {
            edit_state++;
            if (edit_state >= EDIT_MAX_STATES) edit_state = EDIT_PRE_GAS;
        }
        combo_timer = 0;
        combo_lock = 0;
    }

    // --- MENU INTERFACE / РАБОТА В МЕНЮ ---
    if (edit_state != EDIT_OFF) {
        // Blink active parameter LED / Мигание текущего светодиода
        if ((ms_tick / 200) % 2) PIN_LED_BAR = ~(1 << (edit_state - 1));
        else PIN_LED_BAR = 0xFF;

        // Modify values / Изменение значений
        if (!BTN_UP) { // Plus / Плюс
            switch(edit_state) {
                case EDIT_PRE_GAS:  if(cfg.pre_gas < 5) cfg.pre_gas++; break;
                case EDIT_POST_GAS: if(cfg.post_gas < 10) cfg.post_gas++; break;
                case EDIT_ARC:      if(cfg.arc_init < 30) cfg.arc_init++; break;
            }
            __delay_ms(150);
        }
        if (!BTN_DOWN) { // Minus / Минус
            switch(edit_state) {
                case EDIT_PRE_GAS:  if(cfg.pre_gas > 1) cfg.pre_gas--; break;
                case EDIT_POST_GAS: if(cfg.post_gas > 5) cfg.post_gas--; break;
                case EDIT_ARC:      if(cfg.arc_init > 1) cfg.arc_init--; break;
            }
            __delay_ms(150);
        }
        
        // Torch button emergency exit / Выход по нажатию курка
        if (!BTN_START) { edit_state = EDIT_OFF; Settings_Save(); }
    } 
    // --- REGULAR OPERATION / ОБЫЧНЫЙ РЕЖИМ ---
    else {
        if (!BTN_UP) { // Mode switch / Смена режима
            cfg.mode = (cfg.mode + 1) % 4;
            __delay_ms(300);
        }
        // Manual Gas Test / Тест газа
        if (!BTN_DOWN) PIN_GAS = 1; 
        else PIN_GAS = 0;
    }
}