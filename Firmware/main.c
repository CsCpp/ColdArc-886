// =============================================================================
// 1. CONFIGURATION BITS (FUSES) - МАКСИМАЛЬНАЯ ЗАЩИТА ОТ ПОМЕХ
// =============================================================================
#pragma config FOSC = INTRC_NOCLKOUT 
#pragma config WDTE = ON             // Watchdog включен (защита от зависаний)
#pragma config PWRTE = ON            // Таймер старта включен
#pragma config MCLRE = OFF           // ВАЖНО: Внешний сброс ОТКЛЮЧЕН
#pragma config CP = OFF, CPD = OFF   
#pragma config BOREN = OFF           // ВАЖНО: Детектор просадки напряжения ОТКЛЮЧЕН
#pragma config IESO = OFF, FCMEN = OFF
#pragma config LVP = OFF             
#pragma config BOR4V = BOR40V        
#pragma config WRT = OFF

// =============================================================================
// 2. ОПРЕДЕЛЕНИЯ И ПИНЫ
// =============================================================================
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 8000000

#define PIN_GAS      PORTCbits.RC0   // Газ (1 - ВКЛ)
#define PIN_WELD     PORTBbits.RB2   // Сила (0 - ВКЛ, активный низкий)
#define PIN_ARC      PORTBbits.RB1   // Осциллятор (1 - ВКЛ)
#define PIN_LED_BAR  PORTA           // Индикация настроек

#define BTN_START    PORTBbits.RB3   // Кнопка горелки (0 - нажата)
#define BTN_UP       PORTBbits.RB4   // Кнопка ВВЕРХ
#define BTN_DOWN     PORTBbits.RB5   // Кнопка ВНИЗ

// =============================================================================
// 3. ПЕРЕМЕННЫЕ И СОСТОЯНИЯ
// =============================================================================
// S_WELD_4T_3 удален за ненадобностью
typedef enum { 
    S_IDLE, S_PRE_GAS, S_ARC_INIT, S_WELD_2T, 
    S_WELD_4T_1, S_WELD_4T_2, 
    S_COLD_PULSE, S_COLD_PAUSE, S_TACK, S_POST_GAS, S_WAIT_RELEASE 
} weld_state_t;

typedef enum { EDIT_OFF = 0, EDIT_PRE, EDIT_POST, EDIT_ARC, EDIT_PULSE, EDIT_MAX } edit_t;

typedef struct { 
    uint8_t mode; uint8_t pre; uint8_t post; uint8_t arc; uint8_t pulse; 
} settings_t;

volatile uint16_t state_timer = 0; 
volatile uint8_t flag_20ms = 0;    

volatile uint8_t start_debounced = 1; // 1 = отпущена, 0 = нажата
volatile uint8_t up_debounced = 1;
volatile uint8_t down_debounced = 1;

settings_t cfg;                    
edit_t edit_state = EDIT_OFF;      
weld_state_t state = S_IDLE;       

// =============================================================================
// 4. РАБОТА С ПАМЯТЬЮ И ИНДИКАЦИЕЙ
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
    while(WR); 
    eeprom_write(0, cfg.mode); eeprom_write(1, cfg.pre); 
    eeprom_write(2, cfg.post); eeprom_write(3, cfg.arc); 
    eeprom_write(4, cfg.pulse);
}

void ShowMask(uint8_t mask) {
    uint8_t output = mask & 0x3F;           
    if (mask & 0x40) output |= 0x80;        
    if (mask & 0x80) output |= 0x40;        
    PIN_LED_BAR = (uint8_t)~output; 
}

void ShowValue(uint8_t v) {
    uint8_t mask = 0;
    if (v > 8) v = 8;
    for(uint8_t i = 0; i < v; i++) mask |= (uint8_t)(1 << i);
    ShowMask(mask);
}

// =============================================================================
// 5. ИНТЕРФЕЙС И ЦИФРОВОЙ ФИЛЬТР (UI)
// =============================================================================
void UI_Tick() {
    static uint8_t u_l = 0, d_l = 0, s_l = 0, c_t = 0; 
    static uint16_t blink_cnt = 0;
    static uint8_t anim_step = 0, anim_tick = 0;
    
    // Сдвиговый фильтр кнопок (отсекает помехи короче 160 мс)
    static uint8_t f_start = 0xFF, f_up = 0xFF, f_down = 0xFF;

    f_start = (uint8_t)((f_start << 1) | BTN_START);
    f_up    = (uint8_t)((f_up << 1) | BTN_UP);
    f_down  = (uint8_t)((f_down << 1) | BTN_DOWN);

    if (f_start == 0x00) start_debounced = 0; else if (f_start == 0xFF) start_debounced = 1;
    if (f_up == 0x00)    up_debounced = 0;    else if (f_up == 0xFF)    up_debounced = 1;
    if (f_down == 0x00)  down_debounced = 0;  else if (f_down == 0xFF)  down_debounced = 1;

    // Вход в меню настроек (удержание UP + DOWN)
    if (!up_debounced && !down_debounced) {
        if (++c_t > 50) {  
            c_t = 0;
            if (edit_state == EDIT_OFF && state == S_IDLE) edit_state = EDIT_PRE; 
            else { edit_state = EDIT_OFF; Settings_Save(); }
            PIN_LED_BAR = 0xFF; 
        }
    } else c_t = 0;

    if (edit_state != EDIT_OFF) {
        blink_cnt++;
        if ((blink_cnt / 10) % 2) { 
            uint8_t val = (edit_state == EDIT_PRE) ? cfg.pre : 
                          (edit_state == EDIT_POST) ? cfg.post : 
                          (edit_state == EDIT_ARC) ? cfg.arc : cfg.pulse;
            ShowValue(val);
        } else PIN_LED_BAR = 0xFF;

        if (!up_debounced && !u_l) {
            if (edit_state == EDIT_PRE && cfg.pre < 8) cfg.pre++;
            if (edit_state == EDIT_POST && cfg.post < 8) cfg.post++;
            if (edit_state == EDIT_ARC && cfg.arc < 8) cfg.arc++;
            if (edit_state == EDIT_PULSE && cfg.pulse < 8) cfg.pulse++;
            u_l = 1;
        } else if (up_debounced) u_l = 0;

        if (!down_debounced && !d_l) {
            if (edit_state == EDIT_PRE && cfg.pre > 1) cfg.pre--;
            if (edit_state == EDIT_POST && cfg.post > 1) cfg.post--;
            if (edit_state == EDIT_ARC && cfg.arc > 1) cfg.arc--;
            if (edit_state == EDIT_PULSE && cfg.pulse > 1) cfg.pulse--;
            d_l = 1;
        } else if (down_debounced) d_l = 0;

        if (!start_debounced && !s_l) {
            if (++edit_state >= EDIT_MAX) edit_state = EDIT_PRE;
            s_l = 1;
        } else if (start_debounced) s_l = 0;
    } 
    else {
        if (state == S_IDLE) {
            if (!up_debounced && !u_l) { cfg.mode = (cfg.mode + 1) % 4; u_l = 1; } 
            else if (up_debounced) u_l = 0;
            
            ShowMask((uint8_t)(1 << (cfg.mode & 0x07)));
            
            // Ручная продувка газа
            if (!down_debounced) PIN_GAS = 1; else PIN_GAS = 0;
        } 
        else if (state == S_WAIT_RELEASE) {
            ShowMask((uint8_t)(1 << (cfg.mode & 0x07)));
        }
        else {
            // Анимация во время сварки
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
// 6. АВТОМАТ УПРАВЛЕНИЯ (FSM)
// =============================================================================
void FSM_Process() {
    switch(state) {
        case S_IDLE:
            PIN_WELD = 1; // Сила ВЫКЛ (активный 0)
            PIN_ARC = 0;  // Осциллятор ВЫКЛ
            // Если нажали кнопку горелки
            if (start_debounced == 0 && edit_state == EDIT_OFF) {
                PIN_GAS = 1; 
                state_timer = (uint16_t)cfg.pre * 250; 
                state = S_PRE_GAS;
            } 
            break;

        case S_PRE_GAS: 
            if (state_timer == 0) {
                PIN_WELD = 0; // Включаем сварочный ток
                PIN_ARC = 1;  // Включаем осциллятор
                
                if (cfg.mode == 2) { state_timer = (uint16_t)cfg.arc * 10; state = S_COLD_PULSE; }
                else if (cfg.mode == 3) { state_timer = (uint16_t)cfg.pulse * 100; state = S_TACK; }
                else { state_timer = (uint16_t)cfg.arc * 100; state = S_ARC_INIT; }
            }
            break;

        case S_ARC_INIT: 
            if (state_timer == 0) { 
                PIN_ARC = 0; // Время поджига вышло, тушим осциллятор
                state = (cfg.mode == 0) ? S_WELD_2T : S_WELD_4T_1; 
            } 
            break;

        // --- РЕЖИМ 2Т ---
        case S_WELD_2T: 
            // Отпустили кнопку - заканчиваем сварку
            if (start_debounced == 1) { 
                PIN_WELD = 1; PIN_ARC = 0; 
                state_timer = (uint16_t)cfg.post * 375; 
                state = S_POST_GAS; 
            } 
            break;

        // --- РЕЖИМ 4Т (ИСПРАВЛЕННЫЙ) ---
        case S_WELD_4T_1: 
            // Ждем первого ОТПУСКАНИЯ кнопки (дуга горит)
            if (start_debounced == 1) { 
                state = S_WELD_4T_2; 
            } 
            break;

        case S_WELD_4T_2: 
            // Ждем второго НАЖАТИЯ для завершения сварки
            if (start_debounced == 0) { 
                PIN_WELD = 1; PIN_ARC = 0; // Сразу тушим дугу
                state_timer = (uint16_t)cfg.post * 375; 
                state = S_POST_GAS; 
            } 
            break;

        // --- СПЕЦ. РЕЖИМЫ (COLD, TACK) ---
        case S_COLD_PULSE: 
            if (start_debounced == 1) { 
                PIN_WELD = 1; PIN_ARC = 0;
                state_timer = (uint16_t)cfg.post * 375; state = S_POST_GAS; 
            }
            else if (state_timer == 0) { 
                PIN_WELD = 1; PIN_ARC = 0; 
                state_timer = (uint16_t)cfg.pulse * 50; state = S_COLD_PAUSE; 
            }
            break;

        case S_COLD_PAUSE: 
            if (start_debounced == 1) { 
                PIN_WELD = 1; PIN_ARC = 0;
                state_timer = (uint16_t)cfg.post * 375; state = S_POST_GAS; 
            }
            else if (state_timer == 0) { 
                PIN_WELD = 0; PIN_ARC = 1; 
                state_timer = (uint16_t)cfg.arc * 10; state = S_COLD_PULSE; 
            }
            break;

        case S_TACK: 
            if (state_timer == 0) { 
                PIN_WELD = 1; PIN_ARC = 0; 
                state_timer = (uint16_t)cfg.post * 375; state = S_POST_GAS; 
            } 
            break;

        // --- ЗАВЕРШЕНИЕ ЦИКЛА ---
        case S_POST_GAS: 
            PIN_GAS = 1; PIN_WELD = 1; PIN_ARC = 0;
            if (state_timer == 0) { 
                PIN_GAS = 0; 
                state = S_WAIT_RELEASE; 
            }
            break;

        case S_WAIT_RELEASE: 
            // Защита от случайного рестарта: ждем, пока кнопку реально отпустят
            if (start_debounced == 1) state = S_IDLE; 
            break;
    }
}

// =============================================================================
// 7. MAIN & ISR
// =============================================================================
void __interrupt() isr() {
    // Прерывание Timer0 каждые 1мс
    if (T0IF) { 
        static uint8_t div_20 = 0; 
        TMR0 = 6; // Перезагрузка таймера
        if (state_timer > 0) state_timer--;
        if (++div_20 >= 20) { flag_20ms = 1; div_20 = 0; } 
        T0IF = 0; 
    }
}

void main() {
    // Инициализация портов
    PORTA = 0xFF; PORTB = 0x04; PORTC = 0x00; 
    TRISA = 0x00; TRISB = 0x38; TRISC = 0xFE; 
    
    ANSEL = 0; ANSELH = 0; OSCCON = 0x71; // 8 МГц внутренний генератор
    
    // Внутренние подтяжки PORTB (для кнопок)
    OPTION_REGbits.nRBPU = 0; WPUB = 0x38;

    __delay_ms(100); 
    // Защита при включении питания (ждем пока отпустят кнопку горелки)
    while(BTN_START == 0) { CLRWDT(); } 
    __delay_ms(50); 

    // Настройка Timer0
    OPTION_REGbits.T0CS = 0; OPTION_REGbits.PSA = 0; OPTION_REGbits.PS = 0x02; 
    TMR0 = 6; T0IE = 1; GIE = 1;

    Settings_Load();

    // Главный цикл
    while(1) {
        CLRWDT();               
        FSM_Process();          
        if (flag_20ms) { 
            UI_Tick(); 
            flag_20ms = 0; 
        }
    }
}
