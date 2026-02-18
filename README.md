# ESP32-S3 4G/LTE Cellular Module Examples

A comprehensive collection of examples for interfacing ESP32-S3 with popular 4G/LTE cellular modules (EC200U, EG25-G). Includes SMS sending, IoT cloud integration (ThingsBoard), GPS tracking with FreeRTOS, and internet connectivity testing.

## Hardware Requirements

- **Microcontroller**: ESP32-S3
- **4G/LTE Modules** (one or more):
  - Quectel EC200U-CN (Cat 1 LTE)
  - Quectel EG25-G (Cat 4 LTE)
- **SIM Card**: Active 4G/LTE data SIM
- **GPS Module** (optional): L89H for GPS tracking examples
- **Antenna**: 4G LTE antenna (required!)
- **Power**: 5V (4G modules need higher current)

## Project Files Overview

This repository contains three complete example projects:

### 1. EU200c_SMS_SENDER.ino
Simple SMS sending application using EC200U module.

**Features:**
- Send SMS messages via 4G network
- Basic AT command communication
- Network registration check
- Signal quality monitoring

### 2. EC200_tb_tele_data_sent_rtos_2.ino
Advanced IoT tracking system with ThingsBoard integration.

**Features:**
- GPS location tracking with L89H module
- Real-time data upload to ThingsBoard cloud
- FreeRTOS multi-tasking (GPS + data transmission)
- Signal strength, IMEI, ICCID reporting
- Distance tracking and speed monitoring
- UTC to IST timezone conversion

### 3. Eg25-g_internet_check.ino
Internet connectivity testing and initialization for EG25-G module.

**Features:**
- Module power control
- Network registration verification
- PDP context activation
- IP address assignment
- Signal quality check
- Connection diagnostics

## Pin Configuration

### EC200U Module Connections

| ESP32-S3 Pin | Function | EC200U Pin | Description |
|--------------|----------|------------|-------------|
| GPIO 40      | RX       | TX         | UART Receive |
| GPIO 41      | TX       | RX         | UART Transmit |
| GPIO 1       | RTS      | RTS        | Reset/Control |
| GPIO 2       | PWRKEY   | PWRKEY     | Power Key |
| 5V           | VBAT     | VBAT       | Power Supply |
| GND          | GND      | GND        | Ground |

### EG25-G Module Connections

| ESP32-S3 Pin | Function | EG25-G Pin | Description |
|--------------|----------|------------|-------------|
| GPIO 17      | TX       | RX         | UART Transmit |
| GPIO 18      | RX       | TX         | UART Receive |
| GPIO 48      | EN_4G    | PWRKEY     | Power Enable |
| 5V           | VBAT     | VBAT       | Power Supply |
| GND          | GND      | GND        | Ground |

### GPS Module (L89H) - For ThingsBoard Example

| ESP32-S3 Pin | Function | L89H Pin | Description |
|--------------|----------|----------|-------------|
| GPIO 38      | RX       | TX       | GPS UART RX |
| GPIO 39      | TX       | RX       | GPS UART TX |
| 3.3V         | VCC      | VCC      | Power |
| GND          | GND      | GND      | Ground |

**Important**: Always connect 4G antenna before powering on!

## Required Libraries

### For SMS Sender (EU200c_SMS_SENDER.ino):
- **HardwareSerial** (ESP32 built-in)

### For ThingsBoard Example (EC200_tb_tele_data_sent_rtos_2.ino):
- **TinyGPS++** by Mikal Hart
- **FreeRTOS** (ESP32 built-in)
- **HardwareSerial** (ESP32 built-in)

### For Internet Check (Eg25-g_internet_check.ino):
- **HardwareSerial** (ESP32 built-in)

## Installation

1. **Clone this repository:**
   ```bash
   git clone https://github.com/yourusername/esp32-s3-4g-examples.git
   ```

2. **Install required libraries:**
   - Open Arduino IDE
   - Go to **Sketch** → **Include Library** → **Manage Libraries**
   - Search for "TinyGPS++" and install (for ThingsBoard example)

3. **Select your board:**
   - **Tools** → **Board** → **ESP32 Arduino** → **ESP32S3 Dev Module**

4. **Configure settings:**
   - Partition Scheme: Choose based on your needs
   - PSRAM: Enable if available

5. **Choose example:**
   - Open the desired .ino file
   - Modify pins if needed
   - Upload to ESP32-S3

## Example 1: SMS Sender (EC200U)

### Quick Start

1. **Open** `EU200c_SMS_SENDER.ino`
2. **Modify** the phone number (line 29):
   ```cpp
   sendATCommand("AT+CMGS=\"YOUR_PHONE_NUMBER\"");
   ```
3. **Customize** the message (line 30):
   ```cpp
   sendATMessage("Your custom message here!");
   ```
4. **Upload** and open Serial Monitor (115200 baud)

### Serial Output
```
ESP32-S3 Quectel EC200U Communication Setup
Sending: AT
Response: OK
Sending: AT+CSQ
Response: +CSQ: 25,0
Sending: AT+CREG?
Response: +CREG: 0,1
Sending: AT+CMGF=1
Response: OK
Sending: AT+CMGS="8867128090"
Response: >
Sending: Hello, this is a test message from ESP32-S3!
Response: +CMGS: 123
OK
```

### Customization

**Change SMS Center (SMSC):**
```cpp
sendATCommand("AT+CSCA=\"+YOUR_SMSC_NUMBER\"");
```

**Send to multiple recipients:**
```cpp
sendATCommand("AT+CMGS=\"+919876543210\"");
sendATMessage("Message 1");
delay(5000);
sendATCommand("AT+CMGS=\"+918765432109\"");
sendATMessage("Message 2");
```

## Example 2: ThingsBoard IoT Tracker (EC200U + GPS)

### Configuration

1. **Set your APN** (line 21):
   ```cpp
   const char* apn = "your.apn.here";
   ```
   Common APNs:
   - Airtel: `airtelgprs.com`
   - Jio: `jionet`
   - Vodafone: `www` or `portalnmms`
   - BSNL: `bsnlnet`

2. **ThingsBoard credentials** (lines 22-23):
   ```cpp
   const char* thingsboardHost = "thingsboard.cloud";  // or your server
   const char* accessToken = "YOUR_ACCESS_TOKEN";      // from ThingsBoard device
   ```

3. **Verify pins** match your hardware (lines 6-12)

### How It Works

**FreeRTOS Task 1 - GPS Data Collection:**
- Continuously reads GPS NMEA sentences
- Parses location, speed, altitude, satellites
- Updates when new position received

**FreeRTOS Task 2 - Data Transmission:**
- Waits for GPS updates
- Collects cellular info (signal, IMEI, ICCID)
- Sends JSON telemetry to ThingsBoard
- Updates every second when GPS data changes

### JSON Telemetry Format
```json
{
  "latitude": 12.971599,
  "longitude": 77.594566,
  "altitude": 920.5,
  "speed": 15.3,
  "distance": 25.4,
  "date": "2026-2-18",
  "time": "18:30:45",
  "SignalStrength": 85,
  "ICCID": "89910123456789012345",
  "IMEI": "867123456789012",
  "satellites": 12,
  "gpsValid": true
}
```

### ThingsBoard Setup

1. **Create Device:**
   - Login to ThingsBoard
   - Devices → Add Device
   - Copy Access Token

2. **Create Dashboard:**
   - Add Map widget (latitude/longitude)
   - Add Gauge (speed)
   - Add Timeline (date/time)
   - Add Card (signal strength, satellites)

3. **Verify Data:**
   - Latest Telemetry tab should show incoming data
   - Map should update with GPS position

### Serial Output
```
EC200U is ready.
SIM is ready.
Registered to the network.
APN set successfully.
PDP context activated.
HTTP context ID configured.
Latitude: 12.971599 N
Longitude: 77.594566 E
Altitude: 920.5 meters
Speed: 15.3 km/h
Date: 2026-2-18
Time (IST): 18:30:45
HTTP POST request sent successfully.
```

## Example 3: Internet Check (EG25-G)

### Quick Start

1. **Open** `Eg25-g_internet_check.ino`
2. **Modify APN** (line 49):
   ```cpp
   sendATCommand("AT+CGDCONT=1,\"IP\",\"your.apn.here\"");
   ```
3. **Upload** and monitor initialization

### Initialization Sequence

```
1. Power ON module (GPIO 48 control)
2. Wait for "RDY" signal (10 seconds)
3. Check SIM card (AT+CPIN?)
4. Verify network registration (AT+CREG?)
5. Attach to packet domain (AT+CGATT=1)
6. Set APN (AT+CGDCONT)
7. Activate PDP context (AT+CGACT=1,1)
8. Get IP address (AT+CGPADDR=1)
9. Check signal quality (AT+CSQ)
```

### Serial Output
```
HI
4G PWR ON WAITING FOR 10 SEC
RDY Received, proceeding...
Send: AT
OK
Send: ATI
Quectel
EG25-G
Revision: EG25GGBR07A08M2G
Send: AT+CPIN?
+CPIN: READY
OK
Send: AT+CREG?
+CREG: 0,1
Send: AT+CGACT=1,1
OK
Send: AT+CGPADDR=1
+CGPADDR: 1,"10.123.45.67"
end
```

### Network Status Codes

**AT+CREG? Response:**
- `+CREG: 0,0` - Not registered, not searching
- `+CREG: 0,1` - Registered, home network ✅
- `+CREG: 0,2` - Not registered, searching
- `+CREG: 0,3` - Registration denied
- `+CREG: 0,5` - Registered, roaming ✅

**AT+CSQ Response:**
- `+CSQ: 0-9,99` - Poor signal
- `+CSQ: 10-14,99` - Fair signal
- `+CSQ: 15-19,99` - Good signal
- `+CSQ: 20-31,99` - Excellent signal ✅

## AT Commands Reference

### Basic Commands

| Command | Description | Response |
|---------|-------------|----------|
| AT | Test communication | OK |
| ATI | Module information | Module details |
| AT+CGMR | Firmware version | Version info |
| AT+CPIN? | Check SIM status | +CPIN: READY |
| AT+CREG? | Network registration | +CREG: 0,1 |
| AT+CSQ | Signal quality | +CSQ: 25,0 |

### SMS Commands

| Command | Description |
|---------|-------------|
| AT+CMGF=1 | Text mode SMS |
| AT+CMGS="number" | Send SMS |
| AT+CMGR=index | Read SMS |
| AT+CMGL="ALL" | List all SMS |
| AT+CMGD=index | Delete SMS |

### Network Commands

| Command | Description |
|---------|-------------|
| AT+CGATT=1 | Attach to network |
| AT+CGDCONT=1,"IP","apn" | Set APN |
| AT+CGACT=1,1 | Activate PDP context |
| AT+CGPADDR=1 | Get IP address |
| AT+QIDEACT=1 | Deactivate context |

### HTTP Commands (Quectel)

| Command | Description |
|---------|-------------|
| AT+QHTTPURL=len,timeout | Set URL |
| AT+QHTTPCFG="contextid",1 | Set context |
| AT+QHTTPPOST=len,t1,t2 | POST request |
| AT+QHTTPGET=timeout | GET request |
| AT+QHTTPREAD=timeout | Read response |

## Troubleshooting

### Module Not Responding
- **Check power**: 4G modules need 5V, high current (2A+)
- **Check antenna**: Must be connected before power-on
- **Reset module**: Power cycle or use PWRKEY
- **Verify UART pins**: RX/TX not swapped
- **Check baud rate**: 115200 for EC200U/EG25-G

### SIM Card Issues
- **Not detected**: Check SIM insertion, try different SIM
- **PIN locked**: Use `AT+CPIN="1234"` if PIN required
- **No signal**: Move to area with better coverage
- **Wrong APN**: Verify with carrier

### Network Registration Failed
- **Antenna**: Check connection and position
- **Signal**: Use `AT+CSQ` to verify strength
- **Carrier**: Ensure SIM has active data plan
- **Mode**: Try `AT+QCFG="nwscanmode",1` for LTE only

### GPS No Fix (ThingsBoard Example)
- **Antenna**: GPS antenna must be connected
- **Location**: Move outdoors with clear sky view
- **Wait time**: First fix can take 30+ seconds
- **Enable systems**: Use `AT+CGDCONT=1,1,1,1,1,1` to enable all GNSS

### HTTP POST Fails
- **Context**: Ensure PDP context active (`AT+CGACT=1,1`)
- **URL length**: Must match actual URL length
- **Timeout**: Increase timeout values if slow network
- **Server**: Verify ThingsBoard server accessible

### ThingsBoard No Data
- **Token**: Verify access token is correct
- **JSON format**: Check telemetry structure
- **Device**: Ensure device created in ThingsBoard
- **Network**: Confirm internet connectivity with ping

## Power Consumption Optimization

### Sleep Modes
```cpp
// Put module to sleep
sendATCommand("AT+QSCLK=1");

// Wake up
sendATCommand("AT+QSCLK=0");
```

### Reduce Update Frequency
```cpp
// In ThingsBoard example, change task delay:
vTaskDelay(pdMS_TO_TICKS(60000)); // Update every 60 seconds instead of 1
```

### Power Down When Idle
```cpp
sendATCommand("AT+QPOWD=1"); // Normal power down
```

## Advanced Features

### HTTP GET Request
```cpp
void httpGet(String url) {
    String urlCmd = "AT+QHTTPURL=" + String(url.length()) + ",80";
    sendATCommand(urlCmd, "CONNECT", 1000);
    EC200U.print(url);
    sendATCommand("", "OK", 2000);
    
    sendATCommand("AT+QHTTPGET=80", "OK", 30000);
    sendATCommand("AT+QHTTPREAD=80", "", 5000);
}
```

### MQTT for IoT
```cpp
// Configure MQTT
sendATCommand("AT+QMTCFG=\"recv/mode\",0,0,1");
sendATCommand("AT+QMTOPEN=0,\"broker.hivemq.com\",1883");
sendATCommand("AT+QMTCONN=0,\"clientID\"");

// Publish
String topic = "esp32/data";
String data = "{\"temp\":25}";
sendATCommand("AT+QMTPUB=0,0,0,0,\"" + topic + "\"");
EC200U.print(data);
```

### FTP File Upload
```cpp
sendATCommand("AT+QFTPCFG=\"contextid\",1");
sendATCommand("AT+QFTPOPEN=\"ftp.server.com\",21");
sendATCommand("AT+QFTPLOGIN=\"user\",\"pass\"");
sendATCommand("AT+QFTPPUT=\"file.txt\",\"COM:\",0");
```

## Module Comparison

| Feature | EC200U | EG25-G |
|---------|--------|--------|
| Category | Cat 1 | Cat 4 |
| Max Download | 10 Mbps | 150 Mbps |
| Max Upload | 5 Mbps | 50 Mbps |
| GNSS | No | Yes (built-in) |
| Protocols | HTTP, FTP, TCP/UDP, MQTT | HTTP, FTP, TCP/UDP, MQTT, SSL/TLS |
| Power | ~0.8W | ~1.5W |
| Price | Lower | Higher |
| Use Case | IoT sensors, tracking | Video streaming, high data |

## Safety Warnings

⚠️ **IMPORTANT**

- **Antenna**: ALWAYS connect antenna before power-on to avoid damage
- **Power supply**: Use adequate 5V supply (2A minimum)
- **SIM orientation**: Insert SIM correctly (check module datasheet)
- **ESD protection**: Handle modules with anti-static precautions
- **Regulations**: Ensure SIM and module comply with local regulations
- **Data charges**: Monitor data usage to avoid unexpected costs

## Use Cases

### SMS Sender
- 🚨 Alarm notifications
- 📢 Alert systems
- 🔔 Event notifications
- 🔐 2FA/OTP systems

### ThingsBoard Tracker
- 🚗 Vehicle fleet tracking
- 📦 Asset monitoring
- 🚴 Personal GPS tracking
- 🌾 Agricultural equipment monitoring

### Internet Check
- 🔧 Module testing and debugging
- 🌐 Connectivity verification
- 📊 Network quality monitoring
- ⚙️ Production line testing

## License

This project is open source and available under the MIT License.

## Contributing

Pull requests are welcome! Areas for improvement:
- Add more cloud platforms (AWS IoT, Azure)
- Implement MQTT examples
- Add voice call examples
- Create OTA update via 4G

## Author

Your Name

## Resources

- [EC200U AT Commands Manual](https://www.quectel.com/product/lte-ec200u-cn)
- [EG25-G AT Commands Manual](https://www.quectel.com/product/lte-eg25-g)
- [ThingsBoard Documentation](https://thingsboard.io/docs/)
- [TinyGPS++ Library](http://arduiniana.org/libraries/tinygpsplus/)
- [ESP32-S3 Datasheet](https://www.espressif.com/en/products/socs/esp32-s3)

## Acknowledgments

- Quectel for EC200U and EG25-G modules
- ThingsBoard team for IoT platform
- Espressif for ESP32-S3 platform
- Mikal Hart for TinyGPS++ library
- Arduino and FreeRTOS communities
