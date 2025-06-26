#include "sensors.h"
#include "../services/logger.h"

// DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// DHT22
DHT dht(DHTPIN, DHTTYPE);

// A02YYUW via Serial2
HardwareSerial mySerial(2);  // UART2 (GPIO26 RX)

unsigned long lastDhtRead = 0;
unsigned long lastDs18Read = 0;
float last_distance = 0;

int dhtFailCount = 0;
const int maxDhtFail = 4;

void initSensors() {
  LOG_INFO("SENSORS", "Initializing sensors...");

  dht.begin();
  LOG_INFO("SENSORS", "DHT22 initialized on pin " + String(DHTPIN));
  
  ds18b20.begin();
  LOG_INFO("SENSORS", "DS18B20 initialized on pin " + String(ONE_WIRE_BUS));

  if (ultrasonic_mode == 2) {
    mySerial.begin(9600, SERIAL_8N1, SENSOR_RX_PIN, -1);  // A02YYUW
    LOG_INFO("SENSORS", "A02YYUW initialized on RX pin " + String(SENSOR_RX_PIN));
  } else if (ultrasonic_mode == 1) {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    LOG_INFO("SENSORS", "SR04M initialized - TRIG: " + String(TRIG_PIN) + ", ECHO: " + String(ECHO_PIN));
  }
}

void readSensorsLoop() {
  unsigned long now = millis();

  if (use_dht && now - lastDhtRead > temp_refresh_ms) {
    readDHT();
    lastDhtRead = now;
  }

  if (use_ds18b20 && now - lastDs18Read > temp_refresh_ms) {
    readDS18B20();
    lastDs18Read = now;
  }

  if (ultrasonic_mode == 2) {
    float newDistance = readA02YYUW();
    if (newDistance > 0) {
      last_distance = newDistance;
    }
  }
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    dhtFailCount++;
    LOG_WARNING("DHT22", "Read failed (count: " + String(dhtFailCount) + "/" + String(maxDhtFail) + ")");
  } else {
    outside_temp = t;
    outside_humi = h;
    dhtFailCount = 0;
    //LOG_DEBUG("DHT22", "Temp: " + String(t, 1) + "°C, Humidity: " + String(h, 1) + "%");
  }

  if (dhtFailCount >= maxDhtFail) {
    LOG_ERROR("DHT22", "Max failures reached, reinitializing...");
    dht.begin();
    dhtFailCount = 0;
  }
}

void readDS18B20() {
  ds18b20.requestTemperatures();
  float temp = ds18b20.getTempCByIndex(0);
  
  if (temp == DEVICE_DISCONNECTED_C) {
    LOG_ERROR("DS18B20", "Sensor disconnected or read error");
  } else {
    current_temp = temp;
    //LOG_DEBUG("DS18B20", "Water temp: " + String(temp, 1) + "°C");
  }
}

float readSR04M() {
  //LOG_DEBUG("SR04M", "Starting measurement...");
  float sum = 0;
  int validReadings = 0;
  const int samples = 5;
  
  for (int i = 0; i < samples; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    
    if (duration > 0) {
      float distance = duration * 0.0343 / 2.0;
      sum += distance;
      validReadings++;
      //LOG_DEBUG("SR04M", "Sample " + String(i) + ": " + String(distance, 1) + " cm");
    } else {
      LOG_WARNING("SR04M", "Sample " + String(i) + " timeout");
    }
    
    delay(10);
  }
  
  if (validReadings > 0) {
    float avg = sum / validReadings;
    //LOG_INFO("SR04M", "Average distance: " + String(avg, 1) + " cm (" + String(validReadings) + " valid samples)");
    return avg;
  } else {
    LOG_ERROR("SR04M", "No valid readings");
    return -1.0;
  }
}

float readA02YYUW() {
  static unsigned long lastGoodReading = 0;
  static int consecutiveErrors = 0;

  const unsigned long timeout = 100;  // Timeout for waiting full packet (ms)
  unsigned long startMillis = millis();

  while (millis() - startMillis < timeout) {
    // Ensure we have at least 4 bytes
    if (mySerial.available() >= 4) {
      if (mySerial.peek() == 0xFF) {
        byte buffer[4];
        mySerial.readBytes(buffer, 4);

        int distance = (buffer[1] << 8) | buffer[2];
        byte checksum = (buffer[0] + buffer[1] + buffer[2]) & 0xFF;

        if (checksum == buffer[3]) {
          if (distance >= 30 && distance <= 7500) {
            float distanceCm = distance / 10.0;
            lastGoodReading = millis();
            consecutiveErrors = 0;
            LOG_INFO("A02YYUW", "Valid reading: " + String(distanceCm, 1) + " cm");
            return distanceCm;
          } else {
            LOG_WARNING("A02YYUW", "Distance out of range: " + String(distance));
          }
        } else {
          LOG_WARNING("A02YYUW", "Checksum error");
        }
      } else {
        // Discard bytes until we find 0xFF
        mySerial.read();
      }
    }
  }

  if (millis() - lastGoodReading > 5000) {
    consecutiveErrors++;
    if (consecutiveErrors % 10 == 0) {
      LOG_ERROR("A02YYUW", "No valid readings for " + String((millis() - lastGoodReading) / 1000) + " seconds");
    }
  }

  return -1.0;
}



float calculateVolumeLiters(float height_cm) {
  if (tank_shape == 1) {  // Cylinder
    return (PI * pow(tank_diameter_cm / 2, 2) * height_cm) / 1000.0;
  } else {
    return (tank_length_cm * tank_width_cm * height_cm) / 1000.0;
  }
}

float measureDistance() {
  if (ultrasonic_mode == 2) {
    return last_distance;
  } else if (ultrasonic_mode == 1) {
    return readSR04M();
  } else {
    return -1.0;
  }
}

void handleCalibrate() {
  LOG_INFO("CALIB", "Starting calibration...");
  float distance = measureDistance();
  
  if (distance > 0) {
    eau_max_cm = tank_height_cm - distance;
    prefs.begin("cuve", false);
    prefs.putFloat("eau_max", eau_max_cm);
    prefs.end();
    
    LOG_INFO("CALIB", "Calibration successful. Max water level: " + String(eau_max_cm, 1) + " cm");
  } else {
    LOG_ERROR("CALIB", "Calibration failed - invalid distance reading");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}