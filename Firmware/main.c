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

// --- GLOBAL VARIABLES / ПЕРЕМЕННЫЕ ---
uint8_t mode = 0;          
uint8_t gas_time = 5;      
uint8_t arc_init_val = 10; 
uint8_t pulse_time = 10;   
uint8_t tack_time = 5;     

// --- PROTOTYPES / ПРОТОТИПЫ ---
void system_init(void);
void load_settings(void);
void save_settings(void);
void weld_process(void);
void settings_menu(void);
void delay_ms_wdt(uint16_t ms);

void main(void) {
    system_init();
    load_settings();

    while(1) {
        CLRWDT(); 
        LED_BAR = ~(1 << (mode & 0x07)); // Безопасное отображение режима
        
        if (!BTN_START) { 
            __delay_ms(30); 
            if(!BTN_START) weld_process();
        }
        
        if (!BTN_UP) { 
            __delay_ms(50);
            uint16_t hold_cnt = 0;
            while(!BTN_UP && hold_cnt < 500) { __delay_ms(2); hold_cnt++; CLRWDT(); }
            
            if(hold_cnt >= 500) { 
                settings_menu(); 
            } else { 
                mode = (mode + 1) % 4; 
            }
            while(!BTN_UP) CLRWDT(); 
        }
        
        if (!BTN_DOWN) { 
            GAS_RELAY = 1;
            while(!BTN_DOWN) CLRWDT();
            GAS_RELAY = 0;
        }
    }
}

void system_init(void) {
    OSCCON = 0x71; // Важно: 8MHz в Proteus ставим вручную в свойствах чипа
    ANSEL = 0; ANSELH = 0;
    TRISA = 0x00; LED_BAR = 0xFF; 
    TRISB = 0x38; // RB3,4,5 входы
    TRISCbits.TRISC0 = 0; 
    OPTION_REGbits.nRBPU = 0; 
    WPUB = 0x38;
    
    GAS_RELAY = 0; WELD_ALLOW = 1; ARC_IGNITE = 0;
}



void weld_process(void) {
    // 1. ПРЕД-ГАЗ
    GAS_RELAY = 1;
    delay_ms_wdt((uint16_t)gas_time * 100);

    switch(mode) {
        case 0: // 2T MODE
            WELD_ALLOW = 0; 
            ARC_IGNITE = 1;
            uint16_t timeout2t = (uint16_t)arc_init_val * 100;
            while(!BTN_START) { 
                CLRWDT();
                if (timeout2t > 0) { 
                    __delay_ms(1); 
                    timeout2t--; 
                    if (timeout2t == 0) ARC_IGNITE = 0; // Выключаем строго по счетчику
                }
            }
            // Гарантированный сброс при отпускании кнопки
            ARC_IGNITE = 0; 
            WELD_ALLOW = 1; 
            break;

        case 1: // 4T MODE
            WELD_ALLOW = 0; 
            ARC_IGNITE = 1;
            delay_ms_wdt((uint16_t)arc_init_val * 100);
            ARC_IGNITE = 0; 
            while(!BTN_START) CLRWDT(); // Ждем отпускания
            delay_ms_wdt(100);
            while(BTN_START) CLRWDT();  // Ждем повторного нажатия
            WELD_ALLOW = 1; 
            while(!BTN_START) CLRWDT(); // Ждем финального отпускания
            break;

        case 2: // COLD WELD
            while(!BTN_START) {
                WELD_ALLOW = 0; 
                ARC_IGNITE = 1;
                __delay_ms(40); 
                ARC_IGNITE = 0;
                delay_ms_wdt((uint16_t)pulse_time * 10);
                WELD_ALLOW = 1;
                // Пауза с проверкой кнопки для быстрого выхода
                for(uint8_t i=0; i<45; i++) { 
                    if(BTN_START) break; 
                    delay_ms_wdt(10); 
                }
                if(BTN_START) break;
            }
            ARC_IGNITE = 0; WELD_ALLOW = 1;
            break;

        case 3: // TACK MODE
            WELD_ALLOW = 0; 
            ARC_IGNITE = 1;
            __delay_ms(30); 
            ARC_IGNITE = 0;
            delay_ms_wdt((uint16_t)tack_time * 10);
            WELD_ALLOW = 1;
            while(!BTN_START) CLRWDT();
            break;
    }

    // 2. ПОСТ-ГАЗ
    delay_ms_wdt((uint16_t)gas_time * 100);
    GAS_RELAY = 0;
    
    // Финальная страховка
    ARC_IGNITE = 0;
    WELD_ALLOW = 1;
}

void settings_menu(void) {
    uint8_t *p;
    if(mode == 0) p = &gas_time;
    else if(mode == 1) p = &arc_init_val;
    else if(mode == 2) p = &pulse_time;
    else p = &tack_time;

    // Индикация входа: мигнуть шкалой
    LED_BAR = 0x00; __delay_ms(150); LED_BAR = 0xFF; __delay_ms(150);

    while(1) {
        CLRWDT();
        LED_BAR = ~(1 << ((*p / 4) & 0x07)); 
        
        if(!BTN_UP) { if(*p < 31) (*p)++; delay_ms_wdt(150); }
        if(!BTN_DOWN) { if(*p > 1) (*p)--; delay_ms_wdt(150); }
        
        if(!BTN_START) { 
            save_settings();
            while(!BTN_START) CLRWDT();
            __delay_ms(100); 
            return;
        }
        __delay_ms(20);
    }
}

void delay_ms_wdt(uint16_t ms) {
    while(ms--) { __delay_ms(1); CLRWDT(); }
}

void save_settings(void) {
    if(eeprom_read(0x00) != mode) eeprom_write(0x00, mode);
    if(eeprom_read(0x01) != gas_time) eeprom_write(0x01, gas_time);
    if(eeprom_read(0x02) != arc_init_val) eeprom_write(0x02, arc_init_val);
    if(eeprom_read(0x03) != pulse_time) eeprom_write(0x03, pulse_time);
    if(eeprom_read(0x04) != tack_time) eeprom_write(0x04, tack_time);
}

void load_settings(void) {
    mode = eeprom_read(0x00);
    gas_time = eeprom_read(0x01);
    arc_init_val = eeprom_read(0x02);
    pulse_time = eeprom_read(0x03);
    tack_time = eeprom_read(0x04);
    
    // Защита от мусора в EEPROM (особенно для Proteus)
    if(mode > 3) mode = 0;
    if(gas_time > 31) gas_time = 5;
    if(arc_init_val > 31) arc_init_val = 10;
    if(pulse_time > 31) pulse_time = 10;
    if(tack_time > 31) tack_time = 5;
}