# 🌍 Personalized Climate Control System (IoT Project)

A real-time IoT-based dashboard to monitor and control indoor climate parameters including **temperature, humidity, CO₂ level, feels-like temperature, occupancy**, and **smart control of devices** such as **Fan**, **AC**, **Air Purifier**, and **Ventilation** system.

## 🌟 Features

* Real-time sensor data via **ESP32** and **MQTT (EMQX Cloud)**
* Device control through **Auto / Manual / Disabled** modes
* Live charts for environmental parameters
* Automatic fallback to "Disabled" mode when room is unoccupied
* Responsive UI with occupancy-triggered automation logic
* MQTT-to-MySQL logger to persist sensor data

---

## 🚀 Project Folder Structure

```
IoT_Project(PersonalizedClimateControlSystem)/
├── Aim.png                      # Diagrammatic aim of the system
├── esp1.ino                     # ESP32 code 1 (Data Publishing + Device Control)
├── esp2.ino                     # ESP32 code 2 for occupancy calculation
├── IoT_Project_Proposal.pdf     # Initial idea and proposal
├── mqtt-to-mysql.js             # MQTT subscriber to log data to MySQL
├── package.json                 # Node.js metadata
├── Project_Presentation.pdf     # Slides for presentation
├── TheoryAnalysis&Thresholds.pdf # Threshold logic behind decisions
├── public/
    ├── index.html              # Dashboard frontend
    ├── script.js               # All JS logic for charting, MQTT, control
    └── styles.css              # UI styles and responsive design
```

---

## 🧰 Thresholds and Automation Logic

| Parameter       | Condition  | Action                         |
| --------------- | ---------- | ------------------------------ |
| Feels-Like Temp | > 35°C     | Turn ON Fan                    |
| Feels-Like Temp | > 45°C     | Turn ON AC                     |
| CO₂ Level       | > 1600 ppm | Open Ventilation               |
| CO₂ Level       | > 1300 ppm | Turn ON Air Purifier           |
| CO₂ Level       | > 1700 ppm | Alert with Buzzer + LED        |
| Occupancy       | 0 people   | Switch to DISABLED Mode        |
| Occupancy       | ≥1 person  | Restore last known active mode |

---

## 🔧 How to Run

### ✅ Requirements

* ESP32 microcontroller
* Node.js installed
* MQTT broker (e.g., EMQX Cloud)
* MySQL database (optional for logging)

### 1. Flash ESP32

* Upload `esp1.ino` and `esp2.ino` via Arduino IDE.
* Configure WiFi credentials and MQTT broker in code.

### 2. Start Dashboard

```bash
open index.html # or use Live Server in VS Code
```

### 3. Log Data to MySQL (Optional)

First, install the required dependencies by running:

```bash
npm install
```

Then start the MQTT to MySQL logging script with:
```bash
npm start
```

Ensure `package.json` and MySQL credentials are correctly configured.

---

## 🎨 Dashboard Overview

* **Line Charts**: Real-time plotting using Chart.js
* **Device Status**: Coloured dots (green/red) with textual status
* **Mode Switching**: Auto / Manual / Disabled with toggle checkboxes
* **Device Controls**: Toggle AC, Fan, Purifier, Vent in manual mode only

> Mode switching triggers confirmation prompts. Only one mode active at a time.

---

## 🚀 Technologies Used

- **ESP32** (Microcontroller)  
- **MQTT (EMQX Cloud)** for real-time data communication  
- **Node.js** for backend data logging to MySQL  
- **Chart.js** for dynamic and real-time graph visualization  
- **HTML, CSS, JavaScript** for building the interactive frontend dashboard  

## 🔧 Sensors Used

- **DHT22** — Measures temperature and humidity accurately  
- **MQ-135** — Detects air quality and CO₂ concentration  
- **Infrared (IR) Sensors** — Track room occupancy by detecting motion  

## ⚙️ Actuators and Output Devices

- **Servo Motor** — Controls the vent opening and closing  
- **Relays/LEDs** — Control and indicate the status of the fan, air conditioner, and air purifier  
- **Buzzer & Alert LEDs** — Signal alerts for poor air quality conditions

---

## 👨‍💼 Contributors

* Shivansh Santoki
* Ayush Kanani
* Dhyey Thummar

---

### 📄 License

This project is for educational purposes only. Feel free to fork and improve!

---
