#pragma once
#include <Arduino.h>

#define wifi_SSID "CCongut"
#define wifi_PSWD "88888888"

// #define BITS_32_TRANSMIT
#define BITS_24_TRANSMIT

#define UDP_INIT_BIT (1 << 0)
#define I2S0_INIT_BIT (1 << 1)
#define I2S1_INIT_BIT (1 << 2)

#define UDP_PACKAGE_NUM_32 16
#define UDP_PACKAGE_NUM_24 12
#define BYTE_PER_PACKAGE 1440

#define MTCM_SPLBUFF_ALL 5760 // (16 * 1440) / 4
#define MTCM2_SPBUF_SIZE 3840 // 5760 * 2 / 3
#define MTCM1_SPBUF_SIZE 1920 // 5760 * 1 / 3

#define MTCM_SAMPLE_RATE 40000
#define MTCM_BPS 32
#define MTCM_DMA_BUF_CNT 2
#define MTCM_DMA_BUF_LEN 960

const int SIGN_LED = 2;
const int MTCM2_CLK_PIN = 33;
const int MTCM2_WS_PIN = 32;
const int MTCM2_DIN_PIN = 35;
const int MTCM1_CLK_PIN = 27;
const int MTCM1_WS_PIN = 26;
const int MTCM1_DIN_PIN = 25;