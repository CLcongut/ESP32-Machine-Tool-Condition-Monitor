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

#define UDP_PACKAGE_NUM_24 12
#define BYTE_PER_PACKAGE 1440

#define MTCM_SPLBUFF_ALL 5760 // (16 * 1440) / 4
#define MTCM2_SPBUF_SIZE 3840 // 5760 * 2 / 3
#define MTCM1_SPBUF_SIZE 1920 // 5760 * 1 / 3
#define MTCM_PRSBUF_SIZE 17280
#define ADXL_BUFFER_SIZE 360

#define MTCM_SAMPLE_RATE 40000
#define MTCM_BPS 32
#define MTCM2_DMA_BUF_CNT 8
#define MTCM2_DMA_BUF_LEN 480
#define MTCM1_DMA_BUF_CNT 4
#define MTCM1_DMA_BUF_LEN 480

const int STATUS_LED = 2;
const int MTCM2_CLK_PIN = 32;
const int MTCM2_WS_PIN = 33;
const int MTCM2_DIN_PIN = 25;
const int MTCM1_CLK_PIN = 26;
const int MTCM1_WS_PIN = 27;
const int MTCM1_DIN_PIN = 14;