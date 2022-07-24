#include "bw_imu.h"

int UART_NUM = 0;
int freq_mode = 0;

void send_command(uint8_t* command, size_t len)
{
    uart_write_bytes(UART_NUM, command, len);
}

void save_setting(void)
{
    uint8_t cmd[5] = {0x77,0x04,0x00,0x0A,0x0E};
    send_command(cmd,5);
}

void bw_imu_set_baudrate(int baudrate)
{
    uint8_t baud_byte;
    switch(baudrate)
    {
        case BR_2400: baud_byte = 0x00; break;
        case BR_4800: baud_byte = 0x01; break;
        case BR_9600: baud_byte = 0x02; break;
        case BR_19200: baud_byte = 0x03; break;
        case BR_115200: baud_byte = 0x04; break;
        default: baud_byte = 0x02; break;
    }
    uint8_t cmd[6] = {0x77,0x05,0x00,0x0B,baud_byte,0x12};
    send_command(cmd,sizeof(cmd));
}

void bw_imu_set_output_freq(int freqMode)
{
    uint8_t freq_byte;
    freq_mode = freqMode;
    switch(freq_mode)
    {
        case ANSWER: freq_byte = 0x00; break;
        case FREQ_5: freq_byte = 0x01; break;
        case FREQ_10: freq_byte = 0x02; break;
        case FREQ_20: freq_byte = 0x03; break;
        case FREQ_25: freq_byte = 0x04; break;
        case FREQ_50: freq_byte = 0x05; break;
        case FREQ_100: freq_byte = 0x06; break;
        default: freq_byte = 0x00; break;
    }
    uint8_t cmd[6] = {0x77,0x05,0x00,0x0C,freq_byte,0x11};
    send_command(cmd,sizeof(cmd));
    vTaskDelay(500/portTICK_PERIOD_MS);
    save_setting();
}

void bw_imu_set_output_type(int data_type)
{
    uint8_t data_type_byte;
    switch(data_type)
    {
        case TRI_ANGLE: data_type_byte = 0x00; break;
        case TRI_ACC: data_type_byte = 0x01; break;
        case TRI_GYRO: data_type_byte = 0x02; break;
        case TRI_ANGLE_RESERVING: data_type_byte = 0x03; break;
        case QUATERNION: data_type_byte = 0x04; break;
        case ALL_DATA_TYPE: data_type_byte = 0x05; break;
        default: data_type_byte = 0x00; break;
    }
    uint8_t cmd[6] = {0x77,0x05,0x00,0x56,data_type_byte,0x5B};
    send_command(cmd,sizeof(cmd));
}

void bw_imu_init(int uart_num)
{
    UART_NUM = uart_num;
    bw_imu_set_baudrate(BR_115200);
    bw_imu_set_output_freq(ANSWER);
}

void bw_imu_get_ang_veloc(float* pitch, float* roll, float* z)
{
    uint8_t* data = (uint8_t*) malloc(14);
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x50,0x54};
        send_command(cmd,sizeof(cmd));
    }
    uart_read_bytes(UART_NUM, data, 14, 500/portTICK_RATE_MS);
    float ang_veloc[3];
    for(int i = 0; i < 3; i+=3)
    {    
        uint8_t sign = data[4+i]>>4;
        uint8_t int0 = data[4+i]&0x0f;
        uint8_t int1 = data[5+i]>>4;
        uint8_t int2 = data[5+i]&0x0f;
        uint8_t dec0 = data[6+i]>>4;
        uint8_t dec1 = data[6+i]&0x0f;
        ang_veloc[i] = int0*100 + int1*10 + int2 + dec0*1.0/10 + dec1*1.0/100;
        if(sign > 0)
        {
            ang_veloc[i] *= -1;
        }
    }
    free(data);
    *pitch = ang_veloc[0]; *roll = ang_veloc[1]; *z = ang_veloc[2];
}

int bw_imu_get_ang_veloc_raw(uint8_t* data)
{
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x50,0x54};
        send_command(cmd,sizeof(cmd));
    }
    return uart_read_bytes(UART_NUM, data, 14, 500/portTICK_RATE_MS);
}

void bw_imu_get_ang(float* pitch, float* roll, float* yaw)
{
    uint8_t* data = (uint8_t*) malloc(14);
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x04,0x08};
        send_command(cmd,sizeof(cmd));
    }
    uart_read_bytes(UART_NUM, data, 14, 500/portTICK_RATE_MS);
    float ang[3];
    for(int i = 0; i < 3; i+=3)
    {    
        uint8_t sign = data[4+i]>>4;
        uint8_t int0 = data[4+i]&0x0f;
        uint8_t int1 = data[5+i]>>4;
        uint8_t int2 = data[5+i]&0x0f;
        uint8_t dec0 = data[6+i]>>4;
        uint8_t dec1 = data[6+i]&0x0f;
        ang[i] = int0*100 + int1*10 + int2 + dec0*1.0/10 + dec1*1.0/100;
        if(sign > 0)
        {
            ang[i] *= -1;
        }
    }
    free(data);
    *pitch = ang[0]; *roll = ang[1]; *yaw = ang[2];
}

int bw_imu_get_ang_raw(uint8_t* data)
{
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x04,0x08};
        send_command(cmd,sizeof(cmd));
    }
    return uart_read_bytes(UART_NUM, data, 14, 500/portTICK_RATE_MS);
}

void bw_imu_get_quaternion(float q[4])
{
    uint8_t* data = (uint8_t*) malloc(21);
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x57,0x5B};
        send_command(cmd,sizeof(cmd));
    }
    uart_read_bytes(UART_NUM, data, 21, 500/portTICK_RATE_MS);
    for(int i = 0; i < 4; i+=4)
    {    
        uint8_t sign = data[4+i]>>4;
        uint8_t int0 = data[4+i]&0x0f;
        uint8_t dec0 = data[5+i]>>4;
        uint8_t dec1 = data[5+i]&0x0f;
        uint8_t dec2 = data[6+i]>>4;
        uint8_t dec3 = data[6+i]&0x0f;
        uint8_t dec4 = data[7+i]>>4;
        uint8_t dec5 = data[7+i]&0x0f;
        q[i] = int0 + dec0*1.0/10 + dec1*1.0/100 + dec2*1.0/1000 + 
                dec3*1.0/10000 + dec4*1.0/100000 + dec5*1.0/1000000;
        if(sign > 0)
        {
            q[i] *= -1;
        }
    }
    free(data);
}

int bw_imu_get_quaternion_raw(uint8_t* data)
{
    if(freq_mode == ANSWER)
    {
        uint8_t cmd[5] = {0x77,0x04,0x00,0x57,0x5B};
        send_command(cmd,sizeof(cmd));
    }
    return uart_read_bytes(UART_NUM, data, 21, 500/portTICK_RATE_MS);
}


