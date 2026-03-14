# 🛠 TIG Welding Controller (PIC16F886)

## 📖 Project Overview / Обзор проекта

### English
This project provides a robust firmware solution for DIY TIG welding machines based on the PIC16F886 microcontroller. It transforms a basic board into a professional-grade controller capable of managing gas flow, high-frequency (HF) arc ignition, and complex welding sequences.

### Русский
Этот проект представляет собой надежную прошивку для самодельных сварочных аппаратов TIG на базе микроконтроллера PIC16F886. Прошивка превращает плату в контроллер, способный управлять подачей газа, осциллятором поджига (HF) и сложными циклами сварки.

---

## ⚡ Key Features / Основной функционал

### English
* **4 Operating Modes:** 2T, 4T, Cold Pulse (Cold Weld), and Tack (Spot).
* **Precise Timing:** Configurable Pre-flow and Post-flow gas control.
* **HF Ignition:** Integrated arc ignition delay logic.
* **Custom UI:** Adjustable parameters via LED bar scale.
* **Non-volatile Storage:** Saves settings to internal EEPROM.

### Русский
* **4 режима работы:** 2Т, 4Т, Cold Pulse (холодная сварка) и Tack (прихватки).
* **Точные тайминги:** Настраиваемая длительность предпродувки и постпродувки.
* **HF Поджиг:** Логика задержки для работы с осциллятором.
* **Интерфейс:** Регулировка параметров через светодиодную шкалу.
* **Память:** Автоматическое сохранение настроек в EEPROM.

---

## 🔌 Hardware Mapping / Подключение периферии



| Component / Компонент | Pin / Вывод | Function / Назначение |
| :--- | :--- | :--- |
| **Gas Valve / Газ** | **RC0** | Gas solenoid relay |
| **HF Arc / Поджиг** | **RB1** | Oscillator control |
| **Weld Relay / Сварка**| **RB2** | Main contactor (Active Low) |
| **LED Bar / Шкала** | **PORTA** | LED scale indicators |
| **Torch Start / Пуск**| **RB3** | Torch button trigger |
| **Button UP / Вверх** | **RB4** | Menu navigation |
| **Button DOWN / Вниз**| **RB5** | Menu navigation / Gas purge |

> **⚠️ NOTE:** Bits 6 and 7 of `PORTA` are swapped in software to match non-sequential physical PCB layout.

---

## 🚀 Programming & Setup Guide / Настройка и прошивка



### English: Development Environment
1. **Setup:** Install **MPLAB X IDE** and **XC8 Compiler**.
2. **Project:** Create a new "Standalone Project" for **PIC16F886**.
3. **Compile:** Add the `main.c` file to "Source Files". Click "Clean and Build".
4. **Fuses:** Ensure `#pragma config LVP = OFF` is in your code.
5. **Flash:** In `Project Properties -> PICkit 3 -> Power`, enable "Power target circuit" (5.0V). Click "Make and Program Device".

### Русский: Среда разработки и прошивка
1. **Установка:** Установите **MPLAB X IDE** и компилятор **XC8**.
2. **Проект:** Создайте "Standalone Project" для **PIC16F886**.
3. **Компиляция:** Добавьте `main.c` в "Source Files". Нажмите "Clean and Build".
4. **Фьюзы:** Убедитесь, что в коде стоит `#pragma config LVP = OFF`.
5. **Прошивка:** В `Project Properties -> PICkit 3 -> Power` включите "Power target circuit" (5.0V). Нажмите "Make and Program Device".

---
# 🛠 Настройки контроллера сварочного аппарата (TIG/Cold Weld)

## 1. Режимы сварки (Кнопка UP)
* **1 LED:** 2T (Обычный) — варим, пока держим кнопку.
* **2 LED:** 4T (Триггер) — нажали/отпустили = старт, нажали/отпустили = стоп.
* **3 LED:** Cold Weld (Холодная сварка) — серия точек.
* **4 LED:** Tack (Точка) — один точный "выстрел".

---

## 2. Параметры меню (Вход: UP + DOWN зажать)
Переход между пунктами — кнопка горелки **START**.

| Параметр (мигающий LED) | Функция | Диапазон (1-8) |
| :--- | :--- | :--- |
| **1-й светодиод** | Предгаз | 0.25 – 2.0 сек |
| **2-й светодиод** | Постгаз | 0.375 – 3.0 сек |
| **3-й светодиод** | Время дуги (HF) | 0.1 – 0.8 сек |
| **4-й светодиод** | Длительность точки | 0.1 – 0.8 сек |

### ⚡️ Особенности режима Cold Weld (LED 3):
В этом режиме два последних параметра работают иначе:
* **Время дуги (Arc):** Длительность вспышки (10 – 80 мс).
* **Параметр Pulse:** Длительность паузы (50 – 400 мс).

---

## 3. Системы защиты
* **Защита от ВЧ-помех:** Опрос кнопок с фильтром 160 мс.
* **Безопасный старт:** Блокировка поджига, если кнопка зажата при включении в розетку.
* **Anti-Glitch:** Игнорирование всех кнопок, кроме основной, во время горения дуги.
---

## 🛠 Troubleshooting / Решение проблем

* **LED 7/8 issue:** Expected behavior; bits 6 and 7 of `PORTA` are swapped in software.
* **Programming Error:** Check your cables (keep them <15cm) and verify the 10k pull-up resistor on **MCLR** (pin 1).
* **HF Noise:** Add 100nF ceramic capacitor across VDD/VSS near the MCU.
