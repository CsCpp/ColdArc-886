/**
 * File / Файл: welding_fsm.h
 * Description: Welding Finite State Machine (Logic)
 * Описание: Конечный автомат сварки (Логика процесса)
 */

#ifndef WELDING_FSM_H
#define WELDING_FSM_H

#include <stdint.h>

// --- WELDING STATES / СОСТОЯНИЯ СВАРКИ ---
typedef enum {
    S_IDLE,         // Ready to start / Ожидание
    S_PRE_GAS,      // Pre-flow / Пред-газ
    S_ARC_INIT,     // HF Start / Поджиг (Осциллятор)
    S_WELD_2T,      // 2T mode active / Сварка 2Т
    S_WELD_4T_STEP1,// 4T: Waiting for trigger release / 4Т: Ожидание отпускания
    S_WELD_4T_STEP2,// 4T: Welding / 4Т: Процесс сварки
    S_WELD_4T_STEP3,// 4T: Waiting for stop trigger / 4Т: Ожидание нажатия стоп
    S_COLD_PULSE,   // Cold: Current pulse / Cold: Импульс тока
    S_COLD_PAUSE,   // Cold: Pause between pulses / Cold: Пауза
    S_TACK_PROCESS, // Tack: Fixed time weld / Прихватка
    S_POST_GAS,     // Post-flow / Пост-газ
    S_EMERGENCY     // Safety timeout / Аварийная остановка
} weld_state_t;

// Main processing function / Главная функция процесса
void FSM_Process(void);

// Global access to timers / Доступ к таймерам извне
extern volatile uint16_t state_timer;

#endif // WELDING_FSM_H