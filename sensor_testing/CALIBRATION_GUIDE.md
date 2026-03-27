# Sensor Calibration Guide

## 1. MPU6050 (IMU) Calibration

### What to Calibrate:
- **Accelerometer**: Remove gravitational offset and bias
- **Gyroscope**: Remove bias/drift when stationary

### Calibration Steps:

#### A. Accelerometer Calibration
Place sensor on a flat, stable surface and run the calibration code:
```
1. Keep sensor absolutely still on a level surface
2. Run calibration for 10 seconds
3. Record the offset values
4. Add offsets to your code
```

#### B. Gyroscope Calibration
Keep sensor stationary during startup:
```
1. Power on with sensor at rest (no movement)
2. Let it sit still for 5 seconds
3. Initialization automatically zeros the gyroscope
4. Any offset will be subtracted from readings
```

### Expected Values After Calibration:
- **Accelerometer Z-axis**: Should read ~9.81m/s² (or ~1.0g) when level
- **Accelerometer X,Y**: Should read ~0 when level
- **Gyroscope**: Should read ~0°/s when stationary

---

## 2. VL53L0X (Distance Sensor) Calibration

### What to Calibrate:
- **Distance accuracy**: Adjust readings to match true distances
- **Spatial offset**: Account for sensor mounting position

### Calibration Steps:

#### A. Distance Calibration
1. Place known objects at exact distances: 100mm, 200mm, 500mm, 1000mm
2. Record the sensor readings
3. Create a linear correction if needed

#### B. Crosstalk Compensation
1. Point sensor at reflective surface (white wall)
2. Run crosstalk calibration
3. Stores calibration in sensor memory

#### C. Spatial Offset
1. Measure distance from sensor mounting point to front surface
2. Subtract this offset from readings

---

## 3. Servo Calibration

### Trim/Offset Calibration:
1. Command servo to 90° (center position)
2. Physically check if it's centered
3. Adjust PWM value if needed
4. Repeat for 0° and 180° endpoints

