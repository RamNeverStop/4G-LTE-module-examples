#include <HardwareSerial.h>

HardwareSerial SerialAT(2); // Use Serial2 for AT communication

void setup() {
  // Start serial communication with computer
  Serial.begin(115200);
  
  // Start serial communication with Quectel EC200U module
  SerialAT.begin(115200, SERIAL_8N1, 41, 40); // RX=18, TX=17 (example pins)

  delay(3000);
  
  Serial.println("ESP32-S3 Quectel EC200U Communication Setup");

  sendATCommand("AT");
  sendATCommand("AT+CSQ"); // Check signal quality
  sendATCommand("AT+CREG?"); // Check network registration

  // Set SMSC number to an Indian provider's SMSC (replace with correct number if necessary)
 // sendATCommand("AT+CSCA=\"+919898051914\""); // Replace with your provider's SMSC
  
  sendATCommand("AT+CMGF=1"); // Set SMS mode to text
  sendATCommand("AT+CSCS=\"GSM\""); // Set character set to GSM
  
  // Set SMS parameters
  sendATCommand("AT+CSMP=17,167,0,0");
  
  sendATCommand("AT+CMGS=\"8867128090\""); // Set the destination phone number
  sendATMessage("Hello, this is a test message from ESP32-S3!"); // Send the SMS content
}

void loop() {
  // Do nothing in loop
}

void sendATCommand(const char *command) {
  Serial.println(String("Sending: ") + command);
  SerialAT.println(command);
  delay(2000);
  while (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.println("Response: " + response);
  }
}

void sendATMessage(const char *message) {
  Serial.println("Sending: " + String(message));
  SerialAT.print(message);
  delay(500);
  SerialAT.write(26); // ASCII code for CTRL+Z to send the message
  delay(5000);
  while (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.println("Response: " + response);
  }
}
