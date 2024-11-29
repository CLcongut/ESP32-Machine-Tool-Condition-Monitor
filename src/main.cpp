/********************************************** include start*/
#include <Arduino.h>
#include <SPI.h>
#include "WIFI.h"
#include <esp_task_wdt.h>

#include "cl_i2s_lib.h"
#include "mtcm_pins.h"
#include "serial_cmd.h"

#include <ADXL345_WE.h>
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
 * @brief switch UDP for debug
 *
 */
// #define MEMS_UDP_SEND_ENABLE
// #define ADXL_UDP_SEND_ENABLE
#define ALL_UDP_SEND_ENABLE

/**
 * @brief switch time print
 *
 */
// #define TRANSFORM_TIME
// #define UDP_TAKE_TIME
// #define DATA_PROCESS_TIME
// #define UDP_GAP_TIME
// #define MEMS_GAP_TIME
// #define ADXL_GAP_TIME

/**
 * @brief send times limit
 *
 */
// #define SEND_TIMES_LIMIT
#define SEND_TIMES 10

/**
 * @brief skip some times to send
 *
 */
// #define SKIP_TO_SEND
#ifdef SKIP_TO_SEND
#define SKIP_TIMES 10
#else
#define SKIP_TIMES 0
#endif
/************************************************* define end*/

/****************************************** task handle start*/
static TaskHandle_t xUDPSendTask = NULL;
static TaskHandle_t xADXLTask = NULL;
static EventGroupHandle_t xEventMTCM = NULL;
static EventGroupHandle_t xADXLEvent = NULL;
static hw_timer_t *tim0 = NULL;
/******************************************** task handle end*/

/*********************************static data inventory start*/
static int32_t raw_samples_invt[MTCM_RAWBUF_SIZE];
static uint8_t prs_samples_invt[MTCM_PRSBUF_SIZE];

static float raw_adxl_invt[ADXL_RAWFER_SIZE];
static uint32_t raw_lm20_invt[LM20_RAWFER_SIZE];

static uint8_t prs_mixed_invt[MIXED_PRSBUF_SIZE];
/***********************************static data inventory end*/

/*************************************i2s Prerequisites start*/
static CL_I2S_LIB mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
static CL_I2S_LIB mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);
/***************************************i2s Prerequisites end*/

/************************************ UDP Prerequisites start*/
static WiFiUDP udp;
/************************************** UDP Prerequisites end*/

/*********************************** ADXL Prerequisites start*/
static bool VSPI_Flag = true;
static ADXL345_WE ADXL345(ACCEL_CS_PIN, VSPI_Flag);
static uint16_t adxl_tims_cnt = 0;
/************************************* ADXL Prerequisites end*/

/************************* Serial Console Prerequisites start*/
static SerialCmd serialCmd;
static ConfigValue cfgValue;
/************************** Serial Console Prerequisites end*/

static uint32_t startTime = 0;
static uint32_t endTime = 0;

static uint32_t send_times_cnt = 0;
static uint32_t skip_times_cnt = 0;

static uint8_t send_gap_time = 0;
static uint8_t send_run_time = 0;
static bool send_real_time = false;

void UDP_Send_Task(void *param) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(cfgValue.ssid, cfgValue.pswd);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(200);
  }
  Serial.print("\r\nConnected, IP Address: ");
  Serial.println(WiFi.localIP());
  uint32_t ulNotifuValue = 0;
  static uint16_t cmd_send_times = send_run_time * 25 + SKIP_TIMES;
#ifdef WTD_ENABLE
  esp_task_wdt_add(NULL);
#endif
  udp.beginPacket(cfgValue.ipv4, cfgValue.port);
  // udp.print("Connected, IP Address: ");
  // udp.println(WiFi.localIP());
  udp.endPacket();
  xEventGroupSetBits(xEventMTCM, UDP_INIT_BIT);
  for (;;) {
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    if (ulNotifuValue == 3) {
#ifdef WTD_ENABLE
      esp_task_wdt_reset();
#endif
      ulTaskNotifyValueClear(xUDPSendTask, 0xFFFF);
      size_t i;
#if defined(TRANSFORM_TIME) || defined(DATA_PROCESS_TIME)
      TIMESTART;
#endif
      //-------------------------- sound data transform process
      for (i = 0; i < MTCM1_BUFFER_SIZE; i++) {
        uint16_t index1 = i * 9;
        uint16_t index2 = i * 2 + MTCM1_BUFFER_SIZE;
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
      //-------------------------- sound data transform process
#ifdef TRANSFORM_TIME
      TIMEEND;
      Serial.print("Transform Time: ");
      Serial.println(endTime - startTime);
#elif defined(UDP_TAKE_TIME)
      TIMESTART;
#endif
      //--------------------------------sound data send process
#if defined(MEMS_UDP_SEND_ENABLE) || defined(ALL_UDP_SEND_ENABLE)
#ifdef SKIP_TO_SEND
      if (skip_times_cnt < SKIP_TIMES) {
        ;
      } else {
        udp.beginPacket(cfgValue.ipv4, cfgValue.port);
        udp.packetInit(0x1e);
        udp.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
        udp.endPacket();
      }
#else
      udp.beginPacket(cfgValue.ipv4, cfgValue.port);
      udp.packetInit(0x1e);
      udp.write(prs_samples_invt, MTCM_PRSBUF_SIZE);
      udp.endPacket();
#endif
#endif
      //--------------------------------sound data send process
#if defined(UDP_TAKE_TIME)
      TIMEEND;
      Serial.print("UDP Take Time: ");
      Serial.println(endTime - startTime);
#elif defined(DATA_PROCESS_TIME)
      TIMEEND;
      Serial.print("Data Process Time: ");
      Serial.println(endTime - startTime);
#elif defined(UDP_GAP_TIME)
      Serial.print("Task Gap Time: ");
      Serial.println(millis() - startTime);
      TIMESTART;
#endif

      if ((xEventGroupGetBits(xADXLEvent) & ADXL_DONE_BIT) != 0) {
        //------------------------------accle data send process
#if defined(ADXL_UDP_SEND_ENABLE) || defined(ALL_UDP_SEND_ENABLE)
#ifdef SKIP_TO_SEND
        if (skip_times_cnt < SKIP_TIMES) {
          skip_times_cnt++;
        } else {
          udp.beginPacket(cfgValue.ipv4, cfgValue.port);
          udp.packetInit(0x1f);
          udp.write(prs_mixed_invt, MIXED_PRSBUF_SIZE);
          udp.endPacket();
        }
#else
        udp.beginPacket(cfgValue.ipv4, cfgValue.port);
        udp.packetInit(0x1f);
        udp.write(prs_mixed_invt, MIXED_PRSBUF_SIZE);
        udp.endPacket();
#endif
#endif
        //------------------------------accle data send process
      }
#ifdef SEND_TIMES_LIMIT
      if (++send_times_cnt == SKIP_TIMES + SEND_TIMES) vTaskSuspend(NULL);
#endif
      if (send_real_time) {
        continue;
      } else if (--cmd_send_times == 0) {
        // vTaskSuspend(NULL);
        vTaskDelay(100);
        esp_deep_sleep_start();
      }
    }
    vTaskDelay(1);
  }
}

void Tim0Interrupt() { xTaskNotifyGive(xADXLTask); }

void I2S0_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setFormat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM2_DMA_BUF_CNT, MTCM2_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  for (;;) {
    mtcm2.Read(&raw_samples_invt[MTCM1_BUFFER_SIZE], MTCM2_BUFFER_SIZE);
    xTaskNotify(xUDPSendTask, I2S0_DONE_BIT, eSetBits);
#ifdef MEMS_GAP_TIME
    Serial.print("MEMS Gap Time: ");
    Serial.println(millis() - startTime);
    TIMESTART;
#endif
  }
}

void I2S1_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setFormat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM1_DMA_BUF_CNT, MTCM1_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;) {
    mtcm1.Read(raw_samples_invt, MTCM1_BUFFER_SIZE);
    xTaskNotify(xUDPSendTask, I2S1_DONE_BIT, eSetBits);
  }
}

void ADXL_Task(void *param) {
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);

  ADXL345.init();
  ADXL345.setDataRate(ADXL345_DATA_RATE_1600);
  ADXL345.setRange(ADXL345_RANGE_2G);

  timerAlarmEnable(tim0);
  xEventGroupSetBits(xEventMTCM, ADXL_INIT_BIT);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    //-------------------------------accle data storage process
    xyzFloat raw = ADXL345.getRawValues();
    raw_adxl_invt[adxl_tims_cnt * 3 + 0] = raw.x;
    raw_adxl_invt[adxl_tims_cnt * 3 + 1] = raw.y;
    raw_adxl_invt[adxl_tims_cnt * 3 + 2] = raw.z;
    adxl_tims_cnt++;
    if (adxl_tims_cnt % 10 == 0) {
      raw_lm20_invt[adxl_tims_cnt / 10] = analogRead(ANALOG_PIN);
    }
    if (adxl_tims_cnt == ADXL_SAMPLE_CNT) {
      adxl_tims_cnt = 0;
      memcpy(prs_mixed_invt, raw_adxl_invt, ADXL_8BIT_SIZE);
      memcpy(prs_mixed_invt + ADXL_8BIT_SIZE, raw_lm20_invt, LM20_8BIT_SIZE);
      xEventGroupSetBits(xADXLEvent, ADXL_DONE_BIT);
#ifdef ADXL_GAP_TIME
      Serial.print("ADXL Gap Time: ");
      Serial.println(millis() - startTime);
      TIMESTART;
#endif
    }
    //-------------------------------accle data storage process
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("########################################");
  Serial.println("Now Version: " VERSION "\r\n");

  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, Tim0Interrupt, true);
  timerAlarmWrite(tim0, 1000, true);

  serialCmd.begin();
  if (touchRead(TOUCH_PAD) < TOUCH_THRESH || digitalRead(CONFIG_PIN) == 0) {
    serialCmd.cmdScanf();
  }
  serialCmd.~SerialCmd();
  cfgValue = serialCmd.cmdGetConfig(true);

  send_gap_time = cfgValue.gapTime;
  send_run_time = cfgValue.runTime;
  if (send_gap_time == 0 || send_run_time == 0) {
    send_real_time = true;
  }

  esp_sleep_enable_timer_wakeup(uS_TO_S * send_gap_time);

  xEventMTCM = xEventGroupCreate();
  xADXLEvent = xEventGroupCreate();

  xTaskCreatePinnedToCore(UDP_Send_Task, "UDP_Send_Task", 4096, NULL, 5,
                          &xUDPSendTask, 0);
  xTaskCreate(I2S0_Task, "I2S0_Task", 2048, NULL, 4, NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 2048, NULL, 4, NULL);
  xTaskCreate(ADXL_Task, "ADXL_Task", 4096, NULL, 3, &xADXLTask);
}

void loop() {
  xEventGroupSync(xEventMTCM, pdFALSE,
                  I2S0_INIT_BIT | I2S1_INIT_BIT | ADXL_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
  vTaskDelete(NULL);
}
