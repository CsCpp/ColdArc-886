/**
 * File / Файл: hardware.c
 * Description: Implementation of MCU peripheral control
 * Описание: Реализация управления периферией МК
 */

#include "hardware.h"
#include "config.h"

void HW_Init(void) {
    // 1. Internal Oscillator setup (8 MHz) 
    // Настройка внутреннего генератора (8 МГц)
    OSCCON = 0x71; 
    
    // 2. Disable analog functions on all pins
    // Отключение аналоговых функций (все пины цифровые)
    ANSEL = 0; 
    ANSELH = 0;
    
    // 3. Port Directions / Настройка направлений портов
    TRISA = 0x00;        // PORTA as output (LED Bar) / PORTA на выход (Шкала)
    TRISB = 0x38;        // RB3, RB4, RB5 as inputs / Кнопки на вход
    TRISCbits.TRISC0 = 0; // RC0 as output (HF Arc) / Выход поджига
    
    // 4. Initial Outputs State / Начальное состояние выходов
    PIN_LED_BAR = 0xFF;  // All LEDs OFF (Common Anode assumed) / Все LED выкл
    PIN_GAS = 0;         // Gas OFF / Газ выкл
    PIN_WELD = 1;        // Weld OFF (Active Low) / Ток выкл
    PIN_ARC = 0;         // HF OFF / Поджиг выкл
    
    // 5. Timer0 Setup / Настройка Timer0
    // Prescaler 1:8, Fosc/4 -> 1 tick = 1 microsecond * 8 = 4us per count
    // To get 1ms: 1000us / 4us = 250 counts. 256 - 250 = 6 (TMR0 start value)
    OPTION_REG = 0x02;   // Prescaler 1:8 / Делитель 1:8
    TMR0 = 6;            // Initialize timer for 1ms / Инициализация на 1мс
    T0IE = 1;            // Enable Timer0 Interrupt / Разрешить прерывание T0
    GIE = 1;             // Global Interrupt Enable / Разрешить прерывания глобально
}

void HW_SetRelays(uint8_t gas, uint8_t weld, uint8_t arc) {
    PIN_GAS = gas;
    PIN_WELD = weld;     // Remember: 0 is ON for current allow
    PIN_ARC = arc;
}

void HW_UpdateDisplay(uint8_t mode) {
    // Basic mode display: L1=2T, L2=4T, L3=Cold, L4=Tack
    // Простая индикация: L1=2T, L2=4T, L3=Cold, L4=Tack
    PIN_LED_BAR = ~(1 << (mode & 0x07));
}

void HW_BlinkAlert(uint16_t ms_tick) {
    // Blink LEDs 1 and 8 to signal a stuck button error
    // Мигаем 1-м и 8-м светодиодами при ошибке залипания
    if ((ms_tick / 250) % 2) {
        PIN_LED_BAR = 0b01111110; // Active Low: LEDs 1 & 8 ON
    } else {
        PIN_LED_BAR = 0xFF;       // All OFF
    }
}