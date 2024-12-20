#include "cl_i2s_lib.h"

CL_I2S_LIB::CL_I2S_LIB(uint8_t deviceIndex, i2smode_t peripheralActor,
                       i2smode_t transmitMode, i2smode_t modulateMode)
    : _deviceIndex((i2s_port_t)deviceIndex),
      _transmitMode(transmitMode),
      _modulateMode(modulateMode),
      _intrAlloc(0),
      _dmaBufCnt(16),
      _dmaBufLen(64),
      _useApll(true),
      _mclkPin(I2S_PIN_NO_CHANGE) {
  _i2sdvsMode = i2s_mode_t(peripheralActor | transmitMode | modulateMode);
}

void CL_I2S_LIB::begin(uint32_t sampleRate, uint8_t bitsPerSample) {
  _sampleRate = sampleRate;
  _bitsPerSample = (i2s_bits_per_sample_t)bitsPerSample;
}

void CL_I2S_LIB::setFormat(i2schnformat_t channelformat,
                           i2scommformat_t commonformat) {
  _channelformat = (i2s_channel_fmt_t)channelformat;
  _commonformat = (i2s_comm_format_t)commonformat;
}

void CL_I2S_LIB::setIntrAllocFlags(uint8_t intrAlloc) {
  _intrAlloc = intrAlloc;
}

void CL_I2S_LIB::setDMABuffer(int dmaBufCnt, int dmaBufLen) {
  _dmaBufCnt = dmaBufCnt;
  _dmaBufLen = dmaBufLen;
}

void CL_I2S_LIB::setPinMCLK(int mclkPin) { _mclkPin = mclkPin; }

void CL_I2S_LIB::install(int bckPin, int wsPin, int dataPin) {
  i2s_config_t i2s_config = {.mode = _i2sdvsMode,
                             .sample_rate = _sampleRate,
                             .bits_per_sample = _bitsPerSample,
                             .channel_format = _channelformat,
                             .communication_format = _commonformat,
                             .intr_alloc_flags = _intrAlloc,
                             .dma_buf_count = _dmaBufCnt,
                             .dma_buf_len = _dmaBufLen,
                             .use_apll = _useApll};

  i2s_pin_config_t pin_config;
  switch (_transmitMode | _modulateMode) {
    case RX | PCM:
      pin_config = {.mck_io_num = _mclkPin,
                    .bck_io_num = bckPin,
                    .ws_io_num = wsPin,
                    .data_out_num = I2S_PIN_NO_CHANGE,
                    .data_in_num = dataPin};
      break;

    case RX | PDM:
      pin_config = {.mck_io_num = _mclkPin,
                    .bck_io_num = I2S_PIN_NO_CHANGE,
                    .ws_io_num = bckPin,
                    .data_out_num = I2S_PIN_NO_CHANGE,
                    .data_in_num = dataPin};
      break;

    case TX | PCM:
      pin_config = {.mck_io_num = _mclkPin,
                    .bck_io_num = bckPin,
                    .ws_io_num = wsPin,
                    .data_out_num = dataPin,
                    .data_in_num = I2S_PIN_NO_CHANGE};
      break;

    case TX | PDM:
      pin_config = {.mck_io_num = _mclkPin,
                    .bck_io_num = I2S_PIN_NO_CHANGE,
                    .ws_io_num = bckPin,
                    .data_out_num = dataPin,
                    .data_in_num = I2S_PIN_NO_CHANGE};
      break;
  }
  if (ESP_OK != i2s_driver_install(_deviceIndex, &i2s_config, 0, NULL)) {
    log_e("I2S install failed!");
    return;
  }
  if (ESP_OK != i2s_set_pin(_deviceIndex, &pin_config)) {
    log_e("I2S set pins failed!");
    return;
  }
  log_i("I2S Successfully installed!");
}

void CL_I2S_LIB::install(int bckPin, int dataPin) {
  CL_I2S_LIB::install(I2S_PIN_NO_CHANGE, bckPin, dataPin);
}

void CL_I2S_LIB::startI2S() { i2s_start(_deviceIndex); }

void CL_I2S_LIB::stopI2S() { i2s_stop(_deviceIndex); }

size_t CL_I2S_LIB::Read(void *storageAddr, int sampleSize) {
  size_t recvSize;
  i2s_read(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8,
           &recvSize, portMAX_DELAY);
  return recvSize;
}

size_t CL_I2S_LIB::Write(void *storageAddr, int sampleSize) {
  size_t sendSize;
  i2s_write(_deviceIndex, storageAddr, sampleSize * _bitsPerSample / 8,
            &sendSize, portMAX_DELAY);
  return sendSize;
}

void CL_I2S_LIB::End() { i2s_driver_uninstall(_deviceIndex); }