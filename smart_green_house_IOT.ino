#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"    // For token generation and status
#include "addons/RTDBHelper.h"    // For RTDB helper functions
#include <DHT.h>

// Wi-Fi and Firebase Configuration "Niwa003" "Nivein123" "SLT-Fiber" "Slt@235953"  iPhone di Wasana  wasana77      
#define WIFI_SSID "iPhone di Wasana"
#define WIFI_PASSWORD "wasana77"
#define API_KEY "AIzaSyD4wWSS9Wa68eZyWZF4-x8BgkMsLt-5upo"
#define DATABASE_URL "https://greenhouse-71b7c-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Pins Configuration
#define LDR_PIN 32              // Digital GPIO pin for LDR
#define GAS_SENSOR_PIN 34       // Analog GPIO pin for Gas Sensor
#define DHT_PIN 26              // GPIO pin for DHT sensor
#define DHT_TYPE DHT11          // Change to DHT22 if using DHT22
#define MOTOR_PIN 16
#define LED_PIN 15

// Firebase and Sensor Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

int ldrData = 0;                // Store LDR readings
int gasData = 0;                // Store Gas sensor readings
float temperature = 0.0;        // Store temperature readings
float humidity = 0.0;           // Store humidity readings

void connectWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Connect to Wi-Fi
  connectWiFi();

  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up for Firebase anonymous authentication
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase authentication successful!");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign-up error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT sensor initialized.");

  // Configure GPIO pins
  pinMode(LDR_PIN, INPUT);  // Configure LDR pin as input

  // Configure Motor Pin
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW); // Ensure motor is off initially

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Ensure LED is off initially
}

void loop() {
  // Reconnect Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Only proceed if Firebase is ready and signed up
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Read LDR and Gas sensor data
    ldrData = digitalRead(LDR_PIN);         // Read LDR (digital)
    gasData = analogRead(GAS_SENSOR_PIN);  // Read Gas sensor (analog)

    // Read DHT sensor data
    temperature = dht.readTemperature();   // Read temperature
    humidity = dht.readHumidity();         // Read humidity

    // Debugging DHT sensor
    if (isnan(temperature)) {
      Serial.println("Failed to read temperature from DHT sensor.");
    } else {
      Serial.printf("Temperature: %.2f °C\n", temperature);
    }

    if (isnan(humidity)) {
      Serial.println("Failed to read humidity from DHT sensor.");
    } else {
      Serial.printf("Humidity: %.2f %%\n", humidity);
    }

    // Send valid data to Firebase
    if (!isnan(temperature)) {
      if (Firebase.RTDB.setFloat(&fbdo, "Sensor/temperature", temperature)) {
        Serial.println("Temperature successfully sent to Firebase.");
      } else {
        Serial.printf("Failed to send temperature: %s\n", fbdo.errorReason().c_str());
      }
    }

    if (!isnan(humidity)) {
      if (Firebase.RTDB.setFloat(&fbdo, "Sensor/humidity", humidity)) {
        Serial.println("Humidity successfully sent to Firebase.");
      } else {
        Serial.printf("Failed to send humidity: %s\n", fbdo.errorReason().c_str());
      }
    }

    // Send LDR and Gas sensor data to Firebase
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/ldr_data", ldrData)) {
      Serial.println("LDR data successfully sent to Firebase.");
    } else {
      Serial.printf("Failed to send LDR data: %s\n", fbdo.errorReason().c_str());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Sensor/gas_data", gasData)) {
      Serial.println("Gas sensor data successfully sent to Firebase.");
    } else {
      Serial.printf("Failed to send Gas data: %s\n", fbdo.errorReason().c_str() );
    }

    // Read the fan control value from Firebase (e.g., "fan_status" is a boolean in Firebase)
    bool motorState = false; // Default to off
    if (Firebase.RTDB.getBool(&fbdo, "Control/motor_state")) {
      motorState = fbdo.boolData();
      Serial.printf("Motor State received from Firebase: %s\n", motorState ? "ON" : "OFF");

      // Control the motor based on Firebase value
      if (motorState == true) {
        digitalWrite(MOTOR_PIN, HIGH); // Turn motor on
        Serial.println("Motor turned ON.");
      } else {
        digitalWrite(MOTOR_PIN, LOW);  // Turn motor off
        Serial.println("Motor turned OFF.");
      }
    } else {
      Serial.printf("Failed to read motor state: %s\n", fbdo.errorReason().c_str());
    }

    // Read the LED state from Firebase
    bool ledState = false; // Default to OFF
    if (Firebase.RTDB.getBool(&fbdo, "Control/led_state")) { // Replace with your Firebase path
      ledState = fbdo.boolData(); // Get the value of led_state
      Serial.printf("LED State received from Firebase: %s\n", ledState ? "ON" : "OFF");

      // Control the LED based on Firebase value
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.println(ledState ? "LED turned ON." : "LED turned OFF.");
    } else {
      Serial.printf("Failed to read LED state: %s\n", fbdo.errorReason().c_str());
    }
  }
}