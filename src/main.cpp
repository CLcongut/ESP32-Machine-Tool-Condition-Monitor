#include <Arduino.h>
#include "WIFI.h"
#include "I2S_93.h"
#include "mtcm_pins.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

static TaskHandle_t xMEMSUDPTask = NULL;
static TaskHandle_t xADXLUDPTask = NULL;
static TaskHandle_t xADXLTask = NULL;
static EventGroupHandle_t xEventMTCM = NULL;
static hw_timer_t *tim0 = NULL;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
sensors_event_t ADXL_event;
float *ADXL_data_invt;
uint16_t ADXL_data_cnt = 0;

WiFiUDP udp;
IPAddress remote_IP(192, 168, 31, 199);
uint32_t remoteUdpPort = 6060;

int32_t *raw_samples_invt;
uint8_t *prs_samples_invt;

// uint32_t frm_cnt = 0;

void MEMSUDPTask(void *param)
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifi_SSID, wifi_PSWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(200);
  }
  Serial.print("Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  uint32_t ulNotifuValue = 0;
  xEventGroupSetBits(xEventMTCM, UDP_INIT_BIT);
  for (;;)
  {
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 2)
    {
      ulTaskNotifyValueClear(xMEMSUDPTask, 0xFFFF);
      size_t i;
      for (i = 0; i < MTCM1_SPBUF_SIZE; i++)
      {
        prs_samples_invt[i * 9] = raw_samples_invt[i] >> 24;
        prs_samples_invt[i * 9 + 1] = raw_samples_invt[i] >> 16;
        prs_samples_invt[i * 9 + 2] = raw_samples_invt[i] >> 8;
        prs_samples_invt[i * 9 + 3] = raw_samples_invt[i * 2 + MTCM1_SPBUF_SIZE] >> 24;
        prs_samples_invt[i * 9 + 4] = raw_samples_invt[i * 2 + MTCM1_SPBUF_SIZE] >> 16;
        prs_samples_invt[i * 9 + 5] = raw_samples_invt[i * 2 + MTCM1_SPBUF_SIZE] >> 8;
        prs_samples_invt[i * 9 + 6] = raw_samples_invt[i * 2 + 1 + MTCM1_SPBUF_SIZE] >> 24;
        prs_samples_invt[i * 9 + 7] = raw_samples_invt[i * 2 + 1 + MTCM1_SPBUF_SIZE] >> 16;
        prs_samples_invt[i * 9 + 8] = raw_samples_invt[i * 2 + 1 + MTCM1_SPBUF_SIZE] >> 8;
      }
      udp.beginPacket(remote_IP, remoteUdpPort);
      udp.packetInit(0x1e);
      udp.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      udp.endPacket();
      // vTaskDelete(NULL);
    }
    vTaskDelay(1);
  }
}

void ADXLUDPTask(void *param)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    udp.beginPacket(remote_IP, remoteUdpPort);
    udp.packetInit(0x1f);
    udp.write((uint8_t *)ADXL_data_invt, ADXL_BUFFER_SIZE * 4);
    udp.endPacket();
    vTaskDelay(1);
  }
}

void Tim0Interrupt()
{
  xTaskNotifyGive(xADXLTask);
}

void I2S0_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM2_DMA_BUF_CNT, MTCM2_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  for (;;)
  {
    mtcm2.Read(&raw_samples_invt[MTCM1_SPBUF_SIZE], MTCM2_SPBUF_SIZE);
    xTaskNotifyGive(xMEMSUDPTask);
  }
}

void I2S1_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM1_DMA_BUF_CNT, MTCM1_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;)
  {
    mtcm1.Read(raw_samples_invt, MTCM1_SPBUF_SIZE);
    xTaskNotifyGive(xMEMSUDPTask);
  }
}

void ADXL_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);
  ADXL_data_invt = (float *)calloc(ADXL_BUFFER_SIZE, sizeof(float));
  timerAlarmEnable(tim0);
  xEventGroupSetBits(xEventMTCM, ADXL_INIT_BIT);
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    accel.getEvent(&ADXL_event);
    ADXL_data_invt[ADXL_data_cnt * 3] = ADXL_event.acceleration.x;
    ADXL_data_invt[ADXL_data_cnt * 3 + 1] = ADXL_event.acceleration.y;
    ADXL_data_invt[ADXL_data_cnt * 3 + 2] = ADXL_event.acceleration.z;
    ADXL_data_cnt++;
    if (ADXL_data_cnt == ADXL_BUFFER_SIZE / 3)
    {
      ADXL_data_cnt = 0;
      xTaskNotifyGive(xADXLUDPTask);
    }
    vTaskDelay(1);
  }
}

void setup()
{
  Serial.begin(115200);

  raw_samples_invt = (int32_t *)calloc(MTCM_SPLBUFF_ALL, sizeof(int32_t));
  prs_samples_invt = (uint8_t *)calloc(MTCM_PRSBUF_SIZE, sizeof(uint8_t));

  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, Tim0Interrupt, true);
  timerAlarmWrite(tim0, 10000, true);

  xEventMTCM = xEventGroupCreate();
  xTaskCreatePinnedToCore(MEMSUDPTask, "MEMSUDPTask", 4096, NULL, 5, &xMEMSUDPTask, 0);
  xTaskCreatePinnedToCore(ADXLUDPTask, "ADXLUDPTask", 4096, NULL, 4, &xADXLUDPTask, 1);
  xTaskCreate(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 2048, NULL, 4, NULL);
  xTaskCreate(ADXL_Task, "ADXL_Task", 2048, NULL, 1, &xADXLTask);
}

void loop()
{
  xEventGroupSync(xEventMTCM, pdFALSE, I2S0_INIT_BIT | I2S1_INIT_BIT | ADXL_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
  log_w("24 Bit Data Output Mode!");
  vTaskDelete(NULL);
}
