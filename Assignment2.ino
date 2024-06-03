#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

// Create a web server object on port 80
ESP8266WebServer server(80);

// WiFi credentials and relay status
String ssid, password;
bool relayStatus = false;
const int relayPin = D1;

// Forward declarations of functions
void setupWiFi();
void handleRoot();
void handleToggle();
void handleSetting();
void saveCredentialsToEEPROM(const String& ssid, const String& password);
void saveRelayStatusToEEPROM();
void loadCredentialsFromEEPROM();
bool connectToWiFi();

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud
  EEPROM.begin(512);     // Initialize EEPROM with 512 bytes
  delay(100);

  pinMode(relayPin, OUTPUT);    // Set relay pin as output
  digitalWrite(relayPin, LOW);  // Turn relay off initially

  loadCredentialsFromEEPROM();  // Load stored WiFi credentials and relay status from EEPROM

  // Set relay to the last saved state
  digitalWrite(relayPin, relayStatus ? HIGH : LOW);

  // Set up WiFi connection
  setupWiFi();
}

void loop() {
  server.handleClient();  // Handle incoming client requests
}

void setupWiFi() {
  if (connectToWiFi()) {
    // Launch web server in STA mode if connected to WiFi
    Serial.println("Connected to WiFi");
    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.begin();
    Serial.println("Web server started in STA mode");
  } else {
    // Set up Access Point mode if WiFi connection fails
    const char* apSSID = "NodeMCU-AP";
    const char* apPassword = "";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    Serial.print("AP mode - http://");
    Serial.println(WiFi.softAPIP());  // Print the IP address of the AP

    // Launch web server in AP mode
    server.on("/", handleSetting);
    server.on("/setting", handleSetting);
    server.begin();
    Serial.println("Web server started in AP mode");
  }
}

void handleRoot() {
  // Serve the root page in STA mode
  String content = "<!DOCTYPE HTML><html>";
  content += "<head><title>NodeMCU Control</title></head>";
  content += "<body style='text-align:center;'>";
  content += "<h1>WiFi Mode</h1>";
  content += "<p>Welcome to Your NodeMCU</p>";
  content += "<p>Connected to SSID: " + ssid + "</p>";
  content += "<form method='get' action='toggle'>";
  content += "<input type='submit' value='Toggle Relay'></form>";
  content += "<p>Relay is currently " + String(digitalRead(relayPin) ? "ON" : "OFF") + "</p>";
  content += "</body></html>";
  server.send(200, "text/html", content);
}

void handleToggle() {
  // Toggle the relay status
  relayStatus = !relayStatus;
  digitalWrite(relayPin, relayStatus ? HIGH : LOW);
  saveRelayStatusToEEPROM();

  // Serve the relay status page
  String content = "<!DOCTYPE HTML><html>";
  content += "<head><title>NodeMCU Control</title></head>";
  content += "<body style='text-align:center;'>";
  content += "<h1>Relay Status</h1>";
  content += "<p>Relay is now " + String(relayStatus ? "ON" : "OFF") + "</p>";
  content += "<a href='/'>Go Back</a>";
  content += "</body></html>";
  server.send(200, "text/html", content);
}

void handleSetting() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    // If SSID and password are provided, save them to EEPROM
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveCredentialsToEEPROM(ssid, password);

    // Serve the configuration saved page
    String content = "<!DOCTYPE HTML><html>";
    content += "<head><title>NodeMCU Configuration</title></head>";
    content += "<body style='text-align:center;'>";
    content += "<h1>Configuration Saved</h1>";
    content += "<p>Success. Please reboot to apply the new settings.</p>";
    content += "</body></html>";
    server.send(200, "text/html", content);
  } else {
    // Serve the configuration page in AP mode
    String content = "<!DOCTYPE HTML><html>";
    content += "<head><title>NodeMCU Configuration</title></head>";
    content += "<body style='text-align:center;'>";
    content += "<h1>Access Point Mode</h1>";
    content += "<h2>NodeMCU WiFi Configuration</h2>";
    content += "<p><b>Current Configuration:</b></p>";
    content += "<p>SSID: " + ssid + "</p>";
    content += "<p>Password: " + password + "</p>";
    content += "<form method='get' action='setting'>";
    content += "<p><b>New Configuration:</b></p>";
    content += "<label>SSID: </label><br><input name='ssid' length=32><br>";
    content += "<label>Password: </label><br><input name='password' length=32><br>";
    content += "<br><input type='submit' value='Save'></form>";
    content += "</body></html>";
    server.send(200, "text/html", content);
  }
}

bool connectToWiFi() {
  // Attempt to connect to WiFi using stored credentials
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  while (attempts < 20) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.print(".");
    delay(500);
    attempts++;
  }
  Serial.println("Connection timed out");
  return false;
}

void saveCredentialsToEEPROM(const String& newSSID, const String& newPassword) {
  // Save WiFi credentials to EEPROM
  Serial.println("Writing to EEPROM");
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i, i < newSSID.length() ? newSSID[i] : 0);
  }
  for (int i = 32; i < 64; i++) {
    EEPROM.write(i, i - 32 < newPassword.length() ? newPassword[i - 32] : 0);
  }
  EEPROM.commit();
  Serial.println("Write successful");
}

void saveRelayStatusToEEPROM() {
  // Save relay status to EEPROM
  EEPROM.write(128, relayStatus ? 1 : 0);
  EEPROM.commit();
}

void loadCredentialsFromEEPROM() {
  // Load WiFi credentials and relay status from EEPROM
  Serial.println("Reading from EEPROM...");
  ssid = "";
  password = "";
  for (int i = 0; i < 32; i++) {
    ssid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 64; i++) {
    password += char(EEPROM.read(i));
  }
  relayStatus = EEPROM.read(128) == 1;
  Serial.println("WiFi SSID from EEPROM: " + ssid);
  Serial.println("WiFi password from EEPROM: " + password);
  Serial.println("Relay status from EEPROM: " + String(relayStatus ? "ON" : "OFF"));
  Serial.println("Reading successful.");
}

