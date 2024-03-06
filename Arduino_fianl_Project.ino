#include <WiFiManager.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


#define I2CADDR 0x20
#define SS_PIN 5          
#define RST_PIN 16       
const int relayPin = 13;   
const int buttonPin = 15;  
const int hallSensorPin = 12;
int stored_value;
// const char* apiEndpoint = "https://xxxxxxxxxxxxx/rfid";              /////////// url api
// const char* fastApiUrl = "https://xxxxxxxxxxxxxx";
// const char* serverUrl = "https://xxxxxxxxxxxxxx/get_otp_api";
const long utcOffsetInSeconds = 25200;  
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MFRC522 mfrc522(SS_PIN, RST_PIN);
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '4', '7', '*' },
  { '2', '5', '8', '0' },
  { '3', '6', '9', '#' },
  { 'A', 'B', 'C', 'D' }
};

byte rowPins[ROWS] = { 0, 1, 2, 3 };
byte colPins[COLS] = { 4, 5, 6, 7 };
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);

LiquidCrystal_I2C lcd(0x27, 16, 2);

static String inputCode;
static bool otpMode = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  Wire.begin();
  keypad.begin(makeKeymap(keys));
  mfrc522.PCD_Init();
  WiFiManager wifiManager;
  wifiManager.autoConnect("Door_iot");
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  pinMode(buttonPin, INPUT);
  Serial.println("Connected to WiFi!");
  pinMode(hallSensorPin, INPUT);
  lcd.begin();      
  lcd.backlight(); 
}

void loop() {
  int hallSensorValue = digitalRead(hallSensorPin);

  if (digitalRead(buttonPin) == LOW) {
    digitalWrite(relayPin, HIGH);  
    Serial.println("Open Door");
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Open Door");
    delay(5000);
    lcd.clear();
  } else if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("Card UID: ");
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; ++i) {
      uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println(uidString);
    sendToAPI(uidString);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(2000);
    lcd.clear();
  } else {
    char key = keypad.getKey();
    if (key == 'C') {
      otpMode = true;
      Serial.println("Otp Mode");
      inputCode = "";  
      lcd.clear();
      lcd.setCursor((16 - strlen("OTP Mode")) / 2, 0); 
      lcd.print("OTP Mode");
      delay(1000);
    }

    if (otpMode) {
      processOTP(key);
      return;
    } else {
      if (hallSensorValue == 0) {
        digitalWrite(relayPin, LOW);
        
        lcd.setCursor(1, 0);
        lcd.print("Put Your Card");
        timeClient.update();
        lcd.setCursor(1, 1);
        lcd.print("Time: ");
        lcd.print(timeClient.getFormattedTime());
        checkStoredValue();
      }
    }
  }
}



void sendToAPI(String uid) {                     ///////////////ส่วนส่ง rfid ไปเช็ค
  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = "rfid_tag=" + uid;
  int httpResponseCode = http.POST(postData);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);


    DynamicJsonDocument doc(1024);  
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return;
    }
    const char* user = doc["user"];
    const char* pin = doc["pin"];
    const char* status = doc["status"];
    const char* timestamp = doc["timestamp"];

    if (strcmp(status, "unlock") == 0) {
      digitalWrite(relayPin, HIGH);
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Unlock Door");
      lcd.setCursor(0, 1);
      lcd.print("Name: ");
      lcd.print(user);
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("              ");
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("ID: ");
      lcd.print(pin);
      
      
      lcd.setCursor(3, 0);
      lcd.print("Unlock Door");
      lcd.setCursor(0, 1);
      lcd.print("ID: ");
      lcd.print(pin);
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("              ");
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("Name: ");
      lcd.print(user);
      

      lcd.setCursor(3, 0);
      lcd.print("Unlock Door");
      lcd.setCursor(0, 1);
      lcd.print("ID: ");
      lcd.print(pin);
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("              ");
      lcd.setCursor(0, 1);
      delay(1000);
      lcd.print("Name: ");
      lcd.print(user);
      
      
    
      
    } else if (strcmp(status, "lock") == 0) {
      digitalWrite(relayPin, LOW);
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("Lock Door");
      for (int i = 5; i >= 0; i--) {
        lcd.setCursor(4, 1);
        lcd.print("Wait ");
        lcd.print(i);
        lcd.print(" s");
        delay(1000);
      }
    }
  } else {
    Serial.print("Error sending HTTP request, response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}


void sendToFastApi(const char* code) {                       /////////////ส่วนส่ง otp ไปเช็ค
  WiFiClientSecure client;
  HTTPClient http;
  String url = String(fastApiUrl) + "/check_code";
  Serial.println("Sending POST request to: " + url);
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = "code=" + String(code);
  int httpResponseCode = http.POST(postData);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);

    const size_t capacity = JSON_OBJECT_SIZE(2) + 20;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, response);

    const char* status = doc["status"];
    const char* message = doc["message"];
    const char* time = doc["time"];

    if (strcmp(status, "success") == 0) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Unlock Door");
      lcd.setCursor(4, 1);
      lcd.print(time);
      digitalWrite(relayPin, HIGH);

    } else if (strcmp(status, "failure") == 0) {
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("Lock Door");
      lcd.setCursor(4, 1);
      lcd.print(time);
      digitalWrite(relayPin, LOW);

    } else {
      Serial.println("Unknown status received");
    }

    delay(2000);
  } else {
    Serial.println("HTTP POST request failed");
    Serial.println("HTTP Error: " + http.errorToString(httpResponseCode));
  }
  http.end();
}



void checkStoredValue() {               ////////////////เช็คสถานประตู
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        int new_value = doc["stored_value"];
        Serial.print("Stored value: ");
        Serial.println(new_value);
        if (new_value != stored_value) {
          stored_value = new_value;
          if (stored_value == 1) {
            digitalWrite(relayPin, HIGH);
            delay(5000);
           } else if (stored_value == 0) {
            digitalWrite(relayPin, LOW);
          } else {
            Serial.println("Invalid value received");
          }
        }
      } else {
        Serial.println("Error parsing JSON");
      }
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
}

void processOTP(char key) {              /////////ส่วน otp
  if (key >= '0' && key <= '9') {
    Serial.print(key);
    inputCode += key;
    lcd.setCursor(4, 0);
    lcd.print("OTP Mode");
    lcd.setCursor(0, 1);
    lcd.print("Code: ");
    lcd.print(inputCode);
    if (inputCode.length() == 6) {
      Serial.println();
      sendToFastApi(inputCode.c_str());
      delay(2000);
      otpMode = false;
      inputCode = "";
    }
  } else if (key == 'D') {
    if (!inputCode.isEmpty()) {
      inputCode.remove(inputCode.length() - 1);
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("OTP Mode");
      lcd.setCursor(0, 1);
      lcd.print("Code: ");
      lcd.print(inputCode);
    }
  }
}
