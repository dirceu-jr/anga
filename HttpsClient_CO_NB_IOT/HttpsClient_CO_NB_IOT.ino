// Select your modem:
#define TINY_GSM_MODEM_SIM7000SSL

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT Serial1

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

#include "esp_wifi.h"
#include "esp_bt.h"

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#include "DFRobot_MultiGasSensor.h"

// Your GPRS credentials, if any
const char apn[] = "iot.datatem.com.br";
const char gprsUser[] = "datatem";
const char gprsPass[] = "datatem";

// Server details
const char server[] = "api.thingspeak.com";
const int port = 443;

const char* writeAPIKey = "NEC73XT4UKMIPOZM";

#define I2C_ADDRESS    0x74
DFRobot_GAS_I2C gas(&Wire, I2C_ADDRESS);

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClientSecure client(modem);
HttpClient    http(client, server, port);

#define UART_BAUD   115200
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_PIN     12
#define FAN_PIN     2

void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000); //Datasheet Ton mintues = 1S
  digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1200); //Datasheet Ton mintues = 1.2S
  digitalWrite(PWR_PIN, HIGH);
  pinMode(PWR_PIN, INPUT);
}

void modemHardReset() {
  SerialMon.println(F("Performing modem hard reset..."));
  modemPowerOff();
  delay(5000); // Wait 5 seconds for complete power down
  modemPowerOn();
  delay(5000); // Wait 5 seconds for complete power up
}

void refreshAir() {
  // Refresh Air (fan on for 10 seconds)
  digitalWrite(FAN_PIN, HIGH);
  delay(10000);
  digitalWrite(FAN_PIN, LOW);
}

bool connectAndSendData(String gasConcentration) {
  SerialMon.print("Signal quality: ");
  SerialMon.println(modem.getSignalQuality());

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

  // GPRS connection parameters are usually set after network registration
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS already connected.");
  } else {
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      return false;
    }
    SerialMon.println(" success");
  }

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected.");
  }

  // Now try HTTP request
  http.setTimeout(30000);
  http.connectionKeepAlive(); // this may be needed for HTTPS

  // Construct the resource URL
  String resource = String("/update?api_key=") + writeAPIKey + "&field1=" + gasConcentration;

  SerialMon.print(F("Performing HTTPS GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    SerialMon.print(F("failed to connect, error: "));
    SerialMon.println(err);
    return false;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (status <= 0) {
    return false;
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  return true;
}

void disconnectAndShutdown() {
  http.stop();
  SerialMon.println(F("Server disconnected"));

  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));

  // Power down the modem using AT command first
  SerialMon.println(F("Powering down modem with AT command..."));
  modem.sendAT("+CPOWD=1");
  modem.waitResponse();

  // Then turn off the modem power
  SerialMon.println(F("Powering off modem..."));
  modemPowerOff();
  SerialMon.println(F("Modem off"));
}

void setup() {
  // Disable Wi-Fi and Bluetooth
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Fan OFF
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  // Set console baud rate
  SerialMon.begin(115200);
  delay(1000);

  // gas sensor
  // Mode of obtaining data: the main controller needs to request the sensor for data
  gas.changeAcquireMode(gas.PASSIVITY);
  delay(1000);

  // Turn on temperature compensation
  gas.setTempCompensation(gas.ON);

  // modem ON
  modemHardReset();

  // Set GSM module baud rate
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  // SerialMon.println("Restarting modem...");
  // modem.restart();
  SerialMon.println("Initializing modem...");
  modem.init();

  // update modem clock
  // modem.sendAT("+CCLK=\"25/09/07,01:29:00\"");

  // Disable GPS
  modem.disableGPS();

  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    SerialMon.println(" SGPIO=0,4,1,0 false ");
  }

  delay(200);

  String name = modem.getModemName();
  SerialMon.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  SerialMon.println("Modem Info: " + modemInfo);

  // Check SIM status
  SerialMon.print("SIM Status: ");
  SerialMon.println(modem.getSimStatus());

  String res;
  // 1 CAT-M
  // 2 NB-IoT
  // 3 CAT-M and NB-IoT
  res = modem.setPreferredMode(2);
  SerialMon.print("setPreferredMode: ");
  SerialMon.println(res);

  // 2 Automatic
  // 13 GSM only
  // 38 LTE only
  // 51 GSM and LTE only
  res = modem.setNetworkMode(2);
  SerialMon.print("setNetworkMode: ");
  SerialMon.println(res);
}

void loop() {
  refreshAir();

  String gasConcentration = String(gas.readGasConcentrationPPM());

  bool success = connectAndSendData(gasConcentration);
  if (!success) {
    delay(60000);
    return;
  }

  // Shutdown
  disconnectAndShutdown();

  // sleep for 5 minutes
  ESP.deepSleep(300e6);
}
