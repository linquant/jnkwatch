#pragma once
#include <stdbool.h>
#include <stdint.h>

/* QMI8658 6-axis IMU on the shared I2C bus (0x6B). We use the accelerometer for
 * lift-to-wake and the aura activity level; the gyro is left disabled (unused).
 * Returns true if the chip answered with the expected WHO_AM_I. */
bool imu_qmi8658_init(void);

/* Read the accelerometer in g (1.0 == gravity). az is +1 with the screen up.
 * Returns false if the IMU is not initialised or an I2C error occurred. */
bool imu_qmi8658_read_accel(double *ax, double *ay, double *az);

/* Diagnostic: STATUS0 register (bit0/1 = accel/gyro data ready), -1 on error. */
int imu_qmi8658_status0(void);

/* Arm wake-on-motion: put the accel in low-power mode and route a motion
 * interrupt to INT1 (board pin BSP_IMU_INT / GPIO8), so the chip can wake the
 * ESP32-S3 from deep sleep on a wrist raise. Call right before deep-sleeping.
 * `threshold` is the WoM sensitivity in mg (lower = more sensitive). */
void imu_qmi8658_arm_wom(uint8_t threshold_mg);
