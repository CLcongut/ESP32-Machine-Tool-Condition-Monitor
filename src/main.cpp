#include <Arduino.h>
#include "I2S_93.h"
#include "mtcm_pins.h"
#include <SPI.h>
#include "SdFat.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <time.h>
#include "ESP32Time.h"

ESP32Time rtc(28800);

static TaskHandle_t xTFTrasn = NULL;
static bool restart_flag = false;
static EventGroupHandle_t xEventMTCM = NULL;

SdFs sd;
FsFile root;
FsFile file;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

int32_t *raw_samples_invt;
uint8_t *prs_samples_invt;

int write_num = 0;
int file_num = 0;
char file_name[40];

const char *ssid = "CCongut";
const char *password = "88888888";
const char *ntpServer = "cn.pool.ntp.org";

void TFTask(void *param)
{
  esp_task_wdt_add(NULL);
  if (!sd.begin(SdSpiConfig(5, DEDICATED_SPI, 18e6)))
  {
    log_e("Card Mount Failed");
  }
  root.open("/", O_WRITE);
  strcpy(file_name, rtc.getTime("%A%B%d%Y%H%M%S.txt").c_str());
  file.open(file_name, O_WRITE | O_CREAT);
  file.close();

  uint32_t ulNotifuValue = 0;
  xEventGroupSetBits(xEventMTCM, CARD_INIT_BIT);
  for (;;)
  {
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 2)
    {
      esp_task_wdt_reset();
      digitalWrite(STATUS_LED, LOW);
      ulTaskNotifyValueClear(xTFTrasn, 0xFFFF);
      for (int i = 0; i < MTCM1_SPBUF_SIZE; i++)
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

      file.open(file_name, O_WRITE | O_APPEND);
      file.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      file.close();
      write_num++;

      if (write_num >= WRITE_CNT_MAX)
      {
        write_num = 0;
        file_num++;
        memset(file_name, 0, 20);
        // sprintf(file_name, "data%d.txt", file_num);
        strcpy(file_name, rtc.getTime("%A%B%d%Y%H%M%S.txt").c_str());
        file.open(file_name, O_WRITE | O_CREAT);
        file.close();
      }

      if (file_num >= FILES_CNT_MAX)
      {
        // sd.remove(file_name);
        root.close();
        digitalWrite(STATUS_LED, HIGH);
        // log_w("files write done");
        vTaskDelete(NULL);
      }
      digitalWrite(STATUS_LED, HIGH);
    }
    vTaskDelay(1);
  }
}

void I2S0_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, CARD_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM2_DMA_BUF_CNT, MTCM2_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  for (;;)
  {
    mtcm2.Read(&raw_samples_invt[MTCM1_SPBUF_SIZE], MTCM2_SPBUF_SIZE);
    xTaskNotifyGive(xTFTrasn);
  }
}

void I2S1_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, CARD_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM1_DMA_BUF_CNT, MTCM1_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;)
  {
    mtcm1.Read(raw_samples_invt, MTCM1_SPBUF_SIZE);
    xTaskNotifyGive(xTFTrasn);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");

  /*---------set with NTP---------------*/
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    rtc.setTimeStruct(timeinfo);
  }
  Serial.println("Get NTP Time!");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  log_w("WiFi Disconnected!");

  raw_samples_invt = (int32_t *)calloc(MTCM_SPLBUFF_ALL, sizeof(int32_t));
  prs_samples_invt = (uint8_t *)calloc(MTCM_PRSBUF_SIZE, sizeof(uint8_t));

  xEventMTCM = xEventGroupCreate();
  xTaskCreatePinnedToCore(TFTask, "TFTask", 4096, NULL, 3, &xTFTrasn, 0);
  xTaskCreate(I2S0_Task, "I2S0_Task", 4096, NULL, 1, NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 4096, NULL, 1, NULL);
}

void loop()
{
  xEventGroupSync(xEventMTCM, pdFALSE, I2S0_INIT_BIT | I2S1_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
  log_w("Data Storage Mode!");
  Serial.printf("Current time:");
  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  vTaskDelete(NULL);
}
