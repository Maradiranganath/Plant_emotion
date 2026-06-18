# Plant_emotion
Arduino code and connection details available here 


# 🌱 Emotion Plant — Soil Moisture Detector with OLED Display

An Arduino-based soil moisture detector that displays live data on an OLED screen — including a **plant emotion face** that changes mood based on how dry or wet the soil is! Switch between 3 display modes using a single push button.

**By Maradi Innovations**

---

## 📺 Demo

Watch it in action → [Maradi Innovations YouTube](https://youtube.com/@MaradiInnovations)

---

## ✨ Features

- 📊 **Screen 1 — Status Display**: Moisture percentage, live fill bar, status label
- 🌿 **Screen 2 — Emotion Plant**: Big animated plant face that reacts to moisture level (happy, sad, dizzy, worried)
- 📟 **Screen 3 — Gauge Display**: Analog-style meter with smooth animated needle
- 🔘 Single button cycles through all 3 screens
- 🎯 5 moisture levels: DRY, LOW, GOOD, WET, FLOOD — each with a unique face expression

---

## 🛒 Components Used

| Component | Qty |
|---|---|
| Arduino Pro Mini (5V/16MHz) | 1 |
| SSD1306 0.96" I2C OLED Display (128x64) | 1 |
| Capacitive Soil Moisture Sensor V2.0.0 | 1 |
| Push Button | 1 |
| 9V Battery (6F22) | 1 |
| Jumper wires | as needed |

---

## 🔌 Wiring / Connections

| Component Pin | Arduino Pro Mini Pin |
|---|---|
| OLED SDA | A4 |
| OLED SCL | A5 |
| OLED VCC | VCC |
| OLED GND | GND |
| Sensor OUT | A0 |
| Sensor VCC | VCC |
| Sensor GND | GND |
| Button (leg 1) | D2 |
| Button (leg 2) | GND |
| 9V Battery (+) | RAW |
| 9V Battery (−) | GND |

> ⚠️ **Important**: Connect the 9V battery to the **RAW** pin only — never to VCC directly. The onboard regulator converts it to a safe 5V for the OLED and sensor.

> Button uses Arduino's internal pull-up — no external resistor needed.

---

## 📦 Libraries Required

Install via Arduino IDE → **Sketch → Include Library → Manage Libraries**:

- `Adafruit SSD1306`
- `Adafruit GFX Library`

---

## ⚙️ Calibration

Before use, calibrate the sensor for your soil. Open Serial Monitor (9600 baud) and note the raw ADC values:

```cpp
#define DRY_VALUE   620   // Raw reading in dry air
#define WET_VALUE   280   // Raw reading submerged in water
```

Update these two values in the code to match your sensor's readings for accurate percentage mapping.

---

## 🎮 How to Use

1. Upload the code to your Arduino Pro Mini
2. Power on — boot splash appears for 2 seconds
3. Press the button to cycle through:
   - **Screen 1** → Value & Status
   - **Screen 2** → Plant Emotion Face
   - **Screen 3** → Gauge Meter
4. The 3 dots at the bottom right indicate which screen is currently active

---

## 😊 Emotion Reference

| Moisture % | Status | Face Expression |
|---|---|---|
| 0–25% | DRY! | Sad, X eyes, sweat drops |
| 26–45% | LOW | Worried, raised brows, tear |
| 46–70% | GOOD | Happy, big smile, blush |
| 71–85% | WET | Neutral, flat mouth |
| 86–100% | FLOOD | Dizzy, spiral eyes |

---

## 📁 Files

- `soil_moisture_3screen.ino` — Main Arduino sketch

---

## 🔗 Connect

- YouTube: [Maradi Innovations](https://youtube.com/@MaradiInnovations)
- GitHub: [Maradiranganath](https://github.com/Maradiranganath)

---

## 📄 License

Free to use and modify for personal and educational projects.
