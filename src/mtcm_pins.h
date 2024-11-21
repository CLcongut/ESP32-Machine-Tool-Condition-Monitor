#pragma once
#include <Arduino.h>

#define wifi_SSID "CCongut"
#define wifi_PSWD "88888888"

#define UDP_INIT_BIT (1 << 0)
#define I2S0_INIT_BIT (1 << 1)
#define I2S1_INIT_BIT (1 << 2)
#define ADXL_INIT_BIT (1 << 3)

#define ADXL_DONE_BIT (1 << 0)
#define MEMS_DONE_BIT (1 << 1)

#define I2S0_DONE_BIT (1 << 0)
#define I2S1_DONE_BIT (1 << 1)

#define UDP_PACKAGE_NUM_24 12
#define BYTE_PER_PACKAGE 1440

#define MTCM_RAWBUF_SIZE 5760  // (16 * 1440) / 4
#define MTCM_PRSBUF_SIZE 17280
#define MTCM1_BUFFER_SIZE 1920  // 5760 * 1 / 3
#define MTCM2_BUFFER_SIZE 3840  // 5760 * 2 / 3

#define ADXL_SAMPLE_CNT 40
#define ADXL_RAWFER_SIZE 120
#define ADXL_8BIT_SIZE 480

#define LM20_SAMPLE_CNT 4
#define LM20_RAWFER_SIZE 4
#define LM20_8BIT_SIZE 16

#define MIXED_PRSBUF_SIZE 496

#define MTCM_SAMPLE_RATE 48000
#define MTCM_BPS 32
#define MTCM2_DMA_BUF_CNT 8
#define MTCM2_DMA_BUF_LEN 480
#define MTCM1_DMA_BUF_CNT 4
#define MTCM1_DMA_BUF_LEN 480

#define uS_TO_S 1e6
#define D_GAP_TIME 5
#define D_RUN_TIME 2

const int MTCM2_CLK_PIN = 32;
const int MTCM2_WS_PIN = 33;
const int MTCM2_DIN_PIN = 25;
const int MTCM1_CLK_PIN = 26;
const int MTCM1_WS_PIN = 27;
const int MTCM1_DIN_PIN = 14;

const int ACCEL_CS_PIN = 5;

const int STATUS_LED = 2;
const int ANALOG_PIN = 35;