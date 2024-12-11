#pragma once
#include <Arduino.h>

#define CARD_INIT_BIT (1 << 0)
#define I2S0_INIT_BIT (1 << 1)
#define I2S1_INIT_BIT (1 << 2)
#define ADXL_INIT_BIT (1 << 3)

#define ADXL_DONE_BIT (1 << 0)

#define I2S0_DONE_BIT (1 << 0)
#define I2S1_DONE_BIT (1 << 1)

// #define BYTE_PER_PACKAGE 1440

#define MTCM_SPLBUFF_ALL 5760  // (16 * 1440) / 4
#define I2S0_SPBUF_SIZE 3840   // 5760 * 2 / 3
#define I2S1_SPBUF_SIZE 1920   // 5760 * 1 / 3
#define MTCM_PRSBUF_SIZE 17280
#define ADXL_BUFFER_SIZE 360

#define MTCM_SAMPLE_RATE 48000
#define MTCM_BPS 32
#define I2S0_DMA_BUF_CNT 8
#define I2S0_DMA_BUF_LEN 480
#define I2S1_DMA_BUF_CNT 4
#define I2S1_DMA_BUF_LEN 480

#define WRITE_CNT_MAX 18750  // 1min : 1250 | 15min : 18750 | 30min : 37500
// #define FILES_CNT_MAX 10000

const int I2S0_CLK_PIN = 33;
const int I2S0_WS_PIN = 25;
const int I2S0_DIN_PIN = 32;
const int I2S1_CLK_PIN = 27;
const int I2S1_WS_PIN = 26;
const int I2S1_DIN_PIN = 14;

const int BOOT = 0;

// IO2 has other usage
// const int STATUS_LED = 2;
// touchpad temporarily unavailable

// const int TOUCH_PAD = 15;
// #define TOUCH_THRESH 30