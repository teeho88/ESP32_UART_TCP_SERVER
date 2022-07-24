#ifndef BW_IMU_H_
#define BW_IMU_H_

#include "tcp_server.h"
#include "stdint.h"

typedef enum{
    BR_2400,
    BR_4800,
    BR_9600,
    BR_19200,
    BR_115200
} bw_imu_baudrate;

typedef enum{
    ANSWER,
    FREQ_5,
    FREQ_10,
    FREQ_20,
    FREQ_25,
    FREQ_50,
    FREQ_100
} bw_imu_freq_mode;

typedef enum{
    TRI_ANGLE,
    TRI_ACC,
    TRI_GYRO,
    TRI_ANGLE_RESERVING,
    QUATERNION,
    ALL_DATA_TYPE
} bw_imu_data_type;

void bw_imu_init(int uart_num);
void bw_imu_set_baudrate(int baudrate);
void bw_imu_set_output_freq(int freq_mode);
void bw_imu_set_output_type(int data_type);
void bw_imu_get_ang_veloc(float* pitch, float* roll, float* z);
int bw_imu_get_ang_veloc_raw(uint8_t* data);
void bw_imu_get_ang(float* pitch, float* roll, float* yaw);
int bw_imu_get_ang_raw(uint8_t* data);
void bw_imu_get_quaternion(float q[4]);
int bw_imu_get_quaternion_raw(uint8_t* data);


#endif

