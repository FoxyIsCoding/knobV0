#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <BleKeyboard.h>

// Rotary Encoder Pins
#define ENCODER_A 9
#define ENCODER_B 8
#define ENCODER_BTN 7

// Secondary Switch Pins
#define SWITCH_PIN1 2
#define SWITCH_PIN2 1

// Encoder state variables
volatile int encoderPos = 0;
volatile int lastEncoded = 0;
volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// BLE Keyboard
BleKeyboard bleKeyboard("ESP32 Volume Control", "Seeed Studio", 100);

// Web Server
WebServer server(80);

// HTML page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            background-color: white;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            font-family: Arial, sans-serif;
        }
        h1 {
            font-size: 120px;
            color: #333;
            text-align: center;
        }
    </style>
</head>
<body>
    <h1>Meow :3</h1>
</body>
</html>
)rawliteral";

void IRAM_ATTR handleEncoder() {
    int MSB = digitalRead(ENCODER_A);
    int LSB = digitalRead(ENCODER_B);
    
    int encoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | encoded;
    
    if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        encoderPos++;
    }
    if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        encoderPos--;
    }
    
    lastEncoded = encoded;
}

void IRAM_ATTR handleButton() {
    unsigned long currentTime = millis();
    if ((currentTime - lastDebounceTime) > debounceDelay) {
        buttonPressed = true;
        lastDebounceTime = currentTime;
    }
}

void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void setup() {
    Serial.begin(115200);
    
    // Setup encoder pins
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    pinMode(ENCODER_BTN, INPUT_PULLUP);
    
    // Setup secondary switch pins
    pinMode(SWITCH_PIN1, INPUT_PULLUP);
    pinMode(SWITCH_PIN2, INPUT_PULLUP);
    
    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_B), handleEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_BTN), handleButton, FALLING);
    
    // WiFi Manager setup
    WiFiManager wm;
    
    // Reset settings for testing (comment out after first run)
    // wm.resetSettings();
    
    Serial.println("Connecting to WiFi...");
    bool res = wm.autoConnect("ESP32_VolumeControl", "password123");
    
    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } else {
        Serial.println("Connected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
    
    // Start BLE Keyboard
    bleKeyboard.begin();
    Serial.println("Starting BLE Keyboard...");
    
    // Setup web server
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();
    
    static int lastEncoderPos = 0;
    
    // Handle encoder rotation for volume control
    if (encoderPos != lastEncoderPos && bleKeyboard.isConnected()) {
        int diff = encoderPos - lastEncoderPos;
        
        if (diff > 0) {
            // Volume up
            for(int i = 0; i < abs(diff); i++) {
                bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
                delay(10);
            }
            Serial.println("Volume Up");
        } else {
            // Volume down
            for(int i = 0; i < abs(diff); i++) {
                bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
                delay(10);
            }
            Serial.println("Volume Down");
        }
        
        lastEncoderPos = encoderPos;
    }
    
    // Handle encoder button press for mute toggle
    if (buttonPressed && bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_MUTE);
        Serial.println("Mute Toggle");
        buttonPressed = false;
        delay(200); // Debounce
    }
    
    // Display BLE connection status
    static bool lastBleStatus = false;
    if (bleKeyboard.isConnected() != lastBleStatus) {
        lastBleStatus = bleKeyboard.isConnected();
        if (lastBleStatus) {
            Serial.println("BLE Keyboard Connected");
        } else {
            Serial.println("BLE Keyboard Disconnected");
        }
    }
    
    delay(10);
}