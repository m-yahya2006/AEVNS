# AEVNS

## Autonomous Electric Vehicle Navigation System

AEVNS is an undergraduate engineering project exploring the integration of **autonomous navigation, embedded sensing, energy monitoring, wireless telemetry, and machine learning** in a small mobile robotic platform.

I am developing the project as a **Civil Engineering student at GIKI** to explore how embedded systems, robotics, AI, and data-driven methods can complement physical engineering systems.

> **Project Status:** Active development. Individual sensing, control, and telemetry components have been developed and tested, while full mechanical integration and real-world data collection are still in progress.

---

## Project Goals

AEVNS is being developed as a practical learning and experimentation platform for:

- Autonomous obstacle-aware navigation
- Embedded sensor integration
- Battery and solar-energy monitoring
- Wireless robot telemetry
- Real-world data collection
- Machine-learning-based range prediction
- Future experimentation with intelligent engineering inspection

The project is not intended to compete with professional autonomous or inspection systems. Its purpose is to build practical experience connecting physical engineering, electronics, software, and intelligent systems.

---

## System Architecture

The current system is organized around an ESP32-based mobile robot.

```text
Ultrasonic Sensors ───────┐
                          │
Battery INA219 ───────────┤
Solar INA219 ─────────────┤
                          ├──> ESP32 ───> Motor Control
DS18B20 Temperature ──────┤       │
                                  │ Wi-Fi / UDP
                                  ▼
                            Python Receiver
                                  │
                                  ▼
                             CSV Dataset
                                  │
                                  ▼
                       Future ML Range Prediction
```

---

## Current Capabilities

### Autonomous Navigation

Three ultrasonic sensors monitor the:

- Left side
- Front
- Right side

The ESP32 uses these sensor readings to make basic navigation decisions such as continuing forward, slowing down, turning, correcting direction, or reversing when required.

### Energy Monitoring

The robot monitors its electrical system using:

- INA219 current and voltage sensing for the battery
- INA219 sensing for the solar subsystem
- DS18B20 temperature sensing
- Battery state-of-charge estimation

These measurements provide information about the robot's energy consumption and operating condition.

### Wireless Telemetry

The ESP32 transmits robot telemetry over **Wi-Fi using UDP**.

A Python receiver running on a computer listens for the telemetry stream and records the data in CSV format.

Currently logged information includes:

- Timestamp
- Battery voltage
- Battery current
- Battery power
- Estimated battery SOC
- Battery temperature
- Solar voltage
- Solar current
- Solar status
- Left, right, and front obstacle distances
- Navigation zones
- Navigation action
- Drive speed

This dataset will later be used for analysis and machine-learning experiments.

---

## Machine Learning Direction

One planned stage of AEVNS is to use data collected from the physical robot to estimate its remaining driving range.

The intended workflow is:

```text
Physical Robot
      │
      ▼
Sensor Measurements
      │
      ▼
Wireless Telemetry
      │
      ▼
CSV Dataset
      │
      ▼
Data Preparation
      │
      ▼
Regression Model
      │
      ▼
Estimated Remaining Range
```

The machine-learning component is **not presented as complete yet**. Meaningful model development requires real driving data collected after the mechanical platform is fully integrated.

---

## Hardware

Current and planned hardware includes:

- ESP32 DevKit V1
- 3 × HC-SR04 ultrasonic sensors
- 2 × INA219 voltage/current sensors
- DS18B20 temperature sensor
- L298N motor driver
- 2 × TT DC geared motors
- 18650 battery cells
- 6 V solar panel
- TP4056 charging modules
- Custom 3D-printed chassis

---

## Software & Technologies

### Embedded Systems

- C++
- Arduino / ESP32
- PWM motor control
- I2C
- OneWire
- Wi-Fi
- UDP

### Data & Machine Learning

- Python
- CSV data logging
- scikit-learn *(planned for range-prediction experiments)*

### Development

- Git
- GitHub
- VS Code

---

## Repository Structure

```text
AEVNS/
│
├── firmware/
│   └── aevns_firmware/
│       ├── aevns_firmware.ino
│       └── secrets.example.h
│
├── telemetry/
│   └── telemetry_receiver.py
│
├── machine-learning/
├── hardware/
├── docs/
│
├── .gitignore
└── README.md
```

The local `secrets.h` file contains Wi-Fi configuration and is intentionally excluded from version control.

---

## Development Status

### Completed / Tested

- [x] ESP32 firmware development
- [x] Ultrasonic sensor integration
- [x] Motor-control logic
- [x] Battery monitoring
- [x] Solar monitoring
- [x] Temperature sensing
- [x] Wi-Fi communication
- [x] UDP telemetry transmission
- [x] Python UDP receiver
- [x] CSV telemetry logging

### In Progress

- [ ] Final mechanical assembly
- [ ] Custom chassis preparation
- [ ] Complete system integration
- [ ] Testing under real driving load

### Planned

- [ ] Collection of real driving datasets
- [ ] Machine-learning range prediction
- [ ] Model evaluation and refinement
- [ ] Live telemetry dashboard
- [ ] Improved autonomous navigation
- [ ] Basic computer-vision experiments

---

## Security & Local Configuration

Wi-Fi credentials and machine-specific network settings are **not stored in this repository**.

To configure the firmware locally:

1. Copy `secrets.example.h`
2. Rename the copy to `secrets.h`
3. Enter the local Wi-Fi SSID, password, and telemetry receiver IP address
4. Keep `secrets.h` private

The repository's `.gitignore` prevents `secrets.h` from being committed.

---

## Future Direction

AEVNS is primarily a **learning and experimentation platform**.

The goal is to gradually improve the robot by adding capabilities such as basic visual inspection, better navigation, and simple anomaly detection.

It is not intended to compete with professional inspection systems, but to help me understand how **robotics, sensing, AI, and civil engineering concepts** can be combined in a practical prototype.

---

## Author

**Muhammad Yahya**  
Civil Engineering Student at GIKI

**Portfolio:** [itsyahya.com](https://itsyahya.com)  
**GitHub:** [m-yahya2006](https://github.com/m-yahya2006)

---

*AEVNS is an ongoing undergraduate engineering project and will evolve as new hardware, software, and machine-learning components are developed and tested.*