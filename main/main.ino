#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>

const char* mqtt_server = "192.168.1.100";  // Change à ton broker MQTT
const char* mqtt_user = "homeassistant";
const char* mqtt_pass = "ton_mdp";

#define TRIG_PIN 5
#define ECHO_PIN 18

Preferences prefs;
float tank_height_cm = 120.0;
float tank_length_cm = 50.0; 
float tank_width_cm = 40.0;  
float eau_max_cm = 100.0;
float cuve_volume_l = 200.0;

WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.0343 / 2.0;
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Water Level Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; background: linear-gradient(to bottom, #a8eaff, #e6f9ff); color: #004f6d; padding: 2em; text-align: center; }
    .container { max-width: 600px; margin: auto; background: #fff; border-radius: 15px; padding: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    table { width: 100%; margin-bottom: 20px; }
    th, td { padding: 12px; border-bottom: 1px solid #ddd; }
    th { background: #00b4d8; color: white; }
    input, button { padding: 12px; border-radius: 6px; border: 1px solid #00b4d8; }
    button { background: #00b4d8; color: white; cursor: pointer; }
    button:hover { background: #0077b6; }
    form { background: #f0faff; padding: 15px; border-radius: 10px; }
    h2 { margin-bottom: 20px; }
  </style>
  <script>
    async function fetchData() {
      const res = await fetch('/data');
      const data = await res.json();
      document.getElementById('dist').innerText = data.distance + ' cm';
      document.getElementById('water').innerText = data.water_height + ' cm';
      document.getElementById('percent').innerText = data.percent + ' %';
      document.getElementById('liters').innerText = data.volume_liters + ' L';
    }
    setInterval(fetchData, 2000);
    window.onload = fetchData;
  </script>
</head>
<body>
  <div class="container">
    <h2>💧 Water Tank Monitor</h2>
    <table>
      <tr><th>Parameter</th><th>Value</th></tr>
      <tr><td>Distance (sensor → water)</td><td id="dist">--</td></tr>
      <tr><td>Water Height</td><td id="water">--</td></tr>
      <tr><td>Fill Percentage</td><td id="percent">--</td></tr>
      <tr><td>Estimated Volume</td><td id="liters">--</td></tr>
    </table>
    <form method="POST" action="/set">
      <label>Sensor Height (cm): </label><br>
      <input type="number" step="0.1" name="tank_height" value=")rawliteral" + String(tank_height_cm,1) + R"rawliteral("><br>
      <label>Tank Length (cm): </label><br>
      <input type="number" step="0.1" name="tank_length" value=")rawliteral" + String(tank_length_cm,1) + R"rawliteral("><br>
      <label>Tank Width (cm): </label><br>
      <input type="number" step="0.1" name="tank_width" value=")rawliteral" + String(tank_width_cm,1) + R"rawliteral("><br>
      <button type="submit">Update Tank Height</button>
    </form>
    <form method="POST" action="/calibrate">
      <button type="submit">Calibrate Max Water Height</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
server.send(200,"text/html",html);
}

void handleSet(){
  if(server.hasArg("tank_height")) tank_height_cm=server.arg("tank_height").toFloat();
  if(server.hasArg("tank_length")) tank_length_cm=server.arg("tank_length").toFloat();
  if(server.hasArg("tank_width")) tank_width_cm=server.arg("tank_width").toFloat();
  prefs.begin("cuve",false);
  prefs.putFloat("tank_height",tank_height_cm);
  prefs.putFloat("tank_length",tank_length_cm);
  prefs.putFloat("tank_width",tank_width_cm);
  prefs.end();
  server.sendHeader("Location","/");server.send(303);
}

void handleCalibrate(){
  eau_max_cm = tank_height_cm - measureDistance();
  prefs.begin("cuve",false);
  prefs.putFloat("eau_max",eau_max_cm);
  prefs.end();
  server.sendHeader("Location","/");server.send(303);
}

void handleData(){
  float distance = measureDistance();
  float water_height = max(0.0f, tank_height_cm - distance);
  float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
  float volume_liters = (tank_length_cm * tank_width_cm * water_height) / 1000.0;
  
  if (mqtt.connected()) {
  mqtt.publish("cuve/water_distance", String(distance, 1).c_str());
  mqtt.publish("cuve/water_height", String(water_height, 1).c_str());
  mqtt.publish("cuve/water_percent", String(percent, 0).c_str());
  mqtt.publish("cuve/water_volume", String(volume_liters, 1).c_str());
  };
  String json="{";
  json+="\"distance\":"+String(distance,1)+",";
  json+="\"water_height\":"+String(water_height,1)+",";
  json+="\"percent\":"+String(percent,0)+",";
  json+="\"volume_liters\":"+String(volume_liters,1)+"}";
  server.send(200," ",json);
}

void mqttReconnect(){
  while(!mqtt.connected()){mqtt.connect("esp32cuve",mqtt_user,mqtt_pass);delay(2000);}}

void setup(){
  Serial.begin(115200);
  pinMode(TRIG_PIN,OUTPUT);
  pinMode(ECHO_PIN,INPUT);

  prefs.begin("cuve",true);
  tank_height_cm=prefs.getFloat("tank_height",120.0);
  tank_length_cm=prefs.getFloat("tank_length",50.0);
  tank_width_cm=prefs.getFloat("tank_width",40.0);
  eau_max_cm=prefs.getFloat("eau_max",100.0);
  prefs.end();

  // Bloc WiFiManager
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); // Décommente cette ligne si besoin de réinitialiser la config WiFi
  wifiManager.autoConnect("ESP32_Config");
  Serial.println("WiFi connected: ");
  Serial.println(WiFi.localIP());

  mqtt.setServer(mqtt_server,1883);
  server.on("/",handleRoot);
  server.on("/set",HTTP_POST,handleSet);
  server.on("/calibrate",HTTP_POST,handleCalibrate);
  server.on("/data",handleData);
  server.begin();
}

void loop(){
  if(!mqtt.connected()) mqttReconnect(); mqtt.loop(); server.handleClient();}
