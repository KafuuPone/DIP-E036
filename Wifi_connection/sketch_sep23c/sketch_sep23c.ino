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
    body { font-family: Arial, sans-serif; margin: 20px; }
    h2 { color: #333; }
    label { font-size: 16px; margin-right: 10px; }
    select, input { margin-bottom: 10px; padding: 5px; font-size: 16px; }
    #status { font-weight: bold; margin-top: 20px; }
    .slider { width: 300px; }
  </style>
  <script>
    // JavaScript function to send the frequency and volume when user releases the controls
    function sendFrequency() {
      var frequency = document.getElementById("frequency").value;
      var volume = document.getElementById("volume").value;

      // Update the status dynamically as the user interacts with the controls
      document.getElementById("status").innerHTML = "Selected frequency: " + frequency + " MHz, Volume: " + volume;
      
      // Send the frequency and volume values to the server only once
      fetch("/get?frequency=" + frequency + "&volume=" + volume)
      .then(response => response.text())
      .catch(error => console.error("Error:", error));
    }

    // Function to dynamically update volume when user scrolls the slider (but do not send data yet)
    function updateVolumeDisplay() {
      var volume = document.getElementById("volume").value;
      document.getElementById("volDisplay").innerHTML = volume; // Display the volume dynamically
    }

    // Function to dynamically update frequency when user changes selection
    function updateFrequency() {
      sendFrequency(); // Send the updated frequency to the server
    }
  </script>
  </head><body>
  <h2>FM Radio Selector</h2>

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
  
  <br><br>

  <label for="volume">Set Volume (0 - 15):</label>
  <input type="range" id="volume" name="volume" min="0" max="15" step="1" class="slider" oninput="updateVolumeDisplay()" onchange="sendFrequency()"> <!-- Use onchange to send data only when released -->
  <span id="volDisplay">0</span> <!-- This will display the volume dynamically -->
  
  <br><br>

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
  server.onNotFound(notFound);
  server.begin();
  Serial.println("Server started");
}