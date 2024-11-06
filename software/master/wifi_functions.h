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

// Setup website (&server, &curr_freq, &curr_vol, &ready_state, &wifi_freq_update, &wifi_vol_update, &wifi_tune_update, &RDS_radiotext, &bluetooth_mode, &connection_state, &playback_state, &device_name, &media_title, &media_artist, &media_album, &server_bluetooth_mode)
// AsyncWebServer server(80);
void ServerBegin(AsyncWebServer* server_pt, const int* freq_pt, const uint8_t* vol_pt, const bool* state_pt, int* freq_update, uint8_t* vol_update, String* tune_update, const String* radio_text, const bool* bluetooth_mode, const uint8_t* connection_state, const uint8_t* playback_state, char** device_name, char** media_title, char** media_artist, char** media_album, bool* server_bluetooth_mode) {
  // Serve the web page with FM radio station list
  (*server_pt).on("/", HTTP_GET, [=](AsyncWebServerRequest* request) {
    // Radio mode
    if(!(*bluetooth_mode)) {
      request->send_P(200, "text/html", index_html);
    }
    // Bluetooth mode, different html
    else {
      request->send_P(200, "text/html", bluetooth_index_html);
    }
  });

  // Updates bluetooth mode
  (*server_pt).on("/bluetooth", HTTP_GET, [=](AsyncWebServerRequest *request) {
    // Create a JSON response with the state value "0" or "1"
    String jsonResponse = "{\"mode\":\"" + String(*bluetooth_mode) + "\"}";
    request->send(200, "application/json", jsonResponse);
  });

  // Updates on bluetooth metadata
  (*server_pt).on("/bluetooth-metadata", HTTP_GET, [=](AsyncWebServerRequest *request) {
    if(*bluetooth_mode) {
      String temp_device_name = *device_name;
      String temp_media_title = *media_title;
      String temp_media_artist = *media_artist;
      String temp_media_album = *media_album;

      String jsonResponse = "{";
      jsonResponse += "\"connection_state\":\"" + String(*connection_state) + "\",";
      jsonResponse += "\"playback_state\":\"" + String(*playback_state) + "\",";
      jsonResponse += "\"device_name\":\"" + temp_device_name + "\",";
      jsonResponse += "\"media_title\":\"" + temp_media_title + "\",";
      jsonResponse += "\"media_artist\":\"" + temp_media_artist + "\",";
      jsonResponse += "\"media_album\":\"" + temp_media_album + "\"";
      jsonResponse += "}";
      request->send(200, "application/json", jsonResponse);
    }
  });

  // Handle AJAX requests, get current frequency, volume and radiotext
  (*server_pt).on("/update", HTTP_GET, [=](AsyncWebServerRequest *request) {
    // Create a JSON response with both values
    String jsonResponse = "{";
    jsonResponse += "\"frequency\":\"" + String(*freq_pt) + "\",";
    jsonResponse += "\"volume\":\"" + String(*vol_pt) + "\",";
    jsonResponse += "\"radiotext\":\"" + String(*radio_text) + "\"";
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
    String bluetoothModeMessage = "Nan";
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
    if(request->hasParam("bluetooth-mode")) {
      bluetoothModeMessage = request->getParam("bluetooth-mode")->value();
      *server_bluetooth_mode = (bluetoothModeMessage == "true");
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