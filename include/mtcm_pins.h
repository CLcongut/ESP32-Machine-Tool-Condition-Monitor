#pragma once
#include <Arduino.h>

#define wifi_SSID "CCongut"
#define wifi_PSWD "88888888"

#define MTCM_SPLBUFF_ALL 12000
#define MTCM2_SPBUF_SIZE 8000
#define MTCM1_SPBUF_SIZE 4000

#define I2S0_SAMPLE_BUFFER_SIZE 2000
#define I2S1_SAMPLE_BUFFER_SIZE 4000
#define ALLS_SAMPLE_BUFFER_SIZE 12000
#define SAMPLE_RATE 40000

#define MTCM_SAMPLE_RATE 10000
#define MTCM_BPS 32
#define MTCM_DMA_BUF_CNT 50
#define MTCM_DMA_BUF_LEN 1000

const int SIGN_LED = 2;
const int MTCM2_CLK_PIN = 33;
const int MTCM2_WS_PIN = 32;
const int MTCM2_DIN_PIN = 35;
const int MTCM1_CLK_PIN = 27;
const int MTCM1_WS_PIN = 26;
const int MTCM1_DIN_PIN = 25;