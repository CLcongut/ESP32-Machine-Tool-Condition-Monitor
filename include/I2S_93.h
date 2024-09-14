#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esp_i2s
{
#include "driver/i2s.h"
}

class I2S_93
{
public:
  typedef enum
  {
    MASTER = (0x1 << 0), /*!< Master mode*/
    SLAVE = (0x1 << 1),  /*!< Slave mode*/
    TX = (0x1 << 2),     /*!< TX mode*/
    RX = (0x1 << 3),     /*!< RX mode*/
    PDM = (0x1 << 6),    /*!< I2S PDM mode*/
    PCM = 0,             /*!< Default PCM mode*/
  } i2smode_t;

  typedef enum
  {
    RIGHT_LEFT, /*!< Separated left and right channel */
    ALL_RIGHT,  /*!< Load right channel data in both two channels */
    ALL_LEFT,   /*!< Load left channel data in both two channels */
    ONLY_RIGHT, /*!< Only load data in right channel (mono mode) */
    ONLY_LEFT,  /*!< Only load data in left channel (mono mode) */
  } i2schnformate_t;

  typedef enum
  {
    I2S = 0X01,       /*!< I2S communication I2S Philips standard, data launch at second BCK*/
    MSB = 0X02,       /*!< I2S communication MSB alignment standard, data launch at first BCK*/
    PCM_SHORT = 0x04, /*!< PCM Short standard, also known as DSP mode. The period of synchronization signal (WS) is 1 bck cycle.*/
    PCM_LONG = 0x0C,  /*!< PCM Long standard. The period of synchronization signal (WS) is channel_bit*bck cycles.*/
    MAX,              /*!< standard max*/
  } i2scommformat_t;

  
  /**
   * @brief Construct a new i2s_93 objects. Setting use which i2s and i2s mode.
   *
   * @param deviceIndex 0 or 1
   * @param peripheralActor MASTER or SLAVE
   * @param transmitMode TX or RX
   * @param modulateMode PCM or PDM
   */
  I2S_93(uint8_t deviceIndex, i2smode_t peripheralActor, i2smode_t transmitMode, i2smode_t modulateMode);
  /**
   * @brief Set sample rate and bps.
   *
   * @param sampleRate
   * @param bitsPerSample Can use 8, 16, 24, 32.
   */
  void begin(uint32_t sampleRate, uint8_t bitsPerSample);
  /**
   * @brief Set two of the formates.
   *
   * @param channelFormate View i2schnformate_t enum
   * @param commonFormate View i2scommformat_t enum
   */
  void setFormate(i2schnformate_t channelFormate, i2scommformat_t commonFormate);
  /**
   * @brief Set the Intrrupt Alloc Flag.
   *
   * @param intrAlloc 0~7 defult: 0.
   */
  void setIntrAllocFlags(uint8_t intrAlloc);
  /**
   * @brief Set the I2S DMA buffer
   *
   * @param dmaBufCnt
   * @param dmaBufLen
   */
  void setDMABuffer(int dmaBufCnt, int dmaBufLen);
  /**
   * @brief Install i2s with PCM mode
   * 
   * @param bckPin 
   * @param wsPin 
   * @param dataPin 
   */
  void install(int bckPin, int wsPin, int dataPin);
  /**
   * @brief Install i2s with PDM mode
   *
   * @param bckPin
   * @param dataPin
   */
  void install(int bckPin, int dataPin);

  size_t Read(void *storageAddr, int sampleSize);

  size_t Write(void *storageAddr, int sampleSize);

  void End();

#if 0
  void SetSampleRate(uint32_t sampleRate);
  void SetChannelFormat(ChannelFormat format);
  void SetDMABuffer(int cnt, int len);
  void SetIntrAllocFlags(uint8_t interrupt);
  void SetTxAutoClear(bool auto_cl_tx);
  bool SetInputMode(int bckPin, int wsPin, int dinPin);
  bool SetOutputMode(int bckPin, int wsPin, int douPin);
  bool SetInOutputMode(int bckPin, int wsPin, int dinPin, int douPin);

#endif

private:
  esp_i2s::i2s_port_t _deviceIndex;
  esp_i2s::i2s_mode_t _i2sdvsMode;
  i2smode_t _transmitMode;
  i2smode_t _modulateMode;
  uint32_t _sampleRate;
  esp_i2s::i2s_bits_per_sample_t _bitsPerSample;
  esp_i2s::i2s_channel_fmt_t _channelFormate;
  esp_i2s::i2s_comm_format_t _commonFormate;
  uint8_t _intrAlloc;
  int _dmaBufCnt;
  int _dmaBufLen;
  bool _useApll;

#if 0
  esp_i2s::i2s_port_t _i2s_port;
  esp_i2s::i2s_bits_per_sample_t _BPS;
  uint32_t _sampleRate = 44100;
  bool _mode_set = true;
  esp_i2s::i2s_channel_fmt_t _format = esp_i2s::I2S_CHANNEL_FMT_RIGHT_LEFT;
  int _buffer_cnt = 16;
  int _buffer_len = 64;
  uint8_t _interrupt = 0;
  bool _auto_cl_tx = false;
#endif
};