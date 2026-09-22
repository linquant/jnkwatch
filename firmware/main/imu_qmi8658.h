#pragma once
#include <stdbool.h>

/* QMI8658 6-axis IMU on the shared I2C bus (0x6B). We use the accelerometer for
 * lift-to-wake and the aura activity level; the gyro is left disabled (unused).
 * Returns true if the chip answered with the expected WHO_AM_I. */
bool imu_qmi8658_init(void);

/* Read the accelerometer in g (1.0 == gravity). az is +1 with the screen up.
 * Returns false if the IMU is not initialised or an I2C error occurred. */
bool imu_qmi8658_read_accel(double *ax, double *ay, double *az);

/* Diagnostic: STATUS0 register (bit0/1 = accel/gyro data ready), -1 on error. */
int imu_qmi8658_status0(void);
