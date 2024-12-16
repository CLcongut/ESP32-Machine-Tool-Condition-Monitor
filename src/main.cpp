/********************************************** include start*/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <esp_task_wdt.h>

#include "cl_i2s_lib.h"
#include "mtcm_pins.h"
#include "serial_cmd.h"
#include "otaupdata.h"

#include <SdFat.h>
#include <RtcDS1302.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_ADXL345_U.h>
/************************************************ include end*/

/*********************************************** define start*/
#define VERSION "# ver 4.0"

/**
 * @brief File name max length
 *
 */
#define DATAFILE_LEN 40

/**
 * @brief Use chipen pin to control MEMS
 *
 */
#define MicroPhoneEnable() digitalWrite(MEMS_CHIPEN, HIGH)
#define MicroPhoneDisable() digitalWrite(MEMS_CHIPEN, LOW)

/**
 * @brief Max data file size
 *
 */
#define WRITE_CNT_MAX 50  // 1min : 1250 | 15min : 18750 | 30min : 37500
// #define FILES_CNT_MAX 10000

/**
 * @brief switch time print
 *
 */
#define TIMESTART startTime = millis()
#define TIMEEND endTime = millis()
// #define TRANSFORM_TIME
// #define ONCE_SD_WRITE_TIME
// #define DATA_PROCESS_TIME
/************************************************* define end*/

/****************************************** task handle start*/
static TaskHandle_t xMEMSTFTask = NULL;
static TaskHandle_t xADXLTask = NULL;
// static TaskHandle_t xI2S0Task = NULL;
// static TaskHandle_t xI2S1Task = NULL;
static EventGroupHandle_t xEventMTCM = NULL;
static EventGroupHandle_t xADXLEvent = NULL;
static hw_timer_t *tim0 = NULL;
/******************************************** task handle end*/

/*********************************static data inventory start*/
static int32_t raw_samples_invt[MTCM_RAWBUF_SIZE];
static uint8_t prs_samples_invt[MTCM_PRSBUF_SIZE];
// static float ADXL_raw_invt[ADXL_BUFFER_SIZE];
// static uint8_t ADXL_prs_invt[ADXL_BUFFER_SIZE * 4];
/***********************************static data inventory end*/

/*************************************i2s Prerequisites start*/
static CL_I2S_LIB mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
static CL_I2S_LIB mtcm1(1, mtcm1.SLAVE, mtcm1.RX, mtcm1.PCM);
/***************************************i2s Prerequisites end*/

/*********************************** ADXL Prerequisites start*/
#ifdef DISABLEACC
static Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
static sensors_event_t ADXL_event;
uint16_t ADXL_data_cnt = 0;
#endif
/************************************* ADXL Prerequisites end*/

/*******************************************file system start*/
static SdFs sd;
static FsFile root;
static FsFile file;
static FsFile adxl_file;

volatile int write_num = 0;
volatile int file_num = 0;
static char mems_file_name[DATAFILE_LEN];
static char adxl_file_name[DATAFILE_LEN];
/*********************************************file system end*/

/************************* Serial Console Prerequisites start*/
static SerialCmd serialCmd;
static ConfigValue cfgValue;
/*************************** Serial Console Prerequisites end*/

/***************************** NTP Time Synchronization start*/
static ThreeWire ds1302(22, 12, 4);  // IO, SCLK, CE
static RtcDS1302<ThreeWire> Rtc(ds1302);
// static ESP32Time rtc(28800);
const char *ntpServer = "pool.ntp.org";
/******************************* NTP Time Synchronization end*/

static uint32_t startTime = 0;
static uint32_t endTime = 0;
static uint32_t max_time = 0;

void printDateTime(const RtcDateTime &dt, char *datestring = NULL,
                   const char *prefix = "") {
  if (datestring == NULL) {
    char t_date[26];
    snprintf_P(t_date, 26, PSTR("%04u/%02u/%02u %02u:%02u:%02u"), dt.Year(),
               dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
    Serial.print("RTC Time: ");
    Serial.println(t_date);
  } else {
    snprintf_P(datestring, DATAFILE_LEN,
               PSTR("%s-%04u-%02u-%02u-%02u-%02u-%02u.hex"), prefix, dt.Year(),
               dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
    Serial.println(datestring);
  }
}

void MEMS_TF_Task(void *param) {
  esp_task_wdt_init(30, true);
  // esp_task_wdt_add(NULL);

  if (!sd.begin(SdSpiConfig(5, DEDICATED_SPI, 18e6))) {
    log_e("Card Mount Failed");
  }
  root.open("/", O_WRITE);
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now, mems_file_name, "MEMS");
  file.open(mems_file_name, O_WRITE | O_CREAT);
  file.close();
#ifdef DISABLEACC
  strcpy(adxl_file_name, rtc.getTime("A-%F-%H-%M-%S.hex").c_str());
  adxl_file.open(adxl_file_name, O_WRITE | O_CREAT);
  adxl_file.close();
#endif
  root.close();

  esp_task_wdt_reset();
  xEventGroupSetBits(xEventMTCM, CARD_INIT_BIT);

  // vTaskSuspend(NULL);
  for (;;) {
    uint32_t ulNotifuValue = 0;
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 3) {
      esp_task_wdt_reset();
      ulTaskNotifyValueClear(xMEMSTFTask, 0xFFFF);

#if defined(TRANSFORM_TIME) || defined(DATA_PROCESS_TIME)
      TIMESTART;
#endif
#if defined(THREE_CHANNELS)
      for (int i = 0; i < I2S1_BUFFER_SIZE; i++) {
        uint16_t index1 = i * 9;
        uint16_t index2 = i * 2 + I2S1_BUFFER_SIZE;
        int32_t value1 = raw_samples_invt[i];
        int32_t value2 = raw_samples_invt[index2];
        int32_t value3 = raw_samples_invt[index2 + 1];

        prs_samples_invt[index1 + 0] = value1 >> 8;
        prs_samples_invt[index1 + 1] = value1 >> 16;
        prs_samples_invt[index1 + 2] = value1 >> 24;
        prs_samples_invt[index1 + 3] = value2 >> 8;
        prs_samples_invt[index1 + 4] = value2 >> 16;
        prs_samples_invt[index1 + 5] = value2 >> 24;
        prs_samples_invt[index1 + 6] = value3 >> 8;
        prs_samples_invt[index1 + 7] = value3 >> 16;
        prs_samples_invt[index1 + 8] = value3 >> 24;
      }
#elif defined(FOUR_CHANNELS)
      for (int i = 0; i < SAMPLE_IN_40MS; i++) {
        uint16_t index1 = i * 12;
        uint16_t index2 = I2S0_BUFFER_SIZE + i * 2;
        int32_t value1 = raw_samples_invt[i * 2];
        int32_t value2 = raw_samples_invt[i * 2 + 1];
        int32_t value3 = raw_samples_invt[index2];
        int32_t value4 = raw_samples_invt[index2 + 1];

        prs_samples_invt[index1 + 0] = value1 >> 8;
        prs_samples_invt[index1 + 1] = value1 >> 16;
        prs_samples_invt[index1 + 2] = value1 >> 24;
        prs_samples_invt[index1 + 3] = value2 >> 8;
        prs_samples_invt[index1 + 4] = value2 >> 16;
        prs_samples_invt[index1 + 5] = value2 >> 24;
        prs_samples_invt[index1 + 6] = value3 >> 8;
        prs_samples_invt[index1 + 7] = value3 >> 16;
        prs_samples_invt[index1 + 8] = value3 >> 24;
        prs_samples_invt[index1 + 9] = value4 >> 8;
        prs_samples_invt[index1 + 10] = value4 >> 16;
        prs_samples_invt[index1 + 11] = value4 >> 24;
      }
#endif

#ifdef TRANSFORM_TIME
      TIMEEND;
      Serial.print("Transform Time Is: ");
      Serial.print(endTime - startTime);
      Serial.println(" ms");
#elif defined(ONCE_SD_WRITE_TIME)
      TIMESTART;
#endif
      root.open("/", O_WRITE);
      file.open(mems_file_name, O_WRITE | O_APPEND);
      file.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      file.close();
      root.close();
      write_num++;
#ifdef ONCE_SD_WRITE_TIME
      TIMEEND;
      // Serial.print("SD Write Once Needs: ");
      // Serial.print(endTime - startTime);
      // Serial.println(" ms");

      if (endTime - startTime > max_time) {
        max_time = endTime - startTime;
        Serial.print(millis());
        Serial.print(" : ");
        Serial.print("Max SD Write Once Needs: ");
        Serial.print(max_time);
        Serial.println(" ms");
      }
#endif

      // if (write_num >= WRITE_CNT_MAX) {
        // vTaskSuspend(NULL);
        // write_num = 0;
        // file_num++;
        // memset(mems_file_name, 0, DATAFILE_LEN);
        // RtcDateTime now = Rtc.GetDateTime();
        // printDateTime(now, mems_file_name, "MEMS");
        // root.open("/", O_WRITE);
        // file.open(mems_file_name, O_WRITE | O_CREAT);
        // file.close();
        // root.close();
      // }

#ifdef DISABLEACC
      if ((xEventGroupGetBits(xADXLEvent) & ADXL_DONE_BIT) != 0) {
        xEventGroupClearBits(xADXLEvent, ADXL_DONE_BIT);
        root.open("/", O_WRITE);
        adxl_file.open(adxl_file_name, O_WRITE | O_APPEND);
        adxl_file.write(ADXL_prs_invt, ADXL_BUFFER_SIZE * 4);
        adxl_file.close();
        root.close();
        if (file_num >= 1) {
          file_num = 0;
          memset(adxl_file_name, 0, 20);
          strcpy(adxl_file_name, rtc.getTime("A-%F-%H-%M-%S.txt").c_str());
          root.open("/", O_WRITE);
          file.open(adxl_file_name, O_WRITE | O_CREAT);
          file.close();
          root.close();
        }
      }
#endif
#ifdef DATA_PROCESS_TIME
      TIMEEND;
      // Serial.print("Data Process Use Time: ");
      // Serial.print(endTime - startTime);
      // Serial.println(" ms");

      // if (endTime - startTime > max_time) {
      //   max_time = endTime - startTime;
      //   Serial.print(millis());
      //   Serial.print(" : ");
      //   Serial.print("Max Data Process Time: ");
      //   Serial.print(max_time);
      //   Serial.println(" ms");
      // }

      if (endTime - startTime > 30) {
        Serial.print(millis());
        Serial.print(" : ");
        Serial.print("Max Data Process Time: ");
        Serial.print(endTime - startTime);
        Serial.println(" ms");
      }
#endif
    }
    vTaskDelay(1);
  }
}

void Tim0Interrupt() {
  // xTaskNotifyGive(xADXLTask);
}

void delayNanoseconds(uint32_t ns) {
  // 计算需要的循环次数
  uint32_t cycles =
      (ns * (ESP.getCpuFreqMHz() / 1e3)) / 2;  // 每个循环大约需要 2 个时钟周期
  for (uint32_t i = 0; i < cycles; i++) {
    __asm__ __volatile__("nop");  // 空操作，消耗时钟周期
  }
}

void I2S0_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, CARD_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  // delayNanoseconds(300);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  // delayNanoseconds(300);
  mtcm2.setFormat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  // delayNanoseconds(300);
  mtcm2.setDMABuffer(I2S0_DMA_BUF_CNT, I2S0_DMA_BUF_LEN);
  // delayNanoseconds(300);
  // mtcm2.setPinMCLK(1);
  // digitalWrite(I2S0_DIN_PIN, HIGH);
  mtcm2.install(I2S0_CLK_PIN, I2S0_WS_PIN, I2S0_DIN_PIN);

  // delayNanoseconds(200);
  // delayMicroseconds(330);
  // MicroPhoneEnable();

  mtcm2.stopI2S();
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  xEventGroupWaitBits(xEventMTCM, I2S1_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  mtcm2.startI2S();
  for (;;) {
    // ulTaskNotifyTake(pdTRUE, 10);
    // xTaskNotifyGive(xI2S1Task);
    mtcm2.Read(raw_samples_invt, I2S0_BUFFER_SIZE);
    xTaskNotify(xMEMSTFTask, I2S0_DONE_BIT, eSetBits);
  }
}

void I2S1_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, I2S0_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  // delayNanoseconds(100);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  // delayNanoseconds(100);
#ifdef THREE_CHANNELS
  mtcm1.setFormat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
#elif defined(FOUR_CHANNELS)
  mtcm1.setFormat(mtcm1.RIGHT_LEFT, mtcm1.I2S);
#endif
  // delayNanoseconds(100);
  mtcm1.setDMABuffer(I2S1_DMA_BUF_CNT, I2S1_DMA_BUF_LEN);
  // mtcm1.setPinMCLK(3);
  // delayNanoseconds(100);
  mtcm1.install(I2S1_CLK_PIN, I2S1_WS_PIN, I2S1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;) {
    // ulTaskNotifyTake(pdTRUE, 10);
    // xTaskNotifyGive(xI2S0Task);
    mtcm1.Read(&raw_samples_invt[I2S1_BUFFER_SIZE], I2S1_BUFFER_SIZE);
    xTaskNotify(xMEMSTFTask, I2S1_DONE_BIT, eSetBits);
  }
}

#ifdef DISABLEACC
void ADXL_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, CARD_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);
  timerAlarmEnable(tim0);
  xEventGroupSetBits(xEventMTCM, ADXL_INIT_BIT);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    accel.getEvent(&ADXL_event);
    ADXL_raw_invt[ADXL_data_cnt * 3] = ADXL_event.acceleration.x;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 1] = ADXL_event.acceleration.y;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 2] = ADXL_event.acceleration.z;
    ADXL_data_cnt++;
    if (ADXL_data_cnt == ADXL_BUFFER_SIZE / 3) {
      ADXL_data_cnt = 0;
      memcpy(ADXL_prs_invt, ADXL_raw_invt, ADXL_BUFFER_SIZE * 4);
      xEventGroupSetBits(xADXLEvent, ADXL_DONE_BIT);
    }
    vTaskDelay(1);
  }
}
#endif

void WiFi_Init() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (strcmp(cfgValue.pswd, "null") == 0) {
    WiFi.begin(cfgValue.ssid);
  } else {
    WiFi.begin(cfgValue.ssid, cfgValue.pswd);
  }
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(200);
    Serial.print(".");
  }
  Serial.print("\r\nConnected, IP Address: ");
  Serial.println(WiFi.localIP());
}

void OTA_Task(void *param) {
  WiFi_Init();
  Serial.println("\r\nWiFi Connected, 3 Second to update!");
  vTaskDelay(3000);

  for (;;) {
    String updateURL = String(cfgValue.url);
    OTAUpdate otaUpdate;
    otaUpdate.updataBin(updateURL);

    otaUpdate.~OTAUpdate();
    vTaskDelay(1000);
    abort();
  }
}

void ntpTimeSync() {
  WiFi_Init();
  struct tm timeinfo;
  // while (true) {
  //   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //   delay(100);
  //   if (getLocalTime(&timeinfo)) {
  //     rtc.setTimeStruct(timeinfo);
  //     break;
  //   }
  // }
  Serial.println("Get NTP Time!");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi Disconnected!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("########################################");
  Serial.println("Now Version: " VERSION);
  pinMode(MEMS_CHIPEN, OUTPUT);
  Rtc.Begin();
  // RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  // printDateTime(compiled);
  // Serial.println();
  RtcDateTime now = Rtc.GetDateTime();
  // if (now < compiled) {
  //   Rtc.SetDateTime(compiled);
  // }
  printDateTime(now);

  //-------------------------------------serial console process
  serialCmd.begin();
  uint16_t buttontime = 0;
  bool isBoot = false;
  while (buttontime < 1000) {
    buttontime++;
    if (digitalRead(BOOT) == LOW) {
      isBoot = true;
      break;
    }
    vTaskDelay(1);
  }
  if (isBoot) {
    serialCmd.cmdScanf();
  }
  serialCmd.~SerialCmd();
  cfgValue = serialCmd.cmdGetConfig(true);

  if (cfgValue.update) {
    xTaskCreatePinnedToCore(OTA_Task, "OTA_Task", 4096, NULL, 1, NULL, 0);
    vTaskSuspend(NULL);
  }
  //-------------------------------------serial console process

  //----------------------------------------------timer process
  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, Tim0Interrupt, true);
  timerAlarmWrite(tim0, 10000, true);
  //----------------------------------------------timer process

  xEventMTCM = xEventGroupCreate();
  xADXLEvent = xEventGroupCreate();
  xTaskCreatePinnedToCore(MEMS_TF_Task, "MEMS_TF_Task", 4096, NULL, 5,
                          &xMEMSTFTask, 0);

  // xTaskCreatePinnedToCore(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL, 0);
  xTaskCreate(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL);
  // delayMicroseconds(50);
  // delayNanoseconds(100);
  xTaskCreate(I2S1_Task, "I2S1_Task", 2048, NULL, 4, NULL);

  // xTaskCreatePinnedToCore(I2S1_Task, "I2S1_Task", 2048, NULL, 2, NULL, 0);
#ifdef DISABLEACC
  xTaskCreate(ADXL_Task, "ADXL_Task", 2048, NULL, 3, &xADXLTask);
#endif
}

void loop() {
  // xEventGroupSync(xEventMTCM, pdFALSE,
  //                 I2S0_INIT_BIT | I2S1_INIT_BIT | ADXL_INIT_BIT,
  //                 portMAX_DELAY);
  xEventGroupSync(xEventMTCM, pdFALSE, I2S0_INIT_BIT | I2S1_INIT_BIT,
                  portMAX_DELAY);
  // MicroPhoneEnable();
  // vEventGroupDelete(xEventMTCM);
  // Serial.printf("Current time:");
  // Serial.println(rtc.getTime("%F %T"));
  vTaskDelete(NULL);
}
