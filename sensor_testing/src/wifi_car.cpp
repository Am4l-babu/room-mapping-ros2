#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <ESP32Servo.h>

// ============================================================================
// CONFIGURATION - IMPORT FROM config.h (DO NOT EDIT HERE!)
// ============================================================================
// All credentials and settings are in: include/config.h
// Template version: include/config_template.h
#include "config.h"

// ============= WEB SERVER CONFIGURATION =============
WebServer server(80);

// ============================================================================
// MOTOR CONTROL CONFIGURATION
// ============================================================================
// All settings imported from config.h:
// - MOTOR_PWM_FREQUENCY
// - MOTOR_PWM_RESOLUTION
// - DEFAULT_MOTOR_SPEED

// Motor pins are constant and defined here (hardware-specific)
#define MOTOR1_IN1 14
#define MOTOR1_IN2 27
#define MOTOR1_ENA 25

#define MOTOR2_IN3 26
#define MOTOR2_IN4 33
#define MOTOR2_ENB 32

#define LEDC_CHANNEL_M1 0
#define LEDC_CHANNEL_M2 1

// ============================================================================
// SERVO CONFIGURATION
// ============================================================================
// Servo angles imported from config.h:
// - SERVO_LEFT_ANGLE = 150
// - SERVO_CENTER_ANGLE = 90
// - SERVO_RIGHT_ANGLE = 30

#define SERVO_PIN 13

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
// VL53L0X I2C pins from config.h:
// - VL53L0X_SDA = GPIO 21
// - VL53L0X_SCL = GPIO 22

#define VL53L0X_SDA 21
#define VL53L0X_SCL 22
#define VL53L0X_ADDRESS 0x29

VL53L0X sensor;
Servo servo_motor;
uint16_t front_distance = 5000;

// ============= STATE MACHINE STATES =============
enum OperationMode { MANUAL = 0, AUTONOMOUS = 1 };
enum AutoState { FORWARD_STATE = 0, STOP_STATE = 1, SCAN_STATE = 2, DECIDE_STATE = 3, TURN_STATE = 4, IDLE_STATE = 5 };

// ============= GLOBAL STATE VARIABLES =============
OperationMode current_mode = MANUAL;
AutoState current_auto_state = IDLE_STATE;
uint16_t left_distance = 5000;
uint16_t center_distance = 5000;
uint16_t right_distance = 5000;
int current_speed = DEFAULT_MOTOR_SPEED;  // From config.h
int current_servo_angle = SERVO_CENTER_ANGLE;  // From config.h
String current_action = "Idle";
bool wifi_connected = false;

// ============= TIMING VARIABLES =============
unsigned long last_distance_read = 0;
unsigned long last_servo_move = 0;
unsigned long scan_enter_time = 0;
unsigned long distance_read_interval = SENSOR_READ_INTERVAL_MS;  // From config.h
const unsigned long SCAN_DWELL_TIME = SCAN_DWELL_TIME_MS;        // From config.h

// ============= FUNCTION PROTOTYPES =============
void setup_motor_pins();
void setup_pwm_channels();
void setup_servo();
void setup_sensor();
void setup_wifi();
void setup_webserver();
void moveForward(int speed);
void moveBackward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopMotors();
void servo_to_angle(int angle);
void update_distance();
void handle_web_command(String cmd);
void state_machine_autonomous();

// ============= HTML PAGE =============
const char* HTML_PAGE = R"___HTML___(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Robot Car</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 600px;
            width: 100%;
        }
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
            font-size: 2rem;
        }
        .status-board {
            background: #f0f0f0;
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 30px;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        .status-item {
            background: white;
            padding: 15px;
            border-radius: 10px;
            border-left: 4px solid #667eea;
        }
        .status-label {
            font-size: 0.85rem;
            color: #999;
            text-transform: uppercase;
            margin-bottom: 5px;
        }
        .status-value {
            font-size: 1.3rem;
            font-weight: bold;
            color: #333;
        }
        .distance-status {
            grid-column: 1 / -1;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-left-color: white;
        }
        .distance-status .status-value { color: white; font-size: 2rem; }
        .mode-selector {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-bottom: 30px;
        }
        .mode-btn {
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 10px;
            background: white;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.3s;
        }
        .mode-btn.active {
            background: #667eea;
            color: white;
            border-color: #667eea;
        }
        .speed-control {
            margin-bottom: 30px;
        }
        .speed-label {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
            font-weight: bold;
        }
        .speed-slider {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #ddd;
            outline: none;
            -webkit-appearance: none;
        }
        .speed-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        .control-pad {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 10px;
            margin-bottom: 30px;
        }
        .control-btn {
            aspect-ratio: 1;
            border: none;
            border-radius: 10px;
            font-weight: bold;
            cursor: pointer;
            font-size: 1.1rem;
            transition: all 0.2s;
            background: #667eea;
            color: white;
        }
        .control-btn:hover { transform: scale(1.05); }
        .control-btn:active { transform: scale(0.95); }
        .forward { grid-column: 2; }
        .left { grid-column: 1; }
        .stop { grid-column: 2; background: #ff6b6b; }
        .right { grid-column: 3; }
        .backward { grid-column: 2; }
        .info-box {
            background: #f0f0f0;
            border-radius: 10px;
            padding: 15px;
            font-size: 0.9rem;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Robot Car Control</h1>
        
        <div class="status-board">
            <div class="status-item">
                <div class="status-label">Mode</div>
                <div class="status-value" id="mode-display">Manual</div>
            </div>
            <div class="status-item">
                <div class="status-label">Action</div>
                <div class="status-value" id="action-display">Idle</div>
            </div>
            <div class="status-item distance-status">
                <div class="status-label">Distance</div>
                <div class="status-value" id="distance-display">---</div>
            </div>
        </div>
        
        <div class="mode-selector">
            <button class="mode-btn active" id="mode-manual" onclick="setMode('manual')">Manual</button>
            <button class="mode-btn" id="mode-auto" onclick="setMode('auto')">Autonomous</button>
        </div>
        
        <div class="speed-control">
            <div class="speed-label">
                <span>Speed</span>
                <span id="speed-display">150</span>
            </div>
            <input type="range" class="speed-slider" id="speed-slider" min="0" max="255" value="150" oninput="document.getElementById('speed-display').textContent = this.value;">
        </div>
        
        <div class="control-pad">
            <button class="control-btn forward" onmousedown="sendCmd('FORWARD')" onmouseup="sendCmd('STOP')" ontouchstart="sendCmd('FORWARD')" ontouchend="sendCmd('STOP')">FWD</button>
            <button class="control-btn left" onmousedown="sendCmd('LEFT')" onmouseup="sendCmd('STOP')" ontouchstart="sendCmd('LEFT')" ontouchend="sendCmd('STOP')">LEFT</button>
            <button class="control-btn stop" onclick="sendCmd('STOP')">STOP</button>
            <button class="control-btn right" onmousedown="sendCmd('RIGHT')" onmouseup="sendCmd('STOP')" ontouchstart="sendCmd('RIGHT')" ontouchend="sendCmd('STOP')">RIGHT</button>
            <button class="control-btn backward" onmousedown="sendCmd('BACKWARD')" onmouseup="sendCmd('STOP')" ontouchstart="sendCmd('BACKWARD')" ontouchend="sendCmd('STOP')">BACK</button>
        </div>
        
        <div class="info-box">
            <strong>Controls:</strong><br>
            Manual: Use buttons to drive<br>
            Autonomous: Robot scans and avoids obstacles<br>
            Adjust speed before movement
        </div>
    </div>
    
    <script>
        function setMode(mode) {
            const cmd = mode === 'manual' ? 'MODE_MANUAL' : 'MODE_AUTO';
            sendCmd(cmd);
            
            const manualBtn = document.getElementById('mode-manual');
            const autoBtn = document.getElementById('mode-auto');
            
            if (mode === 'manual') {
                manualBtn.classList.add('active');
                autoBtn.classList.remove('active');
            } else {
                autoBtn.classList.add('active');
                manualBtn.classList.remove('active');
            }
        }
        
        function sendCmd(cmd) {
            const speed = document.getElementById('speed-slider').value;
            fetch('/api/cmd?cmd=' + cmd + '&speed=' + speed)
                .then(response => response.json())
                .then(data => updateDisplay(data))
                .catch(err => console.error(err));
        }
        
        function updateDisplay(data) {
            document.getElementById('mode-display').textContent = data.mode === 0 ? 'Manual' : 'Autonomous';
            document.getElementById('action-display').textContent = data.action;
            document.getElementById('distance-display').textContent = data.distance + ' mm';
        }
        
        // Auto update status
        setInterval(() => {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => updateDisplay(data))
                .catch(err => console.error(err));
        }, 500);
        
        // Keyboard controls
        document.addEventListener('keydown', (e) => {
            if (e.key === 'w') sendCmd('FORWARD');
            else if (e.key === 'a') sendCmd('LEFT');
            else if (e.key === 'd') sendCmd('RIGHT');
            else if (e.key === 's') sendCmd('STOP');
            else if (e.key === 'x') sendCmd('BACKWARD');
        });
        
        document.addEventListener('keyup', (e) => {
            if (['w', 'a', 'd', 'x'].includes(e.key)) sendCmd('STOP');
        });
    </script>
</body>
</html>
)___HTML___";

// ============= SETUP AND LOOP =============

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========== ESP32 WIFI 2WD ROBOT CAR ==========");
  Serial.println("Version: 1.0 | WebServer Mode");
  Serial.println("==========================================\n");
  
  setup_motor_pins();
  setup_pwm_channels();
  setup_servo();
  setup_sensor();
  setup_wifi();
  setup_webserver();
  
  Serial.println("[INIT] System ready!\n");
}

void loop() {
  server.handleClient();
  update_distance();
  
  if (current_mode == AUTONOMOUS) {
    state_machine_autonomous();
  }
}

// ============= MOTOR CONTROL =============

void setup_motor_pins() {
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR2_IN3, OUTPUT);
  pinMode(MOTOR2_IN4, OUTPUT);
  
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, LOW);
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, LOW);
  
  Serial.println("[INIT] Motor pins configured");
}

void setup_pwm_channels() {
  ledcSetup(LEDC_CHANNEL_M1, MOTOR_PWM_FREQUENCY, MOTOR_PWM_RESOLUTION);
  ledcAttachPin(MOTOR1_ENA, LEDC_CHANNEL_M1);
  
  ledcSetup(LEDC_CHANNEL_M2, MOTOR_PWM_FREQUENCY, MOTOR_PWM_RESOLUTION);
  ledcAttachPin(MOTOR2_ENB, LEDC_CHANNEL_M2);
  
  ledcWrite(LEDC_CHANNEL_M1, 0);
  ledcWrite(LEDC_CHANNEL_M2, 0);
  
  Serial.println("[INIT] PWM channels configured");
}

void moveForward(int speed) {
  speed = constrain(speed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, HIGH);
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, HIGH);
  ledcWrite(LEDC_CHANNEL_M1, speed);
  ledcWrite(LEDC_CHANNEL_M2, speed);
  current_speed = speed;
  current_action = "Moving Forward";
  Serial.println("[MOTOR] FWD speed=" + String(speed));
}

void moveBackward(int speed) {
  speed = constrain(speed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);
  digitalWrite(MOTOR2_IN3, HIGH);
  digitalWrite(MOTOR2_IN4, LOW);
  ledcWrite(LEDC_CHANNEL_M1, speed);
  ledcWrite(LEDC_CHANNEL_M2, speed);
  current_speed = speed;
  current_action = "Moving Backward";
  Serial.println("[MOTOR] BCK speed=" + String(speed));
}

void turnLeft(int speed) {
  speed = constrain(speed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, LOW);
  ledcWrite(LEDC_CHANNEL_M1, 0);
  
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, HIGH);
  ledcWrite(LEDC_CHANNEL_M2, speed);
  
  current_speed = speed;
  current_action = "Turning Left";
  Serial.println("[MOTOR] LEFT speed=" + String(speed));
}

void turnRight(int speed) {
  speed = constrain(speed, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, HIGH);
  ledcWrite(LEDC_CHANNEL_M1, speed);
  
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, LOW);
  ledcWrite(LEDC_CHANNEL_M2, 0);
  
  current_speed = speed;
  current_action = "Turning Right";
  Serial.println("[MOTOR] RIGHT speed=" + String(speed));
}

void stopMotors() {
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, LOW);
  digitalWrite(MOTOR2_IN3, LOW);
  digitalWrite(MOTOR2_IN4, LOW);
  
  ledcWrite(LEDC_CHANNEL_M1, 0);
  ledcWrite(LEDC_CHANNEL_M2, 0);
  
  current_speed = 0;
  current_action = "Stopped";
  Serial.println("[MOTOR] STOP");
}

// ============= SERVO CONTROL =============

void setup_servo() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo_motor.setPeriodHertz(50);
  servo_motor.attach(SERVO_PIN, 1000, 2000);
  servo_motor.write(SERVO_CENTER_ANGLE);
  current_servo_angle = SERVO_CENTER_ANGLE;
  Serial.println("[INIT] Servo initialized");
}

void servo_to_angle(int angle) {
  angle = constrain(angle, SERVO_RIGHT_ANGLE, 180);
  servo_motor.write(angle);
  current_servo_angle = angle;
  last_servo_move = millis();
}

// ============= SENSOR CONTROL =============

void setup_sensor() {
  Wire.begin(VL53L0X_SDA, VL53L0X_SCL);
  sensor.init();
  sensor.setTimeout(500);
  
  if (!sensor.timeoutOccurred()) {
    Serial.println("[INIT] VL53L0X initialized");
  } else {
    Serial.println("[INIT] VL53L0X timeout!");
  }
}

void update_distance() {
  if (millis() - last_distance_read < distance_read_interval) {
    return;
  }
  
  front_distance = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred()) {
    front_distance = 5000;
  }
  if (front_distance > SENSOR_MAX_RANGE_MM) {
      front_distance = SENSOR_MAX_RANGE_MM;
  }
  
  last_distance_read = millis();
}

// ============= STATE MACHINE =============

void state_machine_autonomous() {
  static unsigned long state_enter_time = 0;
  static bool state_changed = true;
  
  if (state_changed) {
    state_enter_time = millis();
    state_changed = false;
  }
  
  switch (current_auto_state) {
    case FORWARD_STATE: {
      if (state_changed || current_action == "Idle") {
        moveForward(current_speed);
        state_changed = false;
      }
      
      if (front_distance < 200) {
        Serial.println("[AUTO] Obstacle detected!");
        stopMotors();
        current_auto_state = STOP_STATE;
        state_changed = true;
      }
      break;
    }
    
    case STOP_STATE: {
      if (state_changed) {
        stopMotors();
        state_enter_time = millis();
        state_changed = false;
      }
      
      if (millis() - state_enter_time > 500) {
        Serial.println("[AUTO] Starting scan...");
        current_auto_state = SCAN_STATE;
        scan_enter_time = millis();
        servo_to_angle(SERVO_LEFT_ANGLE);
        state_changed = true;
      }
      break;
    }
    
    case SCAN_STATE: {
      unsigned long elapsed = millis() - scan_enter_time;
      
      if (elapsed < SCAN_DWELL_TIME_MS) {
        servo_to_angle(SERVO_LEFT_ANGLE);
        left_distance = front_distance;
      } else if (elapsed < 2 * SCAN_DWELL_TIME_MS) {
        servo_to_angle(SERVO_CENTER_ANGLE);
        center_distance = front_distance;
      } else if (elapsed < 3 * SCAN_DWELL_TIME_MS) {
        servo_to_angle(SERVO_RIGHT_ANGLE);
        right_distance = front_distance;
      } else {
        Serial.println("[AUTO] Scan complete - L:" + String(left_distance) + " C:" + String(center_distance) + " R:" + String(right_distance));
        current_auto_state = DECIDE_STATE;
        state_changed = true;
      }
      break;
    }
    
    case DECIDE_STATE: {
      if (state_changed) {
        uint16_t max_dist = max({left_distance, center_distance, right_distance});
        
        if (max_dist < OBSTACLE_THRESHOLD_MM) {
          Serial.println("[AUTO] No safe path, backing up");
          moveBackward(current_speed);
          current_auto_state = TURN_STATE;
        } else if (center_distance >= max_dist - SAFEGUARD_MARGIN_MM) {
          Serial.println("[AUTO] Center clear, going forward");
          current_auto_state = FORWARD_STATE;
          servo_to_angle(SERVO_CENTER_ANGLE);
        } else if (left_distance > right_distance) {
          Serial.println("[AUTO] Turning left");
          turnLeft(current_speed);
          current_auto_state = TURN_STATE;
        } else {
          Serial.println("[AUTO] Turning right");
          turnRight(current_speed);
          current_auto_state = TURN_STATE;
        }
        state_changed = true;
      }
      break;
    }
    
    case TURN_STATE: {
      if (state_changed) {
        state_enter_time = millis();
        state_changed = false;
      }
      
      if (millis() - state_enter_time > TURN_DURATION_MS) {
        servo_to_angle(SERVO_CENTER_ANGLE);
        current_auto_state = FORWARD_STATE;
        state_changed = true;
      }
      break;
    }
    
    default:
      stopMotors();
      current_action = "Idle";
  }
}

// ============= WIFI & WEB SERVER =============

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println("\n[WIFI] Connected!");
    Serial.println("[WIFI] IP: " + WiFi.localIP().toString());
  } else {
    wifi_connected = false;
    Serial.println("\n[WIFI] Connection failed");
  }
}

void setup_webserver() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", HTML_PAGE);
  });
  
  server.on("/api/cmd", HTTP_GET, []() {
    String cmd = server.arg("cmd");
    String speed_str = server.arg("speed");
    
    if (speed_str.length() > 0) {
      int sp = speed_str.toInt();
      current_speed = constrain(sp, MIN_MOTOR_SPEED, MAX_MOTOR_SPEED);
    }
    
    handle_web_command(cmd);
    
    String json = "{\"mode\": " + String(current_mode) + 
                  ", \"action\": \"" + current_action + 
                  "\", \"distance\": " + String(front_distance) + "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/api/status", HTTP_GET, []() {
    String json = "{\"mode\": " + String(current_mode) + 
                  ", \"action\": \"" + current_action + 
                  "\", \"distance\": " + String(front_distance) + "}";
    server.send(200, "application/json", json);
  });
  
  server.begin();
  Serial.println("[WEB] Server started at http://" + WiFi.localIP().toString());
}

void handle_web_command(String cmd) {
  cmd.toLowerCase();
  
  if (cmd == "forward") {
    if (current_mode == MANUAL) moveForward(current_speed);
  } else if (cmd == "backward") {
    if (current_mode == MANUAL) moveBackward(current_speed);
  } else if (cmd == "left") {
    if (current_mode == MANUAL) turnLeft(current_speed);
  } else if (cmd == "right") {
    if (current_mode == MANUAL) turnRight(current_speed);
  } else if (cmd == "stop") {
    stopMotors();
  } else if (cmd == "mode_manual") {
    current_mode = MANUAL;
    stopMotors();
    current_auto_state = IDLE_STATE;
    Serial.println("[WEB] Mode: MANUAL");
  } else if (cmd == "mode_auto") {
    current_mode = AUTONOMOUS;
    current_auto_state = FORWARD_STATE;
    Serial.println("[WEB] Mode: AUTONOMOUS");
  }
}
