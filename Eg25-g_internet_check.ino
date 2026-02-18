#include <SoftwareSerial.h>

#define RXD2 18
#define TXD2 17
#define EN_4G 48

HardwareSerial serialAT(2);

void setup() {
  Serial.begin(115200);
  serialAT.begin(115200, SERIAL_8N1, RXD2, TXD2);
  pinMode(EN_4G, OUTPUT);
  Serial.println("HI");
  delay(2000);
  // Power Cycle the Module
  digitalWrite(EN_4G, LOW);  // Turn off
  delay(2000);
  digitalWrite(EN_4G, HIGH);   // Turn on
  Serial.println("4G PWR ON WAITING FOR 10 SEC");
  delay(10000);  // Give time for the module to initialize

  //**Wait for RDY before sending AT commands**
  if (!waitForResponse("RDY", 10000)) {
    Serial.println("Error: No RDY signal received!");
   // return;
  }
  Serial.println("RDY Received, proceeding...");

  // **AT Command Sequence**
  sendATCommand("AT");
  sendATCommand("ATI");


  
  if (sendATCommand("AT+CPIN?").indexOf("READY") == -1) {
    Serial.println("SIM not ready");
    //return;
  }



  if (!checkNetworkRegistration()) {
    Serial.println("Not registered to network");
    //return;
  }

  sendATCommand("AT+CGATT=1");  // Attach to packet domain
  sendATCommand("AT+CGACT=0,1");  // Deactivate any existing PDP context
  sendATCommand("AT+CGDCONT=1,\"IP\",\"airtelgprs.com\"");
  //sendATCommand("AT+QICSGP=1,1,\"airtelgprs.com\",1");  // Set PDP context
  
  if (sendATCommand("AT+CGACT=1,1").indexOf("OK") == -1) {
    Serial.println("Failed to activate PDP context");
   // return;
  }

  sendATCommand("AT+CGPADDR=1");  // Get IP Address
  sendATCommand("AT+CSQ");  // Signal Quality
  sendATCommand("AT+CEREG?");  // Network Registration Status
 Serial.println("end");

}

void loop() {
  while (serialAT.available()) {
    Serial.write(serialAT.read());
  }
}

// Function to send AT commands and return response
String sendATCommand(String cmd) {
  serialAT.println(cmd);
  Serial.print("Send: ");
  Serial.println(cmd);

  String response = "";
  long timeout = millis() + 5000;  // 5 seconds timeout
  while (millis() < timeout) {
    while (serialAT.available()) {
      char c = serialAT.read();
      response += c;
      Serial.write(c);
    }
  }
 // Serial.println();
  return response;
}

bool waitForResponse(String expected, int timeout) {
  String response = "";
  long endTime = millis() + timeout;

  while (millis() < endTime) {
    while (serialAT.available()) {
      char c = serialAT.read();
      
      // Ignore non-printable characters (garbage data)
      if (c >= 32 && c <= 126) {  
        response += c;
      }

      Serial.write(c);  // Print received data for debugging

      // Check if expected response is received
      if (response.indexOf(expected) != -1) {
        Serial.println("\nExpected response received: " + expected);
        return true;
      }

      // Prevent buffer overflow (store only last 50 chars)
      if (response.length() > 50) {
        response = response.substring(response.length() - 50);
      }
    }
  }
  Serial.println("\nTimeout waiting for: " + expected);
  return false;
}



// Function to check network registration
bool checkNetworkRegistration() {
  String regStatus = sendATCommand("AT+CREG?");
  return (regStatus.indexOf("+CREG: 0,1") != -1 || regStatus.indexOf("+CREG: 0,5") != -1);
}
