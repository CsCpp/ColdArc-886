/**
 * File / Файл: config.h
 * Description: MCU configuration and pin definitions
 * Описание: Настройки микроконтроллера и определение пинов
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

// --- CONFIGURATION BITS / БИТЫ КОНФИГУРАЦИИ ---
// Internal Oscillator, Watchdog ON, Power-up Timer ON, Master Clear ON
#pragma config FOSC = INTRC_NOCLKOUT // Internal oscillator / Внутренний генератор
#pragma config WDTE = ON             // Watchdog Timer Enabled / Сторожевой таймер ВКЛ
#pragma config PWRTE = ON            // Power-up Timer Enabled / Таймер задержки питания
#pragma config MCLRE = ON            // MCLR pin is Master Clear / Пин сброса активен
#pragma config CP = OFF              // Code Protection OFF / Защита кода ВЫКЛ
#pragma config CPD = OFF             // Data Protection OFF / Защита данных ВЫКЛ
#pragma config BOREN = ON            // Brown-out Reset Enabled / Сброс при падении питания
#pragma config IESO = OFF            // Internal/External Switchover OFF
#pragma config FCMEN = OFF           // Fail-Safe Clock Monitor OFF
#pragma config LVP = OFF             // Low Voltage Programming OFF / Низковольтное прогр. ВЫКЛ

// --- SYSTEM CONSTANTS / СИСТЕМНЫЕ КОНСТАНТЫ ---
#define _XTAL_FREQ 8000000           // 8MHz for __delay_ms / 8МГц для расчетов задержек

// --- PIN MAPPING / КАРТА ПИНОВ ---

// Outputs / Выходы
#define PIN_GAS      PORTBbits.RB1   // Gas Valve Relay / Реле клапана газа
#define PIN_WELD     PORTBbits.RB2   // Welding Current Allow (Active Low) / Разрешение тока (Активный 0)
#define PIN_ARC      PORTCbits.RC0   // HF Ignition / Поджиг (Осциллятор)
#define PIN_LED_BAR  PORTA           // 8-LED Scale on PORTA / Шкала из 8 LED на ПОРТУ А

// Inputs / Входы
#define BTN_START    PORTBbits.RB3   // Torch Trigger / Кнопка на горелке
#define BTN_UP       PORTBbits.RB4   // Up Button / Кнопка ВВЕРХ
#define BTN_DOWN     PORTBbits.RB5   // Down Button / Кнопка ВНИЗ

// --- TIMEOUTS / ТАЙМАУТЫ ---
#define SAFETY_MAX_WELD_SEC 120      // Max continuous weld time / Макс. время дуги (сек)

#endif // CONFIG_H