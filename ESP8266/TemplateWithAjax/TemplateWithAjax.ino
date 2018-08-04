/* This sketch includes: 
 * - Ability to connect to the strongest network
 * - Use of mDNS (multicast DNS) to allow for renaming of IP addresses in the local network.
 *   e.g. setting the IP address to map to http://esp8266.local
 *   Otherwise server accessed from http://server_ip/endpoint
 *   Where server_ip is the IP address of the ESP8266 module
 * - Toggles LED (working in conjunction with the "template-with-ajax-iot.h file", which is just a hardcoded string of the "template-with-ajax-iot.html" file.
 *   
 * Note this example can only run on devices connected to the same
 * Wi-Fi network as connected here. Need to setup Virtual Server/Port Forwarding for access from internet.
 * 
 * Used on a NodeMCU Development Board
 */

 // Start by going e.g. http://esp8266.local:49000
 // or e.g. http://192.168.1.18:49000 (where the 18 is whatever port number gets set)

//**********************************
// Libraries, constants and globals
//**********************************
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library - to search for strongest network
#include <ESP8266mDNS.h>        // Include the mDNS library - if you want to rename the ip address locally
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <FS.h>
#include <WiFiUdp.h>            // Include the Wi-Fi UDP library
/* Note on UDP:
// For HTTP we mainly use TCP connections, but NTP (Network Time Protocol) is based on UDP.
// The main difference between TCP and UDP is that TCP needs a connection to send messages:
// First a handshake is sent by the client, the server responds, and a connection is established,
// and the client can send its messages. After the client has received the response of the server,
// the connection is closed (except when using WebSockets). To send a new message, the client has to
// open a new connection to the server first. This introduces latency and overhead.

// UDP doesn't use a connection, a client can just send a message to the server directly, and the server
// can just send a response message back to the client when it has finished processing. There is, however,
// no guarantee that the messages will arrive at their destination, and there's no way to know whether they
// arrived or not (without sending an acknowledgement, of course). This means that we can't halt the
// program to wait for a response, because the request or response packet could have been lost on
// the Internet, and the ESP8266 will enter an infinite loop.

// Instead of waiting for a response, we just send multiple requests, with a fixed interval between two
// requests, and just regularly check if a response has been received.
*/

// Note: Our web page is now a array of characters stored in variable AJAX_PAGE_HTML.
// Do not use comments in this file. It is HTML data as a character array not a program.
// The HTML code is now in a header file .h not .html file.
// The string literal is in the form: R "delimiter( raw_characters )delimiter", where ===== is the arbitary delimiter used in my case
// PROGMEM is basically a way for the ESP8266 to store the string into flash, saving RAM, and then loading it into RAM when it is needed.. (I think..)
#include "template-with-ajax-iot.h" // The HTML webpage contents
String ajaxPageHtml = AJAX_PAGE_HTML;
// I think the main alternative ways is to use SPIFFS, or store the files in another server and
// do a get request from the ESP8266 to get the string/file

// Note: If you need to parse JSON - there's a good library for it
// https://github.com/bblanchon/ArduinoJson
// https://www.arduino.cc/en/guide/libraries
#include <ArduinoJson.h>

// Note you can access header values in other header files, just need to include them in
// the correct order in the .c file

// There are two ways to use jQuery in ESP8266 Web Sever, first is to use cdn server
// and second is directly putting jQuery on ESP Flash File System.
// E.g. for CDN, just add <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script> to the markup
// Disadvantage of this method is we need active internet connection to the WiFi Hot spot.


// Create an instance of the server
// Specify the port to listen on as an argument
// NOTE: If you use a different port number, you simply access it e.g. http://192.168.1.18:49000/gpio/0
// :80 is set by default, that's why you don't notice it usually
// Where 49000 is an arbitrary port number, guessing that nothing else is using that port,
// (0 to 65535), just note that many port numbers are already reserved/used for other things
// WiFiServer server(49000); // Port 80 is the port number assigned to commonly used internet communication protocol, Hypertext Transfer Protocol (HTTP).
ESP8266WebServer server(49000); // Create a webserver object that listens for HTTP request on port 49000
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiUDP UDP;                    // Create an instance of the WiFiUDP class to send and receive

File fsUploadFile;              // A File variable to temporarily store the received file

// Basically the WiFi network name, and password
const char* ssid = "Your Wi-Fi Network Name"; // The SSID (Service Set Identifier) is the name of the WiFi network you want to connect to.
const char* password = "Your Wi-Fi Network Password";

#define LED_INBUILT 2 // const int LED_INBUILT = 2;

// Domain name for the mDNS responder
const char* mdnsName = "esp8266";

// Network Transfer Protocol vars and constants
IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // A buffer to hold incoming and outgoing packets
const int NTP_PORT = 123;        // Set NTP requests to port 123
unsigned long intervalNTP = 60000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;
unsigned long prevActualTime = 0;

// Function Declarations
void handleRoot();                      // Function prototypes for HTTP handlers
void handleLed();
void handleLogin();
void handleNotFound();
String getContentType(String filename); // Convert the file extension to the MIME type
bool handleFileRead(String path);       // Send the right file to the client (if it exists)
void handleFileUpload();                // Upload a new file to the SPIFFS


//**********************************
// Setup
//**********************************
void setup() {
  Serial.begin(9600); // 115200
  delay(10);

  // Prepare GPIO2
  pinMode(LED_INBUILT, OUTPUT);
  digitalWrite(LED_INBUILT, 0); // Set to low/off by default

  // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startWifi();

  // Start the SPIFFS and list all contents
  startSpiffs();

  // Start the mDNS responder
  startMdns();

  // Start listening for UDP messages to port 123
  // startUdp(); // Turn off UDP for template

  // Start a HTTP server with a file read handler and an upload handler
  startServer();  
  
}


//**********************************
// Loop
//**********************************
// Note: We don't want our loop to take longer than a couple of milliseconds. If we did, the HTTP server etc. would start to misbehave.
// Use millis() in cases where you can, like the blink sketch, to keep loop from locking.

// unsigned long prevMillis = millis();

void loop() {

  // Essentially handles the checking of client, and getting of client that has data available for reading
  server.handleClient(); // Run the server/Listen for HTTP requests from clients

  // printNtpTime(); // Turn off UDP for template
}


//**********************************
// Loop
//**********************************
void startWifi() {

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // Set wifi mode to station (as opposed to AP or AP_STA)
  // Station mode => Connecting to one specific network (i.e. connects to an access point or hot spot)
  // Access Point mode => Sets the ESP8266 as an Access Point, so it has its own SSID and password to connect, that another device (e.g. mobile) would use to connect to the ESP8266
  // AP_STA => Both AP and Station Mode, so it essentially extends the Access Point
  WiFi.mode(WIFI_STA);

  // Control Variable
  // If true, search for the strongest WiFi network from a set list.
  // If false, connect to the single WiFi network that has been set.
  bool useStrongestWifi = true;

  if (!useStrongestWifi) {
    // Initializes the WiFi library's network settings and provides the current status.
    WiFi.begin(ssid, password); // WPA encrypted networks use a password in the form of a string for security

// If you want to use a fixed IP address - Was more for testing DHCP on router to get access from the internet, not just local wifi..
//  IPAddress ip(192, 168, 1, 19); // where xx is the desired IP Address
//  IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
//  Serial.print(F("Setting static ip to : "));
//  Serial.println(ip);
//  IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your
//  WiFi.config(ip, gateway, subnet);
  
    // Wait for the WiFi to connect
    // WL_CONNECTED when connected to a network
    // WL_IDLE_STATUS when not connected to a network, but powered on
    while (WiFi.status() != WL_CONNECTED) { // https://www.arduino.cc/en/Reference/WiFiStatus
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
      
    // Print the IP address of the ESP8266
    Serial.println(WiFi.localIP());
  } else {
    //1 Setting up provision to use ESP8266 as either a station or an access point.
    //1 Since I'll mainly be using it as a station, commented it out to simplify things.
    // Start a Wi-Fi access point, and try to connect to some given access points.
    // Then wait for either an AP or STA connection
//1    WiFi.softAP(ssid, password);             // Start the access point
//1    Serial.print("Access Point \"");
//1    Serial.print(ssid);
//1    Serial.println("\" started\r\n");
    
    // Add list of Wi-Fi networks you may want to connect to
    // wifiMulti.addAP("ssidFromAccesPoint1", "yourPasswordForAccessPoint1");
    // wifiMulti.addAP("ssidFromAccesPoint2", "yourPasswordForAccessPoint2");
    // wifiMulti.addAP("ssidFromAccesPoint3", "yourPasswordForAccessPoint3");
    wifiMulti.addAP(ssid, password);
  
    Serial.println("Connecting ...");
    int i = 0;
    while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
//1      && WiFi.softAPgetStationNum() < 1) {
      delay(1000);
      Serial.print('.');
    }
    Serial.println("");
//1    if (WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
      Serial.println("WiFi connected to ");
      Serial.println(WiFi.SSID()); // Tell us what network we're connected to
      Serial.print("IP address:\t");
      Serial.print(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
//1    } else {
//1      Serial.print("Station connected to ESP8266 AP");
//1    }
    Serial.println("\r\n");
  }

}


void startUdp() {
  Serial.println("Starting UDP");
  UDP.begin(NTP_PORT);                      // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();

  // We need the IP address of the NTP server, so we perform a DNS lookup with the server's hostname.
  // There's not much we can do without the IP address of the time server, so if the lookup fails,reboot the ESP.
  // If we do get an IP, send the first NTP request, and enter the loop.
  if (!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server (sets it to timeServerIP)
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNtpPacket(timeServerIP);  
}


// Start the SPIFFS (Serial Peripheral Interface Flash File System) and list all contents
// Note at the moment, it doesn't work fully because I think you need a plugin
// for the "ESP8266 Sketch Data Upload": https://github.com/esp8266/arduino-esp8266fs-plugin
// But cbb at the moment... (See below for another way to upload files/by-pass the plugin..)
/*
 * So far we've always included the HTML for our web page as string literals in our sketch.
 * This makes our code very hard to read, and you'll run out of memory rather quickly.
 * To help with this, there is the Serial Peripheral Interface Flash File System,
 * or SPIFFS for short. It's a light-weight file system for microcontrollers with an SPI flash chip.
 * The on-board flash chip of the ESP8266 has plenty of space for your webpages, especially if you
 * have the 1MB, 2MB or 4MB version.
 */
void startSpiffs() {
  SPIFFS.begin(); // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) { // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}


// Start the mDNS responder
void startMdns() {
  if (!MDNS.begin(mdnsName)) { // Start the multicast domain name server (i.e. start the mDNS responder for esp8266.local)
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.print("mDNS responder started: http://");
    Serial.print(mdnsName);
    Serial.println(".local");
  }
}


void startServer() { // Start a HTTP server with a file read handler and an upload handler
  // Serves up the "upload.html"
  server.on("/upload", HTTP_GET, []() {                   // If the client requests the upload page
    // Where the upload.html file would be in the /data folder (within the sketch folder), i.e. Ctrl + K
//    if (!handleFileRead("/upload.html"))                // send it if it exists
//      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error

    // Temporary work around for no "Tool > Sketch Data Upload" plugin
    // Simply hardcode the string for /data/upload.html here
    server.send(200, "text/html", "<form method=\"post\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"name\"><input class=\"button\" type=\"submit\" value=\"Upload\"></form>");
  });  

  // Handles the file upload
  server.on("/upload.html", HTTP_POST, []() {   // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");       // Send a success response, and
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.on("/LED", HTTP_POST, handleLed);      // Call the 'handleLed' function when a POST request is made to URI "/LED" (no arguments)
  server.on("/login", HTTP_POST, handleLogin);  // Call the 'handleLogin' function when a POST request is made to URI "/login" (with arguments)

  server.on("/testgetwithargs", HTTP_GET, handleGetValues); // Call the 'handleGetValues' function when a GET request is made to URI "/testgetwithargs" (with arguments)

  server.on("/testajaxpost", HTTP_GET, handleAjaxPostPage); // Call the 'handleAjaxPostPage' function when a POST request is made to URI "/testajaxpost" (with/without arguments) 
  server.on("/testajaxpost", HTTP_POST, handleAjaxPost);  // Call the 'handleAjaxPostPage' function when a POST request is made to URI "/testajaxpost" (with/without arguments) 
  server.on("/getdata", HTTP_GET, handleAjaxGetData);

  // Note in a handler method, you could use (server.method() == HTTP_GET) ? "GET" : "POST";

  // server.on("/", handleRoot);              // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/", HTTP_GET, handleRoot);       // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);          // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
                                              // and check if the file exists
  // To handle it in-line
  // server.onNotFound([]() {
  //   if (!handleFileRead(server.uri()))
  //     server.send(404, "text/plain", "404: Not Found");
  // });

  // Start the server
  server.begin(); // Tells the (ESP8266) server to begin listening for incoming connections.
  Serial.println("HTTP server started.");
}


//**********************************
// Server Handlers
//**********************************
void handleRoot() {
  // server.send(200, "text/plain", "Hello world!");   // Send HTTP status 200 (Ok) and send some text to the browser/client

  // When URI "/" is requested, send a web page with a button to toggle the LED
  // Note this would reload the html... better to send JSON as a response
  server.send(200, "text/html", "<form action=\"/LED\" method=\"POST\"><input type=\"submit\" value=\"Toggle LED\"></form>");

  // When URI "/" is requested, send a web page with a login form
  // server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form><p>Try 'R...' and 'S...' ...</p>");
}


void handleLed() {                          // If a POST request is made to URI /LED
  digitalWrite(LED_INBUILT, !digitalRead(LED_INBUILT));     // Change the state of the LED
  server.sendHeader("Location", "/");       // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}


void handleLogin() {                         // If a POST request is made to URI /login
  if (!server.hasArg("username") || !server.hasArg("password") 
      || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  // If both the username and the password are correct
  if (server.arg("username") == "Reaper" && server.arg("password") == "Sketch") {
    server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p>");
  } else { // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
}


void handleGetValues() {
  String message = "Number of args received: ";
  message += server.args();            // Get number of parameters (this gets the args whether it's from a GET, or a POST request)
  message += "\n";                     // Add a new line
  
  for (int i=0; i<server.args(); i++) {  
    message += "Arg no. " + (String)i + " -> ";   // Include the current iteration value
    message += server.argName(i) + ": ";          // Get the name of the parameter
    message += server.arg(i) + "\n";              // Get the value of the parameter
  } 
  
  server.send(200, "text/plain", message);        // Response to the HTTP request
}


void handleAjaxPostPage() {
  server.send(200, "text/html", ajaxPageHtml);    // Response to the HTTP request
}


void handleAjaxPost() {
    
  // If a straight json object has been sent through
  if (server.hasArg("plain")) {
    StaticJsonBuffer<200> newBuffer;
    Serial.println(server.arg("plain"));
    JsonObject& newjson = newBuffer.parseObject(server.arg("plain"));

//    newjson.printTo(Serial); // Serial.println(newjson); doesn't work here, need to use JsonObject method
//    Serial.println();
//    newjson["test"].printTo(Serial);
//    Serial.println();

    char jsonChar[100];
    newjson.printTo((char*) jsonChar, newjson.measureLength() + 1);

//    newjson["data"][1]["ledState"].printTo(Serial);
//    Serial.println();

    if (newjson["data"][1]["ledState"] == "false") {
      digitalWrite(LED_INBUILT, 0);
    } else if (newjson["data"][1]["ledState"] == "true") {
      digitalWrite(LED_INBUILT, 1);
    }

    // OR
    // String jsonStr;
    // newjson.printTo(jsonStr)
    server.send(200, "text/html", jsonChar);
  }
  
  if (!server.hasArg("ledstate") || server.arg("ledstate") == NULL) { // If the POST request doesn't have expected data values
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  
  // If the ledstate is given
  if (server.arg("ledstate") == "1" || server.arg("ledstate") == "true") { // If set to on, turn on
    // Turn off
    digitalWrite(LED_INBUILT, 1);
    // This is a JSON formatted string that will be served. You can change the values to whatever like.
    // {"data":[{"dataValue":"test"},{"ledState":"1"}], "test":"anothervalue"} This is essentially what is output here..
    server.send(200, "text/html", "{\"data\":[{\"dataValue\":\"test\"}, {\"ledState\":\"1\"}], \"test\":\"anothervalue\"}");
  } else if (server.arg("ledstate") == "0" || server.arg("ledstate") == "false") { // If set to off, turn off
    // Turn on
    digitalWrite(LED_INBUILT, 0);
    server.send(200, "text/html", "{\"data\":[{\"dataValue\":\"test\"}, {\"ledState\":\"0\"}], \"test\":\"anothervalue\"}");
  } else {
    server.send(400, "text/plain", "400: Bad Request");
  }
  
}


void handleAjaxGetData() {
  server.send(200, "text/html", "{\"data\":[{\"dataValue\":\"Different Value From Get\"}, {\"ledState\":\"" + (String) digitalRead(LED_INBUILT) + "\"}], \"test\":\"anothervalue\"}");
}


void handleNotFound(){ // If the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) { // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  }
}


bool handleFileRead(String path) { // Send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // And send it to the client
    file.close();                                          // Then close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}


// Note: This example isn't very secure (obviously). Everyone that can connect to the ESP can upload
// new files, or edit the existing files and insert XSS code, for example. There's also not a lot of
// error checking/handling, like checking if there's enough space in the SPIFFS to upload a new file, etc.
void handleFileUpload(){ // Upload a new file to the SPIFFS (instead of flashing via usb)
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/"+path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: ");
    Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
      // Can also do fsUploadFile.print(...) or fsUploadFile.println(...)
  } else if (upload.status == UPLOAD_FILE_END){
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page (would need this file..)
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}


//**********************************
// Helper Functions
//**********************************
// Convert sizes in bytes to KB and MB
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

// Determine the filetype of a given filename, based on the extension
// Convert the file extension to the MIME type
String getContentType(String filename) { 
  if (filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

// Should probably replace digitalWrite calls with myDigitalWrite, because
// ESP12-E operates with the reverse value
void myDigitalWrite(int pin, int value) { // 0 or 1, LOW or HIGH
  digitalWrite(pin, !value);
}


// char* path - e.g. "/Test.txt"
bool removeSpiffsFile(char* path) {
  SPIFFS.remove(path);
}


//**********************************
// Helper Functions - NTP
//**********************************
// Prints the NTP time to the Serial Monitor, every minute (to be called in loop() method)
void printNtpTime() {
  unsigned long currentMillis = millis();

  // Sends a new NTP request to the time server every minute.
  if (currentMillis - prevNTP > intervalNTP) { // If a minute has passed since last NTP request
    prevNTP = currentMillis;
    Serial.println("\r\nSending NTP request ...");
    sendNtpPacket(timeServerIP);               // Send an NTP request
  }

  // Check if we've got a new response from the server. If this is the case, we update the timeUNIX variable with
  // the new/ timestamp from the server. If we don't get any responses for an hour, then there's something
  // wrong, so we reboot the ESP.
  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    Serial.print("NTP response:\t");
    Serial.println(timeUNIX);
    lastNTPResponse = currentMillis;
  } else if ((currentMillis - lastNTPResponse) > 3600000) { // 1 hour
    Serial.println("More than 1 hour since last NTP response. Rebooting.");
    Serial.flush();
    ESP.reset();
  }

  // This part prints the actual time. The actual time is just the last NTP time
  // plus the time since we received that NTP message.
  uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
  if (actualTime != prevActualTime && timeUNIX != 0) { // If a second has passed since last print
    prevActualTime = actualTime;
    Serial.printf("\rUTC time:\t%d:%d:%d   ", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime));
    Serial.println();
  }  
}

uint32_t getTime() {
  // First try to parse the UDP packet. If there's no packet available, return 0.
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  // If there is a UDP packet available however, read it into the buffer.
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // The NTP timestamp is 32 bits or 4 bytes wide, so we combine these bytes into one long number.
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // This number is the number of seconds since Jan 1, 1900, 00:00:00, but most applications use UNIX time,
  // the number of seconds since Jan 1, 1970, 00:00:00 (UNIX epoch). To convert from NTP time to UNIX time,
  // we just subtract 70 years worth of seconds.
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

// To request the time from the NTP server, you have to send a certain sequence of 48 bytes. We don't need
// any fancy features, so just set the first byte to request the time, and leave all other 47 bytes zero.
// To actually send the packet, you have to start the packet, specifying the IP address of the server,
// and the NTP port number, port 123. Then just write the buffer to the packet, and send it with endPacket.
void sendNtpPacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // Set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // Send a packet requesting a timestamp:
  UDP.beginPacket(address, NTP_PORT); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}
