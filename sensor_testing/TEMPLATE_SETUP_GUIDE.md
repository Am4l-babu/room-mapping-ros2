# Template-Based Configuration Guide for ESP32 Robot Car

## 🔒 Security First: Why Separation Matters

This project uses a **template-based configuration architecture** to protect sensitive data:

- ✅ WiFi credentials stay LOCAL only
- ✅ API keys never committed to git
- ✅ Template shared publicly, actual config private
- ✅ Easy for teams to collaborate safely

---

## 📋 Architecture Overview

```
sensor_testing/
├── include/
│   ├── config_template.h    ← Public template with placeholders
│   ├── config.h             ← NEVER commit! (in .gitignore)
│   ├── motor_control.h      ← Modular components
│   ├── wifi_handler.h
│   ├── sensor_handler.h
│   └── web_server.h
├── src/
│   ├── motor_control.cpp    ← Implementation
│   ├── wifi_handler.cpp
│   ├── sensor_handler.cpp
│   ├── web_server.cpp
│   └── wifi_car.cpp         ← Main file (includes config.h)
└── TEMPLATE_SETUP_GUIDE.md  ← This file
```

---

## 🚀 Initial Setup

### Step 1: Copy Template to Config

```bash
cd /home/ros/ros2_ws/sensor_testing

# Create config.h from template
cp include/config_template.h include/config.h
```

### Step 2: Edit config.h with Your Credentials

```bash
nano include/config.h
```

Replace placeholders:

```cpp
// BEFORE (template):
#define WIFI_SSID "CHANGE_ME_WIFI_NAME"
#define WIFI_PASSWORD "CHANGE_ME_WIFI_PASSWORD"

// AFTER (your actual credentials):
#define WIFI_SSID "Keralavision@1994"
#define WIFI_PASSWORD "babu7362"
```

### Step 3: Verify .gitignore Protects config.h

```bash
# Check if config.h is in .gitignore
grep -n "config.h" /home/ros/ros2_ws/.gitignore

# Output should show:
# sensor_testing/include/config.h
```

### Step 4: Compile & Upload

```bash
cd /home/ros/ros2_ws/sensor_testing

# Build
pio run -e wifi_car

# Upload
pio run -e wifi_car -t upload
```

---

## 🤝 Sharing Project with Team

### For Contributors (Safe Collaboration)

**Share these files publicly:**
- ✅ `config_template.h` - Everyone uses this as reference
- ✅ Source code in `src/`
- ✅ Header files in `include/` (except config.h!)
- ✅ Documentation and guides
- ✅ `.gitignore` - configured to prevent accidents

**Never share:**
- ❌ `config.h` - Local credentials only
- ❌ `.env` files
- ❌ API keys or tokens
- ❌ Database passwords

### Setup Instructions for New Team Members

1. Clone repo normally
2. Copy template: `cp include/config_template.h include/config.h`
3. Edit with their WiFi credentials
4. Compile and test
5. Never commit `config.h`

```bash
# Shortcut for new developers:
git clone <repo>
cp include/config_template.h include/config.h
# Edit config.h with YOUR credentials
pio run -e wifi_car -t upload
```

---

## 🔑 Configuration Structure

### 1. WiFi Credentials

```cpp
// config.h (LOCAL ONLY)
#define WIFI_SSID "Keralavision@1994"
#define WIFI_PASSWORD "babu7362"

// config_template.h (PUBLIC)
#define WIFI_SSID "CHANGE_ME_WIFI_NAME"
#define WIFI_PASSWORD "CHANGE_ME_WIFI_PASSWORD"
```

### 2. Device Identity

```cpp
#define DEVICE_ID "ESP32_ROBOT_001"
#define DEVICE_NAME "WiFi Robot Car"
```

Use for multi-robot scenarios:
- Robot 1: `ESP32_ROBOT_001`
- Robot 2: `ESP32_ROBOT_002`
- Each with unique IP via device ID

### 3. Motor Tuning

```cpp
#define DEFAULT_MOTOR_SPEED 150      // 0-255
#define MOTOR_PWM_FREQUENCY 1000     // Hz
#define MOTOR_PWM_RESOLUTION 8       // bits
```

Adjust per robot if motors have different characteristics.

### 4. Sensor Thresholds

```cpp
#define OBSTACLE_THRESHOLD_MM 200    // Stop distance
#define SAFEGUARD_MARGIN_MM 50       // Center preference
#define SCAN_DWELL_TIME_MS 300       // Time per angle
```

Tune based on environment (narrow hallway vs open space).

### 5. Web Server

```cpp
#define WEB_SERVER_PORT 80
#define STATUS_UPDATE_INTERVAL 500   // ms
```

### 6. Debug Logging

```cpp
#define DEBUG_MODE 1       // 1 = enabled, 0 = disabled
#define DEBUG_MOTOR 1
#define DEBUG_SENSOR 1
#define DEBUG_WIFI 1
#define DEBUG_WEB 1
```

Disable in production to free RAM.

---

## 🔄 Using Modular Headers

### Include in Main File

```cpp
#include "config.h"           // Must be first!
#include "motor_control.h"
#include "wifi_handler.h"
#include "sensor_handler.h"
#include "web_server.h"

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);  // From config.h
  
  motor_init();      // Use from motor_control.h
  wifi_init();       // Use from wifi_handler.h
  sensor_init();     // Use from sensor_handler.h
  web_init();        // Use from web_server.h
}
```

### Access Config Values Anywhere

```cpp
// In any .cpp file:
#include "config.h"

void my_function() {
  int speed = DEFAULT_MOTOR_SPEED;
  Serial.println(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
```

### Benefits

1. **Single source of truth**: Change value in config.h, applies everywhere
2. **Type-safe**: Compile-time constants
3. **No magic strings**: All config centralized
4. **Easy refactoring**: Modular headers prevent coupling

---

## 🛡️ Best Practices

### DO ✅

- ✅ Keep `config.h` in `.gitignore`
- ✅ Update `config_template.h` when adding new config
- ✅ Use `#define` for compile-time constants
- ✅ Document each config value's purpose
- ✅ Provide sensible defaults in template
- ✅ Validate config values on startup

### DON'T ❌

- ❌ Hardcode credentials anywhere else
- ❌ Commit `config.h` to version control
- ❌ Change global configs at runtime (use EEPROM if needed)
- ❌ Store different credentials in multiple files
- ❌ Mix template and actual credentials

---

## 🚀 Future Extensions

### Adding MQTT Support

1. Update `config_template.h`:

```cpp
#define MQTT_ENABLED 1
#define MQTT_BROKER "mqtt.example.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "robot_user"
#define MQTT_PASSWORD "CHANGE_ME_MQTT_PASSWORD"
```

2. Create `mqtt_handler.h` and `mqtt_handler.cpp`
3. User copies template and enables MQTT in their `config.h`

### Adding OTA Updates

```cpp
#define OTA_ENABLED 1
#define OTA_PASSWORD "CHANGE_ME_OTA_PASSWORD"
```

### Adding Cloud Integration

```cpp
#define CLOUD_ENABLED 1
#define CLOUD_API_ENDPOINT "https://api.example.com"
#define CLOUD_API_KEY "CHANGE_ME_CLOUD_API_KEY"
```

---

## 📊 Configuration Locations

| Setting | File | Scope | Notes |
|---------|------|-------|-------|
| WiFi SSID/Pass | config.h | Local only | Never commit |
| Motor speeds | config.h | Local/compile-time | Can tune per robot |
| Sensor thresholds | config.h | Local/compile-time | Environment-dependent |
| API endpoints | config.h | Local only | May vary per deployment |
| Debug logging | config.h | Compile-time | Disable in production |

---

## 🔍 Troubleshooting

### Problem: WiFi not connecting

**Check 1**: Verify `config.h` has correct credentials
```cpp
#define WIFI_SSID "YOUR_ACTUAL_NETWORK_NAME"
#define WIFI_PASSWORD "YOUR_ACTUAL_PASSWORD"
```

**Check 2**: Verify compilation includes config.h
```bash
pio run -e wifi_car -v  # Verbose mode
# Look for: Including 'config.h'
```

**Check 3**: Monitor serial output
```bash
pio device monitor -p /dev/ttyACM1 -b 115200
```

### Problem: Config changes not taking effect

**Solution**: Recompile completely
```bash
pio run -e wifi_car -t clean
pio run -e wifi_car
pio run -e wifi_car -t upload
```

### Problem: Building fails - config.h not found

**Solution**: Create config.h from template
```bash
cp include/config_template.h include/config.h
```

---

## 📝 Maintenance

### Regular Tasks

- [ ] Review `config_template.h` quarterly
- [ ] Update documentation when adding configs
- [ ] Test configurations on test robot
- [ ] Archive old configs with device ID
- [ ] Rotate passwords periodically

### Before Production

- [ ] Verify all CHANGE_ME_* placeholders are replaced
- [ ] Test on target hardware with actual credentials
- [ ] Run through complete functional test
- [ ] Check debug logging is disabled
- [ ] Verify .gitignore prevents config.h commit

---

## 📚 Files Reference

| File | Purpose | Status |
|------|---------|--------|
| config_template.h | Template with placeholders | Public ✅ |
| config.h | Actual credentials | Private (in .gitignore) ✅ |
| motor_control.h | Motor interface | Public ✅ |
| wifi_handler.h | WiFi interface | Public ✅ |
| sensor_handler.h | Sensor interface | Public ✅ |
| web_server.h | Web API interface | Public ✅ |
| .gitignore | Exclusion rules | Public ✅ |

---

## 🎯 Next Steps

1. ✅ Copy `config_template.h` → `config.h`
2. ✅ Edit `config.h` with your credentials
3. ✅ Compile: `pio run -e wifi_car`
4. ✅ Upload: `pio run -e wifi_car -t upload`
5. ✅ Test on hardware
6. ✅ Never commit `config.h`

---

**Created**: 2026-03-29  
**Version**: 1.0  
**Status**: Template Architecture Ready ✅

