"""
config_template.py - Configuration template for Robot Web Control

INSTRUCTIONS:
1. Copy this file to: config.py
   $ cp config_template.py config.py

2. Edit config.py and replace placeholders with your actual values

3. NEVER commit config.py to git - it contains secrets!
   (Already configured in .gitignore)

4. Share config_template.py instead - others will create their own config.py
"""

# ============================================================================
# WIFI CONFIGURATION
# ============================================================================
# WiFi network to control ESP32 robot
WIFI_SSID = "CHANGE_ME_WIFI_NAME"
WIFI_PASSWORD = "CHANGE_ME_WIFI_PASSWORD"

# ============================================================================
# ESP32 CONFIGURATION
# ============================================================================
# IP address or hostname of the robot (will be discovered automatically on same network)
ESP32_HOST = "192.168.1.100"  # Or use mDNS: "esp32.local"
ESP32_PORT = 80

# Device ID for multi-robot scenarios
DEVICE_ID = "ESP32_ROBOT_001"
DEVICE_NAME = "WiFi Robot Car"

# ============================================================================
# WEB SERVER CONFIGURATION (Flask)
# ============================================================================
FLASK_DEBUG = True   # Set to False in production
FLASK_HOST = "0.0.0.0"
FLASK_PORT = 5000

# Session configuration
SECRET_KEY = "CHANGE_ME_SECRET_KEY_USE_STRONG_RANDOM_STRING"

# ============================================================================
# ROBOT CONTROL CONFIGURATION
# ============================================================================
# Motor speed (0-255)
DEFAULT_MOTOR_SPEED = 150

# Obstacle avoidance thresholds (mm)
OBSTACLE_THRESHOLD = 200
SAFEGUARD_MARGIN = 50

# Scan timing (ms)
SCAN_DWELL_TIME = 300
TURN_DURATION = 800

# ============================================================================
# LOGGING CONFIGURATION
# ============================================================================
LOG_LEVEL = "INFO"  # DEBUG, INFO, WARNING, ERROR
LOG_FILE = "robot_control.log"

# ============================================================================
# FUTURE EXTENSIONS
# ============================================================================

# MQTT Configuration (for future integration)
MQTT_ENABLED = False
# MQTT_BROKER = "mqtt.example.com"
# MQTT_PORT = 1883
# MQTT_USERNAME = "robot_user"
# MQTT_PASSWORD = "CHANGE_ME_MQTT_PASSWORD"

# API Keys (for future features)
# API_KEY = "CHANGE_ME_API_KEY"
# CLOUD_API_ENDPOINT = "https://api.example.com"

# ============================================================================
# SECURITY BEST PRACTICES
# ============================================================================
"""
CHECK BEFORE DEPLOYMENT:

✓ All WIFI_* credentials are set to actual values (not CHANGE_ME_*)
✓ config.py is NOT tracking in git (.gitignore should exclude it)
✓ Only config_template.py is in version control
✓ Never commit credentials to any branch
✓ Use unique, strong passwords
✓ Change SECRET_KEY to a random string

MULTI-DEVICE SETUP:
Each ESP32 robot should:
- Connect to the same WiFi network
- Have its own unique DEVICE_ID
- (Optional) Have its own unique IP if on separate networks
"""
