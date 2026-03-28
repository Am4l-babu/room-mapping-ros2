/*
 * ESP32 micro-ROS Client over WiFi (UDP)
 * PHASE 1 & 3: Transport + Safety Implementation
 *
 * Publishes:
 *   - /odom (Odometry @ 100 Hz)
 *   - /imu (Imu @ 50 Hz)
 *   - /scan (LaserScan @ 8 Hz)
 *
 * Subscribes:
 *   - /cmd_vel (Twist) with 500 ms watchdog timeout
 *
 * Hardware:
 *   - Motor control: GPIO 14, 25, 27 (M1); GPIO 26, 32, 33 (M2)
 *   - Encoders: GPIO 4 (M1), GPIO 5 (M2)
 *   - IMU (MPU6050): GPIO 21 (SDA), GPIO 22 (SCL)
 *   - Distance (VL53L0X): GPIO 21 (SDA), GPIO 22 (SCL)
 *   - Servo (sweep): GPIO 13
 */

#include <Arduino.h>
#include <WiFi.h>
#include <rcl/rcl.h>
#include <rcl/publisher.h>
#include <rcl/subscription.h>
#include <rcl/executor.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>
#include <sensor_msgs/msg/imu.h>
#include <sensor_msgs/msg/laser_scan.h>
#include <math.h>
#include <Adafruit_MPU6050.h>
#include <VL53L0X.h>

#include "motors_encoders.h"  // Motor control library

// ============= WIFI CONFIGURATION =============
// EDIT THESE WITH YOUR NETWORK CREDENTIALS
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* AGENT_IP = "192.168.1.100";  // Laptop running micro-ROS agent
const uint16_t AGENT_PORT = 8888;

// ============= MICRO-ROS SETUP =============
rcl_context_t context;
rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;

// Publishers
rcl_publisher_t odom_pub, imu_pub, scan_pub;

// Subscribers
rcl_subscription_t cmd_vel_sub;

// Messages
nav_msgs__msg__Odometry odom_msg;
sensor_msgs__msg__Imu imu_msg;
sensor_msgs__msg__LaserScan scan_msg;
geometry_msgs__msg__Twist cmd_vel_msg;

// ============= HARDWARE SENSORS =============
Adafruit_MPU6050 mpu;
VL53L0X distance_sensor;
volatile long encoder_count_m1 = 0, encoder_count_m2 = 0;
volatile long last_encoder_m1 = 0, last_encoder_m2 = 0;

// ============= CONTROL STATE =============
float current_linear_x = 0.0;
float current_angular_z = 0.0;
unsigned long last_cmd_vel_time = 0;
const unsigned long CMD_VEL_TIMEOUT_MS = 500;  // Safety timeout

// ============= ODOMETRY STATE =============
float odom_x = 0.0, odom_y = 0.0, odom_theta = 0.0;

// ============= CONSTANTS =============
const float WHEEL_DIAMETER = 0.065;       // 65 mm
const float WHEEL_SEPARATION = 0.18;      // 180 mm
const int ENCODER_SLOTS = 20;
const float METERS_PER_SLOT = (M_PI * WHEEL_DIAMETER) / ENCODER_SLOTS;
const float MAX_SPEED = 0.5;              // m/s
const float MAX_ANGULAR = 2.0;            // rad/s

// ============= TIMING =============
unsigned long odometry_timer = 0;
unsigned long imu_timer = 0;
unsigned long scan_timer = 0;
const unsigned long ODOM_INTERVAL_MS = 10;   // 100 Hz
const unsigned long IMU_INTERVAL_MS = 20;    // 50 Hz
const unsigned long SCAN_INTERVAL_MS = 125;  // 8 Hz

// ============= SERVO CONTROL FOR LIDAR SWEEP =============
const int SERVO_PIN = 13;
int servo_angle = 0;

// ============= PID GAINS =============
const float PID_KP = 1.2;
const float PID_KI = 0.3;
const float PID_KD = 0.1;

// ============= ENCODER INTERRUPT HANDLERS =============
void IRAM_ATTR encoder_m1_isr() {
  encoder_count_m1++;
}

void IRAM_ATTR encoder_m2_isr() {
  encoder_count_m2++;
}

// ============= CMD_VEL SUBSCRIPTION CALLBACK =============
void cmd_vel_callback(const void* msgin) {
  const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msgin;
  
  // Update timestamp (safety watchdog)
  last_cmd_vel_time = millis();
  
  // Extract linear and angular velocities
  current_linear_x = msg->linear.x;
  current_angular_z = msg->angular.z;
  
  // Clamp to limits
  current_linear_x = constrain(current_linear_x, -MAX_SPEED, MAX_SPEED);
  current_angular_z = constrain(current_angular_z, -MAX_ANGULAR, MAX_ANGULAR);
  
  Serial.printf("CMD_VEL: linear=%.3f, angular=%.3f\n", current_linear_x, current_angular_z);
}

// ============= WIFI SETUP =============
bool setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    return false;
  }
}

// ============= SENSORS SETUP =============
bool setup_sensors() {
  // Initialize I2C
  Wire.begin(21, 22);  // SDA=21, SCL=22
  
  // MPU6050 (IMU)
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    return false;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 initialized");
  
  // VL53L0X (Distance sensor)
  if (!distance_sensor.init()) {
    Serial.println("VL53L0X not found!");
    return false;
  }
  distance_sensor.setTimeout(500);
  Serial.println("VL53L0X initialized");
  
  return true;
}

// ============= MICRO-ROS SETUP =============
bool setup_micro_ros() {
  allocator = rcl_get_default_allocator();
  
  // Create init options
  rclc_support_init_with_options(&support, 0, NULL, &allocator);
  
  // Create node with namespace
  rclc_node_init_default(&node, "esp32_robot", "", &support);
  
  // Create publishers
  rclc_publisher_init_best_effort(&odom_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(nav_msgs, msg, Odometry), "odom");
  rclc_publisher_init_best_effort(&imu_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(sensor_msgs, msg, Imu), "imu");
  rclc_publisher_init_best_effort(&scan_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(sensor_msgs, msg, LaserScan), "scan");
  
  // Create subscriber (RELIABLE QoS for command)
  rclc_subscription_init_reliable(&cmd_vel_sub, &node, ROSIDL_GET_MSG_TYPE_NAME(geometry_msgs, msg, Twist), "cmd_vel");
  
  // Create executor
  rclc_executor_init(&executor, &support.context, 1, &allocator);  // 1 subscriber
  rclc_executor_add_subscription(&executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);
  
  Serial.println("micro-ROS initialized");
  return true;
}

// ============= MOTOR CONTROL WITH PID =============
void update_motor_commands() {
  // Check watchdog timeout
  if (millis() - last_cmd_vel_time > CMD_VEL_TIMEOUT_MS) {
    // SAFETY: Motor timeout detected
    set_motor1_speed(0);
    set_motor2_speed(0);
    current_linear_x = 0.0;
    current_angular_z = 0.0;
    Serial.println("WATCHDOG: Motor timeout - STOP");
    return;
  }
  
  // Convert twist to motor speeds
  // Differential drive kinematics:
  // v_left  = (v_linear - w * wheel_sep/2) / wheel_radius
  // v_right = (v_linear + w * wheel_sep/2) / wheel_radius
  
  float wheel_radius = WHEEL_DIAMETER / 2.0;
  float target_v_m1 = current_linear_x - (current_angular_z * WHEEL_SEPARATION / 2.0);
  float target_v_m2 = current_linear_x + (current_angular_z * WHEEL_SEPARATION / 2.0);
  
  // Clamp to max speed
  target_v_m1 = constrain(target_v_m1, -MAX_SPEED, MAX_SPEED);
  target_v_m2 = constrain(target_v_m2, -MAX_SPEED, MAX_SPEED);
  
  // Convert speed to PWM (0-255)
  // Assuming MAX_SPEED corresponds to PWM 255
  int pwm_m1 = (int)(target_v_m1 / MAX_SPEED * 255);
  int pwm_m2 = (int)(target_v_m2 / MAX_SPEED * 255);
  
  set_motor1_speed(pwm_m1);
  set_motor2_speed(pwm_m2);
}

// ============= ODOMETRY CALCULATION =============
void update_odometry() {
  // Get encoder deltas
  long delta_m1 = encoder_count_m1 - last_encoder_m1;
  long delta_m2 = encoder_count_m2 - last_encoder_m2;
  
  last_encoder_m1 = encoder_count_m1;
  last_encoder_m2 = encoder_count_m2;
  
  if (delta_m1 == 0 && delta_m2 == 0) return;
  
  // Convert pulses to distances
  float delta_dist_m1 = delta_m1 * METERS_PER_SLOT;
  float delta_dist_m2 = delta_m2 * METERS_PER_SLOT;
  
  // Average distance (forward motion)
  float delta_s = (delta_dist_m1 + delta_dist_m2) / 2.0;
  
  // Difference in distance (rotation)
  float delta_dist_diff = delta_dist_m2 - delta_dist_m1;
  float delta_theta = delta_dist_diff / WHEEL_SEPARATION;
  
  // Update pose
  if (fabs(delta_s) > 0.00001) {
    odom_x += delta_s * cosf(odom_theta + delta_theta / 2.0);
    odom_y += delta_s * sinf(odom_theta + delta_theta / 2.0);
  }
  
  odom_theta += delta_theta;
  
  // Normalize angle
  while (odom_theta > M_PI) odom_theta -= 2 * M_PI;
  while (odom_theta < -M_PI) odom_theta += 2 * M_PI;
}

// ============= PUBLISH ODOMETRY =============
void publish_odometry(rcl_time_point_value_t now) {
  // Reset message
  memset(&odom_msg, 0, sizeof(odom_msg));
  
  // Timestamp
  odom_msg.header.stamp.sec = now / 1000000000;
  odom_msg.header.stamp.nanosec = now % 1000000000;
  odom_msg.header.frame_id = "odom";
  odom_msg.child_frame_id = "base_link";
  
  // Position
  odom_msg.pose.pose.position.x = odom_x;
  odom_msg.pose.pose.position.y = odom_y;
  odom_msg.pose.pose.position.z = 0.0;
  
  // Orientation (from yaw)
  float cy = cosf(odom_theta / 2.0);
  float sy = sinf(odom_theta / 2.0);
  odom_msg.pose.pose.orientation.x = 0.0;
  odom_msg.pose.pose.orientation.y = 0.0;
  odom_msg.pose.pose.orientation.z = sy;
  odom_msg.pose.pose.orientation.w = cy;
  
  // Covariance (encoder odometry is fairly trustworthy short-term)
  odom_msg.pose.covariance[0] = 0.01;   // x
  odom_msg.pose.covariance[7] = 0.01;   // y
  odom_msg.pose.covariance[35] = 0.1;   // theta (radians²)
  
  // Velocity (not implemented yet, set to twist if available)
  odom_msg.twist.twist.linear.x = current_linear_x;
  odom_msg.twist.twist.angular.z = current_angular_z;
  
  // Publish
  rcl_publish(&odom_pub, &odom_msg, NULL);
}

// ============= PUBLISH IMU =============
void publish_imu(rcl_time_point_value_t now) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Reset message
  memset(&imu_msg, 0, sizeof(imu_msg));
  
  // Timestamp
  imu_msg.header.stamp.sec = now / 1000000000;
  imu_msg.header.stamp.nanosec = now % 1000000000;
  imu_msg.header.frame_id = "base_link";
  
  // Accelerometer (m/s²)
  imu_msg.linear_acceleration.x = a.acceleration.x;
  imu_msg.linear_acceleration.y = a.acceleration.y;
  imu_msg.linear_acceleration.z = a.acceleration.z;
  
  // Gyroscope (rad/s)
  imu_msg.angular_velocity.x = g.gyro.x;
  imu_msg.angular_velocity.y = g.gyro.y;
  imu_msg.angular_velocity.z = g.gyro.z;
  
  // Covariance (approximate from datasheet)
  for (int i = 0; i < 9; i++) {
    imu_msg.linear_acceleration_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
    imu_msg.angular_velocity_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
  }
  imu_msg.orientation_covariance[0] = -1.0;  // Orientation not provided
  
  // Publish
  rcl_publish(&imu_pub, &imu_msg, NULL);
}

// ============= PUBLISH SCAN (STUB - Phase 5) =============
void publish_scan(rcl_time_point_value_t now) {
  // Placeholder for Phase 5 improvements
  // For now, minimal scan data
  memset(&scan_msg, 0, sizeof(scan_msg));
  
  scan_msg.header.stamp.sec = now / 1000000000;
  scan_msg.header.stamp.nanosec = now % 1000000000;
  scan_msg.header.frame_id = "base_scan";
  
  scan_msg.angle_min = -M_PI / 2;
  scan_msg.angle_max = M_PI / 2;
  scan_msg.angle_increment = M_PI / 36;  // 37 rays (5° increments)
  scan_msg.time_increment = 0.001;
  scan_msg.scan_time = 0.1;
  scan_msg.range_min = 0.05;
  scan_msg.range_max = 2.0;
  
  // Would update with real scan data here
  // scan_msg.ranges and scan_msg.intensities
  
  rcl_publish(&scan_pub, &scan_msg, NULL);
}

// ============= SETUP =============
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n===== ESP32 micro-ROS Robot Controller =====");
  Serial.println("PHASE 1 & 3: WiFi Transport + Safety");
  
  // Setup motors and encoders
  setup_motors();
  setup_encoders();
  attachInterrupt(digitalPinToInterrupt(4), encoder_m1_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(5), encoder_m2_isr, RISING);
  Serial.println("Motors and encoders initialized");
  
  // Setup sensors
  if (!setup_sensors()) {
    Serial.println("ERROR: Sensor initialization failed!");
    while (1) delay(100);
  }
  
  // Setup WiFi
  if (!setup_wifi()) {
    Serial.println("ERROR: WiFi connection failed!");
    while (1) delay(100);
  }
  
  // Setup micro-ROS
  delay(1000);
  if (!setup_micro_ros()) {
    Serial.println("ERROR: micro-ROS initialization failed!");
    while (1) delay(100);
  }
  
  Serial.println("Setup complete! Running main loop...\n");
  last_cmd_vel_time = millis();  // Initialize watchdog timer
}

// ============= MAIN LOOP =============
void loop() {
  // Update motor commands (checks watchdog)
  update_motor_commands();
  
  // Calculate odometry
  update_odometry();
  
  // Spin micro-ROS executor (handles subscriptions)
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1));
  
  // Get current time
  rcl_time_point_value_t now = rcl_clock_system_default_get_current_time_nanoseconds();
  
  // Publish odometry (100 Hz)
  if (millis() - odometry_timer >= ODOM_INTERVAL_MS) {
    publish_odometry(now);
    odometry_timer = millis();
  }
  
  // Publish IMU (50 Hz)
  if (millis() - imu_timer >= IMU_INTERVAL_MS) {
    publish_imu(now);
    imu_timer = millis();
  }
  
  // Publish scan (8 Hz)
  if (millis() - scan_timer >= SCAN_INTERVAL_MS) {
    publish_scan(now);
    scan_timer = millis();
  }
  
  // Keep watchdog happy
  delayMicroseconds(100);
}
