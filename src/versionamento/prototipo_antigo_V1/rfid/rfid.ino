#include <EEPROM.h> 
#include <SPI.h>     
#include <MFRC522.h>    

//variaveis globais
uint8_t successRead;    

byte readCard[4];   

constexpr uint8_t RST_PIN = 2;    
constexpr uint8_t SS_PIN = 5;   

MFRC522 mfrc522(SS_PIN, RST_PIN);

//Struct que armazena os dados dos pacientes e medicos
struct Cliente {
    byte id[4];
    char nome[50];
    char cargo[50];
};
//array da struct
struct Cliente clientes[100];

//inicia as bibliotecas que serão usadas
void setup() {
  EEPROM.begin(1024);
  

  Serial.begin(9600); 
  SPI.begin();          
  mfrc522.PCD_Init();   

  //Aumenta alcance max da antena
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Controle de Acesso v0.1"));  
  ShowReaderDetails(); 

  //armazena dados da pessoa em uma struct
  strcpy(clientes[1].nome, "Diogo Pelaes Burgierman");
  clientes[1].id[0]= 195;
  clientes[1].id[1]= 121;
  clientes[1].id[2]= 231;
  clientes[1].id[3]= 20;
  strcpy(clientes[1].cargo, "Paciente");

  //armazena dados da pessoa em uma struct
  strcpy(clientes[85].nome, "Drielly Farias");
  clientes[85].id[0]= 26;
  clientes[85].id[1]= 253;
  clientes[85].id[2]= 80;
  clientes[85].id[3]= 191;
  strcpy(clientes[85].cargo, "Fisioterapeuta");

}

//continua procurando enquanto não encontrar uma leitura de id
void loop() {
  do {
    successRead = getID();  
  }
  while (!successRead); 
}

//faz a leitura do id e procura o id nas structs existentes, se encontrado printa o id e as informações da pessoa. Se não encontrado printa "Cadastro não encontrado"
uint8_t getID() {

  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {  
    return 0;
  }

  //printa o id do chio lido
  Serial.println(F("UID do chip lido:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.println(readCard[i]);
  }

  bool encontrado = false;

//compara o id lido com os ids registrados nas structs para encontrar a struct correspondente
for (int i = 0; i < 100; i++) {
  for(int j = 0;j<4;j++){
    if(clientes[i].id[j]!= readCard[j]){
      encontrado = false;
      break;
    }
    encontrado = true;
  }
  //se encontrar printa "encontrado" e as informações da pessoa
  if (encontrado == true) {
    Serial.println("Encontrado");
    Serial.print("Nome:");
    Serial.println(clientes[i].nome);
    Serial.print("Cargo:");
    Serial.println(clientes[i].cargo);
    break;
  }
 }
 //se não encontrar printa "Cadastro não encontrado"
  if (!encontrado) {
    Serial.print("Cadastro não encontrado");
  }

  Serial.println("");
  mfrc522.PICC_HaltA();
  return 1;
}

//printa as informações da versão do software MFRC522
void ShowReaderDetails() {

  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("Versao do software MFRC522: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (desconhecido)"));
  Serial.println("");

  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("ALERTA: Falha na comunicacao, o modulo MFRC522 esta conectado corretamente?"));
    Serial.println(F("SISTEMA ABORTADO: Verifique as conexoes."));
    while (true); 
  }
}