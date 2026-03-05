# ColdArc 886 — Universal TIG Welding Controller

**ColdArc 886** is a high-precision digital controller for TIG welding machines based on the **PIC16F886** microcontroller. It upgrades standard welding inverters with advanced features like Cold Welding (Pulse), Tack welding, and automated 4T logic.



## 🌟 Key Features
* **4 Operating Modes:** * **2T:** Classic manual mode.
    * **4T:** Semi-automatic "latch" mode for long seams.
    * **Cold Welding (Pulse):** Periodic high-precision pulses for thin metals.
    * **Tack Welding:** Single ultra-short spot pulse for assembly.
* **HF Ignition Control:** Synchronized high-frequency arc starting on every pulse.
* **Gas Management:** Adjustable Pre-gas and Post-gas timing.
* **Internal 8MHz Clock:** No external crystal required, maximizing I/O availability.
* **EEPROM Storage:** All settings are automatically saved and restored after power loss.

## 🛠 Hardware Specifications
* **MCU:** Microchip PIC16F886.
* **Clock:** 8 MHz Internal RC.
* **User Interface:** 3 tactile buttons, 8-LED bar graph (Active-Low).
* **Isolation:** Full galvanic isolation via optocouplers (PC817) for Gas, Weld current, and HF-Ignition.

### Pinout Mapping
| Pin | Function | Logic |
|--- |--- |--- |
| **RA0-RA7** | LED Bar Graph | Active Low (0 = ON) |
| **RB1** | Gas Valve Relay | Active High |
| **RB2** | Weld Current Enable| Active Low (Fail-safe) |
| **RB3** | Start Button (Torch)| Active Low (Internal Pull-up) |
| **RB4** | Mode / Up Button | Active Low (Internal Pull-up) |
| **RB5** | Gas Purge / Down | Active Low (Internal Pull-up) |
| **RC0** | HF Start (Oscillator)| Active High |



## 🕹 Navigation & Settings
* **Switch Mode:** Short press **UP (RB4)**.
* **Gas Purge:** Hold **DOWN (RB5)** to test gas flow.
* **Enter Settings:** Long press **UP (RB4)** (2 sec).
    * Use **UP/DOWN** to adjust values.
    * Short press **START (RB3)** to switch between parameters.
    * Long press **UP (RB4)** to Save & Exit.

## 💻 Installation & Firmware
1. Open the project in **MPLAB X IDE**.
2. Compile using **XC8 Compiler**.
3. Flash the `.hex` file using **PICkit 3/4**.
4. **Note:** Ensure LVP (Low Voltage Programming) is DISABLED in your programmer settings to use RB3.

## ⚠️ Safety Warning
This device controls high-power welding equipment. Improper wiring or lack of galvanic isolation can destroy the microcontroller or cause injury. Use optocouplers for all power-side connections!"# ColdArc-886"  
