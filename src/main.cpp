#include <Arduino.h>
#include <driver/i2s.h>
#include <WIFI.h>
// #include "myWiFiUdp.cpp"

#define wifi_SSID "CCongut"
#define wifi_PSWD "88888888"
// #define wifi_SSID "CLcongut"
// #define wifi_PSWD "88888888"

#define I2S0_SAMPLE_BUFFER_SIZE 8000
#define I2S1_SAMPLE_BUFFER_SIZE 4000
#define ALLS_SAMPLE_BUFFER_SIZE 12000
#define SAMPLE_RATE 40000
// #define I2S_MIC_SERIAL_CLOCK GPIO_NUM_27
// #define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_26
// #define I2S_MIC_SERIAL_DATA GPIO_NUM_4

#define UDP_PACKAGE_SIZE 1000

#define sign_LED 2

static TaskHandle_t xUDPTrasn = NULL;
static bool restart_flag = false;

#ifdef TIMEREN
hw_timer_t *tim0_once = NULL;
#endif

WiFiUDP udp;
IPAddress remote_IP(192, 168, 31, 199);
// IPAddress remote_IP(192, 168, 22, 172);
uint32_t remoteUdpPort = 6060;

void I2S_0_Init()
{
  i2s_config_t i2s_config_0 = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024, // dma buff max in :8 & 1024
      .use_apll = false,
  };

  i2s_pin_config_t i2s_mic_pins_0 = {
      .bck_io_num = GPIO_NUM_27,
      .ws_io_num = GPIO_NUM_26,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = GPIO_NUM_25,
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config_0, 0, NULL) == ESP_OK)
  {
    Serial.println("I2S 0 Installed!");
  }
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins_0);
}

void I2S_1_Init()
{
  i2s_config_t i2s_config_1 = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024, // dma buff max in :8 & 1024
      .use_apll = false,
  };

  i2s_pin_config_t i2s_mic_pins_1 = {
      .bck_io_num = GPIO_NUM_33,
      .ws_io_num = GPIO_NUM_32,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = GPIO_NUM_35,
  };
  if (i2s_driver_install(I2S_NUM_1, &i2s_config_1, 0, NULL) == ESP_OK)
  {
    Serial.println("I2S 0 Installed!");
  }
  i2s_set_pin(I2S_NUM_1, &i2s_mic_pins_1);
}

int32_t *samples_inventory_0;
// int32_t *samples_inventory_1;
uint8_t *samples_inventory_1;

#ifdef TIMEREN
void Tim0Interrupt()
{
  restart_flag = true;
}
#endif

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

  samples_inventory_1 = (uint8_t *)calloc(9000 * 4, sizeof(uint8_t));

  for (;;)
  {
    // Serial.println(ulNotifuValue);
    // ulNotifuValue = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    // ulNotifuValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xTaskNotifyWait(0x00, 0x00, &ulNotifuValue, portMAX_DELAY);
    // Serial.println(ulNotifuValue);
    if (ulNotifuValue == 2)
    {
      ulTaskNotifyValueClear(xUDPTrasn, 0xFFFF);
      // Serial.printf("UDP  Transmit Start:%d\r\n", millis());

      for (uint32_t i = 0; i < 12000; i++)
      {
        samples_inventory_1[i * 3] = samples_inventory_0[i] >> 24;
        samples_inventory_1[i * 3 + 1] = samples_inventory_0[i] >> 16;
        samples_inventory_1[i * 3 + 2] = samples_inventory_0[i] >> 8;
      }
      // samples_inventory_1 = (uint8_t *)samples_inventory_0;
      udp.beginPacket(remote_IP, remoteUdpPort);
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
      udp.write(samples_inventory_1, 26280); // 22500,26280:it was stable,18000:ez to calculate half the rate,17520:12 times of 1460
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
    }
    vTaskDelay(5);
  }
}

void I2S_0_Task(void *param)
{
  // vTaskDelay(1000);
  I2S_0_Init();
  // samples_inventory_0 = (int32_t *)calloc(I2S0_SAMPLE_BUFFER_SIZE, sizeof(int32_t));
  vTaskDelay(3000);
  for (;;)
  {
    size_t bytes_read_1 = 0;
    // Serial.printf("I2S 0 Collect Start:%d\r\n", millis());
    i2s_read(I2S_NUM_0, samples_inventory_0, sizeof(int32_t) * I2S0_SAMPLE_BUFFER_SIZE, &bytes_read_1, portMAX_DELAY);
    // Serial.printf("I2S 0 Collect   End:%d\r\n", millis());
    // xTaskNotifyGive(xUDPTrasn);
    // vTaskDelay(1000);
  }
}

void I2S_1_Task(void *param)
{
  // vTaskDelay(1000);
  I2S_1_Init();
  // samples_inventory_1 = (int32_t *)calloc(I2S1_SAMPLE_BUFFER_SIZE, sizeof(int32_t));
  vTaskDelay(3000);
  for (;;)
  {
    size_t bytes_read_1 = 0;
    // Serial.printf("I2S 1 Collect Start:%d\r\n", millis());
    i2s_read(I2S_NUM_1, &samples_inventory_0[8000], sizeof(int32_t) * I2S1_SAMPLE_BUFFER_SIZE, &bytes_read_1, portMAX_DELAY);
    // Serial.printf("I2S 1 Collect   End:%d\r\n", millis());
    xTaskNotifyGive(xUDPTrasn);
    // vTaskDelay(500);
  }
}

void setup()
{
  Serial.begin(115200);
#ifdef TIMEREN
  tim0_once = timerBegin(0, 240, true);
  timerAttachInterrupt(tim0_once, Tim0Interrupt, true);
  timerAlarmWrite(tim0_once, 100000, false);
#endif
  samples_inventory_0 = (int32_t *)calloc(I2S0_SAMPLE_BUFFER_SIZE, sizeof(int32_t));

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

  xTaskCreate(
      I2S_1_Task,
      "I2S_1_Task",
      4096,
      NULL,
      1,
      NULL);

  // delay(1000);
  // timerAlarmEnable(tim0_once);
}

void loop()
{
}
