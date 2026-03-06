#include <xc.h>
#include <stdint.h>

/**
 * =============================================================================
 * 1. CONFIGURATION BITS / БИТЫ КОНФИГУРАЦИИ
 * =============================================================================
 * Detailed settings for PICkit 3 stability.
 * Детальные настройки для стабильной работы с программатором.
 */

// Internal Oscillator, No Clock Out (RA6/RA7 as IO)
// Внутренний генератор, ножки RA6/RA7 свободны для индикации
#pragma config FOSC = INTRC_NOCLKOUT 
#pragma config WDTE = OFF       // Watchdog Timer (OFF for development / ВЫКЛ при разработке)
#pragma config PWRTE = ON       // Power-up Timer (Stabilizes voltage / Стабилизация питания)
#pragma config MCLRE = ON       // MCLR pin as Reset (External 10k resistor / Внешний сброс)
#pragma config CP = OFF         // Code Protection OFF / Защита кода ВЫКЛ
#pragma config CPD = OFF        // EEPROM Protection OFF / Защита EEPROM ВЫКЛ
#pragma config BOREN = ON       // Brown-out Reset (Reset on low voltage / Сброс при просадке)
#pragma config IESO = OFF       // Internal/External Switchover OFF / Переключение тактов ВЫКЛ
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor OFF / Монитор тактов ВЫКЛ
#pragma config LVP = OFF        // Low Voltage Programming OFF (Critical for PK3 / ВАЖНО для PK3)

#define _XTAL_FREQ 8000000      // 8MHz internal clock / Частота 8МГц

/**
 * =============================================================================
 * 2. HARDWARE MAPPING / ПЕРИФЕРИЯ
 * =============================================================================
 */
#define PIN_GAS      PORTCbits.RC0   // Gas Valve / Газовый клапан
#define PIN_WELD     PORTBbits.RB2   // Main Weld Relay (Active Low / Активный 0)
#define PIN_ARC      PORTBbits.RB1   // HF Ignition (Arc) / Осциллятор (Поджиг)
#define PIN_LED_BAR  PORTA           // LED Scale (Common Anode / Общий анод)

#define BTN_START    PORTBbits.RB3   // Torch Trigger / Кнопка горелки
#define BTN_UP       PORTBbits.RB4   // Menu Up / Кнопка Вверх
#define BTN_DOWN     PORTBbits.RB5   // Menu Down / Кнопка Вниз

/**
 * =============================================================================
 * 3. LOGIC TYPES & VARIABLES / ТИПЫ И ПЕРЕМЕННЫЕ
 * =============================================================================
 */

// Finite State Machine (FSM) States / Состояния автомата сварки
typedef enum { 
    S_IDLE,          // Waiting for trigger / Ожидание нажатия
    S_PRE_GAS,       // Pre-flow gas / Предпродувка
    S_ARC_INIT,      // Arc start delay / Задержка розжига
    S_WELD_2T,       // Continuous weld (2T) / Сварка 2Т
    S_WELD_4T_1,     // 4T: Trigger pressed 1 / 4Т: Первое нажатие
    S_WELD_4T_2,     // 4T: Trigger released (welding) / 4Т: Сварка (кнопка отпущена)
    S_WELD_4T_3,     // 4T: Trigger pressed (stop) / 4Т: Завершение (нажата повторно)
    S_COLD_PULSE,    // Cold pulse active / Импульс холодной сварки
    S_COLD_PAUSE,    // Cold pause active / Пауза холодной сварки
    S_TACK,          // Single spot weld / Прихватка (точка)
    S_POST_GAS,      // Post-flow gas / Постпродувка
    S_WAIT_RELEASE   // Prevent auto-restart / Защита от автоповтора
} weld_state_t;

typedef enum { EDIT_OFF = 0, EDIT_PRE, EDIT_POST, EDIT_ARC, EDIT_PULSE, EDIT_MAX } edit_t;

typedef struct { 
    uint8_t mode; uint8_t pre; uint8_t post; uint8_t arc; uint8_t pulse; 
} settings_t;

volatile uint16_t state_timer = 0; // Countdown timer (ms) / Обратный отсчет (мс)
volatile uint8_t flag_20ms = 0;    // UI refresh flag / Флаг обновления интерфейса
settings_t cfg;                    // Current settings / Текущие настройки
edit_t edit_state = EDIT_OFF;      // Menu state / Состояние меню
weld_state_t state = S_IDLE;       // FSM state / Состояние автомата

/**
 * =============================================================================
 * 4. DISPLAY & EEPROM / ИНДИКАЦИЯ И ПАМЯТЬ
 * =============================================================================
 */

// Load from non-volatile memory / Загрузка из энергонезависимой памяти
void Settings_Load() {
    cfg.mode = eeprom_read(0); cfg.pre = eeprom_read(1); 
    cfg.post = eeprom_read(2); cfg.arc = eeprom_read(3);
    cfg.pulse = eeprom_read(4);
    
    // Bounds check / Проверка на корректность данных
    if(cfg.mode > 3) cfg.mode = 0;
    if(cfg.pre < 1 || cfg.pre > 8) cfg.pre = 2;
    if(cfg.post < 1 || cfg.post > 8) cfg.post = 5;
    if(cfg.arc < 1 || cfg.arc > 8) cfg.arc = 2;
    if(cfg.pulse < 1 || cfg.pulse > 8) cfg.pulse = 2;
}

// Save to EEPROM / Сохранение настроек
void Settings_Save() {
    eeprom_write(0, cfg.mode); eeprom_write(1, cfg.pre); 
    eeprom_write(2, cfg.post); eeprom_write(3, cfg.arc);
    eeprom_write(4, cfg.pulse);
}

// LED Scale output with RA6/RA7 swap / Вывод на шкалу с перестановкой пинов
void ShowValue(uint8_t v) {
    uint8_t mask = 0;
    if (v > 8) v = 8;
    for(uint8_t i = 0; i < v; i++) mask |= (1 << i); // Standard mask / Линейная маска
    
    // SWAP LOGIC: RA6 <-> RA7 / ЛОГИКА ПЕРЕСТАНОВКИ
    uint8_t output = mask & 0x3F;      // RA0..RA5 remain the same / Оставляем 0-5 биты
    if (mask & 0x40) output |= 0x80;   // Bit 6 goes to RA7 / 6-й бит (LED7) на RA7
    if (mask & 0x80) output |= 0x40;   // Bit 7 goes to RA6 / 7-й бит (LED8) на RA6
    
    PIN_LED_BAR = ~output; // Common Anode: 0 turns LED ON / Общий анод: 0 зажигает
}

/**
 * =============================================================================
 * 5. USER INTERFACE / ИНТЕРФЕЙС
 * =============================================================================
 */
void UI_Tick() {
    static uint8_t u_l = 0, d_l = 0, s_l = 0, c_t = 0;
    static uint16_t blink_cnt = 0;

    // Enter/Exit Menu: UP+DOWN hold / Вход в меню: удержание Вверх+Вниз
    if (!BTN_UP && !BTN_DOWN) {
        if (++c_t > 50) { // 1 sec delay / Задержка 1 сек
            c_t = 0;
            if (edit_state == EDIT_OFF) edit_state = EDIT_PRE;
            else { edit_state = EDIT_OFF; Settings_Save(); }
            PIN_LED_BAR = 0xFF; 
        }
    } else c_t = 0;

    if (edit_state != EDIT_OFF) {
        blink_cnt++;
        if ((blink_cnt / 10) % 2) { // Blink value / Мигание значения
            uint8_t val = (edit_state == EDIT_PRE) ? cfg.pre : (edit_state == EDIT_POST) ? cfg.post : (edit_state == EDIT_ARC) ? cfg.arc : cfg.pulse;
            ShowValue(val);
        } else PIN_LED_BAR = 0xFF;

        // Settings adjustments / Регулировка параметров
        if (!BTN_UP && !u_l) {
            if (edit_state == EDIT_PRE && cfg.pre < 8) cfg.pre++;
            if (edit_state == EDIT_POST && cfg.post < 8) cfg.post++;
            if (edit_state == EDIT_ARC && cfg.arc < 8) cfg.arc++;
            if (edit_state == EDIT_PULSE && cfg.pulse < 8) cfg.pulse++;
            u_l = 1;
        } else if (BTN_UP) u_l = 0;

        if (!BTN_DOWN && !d_l) {
            if (edit_state == EDIT_PRE && cfg.pre > 1) cfg.pre--;
            if (edit_state == EDIT_POST && cfg.post > 1) cfg.post--;
            if (edit_state == EDIT_ARC && cfg.arc > 1) cfg.arc--;
            if (edit_state == EDIT_PULSE && cfg.pulse > 1) cfg.pulse--;
            d_l = 1;
        } else if (BTN_DOWN) d_l = 0;

        // Switch setting via torch button / Смена параметра кнопкой горелки
        if (!BTN_START && !s_l) {
            if (++edit_state >= EDIT_MAX) edit_state = EDIT_PRE;
            s_l = 1;
        } else if (BTN_START) s_l = 0;
    } else {
        // Mode select / Выбор режима
        if (!BTN_UP && !u_l) { cfg.mode = (cfg.mode + 1) % 4; u_l = 1; }
        else if (BTN_UP) u_l = 0;
        
        // Manual Gas purge (Down) / Ручная продувка газа (Вниз)
        if (state == S_IDLE) PIN_GAS = !BTN_DOWN;
        
        // Show current mode (LED1-4) / Отображение режима (светодиоды 1-4)
        PIN_LED_BAR = ~(1 << (cfg.mode & 0x07));
    }
}

/**
 * =============================================================================
 * 6. WELDING LOGIC (FSM) / ЛОГИКА СВАРКИ
 * =============================================================================
 */


void FSM_Process() {
    switch(state) {
        case S_IDLE:
            PIN_WELD = 1; PIN_ARC = 0; PIN_GAS = 0;
            if (!BTN_START && edit_state == EDIT_OFF) {
                state_timer = (uint16_t)cfg.pre * 100;
                state = S_PRE_GAS;
            }
            break;

        case S_PRE_GAS: // PRE-GAS flow / Предпродувка
            PIN_GAS = 1;
            if (state_timer == 0) {
                PIN_WELD = 0; PIN_ARC = 1; // Relays ON / Реле ВКЛ
                if (cfg.mode == 2) { state_timer = 50; state = S_COLD_PULSE; } // Cold Weld
                else if (cfg.mode == 3) { state_timer = (uint16_t)cfg.pulse * 100; state = S_TACK; } // Tack
                else { state_timer = (uint16_t)cfg.arc * 100; state = S_ARC_INIT; } // TIG
            }
            break;

        case S_ARC_INIT: // Wait for stable arc / Ожидание стабильной дуги
            if (state_timer == 0) {
                PIN_ARC = 0; // HF Ignition OFF / Поджиг ВЫКЛ
                state = (cfg.mode == 0) ? S_WELD_2T : S_WELD_4T_1;
            }
            break;

        case S_WELD_2T: // Standard 2-Stroke / Режим 2Т
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            break;

        case S_WELD_4T_1: if (BTN_START) state = S_WELD_4T_2; break; // Release to start / Отпусти для сварки
        case S_WELD_4T_2: if (!BTN_START) state = S_WELD_4T_3; break; // Press to stop / Нажми для стопа
        case S_WELD_4T_3: if (!BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; } break;

        case S_COLD_PULSE: // COLD WELD: Impulse / Холодная сварка: Импульс
            PIN_WELD = 0; PIN_ARC = 1;
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            else if (state_timer == 0) {
                PIN_WELD = 1; PIN_ARC = 0;
                state_timer = (uint16_t)cfg.pulse * 100;
                state = S_COLD_PAUSE;
            }
            break;

        case S_COLD_PAUSE: // COLD WELD: Pause / Холодная сварка: Пауза
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            else if (state_timer == 0) {
                PIN_WELD = 0; PIN_ARC = 1; state_timer = 50; state = S_COLD_PULSE;
            }
            break;

        case S_TACK: // Single spot weld / Сварка прихватками (точкой)
            if (state_timer == 0) {
                PIN_WELD = 1; PIN_ARC = 0;
                state_timer = (uint16_t)cfg.post * 100;
                state = S_POST_GAS;
            }
            break;

        case S_POST_GAS: // Post-gas flow / Постпродувка
            PIN_GAS = 1; PIN_WELD = 1; PIN_ARC = 0;
            if (state_timer == 0) { PIN_GAS = 0; state = S_WAIT_RELEASE; }
            break;

        case S_WAIT_RELEASE: // Wait for button release / Ожидание отпускания кнопки
            if (BTN_START) state = S_IDLE;
            break;
    }
}

/**
 * =============================================================================
 * 7. SYSTEM: ISR & MAIN / СИСТЕМНЫЕ ФУНКЦИИ
 * =============================================================================
 */

// Interrupt Service Routine / Обработка прерываний
void __interrupt() isr() {
    if (T0IF) { // Timer0 Interrupt (1ms)
        static uint8_t div_20 = 0;
        TMR0 = 6; // Reload timer / Перезагрузка таймера
        if (state_timer > 0) state_timer--;
        if (++div_20 >= 20) { flag_20ms = 1; div_20 = 0; } // UI divider / Делитель для интерфейса
        T0IF = 0; // Clear flag / Сброс флага
    }
}

void main() {
    // OSCCON Register for 8MHz internal oscillator
    OSCCON = 0x71; 
    
    // Digital Mode for all pins / Все ножки в цифровой режим
    ANSEL = 0; ANSELH = 0; 
    
    // TRIS: I/O Direction / Направление портов
    TRISA = 0x00;        // PORTA - Outputs (LEDs) / Выходы (светодиоды)
    TRISB = 0x38;        // RB1, RB2 - Outputs; RB3,4,5 - Inputs / Входы для кнопок
    TRISCbits.TRISC0 = 0;// RC0 - Output (Gas) / Выход (Газ)
    
    // Internal Pull-ups for buttons / Внутренняя подтяжка для кнопок
    OPTION_REGbits.nRBPU = 0; 
    WPUB = 0x38; 

    // Timer0 setup for 1ms / Настройка Timer0 на 1мс
    OPTION_REGbits.T0CS = 0; 
    OPTION_REGbits.PSA = 0; 
    OPTION_REGbits.PS = 0x02; // Prescaler 1:8
    TMR0 = 6; T0IE = 1; GIE = 1;

    Settings_Load(); // Load settings on startup / Загрузка при старте

    while(1) {
        CLRWDT(); // Reset Watchdog (Safety) / Сброс собаки (Безопасность)
        FSM_Process(); // Welding logic / Сварочный автомат
        if (flag_20ms) { UI_Tick(); flag_20ms = 0; } // Interface / Интерфейс
    }
}