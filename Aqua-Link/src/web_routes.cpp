#include "webserver.h"
#include "config.h"
#include "mqtt.h"
#include "settings.h"
#include "sensors.h"
#include "relay.h"
#include "ota.h"
#include <Preferences.h>
#include <Arduino.h>
#include <Update.h>

static void handleMqttStatus() {
  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

static void handleData() {
  float distance = measureDistance();
  float water_height = max(0.0f, tank_height_cm - distance);
  float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
  float volume_liters;
  if (tank_shape == 1) {
    // Cylindrical
    volume_liters = (PI * pow(tank_diameter_cm / 2, 2) * water_height) / 1000.0;
  } else {
    // Rectangular
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
  json += "\"pump_is_on\":" + String(pump_is_on ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void handleFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

static void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(1000);
  ESP.restart();
}

// Web HTML
static void handleRoot() {
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
        // Now pump_is_on directly represents the pump state
        pumpCell.innerText = data.pump_is_on ? "ON" : "OFF";
        pumpCell.style.fontWeight = "bold";

        const pumpBtn = document.getElementById('pumpBtn');
        if (pumpBtn) pumpBtn.style.background = data.pump_is_on ? "#28a745" : "#6c757d";
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
                + tank_name + R"rawliteral(</span> Monitor V2</h2>
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
          <button id="pumpBtn" onclick="togglePump()" style="padding:7px 24px;border-radius:8px;background:#6c757d;color:#fff;font-weight:600;border:none;box-shadow:0 2px 6px rgba(0,0,0,0.2);cursor:pointer;">
            <span id="pumpstate">--</span>
          </button>
        </td>
      </tr>
    </table>
    <a href="/settings">⚙️ Settings →</a>
    <span></span>
    <form id="calibForm" method="POST" action="/calibrate">
      <button type="submit" id="calibBtn">Calibrate Max Water Height</button>
    </form>
  </div>
  <tr><td>Firmware Version</td><td>)rawliteral" + fw_version + R"rawliteral(</td></tr>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

static void handleSettings() {
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
      margin-bottom: 0.7em;
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
          </form>
          <form action="/disable_mqtt" method="POST" onsubmit="return confirm('Are you sure you want to disable MQTT?');">
            <button type="submit" style="background-color:#e63946; color:white; padding:10px; border:none; border-radius:4px;">
              Disable MQTT
            </button>
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
          <div class="section-title">🛠️ Utilities</div>
            <form action="/reboot" method="POST" onsubmit="return confirm('Are you sure you want to reboot the device?');">
              <button type="submit" style="background-color: #f94144; color: white; padding: 10px 20px; border: none; border-radius: 5px;">
                Reboot Device
              </button>
            </form>
            <form method='POST' action='/factory_reset' onsubmit=\"return confirm('Are you sure you want to factory reset? This will erase all settings and WiFi config!');\">
            <button type='submit' style='background:#e63946; color:#fff; margin-top:18px; border-radius:8px;'>Factory Reset</button>
            </form>
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
      window.onload = function() {
        updateTankFields();
        updateMqttStatus();
      };
      document.getElementById('tank_shape').onchange = updateTankFields;

    </script>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

static void handleFirmwarePage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset="UTF-8"><title>Firmware Upload</title></head>
    <body>
      <h2>Upload new firmware</h2>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="update">
        <input type="submit" value="Update">
      </form>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void initWebServer() {
  // Web server setup
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/mqtt_status", HTTP_GET, handleMqttStatus);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/settings", HTTP_POST, saveSettings);
  server.on("/mqtt_connect", HTTP_POST, handleMqttConnect);
  server.on("/topics", HTTP_POST, handleTopicsSave);
  server.on("/temp_settings", HTTP_POST, saveTempSettings);
  server.on("/relay_toggle", HTTP_POST, toggleRelay);
  server.on("/ota", []() {if (!isOTAEnabled()) {setupOTA();} server.send(200, "text/plain", "OTA mode enabled. You can now upload new firmware.");});
  server.on("/disable_mqtt", HTTP_POST, disableMQTT);
  server.on("/reboot", HTTP_POST, []() {server.send(200, "text/html", "<html><body><h1>Rebooting...</h1></body></html>");delay(500);ESP.restart();});
  server.on("/firmware", HTTP_GET, handleFirmwarePage);
  server.on("/upload", HTTP_POST, [](){ server.send(200, "text/plain", "Update complete. Rebooting..."); ESP.restart(); }, handleFirmwareUpload);
  server.begin();
  Serial.println("✅ Web server started");
}