#include "sensors.h"

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

  dht.begin();
  ds18b20.begin();

  if (USE_A02YYUW) {
    mySerial.begin(9600, SERIAL_8N1, SENSOR_RX_PIN, -1);  // RX only
  }else{
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
  }
}

void readSensorsLoop() {
  unsigned long now = millis();

  if (now - lastDhtRead > temp_refresh_ms) {
    readDHT();
    lastDhtRead = now;
  }

  if (now - lastDs18Read > temp_refresh_ms) {
    readDS18B20();
    lastDs18Read = now;
  }

  if (USE_A02YYUW) {
   last_distance = readA02YYUW();
  }
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    dhtFailCount++;
  } else {
    outside_temp = t;
    outside_humi = h;
    dhtFailCount = 0;
  }

  if (dhtFailCount >= maxDhtFail) {
    dht.begin();
    dhtFailCount = 0;
  }
}

void readDS18B20() {
  ds18b20.requestTemperatures();
  current_temp = ds18b20.getTempCByIndex(0);
}

float readSR04M() {
  float sum = 0;
  const int samples = 5;
  for (int i = 0; i < samples; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    float distance = duration * 0.0343 / 2.0;
    sum += distance;
    delay(10);
  }
  return sum / samples;
}

float readA02YYUW() {
  while (mySerial.available() >= 4) {
    if (mySerial.peek() == 0xFF) {
      byte buffer[4];
      mySerial.readBytes(buffer, 4);

      if (buffer[0] == 0xFF) {
        int distance = (buffer[1] << 8) | buffer[2];
        if (distance >= 30 && distance <= 7500) {
          return distance / 10.0;  // mm to cm
        }
      }
    } else {
      mySerial.read();
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
  if (USE_A02YYUW) {
    return last_distance;
  } else {
    return readSR04M();
  }
}

void handleCalibrate() {
  eau_max_cm = tank_height_cm - measureDistance();
  prefs.begin("cuve", false);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.end();
  server.sendHeader("Location", "/");
  server.send(303);
}