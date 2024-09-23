#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

// Set web server port number to 80
WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  // Setting the ESP32 as an Access Point
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();  // Start the server
}

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message in the serial monitor
    String currentLine = "";                // make a String to hold incoming data from the client
    String userInput = "";                  // String to store the user input (message)
    bool inputReceived = false;             // Flag to track if input is received

    while (client.connected()) {            // Loop while the client is connected
      if (client.available()) {             // If there's bytes to read from the client,
        char c = client.read();             // Read a byte
        Serial.write(c);                    // Print it out to the serial monitor
        currentLine += c;                   // Add byte to the currentLine

        // Look for the end of the HTTP request
        if (c == '\n') {
          // If the current line is blank, we got two newline characters in a row
          // That's the end of the client HTTP request, so send a response
          if (currentLine.length() == 2 && inputReceived) {
            // Send HTTP response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Send the response HTML page with user input echoed back
            client.println("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
            client.println("<body><h1>Message Received</h1>");
            client.println("<p>Your message: " + userInput + "</p></body></html>");

            // The HTTP response ends with another blank line
            client.println();
            break;
          } else { // If a newline is received but not the end of the request, clear the currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // If you got anything else but a carriage return character,
          currentLine += c;      // Add it to the end of the currentLine

          // Look for the 'GET /?message=' in the HTTP request
          if (currentLine.indexOf("GET /?message=") >= 0) {
            int msgStart = currentLine.indexOf("message=") + 8;  // Find where the message starts
            int msgEnd = currentLine.indexOf(" ", msgStart);     // Find where the message ends (before a space)
            userInput = currentLine.substring(msgStart, msgEnd); // Extract the user input
            inputReceived = true;                               // Set flag to true
            Serial.println("User Input: " + userInput);          // Print user input to Serial Monitor
          }
        }
      }
    }
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}