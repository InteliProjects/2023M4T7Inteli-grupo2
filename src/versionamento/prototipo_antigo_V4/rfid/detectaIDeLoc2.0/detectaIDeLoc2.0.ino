// Inclusion of necessary libraries for communication with the RFID, Wi-Fi, MQTT, and Ubidots modules
#include <WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include "UbidotsEsp32Mqtt.h"
#include <PubSubClient.h>
#include <map>

// Definition of constants for connection to Ubidots and Wi-Fi network
const char *UBIDOTS_TOKEN = "BBUS-Wr0Chf75jREI6GgNS2a6PZvQqvssaE";
const char *WIFI_SSID = "M54 de Conrado";
const char *WIFI_PASS = "sen@rot1";
const char *DEVICE_LABEL = "sala";
const char *VARIABLE_LABEL = "consultorio";

// Variables for time control
unsigned long timer;
const int PUBLISH_FREQUENCY = 5000; // Time interval for data publication

// Define the time duration in milliseconds that the LED will be on
const int LED_ON_TIME = 1000; // 1000 ms = 1 second

// Variable to control the LED timer
unsigned long ledTimer;

// Ubidots client initialization
Ubidots ubidots(UBIDOTS_TOKEN);

// Definition of pins used for the RFID module
#define SS_PIN    21
#define RST_PIN   22

// Definition of the button pin
#define BUTTON_PIN 33

// Definition of LED pins
#define LED_VERDE 27
#define LED_VERMELHO 26

String nomePessoa = "";
String localizacao = "";
String esp32Id = WiFi.macAddress();

// Add MAC addresses associated with each location
String recepcaoMAC = "A8:42:E3:56:A4:20";
String sala1MAC = "B4:8A:0A:AD:A8:64";
String sala2MAC = "48:E7:29:8B:DB:C8";
// Object to store the authentication key of the RFID card
MFRC522::MIFARE_Key key;

// Object to interact with the RFID module MFRC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Function called when a message is received by the MQTT client
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// Global variables
std::map<String, std::map<String, int>> contagemPorLocalizacao;
std::map<String, std::map<String, unsigned long>> ultimoTempoPorLocalizacao;

// Function to check if a new ID can be counted
bool podeContabilizar(String localizacao, unsigned long tempoAtual) {
    if (ultimoTempoPorLocalizacao.find(localizacao) != ultimoTempoPorLocalizacao.end()) {
        if (ultimoTempoPorLocalizacao[localizacao].find(localizacao) != ultimoTempoPorLocalizacao[localizacao].end()) {
            unsigned long ultimoTempo = ultimoTempoPorLocalizacao[localizacao][localizacao];
            return (tempoAtual - ultimoTempo) > 40000;  // Check if the interval is greater than 40 seconds
        }
    }
    return true;
}

// Function to increment the count of IDs for a specific location
void incrementarContagem(String localizacao, unsigned long tempoAtual) {
    if (podeContabilizar(localizacao, tempoAtual)) {
        // Update the last recorded time for the location
        if (ultimoTempoPorLocalizacao.find(localizacao) != ultimoTempoPorLocalizacao.end()) {
            ultimoTempoPorLocalizacao[localizacao][localizacao] = tempoAtual;
        } else {
            std::map<String, unsigned long> novoMap;
            novoMap[localizacao] = tempoAtual;
            ultimoTempoPorLocalizacao[localizacao] = novoMap;
        }
        // Increment the count for the specific location

        if (contagemPorLocalizacao.find(localizacao) != contagemPorLocalizacao.end()) {
            if (contagemPorLocalizacao[localizacao].find(localizacao) != contagemPorLocalizacao[localizacao].end()) {
                contagemPorLocalizacao[localizacao][localizacao]++;
            } else {
                contagemPorLocalizacao[localizacao][localizacao] = 1;
            }
        } else {
            contagemPorLocalizacao[localizacao][localizacao] = 1;
        }
    } else {
        Serial.println("ID já contabilizado recentemente. Ignorando...");
    }
}

// Function to print the current count by location
void imprimirContagem() {
    Serial.println("Contagem por Localizacao:");
    for (auto const& pair : contagemPorLocalizacao) {
        Serial.print(pair.first);
        Serial.print(": ");
        for (auto const& innerPair : pair.second) {
            Serial.print(innerPair.first);
            Serial.print(" - ");
            Serial.print(innerPair.second);
            Serial.print(", ");
        }
        Serial.println();
    }
}
// Initial program setup
void setup() {
    pinMode(LED_VERMELHO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configure the button pin as input with pull-up
    Serial.begin(115200); // Start serial communication
    SPI.begin(); // Start SPI communication for the RFID module
    mfrc522.PCD_Init(); // Initialize the RFID module
    ubidots.setDebug(true);
    ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
    ubidots.setCallback(callback);
    ubidots.setup();
    ubidots.reconnect();
    timer = millis();// Initialize the timer
    Serial.println("Leitura: Aproxime o seu cartão do leitor...");
}
// Main program loop
void loop() {
    atualizarLed();// Place this at the beginning of the loop to ensure it is always executed
    // Check if a new RFID card is present and can be read
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(LED_VERMELHO, HIGH);
        return;
    } else {
        digitalWrite(LED_VERDE, LOW);// Turn off the Green LED
        acenderLed();// Turn on the Green LED and start the timer
    }
    
    leituraDados();// Read data from the RFID card
    mfrc522.PICC_HaltA(); // Put the card in "halt" state
    mfrc522.PCD_StopCrypto1();// Deactivate encryption for card communication
    // Reconnect to Ubidots if necessary and publish the read data
    if (!ubidots.connected()) {
        ubidots.reconnect();
    }
    
    if ((millis() - timer) > PUBLISH_FREQUENCY) {
        // Check if the string contains a valid name and publish to Ubidots
        if (nomePessoa.length() > 0) {

            // Check the MAC address to determine the location
            if (esp32Id.equals(recepcaoMAC)) {
                localizacao = "Recepcao";
            } else if (esp32Id.equals(sala1MAC)) {
                localizacao = "Sala1";
            } else if (esp32Id.equals(sala2MAC)) {
                localizacao = "Sala2";
            } else {
                localizacao = "Desconhecida";
            }

            incrementarContagem(localizacao, millis());


            char context[100];
            snprintf(context, sizeof(context), "\"nome\":\"%s\",\"localizacao\":\"%s\"", nomePessoa.c_str(), localizacao);
            Serial.println(context);
            ubidots.add(VARIABLE_LABEL, 1, context);
            ubidots.publish(DEVICE_LABEL);
        } else {
           Serial.println("Nome Pessoa está vazio, nada será publicado.");
        }

        // Print the current count
        imprimirContagem();

        timer = millis();
}

    ubidots.loop(); // Keep the MQTT connection active
}
// Function to turn on the LED and start the timer
void acenderLed() {
    digitalWrite(LED_VERDE, HIGH);// Turn on the Green LED
    ledTimer = millis();// Start the timer
}
// Function to check and update the LED state
void atualizarLed() {
    if (millis() - ledTimer < LED_ON_TIME) {
        digitalWrite(LED_VERDE, HIGH);// Turn on the Green LED
        digitalWrite(LED_VERMELHO, LOW);// Turn off the Red LED
    } else {
        digitalWrite(LED_VERDE, LOW);// Turn off the Green LED after the specified time
        digitalWrite(LED_VERMELHO, HIGH); // Reactivate the Red LED
    }
}

void leituraDados() {
    // Print card details to the serial console
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    // Set the default authentication key for all blocks (FF FF FF FF FF FF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
 // Buffer to store the read data from the card
 // Make sure to clear the buffer to avoid garbage from previous reads
  byte buffer[18];
  memset(buffer, 0, sizeof(buffer));// Initialize the buffer with zeros
  byte bloco = 1; // Set the block to be read
  byte tamanho = sizeof(buffer);// Buffer size
    // Authenticate the block for operation
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid));
    // Check if authentication was successful
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
    // Read data from the authenticated block
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
    // Check if reading was successful
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    nomePessoa = "";  // Clear the current string
    for (int i = 0; i < 16; i++) {// Assume the name is in the first 16 bytes of the buffer
            // Add only printable ASCII characters
      if (buffer[i] >= 32 && buffer[i] <= 126) {
        nomePessoa += (char)buffer[i];
      } else if (buffer[i] == '\0') {
        break;// If a null byte is encountered, stop adding
      }
    }
    nomePessoa.trim();// Remove whitespace from the beginning and end
        // Print the person's name or indicate if it's empty
    if (nomePessoa.length() > 0) {
      Serial.print(F("Nome no Cartão: "));
      Serial.println(nomePessoa);
    } 
      // Print the location or indicate if it's empty
    if (localizacao.length() > 0) {
      Serial.print(F("Localizacao atual: "));
      Serial.println(localizacao);
    } else {
      Serial.println(F("Cartão vazio"));
    }
  }
    // Check if the button was pressed
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Call the program reset function
    resetProgram();
  }
  // Print the current count
  imprimirContagem();

  if (digitalRead(BUTTON_PIN) == LOW) {
    resetProgram();
  }
    // Finish reading the RFID card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void resetProgram() {
    // Reinitialize variables, states, and possibly call setup()
}
