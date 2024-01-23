#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <Wire.h>  // comunicação I2C
#include <LiquidCrystal_I2C.h>  // Biblioteca do display LCD com I2C

//RFID
#define SS_PIN    21
#define RST_PIN   22
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16
//Definições para o display LCD (adapte conforme necessário)
#define I2C_ADDR    0x27  // Endereço do dispositivo I2C
#define LCD_COLS    16    // Número de colunas do LCD
#define LCD_ROWS    2     // Número de linhas do LCD
#define LCD_SDA     4
#define LCD_SCL     5

// Definição do pino do botão
#define BUTTON_PIN 33
// Definição do pino do buzzer
#define BUZZER_PIN 17
//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;
// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

//menu para escolha da operação
int menu()
{
  Serial.println(F("\nEscolha uma opção:"));
  //Serial.println(F("0 - Leitura de Dados"));
  Serial.println(F("1 - Gravação de Dados\n"));
  Serial.println(F("2 - Limpar Dados da Tag\n"));
  //fica aguardando enquanto o usuário nao enviar algum dado
  while(!Serial.available()){};
  //recupera a opção escolhida
  int op = (int)Serial.read();
  //remove os proximos dados (como o 'enter ou \n' por exemplo) que vão por acidente
  while(Serial.available()) {
    if(Serial.read() == '\n') break;
    Serial.read();
  }
  return (op-48);//do valor lido, subtraimos o 48 que é o ZERO da tabela ascii
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como entrada com pull-up
  Wire.begin(LCD_SDA, LCD_SCL); // Inicializa a comunicação I2C com os pinos SDA e SCL
  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  // Inicia MFRC522
  mfrc522.PCD_Init();
  // Inicializa o LCD
  lcd.init();  // Inicializa o LCD
  lcd.backlight();  // Ativa a luz de fundo
  lcd.setCursor(0, 0);  // Define o cursor no início do display
  lcd.print("Aproxime a tag");  // Exibe a mensagem inicial
  // Mensagens iniciais no serial monitor
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
}

void resetProgram() {
    // Reinicialize variáveis, estados, e possivelmente chame setup()
}

void loop() {
  // Aguarda a aproximacao do cartao e verifica se há cartão
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    // Se nenhum cartão estiver presente, exibe a mensagem no LCD
    exibirMensagemAproxime();
    return;
  }
  // Verifica se o botão foi pressionado
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Chama a função de reset do programa
        resetProgram();
    }
  // Faz a leitura dos dados
  leituraDados();
}
void exibirMensagemAproxime() {
  lcd.clear();  // Limpa o display LCD
  lcd.setCursor(0, 0);  // Define o cursor no início do display
  lcd.print("Aproxime a tag");  // Exibe a mensagem inicial
}
//faz a leitura dos dados do cartão/tag
void leituraDados() {
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  byte buffer[SIZE_BUFFER] = {0};
  byte bloco = 1;
  byte tamanho = SIZE_BUFFER;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.print(F("\nDados bloco ["));
  Serial.print(bloco);
  Serial.print(F("]: "));
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    Serial.write(buffer[i]);
  }
  Serial.println(" ");

  // Lendo o bloco 2 para obter o cargo
  byte blocoCargo = 2; // Bloco onde o cargo está armazenado
  byte bufferCargo[MAX_SIZE_BLOCK] = {0}; // Buffer para armazenar os dados do cargo
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blocoCargo, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed for cargo: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(blocoCargo, bufferCargo, &tamanho);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading cargo failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Exibindo o cargo no monitor serial
  Serial.print(F("Cargo: "));
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    Serial.write(bufferCargo[i]);
  }
  Serial.println();
  // Verifica se os dados lidos são vazios
  bool dadosVazios = true;
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (buffer[i] != 0) {
      dadosVazios = false;
      break;
    }
  }
    // No final da função leituraDados(), após ler os dados do nome e do cargo:
    exibirDados(buffer, bufferCargo);
  // Mostra as opções dependendo se os dados estão vazios ou não
  if (dadosVazios) {
    Serial.println(F("Dados vazios na tag."));
    Serial.println(F("Opções disponíveis:"));
    Serial.println(F("1 - Gravar Dados"));
    Serial.println(F("2 - Limpar Dados da Tag"));
  } else {
    Serial.println(F("Opções disponíveis:"));
    Serial.println(F("1 - Gravar Dados"));
    Serial.println(F("2 - Limpar Dados da Tag"));
  }
  int opcao = menu();
  if (dadosVazios) {
    if (opcao == 1) {
      gravarDados();
    } else if (opcao == 2) {
      limparDados();
    } else {
      Serial.println(F("Opção inválida."));
    }
  } else {
    if (opcao == 0) {
      leituraDados();
    } else if (opcao == 1) {
      gravarDados();
    } else if (opcao == 2) {
      limparDados();
    } else {
      Serial.println(F("Opção inválida."));
    }
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
void exibirDados(byte *bufferNome, byte *bufferCargo) {
  lcd.clear();  // Limpa o display LCD
  lcd.setCursor(0, 0);  // Define o cursor na primeira linha do display

  // Exibe o nome na primeira linha
  lcd.print("Nome:");
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (bufferNome[i] != 0) {
      lcd.write(bufferNome[i]);
    }
  }

  // Exibe o cargo na segunda linha
  lcd.setCursor(0, 1);  // Move o cursor para a segunda linha
  lcd.print("Cargo:");
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (bufferCargo[i] != 0) {
      lcd.write(bufferCargo[i]);
    }
  }
}

//faz a gravação dos dados no cartão/tag
void gravarDados() {
  // Imprime os detalhes técnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

  // Aguarda 30 segundos para entrada de dados via Serial
  Serial.setTimeout(30000L);
  Serial.println(F("Insira os dados a serem gravados com o caractere '#' ao final\n[máximo de 16 caracteres]:"));

  // Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Buffer para armazenamento dos dados que iremos gravar
  byte buffer[MAX_SIZE_BLOCK] = "";
  byte bloco = 1; // Bloco definido para operação
  byte tamanhoDados; // Tamanho dos dados que vamos operar (em bytes)

  // Recupera no buffer os dados que o usuário inserir pela serial
  tamanhoDados = Serial.readBytesUntil('#', (char*)buffer, MAX_SIZE_BLOCK);
  // Espaços que sobrarem do buffer são preenchidos com espaço em branco
  for (byte i = tamanhoDados; i < MAX_SIZE_BLOCK; i++) {
    buffer[i] = ' ';
  }

  // Gravação dos dados no cartão/tag
  // Autenticação para habilitar uma comunicação segura
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Grava no bloco
  status = mfrc522.MIFARE_Write(bloco, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println(F("Dados gravados com sucesso."));
    tocarSomAdicionar(); // Toca o buzzer após a gravação do nome2
  }

  // Aguarda 1 segundo antes de continuar
  delay(1000);

  // Solicitação de dados adicionais (Cargo)
  Serial.println(F("Insira o cargo com o caractere '#' ao final\n[máximo de 16 caracteres]:"));
  byte bufferCargo[MAX_SIZE_BLOCK] = "";
  byte blocoCargo = 2; // Defina aqui o bloco para gravar o cargo
  byte tamanhoCargo = Serial.readBytesUntil('#', (char*)bufferCargo, MAX_SIZE_BLOCK);
  for (byte i = tamanhoCargo; i < MAX_SIZE_BLOCK; i++) {
    bufferCargo[i] = ' '; // Preenche o restante do buffer com espaços
  }

  // Gravação do cargo no cartão/tag
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blocoCargo, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Autenticação para cargo falhou: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Write(blocoCargo, bufferCargo, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Gravação do cargo falhou: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println(F("Cargo gravado com sucesso."));
    tocarSomAdicionar(); // Toca o buzzer após a gravação do cargo
  }

  // Finaliza a comunicação com o cartão/tag
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void limparDados() {
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Defina os blocos para limpeza
  byte blocosParaLimpar[] = {1, 2}; // Inclua aqui os blocos que você deseja limpar
  for (int j = 0; j < sizeof(blocosParaLimpar); j++) {
    byte buffer[16] = {0}; // Preenche o buffer com zeros para limpar os dados
    byte bloco = blocosParaLimpar[j];
  
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    status = mfrc522.MIFARE_Write(bloco, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    } else {
      Serial.print(F("Dados do bloco "));
      Serial.print(bloco);
      Serial.println(F(" foram apagados com sucesso!"));
    }
  }
  tocarSomApagar();
}

void tocarSomAdicionar() {
  for (int freq = 800; freq <= 1200; freq += 100) {
    tone(BUZZER_PIN, freq, 50);
    delay(60); // Pequeno atraso entre as mudanças de frequência
  }
}
void tocarSomApagar() {
  // Toca um som mais longo para indicar a remoção
  tone(BUZZER_PIN, 500, 300); // 500 Hz por 300 ms
}
