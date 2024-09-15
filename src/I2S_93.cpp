/******************************************
 * version:2.0
 * change mode setting
 * add Constructor
 * can use PDM now
 * change name to I2S_93
 * more simple for my projct
 * use esp_log to print info
 * rebuild whole lib
 * version:2.1
 * remove namespace
 * add some note for function
 * version:2.2
 * fix install pdm mode
 * log_i to log_e
 * correct format to format
 *
 * ver: 2.3 plan:
 * next update need to add more error log
 * and way to defence wrong parameter
 * pdm only for i2s 0
 */
#include "I2S_93.h"

I2S_93::I2S_93(uint8_t deviceIndex, i2smode_t peripheralActor, i2smode_t transmitMode, i2smode_t modulateMode) : _deviceIndex((i2s_port_t)deviceIndex),
                                                                                                                 _transmitMode(transmitMode),
                                                                                                                 _modulateMode(modulateMode),
                                                                                                                 _intrAlloc(0),
                                                                                                                 _dmaBufCnt(16),
                                                                                                                 _dmaBufLen(64),
                                                                                                                 _useApll(false)
{
  _i2sdvsMode = i2s_mode_t(peripheralActor | transmitMode | modulateMode);
}

void I2S_93::begin(uint32_t sampleRate, uint8_t bitsPerSample)
{
  _sampleRate = sampleRate;
  _bitsPerSample = (i2s_bits_per_sample_t)bitsPerSample;
}

void I2S_93::setformat(i2schnformat_t channelformat, i2scommformat_t commonformat)
{
  _channelformat = (i2s_channel_fmt_t)channelformat;
  _commonformat = (i2s_comm_format_t)commonformat;
}

void I2S_93::setIntrAllocFlags(uint8_t intrAlloc)
{
  _intrAlloc = intrAlloc;
}

void I2S_93::setDMABuffer(int dmaBufCnt, int dmaBufLen)
{
  _dmaBufCnt = dmaBufCnt;
  _dmaBufLen = dmaBufLen;
}

void I2S_93::install(int bckPin, int wsPin, int dataPin)
{
  i2s_config_t i2s_config = {
      .mode = _i2sdvsMode,
      .sample_rate = _sampleRate,
      .bits_per_sample = _bitsPerSample,
      .channel_format = _channelformat,
      .communication_format = _commonformat,
      .intr_alloc_flags = _intrAlloc,
      .dma_buf_count = _dmaBufCnt,
      .dma_buf_len = _dmaBufLen,
      .use_apll = _useApll};

  i2s_pin_config_t pin_config;
  switch (_transmitMode | _modulateMode)
  {
  case RX | PCM:
    pin_config = {
        .bck_io_num = bckPin,
        .ws_io_num = wsPin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = dataPin};
    break;

  case RX | PDM:
    pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = bckPin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = dataPin};
    break;

  case TX | PCM:
    pin_config = {
        .bck_io_num = bckPin,
        .ws_io_num = wsPin,
        .data_out_num = dataPin,
        .data_in_num = I2S_PIN_NO_CHANGE};
    break;

  case TX | PDM:
    pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = bckPin,
        .data_out_num = dataPin,
        .data_in_num = I2S_PIN_NO_CHANGE};
    break;
  }
  if (ESP_OK != i2s_driver_install(_deviceIndex, &i2s_config, 0, NULL))
  {
    log_e("I2S install failed!");
    return;
  }
  if (ESP_OK != i2s_set_pin(_deviceIndex, &pin_config))
  {
    log_e("I2S set pins failed!");
    return;
  }
  log_e("I2S Successfully installed!");
}

void I2S_93::install(int bckPin, int dataPin)
{
  I2S_93::install(I2S_PIN_NO_CHANGE, bckPin, dataPin);
}

size_t I2S_93::Read(void *storageAddr, int sampleSize)
{
  size_t recvSize;
  i2s_read(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8, &recvSize, portMAX_DELAY);
  return recvSize;
}

size_t I2S_93::Write(void *storageAddr, int sampleSize)
{
  size_t sendSize;
  i2s_write(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8, &sendSize, portMAX_DELAY);
  return sendSize;
}

void I2S_93::End()
{
  i2s_driver_uninstall(_deviceIndex);
}