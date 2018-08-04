// NOTE: This is the default Blink Sketch, but to use the AT Command Processor,
// we don't actually upload the sketch.
// We simply/purely use the Serial Monitor to write the AT Commands.
// If you accidentally upload this sketch, you have to reset/upload the AT firmware
// back onto the board.

// When a command is entered in to the serial monitor on the computer 
// the Arduino (or NodeMCU in my case) will relay it to the ESP8266

// This file just includes some AT commands, in the general order they should be executed

/*
 Blink the blue LED on the ESP-01 module
 The blue LED on the ESP-01 module is connected to GPIO1 
 (which is also the TXD pin; so we cannot use Serial.print() at the same time) 
 Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  Serial.begin(9600);               // Communication with the host computer
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because 
                                    // it is active low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)
}


void commandList() {
  // See online docs for a list of the commands, responses,
  // and order in which you should execute the commands.

  // Note: When you enter the commands in the serial monitor, remove the outer quotation marks.
  "AT"; // Response: OK
  "AT+CWMODE_CUR=1"; // AT+CWMODE(without_CUR) is deprecated // (Sets WiFi Station Mode) Response: STA (aka Client...)

  // Join Network
  "AT+CWLAP"; // (Search available WiFi Spots (aka Access Points)) Response: <ecn><ssid><rssi><mac><freq offset><freq calibration>
              // <ecn> (Encryption): 0 = OPEN, 1 = WEP, 2 = WPA_PSK, 3 = WPA2_PSK, 4 = WPA_WPA2_PSK
              // <freq offset> frequency offset of AP, unit: KHz. The value of <freq offset> / 2.4 to get the value as ppm
              // <freq calibration> calibration for frequency offset
  // Make sure to remove the backslashes when entering into the serial monitor,
  // these are only here to escape the quotation marks (for arduino files...)
  "AT+CWJAP_CUR=\"Your Wi-Fi Name Here\",\"Your Wi-Fi Password Here\""; // (Join Access Point - may take a few seconds) Response: WIFI CONNECTED, WIFI GOT IP, OK
  "AT+CWJAP_CUR?"; // (Check if connected successfully, or use AT+CWJAP=?, or use AT+CIFSR to find your IP Address)
  "AT+GMR"; // (Check current firmware version)

  // Note for some reason when I connected to the WiFi network, the returned local IP Addresses were
  // AT+CIFSR
  //  +CIFSR:APIP,"4.16.49.32"
  //  +CIFSR:APMAC,"12:04:22:21:00:02"
  //  +CIFSR:STAIP,"197.7.1.150"
  //  +CIFSR:STAMAC,"0c:13:00:03:93:02"
  // Which meant when I tried to ping i.e. 197.7.1.150, it didn't work
  // To get the correct station IP, I had to set it manually
  "AT+CIPSTA=\"192.168.1.50\",\"192.168.1.1\",\"255.255.255.0\""; // Set IP address of ESP8266 station
  // Where "50" is some arbitrary number, that likely isn't used by another device
  // <IP> string, IP address of ESP8266 station
  // [<gateway>] gateway
  // [<netmask>] netmask
  // cmd prompt: ping 192.168.1.50 now works
  // Note to ping 4.16.49.32, you would need to connect to the Access Point of the ESP8266 first.. :)

//  "AT+CWDHCP_CUR=2,1"; // Assigns (STA & AP) and enables an actual device number on the router
  
  // The below line has been known to cause issues so avoid
  // "AT_CIUPDATE"; // Get reply +CIPUPDATE:1, +CIPUPDATE:2, +CIPUPDATE:3, +CIPUPDATE:4, OK

  //----------------------------------------
  // If you want to run as a station/client..
  //----------------------------------------

  // TCP Client
  "AT+CIPMUX=1"; // (Turn on multiple connections) Response: OK
    // NOTE: CIPMUX needs to be set everytime on power up/reset (it does not save this when power is removed)
    // "AT+CIPMUX=1" can only be set when transparent transmission disabled (i.e. AT+CIPMODE=0)
  // Setup server, so another browser on the local network can access the device
  "AT+CIPSERVER=1,80"; // Setup TCP server, on port 80, 1 means enabled
  // If you want to connect to the "internet" (or an external server)
  // Can use the IP address, or the url/domain name... (assuming here that 192.168.1.10 is some external server address)
//  "AT+CIPSTART=4,\"TCP\",\"192.168.1.10\",80"; // (Connect to remote TCP server 192.168.1.10 (the PC (or mobile... whatever...))
    // Params <id><type><address><port>
    // id = 0-4 (ID of network connection (0~4), used for multi-connection)
    // type = TCP/UDP
    // addr = IP address
    // port = port
    // If CIPMUX was set to 0 (i.e. single connection, don't need the ID
    // ==> (CIPMUX=0) AT+CIPSTART = <type>,<addr>,<port>
    // If CIPMUX was set to 1 (i.e. multiple connection (up to 4?, where 4 (i.e. the 5th one would be all maybe?)))
    // ==> (CIPMUX=1) AT+CIPSTART=<id><type>,<addr>,<port>

  // "AT+CIPMODE=1"; // Default, and should normally be left as 0 (Optionally enter into data transmission mode)

  // Note this (AT+CIPSEND) can only be run when there is a TCP connection (if not initialized by AT_CIPSTART,
  // the alternative is to enter the ESP8266 IP address into a browser). Typing the command AT_CIPSTATUS
  // in the Serial Monitor should then display AT+CIPSTATUS, STATUS:3
  // If this is the case where you have not started a TCP connection and used a browser, the ID of the connection
  // for the AT+CIPSEND command has to be 0.
  "AT+CIPSEND=4,110"; // (Send data via channel 4, 110 bytes length - usually you would set this based on length of data string to send. However if you don't know, can just use some arbitrary large number, e.g. 110 or whatever..)
    // First parameter = ID of the connection (0~4), for multi-connect
  // After typing AT+CIPSEND, you would type into the serial monitor something string you would want the
  // browser to display. Could be html markup as well.
  "AT+CIFSR"; // Check module IP address
  // This should also equal 0, if TCP Connection wasn't explicitely setup, and a browser was used to create the connection
  "AT+CIPCLOSE=4"; // Close the TCP/IP Connection, and effectively sends a message added from AT+CIPSEND
  // Can do e.g. AT+CIPSEND=0,5 (Enter) Hello (Enter) AT+CIPCLOSE=0

  // NOTE: Entering the IP Address in a broweser, typically outputs something similar to the below in the Serial Monitor:
  // +IPD,0,430:GET / HTTP/1.1
  // Host: 192.168.1.21
  // Connection: keep-alive
  // Cache-Control: max-age=0
  // Upgrade-Insecure-Requests: 1
  // User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36
  // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
  // Accept-Encoding: gzip, deflate
  // Accept-Language: en-NZ,en-GB;q=0.9,en-US;q=0.8,en;q=0.7

  // When you enter the ESP8266 IP Address in the browser, you'll see the webpage loading spinner for a bit,
  // which means it's waiting for you to close the connection from the ESP8266

  // Start another message send..
  "AT+CIPSTART=4,\"TCP\",\"192.168.1.10\",80";
  "AT+CIPSEND=4,110";
  // Run command to server in Serial, e.g. GET /update?key=123456789&field1=0  (arbitrary example endpoint..)
  "AT+CIPCLOSE=4";


  // When shutting down/destroying..
  "AT+CWQAP"; // Quit Access Point


  //----------------------------------------
  // If you want to run as an access point..
  //----------------------------------------
  "AT+RST"; // Resets the state of the device
  "AT+CWMODE_CUR=3"; // Configure as access point (and station)
  "AT+CWJAP_CUR=\"Your Wi-Fi Name Here\",\"Your Wi-Fi Password Here\""; // CWJAP(without _CUR) is deprecated
  "AT+CWSAP?"; // Self query the access point
  "AT+CWSAP=\"ESP_19A28D\",\"passwork\",1,3,4,0"; // Reset the settings
    // Everything same as default, except I added the password, and changed the encryption to 3 (i.e. WPA2_PSK), which is the same as most WiFi networks use..
    // <ssid>,<pwd>,<chl>,<ecn>,<max conn>,<ssid hidden>
    // <ssid> string, ESP8266 softAP’s SSID
    // <pwd> string, range: 8 ~ 64 bytes ASCII
    // <chl> channel ID
    // <ecn>
    // 0 : OPEN
    // 2 : WPA_PSK
    // 3 : WPA2_PSK
    // 4 : WPA_WPA2_PSK
    // <max conn> maximum count of stations that are allowed to connect to ESP8266 soft-AP (range: [1, 4])
    // <ssid hidden> Broadcast SSID by default
    // 0 : broadcast SSID of ESP8266 soft-AP
    // 1 : do not broadcast SSID of ESP8266 soft-AP
  "AT+CWLIF"; // (List clients connected to ESP8266 softAP) Response: [ip, mac address]
  "AT+CIFSR"; // Return IP Address
  "AT+CIPMUX=1"; // (Turn on multiple connection)
  "AT+CIPSERVER=1,80"; // Turn on server on port 80
}

