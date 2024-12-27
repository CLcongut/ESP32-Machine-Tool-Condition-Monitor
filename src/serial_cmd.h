#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "WiFiUdp.h"
#include <ESPAsyncWebServer.h>

#define SCANF_BUF_SIZE 256
#define CMD_CONFIG 8

#define GAPTIME_OPT_CNT 2
#define GAPTIME_OPT {0, 60}
#define RUNTIME_OPT_CNT 6
#define RUNTIME_OPT {0, 2, 5, 10, 20, 30}

#define StateLEDGLOW() (_ledpin < 0 ? (void)0 : digitalWrite(_ledpin, HIGH))
#define StateLEDFADE() (_ledpin < 0 ? (void)0 : digitalWrite(_ledpin, LOW))

#define UDP_CONSOLE_PORT 8080

extern Preferences cmdprefer;

typedef enum {
  MODE_NULL,
  MODE_UDP,
  MODE_SRL,
} ConsoleMode;

typedef struct {
  char cmd[24];
  uint8_t cmdidx;
} CommandIndex;

typedef struct {
  uint8_t gapTime;
  uint8_t runTime;
  char ssid[24];
  char pswd[24];
  char ipv4[16];
  uint16_t port;
  char url[100];
  bool update;
} ConfigValue;

class SerialCmd {
 private:
  Preferences cmdprefer;
  WiFiUDP udpClient;
  AsyncWebServer server{80};

  const CommandIndex c_cmdIndexAry[CMD_CONFIG];
  const uint8_t c_GAPTIME[GAPTIME_OPT_CNT] = GAPTIME_OPT;
  const uint8_t c_RUNTIME[RUNTIME_OPT_CNT] = RUNTIME_OPT;

  char _rcvCommand[SCANF_BUF_SIZE];
  ConfigValue _configValue;
  ConsoleMode _consoleMode;
  int _ledpin;
  char _frwversion[16];

  const char *htmlForm = R"rawliteral(
<!DOCTYPE HTML>
<html lang="zh">
<head>
    <meta charset="UTF-8">
    <title>电机检测器配置 当前固件版本: %VERSION%</title>
    <style>
        .version {
            font-size: smaller;  /* 设置版本号字体为较小 */
        }
    </style>
</head>
<body>
    <h1>电机检测器配置<br><span class="version">当前固件版本: %VERSION%</span></h1>
    <form action="/set" method="POST">
        <label>WiFi 名称:</label><br>
        <input type="text" name="ssid" value="%SSID%"><br>
        <label>WiFi 密码:</label><br>
        <input type="password" name="password" value="%PASSWORD%"><br>
        <label>UDP目标IP:</label><br>
        <input type="text" name="udp_ip" value="%UDP_IP%"><br>
        <label>UDP目标端口:</label><br>
        <input type="number" name="udp_port" value="%UDP_PORT%"><br>
        <label>待机时间:</label><br>
        <select name="gap_time">
            <option value="0" %GAP_TIME_0%>0s</option>
            <option value="60" %GAP_TIME_60%>60s</option>
        </select><br>
        <label>运行时间:</label><br>
        <select name="run_time">
            <option value="0" %RUN_TIME_0%>0s</option>
            <option value="2" %RUN_TIME_2%>2s</option>
            <option value="5" %RUN_TIME_5%>5s</option>
            <option value="10" %RUN_TIME_10%>10s</option>
            <option value="20" %RUN_TIME_20%>20s</option>
            <option value="30" %RUN_TIME_30%>30s</option>
        </select><br>
        <label>更新固件:</label><br>
        <input type="checkbox" name="update" value="1" %CHECKED%>是否更新<br>
        <label>OTA URL:</label><br>
        <input type="text" name="url" value="%URL%"><br>
        <input type="submit" value="提交">
    </form>
</body>
</html>
)rawliteral";

  const char *charGerl_1 = PSTR(" ...");
  const char *charGerl_2 = PSTR(" -> ");

  const char *charIncp_0_1 = PSTR(" -> Current Firmware Version: ");
  const char *charIncp_1_1 = PSTR(" -> Received: ");
  const char *charIncp_2_1 = PSTR(" -> Set GapTime: ");
  const char *charIncp_2_2 = PSTR(" -> Set RunTime: ");
  const char *charIncp_3_1 = PSTR(" -> Set WiFi SSID: ");
  const char *charIncp_3_2 = PSTR(" -> Set WiFi Password: ");
  const char *charIncp_4_1 = PSTR(" -> Set Target IP: ");
  const char *charIncp_4_2 = PSTR(" -> Set Target Port: ");
  const char *charIncp_5_1 = PSTR(" -> Set OTA URL: ");
  const char *charIncp_6_1 = PSTR(" -> Your Configs Are: ");
  const char *charIncp_6_2 = PSTR(" -> GapTime: ");
  const char *charIncp_6_3 = PSTR(" -> RunTime: ");
  const char *charIncp_6_4 = PSTR(" -> WiFi SSID: ");
  const char *charIncp_6_5 = PSTR(" -> WiFi Password: ");
  const char *charIncp_6_6 = PSTR(" -> Target IP: ");
  const char *charIncp_6_7 = PSTR(" -> Target Port: ");
  const char *charIncp_6_8 = PSTR(" -> OTA URL: ");

  const char *charInfo_1_1 = PSTR("\r\n -> Serial Console Activated!\r\n");
  const char *charInfo_1_2 = PSTR("\r\n -> ESP32 AP Console Activated!\r\n");
  const char *charInfo_1_3 =
      PSTR(" -> Turn To Update Mode\r\n -> When Close Console Will Update\r\n");
  const char *charInfo_1_4 = PSTR(" -> Turn To Normal Mode\r\n");
  const char *charInfo_1_5 = PSTR(" -> Console Closed!\r\n");
  const char *charInfo_2_1 = PSTR(" -> Set To Real-Time Mode!\r\n");
  const char *charInfo_2_2 = PSTR(" -> Rate Config Saved!\r\n");
  const char *charInfo_3_1 = PSTR(" -> WiFi Config Saved!\r\n");
  const char *charInfo_4_1 = PSTR(" -> UDP Config Saved!\r\n");
  const char *charInfo_5_1 = PSTR(" -> OTA URL Saved!\r\n");

  const char *charErro_1_1 = PSTR(
      "Invalid Command!\r\n -> Enter \"/help\" For More "
      "Info\r\n");
  const char *charErro_2_1 =
      PSTR("Invalid GapTime!\r\n -> Valid GapTime: 0, 60\r\n");
  const char *charErro_2_2 =
      PSTR("Invalid RunTime!\r\n -> Valid RunTime: 2, 5, 10, 20, 30\r\n");
  const char *charErro_4_1 =
      PSTR("Do Not Use LocalHost!\r\n -> Valid Format: xxx.xxx.xxx.xxx\r\n");
  const char *charErro_4_2 =
      PSTR("Invalid IP Format!\r\n -> Valid Format: xxx.xxx.xxx.xxx\r\n");
  const char *charErro_4_3 =
      PSTR("Invalid Port Value!\r\n -> Valid Port Value: 1-65535\r\n");

  const char *_helpInfo = PSTR(
      " -> Enter \"/setrate <GapTime> <RunTime>\" To Set Rate\r\n"
      " -> Enter \"/setwifi <SSID> <PSWD>\" To Set WiFi Config\r\n"
      " -> Enter \"/setudp <IPV4> <PORT>\" To Set UDP Traget\r\n"
      " -> Enter \"/setota <URL>\" To Set OTA URL\r\n"
      " -> Enter \"/help\" For More Info\r\n"
      " -> Enter \"/update\" For Update Firmware\r\n"
      " -> Enter \"/close\" To Close Console\r\n"
      " -> Enter \"/ver\" To Check Firmware Version\r\n");

 public:
  SerialCmd();
  ~SerialCmd();
  void begin();
  void setFrimwareVersion(const char *version);
#ifdef ARCHIVE
  size_t cmdSerialScanf();
  size_t cmdWifiudpScanf();
#endif
  size_t cmdGeneralScanf();
  uint8_t findIndex(char *cmd);
  void cmdSetRate(char *gaptime, char *runtime);
  void cmdSetWiFi(char *ssid, char *pswd);
  void cmdSetUDP(char *ipv4, char *port);
  void cmdSetOTA(char *url);
  ConfigValue cmdGetConfig(bool ifPrintInfo);
  uint16_t cmdGetPort();
  void consoleSetMode(ConsoleMode mode);
  void consoleSetStateLight(const int ledpin);
  void consoleSerialPrintf(const char *format, ...);
  void consoleUdpPrintf(const char *format, ...);
  void consoleGeneralPrintf(const char *format, ...);
  void consoleSerialPrintln(const char *str);
  void consoleUdpPrintln(const char *str);
  void consoleGeneralPrintln(const char *str);
  void consoleOnWeb();
};
