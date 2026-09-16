# smart-energy-meter-atmega328p

# Smart Energy Meter with Load Control using ATmega328P

An embedded system-based Smart Energy Meter designed to monitor AC mains voltage, current, active power, and cumulative energy consumption in real time. It features automatic overload protection with a latching relay mechanism, a reset interface, and a dual-page scrolling LCD display.

---

## 🚀 Features

* **Real-Time Measurement:** Monitors voltage, current, and calculated power ($P = V \times I$).
* **Automatic Overload Protection:** Automatically trips the relay to cut off the load if current exceeds the safe threshold (5.0A).
* **Latch Mechanism & Peak Tracking:** Latches the system in an OFF state during an overload event while capturing peak current and peak power values.
* **Cumulative Energy Tracking:** Integrates power over time using Timer1 interrupts to calculate energy consumed in Watt-hours ($Wh$).
* **Interactive UI:** Cycles through two display pages on a 16x2 LCD via PORTD to show voltage, current, power, relay status, and total energy used.
* **Manual Reset Button:** Allows users to clear energy counters, reset the overload latch, and restore power.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** ATmega328P (running at 16 MHz)
* **Voltage Sensor:** Voltage divider circuit connected to analog pin `A0`
* **Current Sensor:** ACS712 (5A variant) connected to analog pin `A1`
* **Display:** 16x2 LCD (operated in 4-bit mode via `PORTD`)
* **Actuator:** Relay module connected to `PB0`
* **Input:** Reset push button connected to `PC2`

---

## 📝 Code Overview

The firmware is written in native **Embedded C** using AVR-GCC libraries. Key components include:
* **ADC Handling:** Oversamples and averages 16 readings to ensure stable voltage and current measurements.
* **Timer1 Interrupt:** Generates precise 1-second intervals (`ISR(TIMER1_COMPA_vect)`) to accurately accumulate energy consumption in Watt-hours.
* **State Machine & Display:** Alternates between monitoring metrics (Page 0) and cumulative energy stats (Page 1) every 5 seconds.

---

## 💻 Installation & Usage

1. Clone or download this repository.
2. Open the `main.c` file in your preferred AVR development environment (e.g., Microchip Studio, VS Code with AVR toolchain, or Arduino IDE configured for bare-metal AVR).
3. Compile the code and flash the resulting `.hex` file onto your **ATmega328P** microcontroller.
