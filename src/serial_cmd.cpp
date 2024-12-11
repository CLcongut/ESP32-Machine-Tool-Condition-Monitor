#include "serial_cmd.h"

Preferences cmdprefer;

SerialCmd::SerialCmd()
    : c_cmdIndexAry{{"/setRate", 1}, {"/setWiFi", 2}, {"/setUDP", 3},
                    {"/setOTA", 4},  {"/help", 5},    {"/update", 6},
                    {"/close", 7}} {}

SerialCmd::~SerialCmd() { cmdprefer.~Preferences(); }

uint8_t SerialCmd::findIndex(char *cmd) {
  for (size_t i = 0; i < CMD_CONFIG; i++) {
    if (strcmp(c_cmdIndexAry[i].cmd, cmd) == 0) {
      return c_cmdIndexAry[i].cmdidx;
    }
  }
  return 0;
}

void SerialCmd::begin() {
  cmdprefer.begin("Configs");
#ifdef ARCHIVE
  _configValue.gapTime = cmdprefer.getUChar("GapTime");
  _configValue.runTime = cmdprefer.getUChar("RunTime");
  strcpy(_configValue.ipv4, cmdprefer.getString("IPV4").c_str());
  _configValue.port = cmdprefer.getUShort("Port");
#endif
  strcpy(_configValue.ssid, cmdprefer.getString("SSID").c_str());
  strcpy(_configValue.pswd, cmdprefer.getString("PSWD").c_str());
  strcpy(_configValue.url, cmdprefer.getString("URL").c_str());
  cmdprefer.end();
  _configValue.update = false;
}

size_t SerialCmd::cmdScanf() {
  Serial.println("\r\n -> Console Activated!\r\n");
  while (true) {
    size_t cmdLen = Serial.available();
    if (cmdLen) {
      size_t index = 0;
      char cmdHead[24];
      char value1[100];
      char value2[24];
      for (;;) {
        if (index < cmdLen) {
          _rcvCommand[index++] = Serial.read();
        } else {
          _rcvCommand[index] = '\0';
          Serial.print("Rcv: ");
          Serial.println(_rcvCommand);
          if (_rcvCommand[0] == '/') {
            sscanf(_rcvCommand, "%23s %99s %23s", cmdHead, value1, value2);
            switch (findIndex(cmdHead)) {
              case 1:
                // cmdSetRate(value1, value2);
                break;
              case 2:
                cmdSetWiFi(value1, value2);
                break;
              case 3:
                // cmdSetUDP(value1, value2);
                break;
              case 4:
                cmdSetOTA(value1);
                break;
              case 5:
                cmdGetHelp();
                break;
              case 6:
                _configValue.update = !_configValue.update;
                if (_configValue.update) {
                  Serial.print(" -> Turn To Update Mode\r\n");
                  Serial.println(" -> When Close Console Will Update\r\n");
                } else {
                  Serial.println(" -> Turn To Normal Mode\r\n");
                }
                break;
              case 7:
                Serial.println(" -> Console Closed!\r\n");
                return 0;
                break;
              default:
                log_e(
                    "Invalid Command!\r\n -> Enter \"/help\" For More "
                    "Info\r\n");
                break;
            }
          }
          break;
        }
      }
    }
  }
  return 0;
}

void SerialCmd::cmdGetHelp() {
  Serial.println(" -> Enter \"/setRate <gapTime> <runTime>\" To Set Rate");
  Serial.println(" -> Enter \"/setWiFi <SSID> <PSWD>\" To Set WiFi Config");
  Serial.println(" -> Enter \"/setUDP <IPV4> <PORT>\" To Set UDP Traget");
  Serial.println(" -> Enter \"/setOTA <URL>\" To Set OTA URL");
  Serial.println(" -> Enter \"/help\" For More Info");
  Serial.println(" -> Enter \"/update\" For Update Firmware");
  Serial.println(" -> Enter \"/close\" To Close Console\r\n");
}

#ifdef ARCHIVE
void SerialCmd::cmdSetRate(char *gaptime, char *runtime) {
  uint8_t t_gaptime = atoi(gaptime);
  uint8_t t_runtime = atoi(runtime);

  if (t_gaptime == c_GAPTIME[0] || t_runtime == c_RUNTIME[0]) {
    _configValue.gapTime = c_GAPTIME[0];
    _configValue.runTime = c_RUNTIME[0];

    Serial.println(" -> Set To Real-Time Mode!\r\n");
  } else {
    for (size_t i = 0; i < GAPTIME_OPT_CNT; i++) {
      if (t_gaptime == c_GAPTIME[i]) {
        _configValue.gapTime = t_gaptime;
        break;
      } else if (i == GAPTIME_OPT_CNT - 1) {
        log_e("Invalid GapTime!\r\n -> Valid GapTime: 0, 60\r\n");
        return;
      }
    }

    for (size_t j = 0; j < RUNTIME_OPT_CNT; j++) {
      if (t_runtime == c_RUNTIME[j]) {
        _configValue.runTime = t_runtime;
        break;
      } else if (j == RUNTIME_OPT_CNT - 1) {
        log_e("Invalid RunTime!\r\n -> Valid RunTime: 2, 5, 10, 20, 30\r\n");
        return;
      }
    }

    Serial.print(" -> Set GapTime: ");
    Serial.println(_configValue.gapTime);
    Serial.print(" -> Set RunTime: ");
    Serial.println(_configValue.runTime);
    Serial.println(" ...");
  }

  cmdprefer.begin("Configs");
  cmdprefer.putUChar("GapTime", _configValue.gapTime);
  cmdprefer.putUChar("RunTime", _configValue.runTime);
  cmdprefer.end();
  Serial.println(" -> Rate Config Saved!\r\n");
}
#endif

void SerialCmd::cmdSetWiFi(char *ssid, char *pswd) {
  strcpy(_configValue.ssid, ssid);
  strcpy(_configValue.pswd, pswd);

  Serial.print(" -> Set WiFi SSID: ");
  Serial.println(_configValue.ssid);
  Serial.print(" -> Set WiFi Password: ");
  Serial.println(_configValue.pswd);
  Serial.println(" ...");

  cmdprefer.begin("Configs");
  cmdprefer.putString("SSID", String(_configValue.ssid));
  cmdprefer.putString("PSWD", String(_configValue.pswd));
  cmdprefer.end();
  Serial.println(" -> WiFi Config Saved!\r\n");
}

#ifdef ARCHIVE
void SerialCmd::cmdSetUDP(char *ipv4, char *port) {
  char *p_t_ipv4 = ipv4;
  char *p_t_port = port;

  uint8_t t_ipv4_len = strlen(p_t_ipv4);
  uint8_t t_point_cnt = 0;
  uint8_t t_number_cnt = 0;

  if (strcmp(p_t_ipv4, "127.0.0.1") == 0) {
    log_e("Do Not Use LocalHost!\r\n -> Valid Format: xxx.xxx.xxx.xxx\r\n");
    return;
  }

  for (size_t i = 0; i < t_ipv4_len; i++) {
    if (p_t_ipv4[i] == '.') {
      t_point_cnt++;
      if (t_number_cnt > 3 || t_number_cnt < 1) {
        log_e("Invalid IP Format!\r\n -> Valid Format: xxx.xxx.xxx.xxx\r\n");
        return;
      }
      t_number_cnt = 0;
    } else {
      t_number_cnt++;
    }
  }
  if (t_point_cnt != 3 || t_number_cnt > 3 || t_number_cnt < 1) {
    log_e("Invalid IP Format!\r\n -> Valid Format: xxx.xxx.xxx.xxx\r\n");
    return;
  }

  if (strlen(p_t_port) > 5 || strlen(p_t_port) < 1) {
    log_e("Invalid Port Value!\r\n -> Valid Port Value: 1-65535\r\n");
    return;
  }

  uint32_t t_port = atoi(p_t_port);
  if (t_port > 65535 || t_port < 1) {
    log_e("Invalid Port Value!\r\n -> Valid Port Value: 1-65535\r\n");
    return;
  }

  strcpy(_configValue.ipv4, p_t_ipv4);
  _configValue.port = t_port;

  Serial.print(" -> Set Target IP: ");
  Serial.println(_configValue.ipv4);
  Serial.print(" -> Set Target Port: ");
  Serial.println(_configValue.port);
  Serial.println(" ...");

  cmdprefer.begin("Configs");
  cmdprefer.putString("IPV4", String(_configValue.ipv4));
  cmdprefer.putUShort("Port", _configValue.port);
  cmdprefer.end();
  Serial.println(" -> UDP Config Saved!\r\n");
}
#endif

void SerialCmd::cmdSetOTA(char *url) {
  strcpy(_configValue.url, url);

  Serial.print(" -> Set OTA URL: ");
  Serial.println(_configValue.url);
  Serial.println(" ...");

  cmdprefer.begin("Configs");
  cmdprefer.putString("URL", String(_configValue.url));
  cmdprefer.end();
  Serial.println(" -> OTA URL Saved!\r\n");
}

ConfigValue SerialCmd::cmdGetConfig(bool ifPrintInfo) {
  ConfigValue t_cfv = _configValue;
  if (ifPrintInfo) {
#ifdef ARCHIVE
    Serial.print(" -> GapTime: ");
    Serial.println(t_cfv.gapTime);
    Serial.print(" -> RunTime: ");
    Serial.println(t_cfv.runTime);
    Serial.print(" -> Target IP: ");
    Serial.println(t_cfv.ipv4);
    Serial.print(" -> Target Port: ");
    Serial.println(t_cfv.port);
#endif
    Serial.println(" -> Your Configs Are: ");
    Serial.print(" -> WiFi SSID: ");
    Serial.println(t_cfv.ssid);
    Serial.print(" -> WiFi Password: ");
    Serial.println(t_cfv.pswd);
    Serial.print(" -> OTA URL: ");
    Serial.println(t_cfv.url);
    Serial.println();
  }
  return t_cfv;
}