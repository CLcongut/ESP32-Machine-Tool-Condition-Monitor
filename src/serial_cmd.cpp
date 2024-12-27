#include "serial_cmd.h"

SerialCmd::SerialCmd()
    : c_cmdIndexAry{{"/setrate", 1}, {"/setwifi", 2}, {"/setudp", 3},
                    {"/setota", 4},  {"/help", 5},    {"/update", 6},
                    {"/close", 7},   {"/ver", 8}} {
  _configValue.update = false;
  _consoleMode = MODE_NULL;
  _ledpin = -1;
}

SerialCmd::~SerialCmd() {
  cmdprefer.~Preferences();
  udpClient.stop();
}

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

  _configValue.gapTime = cmdprefer.getUChar("GapTime");
  _configValue.runTime = cmdprefer.getUChar("RunTime");
  strcpy(_configValue.ssid, cmdprefer.getString("SSID", "null").c_str());
  strcpy(_configValue.pswd, cmdprefer.getString("PSWD", "null").c_str());
  strcpy(_configValue.ipv4, cmdprefer.getString("IPV4", "null").c_str());
  _configValue.port = cmdprefer.getUShort("Port");
  strcpy(_configValue.url, cmdprefer.getString("URL", "null").c_str());

  cmdprefer.end();
}

void SerialCmd::setFrimwareVersion(const char *version) {
  strcpy(_frwversion, version);
}

size_t SerialCmd::cmdGeneralScanf() {
  switch (_consoleMode) {
    case MODE_SRL:
      consoleSerialPrintln(charInfo_1_1);
      break;
    case MODE_UDP:
      consoleSerialPrintln(charInfo_1_2);
      udpClient.begin(UDP_CONSOLE_PORT);
      udpClient.setTxMode(true);
      break;
    case MODE_NULL:
      return 0;
  }
  StateLEDGLOW();
  while (true) {
    size_t cmdLen;
    switch (_consoleMode) {
      case MODE_SRL:
        cmdLen = Serial.available();
        break;
      case MODE_UDP:
        cmdLen = udpClient.parsePacket();
        break;
    }
    if (cmdLen) {
      size_t index = 0;
      char cmdHead[24];
      char value1[100];
      char value2[24];
      for (;;) {
        if (index < cmdLen) {
          switch (_consoleMode) {
            case MODE_SRL:
              _rcvCommand[index++] = Serial.read();
              break;
            case MODE_UDP:
              _rcvCommand[index++] = udpClient.read();
              break;
          }
        } else {
          _rcvCommand[index] = '\0';
          consoleGeneralPrintf("%s%s\r\n", charIncp_1_1, _rcvCommand);
          if (_rcvCommand[0] == '/') {
            sscanf(_rcvCommand, "%23s %99s %23s", cmdHead, value1, value2);
            switch (findIndex(cmdHead)) {
              case 1:
                cmdSetRate(value1, value2);
                break;
              case 2:
                cmdSetWiFi(value1, value2);
                break;
              case 3:
                cmdSetUDP(value1, value2);
                break;
              case 4:
                cmdSetOTA(value1);
                break;
              case 5:
                consoleGeneralPrintln(_helpInfo);
                break;
              case 6:
                _configValue.update = !_configValue.update;
                if (_configValue.update) {
                  consoleGeneralPrintln(charInfo_1_3);
                } else {
                  consoleGeneralPrintln(charInfo_1_4);
                }
                break;
              case 7:
                consoleGeneralPrintln(charInfo_1_5);
                StateLEDFADE();
                return 0;
                break;
              case 8:
                consoleGeneralPrintf("%s%s\r\n", charIncp_0_1, _frwversion);
                break;
              default:
                consoleGeneralPrintln(charErro_1_1);
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

void SerialCmd::cmdSetRate(char *gaptime, char *runtime) {
  uint8_t t_gaptime = atoi(gaptime);
  uint8_t t_runtime = atoi(runtime);

  if (t_gaptime == c_GAPTIME[0] || t_runtime == c_RUNTIME[0]) {
    _configValue.gapTime = c_GAPTIME[0];
    _configValue.runTime = c_RUNTIME[0];
    consoleGeneralPrintln(charInfo_2_1);
  } else {
    for (size_t i = 0; i < GAPTIME_OPT_CNT; i++) {
      if (t_gaptime == c_GAPTIME[i]) {
        _configValue.gapTime = t_gaptime;
        break;
      } else if (i == GAPTIME_OPT_CNT - 1) {
        consoleGeneralPrintln(charErro_2_1);
        return;
      }
    }

    for (size_t j = 0; j < RUNTIME_OPT_CNT; j++) {
      if (t_runtime == c_RUNTIME[j]) {
        _configValue.runTime = t_runtime;
        break;
      } else if (j == RUNTIME_OPT_CNT - 1) {
        consoleGeneralPrintln(charErro_2_2);
        return;
      }
    }
    consoleGeneralPrintf("%s%d\r\n%s%d\r\n%s\r\n%s\r\n", charIncp_2_1,
                         _configValue.gapTime, charIncp_2_2,
                         _configValue.runTime, charGerl_1, charInfo_2_2);
  }
  cmdprefer.begin("Configs");
  cmdprefer.putUChar("GapTime", _configValue.gapTime);
  cmdprefer.putUChar("RunTime", _configValue.runTime);
  cmdprefer.end();
}

void SerialCmd::cmdSetWiFi(char *ssid, char *pswd) {
  strcpy(_configValue.ssid, ssid);
  strcpy(_configValue.pswd, pswd);

  cmdprefer.begin("Configs");
  cmdprefer.putString("SSID", String(_configValue.ssid));
  cmdprefer.putString("PSWD", String(_configValue.pswd));
  cmdprefer.end();

  consoleGeneralPrintf("%s%s\r\n%s%s\r\n%s\r\n%s\r\n", charIncp_3_1,
                       _configValue.ssid, charIncp_3_2, _configValue.pswd,
                       charGerl_1, charInfo_3_1);
}

void SerialCmd::cmdSetUDP(char *ipv4, char *port) {
  char *p_t_ipv4 = ipv4;
  char *p_t_port = port;

  uint8_t t_ipv4_len = strlen(p_t_ipv4);
  uint8_t t_point_cnt = 0;
  uint8_t t_number_cnt = 0;

  if (strcmp(p_t_ipv4, "127.0.0.1") == 0) {
    consoleGeneralPrintln(charErro_4_1);
    return;
  }

  for (size_t i = 0; i < t_ipv4_len; i++) {
    if (p_t_ipv4[i] == '.') {
      t_point_cnt++;
      if (t_number_cnt > 3 || t_number_cnt < 1) {
        consoleGeneralPrintln(charErro_4_2);
        return;
      }
      t_number_cnt = 0;
    } else {
      t_number_cnt++;
    }
  }
  if (t_point_cnt != 3 || t_number_cnt > 3 || t_number_cnt < 1) {
    consoleGeneralPrintln(charErro_4_2);
    return;
  }

  if (strlen(p_t_port) > 5 || strlen(p_t_port) < 1) {
    consoleGeneralPrintln(charErro_4_3);
    return;
  }

  uint32_t t_port = atoi(p_t_port);
  if (t_port > 65535 || t_port < 1) {
    consoleGeneralPrintln(charErro_4_3);
    return;
  }

  strcpy(_configValue.ipv4, p_t_ipv4);
  _configValue.port = t_port;

  cmdprefer.begin("Configs");
  cmdprefer.putString("IPV4", String(_configValue.ipv4));
  cmdprefer.putUShort("Port", _configValue.port);
  cmdprefer.end();

  consoleGeneralPrintf("%s%s\r\n%s%d\r\n%s\r\n%s\r\n", charIncp_4_1,
                       _configValue.ipv4, charIncp_4_2, _configValue.port,
                       charGerl_1, charInfo_4_1);
}

void SerialCmd::cmdSetOTA(char *url) {
  strcpy(_configValue.url, url);

  cmdprefer.begin("Configs");
  cmdprefer.putString("URL", String(_configValue.url));
  cmdprefer.end();

  consoleGeneralPrintf("%s%s\r\n%s\r\n%s\r\n", charIncp_5_1, _configValue.url,
                       charGerl_1, charInfo_5_1);
}

ConfigValue SerialCmd::cmdGetConfig(bool ifPrintInfo) {
  ConfigValue t_cfv = _configValue;
  if (ifPrintInfo) {
    consoleGeneralPrintf(
        "%s\r\n%s%d\r\n%s%d\r\n%s%s\r\n%s%s\r\n%s%s\r\n%s%d\r\n%s%s\r\n",
        charIncp_6_1, charIncp_6_2, t_cfv.gapTime, charIncp_6_3, t_cfv.runTime,
        charIncp_6_4, t_cfv.ssid, charIncp_6_5, t_cfv.pswd, charIncp_6_6,
        t_cfv.ipv4, charIncp_6_7, t_cfv.port, charIncp_6_8, t_cfv.url);
  }
  return t_cfv;
}

uint16_t SerialCmd::cmdGetPort() { return _configValue.port; }

void SerialCmd::consoleSetMode(ConsoleMode mode) { _consoleMode = mode; }

void SerialCmd::consoleSetStateLight(const int ledpin) {
  _ledpin = ledpin;
  pinMode(_ledpin, OUTPUT);
}

void SerialCmd::consoleSerialPrintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buf[SCANF_BUF_SIZE];
  vsnprintf(buf, sizeof(buf), format, args);
  Serial.print(buf);
  va_end(args);
}

void SerialCmd::consoleUdpPrintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buf[SCANF_BUF_SIZE];
  vsnprintf(buf, sizeof(buf), format, args);
  udpClient.beginPacket(udpClient.remoteIP(), udpClient.remotePort());
  udpClient.print(buf);
  udpClient.endPacket_N();
  va_end(args);
}

void SerialCmd::consoleGeneralPrintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buf[SCANF_BUF_SIZE];
  vsnprintf(buf, sizeof(buf), format, args);

  switch (_consoleMode) {
    case MODE_SRL:
      Serial.print(buf);
      break;
    case MODE_UDP:
      udpClient.beginPacket(udpClient.remoteIP(), udpClient.remotePort());
      udpClient.print(buf);
      udpClient.endPacket_N();
      break;
    default:
      break;
  }

  va_end(args);
}

void SerialCmd::consoleSerialPrintln(const char *str) { Serial.println(str); }

void SerialCmd::consoleUdpPrintln(const char *str) {
  udpClient.beginPacket(udpClient.remoteIP(), udpClient.remotePort());
  udpClient.println(str);
  udpClient.endPacket_N();
}

void SerialCmd::consoleGeneralPrintln(const char *str) {
  switch (_consoleMode) {
    case MODE_SRL:
      Serial.println(str);
      break;
    case MODE_UDP:
      udpClient.beginPacket(udpClient.remoteIP(), udpClient.remotePort());
      udpClient.println(str);
      udpClient.endPacket_N();
      break;
    default:
      break;
  }
}

void SerialCmd::consoleOnWeb() {
#if 1
  StateLEDGLOW();
  server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
    String html = htmlForm;
    html.replace("%SSID%", String(_configValue.ssid));
    html.replace("%PASSWORD%", String(_configValue.pswd));
    html.replace("%UDP_IP%", String(_configValue.ipv4));
    html.replace("%UDP_PORT%", String(_configValue.port));
    html.replace("%GAP_TIME_0%", _configValue.gapTime == 0 ? "selected" : "");
    html.replace("%GAP_TIME_60%", _configValue.gapTime == 60 ? "selected" : "");

    html.replace("%RUN_TIME_0%", _configValue.runTime == 0 ? "selected" : "");
    html.replace("%RUN_TIME_2%", _configValue.runTime == 2 ? "selected" : "");
    html.replace("%RUN_TIME_5%", _configValue.runTime == 5 ? "selected" : "");
    html.replace("%RUN_TIME_10%", _configValue.runTime == 10 ? "selected" : "");
    html.replace("%RUN_TIME_20%", _configValue.runTime == 20 ? "selected" : "");
    html.replace("%RUN_TIME_30%", _configValue.runTime == 30 ? "selected" : "");

    html.replace("%CHECKED%", _configValue.update ? "checked" : "");
    html.replace("%VERSION%", String(_frwversion));
    html.replace("%URL%", String(_configValue.url));
    request->send(200, "text/html; charset=utf-8", html);
  });

  String ssid, password, udp_ip, ota_url;
  bool configured = false;

  server.on("/set", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true)) {
      ssid = request->getParam("ssid", true)->value();
      strcpy(_configValue.ssid, ssid.c_str());
    }
    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
      strcpy(_configValue.pswd, password.c_str());
    }
    if (request->hasParam("udp_ip", true)) {
      udp_ip = request->getParam("udp_ip", true)->value();
      strcpy(_configValue.ipv4, udp_ip.c_str());
    }
    if (request->hasParam("udp_port", true)) {
      _configValue.port = request->getParam("udp_port", true)->value().toInt();
    }
    if (request->hasParam("gap_time", true)) {
      _configValue.gapTime =
          request->getParam("gap_time", true)->value().toInt();
    }
    if (request->hasParam("run_time", true)) {
      _configValue.runTime =
          request->getParam("run_time", true)->value().toInt();
    }
    if (request->hasParam("update", true)) {
      _configValue.update = true;
    } else {
      _configValue.update = false;
    }
    if (request->hasParam("url", true)) {
      ota_url = request->getParam("url", true)->value();
      strcpy(_configValue.url, ota_url.c_str());
    }

    configured = true;

    request->send(200, "text/html; charset=utf-8",
                  "完成配置, 你现在可以关闭这个网页.</h1>");
  });

  server.begin();

  while (!configured) {
    vTaskDelay(100);
  }
  StateLEDFADE();

  cmdprefer.begin("Configs");
  cmdprefer.putUChar("GapTime", _configValue.gapTime);
  cmdprefer.putUChar("RunTime", _configValue.runTime);
  cmdprefer.putString("SSID", String(_configValue.ssid));
  cmdprefer.putString("PSWD", String(_configValue.pswd));
  cmdprefer.putString("IPV4", String(_configValue.ipv4));
  cmdprefer.putUShort("Port", _configValue.port);
  cmdprefer.putString("URL", String(_configValue.url));
  cmdprefer.end();
#endif
}

#ifdef ARCHIVE
size_t SerialCmd::cmdSerialScanf() {
  // auto val = 1345;
  // consoleSerialPrintf("test : %d", val);
  Serial.println(charInfo_1_1);
  StateLEDGLOW();
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
                cmdSetRate(value1, value2);
                break;
              case 2:
                cmdSetWiFi(value1, value2);
                break;
              case 3:
                cmdSetUDP(value1, value2);
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
                  Serial.print(charInfo_1_3);
                  Serial.println(charInfo_1_4);
                } else {
                  Serial.println(charInfo_1_5);
                }
                break;
              case 7:
                Serial.println();
                StateLEDFADE();
                return 0;
                break;
              default:
                log_e("%s", charErro_1_1);
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

size_t SerialCmd::cmdWifiudpScanf() {
  Serial.println(charInfo_1_2);
  udpClient.begin(UDP_CONSOLE_PORT);
  udpClient.setTxMode(true);
  StateLEDGLOW();
  while (true) {
    size_t cmdLen = udpClient.parsePacket();
    if (cmdLen) {
      size_t index = 0;
      char cmdHead[24];
      char value1[100];
      char value2[24];
      for (;;) {
        if (index < cmdLen) {
          _rcvCommand[index++] = udpClient.read();
        } else {
          _rcvCommand[index] = '\0';
          udpClient.beginPacket(udpClient.remoteIP(), udpClient.remotePort());
          udpClient.print("Rcv: ");
          udpClient.println(_rcvCommand);
          udpClient.endPacket_N();
          if (_rcvCommand[0] == '/') {
            sscanf(_rcvCommand, "%23s %99s %23s", cmdHead, value1, value2);
            switch (findIndex(cmdHead)) {
              case 1:
                cmdSetRate(value1, value2);
                break;
              case 2:
                cmdSetWiFi(value1, value2);
                break;
              case 3:
                cmdSetUDP(value1, value2);
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
                  udpClient.beginPacket(udpClient.remoteIP(),
                                        udpClient.remotePort());
                  udpClient.print(charInfo_1_3);
                  udpClient.println(charInfo_1_4);
                  udpClient.endPacket_N();
                } else {
                  udpClient.beginPacket(udpClient.remoteIP(),
                                        udpClient.remotePort());
                  udpClient.println(charInfo_1_5);
                  udpClient.endPacket_N();
                }
                break;
              case 7:
                udpClient.beginPacket(udpClient.remoteIP(),
                                      udpClient.remotePort());
                udpClient.println(charInfo_1_6);
                udpClient.endPacket_N();
                StateLEDFADE();
                return 0;
                break;
              default:
                udpClient.beginPacket(udpClient.remoteIP(),
                                      udpClient.remotePort());
                udpClient.println(charErro_1_1);
                udpClient.endPacket_N();
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
#endif