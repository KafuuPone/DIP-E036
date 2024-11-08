#include <string>
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
// #include "LiquidCrystal_I2C.h"
#include "website_html.h"


// Initialize the I2C LCD (Set the LCD address as per your module, typically 0x27 or 0x3F)
// LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD

int time_passed = 0;

//Set the server to connect to the HTTP Port
AsyncWebServer server(80);

float remote_frequency;
int remote_volume;

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Webserver-demo";
const char* password = "123456789";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);

  Serial.println("Setup done.");
  WifiAP_begin(); //Set up the esp32 as access point
  ServerBegin(); //Start the HTTP GET process
}

void loop() {
  // Nothing to do here, everything is handled asynchronously
  // Serial.println(remote_frequency);
  delay(500);
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

  // Handle the GET request when frequency/volume is changed
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String frequencyMessage;
    String volumeMessage;
    if (request->hasParam("frequency")) {
      frequencyMessage = request->getParam("frequency")->value();
    } else {
      frequencyMessage = "Nan";
    }
    if (request->hasParam("volume")){
      volumeMessage = request->getParam("volume")->value();
      remote_volume = volumeMessage.toInt();
    }else{
      volumeMessage = "Nan";
    }

    // Convert the frequency string to a float and store it
    remote_frequency = frequencyMessage.toFloat();
    if(frequencyMessage != "Nan") {
      Serial.print("Selected Frequency: ");
      Serial.println(remote_frequency);
    }
    if(volumeMessage != "Nan") {
      Serial.print("Volume: ");
      Serial.println(remote_volume);
    }

    // Respond with a simple message
    request->send(200, "text/plain", "Frequency received: " + frequencyMessage + " MHz");
  });

  // Handle the /tune_up request
  server.on("/tune_up", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "Tuning Frequency Up");
    Serial.println("Tune up pressed");
  });

  // Handle the /tune_down request
  server.on("/tune_down", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "Tuning Frequency Down");
    Serial.println("Tune down pressed");
  });

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Server started");
}