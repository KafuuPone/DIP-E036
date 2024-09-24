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

float remote_frequency;

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Webserver-demo";
const char* password = "123456789";

const char* PARAM_FREQUENCY = "frequency";

// List of FM Radio frequencies in Singapore
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>FM Radio Selector</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>Select an FM Radio Station in Singapore</h2>
  <form action="/get" method="GET">
    <label for="frequency">Choose a station:</label>
    <select name="frequency" id="frequency">
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
    </select><br><br>
    <input type="submit" value="Submit">
  </form>
  </body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Send web page with FM radio stations list to client
  Serial.println("Make HTTP GET Request");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Handle the form submission and extract the selected frequency
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String frequencyMessage;
    String frequencyParam;
    
    // Check if the "frequency" parameter exists in the request
    if (request->hasParam(PARAM_FREQUENCY)) {
      frequencyMessage = request->getParam(PARAM_FREQUENCY)->value();
      frequencyParam = PARAM_FREQUENCY;
    } else {
      frequencyMessage = "No frequency selected";
    }

    // Print the received frequency to Serial Monitor
    Serial.print("Selected Frequency: ");
    Serial.println(frequencyMessage);
    
    // Convert the selected frequency to float and store in remote_frequency
    remote_frequency = frequencyMessage.toFloat();
    Serial.print("Remote Frequency Set To: ");
    Serial.println(remote_frequency);

    // Respond with confirmation and provide a link to go back to the main page
    request->send(200, "text/html", "You selected the frequency ("
                  + frequencyMessage + " MHz)<br><a href=\"/\">Return to Home Page</a>");
  });
  
  server.onNotFound(notFound);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Empty loop as server runs asynchronously
}