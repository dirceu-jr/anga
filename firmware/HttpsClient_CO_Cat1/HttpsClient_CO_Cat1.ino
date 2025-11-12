// TODO:
// - Ping ThingSpeak Generic HTTP - DONE;
// - Ping ThingSpeak Generic HTTPS - May need to upgrade Firmware to support TLS;
// - Disable Wi-Fi;
// - Disable Bluetooth;
// - Check/disable GPS;
// - ESP32 deep sleep;
// - Modem off while sleep;
// - CO;
// - CO sensor is "on" when ESP32 is sleeping? yes (but low power)...
// - VOC;
// - Optimizations;

// BUGS:
// - bug, it stoped to work but still with batery power;
// - started to work again after batery removed/re-inserted;
// - may turn modem off/on on setup()? (hard reset);
// - also sometimes the modem is not turning off before deep-sleep;

#include "utilities.h"

// #define TINY_GSM_MODEM_A76XXSSL

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#define TINY_GSM_RX_BUFFER 1024

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// #define LOGGING  // <- Logging is for the HTTP library

// Your GPRS credentials, if any
const char apn[] = "iot.datatem.com.br";
const char gprsUser[] = "datatem";
const char gprsPass[] = "datatem";

// Server details
// const char server[] = "httpbin.org";
const char server[] = "api.thingspeak.com";
const int port = 443;

// Generic Cat 1
const char* writeAPIKey = "API Key";

#include "esp_wifi.h"
#include "esp_bt.h"

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// #include "DFRobot_MultiGasSensor.h"

// #define I2C_ADDRESS    0x74
// DFRobot_GAS_I2C gas(&Wire, I2C_ADDRESS);

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

// TinyGsmClient client(modem);
// TinyGsmClientSecure client(modem);
// HttpClient          http(client, server, port);

// #define LED_PIN     12

// void modemPowerOn() {
//   pinMode(PWR_PIN, OUTPUT);
//   digitalWrite(PWR_PIN, LOW);
//   delay(1000); //Datasheet Ton mintues = 1S
//   digitalWrite(PWR_PIN, HIGH);
// }

// void modemPowerOff() {
//   pinMode(PWR_PIN, OUTPUT);
//   digitalWrite(PWR_PIN, LOW);
//   delay(1500); //Datasheet Ton mintues = 1.2S
//   digitalWrite(PWR_PIN, HIGH);
// }

// void modemHardReset() {
//   SerialMon.println(F("Performing modem hard reset..."));
//   modemPowerOff();
//   delay(5000); // Wait 5 seconds for complete power down
//   modemPowerOn();
//   delay(5000); // Wait 5 seconds for complete power up
// }

void setup() {
  // Set LED OFF
  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, HIGH);

  // Set console baud rate
  SerialMon.begin(115200);
  SerialMon.println("Hello A7670SA!!!");
  delay(1000);

  // Disable Wi-Fi and Bluetooth
  SerialMon.println(F("Disabling Wi-Fi and Bluetooth..."));
  esp_wifi_stop();
  esp_bt_controller_disable();

  // // gas sensor
  // // Mode of obtaining data: the main controller needs to request the sensor for data
  // gas.changeAcquireMode(gas.PASSIVITY);
  // delay(1000);
  // // Turn on temperature compensation
  // gas.setTempCompensation(gas.ON);

  // modem
  SerialMon.println(F("Power modem on..."));
  // TODO:
  // - may need to power modem up (apparently it is ON);
  // - if stall again, test with hard reset;
  // modemHardReset();
  // modemPowerOn();

  // Wait 5 seconds for complete power up
  delay(5000);

  // SerialMon.println("Wait...");

  /* Set Power control pin output
  * * @note      Known issues, ESP32 (V1.2) version of T-A7670, T-A7608,
  *            when using battery power supply mode, BOARD_POWERON_PIN (IO12) must be set to high level after esp32 starts, otherwise a reset will occur.
  * */
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);

  // Set modem reset
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  // Pull down DTR to ensure the modem is not in sleep state
  pinMode(MODEM_DTR_PIN, OUTPUT);
  digitalWrite(MODEM_DTR_PIN, LOW);

  // Turn on modem
  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(MODEM_POWERON_PULSE_WIDTH_MS);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  // Set GSM module baud rate
  SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Restating modem...");
  modem.restart();
  // modem.init();

  // Disable GPS
  // modem.disableGPS();

  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  // modem.sendAT("+SGPIO=0,4,1,0");
  // if (modem.waitResponse(10000L) != 1) {
  //   SerialMon.println(" SGPIO=0,4,1,0 false ");
  // }

  delay(200);
  
  String resp;
  modem.sendAT("+SIMCOMATI");
  modem.waitResponse(1000L, resp);
  Serial.println(resp);

  modem.sendAT("+CSUB");
  modem.waitResponse(1000L, resp);
  Serial.println(resp);

  modem.sendAT("ATI");
  modem.waitResponse(1000L, resp);
  Serial.println(resp);

  modem.sendAT("+GMI");
  modem.waitResponse(1000L, resp);
  Serial.println(resp);

  String name = modem.getModemName();
  SerialMon.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  SerialMon.println("Modem Info: " + modemInfo);

  // Check SIM status
  SerialMon.print("SIM Status: ");
  SerialMon.println(modem.getSimStatus());

  String res;
  // 2 Automatic
  // 13 GSM only
  // 38 LTE only
  // 51 GSM and LTE only
  res = modem.setNetworkMode(2);
  SerialMon.print("setNetworkMode: ");
  SerialMon.println(res);
}

void loop() {
  // modem.gprsConnect(apn, gprsUser, gprsPass);

  SerialMon.print("Signal quality: ");
  SerialMon.println(modem.getSignalQuality());

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }

  // Add debugging for local IP
  IPAddress local = modem.localIP();
  SerialMon.print("Local IP: ");
  SerialMon.println(local);

  // Now try HTTP request
  // http.setTimeout(30000);
  // http.connectionKeepAlive(); // this may be needed for HTTPS

  // const char *server_url =  "https://api.thingspeak.com";
  String server_url = String("https://api.thingspeak.com") + String("/update?api_key=") + writeAPIKey + "&field1=" + String(1);

  SerialMon.println(server_url);

  modem.https_set_url(server_url);

  String post_body = "";

  int httpCode = modem.https_post(post_body);
    if (httpCode != 200) {
        Serial.print("HTTP post failed ! error code = ");
        Serial.println(httpCode);
        delay(10000);
        return;
    }

    // Get HTTPS header information
    String header = modem.https_header();
    Serial.print("HTTP Header : ");
    Serial.println(header);

    // Get HTTPS response
    String body = modem.https_body();
    Serial.print("HTTP body : ");
    Serial.println(body);

    // Disconnect http server
    modem.https_end();

  // Construct the resource URL
  // String resource = String("/update?api_key=") + writeAPIKey + "&field1=" + String(gas.readGasConcentrationPPM());
  // String resource = String("/update?api_key=") + writeAPIKey + "&field1=" + String(1);
  // SerialMon.println(resource);
  // String resource = "/post";

  // SerialMon.print(F("Performing HTTPS POST request... "));
  // int err = http.post(resource);
  // if (err != 0) {
  //   SerialMon.print(F("failed to connect, error: "));
  //   SerialMon.println(err);
  //   delay(10000);
  //   return;
  // }

  // int status = http.responseStatusCode();
  // SerialMon.print(F("Response status code: "));
  // SerialMon.println(status);
  // if (!status) {
  //   delay(10000);
  //   return;
  // }

  // SerialMon.println(F("Response Headers:"));
  // while (http.headerAvailable()) {
  //   String headerName  = http.readHeaderName();
  //   String headerValue = http.readHeaderValue();
  //   SerialMon.println("    " + headerName + " : " + headerValue);
  // }

  // int length = http.contentLength();
  // if (length >= 0) {
  //   SerialMon.print(F("Content length is: "));
  //   SerialMon.println(length);
  // }
  // if (http.isResponseChunked()) {
  //   SerialMon.println(F("The response is chunked"));
  // }

  // String body = http.responseBody();
  // SerialMon.println(F("Response:"));
  // SerialMon.println(body);

  // SerialMon.print(F("Body length is: "));
  // SerialMon.println(body.length());

  // Shutdown

  // http.stop();
  SerialMon.println(F("Server disconnected"));

  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));

  // // Power down the modem using AT command first
  // SerialMon.println(F("Powering down modem with AT command..."));
  // modem.sendAT("+CPOWD=1");
  // modem.waitResponse();

  // // Then turn off the modem power
  // SerialMon.println(F("Powering off modem..."));
  // modemPowerOff();
  // SerialMon.println(F("Modem off"));

  // OBS:
  // - check this out:
  // modem.setNetworkActive();
  // modem.setNetworkDeactivate();

  // Wait 5 seconds for complete power down
  delay(5000);

  // sleep for 5 minutes
  ESP.deepSleep(300e6);
}
