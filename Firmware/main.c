// =============================================================================
// 1. CONFIGURATION BITS (FUSES) - Установлены до инклудов
// =============================================================================
#pragma config FOSC = INTRC_NOCLKOUT // Внутренний генератор, RA6/RA7 свободны
#pragma config WDTE = ON             // Watchdog включен (защита от зависаний)
#pragma config PWRTE = OFF           // Power-up Timer выкл (для мгновенного зажима реле)
#pragma config MCLRE = ON            // Сброс через MCLR (резистор 10к на +5В)
#pragma config CP = OFF              // Защита кода выключена
#pragma config CPD = OFF             // Защита EEPROM выключена
#pragma config BOREN = ON            // Сброс при падении питания (Brown-out)
#pragma config IESO = OFF            // Внутреннее/внешнее переключение тактов выкл
#pragma config FCMEN = OFF           // Монитор тактов выкл
#pragma config LVP = OFF             // Low Voltage Programming выкл (критично для стабильности)
#pragma config BOR4V = BOR40V        // Порог сброса питания 4.0В
#pragma config WRT = OFF             // Защита записи выключена

// =============================================================================
// 2. БИБЛИОТЕКИ И ОПРЕДЕЛЕНИЯ
// =============================================================================
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 8000000      // Частота 8МГц для расчетов задержек

// Назначение пинов (Hardware Mapping)
#define PIN_GAS      PORTCbits.RC0   // Газовый клапан (1 - открыто)
#define PIN_WELD     PORTBbits.RB2   // Сварочное реле (0 - ВКЛ / Active Low)
#define PIN_ARC      PORTBbits.RB1   // Поджиг дуги (1 - ВКЛ)
#define PIN_LED_BAR  PORTA           // Шкала светодиодов (Общий анод)

// Кнопки управления
#define BTN_START    PORTBbits.RB3   // Кнопка на горелке (Старт/Стоп)
#define BTN_UP       PORTBbits.RB4   // Кнопка Вверх (Выбор режима/Настройка)
#define BTN_DOWN     PORTBbits.RB5   // Кнопка Вниз (Продувка/Настройка)

// =============================================================================
// 3. ТИПЫ ДАННЫХ И ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// =============================================================================
typedef enum { 
    S_IDLE, S_PRE_GAS, S_ARC_INIT, S_WELD_2T, 
    S_WELD_4T_1, S_WELD_4T_2, S_WELD_4T_3, 
    S_COLD_PULSE, S_COLD_PAUSE, S_TACK, S_POST_GAS, S_WAIT_RELEASE 
} weld_state_t;

typedef enum { EDIT_OFF = 0, EDIT_PRE, EDIT_POST, EDIT_ARC, EDIT_PULSE, EDIT_MAX } edit_t;

typedef struct { 
    uint8_t mode; uint8_t pre; uint8_t post; uint8_t arc; uint8_t pulse; 
} settings_t;

volatile uint16_t state_timer = 0; // Таймер состояний (мс)
volatile uint8_t flag_20ms = 0;    // Флаг обновления интерфейса
settings_t cfg;                    // Структура настроек
edit_t edit_state = EDIT_OFF;      // Состояние меню
weld_state_t state = S_IDLE;       // Текущее состояние автомата

// =============================================================================
// 4. СЕРВИСНЫЕ ФУНКЦИИ (EEPROM И ДИСПЛЕЙ)
// =============================================================================

void Settings_Load() {
    cfg.mode = eeprom_read(0); cfg.pre = eeprom_read(1); 
    cfg.post = eeprom_read(2); cfg.arc = eeprom_read(3); cfg.pulse = eeprom_read(4);
    if(cfg.mode > 3) cfg.mode = 0;
    if(cfg.pre < 1 || cfg.pre > 8) cfg.pre = 2;
    if(cfg.post < 1 || cfg.post > 8) cfg.post = 5;
    if(cfg.arc < 1 || cfg.arc > 8) cfg.arc = 2;
    if(cfg.pulse < 1 || cfg.pulse > 8) cfg.pulse = 2;
}

void Settings_Save() {
    eeprom_write(0, cfg.mode); eeprom_write(1, cfg.pre); 
    eeprom_write(2, cfg.post); eeprom_write(3, cfg.arc); 
    eeprom_write(4, cfg.pulse);
}

void ShowMask(uint8_t mask) {
    // Исправляем порядок бит для правильной работы RA6/RA7
    uint8_t output = mask & 0x3F;           
    if (mask & 0x40) output |= 0x80;        
    if (mask & 0x80) output |= 0x40;        
    PIN_LED_BAR = (uint8_t)~output; // Инвертируем, так как общий анод
}

void ShowValue(uint8_t v) {
    uint8_t mask = 0;
    if (v > 8) v = 8;
    for(uint8_t i = 0; i < v; i++) mask |= (uint8_t)(1 << i);
    ShowMask(mask);
}

// =============================================================================
// 5. ПОЛЬЗОВАТЕЛЬСКИЙ ИНТЕРФЕЙС (КНОПКИ И МЕНЮ)
// =============================================================================
void UI_Tick() {
    static uint8_t u_l = 0, d_l = 0, s_l = 0, c_t = 0; 
    static uint16_t blink_cnt = 0;
    static uint8_t anim_step = 0, anim_tick = 0; 

    // Вход/выход из настроек: зажать Вверх + Вниз
    if (!BTN_UP && !BTN_DOWN) {
        if (++c_t > 50) {  
            c_t = 0;
            if (edit_state == EDIT_OFF) edit_state = EDIT_PRE; 
            else { edit_state = EDIT_OFF; Settings_Save(); }
            PIN_LED_BAR = 0xFF; 
        }
    } else c_t = 0;

    if (edit_state != EDIT_OFF) {
        // Режим РЕДАКТИРОВАНИЯ
        blink_cnt++;
        if ((blink_cnt / 10) % 2) { 
            uint8_t val = (edit_state == EDIT_PRE) ? cfg.pre : 
                          (edit_state == EDIT_POST) ? cfg.post : 
                          (edit_state == EDIT_ARC) ? cfg.arc : cfg.pulse;
            ShowValue(val);
        } else PIN_LED_BAR = 0xFF;

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

        if (!BTN_START && !s_l) {
            if (++edit_state >= EDIT_MAX) edit_state = EDIT_PRE;
            s_l = 1;
        } else if (BTN_START) s_l = 0;
    } 
    else {
        // РАБОЧИЙ РЕЖИМ
        if (state == S_IDLE || state == S_WAIT_RELEASE) {
            anim_step = 0; anim_tick = 0;
            if (state == S_IDLE) {
                if (!BTN_UP && !u_l) { cfg.mode = (cfg.mode + 1) % 4; u_l = 1; }
                else if (BTN_UP) u_l = 0;
            }
            ShowMask((uint8_t)(1 << (cfg.mode & 0x07)));
        } else {
            // Анимация при активной сварке
            if (++anim_tick >= 4) { anim_tick = 0; anim_step = (anim_step + 1) % 4; }
            uint8_t anim_mask = 0;
            switch(anim_step) {
                case 0: anim_mask = 0x81; break; 
                case 1: anim_mask = 0x42; break; 
                case 2: anim_mask = 0x24; break; 
                case 3: anim_mask = 0x18; break; 
            }
            ShowMask(anim_mask);
        }
    }
}

// =============================================================================
// 6. АВТОМАТ УПРАВЛЕНИЯ СВАРКОЙ (FSM)
// =============================================================================
void FSM_Process() {
    switch(state) {
        case S_IDLE:
            PIN_WELD = 1; PIN_ARC = 0; 
            
            // ПРИОРИТЕТ 1: Старт сварки
            if (!BTN_START && edit_state == EDIT_OFF) {
                PIN_GAS = 1; 
                state_timer = (uint16_t)cfg.pre * 100; 
                state = S_PRE_GAS;
            } 
            // ПРИОРИТЕТ 2: Ручная продувка газа (кнопка вниз)
            else if (!BTN_DOWN && edit_state == EDIT_OFF) {
                PIN_GAS = 1;
            }
            // ПРИОРИТЕТ 3: Всё выключено
            else {
                PIN_GAS = 0;
            }
            break;

        case S_PRE_GAS: 
            PIN_GAS = 1;
            if (state_timer == 0) {
                PIN_WELD = 0; PIN_ARC = 1;  
                if (cfg.mode == 2) { state_timer = 50; state = S_COLD_PULSE; }
                else if (cfg.mode == 3) { state_timer = (uint16_t)cfg.pulse * 100; state = S_TACK; }
                else { state_timer = (uint16_t)cfg.arc * 100; state = S_ARC_INIT; }
            }
            break;

        case S_ARC_INIT: 
            if (state_timer == 0) { PIN_ARC = 0; state = (cfg.mode == 0) ? S_WELD_2T : S_WELD_4T_1; }
            break;

        case S_WELD_2T: 
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            break;

        case S_WELD_4T_1: if (BTN_START) state = S_WELD_4T_2; break;
        case S_WELD_4T_2: if (!BTN_START) state = S_WELD_4T_3; break;
        case S_WELD_4T_3: if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; } break;

        case S_COLD_PULSE: 
            PIN_WELD = 0; PIN_ARC = 1; 
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            else if (state_timer == 0) { 
                PIN_WELD = 1; PIN_ARC = 0; 
                state_timer = (uint16_t)cfg.pulse * 100; state = S_COLD_PAUSE; 
            }
            break;

        case S_COLD_PAUSE: 
            if (BTN_START) { state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; }
            else if (state_timer == 0) { 
                PIN_WELD = 0; PIN_ARC = 1; state_timer = 50; state = S_COLD_PULSE; 
            }
            break;

        case S_TACK: 
            if (state_timer == 0) { 
                PIN_WELD = 1; PIN_ARC = 0; 
                state_timer = (uint16_t)cfg.post * 100; state = S_POST_GAS; 
            }
            break;

        case S_POST_GAS: 
            PIN_GAS = 1; PIN_WELD = 1; PIN_ARC = 0;
            if (state_timer == 0) { PIN_GAS = 0; state = S_WAIT_RELEASE; }
            break;

        case S_WAIT_RELEASE: 
            if (BTN_START) state = S_IDLE; 
            break;
    }
}

// =============================================================================
// 7. СИСТЕМНЫЕ ФУНКЦИИ (ПРЕРЫВАНИЯ И MAIN)
// =============================================================================
void __interrupt() isr() {
    if (T0IF) { 
        static uint8_t div_20 = 0; TMR0 = 6; 
        if (state_timer > 0) state_timer--;
        if (++div_20 >= 20) { flag_20ms = 1; div_20 = 0; } 
        T0IF = 0; 
    }
}

void main() {
    // 1. Аппаратная инициализация (безопасное состояние)
    PORTC = 0x00; 
    PORTB = 0x04; // RB2=1 (выкл реле)
    PORTA = 0xFF; // Индикация выкл
    
    TRISC = 0xFE; // RC0 выход
    TRISB = 0x38; // RB0-2 выходы, RB3-5 входы
    TRISA = 0x00; // Все на выход
    
    ANSEL = 0; ANSELH = 0;
    OSCCON = 0x71; // 8MHz
    
    // 2. Подтяжка кнопок
    OPTION_REGbits.nRBPU = 0; 
    WPUB = 0x38;

    // 3. Задержка стабилизации (против ложных нажатий при старте)
    __delay_ms(50); 

    // 4. Таймер
    OPTION_REGbits.T0CS = 0; 
    OPTION_REGbits.PSA = 0; 
    OPTION_REGbits.PS = 0x02; 
    TMR0 = 6; T0IE = 1; GIE = 1;

    Settings_Load();

    while(1) {
        CLRWDT();              
        FSM_Process();         
        if (flag_20ms) { UI_Tick(); flag_20ms = 0; }
    }
}
