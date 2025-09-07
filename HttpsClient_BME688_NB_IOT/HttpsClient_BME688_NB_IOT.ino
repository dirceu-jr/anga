// TODO:
// - Ping ThingSpeak Generic - DONE;
// - Disable Wi-Fi - DONE;
// - Disable Bluetooth - DONE;
// - Check/disable GPS - DONE;
// - ESP32 deep sleep - DONE;
// - Bosch BME68x data - DONE;
// - Modem work better without deep-sleep - DONE;
// - Refresh air inside the case (power on fan for some time) - DONE;
// - May deep-sleep in errors (so a "reboot" occurs);

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
// #define TINY_GSM_RX_BUFFER 1024

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
const char server[] = "api.thingspeak.com";
const int port = 443;

const char* writeAPIKey = "7OETW4UYYTX3NAZF";

#include "esp_wifi.h"
#include "esp_bt.h"

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#include <bsec2.h>

// Create an object of the class Bsec2
Bsec2 envSensor;

// LP (low power) = 3 seconds / ULP (ultra low power) = 300 seconds
#define SAMPLE_RATE BSEC_SAMPLE_RATE_ULP

struct EnvSensorData {
  float iaq;
  int iaq_accuracy;
  float temperature;
  float humidity;
  float static_iaq;
  float co2_equivalent;
  float breath_voc_equivalent;
  float compensated_gas;
};

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClientSecure client(modem);
HttpClient          http(client, server, port);

#define UART_BAUD   115200
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_PIN     12
#define FAN_PIN     0

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

void checkBsecStatus(Bsec2 bsec) {
  if (bsec.status < BSEC_OK) {
    Serial.println("BSEC error code : " + String(bsec.status));
  } else if (bsec.status > BSEC_OK) {
    Serial.println("BSEC warning code : " + String(bsec.status));
  }

  if (bsec.sensor.status < BME68X_OK) {
    Serial.println("BME68X error code : " + String(bsec.sensor.status));
  } else if (bsec.sensor.status > BME68X_OK) {
    Serial.println("BME68X warning code : " + String(bsec.sensor.status));
  }
}

void startEnvSensor() {
  // Desired subscription list of BSEC2 outputs
  bsecSensor sensorList[] = {
    BSEC_OUTPUT_IAQ,
    // BSEC_OUTPUT_RAW_TEMPERATURE,
    // BSEC_OUTPUT_RAW_PRESSURE,
    // BSEC_OUTPUT_RAW_HUMIDITY,
    // BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    // BSEC_OUTPUT_GAS_PERCENTAGE,
    BSEC_OUTPUT_COMPENSATED_GAS
  };

  // for I2C
  Wire.begin();

  // Initialize the library and interfaces
  if (!envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
    checkBsecStatus(envSensor);
  }

  /*
	 *	The default offset provided has been determined by testing the sensor in LP and ULP mode on application board 3.0
	 *	Please update the offset value after testing this on your product 
	 */
	if (SAMPLE_RATE == BSEC_SAMPLE_RATE_ULP) {
		envSensor.setTemperatureOffset(TEMP_OFFSET_ULP);
	} else if (SAMPLE_RATE == BSEC_SAMPLE_RATE_LP) {
		envSensor.setTemperatureOffset(TEMP_OFFSET_LP);
	}

  // Subsribe to the desired BSEC2 outputs
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), SAMPLE_RATE)) {
    checkBsecStatus(envSensor);
  }

  Serial.println("BSEC library version " + \
    String(envSensor.version.major) + "." \
    + String(envSensor.version.minor) + "." \
    + String(envSensor.version.major_bugfix) + "." \
    + String(envSensor.version.minor_bugfix));
}

EnvSensorData readSensorData() {
  const bsecOutputs* outputs = nullptr;
  EnvSensorData data;

  while (true) {
    if (!envSensor.run()) {
      checkBsecStatus(envSensor);
    }

    outputs = envSensor.getOutputs();
    if (outputs && outputs->nOutputs) {
      break;
    }
    delay(100); // Small delay to avoid busy loop
  }

  // Serial.println("BSEC outputs:\n\tTime stamp = " + String((int) (outputs->output[0].time_stamp / INT64_C(1000000))));
  for (uint8_t i = 0; i < outputs->nOutputs; i++) {
    const bsecData output  = outputs->output[i];
    switch (output.sensor_id) {
      case BSEC_OUTPUT_IAQ:
        data.iaq = output.signal;
        data.iaq_accuracy = output.accuracy;
        Serial.println("\tIAQ = " + String(output.signal));
        Serial.println("\tIAQ accuracy = " + String((int) output.accuracy));
        break;
      // case BSEC_OUTPUT_RAW_TEMPERATURE:
      //   Serial.println("\tTemperature = " + String(output.signal));
      //   break;
      // case BSEC_OUTPUT_RAW_PRESSURE:
      //   Serial.println("\tPressure = " + String(output.signal));
      //   break;
      // case BSEC_OUTPUT_RAW_HUMIDITY:
      //   Serial.println("\tHumidity = " + String(output.signal));
      //   break;
      // case BSEC_OUTPUT_RAW_GAS:
      //   Serial.println("\tGas resistance = " + String(output.signal));
      //   break;
      case BSEC_OUTPUT_STABILIZATION_STATUS:
        Serial.println("\tStabilization status = " + String(output.signal));
        break;
      case BSEC_OUTPUT_RUN_IN_STATUS:
        Serial.println("\tRun in status = " + String(output.signal));
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        data.temperature = output.signal;
        Serial.println("\tCompensated temperature = " + String(output.signal));
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        data.humidity = output.signal;
        Serial.println("\tCompensated humidity = " + String(output.signal));
        break;
      case BSEC_OUTPUT_STATIC_IAQ:
        data.static_iaq = output.signal;
        Serial.println("\tStatic IAQ = " + String(output.signal));
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        data.co2_equivalent = output.signal;
        Serial.println("\tCO2 Equivalent = " + String(output.signal));
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        data.breath_voc_equivalent = output.signal;
        Serial.println("\tbVOC equivalent = " + String(output.signal));
        break;
      // case BSEC_OUTPUT_GAS_PERCENTAGE:
      //   Serial.println("\tGas percentage = " + String(output.signal));
      //   break;
      case BSEC_OUTPUT_COMPENSATED_GAS:
        data.compensated_gas = output.signal;
        Serial.println("\tCompensated gas = " + String(output.signal));
        break;
      default:
        break;
    }
  }
  return data;
}

void setup() {
  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Fan off
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  // Set console baud rate
  SerialMon.begin(115200);
  delay(1000);

  // Disable Wi-Fi and Bluetooth
  SerialMon.println(F("Disabling Wi-Fi and Bluetooth..."));
  esp_wifi_stop();
  esp_bt_controller_disable();

  // BSEC initialization
  startEnvSensor();

  // modem
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
  modem.sendAT("+CCLK=\"25/09/05,22:38:00\"");

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
  unsigned long loop_start_time = millis();
  SerialMon.println("Loop start...");

  // Refresh Air (fan on for 10 seconds)
  digitalWrite(FAN_PIN, HIGH);
  delay(10000);
  digitalWrite(FAN_PIN, LOW);

  // Read sensor data
  EnvSensorData reading = readSensorData();

  modemPowerOn();
  delay(5000);

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
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS already connected.");
  } else {
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
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
  String resource = String("/update?api_key=") + writeAPIKey +
                    "&field1=" + String(reading.iaq) +
                    "&field2=" + String(reading.iaq_accuracy) +
                    "&field3=" + String(reading.temperature) +
                    "&field4=" + String(reading.humidity) +
                    "&field5=" + String(reading.static_iaq) +
                    "&field6=" + String(reading.co2_equivalent) +
                    "&field7=" + String(reading.breath_voc_equivalent) +
                    "&field8=" + String(reading.compensated_gas);

  SerialMon.print(F("Performing HTTPS GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    SerialMon.print(F("failed to connect, error: "));
    SerialMon.println(err);
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  // Shutdown

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

  unsigned long loop_duration = millis() - loop_start_time;

  const unsigned long total_cycle_time_ms = 300 * 1000; // 5 minutes in milliseconds
  long sleep_duration_ms = total_cycle_time_ms - loop_duration;

  if (sleep_duration_ms < 0) {
    sleep_duration_ms = 0; // Avoid negative sleep time
  }

  // Light Sleep for the remainder of the 5-minute cycle
  esp_sleep_enable_timer_wakeup(sleep_duration_ms * 1000); // Time in microseconds
  esp_light_sleep_start();

  // Sleep for 5 seconds (for testing purposes)
  // delay(5000);
  \
}
