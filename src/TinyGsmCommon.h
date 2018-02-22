/**
 * @file       TinyGsmCommon.h
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef TinyGsmCommon_h
#define TinyGsmCommon_h

#if defined(SPARK) || defined(PARTICLE)
  #include "Particle.h"
#elif defined(ARDUINO)
  #if ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include <Client.h>
#include <TinyGsmFifo.h>

#ifndef TINY_GSM_YIELD
  #define TINY_GSM_YIELD() { delay(0); }
#endif

#define TINY_GSM_ATTR_NOT_AVAILABLE __attribute__((error("Not available on this modem type")))
#define TINY_GSM_ATTR_NOT_IMPLEMENTED __attribute__((error("Not implemented")))

#if defined(__AVR__)
  #define TINY_GSM_PROGMEM PROGMEM
  typedef const __FlashStringHelper* GsmConstStr;
  #define GFP(x) (reinterpret_cast<GsmConstStr>(x))
  #define GF(x)  F(x)
#else
  #define TINY_GSM_PROGMEM
  typedef const char* GsmConstStr;
  #define GFP(x) x
  #define GF(x)  x
#endif

#ifdef TINY_GSM_DEBUG
namespace {
  template<typename T>
  static void DBG(T last) {
    TINY_GSM_DEBUG.println(last);
  }

  template<typename T, typename... Args>
  static void DBG(T head, Args... tail) {
    TINY_GSM_DEBUG.print(head);
    TINY_GSM_DEBUG.print(' ');
    DBG(tail...);
  }
}
#else
  #define DBG(...)
#endif

template<class T>
const T& TinyGsmMin(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T>
const T& TinyGsmMax(const T& a, const T& b)
{
    return (b < a) ? a : b;
}

template<class T>
uint32_t TinyGsmAutoBaud(T& SerialAT, uint32_t minimum = 9600, uint32_t maximum = 115200)
{
  static uint32_t rates[] = { 115200, 57600, 38400, 19200, 9600, 74400, 74880, 230400, 460800, 2400, 4800, 14400, 28800 };

  for (unsigned i = 0; i < sizeof(rates)/sizeof(rates[0]); i++) {
    uint32_t rate = rates[i];
    if (rate < minimum || rate > maximum) continue;

    DBG("Trying baud rate", rate, "...");
    SerialAT.begin(rate);
    delay(10);
    for (int i=0; i<3; i++) {
      SerialAT.print("AT\r\n");
      String input = SerialAT.readString();
      if (input.indexOf("OK") >= 0) {
        DBG("Modem responded at rate", rate);
        return rate;
      }
    }
  }
  return 0;
}

static inline
IPAddress TinyGsmIpFromString(const String& strIP) {
  int Parts[4] = {0, };
  int Part = 0;
  for (uint8_t i=0; i<strIP.length(); i++) {
    char c = strIP[i];
    if (c == '.') {
      Part++;
      if (Part > 3) {
        return IPAddress(0,0,0,0);
      }
      continue;
    } else if (c >= '0' && c <= '9') {
      Parts[Part] *= 10;
      Parts[Part] += c - '0';
    } else {
      if (Part == 3) break;
    }
  }
  return IPAddress(Parts[0], Parts[1], Parts[2], Parts[3]);
}

static inline
String TinyGsmDecodeHex7bit(String &instr) {
  String result;
  byte reminder = 0;
  int bitstate = 7;
  for (unsigned i=0; i<instr.length(); i+=2) {
    char buf[4] = { 0, };
    buf[0] = instr[i];
    buf[1] = instr[i+1];
    byte b = strtol(buf, NULL, 16);

    byte bb = b << (7 - bitstate);
    char c = (bb + reminder) & 0x7F;
    result += c;
    reminder = b >> bitstate;
    bitstate--;
    if (bitstate == 0) {
      char c = reminder;
      result += c;
      reminder = 0;
      bitstate = 7;
    }
  }
  return result;
}

static inline
String TinyGsmDecodeHex8bit(String &instr) {
  String result;
  for (unsigned i=0; i<instr.length(); i+=2) {
    char buf[4] = { 0, };
    buf[0] = instr[i];
    buf[1] = instr[i+1];
    char b = strtol(buf, NULL, 16);
    result += b;
  }
  return result;
}

static inline
String TinyGsmDecodeHex16bit(String &instr) {
  String result;
  for (unsigned i=0; i<instr.length(); i+=4) {
    char buf[4] = { 0, };
    buf[0] = instr[i];
    buf[1] = instr[i+1];
    char b = strtol(buf, NULL, 16);
    if (b) { // If high byte is non-zero, we can't handle it ;(
#if defined(TINY_GSM_UNICODE_TO_HEX)
      result += "\\x";
      result += instr.substring(i, i+4);
#else
      result += "?";
#endif
    } else {
      buf[0] = instr[i+2];
      buf[1] = instr[i+3];
      b = strtol(buf, NULL, 16);
      result += b;
    }
  }
  return result;
}

#if defined(TINY_GSM_MODEM_SIM800) || defined(TINY_GSM_MODEM_SIM900) || defined(TINY_GSM_MODEM_SIM808) || defined(TINY_GSM_MODEM_SIM868)
  #define GSM_NL "\r\n"

#elif defined(TINY_GSM_MODEM_A6) || defined(TINY_GSM_MODEM_A7)
  #define GSM_NL "\r\n"

#elif defined(TINY_GSM_MODEM_M590)
  #define GSM_NL "\r\n"

#elif defined(TINY_GSM_MODEM_U201)
  #define GSM_NL "\r\n"

#elif defined(TINY_GSM_MODEM_ESP8266)
  #define GSM_NL "\r\n"

#elif defined(TINY_GSM_MODEM_XBEE)
  #define GSM_NL "\r"

#else
  #error "Please define GSM modem model"
  // Some definitions for myself to help debugging
  #define GSM_NL "\r\n"
#endif

static const char GSM_OK[] TINY_GSM_PROGMEM = "OK" GSM_NL;
static const char GSM_ERROR[] TINY_GSM_PROGMEM = "ERROR" GSM_NL;


class TinyGSMModem
{

public:

#ifdef GSM_DEFAULT_STREAM
  TinyGSMModem(Stream& stream = GSM_DEFAULT_STREAM)
#else
  TinyGSMModem(Stream& stream)
#endif
    : stream(stream)
  {}

  /*
   * Basic functions
   */
  virtual bool begin() {
    return init();
  }

  virtual bool init() = 0;
  virtual void setBaud(unsigned long baud) = 0;
  virtual bool testAT(unsigned long timeout = 10000L) = 0;
  virtual void maintain() = 0;
  virtual bool factoryDefault() = 0;
  virtual bool hasSSL() {
    #if defined(TINY_GSM_MODEM_HAS_SSL)
    return true;
    #else
    return false;
    #endif
  }

  /*
   * Power functions
   */

  virtual bool restart() = 0;
  bool poweroff() TINY_GSM_ATTR_NOT_AVAILABLE;
  bool radioOff() TINY_GSM_ATTR_NOT_IMPLEMENTED;
  bool sleepEnable(bool enable = true) TINY_GSM_ATTR_NOT_IMPLEMENTED;

  /*
   * SIM card functions
   */

  virtual bool simUnlock(const char *pin) TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual String getSimCCID() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual String getIMEI() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual int getSimStatus(unsigned long timeout = 10000L) TINY_GSM_ATTR_NOT_AVAILABLE;
  String getOperator() TINY_GSM_ATTR_NOT_AVAILABLE;

  /*
   * Generic network functions
   */

  virtual int getRegistrationStatus() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual int getSignalQuality() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual bool isNetworkConnected() = 0;
  virtual bool waitForNetwork(unsigned long timeout = 60000L) {
    for (unsigned long start = millis(); millis() - start < timeout; ) {
      if (isNetworkConnected()) {
        return true;
      }
      delay(250);
    }
    return false;
  }
  virtual String getLocalIP() TINY_GSM_ATTR_NOT_AVAILABLE;

  virtual IPAddress localIP() {
    return TinyGsmIpFromString(getLocalIP());
  }

  /*
   * WiFi functions
   */

  virtual bool networkConnect(const char* ssid, const char* pwd)
    TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual bool networkDisconnect(const char* ssid, const char* pwd)
    TINY_GSM_ATTR_NOT_AVAILABLE;

  /*
   * GPRS functions
   */

  virtual bool gprsConnect(const char* apn, const char* user = "", const char* pwd = "")
     TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual bool gprsDisconnect() TINY_GSM_ATTR_NOT_AVAILABLE;

  /*
   * Messaging functions
   */

  virtual void sendUSSD() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual void sendSMS() TINY_GSM_ATTR_NOT_AVAILABLE;
  virtual bool sendSMS(const String& number, const String& text) TINY_GSM_ATTR_NOT_AVAILABLE;


  /*
   * Location functions
   */

  String getGsmLocation() TINY_GSM_ATTR_NOT_AVAILABLE;

  /*
   * Battery functions
   */

  uint16_t getBattVoltage() TINY_GSM_ATTR_NOT_AVAILABLE;
  int getBattPercent() TINY_GSM_ATTR_NOT_AVAILABLE;

public:

  /* Utilities */

  template<typename T>
  void streamWrite(T last) {
    stream.print(last);
  }

  template<typename T, typename... Args>
  void streamWrite(T head, Args... tail) {
    stream.print(head);
    streamWrite(tail...);
  }

  bool streamSkipUntil(char c) {
    const unsigned long timeout = 1000L;
    unsigned long startMillis = millis();
    while (millis() - startMillis < timeout) {
      while (millis() - startMillis < timeout && !stream.available()) {
        TINY_GSM_YIELD();
      }
      if (stream.read() == c)
        return true;
    }
    return false;
  }

  template<typename... Args>
  void sendAT(Args... cmd) {
    streamWrite("AT", cmd..., GSM_NL);
    stream.flush();
    TINY_GSM_YIELD();
    DBG("### AT:", cmd...);
  }

  // TODO: Optimize this!
  virtual uint8_t waitResponse(uint32_t timeout, String& data,
                               GsmConstStr r1=GFP(GSM_OK),
                               GsmConstStr r2=GFP(GSM_ERROR),
                               GsmConstStr r3=NULL,
                               GsmConstStr r4=NULL,
                               GsmConstStr r5=NULL) = 0;

protected:
  Stream&       stream;
};

#endif
