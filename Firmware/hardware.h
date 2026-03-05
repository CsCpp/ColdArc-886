/**
 * File / Файл: hardware.h
 * Description: Hardware Abstraction Layer (HAL) interfaces
 * Описание: Интерфейсы уровня абстракции оборудования
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

// Initialize ports and timers / Инициализация портов и таймеров
void HW_Init(void);

// Set state of relays / Управление состоянием реле
// gas: 1=ON, weld: 0=ON (Active Low), arc: 1=ON
void HW_SetRelays(uint8_t gas, uint8_t weld, uint8_t arc);

// Update LED bar based on mode / Обновление шкалы в зависимости от режима
void HW_UpdateDisplay(uint8_t mode);

// Error blinking for startup check / Мигание при ошибке старта
void HW_BlinkAlert(uint16_t ms_tick);

#endif // HARDWARE_H