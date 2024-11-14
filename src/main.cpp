/********************************************** include start*/
#include <Arduino.h>
#include <Wire.h>
#include "WIFI.h"
#include <esp_task_wdt.h>

#include "cl_i2s_lib.h"
#include "mtcm_pins.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_AHTX0.h>
/************************************************ include end*/

/*********************************************** define start*/
#define TIMESTART startTime = millis()
#define TIMEEND endTime = millis()

/**
 * @brief switch watchdog
 *
 */
// #define WTD_ENABLE

/**
 * @brief switch time print
 *
 */
// #define TRANSFORM_TIME
// #define UDP_TAKE_TIME
// #define DATA_PROCESS_TIME
// #define TASK_GAP_TIME
/************************************************* define end*/

/****************************************** task handle start*/
static TaskHandle_t xMEMSUDPTask = NULL;
// static TaskHandle_t xADXLUDPTask = NULL;
static TaskHandle_t xADXLTask = NULL;
static EventGroupHandle_t xEventMTCM = NULL;
static EventGroupHandle_t xADXLEvent = NULL;
/******************************************** task handle end*/

static hw_timer_t *tim0 = NULL;

static CL_I2S_LIB mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
static CL_I2S_LIB mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

Adafruit_ADXL345_Unified adxl345 = Adafruit_ADXL345_Unified(0x15);
sensors_event_t ADXL_event;
float *ADXL_raw_invt;
uint8_t *ADXL_prs_invt;
uint16_t ADXL_data_cnt = 0;

Adafruit_AHTX0 aht20;

WiFiUDP udp;
// WiFiUDP adxlUdp;
IPAddress remote_IP(192, 168, 31, 199);
uint32_t remoteUdpPort = 6060;

int32_t *raw_samples_invt;
uint8_t *prs_samples_invt;

// uint32_t frm_cnt = 0;

uint32_t startTime = 0;
uint32_t endTime = 0;

void MEMSUDPTask(void *param) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifi_SSID, wifi_PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(200);
  }
  Serial.print("Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  uint32_t ulNotifuValue = 0;
#ifdef WTD_ENABLE
  esp_task_wdt_add(NULL);
#endif
  xEventGroupSetBits(xEventMTCM, UDP_INIT_BIT);
  for (;;) {
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 3) {
#ifdef WTD_ENABLE
      esp_task_wdt_reset();
#endif
      ulTaskNotifyValueClear(xMEMSUDPTask, 0xFFFF);
      size_t i;
#if defined(TRANSFORM_TIME) || defined(DATA_PROCESS_TIME)
      TIMESTART;
#endif
      //-------------------------- sound data transform process
      for (i = 0; i < MTCM1_BUFFER_SIZE; i++) {
        uint16_t index1 = i * 9;
        uint16_t index2 = i * 2 + MTCM1_BUFFER_SIZE;
        uint16_t value1 = raw_samples_invt[i];
        uint16_t value2 = raw_samples_invt[index2];
        uint16_t value3 = raw_samples_invt[index2 + 1];

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
      //-------------------------- sound data transform process
#ifdef TRANSFORM_TIME
      TIMEEND;
      Serial.print("Transform Time: ");
      Serial.println(endTime - startTime);
#endif
#ifdef UDP_TAKE_TIME
      TIMESTART;
#endif
      //-------------------------------------- udp send process
      udp.beginPacket(remote_IP, remoteUdpPort);
      udp.packetInit(0x1e);
      udp.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      udp.endPacket();
      //-------------------------------------- udp send process
#if defined(UDP_TAKE_TIME)
      TIMEEND;
      Serial.print("UDP Take Time: ");
      Serial.println(endTime - startTime);
#elif defined(DATA_PROCESS_TIME)
      TIMEEND;
      Serial.print("Data Process Time: ");
      Serial.println(endTime - startTime);
#elif defined(TASK_GAP_TIME)
      Serial.print("Task Gap Time: ");
      Serial.println(millis() - startTime);
      TIMESTART;
#endif

      if ((xEventGroupGetBits(xADXLEvent) & ADXL_DONE_BIT) != 0) {
        udp.beginPacket(remote_IP, remoteUdpPort);
        udp.packetInit(0x1f);
        udp.write(ADXL_prs_invt, ADXL_PRSBUF_SIZE);
        udp.endPacket();
      }
      // vTaskSuspend(NULL);
    }
    vTaskDelay(1);
  }
}

void Tim0Interrupt() { xTaskNotifyGive(xADXLTask); }

void I2S0_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM2_DMA_BUF_CNT, MTCM2_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  for (;;) {
    mtcm2.Read(&raw_samples_invt[MTCM1_BUFFER_SIZE], MTCM2_BUFFER_SIZE);
    xTaskNotify(xMEMSUDPTask, I2S0_DONE_BIT, eSetBits);
  }
}

void I2S1_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM1_DMA_BUF_CNT, MTCM1_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;) {
    mtcm1.Read(raw_samples_invt, MTCM1_BUFFER_SIZE);
    xTaskNotify(xMEMSUDPTask, I2S1_DONE_BIT, eSetBits);
  }
}

void ADXL_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  adxl345.begin();
  adxl345.setDataRate(ADXL345_DATARATE_1600_HZ);
  adxl345.setRange(ADXL345_RANGE_8_G);
  ADXL_raw_invt = (float *)calloc(ADXL_RAWFER_SIZE, sizeof(float));
  ADXL_prs_invt = (uint8_t *)calloc(ADXL_PRSBUF_SIZE, sizeof(uint8_t));
  timerAlarmEnable(tim0);
  xEventGroupSetBits(xEventMTCM, ADXL_INIT_BIT);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    log_e("uulala");
    adxl345.getEvent(&ADXL_event);
    ADXL_raw_invt[ADXL_data_cnt * 3] = ADXL_event.acceleration.x;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 1] = ADXL_event.acceleration.y;
    ADXL_raw_invt[ADXL_data_cnt * 3 + 2] = ADXL_event.acceleration.z;
    ADXL_data_cnt++;
    log_e("yahaha");
    if (ADXL_data_cnt == ADXL_SAMPLE_CNT - 1) {
      ADXL_data_cnt = 0;
      memcpy(ADXL_prs_invt, ADXL_raw_invt, ADXL_PRSBUF_SIZE);
      // xTaskNotify(xADXLUDPTask, 0x02, eSetBits);
      xEventGroupSetBits(xADXLEvent, ADXL_DONE_BIT);
    }
    // vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);

  raw_samples_invt = (int32_t *)calloc(MTCM_ALL_SAMPLE_CNT, sizeof(int32_t));
  prs_samples_invt = (uint8_t *)calloc(MTCM_PRSBUF_SIZE, sizeof(uint8_t));

  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, Tim0Interrupt, true);
  timerAlarmWrite(tim0, 1000, true);

  xEventMTCM = xEventGroupCreate();
  xADXLEvent = xEventGroupCreate();
  xTaskCreatePinnedToCore(MEMSUDPTask, "MEMSUDPTask", 4096, NULL, 5,
                          &xMEMSUDPTask, 0);
  xTaskCreate(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 2048, NULL, 4, NULL);
  xTaskCreate(ADXL_Task, "ADXL_Task", 2048, NULL, 3, &xADXLTask);
}

void loop() {
  xEventGroupSync(xEventMTCM, pdFALSE,
                  I2S0_INIT_BIT | I2S1_INIT_BIT | ADXL_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
  log_w("24 Bit Data Output Mode!");
  vTaskDelete(NULL);
}
