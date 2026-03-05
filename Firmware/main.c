/**
 * Project / Проект: ColdArc-886
 * File / Файл: main.c
 * MCU / МК: PIC16F886
 */

#include "config.h"
#include "hardware.h"
#include "settings.h"
#include "welding_fsm.h"

// --- GLOBAL VARIABLES / ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
volatile uint16_t sys_ms = 0;       // System uptime in ms / Время работы системы в мс
volatile uint16_t state_timer = 0;  // Timer for welding phases / Таймер фаз сварки

// --- INTERRUPT SERVICE ROUTINE / ОБРАБОТЧИК ПРЕРЫВАНИЙ ---
void __interrupt() isr(void) {
    // Timer0 interrupt every 1ms (Internal OSC 8MHz, Prescaler 1:8)
    // Прерывание Timer0 каждые 1мс (Внутр. OSC 8МГц, Делитель 1:8)
    if (T0IF) {                         
        TMR0 = 6;                       // Offset for exact 1ms / Смещение для точности 1мс
        
        if (state_timer > 0) {
            state_timer--;              // Decrement welding timer / Отсчет таймера сварки
        }
        
        sys_ms++;                       // Increment global ticks / Инкремент системных тиков
        T0IF = 0;                       // Clear flag / Сброс флага
    }
}

// --- MAIN LOOP / ГЛАВНЫЙ ЦИКЛ ---
void main(void) {
    // 1. Hardware abstraction layer initialization
    // 1. Инициализация драйверов оборудования
    HW_Init(); 
    
    // 2. Load settings from non-volatile memory (EEPROM)
    // 2. Загрузка настроек из энергонезависимой памяти (EEPROM)
    Settings_Load();

    // 3. Safety Check: Ensure no buttons are stuck on power-up
    // 3. Проверка безопасности: кнопки не должны быть зажаты при включении
    while (!BTN_START || !BTN_UP || !BTN_DOWN) {
        CLRWDT();                       // Reset Watchdog / Сброс сторожевого таймера
        HW_BlinkAlert(sys_ms);          // Blink LEDs to show error / Мигание LED при ошибке
    }
    
    // Reset display to normal after check
    // Возврат индикации в норму после проверки
    HW_UpdateDisplay(cfg.mode); 

    while(1) {
        CLRWDT();                       // Clear Watchdog Timer / Сброс Watchdog

        // A) Welding State Machine Execution
        // А) Выполнение сварочного автомата (состояния процесса)
        FSM_Process();

        // B) User Interface Processing (every 10ms for debounce)
        // Б) Обработка интерфейса (каждые 10мс для подавления дребезга)
        if (sys_ms % 10 == 0) {
            UI_HandleButtons(sys_ms);
        }

        // C) LED Display Update logic
        // В) Логика обновления светодиодной шкалы
        if (edit_state == EDIT_OFF) {
            // Show current mode if not in Menu
            // Показываем режим, если мы не в меню настроек
            HW_UpdateDisplay(cfg.mode);
        }
        // If in Menu, LED_BAR is controlled inside UI_HandleButtons
        // Если в меню, LED_BAR управляется внутри UI_HandleButtons
    }
}