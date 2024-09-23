#include <Arduino.h>
#include "WIFI.h"
#include "I2S_93.h"
#include "mtcm_pins.h"

static TaskHandle_t xUDPTrasn = NULL;
static bool restart_flag = false;
static EventGroupHandle_t xEventMTCM = NULL;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

WiFiUDP udp;
IPAddress remote_IP(192, 168, 31, 199);
uint32_t remoteUdpPort = 6060;

int32_t *raw_samples_invt;
uint8_t *prs_samples_invt;

void UDPTask(void *param)
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
    // Serial.printf("Noti Val: %d\r\n",ulNotifuValue);
    if (ulNotifuValue == 2)
    {
      log_e("time1");
      ulTaskNotifyValueClear(xUDPTrasn, 0xFFFF);
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
      log_e("time4");

#ifdef BITS_32_TRANSMIT
      prs_samples_invt = (uint8_t *)raw_samples_invt;
      udp.beginPacket(remote_IP, remoteUdpPort);
      udp.write(prs_samples_invt, UDP_PACKAGE_NUM_32 * BYTE_PER_PACKAGE);
      udp.endPacket();
      log_e("time2");
      vTaskDelete(NULL);
#elif defined BITS_24_TRANSMIT
      // for (int i = 0; i < MTCM_SPLBUFF_ALL; i++)
      // {
      //   prs_samples_invt[i * 3] = raw_samples_invt[i] >> 24;
      //   prs_samples_invt[i * 3 + 1] = raw_samples_invt[i] >> 16;
      //   prs_samples_invt[i * 3 + 2] = raw_samples_invt[i] >> 8;
      // }
      // udp.beginPacket(remote_IP, remoteUdpPort);
      // udp.write(prs_samples_invt, UDP_PACKAGE_NUM_24 * BYTE_PER_PACKAGE);
      // udp.endPacket();
      // log_e("time3");
      vTaskDelete(NULL);
#endif
    }
    vTaskDelay(1);
  }
}

void I2S0_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S0_INIT_BIT);
  for (;;)
  {
    mtcm2.Read(&raw_samples_invt[MTCM1_SPBUF_SIZE], MTCM2_SPBUF_SIZE);
    xTaskNotifyGive(xUDPTrasn);
  }
}

void I2S1_Task(void *param)
{
  xEventGroupWaitBits(xEventMTCM, UDP_INIT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  xEventGroupSetBits(xEventMTCM, I2S1_INIT_BIT);
  for (;;)
  {
    mtcm1.Read(raw_samples_invt, MTCM1_SPBUF_SIZE);
    xTaskNotifyGive(xUDPTrasn);
  }
}

void setup()
{
  Serial.begin(115200);

  raw_samples_invt = (int32_t *)calloc(MTCM_SPLBUFF_ALL, sizeof(int32_t));
#ifdef BITS_24_TRANSMIT
  prs_samples_invt = (uint8_t *)calloc(MTCM_SPLBUFF_ALL * 3, sizeof(uint8_t));
#endif

  xEventMTCM = xEventGroupCreate();
  xTaskCreatePinnedToCore(UDPTask,"UDPTask",4096,NULL,5,&xUDPTrasn,0);
  xTaskCreate( I2S0_Task,"I2S0_Task",4096,NULL,1,NULL);
  xTaskCreate(I2S1_Task, "I2S1_Task", 4096, NULL, 1, NULL);
}

void loop()
{
  xEventGroupSync(xEventMTCM, pdFALSE, I2S0_INIT_BIT | I2S1_INIT_BIT, portMAX_DELAY);
  vEventGroupDelete(xEventMTCM);
#ifdef BITS_32_TRANSMIT
  log_w("32 Bit Data Output Mode!");
#elif defined BITS_24_TRANSMIT
  log_w("24 Bit Data Output Mode!");
#endif
  log_w("EventGroup Has Been Deleted!");
  log_w("Main Loop Will Be Deleted!");
  vTaskDelete(NULL);
}
