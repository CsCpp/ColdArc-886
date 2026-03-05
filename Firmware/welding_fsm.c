/**
 * File / Файл: welding_fsm.c
 * Description: Implementation of welding modes and timing
 * Описание: Реализация режимов сварки и временных циклов
 */

#include "welding_fsm.h"
#include "hardware.h"
#include "settings.h"
#include "config.h"

static weld_state_t state = S_IDLE;
static uint16_t safety_counter = 0; // Safety weld timer / Защитный таймер

void FSM_Process(void) {
    // 1. Safety check: prevent long continuous arc (2 minutes)
    // 1. Защита: прерывание дуги при превышении 2-х минут
    if (state >= S_WELD_2T && state <= S_TACK_PROCESS) {
        if (++safety_counter > (SAFETY_MAX_WELD_SEC * 1000)) {
            state = S_EMERGENCY;
        }
    } else {
        safety_counter = 0;
    }

    // 2. State Machine / Конечный автомат
    switch(state) {
        case S_IDLE:
            HW_SetRelays(0, 1, 0); // All OFF / Все выключено
            if (!BTN_START && edit_state == EDIT_OFF) {
                state_timer = (uint16_t)cfg.pre_gas * 100; // 0.1s step
                state = S_PRE_GAS;
            }
            break;

        case S_PRE_GAS:
            HW_SetRelays(1, 1, 0); // Gas ON / Газ ВКЛ
            if (state_timer == 0) {
                HW_SetRelays(1, 0, 1); // Gas + Weld + HF ON
                state_timer = (uint16_t)cfg.arc_init * 100;
                state = S_ARC_INIT;
            }
            break;

        case S_ARC_INIT:
            if (state_timer == 0) {
                HW_SetRelays(1, 0, 0); // HF OFF, keep Weld / Осциллятор ВЫКЛ
                if (cfg.mode == 0) state = S_WELD_2T;
                else if (cfg.mode == 1) state = S_WELD_4T_STEP1;
                else if (cfg.mode == 2) state = S_COLD_PULSE;
                else {
                    state_timer = 500; // Fixed Tack time 0.5s / Прихватка 0.5с
                    state = S_TACK_PROCESS;
                }
            }
            break;

        case S_WELD_2T:
            if (BTN_START) { // Trigger released / Кнопка отпущена
                state_timer = (uint16_t)cfg.post_gas * 100;
                state = S_POST_GAS;
            }
            break;

        case S_WELD_4T_STEP1: // Waiting for release after start / Ожидание отпускания
            if (BTN_START) state = S_WELD_4T_STEP2;
            break;

        case S_WELD_4T_STEP2: // Welding in 4T / Сварка в 4Т
            if (!BTN_START) state = S_WELD_4T_STEP3;
            break;

        case S_WELD_4T_STEP3: // Waiting for press to stop / Ожидание нажатия для СТОП
            if (BTN_START) {
                state_timer = (uint16_t)cfg.post_gas * 100;
                state = S_POST_GAS;
            }
            break;

        case S_COLD_PULSE:
            HW_SetRelays(1, 0, 1); // Short pulse with HF / Короткий импульс с ВЧ
            state_timer = 40;      // 40ms pulse / 40мс импульс
            state = S_COLD_PAUSE;
            break;

        case S_COLD_PAUSE:
            if (state_timer == 0) {
                HW_SetRelays(1, 1, 0); // Current OFF / Ток ВЫКЛ
                if (BTN_START) { // Finish if trigger released / Конец если кнопка отпущена
                    state_timer = (uint16_t)cfg.post_gas * 100;
                    state = S_POST_GAS;
                } else {
                    state_timer = (uint16_t)cfg.pulse_time * 100; // Pause from settings
                    if (state_timer == 0) state = S_COLD_PULSE; // Loop / Цикл
                }
            }
            break;

        case S_POST_GAS:
            HW_SetRelays(1, 1, 0); // Gas only / Только газ
            if (state_timer == 0 || !BTN_START) { // Re-start allowed / Повторный старт разрешен
                state = S_IDLE;
            }
            break;

        case S_EMERGENCY:
            HW_SetRelays(0, 1, 0); // Shutdown / Аварийное выключение
            if (BTN_START) state = S_IDLE; // Reset after button release / Сброс
            break;
    }
}