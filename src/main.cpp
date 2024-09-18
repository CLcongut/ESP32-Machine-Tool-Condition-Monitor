#include <Arduino.h>
#include "WIFI.h"
#include "I2S_93.h"
#include "mtcm_pins.h"

static TaskHandle_t xUDPTrasn = NULL;
static bool restart_flag = false;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PCM);
I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PCM);

WiFiUDP udp;
IPAddress remote_IP(192, 168, 31, 199);
uint32_t remoteUdpPort = 6060;

int32_t *samples_inventory_0;
uint8_t *samples_inventory_1;

void UDPTask(void *param)
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifi_SSID, wifi_PSWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
  }
  Serial.print("Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  uint32_t ulNotifuValue = 0;

  for (;;)
  {
    Serial.println(ulNotifuValue);
    ulNotifuValue = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    ulNotifuValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    Serial.println(ulNotifuValue);
    if (ulNotifuValue == 2)
    {
      ulTaskNotifyValueClear(xUDPTrasn, 0xFFFF);
      Serial.printf("UDP  Transmit Start:%d\r\n", millis());
#ifdef BITS_24_TRANSMIT
      for (int i = 0; i < MTCM_SPLBUFF_ALL; i++)
      {
        samples_inventory_1[i * 3] = samples_inventory_0[i] >> 24;
        samples_inventory_1[i * 3 + 1] = samples_inventory_0[i] >> 16;
        samples_inventory_1[i * 3 + 2] = samples_inventory_0[i] >> 8;
      }
      udp.beginPacket(remote_IP, remoteUdpPort);
      udp.write(samples_inventory_1, UDP_PACKAGE_NUM_24 * BYTE_PER_PACKAGE);
#endif

#ifdef BITS_32_TRANSMIT
      samples_inventory_1 = (uint8_t *)samples_inventory_0;
      udp.beginPacket(remote_IP, remoteUdpPort);
      udp.write(samples_inventory_1, UDP_PACKAGE_NUM_32 * BYTE_PER_PACKAGE);
#endif
    }
    vTaskDelay(5);
  }
}

void I2S0_Task(void *param)
{
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.I2S);
  mtcm2.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_WS_PIN, MTCM2_DIN_PIN);
  vTaskDelay(1000);
  for (;;)
  {
    mtcm2.Read(&samples_inventory_0[MTCM1_SPBUF_SIZE], MTCM2_SPBUF_SIZE);
    xTaskNotifyGive(xUDPTrasn);
  }
}

void I2S1_Task(void *param)
{
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.ONLY_RIGHT, mtcm1.I2S);
  mtcm1.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_WS_PIN, MTCM1_DIN_PIN);
  vTaskDelay(1000);
  for (;;)
  {
    mtcm1.Read(samples_inventory_0, MTCM1_SPBUF_SIZE);
    xTaskNotifyGive(xUDPTrasn);
  }
}

void setup()
{
  Serial.begin(115200);
  samples_inventory_0 = (int32_t *)calloc(MTCM_SPLBUFF_ALL, sizeof(int32_t));
#ifdef BITS_24_TRANSMIT
  samples_inventory_1 = (uint8_t *)calloc(MTCM_SPLBUFF_ALL * 3, sizeof(uint8_t));
#endif
  xTaskCreatePinnedToCore(
      UDPTask,
      "UDPTask",
      4096,
      NULL,
      5,
      &xUDPTrasn,
      0);

  xTaskCreate(
      I2S0_Task,
      "I2S0_Task",
      4096,
      NULL,
      1,
      NULL);

  xTaskCreate(
      I2S1_Task,
      "I2S1_Task",
      4096,
      NULL,
      1,
      NULL);
}

void loop()
{
}
