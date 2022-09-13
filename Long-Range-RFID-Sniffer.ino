/***************************************************************
 *                  Long Range RFID Sniffer                    *
 *                                                             *
 * Reads RFID cards from a long/medium distance, and displays  *
 * the card IDs on a web page on a WiFi network (SSID of a     *
 * printer by default) created by the device.                  *
 *                                                             *
 * This is intended to be loaded onto a WiFi-capable           *
 * Arduino-compatible board and used with a Wiegand long-range *
 * reader (like the HID MaxiProx 5375.)                        *
 *                                                             *
 * by Ege F, 2022                                              *
 * credit to Tom Igoe and Adafruit for parts of the WiFi code  *
 ***************************************************************/



#include <SPI.h>
#include <WiFiNINA.h>
#include <Wiegand.h>

// Wiegand data pins
#define PIN_D0 2
#define PIN_D1 3

// The object that handles the wiegand protocol
Wiegand wiegand;

// String that will store all the capture messages
String captures = "";

// <WiFi Stuff>

char ssid[] = "DIRECT-04-HP OfficeJet 250";        // Pretend to be a printer since we can't hide SSID
char pass[] = "KeepItSimple";

int status = WL_IDLE_STATUS;

// </WiFi Stuff>

// Web server
WiFiServer server(80);

// Favicon of the captures page (id-card-clip from Font Awesome)
String favicon = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAaCAIAAAAvw3vjAAABhWlDQ1BJQ0MgcHJvZmlsZQAAKJF9kT1Iw0AcxV9TpSoVByuIOGSoThZERQQXqWIRLJS2QqsOJpd+QZOGJMXFUXAtOPixWHVwcdbVwVUQBD9AHJ2cFF2kxP8lhRYxHhz34929x907QKiXmWp2jAOqZhnJWFTMZFfFwCu6EUQIsxiQmKnHU4tpeI6ve/j4ehfhWd7n/hy9Ss5kgE8knmO6YRFvEE9vWjrnfeIQK0oK8TnxmEEXJH7kuuzyG+eCwwLPDBnp5DxxiFgstLHcxqxoqMRTxGFF1ShfyLiscN7irJarrHlP/sJgTltJcZ3mMGJYQhwJiJBRRQllWIjQqpFiIkn7UQ//kONPkEsmVwmMHAuoQIXk+MH/4He3Zn5ywk0KRoHOF9v+GAECu0CjZtvfx7bdOAH8z8CV1vJX6sDMJ+m1lhY+Avq2gYvrlibvAZc7wOCTLhmSI/lpCvk88H5G35QF+m+BnjW3t+Y+Th+ANHW1fAMcHAKjBcpe93h3V3tv/55p9vcDvalyxWWHRAEAAAAJcEhZcwAALiMAAC4jAXilP3YAAAAHdElNRQfmCQwVAwh0FDWcAAAAGXRFWHRDb21tZW50AENyZWF0ZWQgd2l0aCBHSU1QV4EOFwAAAypJREFUSMdj/P//PwNe8O/Pr+lTZ09cfujP//8MDAyMjExpke6F2fFsLEz4NTISNPrp/fsmwaXFCV7K8hIMDAxPHjxoW7jv8PrJKvKS+DWyMBAC//79Z2BgcHa2NdRWZWBguHH5fNvCfQyEHAQ1+tOHdwcPHb9599HvP3+R5aRl5WLD/XBq/f9v7eq1tx6+RDGOhUVZWdHe1lJYkI/lw9tX6Vk1F599dTRSZmND8QQjK/c/Bqjrvn37/vXrVwgDruDdm3cvXr1D1vL716+law+ILtm2dHojy/ZNW2++/7dv7WQpMSGsjuPj55HiZApNb2JkZGBgYPj/n4GTg52Xl4eBkSk1Kx1T/btXz0NjSlZvOcBy5eZND3tTXOYyMDDwi4htXNZ1+/4TWOgyKioqiAvz4VIvJCbpbqd689Y9ln///3KyEIhMGXkFGXkFBqIBKwvL////mRhoBliIVPf/3783b9/+Z2AUERFmgoQ6VYx+/+ZlZ9fURXuu/WdgCHY2rinPkBARJKiLcID8//u7raX35L0PyyeVrplU9Pz+3eqWaX/+/aeCq189e7b40L2V02rsLQwYGBg4mP/5ZE94/OyNoowopa6GZyDqR6OYlFScnVJT79ya3EgWhr+TJi/wsDOSlRKhQlgzMrNW1hSbKApGF/SGFUwQU1Rur85kYWKkTgoRFBHv6KwvffPuPwOjiIgQIxUT35/fv37++s3JxcnAwPDt2zc2NjZWVlZKjf7y6eOaNRtnLN7x+NMvuKAIP3dmrFdEsI8AHzeZRv/68a2ysmnLuWdV6QHKCtKwdM7w6MGDnllrj5y5Pqu3iouDjRyjTx8/tub4ww3zWswNNFBlbEwNNF2S2w8eP+/paE5OCrlx5bq6toqpvgamlLaBobc6z5nLd/AFCCMDwx8cFZ2BqYmUlhH2ZMbIFJsSy8QtjL06/f+fkYGBRU5Sdt2pm99//uZkR490YwtLPI6yd3bGKv775/eLVx+pGtsyPrh93TOm1tZC19nWkMjSEm/1/+/YkRPrT9zfsaST8f///7dvXJu3cM2uk7eIaAEQyrqMjHYWOkmxIbqaygC5JzHWlOIjrwAAAABJRU5ErkJggg==";

void setup() {

  // <Wiegand stuff>

  //Initialize serial
  Serial.begin(9600);

  //Install listeners and initialize Wiegand reader
  wiegand.onReceive(receivedData, "Card read: ");
  wiegand.onReceiveError(receivedDataError, "Card read error: ");
  wiegand.onStateChange(stateChanged, "State changed: ");
  wiegand.begin(Wiegand::LENGTH_ANY, true);

  //initialize pins as INPUT and attaches interruptions
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_D0), pinStateChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_D1), pinStateChanged, CHANGE);

  //Sends the initial pin state to the Wiegand library
  pinStateChanged();


  // </Wiegand stuff>

  // <WiFi stuff>

  wifi: // Label for retrying WiFi
  Serial.println("Starting AP");
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // Wait and try again
    delay(3000);
    goto wifi;
  }
  // Check NINA firmware version and warn if it's old
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("Please upgrade the firmware. Current firmware is ");
    Serial.println(fv);
  }
  
  // Create the WiFi network
  Serial.print("Creating access point named: ");
  Serial.println(ssid);
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // Wait and try again
    delay(3000);
    goto wifi;
  }
  // Wait 10 seconds (example has this, not sure how useful)
  delay(10000);

  // </WiFi stuff>
  
  // <web server stuff>
  
  // start the web server on port 80
  server.begin();
  // Print connection status
  printWiFiStatus();

  // </web server stuff>

  Serial.println("setup done");
}

// Tends to the Wiegand library
void doWiegand(){
  noInterrupts();
  wiegand.flush();
  interrupts();
}

void loop() {
  // Tend to the wiegand library
  doWiegand();
  
  // <WiFi stuff>

  // Compare the previous status to the current status
  if (status != WiFi.status()) {
    // It has changed, update the variable
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      // A device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // A device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  // </WiFi stuff>

  // <web server stuff>

  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // if we get a client,
    Serial.println("new client");           // print a message out to serial

    String currentLine = "";                // Current line of the request

    while (client.connected()) {            // Loop while the client's connected
      // We still have to tend to the Wiegand library when we're connected to a client
      doWiegand();
      if (client.available()) {             // If there're bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to serial

        if (c == '\n') {                    // If the byte is a newline character, we're at the end of a line
          // If the current line is blank, we got two newline characters in a row.
          // That's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Body

            // Get MAC to identify device in case multiple are deployes
            byte mac[6];
            WiFi.macAddress(mac);

            // Page
            client.println("<html><head><title>RFID Sniffer</title><link rel='shortcut icon' href = '");
            client.println(favicon);
            client.println("'></head><body><p>Device MAC: ");
            client.print(mac[5],HEX);
            client.print(":");
            client.print(mac[4],HEX);
            client.print(":");
            client.print(mac[3],HEX);
            client.print(":");
            client.print(mac[2],HEX);
            client.print(":");
            client.print(mac[1],HEX);
            client.print(":");
            client.println(mac[0],HEX);
            client.println("</p>");
            // If we have anything captured, echo
            if(captures.length() == 0){
              client.println("<p>Nothing yet</p>");
            }else{
              
              client.println("<p>Dump:");
              client.println("<textarea style='width: 100%; height: 80vh'>");
              client.println(captures);
              client.println("</textarea></p>");
            }
            client.println("<button onclick='location.reload();'>Refresh</button></body></html>");
            // End of page
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop:
            break;
          }
          else {      // If we got a newline, clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r') {    // Otherwise, add it to currentLine (ignore CR)
          currentLine += c;
        }
      }
    }

    // If there isn't a client connected anymore, close the connection
    client.stop();
    Serial.println("client disconnected");
  }

  // </web server stuff>

}

// Prints debug information
void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

// <Wiegand helper functions/ISRs>

// When any of the pins have changed, update the state of the wiegand library
void pinStateChanged() {
  wiegand.setPin0State(digitalRead(PIN_D0));
  wiegand.setPin1State(digitalRead(PIN_D1));
}

// Notifies when a reader has been connected or disconnected.
void stateChanged(bool plugged, const char* message) {
    Serial.print(message);
    Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

// Notifies when a card was read.
void receivedData(uint8_t* data, uint8_t bits, const char* message) {
  // Debug output of the message
  Serial.print(message);
  Serial.print(bits);
  Serial.print("bits / ");
  
  // Store the message
  captures += message;
  captures += bits;
  captures += "bits / ";
  
  // Serial print and store the card ID in hex
  uint8_t bytes = (bits+7)/8;
  for (int i=0; i<bytes; i++) {
      Serial.print(data[i] >> 4, 16);
      Serial.print(data[i] & 0xF, 16);
      captures += String(data[i] >> 4, 16);
      captures += String(data[i] & 0xF, 16);
  }
  
  // NL signifies end of record
  captures += "\n";
  
  Serial.println();
}

// Notifies when an invalid transmission is detected (invalid bit length, malformed symbol, etc.)
void receivedDataError(Wiegand::DataError error, uint8_t* rawData, uint8_t rawBits, const char* message) {
  // Debug output of the message
  Serial.print(message);
  Serial.print(Wiegand::DataErrorStr(error));
  Serial.print(" - Raw data: ");
  Serial.print(rawBits);
  Serial.print("bits / ");

  // Store the message
  captures += message;
  captures += Wiegand::DataErrorStr(error);
  captures += " - Raw data: ";
  captures += rawBits;
  captures += "bits / ";

  // Serial print and store the received data (likely a card ID of non-standard length) in hex
  uint8_t bytes = (rawBits+7)/8;
  for (int i=0; i<bytes; i++) {
    Serial.print(rawData[i] >> 4, 16);
    Serial.print(rawData[i] & 0xF, 16);
    captures += String(rawData[i] >> 4, 16);
    captures += String(rawData[i] & 0xF, 16);
  }
  
  // NL signifies end of record
  captures += "\n";

  Serial.println();
}

// <Wiegand helper functions/ISRs>
