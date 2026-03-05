⚡ ColdArc 886 — Professional TIG/Cold Weld Controller (v2.0)
ColdArc 886 — это цифровой контроллер для сварочных инверторов на базе PIC16F886. В версии 2.0 программная часть полностью переписана на архитектуру конечного автомата (FSM), что обеспечивает прецизионную точность таймингов и промышленную надежность.
🇷🇺 Описание (RU)
🌟 Основные изменения в v2.0
Архитектура FSM: Процесс сварки разделен на 12 независимых состояний. Никаких блокирующих задержек (delay_ms), полный контроль системы в реальном времени.
Системный Тик (1мс): Все временные интервалы управляются аппаратным таймером TMR0.
Безопасность (Safety Timeout): Автоматическое прерывание дуги через 120 секунд (защита от залипания кнопки/перегрева).
Защита при старте: Блокировка работы, если кнопка горелки нажата в момент включения питания.
Улучшенный Cold Weld: Идеально стабильные импульсы для сварки тонких металлов «чешуйкой».
🛠 Технические характеристики
МК: PIC16F886 (8 МГц Internal RC).
Режимы: 2Т (Ручной), 4Т (Полуавтомат), Cold Weld (Импульсный), Tack (Прихватка).
Изоляция: Требуется гальваническая развязка всех выходов (рекомендуются оптопары PC817).
🇺🇸 Description (EN)
🌟 Key Updates in v2.0
FSM Architecture: The welding process is now managed by a 12-state Finite State Machine. Zero blocking delays, enabling real-time system monitoring.
System Tick (1ms): All timings are driven by the hardware TMR0 interrupt for microsecond precision.
Safety Timeout: Hardware-level arc cutoff after 120 seconds to prevent torch overheating or stuck-button accidents.
Power-On Protection: System stays in safe mode if the torch trigger is depressed during power-up.
Enhanced Cold Weld: Rock-solid pulse consistency for high-quality "fish-scale" welds on thin sheets.
🛠 Specifications
MCU: Microchip PIC16F886 (8 MHz Internal).
Modes: 2T (Manual), 4T (Latch), Cold Weld (Pulse), Tack (Spot).
Control: Active-Low logic for buttons and LED bar; Opto-isolated outputs.
🕹 Pinout / Пиновка (FSM Logic)
Pin	Name	Function (RU/EN)
RA0-RA7	LED_BAR	Индикация параметров / LED Bar Graph (Active-Low)
RB1	GAS_RELAY	Газовый клапан / Gas Valve (1 = ON)
RB2	WELD_ALLOW	Разрешение тока / Weld Current (0 = ON/Safe-fail)
RB3	BTN_START	Кнопка горелки / Torch Trigger (Active-Low)
RB4	BTN_UP	Режим / Вверх / Mode / Up
RB5	BTN_DOWN	Продувка / Вниз / Gas Purge / Down
RC0	ARC_IGNITE	Осциллятор (ВЧ) / HF Ignition (1 = ON)
🚀 Installation / Установка
Compile using XC8 v2.x+.
Flash via PICkit 3/4.
Important: Disable LVP in programmer settings. / Важно: Отключите LVP в настройках программатора.
⚠️ Disclaimer
RU: Использование данного устройства со сварочным оборудованием сопряжено с риском. Автор не несет ответственности за повреждение оборудования.
EN: High voltage/current hazard. Use at your own risk. Always ensure proper galvanic isolation for the control board.

📊 Настройка параметров / Parameter Limits
Каждому режиму соответствует свой настраиваемый параметр, отображаемый на шкале LED_BAR (8 светодиодов).
Each mode has a dedicated parameter adjustable via the LED_BAR (8-step scale).
Режим (Mode)	Параметр (Parameter)	Диапазон (Range)	Описание (Description)
2T	Gas Time	0.1 - 3.1 сек	Время пред-газа и пост-газа / Pre & Post flow.
4T	Arc Init	0.1 - 3.1 сек	Время работы осциллятора (ВЧ) / HF Start duration.
Cold Weld	Pulse Time	10 - 310 мс	Пауза между импульсами тока / Delay between pulses.
Tack	Tack Time	10 - 310 мс	Длительность одиночной точки / Single spot time.
🔌 Схема подключения / Wiring Diagram (Text-based)
Для защиты PIC16F886 обязательно используйте опторазвязку.
Mandatory: Use optocouplers (e.g., PC817) for all power-side signals.
text
       [ PIC16F886 ]           [ ISOLATION / RELAY ]          [ WELDING MACHINE ]
      ----------------       -----------------------        ---------------------
      RB1 (Gas)   --------> [ Opto -> Relay 12V ] --------> Gas Valve (Solenoid)
      RB2 (Weld)  --------> [ Opto -> Relay/FET ] --------> Inverter Trigger (PWM)
      RC0 (HF)    --------> [ Opto -> Relay/SCR ] --------> HF Ignition / Oscillator
      
      RB3 (Start) <-------- [ Torch Switch ] <------------- GND (Vss)
      RB4 (Mode)  <-------- [ Button UP ] <---------------- GND (Vss)
      RB5 (Down)  <-------- [ Button DOWN ] <-------------- GND (Vss)
      
      RA0-RA7     --------> [ 220 Ohm Resistors ] --------> [ LED BAR Graph (Cathode)]
