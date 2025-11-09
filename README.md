# Reactive LED Light Strip

### Motion-Triggered Ambient Lighting with Candle-Like Flicker

---

## 🧭 Overview

This Arduino project implements a **reactive LED light strip** controlled by **two PIR motion sensors** and an **LDR (light-dependent resistor)**.

When motion is detected from the left or right sensor, the LED strip **fades in sequentially** from that side. Once fully on, the LEDs remain active and simulate a **natural candle flicker effect** using PWM modulation.

If no movement is detected for a defined time, the LEDs **gradually fade out** until the strip turns off completely.

The system only activates when the ambient light (measured by the LDR) is below a defined threshold — ensuring it operates only in the dark.

---

## ⚙️ Features

* **Directional activation**
  Motion from the left or right side triggers a corresponding fade-in animation.

* **Realistic candle flicker**
  Each LED channel runs a unique PWM flicker sequence, producing an organic, unsynchronized glow.

* **Automatic shutoff**
  The strip stays on as long as motion continues. After a set delay (`onTimeLight`), it fades out automatically.

* **Light-sensitive control**
  The system ignores motion if ambient light exceeds the threshold (`lumidity`).

* **Fully configurable parameters**
  Timing, brightness, and flicker characteristics can easily be adjusted in the code.

---

## 🧩 Hardware

| Component                            | Description                                            |
| ------------------------------------ | ------------------------------------------------------ |
| **Arduino (e.g. Uno/Nano)**          | Main controller                                        |
| **LED strip**                        | 6 channels, connected to PWM pins (3, 5, 6, 9, 10, 11) |
| **2× PIR motion sensors**            | Left sensor → Pin 2, Right sensor → Pin 4              |
| **LDR (photoresistor)**              | Connected to analog input A0                           |
| **Optional resistors & transistors** | Depending on LED current requirements                  |

---

## 🔌 Pin Configuration

| Function     | Pin                |
| ------------ | ------------------ |
| LED channels | 3, 5, 6, 9, 10, 11 |
| PIR (Left)   | 2                  |
| PIR (Right)  | 4                  |
| LDR          | A0                 |

---

## 🧠 State Machine

| State | Description                                           |
| ----- | ----------------------------------------------------- |
| **0** | LEDs off – waiting for motion; LDR threshold check    |
| **1** | Fade-in from left (sequential)                        |
| **2** | Fade-in from right (sequential)                       |
| **3** | Fully on – candle flicker active; motion resets timer |
| **4** | Fade-out (all channels)                               |

---

## 🔧 Key Parameters

| Constant           | Description                             | Default |
| ------------------ | --------------------------------------- | ------- |
| `onTimeLight`      | Time (ms) LEDs remain on without motion | `10000` |
| `dimSpeedHi`       | Fade-in speed (lower = faster)          | `3`     |
| `dimSpeedLo`       | Fade-out speed (lower = faster)         | `5`     |
| `lumidity`         | LDR threshold for activation            | `200`   |
| `FLICKER_DELAY_MS` | Flicker update interval                 | `35 ms` |

---

## 🕯️ Candle Flicker Effect

Each LED channel cycles independently through a set of predefined brightness values (`candleVals[]`). This creates a warm, dynamic flicker resembling candlelight. The indices for each LED are offset to avoid synchronized motion.

---

## 🧰 Setup Instructions

1. Connect all components according to the **pinout table** above.
2. Upload the code to your Arduino using the **Arduino IDE**.
3. Adjust constants (e.g., `lumidity`, `onTimeLight`) as needed.
4. Power up the circuit — the LEDs should remain off until ambient light is low and motion is detected.

---

## 📂 File Structure

```
Reactive_LED_Strip/
├── Reactive_LED_Strip.ino
└── README.md
```

---

## 🚀 Future Improvements

* Non-blocking fade animations (using millis() instead of delay)
* Adjustable flicker speed or randomization intensity
* Configurable via serial or Bluetooth interface
* Optional ambient-color mode using RGB LED strips

---

## 📜 License

MIT License Copyright (c) 2025 Julian Kampitsch 

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

