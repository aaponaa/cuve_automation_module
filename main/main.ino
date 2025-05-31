#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// DHT22
#define DHTPIN 33 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float outside_temp = 0.0;
float outside_humi = 0.0;

// SRD-05VDC-SL-C
#define RELAY_PIN 27

bool relay_state = false;
bool relay_inverted = true;

// AJ-SRO4M
#define TRIG_PIN 17
#define ECHO_PIN 16

// DS18B20
#define ONE_WIRE_BUS 18
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

Preferences prefs;

WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

//Mqtt
String mqtt_host = "";
String mqtt_username = "";
String mqtt_password = "";

bool relay_active_low = true;  // Set to true if relay is active LOW (default for most SRD modules)

struct TopicOption {
  const char* label;
  const char* topic_suffix;
  bool enabled;
};
TopicOption topicOptions[] = {
  {"Distance (sensor → water)", "water_distance", true},
  {"Water Height", "water_height", true},
  {"Fill Percentage", "water_percent", true},
  {"Estimated Volume", "water_volume", true},
  {"Temperature (Water)", "water_temp", true},
  {"Outside Temperature", "outside_temp", true},
  {"Outside Humidity", "outside_humi", true},
  {"Pump State", "relay_state", true}
};
const int NUM_TOPICS = sizeof(topicOptions)/sizeof(TopicOption);


int temp_refresh_ms = 2000;
int mqtt_publish_ms = 5000;

unsigned long lastMqttAttempt = 0;

// Tank Data
String tank_name = "WaterTank";
int tank_shape = 0;
float current_temp = 0.0;
float tank_diameter_cm = 0;
float tank_height_cm = 0;
float tank_length_cm = 0;
float tank_width_cm = 0;
float eau_max_cm = 0;
float cuve_volume_l = 0;

String clientId = "ESP32Client-" + tank_name;

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.0343 / 2.0;
}  


// Web HTML
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <title>Water Level Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
  body {
    font-family: 'Segoe UI', Arial, sans-serif;
    background: linear-gradient(135deg, #e0f7fa 0%, #a7c8ff 100%);
    color: #003049;
    margin: 0;
    min-height: 100vh;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .container {
    max-width: 420px;
    width: 95vw;
    margin: 2em auto;
    background: rgba(255, 255, 255, 0.75);
    backdrop-filter: blur(9px) saturate(140%);
    border-radius: 22px;
    box-shadow: 0 6px 32px rgba(50, 130, 184, 0.20);
    padding: 2.2em 1.3em 2em 1.3em;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  h2 {
    margin-bottom: 26px;
    font-weight: 700;
    letter-spacing: 0.01em;
    color: #0077b6;
    text-shadow: 0 2px 8px rgba(0,180,216,0.08);
    display: flex;
    align-items: center;
    gap: 0.4em;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    margin-bottom: 20px;
    box-shadow: 0 2px 10px rgba(0,180,216,0.04);
    background: rgba(255,255,255,0.6);
    border-radius: 12px;
    overflow: hidden;
  }

  th, td {
    padding: 15px 8px;
    text-align: left;
  }

  th {
    background: #00b4d8;
    color: #fff;
    font-size: 1.05rem;
    letter-spacing: 0.02em;
    font-weight: 600;
  }

  tr:not(:last-child) td {
    border-bottom: 1px solid #e1e8ed;
  }

  td {
    font-size: 1.03rem;
    color: #00506e;
    font-weight: 500;
  }

  a {
    color: #0077b6;
    font-weight: 600;
    text-decoration: none;
    margin-bottom: 1.1em;
    display: inline-block;
    transition: color 0.18s;
  }

  a:hover {
    color: #023e8a;
    text-decoration: underline;
  }

  form {
    padding: 16px;
    border-radius: 10px;
    margin-top: 10px;
    width: 100%;
    display: flex;
    justify-content: center;
  }

  button[type="submit"] {
    padding: 11px 25px;
    border-radius: 7px;
    border: none;
    background: linear-gradient(90deg,#0096c7,#00b4d8);
    color: #fff;
    font-weight: 600;
    font-size: 1rem;
    cursor: pointer;
    transition: background 0.18s, transform 0.10s;
    box-shadow: 0 2px 12px rgba(0,180,216,0.10);
  }

  button[type="submit"]:hover {
    background: linear-gradient(90deg,#0077b6,#0096c7);
    transform: translateY(-1px) scale(1.03);
  }

  /* Responsive for mobile */
  @media (max-width: 650px) {
    .container {
      padding: 1em 0.2em 1.2em 0.2em;
      border-radius: 12px;
    }
    table th, table td {
      padding: 9px 4px;
      font-size: 0.98rem;
    }
    h2 {
      font-size: 1.1rem;
    }
  }

  </style>
  <script>
    async function fetchData() {
      const res = await fetch('/data');
      const data = await res.json();
      document.getElementById('temp').innerText = data.temp !== undefined ? data.temp + ' °C' : '--';
      document.getElementById('outside_temp').innerText = (data.outside_temp !== undefined && data.outside_temp > -50) ? data.outside_temp + ' °C' : '--';
      document.getElementById('outside_humi').innerText = (data.outside_humi !== undefined && data.outside_humi >= 0) ? data.outside_humi + ' %' : '--';
      document.getElementById('dist').innerText = data.distance + ' cm';
      document.getElementById('water').innerText = data.water_height + ' cm';
      document.getElementById('percent').innerText = data.percent + ' %';
      document.getElementById('liters').innerText = data.volume_liters + ' L';
      const pumpCell = document.getElementById('pumpstate');
      if (pumpCell) {
        // Inverse l'affichage si le relais est actif à l'état bas
        const isPumpOn = !data.relay_state;  // inversé
        pumpCell.innerText = isPumpOn ? "ON" : "OFF";
        pumpCell.style.fontWeight = "bold";

        const pumpBtn = document.getElementById('pumpBtn');
        if (pumpBtn) pumpBtn.style.background = isPumpOn ? "#00b4d8" : "#adb5bd";
      }

    }

    function togglePump() {
      fetch('/relay_toggle', {method:'POST'})
        .then(_=>fetchData());
    }

    setInterval(fetchData, 2000);
    window.onload = fetchData;

          document.addEventListener('DOMContentLoaded', function(){
        var form = document.getElementById('calibForm');
        form.onsubmit = function(ev){
          if(!confirm("Are you sure you want to calibrate the maximum water height?\nThis will overwrite the current calibration.")){
            ev.preventDefault();
            return false;
          }
        };
      });
  </script>
  </head>
  <body>
  <div class="container">
    <h2>💧 <span id="tankname">)rawliteral"
                + tank_name + R"rawliteral(</span> Monitor</h2>
    <table>
      <tr><th>Parameter</th><th>Value</th></tr>
      <tr><td>Temperature</td><td id="temp">--</td></tr>
      <tr><td>Outside Temperature</td><td id="outside_temp">--</td></tr>
      <tr><td>Outside Humidity</td><td id="outside_humi">--</td></tr>
      <tr><td>Distance (sensor → water)</td><td id="dist">--</td></tr>
      <tr><td>Water Height</td><td id="water">--</td></tr>
      <tr><td>Fill Percentage</td><td id="percent">--</td></tr>
      <tr><td>Estimated Volume</td><td id="liters">--</td></tr>
      <tr>
        <td>Pump State</td>
        <td>
          <button id="pumpBtn" onclick="togglePump()" style="padding:7px 24px;border-radius:8px;background:#00b4d8;color:#fff;font-weight:600;border:none;box-shadow:0 2px 6px #90e0ef88;cursor:pointer;">
            <span id="pumpstate">--</span>
          </button>
        </td>
      </tr>
    </table>
      <a href="/settings">⚙️ Settings -></a>
      <span></span>
    <form id="calibForm" method="POST" action="/calibrate">
      <button type="submit" id="calibBtn">Calibrate Max Water Height</button>
    </form>

  </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSettings() {
  String html = R"rawliteral(
 <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <title>Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      margin: 0;
      min-height: 100vh;
      padding: 0;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .container {
      max-width: 500px;
      width: 97vw;
      margin: 2.4em auto;
      background: rgba(255,255,255,0.77);
      border-radius: 24px;
      box-shadow: 0 6px 32px rgba(50,130,184,0.18);
      padding: 1.5em 1.3em 1.5em 1.3em;
      backdrop-filter: blur(9px) saturate(140%);
    }
    h2 {
      font-weight: 700;
      color: #0077b6;
      letter-spacing: 0.01em;
      margin: 0.5em;
      text-align: center;
    }

    form {
      display: flex;
      flex-direction: column;
    }
    .setting-section {
      background: rgba(255,255,255,0.63);
      border-radius: 18px;
      box-shadow: 0 2px 12px rgba(0,180,216,0.08);
      padding: 1.5em 1em 1em 1em;
      margin-bottom: 10px;
      display: flex;
      flex-direction: column;
      position: relative;
      transition: box-shadow 0.2s;
    }
    .setting-section:hover {
      box-shadow: 0 4px 20px rgba(0,180,216,0.14);
    }
    .section-title {
      display: flex;
      align-items: center;
      font-size: 1.1em;
      font-weight: 600;
    x  margin-bottom: 0.7em;
      color: #0096c7;
      gap: 0.5em;
      letter-spacing: 0.01em;
    }
    label {
      font-weight: 600;
      font-size: 1em;
      color: #00506e;
      margin-bottom: 4px;
      margin-top: 0.5em;
      text-align: left;
      display: block;
    }
    input {
      width: 100%;
      padding: 5px;
      border-radius:  5px;
      border: 1px solid #b1c2d6;
      font-size: 1.02em;
      background: rgba(245,250,255,0.60);
      box-sizing: border-box;
      transition: border 0.15s;
    }
    input:focus {
      outline: none;
      border: 1.5px solid #00b4d8;
      background: #f3fbff;
    }
    button[type="submit"] {
      padding: 13px 32px;
      border-radius: 8px;
      border: none;
      background: linear-gradient(90deg,#0096c7,#00b4d8);
      color: #fff;
      font-weight: 600;
      font-size: 1.09rem;
      cursor: pointer;
      margin-top: 10px;
      margin-bottom: 8px;
      box-shadow: 0 2px 12px rgba(0,180,216,0.08);
      transition: background 0.16s, transform 0.10s;
    }
    button[type="submit"]:hover {
      background: linear-gradient(90deg,#0077b6,#0096c7);
      transform: translateY(-1px) scale(1.03);
    }
    a {
      color: #0077b6;
      font-weight: 600;
      text-decoration: none;
      display: inline-block;
      margin: 0.9em 0 0 0;
      font-size: 1.06em;
      transition: color 0.17s;
    }
    a:hover {
      color: #023e8a;
      text-decoration: underline;
    }
    @media (max-width: 650px) {
      .container {
        padding: 1em 0.1em 1.2em 0.1em;
        border-radius: 14px;
      }
      .setting-section {
        padding: 1.1em 0.7em 0.8em 0.7em;
        border-radius: 10px;
      }
      h2 { font-size: 1.25rem; }
      .section-title { font-size: 1em; }
    }
    select {
      padding: 10px 16px;
      border-radius: 8px;
      border: 1.5px solid #48cae4;
      background: linear-gradient(90deg, #caf0f8 60%, #90e0ef 100%);
      color: #005077;
      font-size: 1.08em;
      font-weight: 500;
      box-shadow: 0 2px 6px rgba(72,202,228,0.10);
      transition: border-color 0.2s, box-shadow 0.2s;
      margin-bottom: 14px;
      outline: none;
    }
    select:focus {
      border-color: #0077b6;
      box-shadow: 0 0 6px #00b4d8aa;
    }
    option {
      background: #e0fbfc;
      color: #005077;
      font-weight: 500;
    }
    .section-title {
      margin-bottom: 10px;
      color: #0096c7;
      font-size: 1.19em;
      font-weight: 600;
      letter-spacing: 0.05em;
      text-shadow: 0 2px 4px #b7e5f8;
    }
  </style>
  </head>
  <body>
    <div class="container">
      <h2>⚙️ Settings</h2>
        
        <!-- Tank Settings Section -->
        <div class="setting-section">
        <form method="POST" action="/settings">
          <div class="section-title">🛢️ Tank Settings</div>
          <label>Tank Name:</label>
          <input type="text" name="tank_name" maxlength="32" value=")rawliteral"
                + tank_name + R"rawliteral(" required>
          <label>Sensor Height (cm):</label>
          <input type="number" step="0.1" name="tank_height" value=")rawliteral"
                + String(tank_height_cm, 1) + R"rawliteral(" required>
        )rawliteral";
  html += "<label>Tank Shape:</label>";
  html += "<select name='tank_shape' id='tank_shape'>";
  html += "<option value='0'";
  if (tank_shape == 0) html += " selected";
  html += ">Rectangular</option>";
  html += "<option value='1'";
  if (tank_shape == 1) html += " selected";
  html += ">Round (cylinder)</option>";
  html += "</select>";

  html += "<div id='rect_fields'>";
  html += "<label>Tank Length (cm):</label>";
  html += "<input type='number' step='0.1' name='tank_length' value='" + String(tank_length_cm, 1) + "'>";
  html += "<label>Tank Width (cm):</label>";
  html += "<input type='number' step='0.1' name='tank_width' value='" + String(tank_width_cm, 1) + "'>";
  html += "</div>";

  html += "<div id='cyl_fields'>";
  html += "<label>Tank Diameter (cm):</label>";
  html += "<input type='number' step='0.1' name='tank_diameter' value='" + String(tank_diameter_cm, 1) + "'>";
  html += "</div>";
  html += R"rawliteral(

          <button type="submit">Save</button>
          </form>
        </div>

        <!-- MQTT Settings Section -->
        <div class="setting-section">
          <div class="section-title">📡 MQTT Settings</div>
          <form id="mqttForm" method="POST" action="/mqtt_connect" style="margin-bottom:0">
            <label>MQTT Server:</label>
            <input type="text" name="mqtt_host" value=")rawliteral"
          + mqtt_host + R"rawliteral(" required>
            <label>MQTT Username:</label>
            <input type="text" name="mqtt_user" value=")rawliteral"
          + mqtt_username + R"rawliteral(">
            <label>MQTT Password:</label>
            <input type="password" name="mqtt_pass" value=")rawliteral"
          + mqtt_password + R"rawliteral(" autocomplete="off">
            <div style="margin-top:1em;">
              <span id="mqttStatus" style="font-weight:bold;">Status: Checking...</span>
              <button type="submit" style="margin-left:12px;">Connect</button>
            </div>
          </form>)rawliteral";

  html += "<div class=\"setting-section\">";
  html += "<div class=\"section-title\">Topics</div>";
  html += "<form method=\"POST\" action=\"/topics\">";
  html += "<table style=\"width:100%;\">";
  for (int i = 0; i < NUM_TOPICS; ++i) {
    html += "<tr><td>";
    // Display prefix + suffix (full topic)
    html += tank_name;
    html += "/";
    html += topicOptions[i].topic_suffix;
    html += "</td><td style='text-align:right'><input type='checkbox' name='enable_";
    html += topicOptions[i].topic_suffix;
    html += "' ";
    if (topicOptions[i].enabled) html += "checked";
    html += "></td></tr>";
  }
  html += "</table>";

  html += "<label>MQTT publish period (seconds):</label>";
  html += "<input type='number' min='1' max='3600' name='mqtt_publish' value='" + String(mqtt_publish_ms / 1000) + "' required>";

  html += "<label>Temperature read period (seconds):</label>";
  html += "<input type='number' min='1' max='3600' name='temp_refresh' value='" + String(temp_refresh_ms / 1000) + "' required>";

  html += "<button type='submit'>Save</button>";
  html += "</form></div>";


  html += R"rawliteral(</div>

        <div class="setting-section">
          <div class="section-title">🛠️ Factory Reset</div>
            <form method='POST' action='/factory_reset' onsubmit=\"return confirm('Are you sure you want to factory reset? This will erase all settings and WiFi config!');\">
            <button type='submit' style='background:#e63946; color:#fff; margin-top:18px; border-radius:8px;'>Factory Reset</button>
            </form>
        </div>

        <div class="setting-section">
          <div class="section-title">🛠️ Other Topic</div>
          ...
        </div>

      <a href="/">← Back to Dashboard</a>
    </div>
    <script>
      function updateMqttStatus() {
        fetch('/mqtt_status')
          .then(r=>r.json())
          .then(j=>{
            let el = document.getElementById('mqttStatus');
            if(j.connected) {
              el.textContent = "Status: Connected ✅";
              el.style.color = "#28a745";
            } else {
              el.textContent = "Status: Disconnected ❌";
              el.style.color = "#ff0033";
            }
          });
      }
      function connectMQTT() {
        fetch('/mqtt_connect', {method:'POST'}).then(updateMqttStatus);
      }
      window.onload = updateMqttStatus;
      setInterval(updateMqttStatus, 5000);

      // Intercepte le submit du formulaire MQTT pour l'AJAX + status
      document.getElementById("mqttForm").onsubmit = function(ev){
        ev.preventDefault();
        let formData = new FormData(ev.target);
        fetch('/mqtt_connect', {
          method:'POST',
          body:formData
        }).then(r=>r.json()).then(()=>{
          updateMqttStatus();
        });
        return false;
      };

      function updateTankFields() {
        var shape = document.getElementById('tank_shape').value;
        document.getElementById('rect_fields').style.display = (shape == '0') ? 'block' : 'none';
        document.getElementById('cyl_fields').style.display = (shape == '1') ? 'block' : 'none';
      }
      window.onload = updateTankFields;
      document.getElementById('tank_shape').onchange = updateTankFields;

    </script>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}


// Data Save Handle
void handleSettingsSave() {
  if (server.hasArg("tank_name")) tank_name = server.arg("tank_name");
  if (server.hasArg("tank_shape")) tank_shape = server.arg("tank_shape").toInt();
  if (server.hasArg("tank_diameter")) tank_diameter_cm = server.arg("tank_diameter").toFloat();
  if (server.hasArg("tank_height")) tank_height_cm = server.arg("tank_height").toFloat();
  if (server.hasArg("tank_length")) tank_length_cm = server.arg("tank_length").toFloat();
  if (server.hasArg("tank_width")) tank_width_cm = server.arg("tank_width").toFloat();

  prefs.begin("cuve", false);
  prefs.putInt("tank_shape", tank_shape);
  prefs.putFloat("tank_diameter", tank_diameter_cm);
  prefs.putString("tank_name", tank_name);
  prefs.putFloat("tank_height", tank_height_cm);
  prefs.putFloat("tank_length", tank_length_cm);
  prefs.putFloat("tank_width", tank_width_cm);
  prefs.end();

  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleTopicsSave() {
  prefs.begin("cuve", false);
  if (server.hasArg("temp_refresh")) temp_refresh_ms = server.arg("temp_refresh").toInt() * 1000;
  if (server.hasArg("mqtt_publish")) mqtt_publish_ms = server.arg("mqtt_publish").toInt() * 1000;
  prefs.putInt("temp_refresh_ms", temp_refresh_ms);
  prefs.putInt("mqtt_publish_ms", mqtt_publish_ms);
  for (int i = 0; i < NUM_TOPICS; ++i) {
    String field = String("enable_") + topicOptions[i].topic_suffix;
    if (server.hasArg(field)) {
      topicOptions[i].enabled = true;
    } else {
      topicOptions[i].enabled = false;
    }
    prefs.putBool(topicOptions[i].topic_suffix, topicOptions[i].enabled);
  }
  prefs.end();
  server.sendHeader("Location", "/settings");
  server.send(303);
}


// Handle Data
void handleCalibrate() {
  eau_max_cm = tank_height_cm - measureDistance();
  prefs.begin("cuve", false);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.end();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleData() {
  float distance = measureDistance();
  float water_height = max(0.0f, tank_height_cm - distance);
  float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
  float volume_liters;
  if (tank_shape == 1) {
    // Cylindrique
    volume_liters = (PI * pow(tank_diameter_cm / 2, 2) * water_height) / 1000.0;
  } else {
    // Rectangulaire
    volume_liters = (tank_length_cm * tank_width_cm * water_height) / 1000.0;
  }

  String json = "{";
  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"water_height\":" + String(water_height, 1) + ",";
  json += "\"percent\":" + String(percent, 0) + ",";
  json += "\"volume_liters\":" + String(volume_liters, 1) + ",";
  json += "\"temp\":" + String(current_temp, 1) + ",";
  json += "\"outside_temp\":" + String(isnan(outside_temp) ? -99 : outside_temp, 1) + ",";
  json += "\"outside_humi\":" + String(isnan(outside_humi) ? -1 : outside_humi, 1) + ",";
  json += "\"relay_state\":" + String(relay_state ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void handleTempSettingsSave() {
  if (server.hasArg("temp_refresh")) temp_refresh_ms = server.arg("temp_refresh").toInt() * 1000;
  prefs.begin("cuve", false);
  prefs.putInt("temp_refresh_ms", temp_refresh_ms);
  prefs.end();

  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleFactoryReset() {
  // Efface la partition NVS "cuve"
  prefs.begin("cuve", false);
  prefs.clear();
  prefs.end();

  // (Optionnel) Efface le WiFiManager (reset WiFi)
  WiFi.disconnect(true, true);  // Efface config WiFi

  server.send(200, "text/html", "<html><body><h2>Device has been reset.<br>Restarting...</h2></body></html>");
  delay(1500);
  ESP.restart();  // Redémarre l'ESP32
}

void handleRelayToggle() {
  relay_state = !relay_state;
  prefs.begin("cuve", false);
  prefs.putBool("relay_state", relay_state);
  prefs.end();
  digitalWrite(RELAY_PIN, relay_active_low ? !relay_state : relay_state);
  server.sendHeader("Location", "/");
  server.send(303);
}


// Mqtt
void handleMqttConnect() {
  if (server.hasArg("mqtt_host")) mqtt_host = server.arg("mqtt_host");
  if (server.hasArg("mqtt_user")) mqtt_username = server.arg("mqtt_user");
  if (server.hasArg("mqtt_pass")) mqtt_password = server.arg("mqtt_pass");

  prefs.begin("cuve", false);
  prefs.putString("mqtt_host", mqtt_host);
  prefs.putString("mqtt_user", mqtt_username);
  prefs.putString("mqtt_pass", mqtt_password);
  prefs.end();

  mqtt.setServer(mqtt_host.c_str(), 1883);
  mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str());

  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleMqttStatus() {
  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void mqttReconnect() {
  if (!mqtt.connected() && millis() - lastMqttAttempt > mqtt_publish_ms) {
    lastMqttAttempt = millis();
    mqtt.connect("esp32cuve", mqtt_username.c_str(), mqtt_password.c_str());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String top = String(topic);
  if (top.endsWith("/relay_cmd")) {
    String cmd = "";
    for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
    cmd.trim();
    if (cmd.equalsIgnoreCase("on")) relay_state = true;
    else if (cmd.equalsIgnoreCase("off")) relay_state = false;
    digitalWrite(RELAY_PIN, relay_active_low ? !relay_state : relay_state);
  }
}


void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, relay_active_low ? !relay_state : relay_state);
  digitalWrite(RELAY_PIN, relay_active_low ? HIGH : LOW);

  temp_refresh_ms = prefs.getInt("temp_refresh_ms", 4000);
  mqtt_publish_ms = prefs.getInt("mqtt_publish_ms", 5000);

  sensors.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  prefs.begin("cuve", true);
  tank_shape = prefs.getInt("tank_shape", 0);
  tank_diameter_cm = prefs.getFloat("tank_diameter", 0.0);
  tank_name = prefs.getString("tank_name", "WaterTank");
  tank_height_cm = prefs.getFloat("tank_height", 120.0);
  tank_length_cm = prefs.getFloat("tank_length", 50.0);
  tank_width_cm = prefs.getFloat("tank_width", 40.0);
  eau_max_cm = prefs.getFloat("eau_max", 100.0);
  relay_state = prefs.getBool("relay_state", false);

  for (int i = 0; i < NUM_TOPICS; ++i) {
    topicOptions[i].enabled = prefs.getBool(topicOptions[i].topic_suffix, true);
  }
  prefs.end();

  WiFiManager wifiManager;
  wifiManager.autoConnect("Tank_Automation_Config");
  Serial.println("WiFi connected: ");
  Serial.println(WiFi.localIP());

  prefs.begin("cuve", true);
  mqtt_host = prefs.getString("mqtt_host", "");
  mqtt_username = prefs.getString("mqtt_user", "");
  mqtt_password = prefs.getString("mqtt_pass", "");
  prefs.end();

  mqtt.setServer(mqtt_host.c_str(), 1883);
  mqtt.setCallback(mqttCallback);
  String subTopic = tank_name;
  subTopic.replace(" ", "_");
  subTopic += "/relay_cmd";
  mqtt.subscribe(subTopic.c_str());


  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/mqtt_status", HTTP_GET, handleMqttStatus);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/settings", HTTP_POST, handleSettingsSave);
  server.on("/mqtt_connect", HTTP_POST, handleMqttConnect);
  server.on("/topics", HTTP_POST, handleTopicsSave);
  server.on("/temp_settings", HTTP_POST, handleTempSettingsSave);
  server.on("/relay_toggle", HTTP_POST, handleRelayToggle);
  server.begin();
}


void loop() {

  server.handleClient();

  static unsigned long lastDhtRead = 0;
  if (millis() - lastDhtRead > temp_refresh_ms) {
    outside_temp = dht.readTemperature();
    outside_humi = dht.readHumidity();
    lastDhtRead = millis();
  }

  static unsigned long lastTempRead = 0;
  if (millis() - lastTempRead > temp_refresh_ms) {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C && temp > -50 && temp < 100) {
      current_temp = temp;
    }
    lastTempRead = millis();
  }


  mqttReconnect();
  mqtt.loop();
  static unsigned long lastMqttPublish = 0;
  if (millis() - lastMqttPublish > mqtt_publish_ms) {
    lastMqttPublish = millis();

    float distance = measureDistance();
    float water_height = max(0.0f, tank_height_cm - distance);
    float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
    float volume_liters;
    if (tank_shape == 1) {  // Cylindrical
      volume_liters = (PI * pow(tank_diameter_cm / 2, 2) * water_height) / 1000.0;
    } else {  // Rectangular
      volume_liters = (tank_length_cm * tank_width_cm * water_height) / 1000.0;
    }
    if (mqtt.connected()) {
      String prefix = tank_name;
      prefix.replace(" ", "_");
      for (int i = 0; i < NUM_TOPICS; ++i) {
        if (topicOptions[i].enabled) {
          String topic = prefix + "/" + topicOptions[i].topic_suffix;
          String val;
          if (strcmp(topicOptions[i].topic_suffix, "water_distance") == 0) val = String(distance, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "water_height") == 0) val = String(water_height, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "water_percent") == 0) val = String(percent, 0);
          else if (strcmp(topicOptions[i].topic_suffix, "water_volume") == 0) val = String(volume_liters, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "water_temp") == 0) val = String(current_temp, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "outside_temp") == 0) val = String(outside_temp, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "outside_humi") == 0) val = String(outside_humi, 1);
          else if (strcmp(topicOptions[i].topic_suffix, "relay_state") == 0) val = relay_state ? "ON" : "OFF";
          mqtt.publish(topic.c_str(), val.c_str());
        }
      }
    }
  }
}
