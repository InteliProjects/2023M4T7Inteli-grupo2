#include <WiFi.h>
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>

#define LED_BUILTIN 2
const char *ssid = "JINWAR";
const char *password = "jinwar11"; //precisa ter 8 caracteres
AsyncWebServer server(80);

const uint8_t relay = 13;
const uint8_t wipeB = 33;

const uint8_t RST_PIN = 2;
const uint8_t SS_PIN = 5;

uint8_t successRead; 


MFRC522 mfrc522(SS_PIN, RST_PIN);

struct Cliente {
  byte id[4];
  char nome[50];
  char cargo[50];
};

struct Cliente clientes[100];

void setup() {
  EEPROM.begin(1024);

  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  //Aumenta alcance max da antena
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Controle de Acesso v0.1"));
  ShowReaderDetails();

  strcpy(clientes[1].nome, "Diogo Pelaes Burgierman");
  clientes[1].id[0] = 195;
  clientes[1].id[1] = 121;
  clientes[1].id[2] = 231;
  clientes[1].id[3] = 20;
  strcpy(clientes[1].cargo, "Paciente");

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(F("Endereço IP do Ponto de Acesso: "));
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);

  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Ligar o LED (ou executar a ação desejada)
    // Substitua esta linha de acordo com sua configuração
    digitalWrite(LED_BUILTIN, HIGH);
    request->send(200, "text/plain", "LED ligado");
  });

  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Desligar o LED (ou executar a ação desejada)
    digitalWrite(LED_BUILTIN, LOW);
    request->send(200, "text/plain", "LED desligado");
  });

  server.begin();
}

void loop() {
  do {
    successRead = getID();
  }
  while (!successRead);
}

uint8_t getID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }

  Serial.println(F("UID do chip lido:"));
  for (uint8_t i = 0; i < 4; i++) {
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.println(readCard[i]);
  }

  bool encontrado = false;

  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 4; j++) {
      if (clientes[i].id[j] != readCard[j]) {
        encontrado = false;
        break;
      }
      encontrado = true;
    }
    if (encontrado == true) {
      Serial.println("Encontrado");
      Serial.print("Nome:");
      Serial.println(clientes[i].nome);
      Serial.print("Cargo:");
      Serial.println(clientes[i].cargo);
      break;
    }
  }
  if (!encontrado) {
    Serial.print("Cadastro não encontrado");
  }

  Serial.println("");
  mfrc522.PICC_HaltA();
  return 1;
}

void ShowReaderDetails() {
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("Versao do software MFRC522: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (desconhecido"));
  Serial.println("");

  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("ALERTA: Falha na comunicacao, o modulo MFRC522 esta conectado corretamente?"));
    Serial.println(F("SISTEMA ABORTADO: Verifique as conexoes."));
    while (true);
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<html><body>";
  html += "<h1>ESP32 Ponto de Acesso</h1>";
  html += "<p><a href='/led/on'>Ligar LED</a></p>";
  html += "<p><a href='/led/off'>Desligar LED</a></p>";
  html += "</body></html>";
  request->send(200, "text/html", html);
}

