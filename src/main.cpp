#include <Arduino.h>
#include "I2S_93.h"
#include "mtcm_pins.h"
#include <SPI.h>
#include "SdFat.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <time.h>
#include "ESP32Time.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

static ESP32Time rtc(28800);

static TaskHandle_t xMEMSTFTask = NULL;
static TaskHandle_t xADXLTask = NULL;
static EventGroupHandle_t xEventMTCM = NULL;
static EventGroupHandle_t xADXLEvent = NULL;
static hw_timer_t *tim0 = NULL;

static SdFs sd;
static FsFile root;
static FsFile file;
static FsFile adxl_file;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

static Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
static sensors_event_t ADXL_event;
float *ADXL_raw_invt;
uint16_t ADXL_data_cnt = 0;
uint8_t *ADXL_prs_invt;

int32_t *raw_samples_invt;
uint8_t *prs_samples_invt;

volatile int write_num = 0;
volatile int file_num = 0;
char mems_file_name[40];
char adxl_file_name[40];

// const char *ssid = "CCongut";
const char *ssid = "CLcongut";
const char *password = "88888888";
const char *ntpServer = "cn.pool.ntp.org";

void MEMSTFTask(void *param)
{
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);
  if (!sd.begin(SdSpiConfig(5, DEDICATED_SPI, 18e6)))
  {
    log_e("Card Mount Failed");
  }
  root.open("/", O_WRITE);
  strcpy(mems_file_name, rtc.getTime("M-%F-%H-%M-%S.txt").c_str());
  file.open(mems_file_name, O_WRITE | O_CREAT);
  file.close();
  strcpy(adxl_file_name, rtc.getTime("A-%F-%H-%M-%S.txt").c_str());
  adxl_file.open(adxl_file_name, O_WRITE | O_CREAT);
  adxl_file.close();
  root.close();

  esp_task_wdt_reset();
  xEventGroupSetBits(xEventMTCM, CARD_INIT_BIT);
  for (;;)
  {
    uint32_t ulNotifuValue = 0;
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 3)
    {
      esp_task_wdt_reset();
      digitalWrite(STATUS_LED, LOW);
      ulTaskNotifyValueClear(xMEMSTFTask, 0xFFFF);
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
      root.open("/", O_WRITE);
      file.open(mems_file_name, O_WRITE | O_APPEND);
      file.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      file.close();
      root.close();
      write_num++;
      Serial.println("MEMS Data");

      if (write_num >= WRITE_CNT_MAX)
      {
        write_num = 0;
        file_num++;
        memset(mems_file_name, 0, 20);
        strcpy(mems_file_name, rtc.getTime("M-%F-%H-%M-%S.txt").c_str());
        root.open("/", O_WRITE);
        file.open(mems_file_name, O_WRITE | O_CREAT);
        file.close();
        root.close();
        Serial.println("MEMS File");
      }

      digitalWrite(STATUS_LED, HIGH);
      if ((xEventGroupGetBits(xADXLEvent) & ADXL_DONE_BIT) != 0)
      {
        xEventGroupClearBits(xADXLEvent, ADXL_DONE_BIT);
        root.open("/", O_WRITE);
        adxl_file.open(adxl_file_name, O_WRITE | O_APPEND);
        adxl_file.write(ADXL_prs_invt, ADXL_BUFFER_SIZE * 4);
        adxl_file.close();
        root.close();
        Serial.println("ADXL Data");
        if (file_num >= 1)
        {
          file_num = 0;
          memset(adxl_file_name, 0, 20);
          strcpy(adxl_file_name, rtc.getTime("A-%F-%H-%M-%S.txt").c_str());
          root.open("/", O_WRITE);
          file.open(adxl_file_name, O_WRITE | O_CREAT);
          file.close();
          root.close();
          Serial.println("ADXL File");
        }
      }
    }
    vTaskDelay(1);
  }
}

void Tim0Interrupt()
{
  xTaskNotifyGive(xADXLTask);
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
    xTaskNotify(xMEMSTFTask, I2S0_DONE_BIT, eSetBits);
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
    xTaskNotify(xMEMSTFTask, I2S1_DONE_BIT, eSetBits);
  }
}

void ADXL_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, CARD_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);
  ADXL_raw_invt = (float *)calloc(ADXL_BUFFER_SIZE, sizeof(float));
  ADXL_prs_invt = (uint8_t *)calloc(ADXL_BUFFER_SIZE * 4, sizeof(uint8_t));
  timerAlarmEnable(tim0);
  xEventGroupSetBits(xEventMTCM, ADXL_INIT_BIT);
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    accel.getEvent(&ADXL_event);
    ADXL_raw_invt[ADXL_data_cnt * 3] = ADXL_event.acceleration.x;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 1] = ADXL_event.acceleration.y;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 2] = ADXL_event.acceleration.z;
    ADXL_data_cnt++;
    if (ADXL_data_cnt == ADXL_BUFFER_SIZE / 3)
    {
      ADXL_data_cnt = 0;
      memcpy(ADXL_prs_invt, ADXL_raw_invt, ADXL_BUFFER_SIZE * 4);
      xEventGroupSetBits(xADXLEvent, ADXL_DONE_BIT);
    }
    vTaskDelay(1);
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

  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, Tim0Interrupt, true);
  timerAlarmWrite(tim0, 10000, true);

  xEventMTCM = xEventGroupCreate();
  xADXLEvent = xEventGroupCreate();
  xTaskCreatePinnedToCore(MEMSTFTask, "MEMSTFTask", 4096, NULL, 5, &xMEMSTFTask, 0);
  xTaskCreate(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 2048, NULL, 4, NULL);
  xTaskCreate(ADXL_Task, "ADXL_Task", 2048, NULL, 3, &xADXLTask);
}

void loop()
{
  xEventGroupSync(xEventMTCM, pdFALSE, I2S0_INIT_BIT | I2S1_INIT_BIT | ADXL_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
  log_w("Data Storage Mode!");
  Serial.printf("Current time:");
  Serial.println(rtc.getTime("%F %T"));
  vTaskDelete(NULL);
}
