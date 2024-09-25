#include <Arduino.h>
#include <iostream>
#include <string>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#else
#include <WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>

// Initialize the I2C LCD (Set the LCD address as per your module, typically 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD

int time_passed = 0;

//Set the server to connect to the HTTP Port
AsyncWebServer server(80);

float remote_frequency;
int remote_volume;
bool tune_freq_up = false;
bool tune_freq_down = false;

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Webserver-demo";
const char* password = "123456789";

const char* PARAM_FREQUENCY = "frequency";

// Web server interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>FM Radio Selector</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f4f8;
      color: #333;
      margin: 0;
      padding: 20px;
      text-align: center;
    }

    h2 {
      color: #0078d7;
      font-size: 28px;
      margin-bottom: 20px;
      text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.2);
    }

    label {
      font-size: 18px;
      font-weight: bold;
      color: #0057a7;
      margin-right: 10px;
    }

    select, input[type="range"], button {
      font-size: 16px;
      padding: 10px;
      margin: 10px 0;
      border: 2px solid #0078d7;
      border-radius: 5px;
      transition: all 0.3s ease;
    }

    select:hover, input[type="range"]:hover, button:hover {
      border-color: #0057a7;
      cursor: pointer;
    }

    input[type="range"] {
      width: 300px;
      height: 10px;
      background-color: #e0e0e0;
      appearance: none;
      outline: none;
      transition: background-color 0.3s ease;
    }

    input[type="range"]::-webkit-slider-thumb {
      appearance: none;
      width: 20px;
      height: 20px;
      background: #0078d7;
      border-radius: 50%;
      cursor: pointer;
      transition: background 0.3s ease;
    }

    input[type="range"]::-webkit-slider-thumb:hover {
      background: #0057a7;
    }

    #status {
      font-size: 18px;
      margin-top: 20px;
      font-weight: bold;
      color: #333;
      padding: 10px;
      background-color: #f9f9f9;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      display: inline-block;
    }

    .slider-container, .button-container {
      margin-bottom: 20px;
    }

    button {
      background-color: #0078d7;
      color: white;
      border: none;
      padding: 12px 24px;
      font-size: 16px;
      border-radius: 5px;
      transition: background-color 0.3s ease;
    }

    button:hover {
      background-color: #0057a7;
    }

    /* Media query for smaller screens */
    @media (max-width: 600px) {
      input[type="range"] {
        width: 100%;
      }

      h2 {
        font-size: 24px;
      }

      label {
        font-size: 16px;
      }
    }
  </style>
  <script>
    function sendFrequency() {
      var frequency = document.getElementById("frequency").value;
      var volume = document.getElementById("volume").value;

      // Update the status dynamically
      document.getElementById("status").innerHTML = "Selected frequency: " + frequency + " MHz, Volume: " + volume;

      // Send the frequency and volume values to the server
      fetch("/get?frequency=" + frequency + "&volume=" + volume)
      .then(response => response.text())
      .catch(error => console.error("Error:", error));
    }

    function updateVolumeDisplay() {
      var volume = document.getElementById("volume").value;
      document.getElementById("volDisplay").innerHTML = volume;
    }

    function updateFrequency() {
      sendFrequency();
    }

    function tuneFrequencyUp() {
      // Send a request to set tune_freq_up to true
      fetch("/tune_freq_up")
      .then(response => response.text())
      .then(data => {
        document.getElementById("status").innerHTML = "Tuning Frequency Up...";
      })
      .catch(error => console.error("Error:", error));
    }

    function tuneFrequencyDown() {
      // Send a request to set tune_freq_down to true
      fetch("/tune_freq_down")
      .then(response => response.text())
      .then(data => {
        document.getElementById("status").innerHTML = "Tuning Frequency Down...";
      })
      .catch(error => console.error("Error:", error));
    }
  </script>
  </head><body>
  <h2>FM Radio Selector</h2>

  <div class="slider-container">
    <label for="frequency">Choose a station:</label>
    <select id="frequency" name="frequency" onchange="updateFrequency()">
      <option value="88.3">88.3 MHz (Kiss92 FM)</option>
      <option value="89.3">89.3 MHz (ONE FM 89.3)</option>
      <option value="90.5">90.5 MHz (Gold 90.5FM)</option>
      <option value="91.3">91.3 MHz (ONE 91.3FM)</option>
      <option value="92.4">92.4 MHz (Symphony 92.4)</option>
      <option value="93.8">93.8 MHz (938NOW)</option>
      <option value="94.2">94.2 MHz (Oli 96.8 FM)</option>
      <option value="95.8">95.8 MHz (Capital 95.8FM)</option>
      <option value="96.3">96.3 MHz (Hao FM 96.3)</option>
      <option value="98.0">98.0 MHz (Warna 94.2 FM)</option>
      <option value="99.5">99.5 MHz (Symphony FM)</option>
    </select>
  </div>

  <div class="slider-container">
    <label for="volume">Set Volume (0 - 15):</label>
    <input type="range" id="volume" name="volume" min="0" max="15" step="1" class="slider" oninput="updateVolumeDisplay()" onchange="sendFrequency()">
    <span id="volDisplay">0</span>
  </div>

  <!-- Buttons for tuning frequency up and down -->
  <div class="button-container">
    <button onclick="tuneFrequencyUp()">Tune Frequency Up</button>
    <button onclick="tuneFrequencyDown()">Tune Frequency Down</button>
  </div>

  <div id="status">Selected frequency: None, Volume: 0</div>
  </body></html>)rawliteral";



void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  // Print a welcome message
  lcd.setCursor(0, 0);
  lcd.print("welcome to E036");
  delay(2000);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("FM  Receiver");
  lcd.setCursor(0,1);
  lcd.print("Frequency: 100.00 MHz");
  lcd.clear();

  WifiAP_begin(); //Set up the esp32 as access point
  ServerBegin(); //Start the HTTP GET process
}

void loop() {
  // Nothing to do here, everything is handled asynchronously
  lcd.setCursor(0, 0);
  lcd.print("Frequency: ");
  lcd.setCursor(0, 1);
  lcd.print(remote_frequency);
  lcd.print(" MHz");
}

void WifiAP_begin(){
  Serial.println("Configuring access point...");

  // Set up the WiFi access point
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

}

void ServerBegin(){
  // Serve the web page with FM radio station list
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Handle the GET request when a frequency is selected
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String frequencyMessage;
    String volumeMessage;
    if (request->hasParam(PARAM_FREQUENCY)) {
      frequencyMessage = request->getParam(PARAM_FREQUENCY)->value();
    } else {
      frequencyMessage = "No frequency selected";
    }
    if (request->hasParam("volume")){
      volumeMessage = request->getParam("volume")->value();
      remote_volume = volumeMessage.toInt();
    }else{
      volumeMessage = "No volume received";
    }

    // Convert the frequency string to a float and store it
    remote_frequency = frequencyMessage.toFloat();
    Serial.print("Selected Frequency: ");
    Serial.println(remote_frequency);
    Serial.print("Volume: ");
    Serial.println(remote_volume);


    // Respond with a simple message
    request->send(200, "text/plain", "Frequency received: " + frequencyMessage + " MHz");
  });
  server.on("/tune_freq_up", HTTP_GET, [](AsyncWebServerRequest * request) {
    tune_freq_up = true;
    request->send(200, "text/plain", "Tuning Frequency Up");
    Serial.println("Tuning up the volume");
  });

  // Handle the /tune_freq_down request
  server.on("/tune_freq_down", HTTP_GET, [](AsyncWebServerRequest * request) {
    tune_freq_down = true;
    request->send(200, "text/plain", "Tuning Frequency Down");
    Serial.println("Tuning down the volume");
      });
  server.onNotFound(notFound);
  server.begin();
  Serial.println("Server started");
}
