#include <xc.h>
#include <stdint.h>

// --- CONFIGURATION / КОНФИГУРАЦИЯ ---
#pragma config FOSC = INTRC_NOCLKOUT, WDTE = ON, PWRTE = ON, MCLRE = ON
#pragma config CP = OFF, CPD = OFF, BOREN = ON, IESO = OFF, FCMEN = OFF, LVP = OFF

#define _XTAL_FREQ 8000000 

// --- PIN DEFINITIONS / ПИНЫ ---
#define GAS_RELAY    PORTBbits.RB1 
#define WELD_ALLOW   PORTBbits.RB2 // 0 = Current ON / 0 = Ток ВКЛ
#define ARC_IGNITE   PORTCbits.RC0 // 1 = HF ON / 1 = Поджиг ВКЛ
#define LED_BAR      PORTA      

#define BTN_START    PORTBbits.RB3 
#define BTN_UP       PORTBbits.RB4 
#define BTN_DOWN     PORTBbits.RB5 

// --- SAFETY PARAMETERS / ПАРАМЕТРЫ БЕЗОПАСНОСТИ ---
#define WELD_MAX_TIME_SEC 120 // Max arc duration to prevent overheating / Макс. время дуги

// --- STATE MACHINE STATES / СОСТОЯНИЯ АВТОМАТА ---
typedef enum {
    SM_IDLE,            // Waiting for start / Ожидание нажатия
    SM_PRE_GAS,         // Gas pre-flow / Пред-газ
    SM_ARC_INIT,        // High frequency ignition / Поджиг дуги (ВЧ)
    SM_WELD_2T,         // Standard 2T welding / Сварка 2Т
    SM_4T_WAIT_RELEASE, // 4T: Wait for release after start / 4Т: Ожидание после поджига
    SM_4T_WELDING,      // 4T: Main welding process / 4Т: Основной процесс
    SM_4T_WAIT_STOP,    // 4T: Wait for final release / 4Т: Финальное отпускание
    SM_COLD_PULSE,      // Cold Weld: Current pulse / Cold Weld: Импульс
    SM_COLD_PAUSE,      // Cold Weld: Pause / Cold Weld: Пауза
    SM_TACK_PROCESS,    // Tack welding mode / Режим прихватки
    SM_POST_GAS,        // Gas post-flow / Пост-газ
    SM_EMERGENCY_STOP   // Safety timeout triggered / Аварийная остановка
} weld_state_t;

// --- GLOBAL VARIABLES / ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
volatile uint16_t state_timer = 0;    // Countdown for states (ms) / Счетчик для состояний (мс)
volatile uint16_t ms_counter = 0;     // System tick / Системный счетчик
volatile uint16_t safety_timer_ms = 0;
volatile uint16_t safety_timer_sec = 0;

weld_state_t current_state = SM_IDLE;
uint8_t mode = 0;      
uint8_t gas_time = 5;      
uint8_t arc_init_val = 10; 
uint8_t pulse_time = 10;   
uint8_t tack_time = 5;     

// --- PROTOTYPES / ПРОТОТИПЫ ---
void system_init(void);
void process_welding_fsm(void);
void handle_ui(void);
void load_settings(void);
void save_settings(void);

// --- INTERRUPT SERVICE ROUTINE / ПРЕРЫВАНИЕ ---
void __interrupt() isr(void) {
    if (T0IF) {
        TMR0 = 6; // 1ms tick at 8MHz / Тик 1мс при 8МГц
        if (state_timer > 0) state_timer--;
        ms_counter++;
        
        // Safety timer: increments only when welding / Таймер безопасности (считает при сварке)
        if (current_state == SM_WELD_2T || current_state == SM_4T_WELDING || current_state == SM_COLD_PULSE) {
            if (++safety_timer_ms >= 1000) {
                safety_timer_ms = 0;
                safety_timer_sec++;
            }
        } else {
            safety_timer_ms = 0;
            safety_timer_sec = 0;
        }
        T0IF = 0;
    }
}

void main(void) {
    system_init();
    load_settings();

    // Stuck button protection on startup / Защита от залипания кнопок при включении
    while (!BTN_START || !BTN_UP || !BTN_DOWN) {
        CLRWDT();
        if ((ms_counter / 100) % 2) LED_BAR = 0b10000001; // Blink edges / Мигать краями
        else LED_BAR = 0x00;
    }

    while(1) {
        CLRWDT(); 
        process_welding_fsm(); // Handle welding logic / Логика сварки
        handle_ui();           // Handle buttons and LEDs / Кнопки и индикация
    }
}

void system_init(void) {
    OSCCON = 0x71; // 8MHz Internal clock / 8МГц внутр. осциллятор
    ANSEL = 0; ANSELH = 0; // Digital I/O / Все пины цифровые
    TRISA = 0x00; LED_BAR = 0xFF; 
    TRISB = 0x38; // RB3,4,5 inputs / Входы
    TRISCbits.TRISC0 = 0; // RC0 output (HF) / Выход поджига
    
    OPTION_REG = 0x02; // Prescaler TMR0 1:8 / Делитель 1:8
    TMR0 = 6;
    T0IE = 1; GIE = 1; // Enable interrupts / Включить прерывания

    GAS_RELAY = 0; WELD_ALLOW = 1; ARC_IGNITE = 0; // Default states / Исх. состояния
}

// --- WELDING STATE MACHINE / КОНЕЧНЫЙ АВТОМАТ ---

void process_welding_fsm(void) {
    // Check safety timeout / Проверка аварийного таймаута
    if (safety_timer_sec >= WELD_MAX_TIME_SEC && current_state != SM_EMERGENCY_STOP) {
        current_state = SM_EMERGENCY_STOP;
    }

    switch(current_state) {
        case SM_IDLE:
            WELD_ALLOW = 1; ARC_IGNITE = 0; GAS_RELAY = 0;
            if (!BTN_START) {
                state_timer = (uint16_t)gas_time * 100;
                GAS_RELAY = 1;
                current_state = SM_PRE_GAS;
            }
            break;

        case SM_PRE_GAS: // Gas flow before arc / Продувка до сварки
            if (state_timer == 0) {
                WELD_ALLOW = 0; // Relay ON (active low) / Ток ВКЛ
                ARC_IGNITE = 1; // HF ON / Поджиг ВКЛ
                state_timer = (uint16_t)arc_init_val * 100;
                current_state = SM_ARC_INIT;
            }
            break;

        case SM_ARC_INIT: // High frequency burst / Работа осциллятора
            if (state_timer == 0) {
                ARC_IGNITE = 0;
                if (mode == 0) current_state = SM_WELD_2T;
                else if (mode == 1) current_state = SM_4T_WAIT_RELEASE;
                else if (mode == 2) current_state = SM_COLD_PULSE;
                else {
                    state_timer = (uint16_t)tack_time * 10;
                    current_state = SM_TACK_PROCESS;
                }
            }
            break;

        case SM_WELD_2T: // Standard 2T: hold button to weld / 2Т: держим кнопку
            if (BTN_START) { 
                WELD_ALLOW = 1;
                state_timer = (uint16_t)gas_time * 100;
                current_state = SM_POST_GAS;
            }
            break;

        case SM_4T_WAIT_RELEASE: // 4T step 2 / 4Т шаг 2
            if (BTN_START) current_state = SM_4T_WELDING;
            break;

        case SM_4T_WELDING: // 4T step 3 (main arc) / 4Т шаг 3 (основная дуга)
            if (!BTN_START) current_state = SM_4T_WAIT_STOP;
            break;

        case SM_4T_WAIT_STOP: // 4T step 4 (release to end) / 4Т шаг 4 (финал)
            if (BTN_START) {
                WELD_ALLOW = 1;
                state_timer = (uint16_t)gas_time * 100;
                current_state = SM_POST_GAS;
            }
            break;

        case SM_COLD_PULSE: // Short powerful pulse / Короткий импульс
            ARC_IGNITE = 1; // HF with every pulse / Поджиг на каждый импульс
            state_timer = 40; // 40ms pulse / 40мс импульс
            current_state = SM_COLD_PAUSE;
            break;

        case SM_COLD_PAUSE: // Pause between pulses / Пауза
            if (state_timer == 0) {
                ARC_IGNITE = 0;
                if (BTN_START) { // Button released / Кнопку отпустили
                    WELD_ALLOW = 1;
                    state_timer = (uint16_t)gas_time * 100;
                    current_state = SM_POST_GAS;
                } else {
                    state_timer = (uint16_t)pulse_time * 10;
                    current_state = SM_ARC_INIT; // Next cycle / На новый круг
                }
            }
            break;

        case SM_TACK_PROCESS: // Single spot weld / Одиночная прихватка
            if (state_timer == 0) {
                WELD_ALLOW = 1;
                if (BTN_START) {
                    state_timer = (uint16_t)gas_time * 100;
                    current_state = SM_POST_GAS;
                }
            }
            break;

        case SM_EMERGENCY_STOP: // Timeout alert / Авария по времени
            WELD_ALLOW = 1; ARC_IGNITE = 0;
            if ((ms_counter / 200) % 2) LED_BAR = 0x00; // Blink bar / Мигать шкалой
            else LED_BAR = 0xFF;
            
            if (BTN_START) { // Reset on release / Сброс при отпускании
                state_timer = (uint16_t)gas_time * 100;
                current_state = SM_POST_GAS;
            }
            break;

        case SM_POST_GAS: // Gas flow after weld / Охлаждение после сварки
            if (state_timer == 0) {
                GAS_RELAY = 0;
                current_state = SM_IDLE;
            }
            break;
    }
}

// --- USER INTERFACE / ИНТЕРФЕЙС ---
void handle_ui(void) {
    static uint16_t ui_timer = 0;
    if (current_state != SM_IDLE) return;

    LED_BAR = ~(1 << (mode & 0x07)); // Show mode / Показать режим

    if (!BTN_UP && (ms_counter - ui_timer > 250)) { // Mode switch / Смена режима
        mode = (mode + 1) % 4;
        save_settings();
        ui_timer = ms_counter;
    }

    if (!BTN_DOWN) GAS_RELAY = 1; // Manual gas test / Тест газа
    else if (current_state == SM_IDLE) GAS_RELAY = 0;
}

// --- NON-VOLATILE MEMORY / ПАМЯТЬ ---
void save_settings(void) {
    if(eeprom_read(0x00) != mode) eeprom_write(0x00, mode);
    // Add other variables as needed / Добавьте другие переменные...
}

void load_settings(void) {
    mode = eeprom_read(0x00);
    if(mode > 3) mode = 0; // Bound check / Проверка границ
    // Load others...
}
