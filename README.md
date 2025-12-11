# Punch Speed Calculator

**Test, measure, and compare your punch speed!**

A wearable device that measures punch speed in real-time. It uses an IMU to capture acceleration data, an STM32 microcontroller to compute velocity, and an OLED display to show results instantly.

Press start, raise your fist, and on the count of 3, punch!

<p align="center">
  <img src="https://github.com/user-attachments/assets/d734e465-986b-4821-9777-5d95d125b6bd" width="280" />
</p>

<p align="center">
  <a href="https://github.com/user-attachments/assets/401e8356-cbd4-48f5-857c-f3cd18cc75a6">📹 Watch Video Demo</a>
</p>

## Features

- **Real-time speed measurement** — Results displayed in km/h
- **Audible countdown** — 3-2-1 buzzer countdown before measurement
- **Compact wearable design** — Straps to your wrist
- **Battery powered** — Runs on a single AAA battery
- **1000 Hz sampling rate** — Captures acceleration every 1ms for accurate results

---

## PCB Design

The PCB was designed in **Altium Designer** and simulated in **LTspice**. The board integrates the IMU, OLED display, AAA battery holder, boost converter, buzzer, and STM32 Blackpill development board.


| Component | Description |
|-----------|-------------|
| MCU | STM32F411CEU6 (Blackpill) |
| IMU | MPU6050 (3-axis accelerometer/gyroscope) |
| Display | 128×64 OLED (I2C, SSD1306) |
| Power | AAA battery with 1.5V → 3.3V boost converter |
| Input | Tactile push button (start) |
| Audio | Piezo buzzer (countdown) |


<p align="center">
  <img src="https://github.com/user-attachments/assets/48c665f6-3664-49e9-9fc3-91480ee0d0ef" width="48%" />
  <img src="https://github.com/user-attachments/assets/a605d98c-3192-418c-b760-b5ef0c8e9ead" width="48%" />
</p>

---

## Enclosure

The enclosure was designed in **SolidWorks** and 3D printed. It features PCB mounting posts, a protective barrier for components, and slots for a velcro wrist strap.

<p align="center">
  <img src="https://github.com/user-attachments/assets/55688187-96d2-4980-809e-3ef7479667b8" width="48%" />
  <img src="https://github.com/user-attachments/assets/d856b182-4ac0-42fe-977a-2433b961a405" width="48%" />
</p>

---

## STM32 Firmware

The firmware is written in **C** using the **STM32 HAL** library in **STM32CubeIDE**.

### Program Flow

1. **Idle State** — Display shows "PRESS START" and waits for button press
2. **Countdown** — Buzzer sounds 3 times (250ms on, 500ms off) while display shows 3, 2, 1
3. **Data Capture** — IMU records X-axis acceleration every 1ms for 1 second (1000 samples)
4. **Processing** — Filters for positive acceleration values only (punch phase, excludes impact deceleration)
5. **Calculation** — Computes average acceleration and derives final velocity
6. **Display** — Shows punch speed in km/h for 5 seconds

### Speed Calculation

```
velocity = average_acceleration × gravity × time
speed (km/h) = velocity × 3.6
```

Where:
- `average_acceleration` = sum of positive samples ÷ count
- `gravity` = 9.81 m/s²
- `time` = count × 0.001s

### Typical Results

| Punch Type | Speed |
|------------|-------|
| Light jab | 10–20 km/h |
| Casual punch | 20–35 km/h |
| Hard punch | 35–50 km/h |
| Trained boxer | 50–80+ km/h |
