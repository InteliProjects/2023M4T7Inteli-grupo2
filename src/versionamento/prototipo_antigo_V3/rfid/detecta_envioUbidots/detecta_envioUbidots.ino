// Inclusão de bibliotecas necessárias para comunicação com o módulo RFID, Wi-Fi, MQTT e Ubidots
#include <MFRC522.h>
#include <SPI.h>
#include "UbidotsEsp32Mqtt.h"
#include <PubSubClient.h>

// Definição de constantes para a conexão com o Ubidots e a rede Wi-Fi
const char *UBIDOTS_TOKEN = "BBUS-Wr0Chf75jREI6GgNS2a6PZvQqvssaE";
const char *WIFI_SSID = "Inteli-welcome";
const char *WIFI_PASS = "";
const char *DEVICE_LABEL = "sala";
const char *VARIABLE_LABEL = "consultorio";

// Variáveis para controle de tempo
unsigned long timer;
const int PUBLISH_FREQUENCY = 5000; // Intervalo de tempo para publicação dos dados

// Defina o tempo de duração em milissegundos que o LED ficará aceso
const int LED_ON_TIME = 1000; // 1000 ms = 1 segundos

// Variável para controlar o temporizador do LED
unsigned long ledTimer;

// Inicialização do cliente Ubidots
Ubidots ubidots(UBIDOTS_TOKEN);

// Definição dos pinos utilizados para o módulo RFID
#define SS_PIN    21
#define RST_PIN   22

// Definição do pino do botão
#define BUTTON_PIN 33

// Definição dos pinos dos LEDs
#define LED_VERDE 27
#define LED_VERMELHO 26

// Variável para armazenar o nome lido do cartão RFID
String nomePessoa = "";

// Objeto para armazenar a chave de autenticação do cartão RFID
MFRC522::MIFARE_Key key;

// Objeto para interagir com o módulo RFID MFRC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Função chamada quando uma mensagem é recebida pelo cliente MQTT
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// Configuração inicial do programa
void setup() {
    pinMode(LED_VERMELHO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como entrada com pull-up
    Serial.begin(115200); // Inicia comunicação serial
    SPI.begin();          // Inicia comunicação SPI para o módulo RFID
    mfrc522.PCD_Init();   // Inicializa o módulo RFID
    ubidots.setDebug(true);
    ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
    ubidots.setCallback(callback);
    ubidots.setup();
    ubidots.reconnect();
    timer = millis();     // Inicializa o timer
    Serial.println("Leitura: Aproxime o seu cartao do leitor...");
}

// Loop principal do programa
void loop() {

     atualizarLed(); // Coloque isso no início do loop para garantir que seja sempre executado

    // Verifica se um novo cartão RFID está presente e pode ser lido
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(LED_VERMELHO, HIGH);
        return;
    } else {
        digitalWrite(LED_VERDE, LOW);  // Apaga o LED Verde
        acenderLed(); // Acende o LED verde e inicia o temporizador
    }
    
    leituraDados(); // Lê os dados do cartão RFID
    mfrc522.PICC_HaltA(); // Coloca o cartão em estado de "parada"
    mfrc522.PCD_StopCrypto1(); // Desativa criptografia para comunicação com o cartão
    // Reconecta ao Ubidots se necessário e publica os dados lidos
    if (!ubidots.connected()) {
        ubidots.reconnect();
    }
    if ((millis() - timer) > PUBLISH_FREQUENCY) {
        // Verifica se a string contém um nome válido e publica no Ubidots
        if (nomePessoa.length() > 0) {
            char context[100];
            snprintf(context, sizeof(context), "\"nome\":\"%s\"", nomePessoa.c_str());
            Serial.println(context);
            ubidots.add(VARIABLE_LABEL, 1, context);
            ubidots.publish(DEVICE_LABEL);
        } else {
            Serial.println("Nome Pessoa está vazio, nada será publicado.");
        }
        timer = millis();
    }
    ubidots.loop(); // Mantém a conexão MQTT ativa
}

// Função para acender o LED e iniciar o temporizador
void acenderLed() {
    digitalWrite(LED_VERDE, HIGH); // Acende o LED verde
    ledTimer = millis(); // Inicia o temporizador
}

// Função para verificar e atualizar o estado do LED
void atualizarLed() {
    if (millis() - ledTimer < LED_ON_TIME) {
        digitalWrite(LED_VERDE, HIGH); // Acende o LED verde
        digitalWrite(LED_VERMELHO, LOW); // Desliga o LED vermelho
    } else {
        digitalWrite(LED_VERDE, LOW); // Apaga o LED verde após o tempo especificado
        digitalWrite(LED_VERMELHO, HIGH); // Reativa o LED vermelho
    }
}

void leituraDados() {
    // Imprime detalhes do cartão no console serial
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    // Configura a chave de autenticação padrão para todos os blocos (FF FF FF FF FF FF)
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    // Buffer para armazenar os dados lidos do cartão
    // Certifique-se de limpar o buffer para evitar lixo de leituras anteriores
    byte buffer[18];
    memset(buffer, 0, sizeof(buffer)); // Inicializa o buffer com zeros
    byte bloco = 1; // Define o bloco a ser lido
    byte tamanho = sizeof(buffer); // Tamanho do buffer
    // Autentica o bloco para operação
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid));
    // Verifica se a autenticação foi bem-sucedida
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    // Lê os dados do bloco autenticado
    status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
    // Verifica se a leitura foi bem-sucedida
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    } else {
        nomePessoa = ""; // Limpa a string atual
        for (int i = 0; i < 16; i++) { // Assume que o nome esteja nos primeiros 16 bytes do buffer
            // Adiciona apenas caracteres ASCII imprimíveis
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                nomePessoa += (char)buffer[i];
            } else if (buffer[i] == '\0') {
                break; // Se encontrar um byte nulo, pare de adicionar
            }
        }
        nomePessoa.trim(); // Remove espaços em branco do início e do fim
        // Imprime o nome da pessoa ou indica se está vazio
        if (nomePessoa.length() > 0) {
            Serial.print(F("Nome no Cartão: "));
            Serial.println(nomePessoa);
        } else {
            Serial.println(F("Cartão vazio"));
        }
    }
    // Verifica se o botão foi pressionado
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Chama a função de reset do programa
        resetProgram();
    }
    // Termina a leitura do cartão RFID
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
void resetProgram() {
    // Reinicialize variáveis, estados, e possivelmente chame setup()
}