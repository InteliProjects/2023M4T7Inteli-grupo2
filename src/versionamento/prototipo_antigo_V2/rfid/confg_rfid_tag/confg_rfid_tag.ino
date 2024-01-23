#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI

#include <Wire.h>  // comunicação I2C
#include <LiquidCrystal_I2C.h>  // Biblioteca do display LCD com I2C

//RFID
#define SS_PIN    21
#define RST_PIN   22
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16
// Definições para o display LCD (adapte conforme necessário)
#define I2C_ADDR    0x27  // Endereço do dispositivo I2C
#define LCD_COLS    16    // Número de colunas do LCD
#define LCD_ROWS    2     // Número de linhas do LCD
#define LCD_SDA     4
#define LCD_SCL     5


//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

void setup() {
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

void loop() {
  // Aguarda a aproximacao do cartao e verifica se há cartão
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    // Se nenhum cartão estiver presente, exibe a mensagem no LCD
    exibirMensagemAproxime();
    return;
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

  // Verifica se os dados lidos são vazios
  bool dadosVazios = true;
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (buffer[i] != 0) {
      dadosVazios = false;
      break;
    }
  }

  // Depois de ler os dados da tag, chame a função para exibir no LCD
  exibirDados(buffer);

  // Mostra as opções dependendo se os dados estão vazios ou não
  if (dadosVazios) {
    Serial.println(F("Dados vazios na tag."));
    Serial.println(F("Opções disponíveis:"));
    Serial.println(F("1 - Gravar Dados"));
    Serial.println(F("2 - Limpar Dados da Tag"));
  } else {
    Serial.println(F("Opções disponíveis:"));
    Serial.println(F("0 - Leitura de Dados"));
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

void exibirDados(byte *buffer) {
  lcd.clear();  // Limpa o display LCD
  lcd.setCursor(0, 0);  // Define o cursor no início do display

  bool dadosVazios = true;
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (buffer[i] != 0) {
      dadosVazios = false;
      break;
    }
  }

  if (dadosVazios) {
    lcd.print("Sem dados");  // Exibe "Sem dados" se não houver dados na tag
  } else {
    lcd.print("Tag:");  // Se houver dados, exibe o conteúdo
    lcd.setCursor(0, 1);  // Vai para a segunda linha do display
    for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
      lcd.write(buffer[i]);
    }
  }
}

//faz a gravação dos dados no cartão/tag
void gravarDados()
{
  //imprime os detalhes tecnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); 
  // aguarda 30 segundos para entrada de dados via Serial
  Serial.setTimeout(30000L) ;     
  Serial.println(F("Insira os dados a serem gravados com o caractere '#' ao final\n[máximo de 16 caracteres]:"));

  //Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //buffer para armazenamento dos dados que iremos gravar
  byte buffer[MAX_SIZE_BLOCK] = "";
  byte bloco; //bloco que desejamos realizar a operação
  byte tamanhoDados; //tamanho dos dados que vamos operar (em bytes)

  //recupera no buffer os dados que o usuário inserir pela serial
  //serão todos os dados anteriores ao caractere '#'
  tamanhoDados = Serial.readBytesUntil('#', (char*)buffer, MAX_SIZE_BLOCK);
  //espaços que sobrarem do buffer são preenchidos com espaço em branco
  for(byte i=tamanhoDados; i < MAX_SIZE_BLOCK; i++)
  {
    buffer[i] = ' ';
  }
 
  bloco = 1; //bloco definido para operação
  String str = (char*)buffer; //transforma os dados em string para imprimir
  Serial.println(str);

  //Authenticate é um comando para autenticação para habilitar uma comuinicação segura
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    bloco, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    delay(1000);
    return;
  }
  //else Serial.println(F("PCD_Authenticate() success: "));
 
  //Grava no bloco
  status = mfrc522.MIFARE_Write(bloco, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    delay(1000);
    return;
  }
  else{
    Serial.println(F("MIFARE_Write() success: "));
    delay(1000);
  }
 
}

void limparDados() {
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  byte buffer[16] = {0}; // Preenche o buffer com zeros para limpar os dados

  byte bloco = 1;
  String str = (char *)buffer;

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
    Serial.println(F("Dados da tag foram apagados com sucesso!"));
  }
}

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



