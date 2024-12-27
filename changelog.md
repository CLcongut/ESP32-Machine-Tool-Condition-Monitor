# ver 6.6
### 2024-12-26

## Feature

- 增加ESPAsyncWebServer库
- 网页配置功能初版

## Changed

- 暂时关闭AP UDP配置
- 网页标题改为两行

# ver 6.5
### 2024-12-26

## Fixed

- 修复温度采集次数不正确
- 温度引脚是板子问题，等待2.2.3版本
- 增加pref获取的默认值

## Feature

- AP热点用端口号命名
- OTA提示灯可用函数空中

# ver 6.4
### 2024-12-25

## Feature

- 将串口和UDP的格式化输出打包成函数
- 使用宏来开关提示灯
- 整理语句，更加简洁美观
- 新增固件版本打印指令/ver

## Changed

- 不带参语句预制化
- 指令改成全小写
- 旧解析函数归档
- SerialCmd Class中变量赋初始值
- 移除 void cmdGetHelp(); 已经成为单独语句
- 取消了log_e()的使用

## Fixed

- 修复一些小写问题
- 合并 charInfo_1_3 和 charInfo_1_4, 重新排序
- 修复了udp无法打印全部信息

# ver 6.3b
### 2024-12-23

## Feature

- 增加UDP任务耗时打印
- 关闭分时
- 增加AP控制台配置功能
- 增加BOOT引脚功能

## Changed

- 将串口控制台中长段const char* 尽数存于flash

# ver 6.2
### 2024-12-20

## Feature

- 适配新板子（连接clk和ws）
- i2s1为从

# ver 6.1
### 2024-12-16

## Fixed

- 删除多余的cmdprefer.end();

## Feature

- 增加cl_i2s库的开启关闭功能
- 同步两个i2s

# ver 6.0
### 2024-12-3

## Feature

- 增加OTA空中下载
- WiFi连接中会打印”...“
- 将连接WiFi打包成函数

## Fixed

- 修复无法连接无密码WiFi的问题
- 移除工程所需wifi.h，增加兼容性

# ver 5.1
### 2024-11-23

## Feature

- 增加串口打印软件版本

## Changed

- 适配到新板子，更改引脚
- 关闭warning调试等级
- 更删除“24位”提示信息
- 移除写在程序里的配置信息

## Fixed

- 使用新版本cl_i2s_lib
- 修复setformat -> setFormat

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