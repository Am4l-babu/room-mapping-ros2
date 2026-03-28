/*
 * ESP32 micro-ROS Client with Time Synchronization
 * PHASES 1, 2, 3: WiFi Transport + Time Sync + Safety Watchdog
 *
 * Features:
 *   - WiFi connectivity + micro-ROS
 *   - Synchronized timestamps with ROS 2 (Phase 2)
 *   - Motor watchdog safety (Phase 3)
 *   - High-rate publishing (100 Hz odom, 50 Hz imu, 8 Hz scan)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <rcl/rcl.h>
#include <rcl/publisher.h>
#include <rcl/subscription.h>
#include <rcl/executor.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>
#include <sensor_msgs/msg/imu.h>
#include <sensor_msgs/msg/laser_scan.h>
#include <math.h>
#include <Adafruit_MPU6050.h>
#include <VL53L0X.h>

#include "motors_encoders.h"
#include "time_sync.h"  // NEW: Time synchronization module

// ============= WIFI CONFIGURATION =============
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* AGENT_IP = "192.168.1.100";
const uint16_t AGENT_PORT = 8888;

// ============= MICRO-ROS SETUP =============
rcl_context_t context;
rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;
rcl_clock_t clock;  // NEW: For time synchronization

// Publishers
rcl_publisher_t odom_pub, imu_pub, scan_pub;

// Subscribers
rcl_subscription_t cmd_vel_sub;

// Messages
nav_msgs__msg__Odometry odom_msg;
sensor_msgs__msg__Imu imu_msg;
sensor_msgs__msg__LaserScan scan_msg;
geometry_msgs__msg__Twist cmd_vel_msg;

// ============= HARDWARE =============
Adafruit_MPU6050 mpu;
VL53L0X distance_sensor;
volatile long encoder_count_m1 = 0, encoder_count_m2 = 0;
volatile long last_encoder_m1 = 0, last_encoder_m2 = 0;

// ============= CONTROL STATE =============
float current_linear_x = 0.0;
float current_angular_z = 0.0;
unsigned long last_cmd_vel_time = 0;
const unsigned long CMD_VEL_TIMEOUT_MS = 500;

// ============= ODOMETRY STATE =============
float odom_x = 0.0, odom_y = 0.0, odom_theta = 0.0;

// ============= CONSTANTS =============
const float WHEEL_DIAMETER = 0.065;
const float WHEEL_SEPARATION = 0.18;
const int ENCODER_SLOTS = 20;
const float METERS_PER_SLOT = (M_PI * WHEEL_DIAMETER) / ENCODER_SLOTS;
const float MAX_SPEED = 0.5;
const float MAX_ANGULAR = 2.0;

// ============= TIMING =============
unsigned long odometry_timer = 0;
unsigned long imu_timer = 0;
unsigned long scan_timer = 0;
unsigned long time_sync_timer = 0;  // NEW: Time sync timer
const unsigned long ODOM_INTERVAL_MS = 10;      // 100 Hz
const unsigned long IMU_INTERVAL_MS = 20;       //  50 Hz
const unsigned long SCAN_INTERVAL_MS = 125;     //   8 Hz
const unsigned long TIME_SYNC_INTERVAL_MS = 30000;  // NEW: Sync every 30s

// ============= ENCODER INTERRUPTS =============
void IRAM_ATTR encoder_m1_isr() {
    encoder_count_m1++;
}

void IRAM_ATTR encoder_m2_isr() {
    encoder_count_m2++;
}

// ============= CMD_VEL CALLBACK =============
void cmd_vel_callback(const void* msgin) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msgin;
    
    last_cmd_vel_time = millis();
    current_linear_x = msg->linear.x;
    current_angular_z = msg->angular.z;
    
    current_linear_x = constrain(current_linear_x, -MAX_SPEED, MAX_SPEED);
    current_angular_z = constrain(current_angular_z, -MAX_ANGULAR, MAX_ANGULAR);
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
    Wire.begin(21, 22);
    
    if (!mpu.begin()) {
        Serial.println("MPU6050 not found!");
        return false;
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 initialized");
    
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
    rclc_support_init_with_options(&support, 0, NULL, &allocator);
    
    // Initialize system clock (NEW: For time sync)
    rcl_clock_init(RCL_SYSTEM_TIME, &clock, &allocator);
    
    rclc_node_init_default(&node, "esp32_robot", "", &support);
    
    // Publishers
    rclc_publisher_init_best_effort(&odom_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(nav_msgs, msg, Odometry), "odom");
    rclc_publisher_init_best_effort(&imu_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(sensor_msgs, msg, Imu), "imu");
    rclc_publisher_init_best_effort(&scan_pub, &node, ROSIDL_GET_MSG_TYPE_NAME(sensor_msgs, msg, LaserScan), "scan");
    
    // Subscriber (RELIABLE for safety)
    rclc_subscription_init_reliable(&cmd_vel_sub, &node, ROSIDL_GET_MSG_TYPE_NAME(geometry_msgs, msg, Twist), "cmd_vel");
    
    // Executor
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);
    
    // NEW: Initialize time synchronization
    time_sync_init();
    
    Serial.println("micro-ROS initialized");
    return true;
}

// ============= MOTOR CONTROL =============
void update_motor_commands() {
    // WATCHDOG: Safety timeout
    if (millis() - last_cmd_vel_time > CMD_VEL_TIMEOUT_MS) {
        set_motor1_speed(0);
        set_motor2_speed(0);
        current_linear_x = 0.0;
        current_angular_z = 0.0;
        return;
    }
    
    // Differential drive
    float target_v_m1 = current_linear_x - (current_angular_z * WHEEL_SEPARATION / 2.0);
    float target_v_m2 = current_linear_x + (current_angular_z * WHEEL_SEPARATION / 2.0);
    
    target_v_m1 = constrain(target_v_m1, -MAX_SPEED, MAX_SPEED);
    target_v_m2 = constrain(target_v_m2, -MAX_SPEED, MAX_SPEED);
    
    int pwm_m1 = (int)(target_v_m1 / MAX_SPEED * 255);
    int pwm_m2 = (int)(target_v_m2 / MAX_SPEED * 255);
    
    set_motor1_speed(pwm_m1);
    set_motor2_speed(pwm_m2);
}

// ============= ODOMETRY =============
void update_odometry() {
    long delta_m1 = encoder_count_m1 - last_encoder_m1;
    long delta_m2 = encoder_count_m2 - last_encoder_m2;
    
    last_encoder_m1 = encoder_count_m1;
    last_encoder_m2 = encoder_count_m2;
    
    if (delta_m1 == 0 && delta_m2 == 0) return;
    
    float delta_dist_m1 = delta_m1 * METERS_PER_SLOT;
    float delta_dist_m2 = delta_m2 * METERS_PER_SLOT;
    
    float delta_s = (delta_dist_m1 + delta_dist_m2) / 2.0;
    float delta_dist_diff = delta_dist_m2 - delta_dist_m1;
    float delta_theta = delta_dist_diff / WHEEL_SEPARATION;
    
    if (fabs(delta_s) > 0.00001) {
        odom_x += delta_s * cosf(odom_theta + delta_theta / 2.0);
        odom_y += delta_s * sinf(odom_theta + delta_theta / 2.0);
    }
    
    odom_theta += delta_theta;
    
    while (odom_theta > M_PI) odom_theta -= 2 * M_PI;
    while (odom_theta < -M_PI) odom_theta += 2 * M_PI;
}

// ============= PUBLISH ODOMETRY (with synchronized timestamps) =============
void publish_odometry() {
    rcl_time_point_value_t now = time_sync_get_timestamp();  // NEW: Synchronized time
    
    memset(&odom_msg, 0, sizeof(odom_msg));
    
    odom_msg.header.stamp.sec = now / 1000000000;
    odom_msg.header.stamp.nanosec = now % 1000000000;
    odom_msg.header.frame_id = "odom";
    odom_msg.child_frame_id = "base_link";
    
    odom_msg.pose.pose.position.x = odom_x;
    odom_msg.pose.pose.position.y = odom_y;
    odom_msg.pose.pose.position.z = 0.0;
    
    float cy = cosf(odom_theta / 2.0);
    float sy = sinf(odom_theta / 2.0);
    odom_msg.pose.pose.orientation.x = 0.0;
    odom_msg.pose.pose.orientation.y = 0.0;
    odom_msg.pose.pose.orientation.z = sy;
    odom_msg.pose.pose.orientation.w = cy;
    
    odom_msg.pose.covariance[0] = 0.01;
    odom_msg.pose.covariance[7] = 0.01;
    odom_msg.pose.covariance[35] = 0.1;
    
    odom_msg.twist.twist.linear.x = current_linear_x;
    odom_msg.twist.twist.angular.z = current_angular_z;
    
    rcl_publish(&odom_pub, &odom_msg, NULL);
}

// ============= PUBLISH IMU (with synchronized timestamps) =============
void publish_imu() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    rcl_time_point_value_t now = time_sync_get_timestamp();  // NEW: Synchronized time
    
    memset(&imu_msg, 0, sizeof(imu_msg));
    
    imu_msg.header.stamp.sec = now / 1000000000;
    imu_msg.header.stamp.nanosec = now % 1000000000;
    imu_msg.header.frame_id = "base_link";
    
    imu_msg.linear_acceleration.x = a.acceleration.x;
    imu_msg.linear_acceleration.y = a.acceleration.y;
    imu_msg.linear_acceleration.z = a.acceleration.z;
    
    imu_msg.angular_velocity.x = g.gyro.x;
    imu_msg.angular_velocity.y = g.gyro.y;
    imu_msg.angular_velocity.z = g.gyro.z;
    
    for (int i = 0; i < 9; i++) {
        imu_msg.linear_acceleration_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
        imu_msg.angular_velocity_covariance[i] = (i % 4 == 0) ? 0.01 : 0.0;
    }
    imu_msg.orientation_covariance[0] = -1.0;
    
    rcl_publish(&imu_pub, &imu_msg, NULL);
}

// ============= PUBLISH SCAN (with synchronized timestamps) =============
void publish_scan() {
    rcl_time_point_value_t now = time_sync_get_timestamp();  // NEW: Synchronized time
    
    memset(&scan_msg, 0, sizeof(scan_msg));
    
    scan_msg.header.stamp.sec = now / 1000000000;
    scan_msg.header.stamp.nanosec = now % 1000000000;
    scan_msg.header.frame_id = "base_scan";
    
    scan_msg.angle_min = -M_PI / 2;
    scan_msg.angle_max = M_PI / 2;
    scan_msg.angle_increment = M_PI / 36;
    scan_msg.time_increment = 0.001;
    scan_msg.scan_time = 0.1;
    scan_msg.range_min = 0.05;
    scan_msg.range_max = 2.0;
    
    rcl_publish(&scan_pub, &scan_msg, NULL);
}

// ============= SETUP =============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n===== ESP32 micro-ROS Robot Controller =====");
    Serial.println("PHASES 1, 2, 3: Transport + Time Sync + Safety");
    
    setup_motors();
    setup_encoders();
    attachInterrupt(digitalPinToInterrupt(4), encoder_m1_isr, RISING);
    attachInterrupt(digitalPinToInterrupt(5), encoder_m2_isr, RISING);
    
    if (!setup_sensors()) {
        Serial.println("ERROR: Sensor initialization failed!");
        while (1) delay(100);
    }
    
    if (!setup_wifi()) {
        Serial.println("ERROR: WiFi connection failed!");
        while (1) delay(100);
    }
    
    delay(1000);
    if (!setup_micro_ros()) {
        Serial.println("ERROR: micro-ROS initialization failed!");
        while (1) delay(100);
    }
    
    Serial.println("Setup complete! Running main loop...\n");
    last_cmd_vel_time = millis();
    time_sync_timer = millis();
}

// ============= MAIN LOOP =============
void loop() {
    update_motor_commands();
    update_odometry();
    
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1));
    
    // NEW: Periodic time synchronization
    if (millis() - time_sync_timer >= TIME_SYNC_INTERVAL_MS) {
        time_sync_update(&clock);
        time_sync_timer = millis();
    }
    
    // Publish odometry (100 Hz)
    if (millis() - odometry_timer >= ODOM_INTERVAL_MS) {
        publish_odometry();
        odometry_timer = millis();
    }
    
    // Publish IMU (50 Hz)
    if (millis() - imu_timer >= IMU_INTERVAL_MS) {
        publish_imu();
        imu_timer = millis();
    }
    
    // Publish scan (8 Hz)
    if (millis() - scan_timer >= SCAN_INTERVAL_MS) {
        publish_scan();
        scan_timer = millis();
    }
    
    delayMicroseconds(100);
}
