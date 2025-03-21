#include "mpu9250.h"

#define IIC_DMA_ENABLED 0
#define IIC_HARDWARE_ENABLED 1
#define IIC_SOFTWARE_ENABLED 0

extern I2C_HandleTypeDef hi2c1;

static uint8_t mpu9250_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t size)
{
#if IIC_DMA_ENABLED
  return HAL_I2C_Mem_Read_DMA(&hi2c1, (dev_addr << 1) | 0x00, reg_addr, I2C_MEMADD_SIZE_8BIT, buf, size);
#elif IIC_HARDWARE_ENABLED
  return HAL_I2C_Mem_Read(&hi2c1, (dev_addr << 1) | 0x00, reg_addr, I2C_MEMADD_SIZE_8BIT, buf, size, 5);
#else
  return i2c_read_len(dev_addr, reg_addr, size, buf);
#endif
}

static uint8_t mpu9250_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t size)
{
#if IIC_DMA_ENABLED
  HAL_I2C_Mem_Write_DMA(&hi2c1, (dev_addr << 1) | 0x00, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size);
#elif IIC_HARDWARE_ENABLED
  return HAL_I2C_Mem_Write(&hi2c1, (dev_addr << 1) | 0x00, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size, 5);
#else
  return i2c_write_len(dev_addr, reg_addr, size, data);
#endif
}

/**
 *range :0,±250deg/s, 1,±500deg/s, 2,±1000deg/s, 3,±2000deg/s
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t mpu_set_gyro_range(uint8_t range)
{
  uint8_t data1, data2;
  data1 = range << 3;
  mpu9250_write(MPU9250_ADDR, MPU_GYRO_CFG_REG, &data1, 1);
  mpu9250_read(MPU9250_ADDR, MPU_GYRO_CFG_REG, &data2, 1);
  if(data1 == data2)
    return 0;
  else
    return 1;
}

/**
 *range :0,±2g, 1,±4g, 2,±8g, 3,±16g
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t mpu_set_accl_range(uint8_t range)
{
  uint8_t data = range << 3;
  mpu9250_write(MPU9250_ADDR, MPU_ACCL_CFG_REG, &data, 1);
  return 0;
}

/**
 *des   :设置低通滤波器
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t MPU_Set_LPF(uint16_t lpf)
{
  uint8_t data = 0;
  if (lpf >= 188)
    data = 1;
  else if (lpf >= 98)
    data = 2;
  else if (lpf >= 42)
    data = 3;
  else if (lpf >= 20)
    data = 4;
  else if (lpf >= 10)
    data = 5;
  else
    data = 6;
  return mpu9250_write(MPU9250_ADDR, MPU_CFG_REG, &data, 1);
}

/**
 *des   :设置采样率
 *rate  :采样率
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t MPU_Set_rate(uint16_t rate)
{
  uint8_t data;
  if (rate > 1000)
    rate = 1000;
  if (rate < 4)
    rate = 1;
  data = 1000 / rate - 1;
  data = mpu9250_write(MPU9250_ADDR, MPU_SAMPLE_RATE_REG, &data, 1);

  return MPU_Set_LPF(rate / 2); // 自动设置LPF为采样率的一半
}

uint8_t mpu9250_init(void)
{
  uint8_t res = 0, data = 0, buf = 0;
#if IIC_SOFTWARE_ENABLED
  i2c_init();
#endif
  data = 0x80;
  mpu9250_write(MPU9250_ADDR, MPU_PWR_MGMT1_REG, &data, 1); // 复位MPU9250
  delay_ms(100);
  data = 0x00;
  mpu9250_write(MPU9250_ADDR, MPU_PWR_MGMT1_REG, &data, 1); // 唤醒MPU9250 H_RESET会自动清0

  res = mpu9250_read(MPU9250_ADDR, MPU_WHO_AM_I, &buf, 1); // 读取MPU9250的ID
  if (buf == MPU9250_ID)
  {
    data = 0x03;
    mpu9250_write(MPU9250_ADDR, MPU_PWR_MGMT1_REG, &data, 1);
    mpu9250_read(MPU9250_ADDR, MPU_PWR_MGMT1_REG, &buf, 1);
    data = 0x00;
    mpu9250_write(MPU9250_ADDR, MPU_PWR_MGMT2_REG, &data, 1); // 开启陀螺仪和加速度计(on)
  }
  else
  {
    return 1;
  }

  mpu_set_gyro_range(3);                                 // 设置陀螺仪的量程为±2000deg/s
  mpu_set_accl_range(3);                                 // 设置加速度计的量程为±16G
  MPU_Set_rate(200);                                     // 设置采样率为500Hz
  data = 0x00;
  mpu9250_write(MPU9250_ADDR, MPU_INT_EN_REG, &data, 1);    // 关闭所有中断
  mpu9250_write(MPU9250_ADDR, MPU_USER_CTRL_REG, &data, 1); // I2C主模式关闭
  mpu9250_write(MPU9250_ADDR, MPU_FIFO_EN_REG, &data, 1);   // 关闭FIFO
  data = 0x02;
  mpu9250_write(MPU9250_ADDR, MPU_INTBP_CFG_REG, &data, 1); // 开启BYPASS模式，可以直接读取磁力计

  res = mpu9250_read(MAG_ADDR, MAG_WHO_AM_I, &buf, 1);
  if (buf == AK8963_ID)
  {

    // data = 0x10 | 0x06; // 16 bit, continuous mode 2, 100hz
    data = 0x10 | 0x01; // 16bit, single mode
    // data = 0x00 | 0x01; // 14bit, single mode
    mpu9250_write(MAG_ADDR, MAG_CNTL1, &data, 1);
  }
  else
  {
    return 1;
  }

  return 0;
}

/**
 *return:温度值
 */
int16_t mpu9250_get_temperture(void)
{
  uint8_t buf[2];
  int16_t raw;
  float temp;

  mpu9250_read(MPU9250_ADDR, MPU_TEMP_OUTH_REG, buf, 2);
  raw = ((uint8_t)buf[0] << 8) | buf[1];
  temp = ((double)raw) / 333.87 + 21; // ???(TEMP_degC = ((TEMP_OUT –RoomTemp_Offset)/Temp_Sensitivity) + 21degC)
  return temp * 100;
}

/**
 *des   :得到陀螺仪值(原始值)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t _mpu9250_get_gyro_raw(int16_t *gx, int16_t *gy, int16_t *gz)
{
  uint8_t buf[6], res;
  res = mpu9250_read(MPU9250_ADDR, MPU_GYRO_XOUT_H, buf, 6);
  if (res == 0)
  {
    *gx = ((int16_t)buf[0] << 8) | buf[1];
    *gy = ((int16_t)buf[2] << 8) | buf[3];
    *gz = ((int16_t)buf[4] << 8) | buf[5];
  }
  return res;
}

/**
 *des   :得到加速度值(原始值)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t _mpu9250_get_accl_raw(int16_t *ax, int16_t *ay, int16_t *az)
{
  uint8_t buf[6], res;
  res = mpu9250_read(MPU9250_ADDR, MPU_ACCL_XOUT_H, buf, 6);
  if (res == 0)
  {
    *ax = ((int16_t)buf[0] << 8) | buf[1];
    *ay = ((int16_t)buf[2] << 8) | buf[3];
    *az = ((int16_t)buf[4] << 8) | buf[5];
  }
  return res;
}

/**
 *des   :得到磁力计值(原始值)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t _mpu9250_get_mag_raw(int16_t *mx, int16_t *my, int16_t *mz)
{
  uint8_t buf[6] = {0}, res = 1, data = 0;

  uint8_t st1 = 0;
  mpu9250_read(MAG_ADDR, MAG_ST1, &st1, 1);
  if(st1 & 0x01)
  {
    res = mpu9250_read(MAG_ADDR, MAG_HXL, buf, 6);
  }

  if (res == 0)
  {
    *mx = (((int16_t)buf[1] << 8) | buf[0]);
    *my = (((int16_t)buf[3] << 8) | buf[2]);
    *mz = (((int16_t)buf[5] << 8) | buf[4]);
  }
  // data = 0x10 | 0x06; // 16 bit, continuous mode 2, 100 hz
  data = 0x10 | 0x01; // 16bit, single mode
  // data = 0x00 | 0x01; // 14bit, single mode
  mpu9250_write(MAG_ADDR, MAG_CNTL1, &data, 1); // AK8963读完之后都需要重新设置为单次测试模式

  return res;
}

/**
 *des   :得到陀螺仪值(单位：弧度/秒)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t mpu9250_get_gyro(mpu9250_data_t *buf)
{
  mpu9250_raw_data_t tmp;

  if (_mpu9250_get_gyro_raw(&tmp.x, &tmp.y, &tmp.z) == 0)
  {
    buf->data[0] = ((float)tmp.x) * MPU9250_GYRO_2000_SEN;
    buf->data[1] = ((float)tmp.y) * MPU9250_GYRO_2000_SEN;
    buf->data[2] = ((float)tmp.z) * MPU9250_GYRO_2000_SEN;
    return 0;
  }
  return 1;
}

/**
 *des   :得到加速度值(单位：米/平方秒)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t mpu9250_get_accl(mpu9250_data_t *buf)
{
  mpu9250_raw_data_t tmp;

  if (_mpu9250_get_accl_raw(&tmp.x, &tmp.y, &tmp.z) == 0)
  {
    buf->data[0] = ((float)tmp.x) * MPU9250_ACCL_16G_SEN;
    buf->data[1] = ((float)tmp.y) * MPU9250_ACCL_16G_SEN;
    buf->data[2] = ((float)tmp.z) * MPU9250_ACCL_16G_SEN;
    return 0;
  }
  return 1;
}

/**
 *des   :得到磁力计值(单位：uT)
 *return:返回0设置成功，返回其他设置失败
 */
uint8_t mpu9250_get_mag(mpu9250_data_t *buf)
{
  mpu9250_raw_data_t tmp;

  if (_mpu9250_get_mag_raw(&tmp.x, &tmp.y, &tmp.z) == 0)
  {
    buf->data[0] = ((float)tmp.x) * 0.15;
    buf->data[1] = ((float)tmp.y) * 0.15;
    buf->data[2] = ((float)tmp.z) * 0.15;
    return 0;
  }
  return 1;
}
