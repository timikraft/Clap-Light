#include <WiFi.h>
#include <HTTPClient.h>

// ==================== WIFI SETTINGS ====================
const char* WIFI_SSID = "YOUR_SSID";        // <--- Enter your WiFi name
const char* WIFI_PASS = "YOUR_PASSWORD";    // <--- Enter your WiFi password

// ==================== TASMOTA SETTINGS ====================
const char* TASMOTA_IP = "192.168.1.XXX";   // <--- Enter your Device IP
const char* TASMOTA_CMD = "POWER TOGGLE";   // Command: ON, OFF, or TOGGLE

// ==================== MICROPHONE SETTINGS ====================
const int MIC_PIN = 34;            // AO connected to GPIO34
const int THRESHOLD = 600;         // Sensitivity threshold (adjust this based on serial plotter)
const int MAX_PEAK_DURATION = 50;  // ms, max duration of a sound to be counted as a "clap"
const int CLAP_DELAY = 1500;       // ms cooldown between claps to prevent double switching

unsigned long lastClapTime = 0;
unsigned long peakStart = 0;
bool inPeak = false;

void setup() {
  Serial.begin(115200); // Higher baud rate for better plotting
  delay(1000);

  Serial.println("Starting ESP32 Clap-to-Tasmota Project...");

  // Set ADC attenuation to 11dB to allow reading up to ~3.1V
  analogSetPinAttenuation(MIC_PIN, ADC_11db);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int micValue = analogRead(MIC_PIN);
  
  // Use Serial Plotter to see this line
  // Serial.println(micValue);

  // Check if signal is above the threshold
  if (micValue > THRESHOLD) {
    if (!inPeak) {
      inPeak = true;
      peakStart = millis();
    }
  } else {
    if (inPeak) {
      inPeak = false;
      unsigned long duration = millis() - peakStart;
      
      // If the sound was short enough (like a clap, not a long scream)
      if (duration > 0 && duration <= MAX_PEAK_DURATION) {
        unsigned long now = millis();
        
        // Cooldown check
        if (now - lastClapTime > CLAP_DELAY) {
          lastClapTime = now;
          Serial.println("Clap detected! Sending Tasmota command...");
          sendTasmotaCommand();
        }
      }
    }
  }

  delay(1); // Small delay for stability
}

void sendTasmotaCommand() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Command aborted.");
    return;
  }

  HTTPClient http;
  // Construct the Tasmota URL
  String url = String("http://") + TASMOTA_IP + "/cm?cmnd=" + urlencode(TASMOTA_CMD);
  
  Serial.print("Requesting URL: ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.print("Tasmota Response Code: ");
    Serial.println(httpCode);
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Helper function for URL encoding
String urlencode(const String &str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += char(code0 + (code0 < 10 ? '0' : 'A' - 10));
      encoded += char(code1 + (code1 < 10 ? '0' : 'A' - 10));
    }
  }
  return encoded;
}
