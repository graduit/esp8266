// Whenever you click on a button in the HTML page a GET request is sent to the ESP8266

//+IPD,0,345:GET /?pin=13 HTTP/1.1
//Host: 192.168.4.1
//Connection: keep-alive
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
//User-Agent:Moila/.0(inow N 61;WO64 AplWeKi/57.6(KTM, ik Gco)Chom/3.0211.5 afri53.3
//Acep-Ecoin: zp,delae,sdh
//Acep-Lngag: nUSenq=.8
//To know when request is in progress, the Arduino looks for the string “+IPD,” in the Serial buffer using Serial.find
//The code then reads the next character (the connection id, 0 in the example request above). The connection ID is needed to know which connection to close (different simultaneous requests have a different ID).
//Next we get the pin number by looking for the string “?pin=” in the serial buffer, once again using Serial.find
//Now that we have the pin number we know which pin to toggle

//Post ajax requests will conjure up a response like:
//+IPD,0,434:POST / HTTP/1.1
//Host: 192.168.1.22
//Connection: keep-alive
//Content-Length: 12
//Accept: application/json, text/javascript, */*; q=0.01
//Origin: null
//User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36
//Content-Type: application/x-www-form-urlencoded; charset=UTF-8
//Accept-Encoding: gzip, deflate
//Accept-Language: en-NZ,en-GB;q=0.9,en-US;q=0.8,en;q=0.7
//
//
//+IPD,0,12:{"pin":"13"}


// NOTE: There was a small issue where if you clicked "Toggle LED 13" before the send/connection close completed,
// it would jam up a bit... couldn't be bothered fixing at the time, had other things to work on..
// Maybe come back and fix it, if I end up needing this code for anything...


#include <ArduinoJson.h>

#define DEBUG true // Turn debug message on or off in serial

// Function declarations
void handleEspResponse();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(13, LOW);            // LED_BUILTIN = 13

  // Pin Modes for ESP TX/RX
  Serial.begin(115200); // Atmega2560 // Communication with the host computer
  Serial1.begin(115200); // ESP01

  setupEspConnection();

  // Testing getJson function
  //  connectToExternalServer("yourdomainname.co.nz"); // Shouldn't have https:// or http:// ...
  //  String response = getJson("yourdomainname.co.nz", "/directory/filename.js", "");
  //  response = response.substring(response.indexOf("+IPD,"));
  //  String jsonString = response.substring(response.indexOf("\r\n\r\n") + (String("\r\n\r\n").length()));
  //  Serial.println(jsonString);
  //  StaticJsonBuffer<200> newBuffer;
  //  JsonObject& newjson = newBuffer.parseObject(jsonString);
  //  String test = newjson["test"]; // jsonString e.g. {"test": "testvalue","one": "two"}
  //  Serial.println(test);

  // Testing postJson function
  //  connectToExternalServer("yourdomainname.co.nz"); // Shouldn't have https:// or http:// ...
  //  String response = postJson("yourdomainname.co.nz", "/directory/filename.php", "key=mykey&value=myvalue");
  //  Serial.println(response);
  //  response = response.substring(response.indexOf("+IPD,"));
  //  response = response.substring(response.indexOf("\r\n\r\n") + (String("\r\n\r\n").length()));
  //  String jsonString = midString(response, "\r\n", "\r\n");
  //  Serial.println(jsonString);
  //  StaticJsonBuffer<200> newBuffer;
  //  JsonObject& newjson = newBuffer.parseObject(jsonString);
  //  String test = newjson["one"]; // jsonString e.g. {"one":"mykeytest","two":"myvaluetest"}
  //  Serial.println(test);


  delay(2000); // Basically just gives you some time to open the Serial Monitor
}

// the loop function runs over and over again forever
void loop() {

  handleEspResponse();

}



const int DATA_IN_BUFFER_ARRAY_SIZE = 20;
/*
  Name: sendData
  Description: Function used to send data to ESP8266.
  Params: command - the data/command to send; timeout
  Params: timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
  Params: debug - print to Serial window?(true = yes, false = no)
  Params: successKeyword - a keyword in the response that signals a complete and succesful response
  Params: failureKeyword - a keyword in the response that signals a complete and unsuccesful response
  Returns: The response from the esp8266 (if there is a reponse)
*/
String sendEspData(String command, const int timeout, boolean debug, String successKeyword = "", String failureKeyword = "") {
  String response = "";
  char dataInBuffer[DATA_IN_BUFFER_ARRAY_SIZE]; // This is the buffer - if the success/failureKeyword is longer than 20, then increase this
  int dataInBufferIndex = 0;
  bool isEndOfResponse = false;
  Serial1.print(command); // Send the input command to the esp8266

  long int time = millis();

  while ((time + timeout) > millis()) {
    if (!isEndOfResponse) {
      while (Serial1.available()) {
        // The esp has data so display its output to the serial window
        char c = Serial1.read(); // read the next character.
        response += c;

        // Rolling buffer, so it holds the latest 20 chars (used for searching for keywords)
        if (dataInBufferIndex > (DATA_IN_BUFFER_ARRAY_SIZE - 1)) {
          dataInBufferIndex = (DATA_IN_BUFFER_ARRAY_SIZE - 1);
          for (int i = 0; i < (DATA_IN_BUFFER_ARRAY_SIZE - 1); i++) { // keysize-1 because everthing is shifted over - see next line
            dataInBuffer[i] = dataInBuffer[i + 1]; // So the data at 0 becomes the data at 1, and so on.... the last value is where we'll put the new data
            // So if your buffer was, 1234, it would become 2344, where that last '4' becomes the new data
          }
        }
        dataInBuffer[dataInBufferIndex] = c;
        dataInBufferIndex++;
      }

      // If a keyword is found, end the loop early, so you don't have to wait till the whole timeout time
      if (successKeyword.length() > 0 || failureKeyword.length() > 0) {
        // Terminate any unfilled char array with null, otherwise you get errors...
        if (dataInBufferIndex < DATA_IN_BUFFER_ARRAY_SIZE) {
          dataInBuffer[dataInBufferIndex] = '\0';
        }
        String dataInBufferStr = (String(dataInBuffer));
        dataInBufferStr.toUpperCase();
        successKeyword.toUpperCase();
        failureKeyword.toUpperCase();
        if (successKeyword.length() > 0 && dataInBufferStr.indexOf(successKeyword) >= 0) {
          isEndOfResponse = true;
        }
        if (failureKeyword.length() > 0 && dataInBufferStr.indexOf(failureKeyword) >= 0) {
          isEndOfResponse = true;
        }
      }
    } else {
      break;
    }
  }
  // TODO: Could probably handle case for when timeout exceeded..

  if (debug) {
    Serial.print(response);
  }

  return response;
}


void setupEspConnection() {

  // sendEspData("AT+RST\r\n", 2000, DEBUG); // reset module
  // Acts as an Access Point
  // sendData("AT+CWMODE=2\r\n", 1000, DEBUG); // configure as access point

  // Acts as Access Point AND Station
  sendEspData("AT+RST\r\n", 2000, DEBUG); // reset module
  sendEspData("AT+CWMODE=3\r\n", 1000, DEBUG, "OK"); // configure as access point
  sendEspData("AT+CWJAP=\"Your Wi-Fi Name\",\"Your Wi-Fi Password\"\r\n", 10000, DEBUG, "OK");
  // delay(10000);

  sendEspData("AT+CIFSR\r\n", 1000, DEBUG, "OK"); // get ip address //192.168.4.1
  sendEspData("AT+CIPMUX=1\r\n", 1000, DEBUG, "OK"); // configure for multiple connections
  // Default CWSAP was: "FaryLink_230131","",1,0,4,0
  sendEspData("AT+CWSAP=\"FairyLink\",\"esp01testpass\",1,3,4,0\r\n", 1000, DEBUG);
  // Everything same as default, except I added the password, and changed the encryption to 3 (i.e. WPA2_PSK), which is the same as my WiFi network uses..
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
  sendEspData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG); // turn on server on port 80

  Serial.println();
  Serial.println("Server has successfully been setup. Ready to enter IP Address into browser.");
}


// Note: Generally you wouldn't use the esp to serve web pages,
// you would/should only really use it to host/serve data, e.g. state of leds etc,
// and you should host the website somewhere, using http requests to interact with the esp
// to get the data


/**
   Searches for a keyword
   @params keyword - The keyword to search for, e.g. {'G','E','T'}, {'P','O','S','T'}, {'\r','\n','\r','\n'} etc
   @params receivedRequest - The received request (can be the received request "so far" as well, in a loop)
   @returns 1 if found or 0 if not
*/
boolean findKeywordInReceivedRequest(char* keyword, char* receivedRequest, int keywordLength, int receivedRequestLength) {

  // int receivedRequestLength = strlen(receivedRequest);
  // int keywordLength = strlen(keyword);

  // Removes any blank items (i.e. from the buffer), also shortens it to only find the keyword
  /*  char trimmedReceivedRequest[keywordLength + 1];
    int i=0;
    for (i; i<keywordLength; i++) {
      trimmedReceivedRequest[i] = receivedRequest[i + receivedRequestLength - keywordLength];
    }
    trimmedReceivedRequest[i] = '\0'; */

  bool keywordFound = true;
  // Only search if there is enough chars in the request to even match with the keyword
  if (receivedRequestLength > keywordLength) {
    for (int i = 0; i < keywordLength; i++) {
      //  Serial.print(trimmedReceivedRequest[strlen(trimmedReceivedRequest) - (strlen(keyword)-i)]);
      //  Serial.print("      ");
      //  Serial.print((uint8_t) trimmedReceivedRequest[strlen(trimmedReceivedRequest) - (strlen(keyword)-i)]);

      if (receivedRequest[receivedRequestLength - (keywordLength - i)] != keyword[i]) {
        keywordFound = false;
      }

      //Serial.print((uint8_t) trimmedReceivedRequest[i]);
      //Serial.print("     ");
      //Serial.print(trimmedReceivedRequest[i]);
      //Serial.print("     ");
      //Serial.println(sizeof(keyword));
    }
    // Serial.println();
  } else {
    keywordFound = false;
  }

  return keywordFound;
}


/**

   @param haystack - The char array to search within
   @param haystackLength - The char array length (primarily for optimizing processing time)
   @params keyword - The char array for the "string" to search for
   @param keywordLength - The char array length (primarily for optimizing processing time)
   @params startingIndex - Where to start searching for the keyword,
   @params occurance - Search for the nth occurance of the search string (after the startingIndex)
   @params leftOrRight - Whether to return the index to the left of the found keyword (0), or to the right (1). Right by default
   @returns bool -1 on fail, or index on success
*/
int findKeywordIndexInCharArray(char* haystack, int haystackLength, char* keyword, int keywordLength, int startingIndex = 0, int occurance = 1, bool leftOrRight = 1) {
  int keywordIndex = -1;
  int keywordMatchCount = 0; // I.e. occurance count
  bool keywordMatch = false;

  int newKeywordMatchedCharIndex = 0;

  for (int i = startingIndex; i < haystackLength; i++) {
    for (int j = newKeywordMatchedCharIndex; j < keywordLength; j++) {
      if (haystack[i] != keyword[j]) {
        newKeywordMatchedCharIndex = 0; // continue 2;
        keywordMatch = false;
        break;
      } else {
        keywordMatch = true;
        // If you reach the last character, it means it has been matching all this time
        if (j == keywordLength - 1) {
          keywordMatchCount++;
          if (keywordMatchCount == occurance) {
            if (leftOrRight) { // Return the index to the right
              keywordIndex = i + 1;
            } else { // Return the index to the left
              keywordIndex = i - keywordLength;
            }
            i = haystackLength; // break 2;
          } else {
            keywordMatch = false; // Reset
            newKeywordMatchedCharIndex = 0;
            break;
          }
        }
        newKeywordMatchedCharIndex = j + 1;
        break;
      }

    }
  }

  // Could probably do with some error handling...

  return keywordIndex;
}


// TODO: Make function to get the char array between two indexes... (try avoid Strings... especially on arduino devices..)
String getStringBetweenCharArray() {

}


const int DATA_IN_BUFFER_ARRAY_SIZE_LARGE = 512; // The size of buffer to maintain from i.e. a post request (with all its headers...)
char largeBuffer[DATA_IN_BUFFER_ARRAY_SIZE_LARGE + 1];
int largeBufferIndex = 0;

bool hasDataInStarted = false;
char dataInType = 'E'; // G => GET, P => POST, E => "Empty"
const char postCharArray[] = {'P', 'O', 'S', 'T'};
const char getCharArray[] = {'G', 'E', 'T'};
const char startOfPostResponse[] = {'\r', '\n', '\r', '\n'};
// Handles the input data from the esp
void handleEspResponse() {
  if (Serial1.available()) { // check if the esp is sending a message
    /* while(Serial1.available()) {
      // The esp has data so display its output to the serial window
      char c = Serial1.read(); // read the next character.
      Serial.write(c);
      } */

    // Note Serial1.readBytes() is a useful function but is a blocking function, so try to avoid...
    /*
      while(Serial1.available() > 0){
      // "readBytes" terminates if the determined length has been read, or it
      // times out. It fills "Buffer" with 1 to 90 bytes of data. To change the
      // timeout use: Serial.setTimeout() in setup(). Default timeout is 1000ms.
      Serial1.readBytes(largeBuffer, DATA_IN_BUFFER_ARRAY_SIZE_LARGE);
      }

      // Print out "your" 512 char buffer's contents.
      Serial.print(largeBuffer);
      Serial.println("");
    */

    //    Serial1.readBytes(largeBuffer, DATA_IN_BUFFER_ARRAY_SIZE_LARGE);
    //    Serial.print(largeBuffer);
    //    Serial.println("");

    // Serial.print(Serial1.read());


    /**
       NOTE! This below code is incomplete. It does work for a basic form post (non json),
       but when the sent data gets too big, i.e. with a lot of headers etc, the Serial buffer may get full,
       after which sent data may be discarded (so you may have missing chars etc from the data...)
       Not to mention the atmega2560 range of devices don't have the greatest processors, or multithread
       etc, so probably not a good idea to go too in depth. If I were to complete the thought, I probably
       would have done a search for whether the request was GET/POST, and handling json/form data.
       Also needed to close the connection.
       With this in mind, it's probably a good idea to use timeouts, that may end early if a desired
       character/string is found.

       Ideally you should be storing the entire serial input in your own buffer/char array,
       and processing when complete, of course, the whole purpose of the above was to find out when
       the input was complete...

       ACTUALLY! Turns out you can greatly reduce the processing speed by not using strlen()...
       I.e. by keeping track of the input sizes!
       To be honest, I think the biggest problem was "int receivedRequestLength = strlen(receivedRequest);"
       because the receivedRequest kept growing...

       Serial1.readBytes has a timeout of 1 second by default, which
    */

    if (hasDataInStarted == true) {
      char c = Serial1.read();
      // Serial.println(largeBufferIndex);
      largeBuffer[largeBufferIndex - 1] = c;
      largeBuffer[largeBufferIndex++] = '\0';

      if (dataInType == 'E') {
        if (findKeywordInReceivedRequest(postCharArray, largeBuffer, 4, largeBufferIndex - 1)) {
          dataInType = 'P';
        } else if (findKeywordInReceivedRequest(getCharArray, largeBuffer, 3, largeBufferIndex - 1)) {
          dataInType = 'G';
        }
      } else {

        if (dataInType == 'P') {

          // Must be shorter than expected request
          char endOfDataInKeyword[] = {'\r', '\n', '\r', '\n'};
          bool endOfDataInKeywordFound = findKeywordInReceivedRequest(endOfDataInKeyword, largeBuffer, 4, largeBufferIndex - 1);

          if (endOfDataInKeywordFound) {
            hasDataInStarted = false;
            Serial.println(String(largeBuffer));
            // Serial.println("Boom WHAT");
            // Serial.println(Serial1.readStringUntil('\n')); // Returns the post data..

            // TODO: Would need to determine whether it's json data coming in, or form data, to handle accordingly..
            // For now, just work with form data (from the Alexa curl request)

            // Just short-cutted this for now, dealing with the string, but really should be using char array...
            String postRequest = Serial1.readStringUntil('\n');
            int strLen = postRequest.length() + 1;
            char queryParams[strLen];
            postRequest.toCharArray(queryParams, strLen);

            Serial.println(queryParams);

            const char queryParamsSeparator[2] = "&";
            // Get the first token
            char *token = strtok(queryParams, queryParamsSeparator); // Splits a C string into substrings, based on a separator character

            // Process the token / Walk through other tokens
            while (token != NULL) { // each token is the key value set, e.g. pin=13

              int valueLength = 0;

              // Split the token into a key-value pair
              char* separator = strchr(token, '='); // Returns a pointer to the first occurrence of the character c in the string str, or NULL if the character is not found
              if (separator != 0) { // If the key-value pair is indeed separated by an equals sign..
                // (separator + 1) = The value
                // Serial.println(separator + 1); // +1 to move the pointer across 1, i.e. to skip the equals sign
                // Serial.println("Value Length test: " + (String) strlen(separator + 1));
                valueLength = sizeof(separator) / sizeof(char);
                // Serial.println("Value Length test: " + (String) valueLength);
              }

              // The index for the start of the '=' sign
              int index = (int) (separator - token);
              // Serial.println(index);

              // The query param key
              char key[index + 1];
              memcpy(key, token /* Offset */, index /* Length */);
              key[index] = 0; // Add terminator
              // Serial.println(key);

              // The query param value (if you want to be explicit and not use (separator + 1)...)
              char value[valueLength + 1];
              memcpy(value, (separator + 1) /* Offset */, valueLength /* Length */);
              value[valueLength] = 0; // Add terminator
              // Serial.println(value);

              if (strcmp(key, "pin") == 0) {
                if (strcmp(value, "13") == 0) {
                  int pinNumber = atoi(value); // Converts char array to int
                  digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin {
                }
              }

              // For controlling the LED explicitly
              if (strcmp(key, "pin13") == 0) {
                if (strcmp(value, "1") == 0) {
                  int pinNumber = 13;
                  digitalWrite(pinNumber, 1); // Turn on LED
                } else if (strcmp(value, "0") == 0) {
                  int pinNumber = 13;
                  digitalWrite(pinNumber, 0); // Turn off LED
                }
              }

              Serial.println(token);
              // Walk through other tokens
              token = strtok(NULL, queryParamsSeparator);
            }

            // Get the connection id
            char comma[] = {','};
            int firstCommaIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, comma, 1, 0, 1));
            int secondCommaIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, comma, 1, 0, 2, 0));
            char connectionId = '0';
            for (int i = firstCommaIndex; i <= (secondCommaIndex); i++) {
              connectionId = largeBuffer[i]; // Should just be 1 char long here (i.e. 0-4)
            }

            // Send a json response back
            String atCommand = "{\"test\":\"test\"}";
            // int sendRequestLength = ((String) atCommand).length();
            // sendEspData("AT+CIPSEND=" + (String) connectionId + "," + String(sendRequestLength) + "\r\n", 5000, DEBUG);
            // sendEspData(atCommand + "\r\n", 5000, DEBUG);
            sendHttpResponse(atoi(connectionId), atCommand);

            // Close the connection
            String closeCommand = "AT+CIPCLOSE=";
            closeCommand += connectionId; // Append connection id
            closeCommand += "\r\n";

            // When you sendHttpResponse, it automatically closes the connection, so don't need the below.
            // Would be good to check connection status, and if not connected, run the below, but no harm leaving.
            sendEspData(closeCommand, 1000, DEBUG); // Close connection

            // Clear the char array
            memset(largeBuffer, 0, sizeof(largeBuffer));
            largeBufferIndex = 0;
            dataInType = 'E';
          } else {
            // TODO: handle timeout...
          }

        } else if (dataInType == 'G') {

          // Must be shorter than expected request
          char endOfDataInKeyword[] = {'\r', '\n', '\r', '\n'};
          bool endOfDataInKeywordFound = findKeywordInReceivedRequest(endOfDataInKeyword, largeBuffer, 4, largeBufferIndex - 1);

          if (endOfDataInKeywordFound) {
            hasDataInStarted = false;
            Serial.println(String(largeBuffer));
            // Serial.println("Boom WHAT");
            Serial.println(Serial1.readStringUntil('\n')); // Returns the post data..

            // Get the query params
            char queryParamsStartKey[] = {'G', 'E', 'T', ' ', '/', '?'};
            char queryParamsEndKey[] = {' ', 'H', 'T', 'T', 'P'};
            // TODO: Needs some error handling to be honest...
            int queryParamsStartIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, queryParamsStartKey, 6, 0, 1));
            int queryParamsEndIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, queryParamsEndKey, 5, 0, 1, 0));
            char queryParams[queryParamsEndIndex - queryParamsStartIndex + 2];
            int queryParamsCursorPosition = 0;
            for (int i = queryParamsStartIndex; i <= (queryParamsEndIndex); i++) {
              queryParams[queryParamsCursorPosition++] = largeBuffer[i];
            }
            queryParams[queryParamsCursorPosition] = '\0';

            const char queryParamsSeparator[2] = "&";
            // Get the first token
            char *token = strtok(queryParams, queryParamsSeparator); // Splits a C string into substrings, based on a separator character

            // Process the token / Walk through other tokens
            while (token != NULL) { // each token is the key value set, e.g. pin=13

              int valueLength = 0;

              // Split the token into a key-value pair
              char* separator = strchr(token, '='); // Returns a pointer to the first occurrence of the character c in the string str, or NULL if the character is not found
              if (separator != 0) { // If the key-value pair is indeed separated by an equals sign..
                // (separator + 1) = The value
                // Serial.println(separator + 1); // +1 to move the pointer across 1, i.e. to skip the equals sign
                // Serial.println("Value Length test: " + (String) strlen(separator + 1));
                valueLength = sizeof(separator) / sizeof(char);
                // Serial.println("Value Length test: " + (String) valueLength);
              }

              // The index for the start of the '=' sign
              int index = (int) (separator - token);
              // Serial.println(index);

              // The query param key
              char key[index + 1];
              memcpy(key, token /* Offset */, index /* Length */);
              key[index] = 0; // Add terminator
              // Serial.println(key);

              // The query param value (if you want to be explicit and not use (separator + 1)...)
              char value[valueLength + 1];
              memcpy(value, (separator + 1) /* Offset */, valueLength /* Length */);
              value[valueLength] = 0; // Add terminator
              // Serial.println(value);

              if (strcmp(key, "pin") == 0) {
                if (strcmp(value, "13") == 0) {
                  int pinNumber = atoi(value); // Converts char array to int
                  digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin {
                }
              }

              // For controlling the LED explicitly
              if (strcmp(key, "pin13") == 0) {
                if (strcmp(value, "1") == 0) {
                  int pinNumber = 13;
                  digitalWrite(pinNumber, 1); // Turn on LED
                } else if (strcmp(value, "0") == 0) {
                  int pinNumber = 13;
                  digitalWrite(pinNumber, 0); // Turn off LED
                }
              }

              Serial.println(token);
              // Walk through other tokens
              token = strtok(NULL, queryParamsSeparator);
            }

            // Get the connection id
            char comma[] = {','};
            int firstCommaIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, comma, 1, 0, 1));
            int secondCommaIndex = (findKeywordIndexInCharArray(largeBuffer, largeBufferIndex - 1, comma, 1, 0, 2, 0));
            char connectionId = '0';
            for (int i = firstCommaIndex; i <= (secondCommaIndex); i++) {
              connectionId = largeBuffer[i]; // Should just be 1 char long here (i.e. 0-4)
            }

            // Send a json response back
            String atCommand = "{\"test\":\"test\"}";
            // int sendRequestLength = ((String) atCommand).length();
            // sendEspData("AT+CIPSEND=" + (String) connectionId + "," + String(sendRequestLength) + "\r\n", 5000, DEBUG);
            // sendEspData(atCommand + "\r\n", 5000, DEBUG);
            sendHttpResponse(atoi(connectionId), atCommand);

            // Close the connection
            String closeCommand = "AT+CIPCLOSE=";
            closeCommand += connectionId; // Append connection id
            closeCommand += "\r\n";

            // When you sendHttpResponse, it automatically closes the connection, so don't need the below.
            // Would be good to check connection status, and if not connected, run the below, but no harm leaving.
            sendEspData(closeCommand, 1000, DEBUG); // Close connection

            // Clear the char array
            memset(largeBuffer, 0, sizeof(largeBuffer));
            largeBufferIndex = 0;
            dataInType = 'E';
          } else {
            // TODO: handle timeout...
          }

        }

      }

    } else {

      if (Serial1.find("+IPD,")) { // Moves cursor to the key after "+IPD, "
        hasDataInStarted = true;
        largeBuffer[largeBufferIndex++] = '+';
        largeBuffer[largeBufferIndex++] = 'I';
        largeBuffer[largeBufferIndex++] = 'P';
        largeBuffer[largeBufferIndex++] = 'D';
        largeBuffer[largeBufferIndex++] = ',';
        largeBuffer[largeBufferIndex++] = '\0';
        Serial.println("+IPD, found");
      }

    }


    /* 
      // Send a webpage back
      //      String webpage = "<h1>Hello</h1><h2>World!</h2><button>LED1</button>";
      //
      //      String cipSend = "AT+CIPSEND=";
      //      cipSend += connectionId;
      //      cipSend += ",";
      //      cipSend +=webpage.length();
      //      cipSend +="\r\n";
      //
      //      sendEspData(cipSend, 1000, DEBUG);
      //      sendEspData(webpage, 1000, DEBUG);
      //
      //      webpage="<button>LED2</button>";
      //
      //      cipSend = "AT+CIPSEND=";
      //      cipSend += connectionId;
      //      cipSend += ",";
      //      cipSend += webpage.length();
      //      cipSend +="\r\n";
      //
      //      sendEspData(cipSend, 1000, DEBUG);
      //      sendEspData(webpage, 1000, DEBUG);

      // Close the connection
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId; // Append connection id
      closeCommand += "\r\n";

      sendEspData(closeCommand, 1000, DEBUG); // Close connection
      }
    */
  }
}


/**
   Basically connects to an external server (e.g. thingspeak servers, or your own server)
   @params serverIpAddress - The ip address, or dns, e.g. \"api.thingspeak.com\"
   @returns 1 if successful or 0 if not
*/
boolean connectToExternalServer(String serverIpAddress) {
  Serial.println("Connecting to " + serverIpAddress + "...");

  sendEspData("AT+CIPSTART=0,\"TCP\",\"" + serverIpAddress + "\",80\r\n", 3000, DEBUG);
}


/**
   Read everything out of the serial buffer and what's still coming, and gets rid of it
   Basically found the keyword, so ignore everything else...
*/
void serialDumpEsp() {
  char temp;
  while (Serial1.available()) {
    temp = Serial1.read();
    delay(1); // Could play around with this value if buffer overflows are occuring
  }
}


String getJson(String host, String endpoint, String data) {
  String atCommand = "GET ";
  atCommand += endpoint + "?" + data + " ";
  atCommand += "HTTP/1.1\r\nHost: ";
  atCommand += host + "\r\n";
  atCommand += "\r\n";

  int sendRequestLength = ((String) atCommand).length();

  sendEspData("AT+CIPSEND=0," + String(sendRequestLength) + "\r\n", 5000, DEBUG);
  return sendEspData(atCommand + "\r\n", 5000, DEBUG);

  // Close the connection
  //  String closeCommand = "AT+CIPCLOSE=";
  //  closeCommand += 0; // Append connection id
  //  closeCommand += "\r\n";
  //
  //  sendEspData(closeCommand, 1000, DEBUG); // Close connection
}


/*

   @params host - The host name, e.g. yourdomainname.co.nz
   @params endpoint - The endpoint url e.g. "/main.php"
   @param data - (aka urlparams) e.g. "strawberries=17&lemons=22"
*/
String postJson(String host, String endpoint, String data) {
  String atCommand = "POST ";
  atCommand += endpoint + " ";
  atCommand += "HTTP/1.1\r\nHost: ";
  atCommand += host + "\r\n";
  atCommand += "Content-Type: application/x-www-form-urlencoded\r\n";
  atCommand += "Content-Length: " + (String) ((String)data).length() + "\r\n\r\n" + (String) ((String)data) + "\r\n";
  atCommand += "\r\n";

  int sendRequestLength = ((String) atCommand).length();

  sendEspData("AT+CIPSEND=0," + String(sendRequestLength) + "\r\n", 5000, DEBUG);
  return sendEspData(atCommand + "\r\n", 5000, DEBUG);
}


/*
   Returns the string starting from a start string, to end string.
   Useful to extract e.g. JSON data
*/
String midString(String str, String start, String finish) {
  int locStart = str.indexOf(start);
  if (locStart == -1) return "";
  locStart += start.length();
  int locFinish = str.indexOf(finish, locStart);
  if (locFinish == -1) return "";
  return str.substring(locStart, locFinish);
}


/*
   Send HTTP Response, i.e. for a get or post response
*/
void sendHttpResponse(int connectedId, String content) {
  // Build HTTP response
  String httpResponse;
  String httpHeader;
  // HTTP Header
  // httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n";
  httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\n";
  httpHeader += "Access-Control-Allow-Origin: *\r\n";
  httpHeader += "Content-Length: ";
  httpHeader += content.length();
  httpHeader += "\r\n";
  httpHeader += "Connection: close\r\n\r\n";
  httpResponse = httpHeader + content + " "; // There is a bug in this code: the last character of "content" is not send, cheated by adding this extra space.
  sendCipData(connectedId, httpResponse);
}


String sendEspResponseData(String command, const int timeout, boolean debug, String successKeyword = "", String failureKeyword = "") {
  String response = "";
  char dataInBuffer[DATA_IN_BUFFER_ARRAY_SIZE]; // This is the buffer - if the success/failureKeyword is longer than 20, then increase this
  int dataInBufferIndex = 0;
  bool isEndOfResponse = false;
  //  Serial1.print(command); // Send the input command to the esp8266

  // This mini block replaces Serial1.print because with arduino you can only send 64 bytes at a time
  // but with http responses, you need more for the header etc.
  int dataSize = command.length();
  char data[dataSize];
  command.toCharArray(data, dataSize);
  Serial1.write(data, dataSize);
  if (debug) {
    Serial.println("\r\n==== HTTP Response From Arduino ====");
    Serial.write(data, dataSize);
    Serial.println("\r\n====================================");
  }

  long int time = millis();

  while ((time + timeout) > millis()) {
    if (!isEndOfResponse) {
      while (Serial1.available()) {
        // The esp has data so display its output to the serial window
        char c = Serial1.read(); // read the next character.
        response += c;

        // Rolling buffer, so it holds the latest 20 chars (used for searching for keywords)
        if (dataInBufferIndex > (DATA_IN_BUFFER_ARRAY_SIZE - 1)) {
          dataInBufferIndex = (DATA_IN_BUFFER_ARRAY_SIZE - 1);
          for (int i = 0; i < (DATA_IN_BUFFER_ARRAY_SIZE - 1); i++) { // keysize-1 because everthing is shifted over - see next line
            dataInBuffer[i] = dataInBuffer[i + 1]; // So the data at 0 becomes the data at 1, and so on.... the last value is where we'll put the new data
            // So if your buffer was, 1234, it would become 2344, where that last '4' becomes the new data
          }
        }
        dataInBuffer[dataInBufferIndex] = c;
        dataInBufferIndex++;
      }

      // If a keyword is found, end the loop early, so you don't have to wait till the whole timeout time
      if (successKeyword.length() > 0 || failureKeyword.length() > 0) {
        // Terminate any unfilled char array with null, otherwise you get errors...
        if (dataInBufferIndex < DATA_IN_BUFFER_ARRAY_SIZE) {
          dataInBuffer[dataInBufferIndex] = '\0';
        }
        String dataInBufferStr = (String(dataInBuffer));
        dataInBufferStr.toUpperCase();
        successKeyword.toUpperCase();
        failureKeyword.toUpperCase();
        if (successKeyword.length() > 0 && dataInBufferStr.indexOf(successKeyword) >= 0) {
          isEndOfResponse = true;
        }
        if (failureKeyword.length() > 0 && dataInBufferStr.indexOf(failureKeyword) >= 0) {
          isEndOfResponse = true;
        }
      }
    } else {
      break;
    }
  }
  // TODO: Could probably handle case for when timeout exceeded..

  if (debug) {
    Serial.print(response);
  }

  return response;
}


/*
   Sends a CIPSEND=<connectionId>,<data> command
*/
void sendCipData(int connectionId, String data) {
  String cipSend = "AT+CIPSEND=";
  cipSend += connectionId;
  cipSend += ",";
  cipSend += data.length();
  cipSend += "\r\n";
  // TODO: Note atm this would close the connection twice (not a major, should fix, but dw for now)..
  sendEspData(cipSend, 1000, DEBUG); // sendCommand(cipSend, 1000, DEBUG);
  sendEspResponseData(data, 1000, DEBUG);
}


/*
   Note the ESP8266-01 came the following version pre-installed
   AT version:1.3.0.0(Jul 14 2016 18:54:01)
   SDK version:2.0.0(5a875ba)
   v1.0.0.3
   Mar 13 2018 09:35:47
*/
