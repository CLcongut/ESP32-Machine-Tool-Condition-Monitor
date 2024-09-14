/******************************************
 * version:2.0
 * change mode setting
 * add Constructor
 * can use PDM now
 * change name to I2S_93
 * more simple for my projct
 * use esp_log to print info
 * rebuild whole lib
 */
#include "I2S_93.h"

I2S_93::I2S_93(uint8_t deviceIndex, i2smode_t peripheralActor, i2smode_t transmitMode, i2smode_t modulateMode) : _deviceIndex((esp_i2s::i2s_port_t)deviceIndex),
                                                                                                                 _transmitMode(transmitMode),
                                                                                                                 _modulateMode(modulateMode),
                                                                                                                 _intrAlloc(0),
                                                                                                                 _dmaBufCnt(16),
                                                                                                                 _dmaBufLen(64),
                                                                                                                 _useApll(false)
{
  _i2sdvsMode = esp_i2s::i2s_mode_t(peripheralActor | transmitMode | modulateMode);
}

void I2S_93::begin(uint32_t sampleRate, uint8_t bitsPerSample)
{
  _sampleRate = sampleRate;
  _bitsPerSample = (esp_i2s::i2s_bits_per_sample_t)bitsPerSample;
}

void I2S_93::setFormate(i2schnformate_t channelFormate, i2scommformat_t commonFormate)
{
  _channelFormate = (esp_i2s::i2s_channel_fmt_t)channelFormate;
  _commonFormate = (esp_i2s::i2s_comm_format_t)commonFormate;
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
  esp_i2s::i2s_config_t i2s_config = {
      .mode = _i2sdvsMode,
      .sample_rate = _sampleRate,
      .bits_per_sample = _bitsPerSample,
      .channel_format = _channelFormate,
      .communication_format = _commonFormate,
      .intr_alloc_flags = _intrAlloc,
      .dma_buf_count = _dmaBufCnt,
      .dma_buf_len = _dmaBufLen,
      .use_apll = _useApll};

  esp_i2s::i2s_pin_config_t pin_config;
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
  log_i("I2S Successfully installed!\r\n");
}

void I2S_93::install(int bckPin, int dataPin)
{
  I2S_93::install(bckPin, I2S_PIN_NO_CHANGE, dataPin);
}

size_t I2S_93::Read(void *storageAddr, int sampleSize)
{
  size_t recvSize;
  esp_i2s::i2s_read(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8, &recvSize, portMAX_DELAY);
  return recvSize;
}

size_t I2S_93::Write(void *storageAddr, int sampleSize)
{
  size_t sendSize;
  esp_i2s::i2s_write(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8, &sendSize, portMAX_DELAY);
  return sendSize;
}

void I2S_93::End()
{
  esp_i2s::i2s_driver_uninstall(_deviceIndex);
}

#if 0
void I2S_93::begin(uint8_t i2s_port, uint8_t BPS)
{
  switch (i2s_port)
  {
  case 0:
    _i2s_port = esp_i2s::I2S_NUM_0;
    break;

  case 1:
    _i2s_port = esp_i2s::I2S_NUM_1;
    break;

  default:
    Serial.println("error i2s port set");
    break;
  }

  switch (BPS)
  {
  case 8:
    _BPS = I2S_BITS_PER_SAMPLE_8BIT;
    break;

  case 16:
    _BPS = I2S_BITS_PER_SAMPLE_16BIT;
    break;

  case 24:
    _BPS = I2S_BITS_PER_SAMPLE_24BIT;
    break;

  case 32:
    _BPS = I2S_BITS_PER_SAMPLE_32BIT;
    break;

  default:
    Serial.println("error BPS set");
    break;
  }
}

void I2S_93::SetSampleRate(uint32_t sampleRate)
{
  _sampleRate = sampleRate;
}

void I2S_93::SetChannelFormat(ChannelFormat format)
{
  switch (format)
  {
  case RIGHT_LEFT:
    _format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    break;

  case ALL_RIGHT:
    _format = I2S_CHANNEL_FMT_ALL_RIGHT;
    break;

  case ALL_LEFT:
    _format = I2S_CHANNEL_FMT_ALL_LEFT;
    break;

  case ONLY_RIGHT:
    _format = I2S_CHANNEL_FMT_ONLY_RIGHT;
    break;

  case ONLY_LEFT:
    _format = I2S_CHANNEL_FMT_ONLY_LEFT;
    break;

  default:
    Serial.println("error Channel Format");
    break;
  }
}

void I2S_93::SetDMABuffer(int cnt, int len)
{
  _buffer_cnt = cnt;
  _buffer_len = len;
}

void I2S_93::SetIntrAllocFlags(uint8_t interrupt)
{
  switch (interrupt)
  {
  case 0:
    _interrupt = 0;
    break;
  case 1:
    _interrupt = ESP_INTR_FLAG_LEVEL1;
    break;
  case 2:
    _interrupt = ESP_INTR_FLAG_LEVEL2;
    break;
  case 3:
    _interrupt = ESP_INTR_FLAG_LEVEL3;
    break;
  case 4:
    _interrupt = ESP_INTR_FLAG_LEVEL4;
    break;
  case 5:
    _interrupt = ESP_INTR_FLAG_LEVEL5;
    break;
  case 6:
    _interrupt = ESP_INTR_FLAG_LEVEL6;
    break;
  case 7:
    _interrupt = ESP_INTR_FLAG_NMI;
    break;

  default:
    Serial.println("error Intr Alloc Flags");
    break;
  }
}

void I2S_93::SetTxAutoClear(bool auto_cl_tx)
{
  _auto_cl_tx = auto_cl_tx;
}

bool I2S_93::SetInputMode(int bckPin, int wsPin, int dinPin)
{
  if (_mode_set)
  {
    _mode_set = false;
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = _sampleRate,
        .bits_per_sample = _BPS,
        .channel_format = _format,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = _interrupt,
        .dma_buf_count = _buffer_cnt,
        .dma_buf_len = _buffer_len,
        .use_apll = false};

    i2s_pin_config_t pin_config = {
        .bck_io_num = bckPin,
        .ws_io_num = wsPin,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = dinPin};

    if (ESP_OK != i2s_driver_install(_i2s_port, &i2s_config, 0, NULL))
    {
      Serial.println("install i2s driver failed");
      return false;
    }
    if (ESP_OK != i2s_set_pin(_i2s_port, &pin_config))
    {
      Serial.println("i2s set pin failed");
      return false;
    }
    Serial.println("i2s set input mode");
    return true;
  }
  else
  {
    Serial.println("you can only set mode once");
    return false;
  }
}

bool I2S_93::SetOutputMode(int bckPin, int wsPin, int douPin)
{
  if (_mode_set)
  {
    _mode_set = false;
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = _sampleRate,
        .bits_per_sample = _BPS,
        .channel_format = _format,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = _interrupt,
        .dma_buf_count = _buffer_cnt,
        .dma_buf_len = _buffer_len,
        .use_apll = false,
        .tx_desc_auto_clear = _auto_cl_tx};

    i2s_pin_config_t pin_config = {
        .bck_io_num = bckPin,
        .ws_io_num = wsPin,
        .data_out_num = douPin,
        .data_in_num = I2S_PIN_NO_CHANGE};

    if (ESP_OK != i2s_driver_install(_i2s_port, &i2s_config, 0, NULL))
    {
      Serial.println("install i2s driver failed");
      return false;
    }
    if (ESP_OK != i2s_set_pin(_i2s_port, &pin_config))
    {
      Serial.println("i2s set pin failed");
      return false;
    }
    Serial.println("i2s set output mode");
    return true;
  }
  else
  {
    Serial.println("you can only set mode once");
    return false;
  }
}

bool I2S_93::SetInOutputMode(int bckPin, int wsPin, int dinPin, int douPin)
{
  if (_mode_set)
  {
    _mode_set = false;
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = _sampleRate,
        .bits_per_sample = _BPS,
        .channel_format = _format,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = _interrupt,
        .dma_buf_count = _buffer_cnt,
        .dma_buf_len = _buffer_len,
        .use_apll = false,
        .tx_desc_auto_clear = _auto_cl_tx};

    i2s_pin_config_t pin_config = {
        .bck_io_num = bckPin,
        .ws_io_num = wsPin,
        .data_out_num = douPin,
        .data_in_num = dinPin};

    if (ESP_OK != i2s_driver_install(_i2s_port, &i2s_config, 0, NULL))
    {
      Serial.println("install i2s driver failed");
      return false;
    }
    if (ESP_OK != i2s_set_pin(_i2s_port, &pin_config))
    {
      Serial.println("i2s set pin failed");
      return false;
    }
    Serial.println("i2s set inoutput mode");
    return true;
  }
  else
  {
    Serial.println("you can only set mode once");
    return false;
  }
}


#endif