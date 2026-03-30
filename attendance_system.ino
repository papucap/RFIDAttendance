#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

#define RST_PIN D3
#define SS_PIN  D4

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

int blockNum = 2;
byte bufferLen = 18;
byte readBlockData[18];

String card_holder_name;
const String sheet_url = "https://script.google.com/macros/s/AKfycbziv5dQMJ0B8i6JdanoUqH0mJobfL75qefWa-YdqaP-bsuoFm2lxilYg7asaXtg3cCiCA/exec?name="; 

#define WIFI_SSID "TVOJE_NAZEV_WIFI" 
#define WIFI_PASSWORD "TVOJE_HESLO" 

void ReadDataFromBlock(int blockNum, byte readBlockData[]);

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing system...");

  // WiFi Connectivity
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize SPI bus and MFRC522
  SPI.begin();
  Serial.println("Place your card to the reader...");
}

void loop() {
  mfrc522.PCD_Init();

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("\nReading data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);

  Serial.print(F("Card Holder Name: "));
  String name = "";
  for (int j = 0; j < 16; j++) {
    if (readBlockData[j] != 0) {
      name += (char)readBlockData[j];
    }
  }
  Serial.println(name);

  // HTTPS Communication
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure(); // Ignore SSL certificate

    card_holder_name = sheet_url + name;
    card_holder_name.trim();

    HTTPClient https;
    Serial.print(F("[HTTPS] Sending data to Google Sheets...\n"));

    if (https.begin(*client, card_holder_name)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        Serial.printf("[HTTPS] Response code: %d\n", httpCode);
        Serial.println("Data Recorded Successfully!");
      } else {
        Serial.printf("[HTTPS] GET failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.println("[HTTPS] Unable to connect");
    }
  }
  
  delay(2000); 
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println("Block read success.");
}