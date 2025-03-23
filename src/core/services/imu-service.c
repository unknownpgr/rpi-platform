#include <imu-service.h>

#include <dev.h>

#define ICM42067_ADDR 0x68
#define ICM42067_WHOAMI 0x75
#define ICM42067_WHOAMI_VAL 0x67
#define ICM42067_DATA 0x0B
#define ICM42067_PWR_MGMT 0x1F

#define BMM150_ADDR 0x12
#define BMM150_WHOAMI 0x40
#define BMM150_WHOAMI_VAL 0x32
#define BMM150_DATA 0x42
#define BMM150_PWR_CNTL_1 0x4B
#define BMM150_PWR_CNTL_2 0x4C

void imu_init()
{
    dev_i2c_enable(true);

    // Read dummy data once
    uint8_t dummy;
    dev_i2c_read_register(ICM42067_ADDR, ICM42067_WHOAMI, &dummy, 1);

    // ICM-42607
    {

        // Check if the ICM-42607 WHOAMI register is correct
        uint8_t whoami;
        if (!dev_i2c_read_register(ICM42067_ADDR, ICM42067_WHOAMI, &whoami, 1))
        {
            perror("Failed to read ICM-42607 WHOAMI register");
            return;
        }

        if (whoami != ICM42067_WHOAMI_VAL)
        {
            perror("ICM-42607 WHOAMI mismatch: 0x%02X, expected: 0x%02X", whoami, ICM42067_WHOAMI_VAL);
            return;
        }

        // Turn gyro / accel on
        uint8_t pwr_mgmt = 0x00;
        pwr_mgmt |= 0b11 << 2; // Gyroscope in low noise mode
        pwr_mgmt |= 0b11 << 0; // Accelerometer in low noise mode
        if (!dev_i2c_write_register(ICM42067_ADDR, ICM42067_PWR_MGMT, &pwr_mgmt, 1))
        {
            perror("Failed to write ICM-42607 PWR_MGMT register");
            return;
        }
    }

    // BMM150
    {
        // Soft reset
        uint8_t reset = 1 << 7 || 1 << 1;
        if (!dev_i2c_write_register(BMM150_ADDR, BMM150_PWR_CNTL_1, &reset, 1))
        {
            perror("Failed to write BMM150 PWR_CNTL_2 register");
            return;
        }

        // Power on
        uint8_t pwr1 = 0x01;
        if (!dev_i2c_write_register(BMM150_ADDR, BMM150_PWR_CNTL_1, &pwr1, 1))
        {
            perror("Failed to write BMM150 PWR_CNTL_1 register");
            return;
        }

        // Turn into normal mode
        uint8_t pwr2 = 0x00;
        if (!dev_i2c_write_register(BMM150_ADDR, BMM150_PWR_CNTL_2, &pwr2, 1))
        {
            perror("Failed to write BMM150 PWR_CNTL_2 register");
            return;
        }
    }
}

void imu_read(imu_data_t *data)
{
    uint8_t buf[12];
    if (!dev_i2c_read_register(ICM42067_ADDR, ICM42067_DATA, buf, 12))
    {
        perror("Failed to read ICM-42607 accelerometer data");
        return;
    }

    int16_t accel_x = (int16_t)(buf[0] << 8 | buf[1]);
    int16_t accel_y = (int16_t)(buf[2] << 8 | buf[3]);
    int16_t accel_z = (int16_t)(buf[4] << 8 | buf[5]);
    int16_t gyro_x = (int16_t)(buf[6] << 8 | buf[7]);
    int16_t gyro_y = (int16_t)(buf[8] << 8 | buf[9]);
    int16_t gyro_z = (int16_t)(buf[10] << 8 | buf[11]);

    data->ax = accel_x;
    data->ay = accel_y;
    data->az = accel_z;
    data->gx = gyro_x;
    data->gy = gyro_y;
    data->gz = gyro_z;
}

void imu_read_geomagnetic(imu_geomagnetic_t *data)
{
    uint8_t buf[8];
    if (!dev_i2c_read_register(BMM150_ADDR, BMM150_DATA, buf, 8))
    {
        perror("Failed to read BMM150 geomagnetic data");
        return;
    }

    int16_t x = (int16_t)(buf[1] << 8 | buf[0]); // 13-bit
    int16_t y = (int16_t)(buf[3] << 8 | buf[2]); // 13-bit
    int16_t z = (int16_t)(buf[5] << 8 | buf[4]); // 15-bit
    int16_t r = (int16_t)(buf[7] << 8 | buf[6]); // 14-bit

    data->x = x;
    data->y = y;
    data->z = z;
    data->r = r;
}