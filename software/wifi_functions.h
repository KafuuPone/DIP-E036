#ifndef wifi_functions_h
#define wifi_functions_h

#include "string"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

#include "constants.h"    // containing wifi name and password
#include "website_html.h" // html for the website

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

// Setup Wi-Fi
void WifiAP_begin() {
  Serial.println("Configuring access point...");

  // Set up the WiFi access point
  if(!WiFi.softAP(WIFI_SSID, WIFI_PW)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

// Setup website (server_pt = &server, freq_pt = &curr_freq, vol_pt = &curr_vol, state_pt = &ready_state, freq_update = &wifi_freq_update, vol_update = &wifi_vol_update, tune_update = &wifi_tune_update)
// AsyncWebServer server(80);
void ServerBegin(AsyncWebServer* server_pt, int* freq_pt, uint8_t* vol_pt, bool* state_pt, int* freq_update, uint8_t* vol_update, String* tune_update) {
  // Serve the web page with FM radio station list
  (*server_pt).on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
  });

  // Handle AJAX requests, get current frequency and volume
  (*server_pt).on("/update", HTTP_GET, [=](AsyncWebServerRequest *request) {
    // Create a JSON response with both values
    String jsonResponse = "{";
    jsonResponse += "\"frequency\":\"" + String(*freq_pt) + "\",";
    jsonResponse += "\"volume\":\"" + String(*vol_pt) + "\"";
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Retrieve current state (radio ready or not), ready = true
  (*server_pt).on("/status", HTTP_GET, [=](AsyncWebServerRequest *request) {
    // Create a JSON response with the state value "0" or "1"
    String jsonResponse = "{\"status\":\"" + String(*state_pt) + "\"}";
    request->send(200, "application/json", jsonResponse);
  });

  // If commands of settings are changed from server, gets info
  (*server_pt).on("/get", HTTP_GET, [=](AsyncWebServerRequest* request) {
    String frequencyMessage = "Nan";
    String volumeMessage = "Nan";
    String tuneMessage = "Nan";
    if(request->hasParam("frequency")) {
      frequencyMessage = request->getParam("frequency")->value();
      *freq_update = frequencyMessage.toFloat() * 10;
    }
    if(request->hasParam("volume")) {
      volumeMessage = request->getParam("volume")->value();
      *vol_update = volumeMessage.toInt();
    }
    if(request->hasParam("tune")) {
      tuneMessage = request->getParam("tune")->value();
      *tune_update = tuneMessage;
    }
  });

  // If tune up is activated from server, gets info
  (*server_pt).on("/tune_up", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Tune up pressed");
  });

  // If tune down is activated from server, gets info
  (*server_pt).on("/tune_down", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Tune down pressed");
  });

  (*server_pt).onNotFound(notFound);
  (*server_pt).begin();
  Serial.println("Server started");
}

#endif