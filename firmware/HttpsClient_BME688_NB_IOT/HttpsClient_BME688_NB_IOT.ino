// TODO:
// - May deep-sleep in errors (so a "reboot" occurs)?;

// Select the modem
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

#include "esp_wifi.h"
#include "esp_bt.h"

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#include <bsec2.h>

// Your GPRS credentials, if any
const char apn[] = "iot.datatem.com.br";
const char gprsUser[] = "datatem";
const char gprsPass[] = "datatem";

// Server details
const char server[] = "api.thingspeak.com";
const int port = 443;

const char* writeAPIKey = "API Key";

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

void checkBsecStatus(const Bsec2 &bsec) {
  if (bsec.status < BSEC_OK) {
    SerialMon.print(F("BSEC error code : "));
    SerialMon.println(bsec.status);
  } else if (bsec.status > BSEC_OK) {
    SerialMon.print(F("BSEC warning code : "));
    SerialMon.println(bsec.status);
  }

  if (bsec.sensor.status < BME68X_OK) {
    SerialMon.print(F("BME68X error code : "));
    SerialMon.println(bsec.sensor.status);
  } else if (bsec.sensor.status > BME68X_OK) {
    SerialMon.print(F("BME68X warning code : "));
    SerialMon.println(bsec.sensor.status);
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

  SerialMon.printf("BSEC library version %d.%d.%d.%d\n",
    envSensor.version.major,
    envSensor.version.minor,
    envSensor.version.major_bugfix,
    envSensor.version.minor_bugfix);
}

EnvSensorData readSensorData() {
  const bsecOutputs* outputs = nullptr;
  EnvSensorData data = {};

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

  // SerialMon.println("BSEC outputs:\n\tTime stamp = " + String((int) (outputs->output[0].time_stamp / INT64_C(1000000))));
  for (uint8_t i = 0; i < outputs->nOutputs; i++) {
    const bsecData output  = outputs->output[i];
    switch (output.sensor_id) {
      case BSEC_OUTPUT_IAQ:
        data.iaq = output.signal;
        data.iaq_accuracy = output.accuracy;
        SerialMon.print(F("\tIAQ = "));
        SerialMon.println(output.signal);
        SerialMon.print(F("\tIAQ accuracy = "));
        SerialMon.println((int) output.accuracy);
        break;
      // case BSEC_OUTPUT_RAW_TEMPERATURE:
      //   SerialMon.print(F("\tTemperature = "));
      //   SerialMon.println(output.signal);
      //   break;
      // case BSEC_OUTPUT_RAW_PRESSURE:
      //   SerialMon.print(F("\tPressure = "));
      //   SerialMon.println(output.signal);
      //   break;
      // case BSEC_OUTPUT_RAW_HUMIDITY:
      //   SerialMon.print(F("\tHumidity = "));
      //   SerialMon.println(output.signal);
      //   break;
      // case BSEC_OUTPUT_RAW_GAS:
      //   SerialMon.print(F("\tGas resistance = "));
      //   SerialMon.println(output.signal);
      //   break;
      case BSEC_OUTPUT_STABILIZATION_STATUS:
        SerialMon.print(F("\tStabilization status = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_RUN_IN_STATUS:
        SerialMon.print(F("\tRun in status = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        data.temperature = output.signal;
        SerialMon.print(F("\tCompensated temperature = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        data.humidity = output.signal;
        SerialMon.print(F("\tCompensated humidity = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_STATIC_IAQ:
        data.static_iaq = output.signal;
        SerialMon.print(F("\tStatic IAQ = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        data.co2_equivalent = output.signal;
        SerialMon.print(F("\tCO2 Equivalent = "));
        SerialMon.println(output.signal);
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        data.breath_voc_equivalent = output.signal;
        SerialMon.print(F("\tbVOC equivalent = "));
        SerialMon.println(output.signal);
        break;
      // case BSEC_OUTPUT_GAS_PERCENTAGE:
      //   SerialMon.print(F("\tGas percentage = "));
      //   SerialMon.println(output.signal);
      //   break;
      case BSEC_OUTPUT_COMPENSATED_GAS:
        data.compensated_gas = output.signal;
        SerialMon.print(F("\tCompensated gas = "));
        SerialMon.println(output.signal);
        break;
      default:
        break;
    }
  }
  return data;
}

void disconnectAndPowerModemOff() {
  // Disconnect from the server
  http.stop();
  SerialMon.println(F("Server disconnected"));

  // Disconnect from GPRS
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));

  // Power down the modem using AT command first
  SerialMon.println(F("Powering down modem with AT command..."));
  modem.poweroff();
}

// Recirculate Air (fan on for 15 seconds)
void recirculateAir() {
  digitalWrite(FAN_PIN, HIGH);
  delay(15000);
  digitalWrite(FAN_PIN, LOW);
}

bool connectAndSendData(const EnvSensorData &reading) {
  SerialMon.print(F("Signal quality: "));
  SerialMon.println(modem.getSignalQuality());

  // Wait for network for 60 seconds
  SerialMon.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    SerialMon.println(F(" fail"));
    return false;
  }
  SerialMon.println(F(" success"));

  if (modem.isNetworkConnected()) {
    SerialMon.println(F("Network connected"));
  }

  // GPRS connection parameters are usually set after network registration
  if (modem.isGprsConnected()) {
    SerialMon.println(F("GPRS already connected."));
  } else {
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(F(" fail"));
      return false;
    }
    SerialMon.println(F(" success"));
  }

  if (modem.isGprsConnected()) {
    SerialMon.println(F("GPRS connected."));
  }

  // Now try HTTP request
  http.setTimeout(30000);
  http.connectionKeepAlive(); // this may be needed for HTTPS

  // Construct the resource URL using a fixed buffer (no dynamic heap allocations)
  char resource[256];
  snprintf(resource, sizeof(resource),
           "/update?api_key=%s&field1=%.2f&field2=%d&field3=%.2f&field4=%.2f&field5=%.2f&field6=%.2f&field7=%.2f&field8=%.2f",
           writeAPIKey,
           reading.iaq,
           reading.iaq_accuracy,
           reading.temperature,
           reading.humidity,
           reading.static_iaq,
           reading.co2_equivalent,
           reading.breath_voc_equivalent,
           reading.compensated_gas);

  SerialMon.print(F("Performing HTTPS GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    SerialMon.print(F("failed to connect, error: "));
    SerialMon.println(err);
    return false;
  }

  // Check HTTP response status
  // int status = http.responseStatusCode();
  // SerialMon.print(F("Response status code: "));
  // SerialMon.println(status);
  // if (status <= 0) {
  //   return false;
  // }

  // Read HTTP response body (optional)
  // SerialMon.println(F("Response body:"));
  // while (http.available()) {
  //   char c = http.read();
  //   SerialMon.print(c);
  // }
  // SerialMon.println();

  return true;
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
  delay(200);

  // BSEC initialization
  startEnvSensor();

  // modem OFF/ON cycle
  modemHardReset();

  // Set GSM module baud rate
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(200);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Restarting modem...");
  modem.restart();
  // SerialMon.println("Initializing modem...");
  // modem.init();

  // Disable GPS
  modem.disableGPS();

  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    SerialMon.println("SGPIO=0,4,1,0 false");
  }

  SerialMon.print(F("Modem Name: "));
  SerialMon.println(modem.getModemName());

  SerialMon.print(F("Modem Info: "));
  SerialMon.println(modem.getModemInfo());

  // Check SIM status
  SerialMon.print(F("SIM Status: "));
  SerialMon.println(modem.getSimStatus());

  // 1 CAT-M
  // 2 NB-IoT
  // 3 CAT-M and NB-IoT
  SerialMon.print(F("setPreferredMode: "));
  SerialMon.println(modem.setPreferredMode(2));

  // 2 Automatic
  // 13 GSM only
  // 38 LTE only
  // 51 GSM and LTE only
  SerialMon.print(F("setNetworkMode: "));
  SerialMon.println(modem.setNetworkMode(38));
}

void loop() {
  unsigned long loop_start_time = millis();
  SerialMon.printf("Loop start... [Free Heap: %d, Max Block: %d]\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());

  recirculateAir();

  // Read sensor data
  EnvSensorData reading = readSensorData();

  modemPowerOn();
  delay(5000); // Wait for modem to boot

  // Connect to network, GPRS and send data
  bool success = connectAndSendData(reading);

  disconnectAndPowerModemOff();

  if (success) {
    SerialMon.println("Entering light sleep for success");
  } else {
    SerialMon.println("Entering light sleep for error");
  }

  // Calculate elapsed time
  unsigned long loop_duration = millis() - loop_start_time;

  // Calculate sleep duration to complete a 5-minute cycle
  const unsigned long total_cycle_time_ms = 300 * 1000; // 5 minutes in milliseconds
  long sleep_duration_ms = total_cycle_time_ms - loop_duration;

  // Avoid negative sleep time
  if (sleep_duration_ms < 0) {
    sleep_duration_ms = 0;
  }

  // Light Sleep for the remainder of the 5-minute cycle
  esp_sleep_enable_timer_wakeup(sleep_duration_ms * 1000); // Time in microseconds
  esp_light_sleep_start();
}
