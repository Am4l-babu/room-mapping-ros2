# 🔐 Security & Configuration Management Guide

This guide explains the template-based configuration system for the WiFi Robot Car project. It ensures that sensitive credentials (WiFi passwords, API keys, etc.) are never accidentally committed to version control.

## 📋 Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Architecture](#architecture)
4. [Configuration Files](#configuration-files)
5. [Best Practices](#best-practices)
6. [Troubleshooting](#troubleshooting)
7. [Team Collaboration](#team-collaboration)

---

## Overview

The project uses a **template-based configuration system** where:

- **Templates** (`config_template.*`) - Public files with placeholder values, committed to git
- **Configs** (`config.*`) - Private files with actual credentials, excluded from git (`.gitignore`)

### Why This Matters

```
❌ BAD: Hardcoded credentials in source code
   - Visible in git history forever
   - Exposed if repository becomes public
   - Risk of accidental commits
   - Difficult to manage across team

✅ GOOD: Template-based configuration
   - Credentials only on local machines
   - Cannot be accidentally committed
   - Easy for team to manage
   - Professional security practice
```

---

## Quick Start

### 1️⃣ Initial Setup

```bash
# Navigate to project root
cd /home/ros/ros2_ws

# Run setup script (creates config files from templates)
./setup_config.sh
```

This will:
- ✅ Copy `config_template.h` → `config.h` (C++)
- ✅ Copy `config_template.py` → `config.py` (Python)
- ✅ Verify `.gitignore` configuration

### 2️⃣ Edit Configuration Files

#### For C++ (ESP32 firmware):
```bash
nano sensor_testing/include/config.h
```

Replace these placeholders:
```cpp
const char* WIFI_SSID = "YOUR_WIFI_NETWORK_HERE";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE";
```

#### For Python (Web Control):
```bash
nano robot_web_control/config.py
```

Replace these placeholders:
```python
WIFI_SSID = "YOUR_WIFI_NETWORK_HERE"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE"
SECRET_KEY = "your-secret-key-here"
```

### 3️⃣ Verify Configuration

```bash
# Test C++ build
cd sensor_testing
pio run -e wifi_car

# Test Python import
cd robot_web_control
python3 -c "from config import WIFI_SSID, WIFI_PASSWORD; print(f'Config loaded: {WIFI_SSID}')"
```

---

## Architecture

### File Structure

```
/home/ros/ros2_ws/
├── .gitignore                                    ← Excludes all config.* files
│
├── sensor_testing/                              ← C++ / ESP32 Firmware
│   ├── include/
│   │   ├── config_template.h                    ✅ Committed (public)
│   │   ├── config.h                             ❌ .gitignored (private)
│   │   ├── motor_control.h                      ✅ Committed
│   │   ├── wifi_handler.h                       ✅ Committed
│   │   ├── sensor_handler.h                     ✅ Committed
│   │   └── web_server.h                         ✅ Committed
│   └── src/
│       └── wifi_car.cpp                         ✅ Committed (uses config.h)
│
├── robot_web_control/                           ← Python / Flask Web UI
│   ├── config_template.py                       ✅ Committed (public)
│   ├── config.py                                ❌ .gitignored (private)
│   ├── app.py                                   ✅ Committed (uses config.py)
│   ├── requirements.txt
│   ├── templates/
│   └── static/
│
├── TEMPLATE_SETUP_GUIDE.md                      ✅ Comprehensive documentation
├── SECURITY_CONFIG_GUIDE.md                     ✅ This file
└── setup_config.sh                              ✅ Setup helper script
```

### Git Ignore Configuration

File: `.gitignore`

```ini
# ============================================================================
# CREDENTIALS & SENSITIVE DATA - NEVER COMMIT THESE!
# ============================================================================
sensor_testing/include/config.h
robot_web_control/config.py
.env
.env.local
.env.*.local
*.secret
*credentials*
```

---

## Configuration Files

### C++ Configuration: `config.h`

**Location:** `sensor_testing/include/config.h`

**Purpose:** Provides all configuration constants for ESP32 firmware

**Key Settings:**

| Setting | Purpose | Example |
|---------|---------|---------|
| `WIFI_SSID` | WiFi network name | `"Keralavision@1994"` |
| `WIFI_PASSWORD` | WiFi network password | `"babu7362"` |
| `DEFAULT_MOTOR_SPEED` | Motor speed (0-255) | `150` |
| `SERVO_LEFT_ANGLE` | Servo left position (°) | `150` |
| `SERVO_CENTER_ANGLE` | Servo center position (°) | `90` |
| `SERVO_RIGHT_ANGLE` | Servo right position (°) | `30` |

**Usage in Code:**

```cpp
#include "config.h"  // At the top of wifi_car.cpp

void setup() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    analogWrite(PWM_PIN, DEFAULT_MOTOR_SPEED);
    servo_motor.write(SERVO_CENTER_ANGLE);
}
```

### Python Configuration: `config.py`

**Location:** `robot_web_control/config.py`

**Purpose:** Provides all configuration constants for Flask web application

**Key Settings:**

| Setting | Purpose | Example |
|---------|---------|---------|
| `WIFI_SSID` | WiFi network name | `"Keralavision@1994"` |
| `WIFI_PASSWORD` | WiFi network password | `"babu7362"` |
| `SERIAL_PORT` | ESP32 serial port | `"/dev/ttyACM0"` |
| `BAUD_RATE` | Serial communication speed | `115200` |
| `SECRET_KEY` | Flask session key | Random string |
| `FLASK_PORT` | Web server port | `5000` |

**Usage in Code:**

```python
from config import WIFI_SSID, WIFI_PASSWORD, FLASK_PORT

app.config['SECRET_KEY'] = SECRET_KEY

@app.route('/status')
def status():
    return jsonify({
        'robot_id': DEVICE_ID,
        'robot_name': DEVICE_NAME
    })
```

---

## Best Practices

### DO ✅

- ✅ **Keep config files local** - Create them on each machine, never commit them
- ✅ **Use strong passwords** - At least 12 characters, mixed case, numbers, symbols
- ✅ **Share templates only** - Commit `config_template.*`, not actual configs
- ✅ **Document defaults** - Keep template files well-documented
- ✅ **Review .gitignore regularly** - Ensure all credential files are excluded
- ✅ **Use environment variables for CI/CD** - For automated deployments
- ✅ **Update templates for future needs** - Add new sections before developers need them
- ✅ **Backup your configs** - Keep local copies, they're not version controlled
- ✅ **Rotate passwords** - Change WiFi passwords if compromised
- ✅ **Separate concerns** - Different configs for development, staging, production

### DON'T ❌

- ❌ **Never commit config.h or config.py** - Always exclude them from git
- ❌ **Never hardcode credentials** - Always use config files or environment variables
- ❌ **Don't store passwords in comments** - Even commented code is visible in history
- ❌ **Don't share actual config files** - Only share templates
- ❌ **Don't use weak passwords** - Avoid "password123" or "12345678"
- ❌ **Don't mix configs** - Keep development and production separate
- ❌ **Don't commit secrets to branches** - Even temporary branches
- ❌ **Don't log sensitive data** - Never print credentials to console/logs
- ❌ **Don't use production credentials in development** - Use test values locally
- ❌ **Don't forget to .gitignore new config files** - Update .gitignore before creating them

---

## Security Checklist

Before deploying or committing:

- [ ] `config.h` is NOT in git (check `.gitignore`)
- [ ] `config.py` is NOT in git (check `.gitignore`)
- [ ] `config_template.h` HAS example/placeholder values
- [ ] `config_template.py` HAS example/placeholder values
- [ ] No credentials appear in actual source code (not even old versions)
- [ ] WiFi password is strong (12+ characters)
- [ ] SSH keys are not in any repository
- [ ] API keys are only in config files
- [ ] `.env` files are in .gitignore if used
- [ ] Team members have instructions to copy templates before editing

---

## Team Collaboration

### For a New Team Member

1. **Clone the repository:**
   ```bash
   git clone https://github.com/your-org/robot-car.git
   cd robot-car
   ```

2. **Run setup script:**
   ```bash
   ./setup_config.sh
   ```

3. **Edit configuration files with actual values:**
   ```bash
   nano sensor_testing/include/config.h
   nano robot_web_control/config.py
   ```

4. **Build and test:**
   ```bash
   cd sensor_testing && pio run -e wifi_car
   cd robot_web_control && python3 app.py
   ```

### For Repository Maintainers

**When adding new configuration sections:**

1. Add to `config_template.*` with placeholder values
2. Document the setting with comments
3. Add to this guide
4. Do NOT commit actual `config.*` files
5. Notify team about new configuration options

**When credentials are exposed:**

1. ⚠️ **IMMEDIATELY** revoke the exposed credentials
2. Create new, unique credentials
3. Update your local `config.*` file
4. Commit a message indicating the change
5. Notify affected team members

---

## Troubleshooting

### Problem: "ImportError: No module named config"

**Cause:** `config.py` doesn't exist (not created from template)

**Solution:**
```bash
cd robot_web_control
cp config_template.py config.py
nano config.py  # Edit with actual values
```

### Problem: "fatal: config.h: Permission denied"

**Cause:** File exists but wrong permissions

**Solution:**
```bash
rm sensor_testing/include/config.h
cp sensor_testing/include/config_template.h sensor_testing/include/config.h
chmod 600 sensor_testing/include/config.h  # Read/write owner only
```

### Problem: WiFi connection fails in firmware

**Cause:** WIFI_SSID or WIFI_PASSWORD incorrect in `config.h`

**Solution:**
1. Verify values match your actual WiFi network
2. Check for typos (especially quotes and special characters)
3. Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
4. Restart ESP32 after updating config
5. Check `config.h` was recompiled: `pio run -e wifi_car --verbose`

### Problem: Configuration changes don't take effect

**Cause:** Code not recompiled after config changes

**Solution:**
```bash
# Force rebuild (don't use cached files)
pio run -e wifi_car --verbose --target clean
pio run -e wifi_car
```

### Problem: Git keeps tracking config.h / config.py

**Cause:** Files were committed before being added to .gitignore

**Solution:**
```bash
# Remove from git (but keep local files)
git rm --cached sensor_testing/include/config.h
git rm --cached robot_web_control/config.py
git commit -m "Stop tracking config files"
git push
```

---

## Future Extensions

The configuration system is designed to expand easily:

### Adding MQTT Support

In `config_template.h`:
```cpp
// MQTT Configuration
#define MQTT_ENABLED true
#define MQTT_BROKER "mqtt.example.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "robot_user"
#define MQTT_PASSWORD "CHANGE_ME_MQTT_PASSWORD"
```

### Adding Cloud API

In `config_template.py`:
```python
API_KEY = "CHANGE_ME_API_KEY"
CLOUD_ENDPOINT = "https://api.example.com"
CLOUD_ENABLED = False
```

---

## Related Documentation

- 📖 [TEMPLATE_SETUP_GUIDE.md](TEMPLATE_SETUP_GUIDE.md) - Detailed template architecture
- 📖 [QUICK_START_GUIDE.md](QUICK_START_GUIDE.md) - Project quick start
- 📖 [README.md](README.md) - Main project overview

---

## Questions or Issues?

If you have questions about the configuration system:

1. Check this guide and [TEMPLATE_SETUP_GUIDE.md](TEMPLATE_SETUP_GUIDE.md)
2. Review existing `config_template.*` files for examples
3. Ask the team lead or project maintainer
4. Create an issue on GitHub (without including actual credentials!)

---

**Last Updated:** 2024-03-29  
**Version:** 1.0  
**Status:** ✅ Production Ready
