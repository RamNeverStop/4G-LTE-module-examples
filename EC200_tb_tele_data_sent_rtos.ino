#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <FreeRTOS.h>

#define RTS_4G     1
#define PW_4G      2
#define TX_4G      41
#define RX_4G      40

#define TX_GPS     39
#define RX_GPS     38

// Create GPS object
TinyGPSPlus gps;

// Create hardware serial objects for L89H and EC200U
HardwareSerial gpsSerial(1);
HardwareSerial EC200U(2);

const char* apn = "airtelgprs.com"; // APN for your SIM
const char* thingsboardHost = "thingsboard.cloud";
const char* accessToken = "RaX1CkFX2orF22tmnkuX";

double prevLat = 0.0;
double prevLng = 0.0;
bool hasPrevLocation = false;
bool gpsDataUpdated = false; // Flag to indicate GPS data is updated

void setup() {
  Serial.begin(115200);
  pinMode(RTS_4G, OUTPUT);
  pinMode(PW_4G, OUTPUT);

  digitalWrite(RTS_4G, LOW);  
  digitalWrite(PW_4G, LOW);  

  // Switch to internal antenna and turn antenna power ON
  sendAntennaCommand();
  parseAntennaStatus("$PSTMANTENNASTATUS,0,1,1,0*6F");

  // Initialize GPS module
  gpsSerial.begin(9600, SERIAL_8N1, RX_GPS, TX_GPS); // RX=2, TX=1 (modify pins as per your connection)
  gpsSerial.println("$PSTMCFGCONST,0,0,0,0,0,2*23"); // Enabling IRNSS
  delay(1000);
  gpsSerial.println("$PSTMCFGCONST,1,0,0,0,0,0*3E"); // Enabling GPS 
  delay(1000);
  gpsSerial.println("$PSTMCFGCONST,1,1,1,1,1,1*20"); // Enabling GPS, GLONASS, Galileo, QZSS, BeiDou, IRNSS
  
  // Initialize EC200U module
  EC200U.begin(115200, SERIAL_8N1, RX_4G, TX_4G); // RX, TX pins for EC200U
  if (!sendATCommand("AT", "OK", 1000)) {
    Serial.println("Failed to communicate with EC200U.");
    digitalWrite(RTS_4G, HIGH);  
    delay(250);
    Serial.println("4G_module resetted, waiting for 15 s");                      
    digitalWrite(RTS_4G, LOW);   
    delay(15000);  
    esp_restart();                    
    return;
  }
  Serial.println("EC200U is ready.");

  if (!sendATCommand("AT+CPIN?", "+CPIN: READY", 5000)) {
    Serial.println("SIM not ready or not inserted.");
    return;
  }
  Serial.println("SIM is ready.");

  if (!sendATCommand("AT+CREG?", "+CREG: 0,1", 5000) && !sendATCommand("AT+CREG?", "+CREG: 0,5", 5000)) {
    Serial.println("Not registered to the network.");
    return;
  }
  Serial.println("Registered to the network.");

  sendATCommand("AT+QIDEACT=1", "OK", 5000);

  String apnCommand = "AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"";
  if (!sendATCommand(apnCommand, "OK", 1000)) {
    Serial.println("Failed to set APN.");
    return;
  }
  Serial.println("APN set successfully.");

  if (!sendATCommand("AT+CGACT=1,1", "OK", 1000)) {
    Serial.println("Failed to activate PDP context.");
    return;
  }
  Serial.println("PDP context activated.");

  if (!sendATCommand("AT+CGPADDR=1", "+CGPADDR", 1000)) {
    Serial.println("Failed to query PDP context.");
    return;
  }
  Serial.println("PDP context query successful.");

  if (!sendATCommand("AT+QHTTPCFG=\"contextid\",1", "OK", 1000)) {
    Serial.println("Failed to configure HTTP context ID.");
    return;
  }
  Serial.println("HTTP context ID configured.");

  // Create FreeRTOS tasks
  xTaskCreate(receiveGPSTask, "ReceiveGPS", 4096, NULL, 2, NULL); // Set higher priority for GPS task
  xTaskCreate(sendDataTask, "SendData", 4096, NULL, 1, NULL);
}

void loop() {
  // Empty loop since tasks are handled by FreeRTOS
}


void sendAntennaCommand() {
    // Command to switch to internal antenna and turn the antenna power ON
    String command = "$PSTMSETANTSENSMANUAL,1,0,1,0";
    
    // Calculate checksum
    int checksum = 0;
    for (size_t i = 1; i < command.length(); i++) { // Start from 1 to skip the '$'
        checksum += command[i];
    }
    checksum = checksum % 256; // Modulo 256
    String checksumHex = String(checksum, HEX);
    if (checksumHex.length() < 2) {
        checksumHex = "0" + checksumHex; // Pad with leading zero if needed
    }

    // Convert checksum to uppercase (modify in place)
    checksumHex.toUpperCase(); // This modifies checksumHex in place
    command += "*" + checksumHex + "\r\n"; // Append checksum and CRLF
    gpsSerial.print(command); // Send the command to the GPS module
    Serial.println(command); // For debugging purposes
}


// Task to receive GPS data
void receiveGPSTask(void *pvParameters) {
  while (1) {
    while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      //Serial.write(c);
      gps.encode(c);
    }

    if (gps.location.isUpdated()) {
      gpsDataUpdated = true;
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
  }
}

// Task to send data to ThingsBoard
void sendDataTask(void *pvParameters) {
  while (1) {
    if (gpsDataUpdated || !gps.location.isValid()) {
      gpsDataUpdated = false;

      double currentLat = gps.location.isValid() ? gps.location.lat() : 0.0;
      double currentLng = gps.location.isValid() ? gps.location.lng() : 0.0;
      double distance = 0.0;

      if (gps.location.isValid() && hasPrevLocation) {
        distance = TinyGPSPlus::distanceBetween(prevLat, prevLng, currentLat, currentLng);
      }

      prevLat = currentLat;
      prevLng = currentLng;
      hasPrevLocation = gps.location.isValid();

      // Convert UTC to IST
      int utcHour = gps.time.hour();
      int utcMinute = gps.time.minute();
      int utcSecond = gps.time.second();
      int istHour, istMinute, istSecond;

      convertUTCtoIST(utcHour, utcMinute, utcSecond, istHour, istMinute, istSecond);

      // Prepare telemetry data
      String postData = "{";
      if (gps.location.isValid()) {
        postData += "\"latitude\":" + String(currentLat, 6) + ",";
        postData += "\"longitude\":" + String(currentLng, 6) + ",";
        postData += "\"altitude\":" + String(gps.altitude.meters()) + ",";
        postData += "\"speed\":" + String(gps.speed.kmph()) + ",";
        postData += "\"distance\":" + String(distance) + ",";
        postData += "\"date\":\"" + String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + "\",";
        postData += "\"time\":\"" + String(istHour) + ":" + String(istMinute) + ":" + String(istSecond) + "\",";
      }
      
      postData += "\"SignalStrength\":" + String(getSignalStrength()) + ",";
      postData += "\"ICCID\":\"" + getSIMNumber() + "\",";
      postData += "\"IMEI\":\"" + getIMEINumber() + "\",";
      postData += "\"satellites\":" + String(gps.satellites.value()) + ",";
      postData += "\"gpsValid\":" + String(gps.location.isValid() ? "true" : "false");
      postData += "}";

      // Print the date and time being sent
      Serial.print("Date: ");
      Serial.print(gps.date.year());
      Serial.print("-");
      Serial.print(gps.date.month());
      Serial.print("-");
      Serial.println(gps.date.day());

      Serial.print("Time (IST): ");
      Serial.print(istHour);
      Serial.print(":");
      Serial.print(istMinute);
      Serial.print(":");
      Serial.println(istSecond);

      sendTelemetryData(postData);
      displayLocation();
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
  }
}

void convertUTCtoIST(int utcHour, int utcMinute, int utcSecond, int &istHour, int &istMinute, int &istSecond) {
  // Convert UTC to IST
  istHour = (utcHour + 5) % 24; // Adjust hour for IST
  istMinute = (utcMinute + 30) % 60; // Adjust minutes for IST
  if (istMinute < 30) {
    istHour = (istHour + 23) % 24; // If minutes exceed, adjust hour
  }
  istSecond = utcSecond; // Keep seconds same
}

void sendTelemetryData(String postData) {
  String url = "http://" + String(thingsboardHost) + "/api/v1/" + String(accessToken) + "/telemetry";
  String urlCommand = "AT+QHTTPURL=" + String(url.length()) + ",80";

  if (!sendATCommand(urlCommand, "CONNECT", 1000)) {
    Serial.println("Failed to set HTTP URL command.");
    return;
  }
  EC200U.print(url);
  if (!sendATCommand("", "OK", 2000)) {
    Serial.println("Failed to set HTTP URL.");
    return;
  }

  if (!sendATCommand("AT+QHTTPCFG=\"contenttype\",1", "OK", 1000)) {
    Serial.println("Failed to set HTTP content type.");
    return;
  }

  String postCommand = "AT+QHTTPPOST=" + String(postData.length()) + ",80,80";
  if (!sendATCommand(postCommand, "CONNECT", 2000)) {
    Serial.println("Failed to send HTTP POST command.");
    return;
  }
  EC200U.print(postData);
  if (!sendATCommand("", "OK", 2000)) {
    Serial.println("Failed to send HTTP POST request.");
    return;
  }
  Serial.println("");
  Serial.println("HTTP POST request sent successfully.");
  Serial.println("");
}

bool sendATCommand(String command, String response, uint32_t timeout) {
  EC200U.println(command);
  uint32_t start = millis();
  String buffer = "";
  while (millis() - start < timeout) {
    if (EC200U.available()) {
      char c = EC200U.read();
      buffer += c;
      Serial.write(c); // Print received characters to Serial Monitor for debugging
      if (buffer.indexOf(response) != -1) {
        return true;
      }
    }
  }
  return false;
}

String getSignalStrength() {
  if (!sendATCommand("AT+CSQ", "+CSQ:", 1000)) {
    Serial.println("Failed to get signal strength.");
    return "0";
  }
  // Extract the signal strength from the response
  // Assuming response is "+CSQ: <rssi>,<ber>"
  String response = EC200U.readString();
  int rssi = response.substring(response.indexOf(":") + 2, response.indexOf(",")).toInt();
  int percentage = map(rssi, 0, 31, 0, 100); // Convert RSSI to percentage
  return String(percentage);
}

String getSIMNumber() {
  if (!sendATCommand("AT+QCCID", "+QCCID:", 1000)) {
    Serial.println("Failed to get SIM number.");
    return "";
  }
  // Extract the SIM number from the response
  String response = EC200U.readString();
  String simNumber = response.substring(response.indexOf(":") + 2);
  simNumber.trim();
  return simNumber;
}


String getIMEINumber() {
  if (!sendATCommand("AT+GSN", "", 1000)) {
    Serial.println("Failed to get IMEI number.");
    return "";
  }
  // Extract the IMEI number from the response
  String response = EC200U.readString();
  response.trim();
  return response;
}

void displayLocation() {
  if (gps.location.isUpdated()) {
    double currentLat = gps.location.lat();
    double currentLng = gps.location.lng();

    Serial.print("Latitude: ");
    Serial.print(currentLat, 6);
    Serial.print(" ");
    Serial.println(gps.location.rawLat().negative ? "S" : "N");

    Serial.print("Longitude: ");
    Serial.print(currentLng, 6);
    Serial.print(" ");
    Serial.println(gps.location.rawLng().negative ? "W" : "E");

    if (hasPrevLocation) {
      double distance = TinyGPSPlus::distanceBetween(prevLat, prevLng, currentLat, currentLng);
      Serial.print("Distance from previous location: ");
      Serial.print(distance);
      Serial.println(" meters");
    }

    // Update previous coordinates
    prevLat = currentLat;
    prevLng = currentLng;
    hasPrevLocation = true;
  } else if (gps.location.isValid()) {
    Serial.println("Valid GPS data but location not updated.");
  } else {
    Serial.println("Waiting for valid GPS fix...");
  }

  if (gps.altitude.isUpdated()) {
    Serial.print("Altitude: ");
    Serial.print(gps.altitude.meters());
    Serial.println(" meters");
  }

  if (gps.speed.isUpdated()) {
    Serial.print("Speed: ");
    Serial.print(gps.speed.kmph());
    Serial.println(" km/h");
  }

  if (gps.date.isUpdated()) {
    Serial.print("Date (UTC): ");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.println(gps.date.year());
  }

  if (gps.time.isUpdated()) {
    Serial.print("Time (UTC): ");
    if (gps.time.hour() < 10) Serial.print('0');
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print('0');
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print('0');
    Serial.println(gps.time.second());

    int utcHour = gps.time.hour();
    int utcMinute = gps.time.minute();
    int utcSecond = gps.time.second();

    // Convert UTC to IST
    int istHour, istMinute, istSecond;
    convertUTCtoIST(utcHour, utcMinute, utcSecond, istHour, istMinute, istSecond);

    Serial.print("Time (IST): ");
    if (istHour < 10) Serial.print('0');
    Serial.print(istHour);
    Serial.print(":");
    if (istMinute < 10) Serial.print('0');
    Serial.print(istMinute);
    Serial.print(":");
    if (istSecond < 10) Serial.print('0');
    Serial.println(istSecond);
  }

  if (gps.satellites.isUpdated()) {
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
  }

  if (gps.hdop.isUpdated()) {
    Serial.print("HDOP: ");
    Serial.println(gps.hdop.value());
  }
}

void parseAntennaStatus(String nmeaMessage) {
    // Check if the message is a PSTMANTENNASTATUS message
    if (nmeaMessage.startsWith("$PSTMANTENNASTATUS")) {
        // Remove the $PSTMANTENNASTATUS part of the message
        nmeaMessage = nmeaMessage.substring(19); // Skip the first 19 characters
        
        // Split the message by commas
        int ant_status = nmeaMessage.substring(0, nmeaMessage.indexOf(',')).toInt();
        nmeaMessage = nmeaMessage.substring(nmeaMessage.indexOf(',') + 1);
        
        int op_mode = nmeaMessage.substring(0, nmeaMessage.indexOf(',')).toInt();
        nmeaMessage = nmeaMessage.substring(nmeaMessage.indexOf(',') + 1);
        
        int rf_path = nmeaMessage.substring(0, nmeaMessage.indexOf(',')).toInt();
        nmeaMessage = nmeaMessage.substring(nmeaMessage.indexOf(',') + 1);
        
        int pwr_switch = nmeaMessage.substring(0, nmeaMessage.indexOf('*')).toInt();
        
        // Print out the parsed information
        Serial.println("Antenna Status:");
        
        // Interpret and display antenna status
        switch (ant_status) {
            case 0:
                Serial.println("  Status: Normal");
                break;
            case 1:
                Serial.println("  Status: Open");
                break;
            case 2:
                Serial.println("  Status: Short-circuited");
                break;
            default:
                Serial.println("  Status: Unknown");
                break;
        }
        
        // Interpret and display operating mode
        Serial.print("  Operating Mode: ");
        if (op_mode == 0) {
            Serial.println("Auto (software controlled)");
        } else if (op_mode == 1) {
            Serial.println("Manual (command controlled)");
        } else {
            Serial.println("Unknown");
        }
        
        // Interpret and display RF path
        Serial.print("  RF Path: ");
        if (rf_path == 0) {
            Serial.println("External antenna");
        } else if (rf_path == 1) {
            Serial.println("Internal antenna");
        } else {
            Serial.println("Unknown");
        }
        
        // Interpret and display power switch status
        Serial.print("  Antenna Power: ");
        if (pwr_switch == 0) {
            Serial.println("ON");
        } else if (pwr_switch == 1) {
            Serial.println("OFF");
        } else {
            Serial.println("Unknown");
        }
    }
}

