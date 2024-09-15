#pragma once
#include <Arduino.h>

#define wifi_SSID "CCongut"
#define wifi_PSWD "88888888"

#define MTCM_SPLBUFF_ALL 12000
#define MTCM2_SPBUF_SIZE 8000
#define MTCM1_SPBUF_SIZE 4000

#define I2S0_SAMPLE_BUFFER_SIZE 8000
#define I2S1_SAMPLE_BUFFER_SIZE 4000
#define ALLS_SAMPLE_BUFFER_SIZE 12000
#define SAMPLE_RATE 40000

#define MTCM_SAMPLE_RATE 40000
#define MTCM_BPS 16
#define MTCM_DMA_BUF_CNT 8
#define MTCM_DMA_BUF_LEN 1024

const int SIGN_LED = 2;
const int MTCM2_CLK_PIN = 25;
const int MTCM2_DIN_PIN = 33;
const int MTCM1_CLK_PIN = 27;
const int MTCM1_DIN_PIN = 26;