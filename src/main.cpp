#include <Arduino.h>
#include "WIFI.h"
#include "I2S_93.h"
#include "mtcm_pins.h"

static TaskHandle_t xUDPTrasn = NULL;
static bool restart_flag = false;

I2S_93 mtcm2(0, mtcm2.MASTER, mtcm2.RX, mtcm2.PDM);
// I2S_93 mtcm1(1, mtcm1.MASTER, mtcm1.RX, mtcm1.PDM);

WiFiUDP udp;
IPAddress remote_IP(192, 168, 31, 199);
uint32_t remoteUdpPort = 6060;

int16_t *samples_inventory_0;
uint8_t *samples_inventory_1;

void UDPTask(void *param)
{
  WiFi.mode(WIFI_STA);
  // WiFi.setSleep(false);
  // WiFi.begin(wifi_SSID, wifi_PSWD);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(200);
  // }
  // Serial.print("Connected, IP Address: ");
  // Serial.println(WiFi.localIP());
  // uint32_t ulNotifuValue = 0;

  // samples_inventory_1 = (uint8_t *)calloc(9000 * 4, sizeof(uint8_t));

  for (;;)
  {
    // Serial.println(ulNotifuValue);
    // ulNotifuValue = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    // ulNotifuValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    // Serial.println(ulNotifuValue);
    // if (ulNotifuValue == 2)
    // {
      // ulTaskNotifyValueClear(xUDPTrasn, 0xFFFF);
      // Serial.printf("UDP  Transmit Start:%d\r\n", millis());
#if 0
      for (uint32_t i = 0; i < 12000; i++)
      {
        samples_inventory_1[i * 3] = samples_inventory_0[i] >> 24;
        samples_inventory_1[i * 3 + 1] = samples_inventory_0[i] >> 16;
        samples_inventory_1[i * 3 + 2] = samples_inventory_0[i] >> 8;
      }
#endif
      // samples_inventory_1 = (uint8_t *)samples_inventory_0;
      // udp.beginPacket(remote_IP, remoteUdpPort);
#if 0
      for (uint32_t i = 0; i < 8000; i++)
      {
        udp.write(samples_inventory_0[i] >> 24);
        udp.write(samples_inventory_0[i] >> 16);
        udp.write(samples_inventory_0[i] >> 8);
        udp.write(samples_inventory_0[i]);
      }
#endif
#if 0
      size_t sent;
      do
      {
        sent = udp.write(samples_inventory_1, 8000);
      } while (!sent);
      do
      {
        udp.write(&samples_inventory_1[8000], 8000);
      } while (!sent);
      do
      {
        udp.write(&samples_inventory_1[16000], 8000);
      } while (!sent);
      do
      {
        udp.write(&samples_inventory_1[24000], 8000);
      } while (!sent);
#endif
#if 1
      // udp.write(samples_inventory_1, 26280); // 22500,26280:it was stable,18000:ez to calculate half the rate,17520:12 times of 1460
                                             // udp.write(&samples_inventory_1[12000], 12000);
                                             // udp.write(&samples_inventory_1[24000], 12000);
#endif
      // vTaskDelay(2);
      // udp.endPacket();
      // free(samples_inventory_1);
      // Serial.printf("UDP  Transmit END 1:%d\r\n", millis());
      // vTaskDelay(2);
      // udp.beginPacket(remote_IP, remoteUdpPort);
      // for (uint32_t i = 0; i < I2S1_SAMPLE_BUFFER_SIZE; i++)
      // {
      //   udp.write(samples_inventory_1[i] >> 24);
      //   udp.write(samples_inventory_1[i] >> 16);
      //   udp.write(samples_inventory_1[i] >> 8);
      //   udp.write(samples_inventory_1[i]);
      // }
      // vTaskDelay(2);
      // udp.endPacket();

      // Serial.printf("UDP  Transmit END 2:%d\r\n", millis());
    // }
    vTaskDelay(5);
  }
}

void I2S_0_Task(void *param)
{
  mtcm2.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm2.setformat(mtcm2.RIGHT_LEFT, mtcm2.PCM_SHORT);
  mtcm2.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm2.install(MTCM2_CLK_PIN, MTCM2_DIN_PIN);
  vTaskDelay(3000);
  for (;;)
  {
    mtcm2.Read(samples_inventory_0, MTCM2_SPBUF_SIZE);
    // xTaskNotifyGive(xUDPTrasn);
  }
}

#if 0
void I2S_1_Task(void *param)
{
  mtcm1.begin(MTCM_SAMPLE_RATE, MTCM_BPS);
  mtcm1.setformat(mtcm1.RIGHT_LEFT, mtcm1.PCM_SHORT);
  mtcm1.setDMABuffer(MTCM_DMA_BUF_CNT, MTCM_DMA_BUF_LEN);
  mtcm1.install(MTCM1_CLK_PIN, MTCM1_DIN_PIN);
  vTaskDelay(3000);
  for (;;)
  {
    mtcm1.Read(&samples_inventory_0[8000], MTCM1_SPBUF_SIZE);
    // xTaskNotifyGive(xUDPTrasn);
  }
}
#endif

void setup()
{
  Serial.begin(115200);
  samples_inventory_0 = (int16_t *)calloc(MTCM_SPLBUFF_ALL, sizeof(int16_t));
  xTaskCreatePinnedToCore(
      UDPTask,
      "UDPTask",
      4096,
      NULL,
      5,
      &xUDPTrasn,
      0);

  xTaskCreate(
      I2S_0_Task,
      "I2S_0_Task",
      4096,
      NULL,
      1,
      NULL);

  // xTaskCreate(
  //     I2S_1_Task,
  //     "I2S_1_Task",
  //     4096,
  //     NULL,
  //     1,
  //     NULL);
}

void loop()
{
}
