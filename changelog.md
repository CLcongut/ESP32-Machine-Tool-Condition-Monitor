# ver 5.0
### 2024-11-21

## Feature

- 增加串口控制台功能，在另一个工程完成的编写测试
- 完成串口控制台移植适配


# ver 4.6
### 2024-11-16

## Feature

- 增加ADC读取lm20模拟温度传感器，100hz
- 增加跳过自定义次数再发送的功能

## Changed

- 将adxl专用8位invt改为与lm20的混合invt


# Ver 4.5
### 2024-11-15

## Changed

- 更换ADXL库，不使用adafruit库
- 移除aht20库
- 使用ADXL345_WE库，SPI通信
- ADXL_raw_invt -> raw_adxl_invt
- ADXL_prs_invt -> prs_adxl_invt
- ADXL_data_cnt -> adxl_tims_cnt
- 增加ADXL_Task分配内存

## Feature

- 增加UDP发送开关，调试用
- 增加UDP发送次数开关，调试用
- 增加MEMS运行间隔测量
- 增加ADXL运行间隔测量

## Fixed

- 将MEMSUDPTask改名为UDP_Send_Task规范
- 将全局变量及对象改为静态
- 将用于存放覆写数据的内存从栈区改为堆区
- 修复数据只有高位有效的问题(uint16_t -> int32_t)
- 