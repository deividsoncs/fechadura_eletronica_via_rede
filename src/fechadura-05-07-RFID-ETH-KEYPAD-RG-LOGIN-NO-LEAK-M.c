/**
    Protótipo de Fechadura Eletrônica
    Deividson Calixto da Silva
*/
#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Key.h>
#include <Keypad.h>
#include <avr/wdt.h>
#include <sha1.h>
#include <mysql.h>
#include <stdlib.h>


/* Configurações de Ethernet */
byte macAddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress servidorSQL(10, 0, 10, 203);
IPAddress meuIP(10, 0, 10, 32);
IPAddress meuDNS(10, 0, 10, 10);
IPAddress gw(10, 0, 10, 1);
IPAddress mask(255, 255, 255, 0);

//conector do banco mysql
Connector my_conn;

//usuário Banco
char user[] = "arduino2";
char password[] = "123456";
const char PORTA_ATUACAO[] = "PORTA X";
char SELECT_RFID[100] = "SELECT id, SUBSTRING(nome,1,16), senha FROM arduino_db.usuarios WHERE(chave like '%s');";
char SELECT_RG[100] = "SELECT id, SUBSTRING(nome,1,16), senha, rg FROM arduino_db.usuarios WHERE(rg like '%s');";
//INSERT de acionamento da porta
char INSERT_HISTORICO_RFID[180] = "INSERT INTO arduino_db.historico (porta, data_hora, id_usuario, tipo_acesso) VALUES ('%s', now(), %d, 'RFID');";
char INSERT_HISTORICO_RG[180] = "INSERT INTO arduino_db.historico (porta, data_hora, id_usuario, tipo_acesso) VALUES ('%s', now(), %d, 'RG');";
char QUERY_AUX[180] = "";

//usuario resgatando do banco via SELECT
long idUsuario = 0;
//char* chave_acesso;
char* nomeUsuario;
char* senhaBco;
String strChave = "";

unsigned long millisAnteriorRFID = 0;
unsigned long millisAtualRFID = 0;
unsigned long millisAnteriorConex = 0;
unsigned long millisAtualConex = 0;
unsigned long millisAnteriorlSenha = 0;

//Tempo de execução de leitura do shield RFID
const int TEMPO_LED_RFID = 500;
const long TEMPO_VERIFICA_CONEX = 60000; //um em um minuto checar
const int TEMPO_LEITURA_SENHA = 10000;

//Para efeito de simulação Entrada RFID sera o ACIONAMENTO DO PINO 4
//const int botao_RFID = 23;

//Led de leitura efetuada do cartão RFID
const int LED_LEITURA_RFID = 24;
//Led de Liberação de acesso;
const int LED_LIB_ACESSO_RFID = 26;
//Led indicado de Acesso Inválido
const int LED_ACESSO_INVALIDO = 22;
//rele de acionamento da porta
const int PORTA_RELE = 36;

//BUZZER;
const int BUZZER = 7;

//Display LCD
const int PINO_PWM_LCD = 6;
const int BACKLIGHT_LED = 25;


//controle de excecução
int parado = 0;
//contador de falhas de conexão
int num_fails = 0;

#define MAX_FAILED_CONNECTS 5
#define ABERTURA_PORTA 1000
#define RST 5
#define SS 53
#define ETH_PIN 10
#define BOTAO_ABRIR 28

//módulo de TAGS de 13,56MHz
MFRC522 mfrc522(SS, RST);

//LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(31, 30, 32, 33, 34, 35);

//teclado
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

//byte rowPins[ROWS] = {42, 43, 44, 45};
byte rowPins[ROWS] = {45, 44, 43, 42};

//byte colPins[COLS] = {38, 39, 40};
byte colPins[COLS] = {40, 39, 38};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  //regula o bounce do teclado
  keypad.setDebounceTime(60);

  //ligação teclado
  //linhas
  pinMode(42, OUTPUT);
  pinMode(43, OUTPUT);
  pinMode(44, OUTPUT);
  pinMode(45, OUTPUT);
  //colunas
  pinMode(38, INPUT);
  pinMode(39, INPUT);
  pinMode(40, INPUT);

  //botoeira de abrir porta
  pinMode(BOTAO_ABRIR, INPUT_PULLUP);


  lcd.begin(16, 2);
  pinMode(PINO_PWM_LCD, OUTPUT);

  //contraste do LCD;
  analogWrite(PINO_PWM_LCD, 60);
  lcd.setCursor(0, 0);
  lcd.write("-+#DINT--PMES#+-");

  //backlight led
  pinMode(BACKLIGHT_LED, OUTPUT);
  digitalWrite(BACKLIGHT_LED, HIGH);

  pinMode(LED_LEITURA_RFID, OUTPUT);
  pinMode(LED_LIB_ACESSO_RFID, OUTPUT);
  pinMode(LED_ACESSO_INVALIDO, OUTPUT);
  //  pinMode(botao_RFID, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(PORTA_RELE, OUTPUT);
  digitalWrite(PORTA_RELE, HIGH);

  digitalWrite(LED_ACESSO_INVALIDO, HIGH);
  digitalWrite(LED_LEITURA_RFID, HIGH);
  digitalWrite(LED_LIB_ACESSO_RFID, HIGH);

  Ethernet.begin(macAddr, meuIP, meuDNS, gw, mask);
  Serial.begin(115200);
  Serial.println("Inicializando...");
  Serial.println("Meu IP: ");
  Serial.println(Ethernet.localIP());
  lcd.setCursor(0, 1);
  lcd.write("Iniciando.....");
  delay(1000);
  digitalWrite(LED_ACESSO_INVALIDO, LOW);
  digitalWrite(LED_LEITURA_RFID, LOW);
  digitalWrite(LED_LIB_ACESSO_RFID, LOW);

  lcd.setCursor(14, 1);
  lcd.write("OK");
  delay(1000);

  //lcd.clear();
  //conecta ao banco de dados
  conectar();

  selecionaRFID();
  SPI.begin();
  mfrc522.PCD_Init();
  delay(1000);

  //coloco o watchDog por * segundos;
  wdt_enable(WDTO_8S);
}

void loop() {
  //reset do contador do watchdog, estou vivo!
  wdt_reset();




  selecionaRFID();
  lcd.setCursor(0, 0);
  lcd.write("-+#DINT--PMES#+-");
  lcd.setCursor(0, 1);
  //lcd.write("0123456789abcdef");
  lcd.write("Aproxime Cartao!");

  //desliga backlight
  digitalWrite(BACKLIGHT_LED, LOW);

  millisAtualRFID = millis();
  millisAtualConex = millis();

  //Evitar overflow do millis()
  if (millisAtualRFID < millisAnteriorRFID) {
    millisAnteriorRFID = millisAtualRFID;
  }
  if (millisAtualConex < millisAnteriorConex) {
    millisAnteriorConex = millisAtualConex;
  }

  //##############################################################################################
  //##################################Entrada por Cartão RFID#####################################
  //##############################################################################################

  //Período de leitura do RFID.
  if (millisAtualRFID - millisAnteriorRFID >= TEMPO_LED_RFID) {
    //marco o tempo o qual realizei a tarefa
    millisAnteriorRFID = millisAtualRFID;
    if (digitalRead(LED_LEITURA_RFID) == LOW) {
      digitalWrite(LED_LEITURA_RFID, HIGH);
    } else {
      digitalWrite(LED_LEITURA_RFID, LOW);
    }

    if (mfrc522.PICC_IsNewCardPresent()) {
      if (mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(BACKLIGHT_LED, HIGH);
        Serial.println("Memoria Livre (ANTES LEITURA DO CARTÃO):");
        Serial.println(get_free_memory());
        /*
          // Show some details of the PICC (that is: the tag/card)
          Serial.print(F("Card UID:"));
          //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
          Serial.println();
          Serial.print(F("PICC type: "));
          MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
          Serial.println(mfrc522.PICC_GetTypeName(piccType));
        */
        //Serial.println("Cartao detectado!");
        //Mostra UID na serial
        //Serial.print("UID da tag :");
        strChave = "";
        char chave[mfrc522.uid.size];
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
          //Serial.print(mfrc522.uid.uidByte[i], HEX);
          strChave.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
          strChave.concat(String(mfrc522.uid.uidByte[i], HEX));
          //chave[i] = toupper(HEX);
        }
        //Serial.println("");
        //paro e encerro o RFID e Habilito do Ethernet
        mfrc522.PICC_HaltA();
        //mfrc522.PCD_StopCrypto1();
        //desabilito RFID e habilito a Ethernet
        selecionaETH();
        //Serial.println("conteudo:");
        //Serial.println(conteudo);
        //Serial.println("");
        //Eliminar excesso de caracteres, evitar um possível overflow
        for (byte i = 0; i < 12; i++) {
          chave[i] = toupper(strChave[i]);
          //Serial.print(chave[i]);
        }

        tone(BUZZER, 1500, 200);

        if (my_conn.is_connected()) {
          wdt_reset();
          //my_conn.cmd_query("SELECT * FROM usuarios");
          //my_conn.show_results();

          //      String Final, Select do RFID, CHAVE
          sprintf(QUERY_AUX, SELECT_RFID, chave);
          Serial.println(QUERY_AUX);
          my_conn.cmd_query(QUERY_AUX);
          my_conn.get_columns();
          row_values *row = NULL;
          row = my_conn.get_next_row();

          do {
            if (row != NULL) {
              //resgatando os dados do usuário encontrado
              idUsuario = atol(row->values[0]);
              nomeUsuario = row->values[1];
              senhaBco = row->values[2];
              //Serial.println(nomeUsuario);
              //my_conn.show_results();
              lcd.setCursor(0, 1);
              lcd.write("   BEM VINDO!   ");
              delay(1000);
              lcd.setCursor(0, 1);
              lcd.write("                ");
              lcd.setCursor(0, 1);
              lcd.write(nomeUsuario);
              delay(1000);
              lcd.setCursor(0, 1);
              lcd.write("Senha:          ");

              if (senhaBco != NULL) {
                char senhaDig[] = "0000";
                byte c = 0;
                //Serial.println("Digite a senha");
                unsigned long millisAnteriorlSenha = millis();
                //loop de leitura de senha, lê o teclado enquanto o tempo não esgotar ou 4 digitos serem digitados.
                do {
                  wdt_reset();
                  //caso o usuário não termine de digitar a senha no tempo estipulado sai do loop
                  if (millis() - millisAnteriorlSenha > TEMPO_LEITURA_SENHA) {
                    break;
                  } else {
                    char keyPress = keypad.getKey();
                    if (keyPress != NO_KEY) {
                      tone(BUZZER, 1500, 200);
                      lcd.setCursor(6 + c, 1);
                      lcd.write("*");
                      senhaDig[c] = keyPress;
                      //Serial.print(keyPress);
                      c++;
                    }
                  }
                } while (c < 4);
                //verifico a senha
                /*
                                uint8_t *hash;
                                Sha1.init();
                                Sha1.print("1234");
                                hash = Sha1.result();
                */
                //Serial.println("TESTE HASH");
                //Serial.println(hash[0]);
                //printHash(hash);

                if (strcmp(senhaBco, senhaDig) == 0) {
                  //Serial.println(">>>>>>>>>>>>> SENHAS IGUAIS");
                  //Faço uma espécie de preparedStatement para execução da Query do Histórico;
                  sprintf(QUERY_AUX, INSERT_HISTORICO_RFID, PORTA_ATUACAO, idUsuario);
                  //inserção na tabela de hitórico!
                  my_conn.cmd_query(QUERY_AUX);
                  acessoPermitido();
                } else {
                  //Serial.println(">>>>>>>>>>>>> SENHAS DIFERENTES");
                  acessoNegado();
                }
              } else {
                acessoNegado();
              }
            } else {
              acessoNegado();
            }
            row = NULL;
          } while (row != NULL);
          //Libera o buffer de linhas
          my_conn.free_row_buffer();                               
          //Libera o buffer de colunas;
          my_conn.free_columns_buffer();
          my_conn.clear_ok_packet();

          Serial.println("Memoria Livre (DEPOIS CONSULTA):");
          Serial.println(get_free_memory());
          num_fails = 0;
        } else {
          //Reset para restauração, visto que não está conseguindo reconectar pelo conectar();
          soft_reset();
        }
      }
    }
  }

  //Verifico se é tempo de checar a conexão com o banco!
  if (millisAtualConex - millisAnteriorConex >= TEMPO_VERIFICA_CONEX) {
    millisAnteriorConex = millisAtualConex;
    if (!my_conn.is_connected()) {
      //Reset para restauração, visto que não está conseguindo reconectar pelo conectar();
      soft_reset();
    }
  }

  //Entrada por botoeira.
  if (digitalRead(BOTAO_ABRIR) == LOW) {
    delay(100);
    acessoPermitido();
  }

  //##############################################################################################
  //##################################Entrada por RG e Senha######################################
  //##############################################################################################

  char keyPress = keypad.getKey();
  if (keyPress == '#') {
    Serial.println("Memoria Livre RG (ANTES CONSULTA):");
    Serial.println(get_free_memory());
    digitalWrite(BACKLIGHT_LED, HIGH);
    tone(BUZZER, 1500, 200);
    lcd.setCursor(0, 1);
    lcd.write("Digite RG:      ");
    unsigned long millisAnteriorRG = millis();
    char keyRG;
    String strRG = "";
    byte tamRG = 0;
    //Serial.println("RG: ");
    do {
      wdt_reset();
      //caso o usuário não termine de digitar rg no tempo estipulado sai do loop
      if (millis() - millisAnteriorRG > TEMPO_LEITURA_SENHA) {
        break;
      } else {
        keyRG = keypad.getKey();
        if (keyRG == '#' || keyRG == '*') {
          break;
        }
        if (keyRG != NO_KEY && keyRG != '#' && keyRG != '*') {
          //Serial.print(keyRG);
          tone(BUZZER, 1500, 200);
          lcd.setCursor(10 + tamRG, 1);
          lcd.write(keyRG);
          tamRG++;
          strRG.concat(keyRG);
        }
      }
    } while (keyRG != '#' || keyRG != '*');

    char *rgAux = new char[strRG.length() + 1];
    strcpy(rgAux, strRG.c_str());

    //Se confirmou com a tecla #
    if (keyRG == '#' && rgAux != NULL) {
      if (my_conn.is_connected()) {
        //      String Final, Select do RFID, CHAVE
        sprintf(QUERY_AUX, SELECT_RG, rgAux);

        delete [] rgAux;

        Serial.println(QUERY_AUX);
        my_conn.cmd_query(QUERY_AUX);
        my_conn.get_columns();
        row_values *row = NULL;
        row = my_conn.get_next_row();
        do {
          if (row != NULL) {
            //resgatando os dados do usuário encontrado
            idUsuario = atol(row->values[0]);
            nomeUsuario = row->values[1];
            senhaBco = row->values[2];
            //Serial.println(nomeUsuario);
            //my_conn.show_results();
            lcd.setCursor(0, 1);
            lcd.write("   BEM VINDO!   ");
            delay(1000);
            lcd.setCursor(0, 1);
            lcd.write("                ");
            lcd.setCursor(0, 1);
            lcd.write(nomeUsuario);
            delay(1000);
            lcd.setCursor(0, 1);
            lcd.write("Senha:          ");
            //Serial.println("Serial!");

            if (senhaBco != NULL) {
              char senhaDig[] = "0000";
              //contador
              byte c = 0;
              //Serial.println("Digite a senha");
              unsigned long millisAnteriorlSenha = millis();
              //loop de leitura de senha, lê o teclado enquanto o tempo não esgotar ou 4 digitos serem digitados.
              do {
                wdt_reset();
                //caso o usuário não termine de digitar a senha no tempo estipulado sai do loop
                if (millis() - millisAnteriorlSenha > TEMPO_LEITURA_SENHA) {
                  break;
                } else {
                  char keyPress = keypad.getKey();
                  if (keyPress != NO_KEY) {
                    tone(BUZZER, 1500, 200);
                    lcd.setCursor(6 + c, 1);
                    lcd.write("*");
                    senhaDig[c] = keyPress;
                    //Serial.print(keyPress);
                    c++;
                  }
                }
              } while (c < 4);
              //verifico a senha
              if (strcmp(senhaBco, senhaDig) == 0) {
                //Serial.println(">>>>>>>>>>>>> SENHAS IGUAIS");
                //Faço uma espécie de preparedStatement para execução da Query do Histórico;
                sprintf(QUERY_AUX, INSERT_HISTORICO_RG, PORTA_ATUACAO, idUsuario);
                //inserção na tabela de hitórico!
                my_conn.cmd_query(QUERY_AUX);
                acessoPermitido();

              } else {
                //Serial.println(">>>>>>>>>>>>> SENHAS DIFERENTES");
                acessoNegado();
              }
            } else {
              acessoNegado();
            }

          } else {
            acessoNegado();
          }
          //free(row);
          row = NULL;
        } while (row != NULL);
        //Libera o buffer de linhas
        my_conn.free_row_buffer();
        //Libera o buffer de colunas;
        my_conn.free_columns_buffer();
        my_conn.clear_ok_packet();
        Serial.println("Memoria Livre RG (DEPOIS CONSULTA):");
        Serial.println(get_free_memory());
        num_fails = 0;
      } else {
        //Reset para restauração, visto que não está conseguindo reconectar pelo conectar();
        soft_reset();
      }
    }
  }
}

void acessoPermitido() {
  digitalWrite(LED_LIB_ACESSO_RFID, HIGH);
  //Serial.println("Entrada Liberada!");
  //delay de abertura
  digitalWrite(PORTA_RELE, LOW);
  tone(BUZZER, 2900, 300);
  delay(ABERTURA_PORTA);
  digitalWrite(PORTA_RELE, HIGH);
  digitalWrite(LED_LIB_ACESSO_RFID, LOW);
  //digitalWrite(PORTA_RELE, HIGH);
}

void acessoNegado() {
  //Serial.println("Acesso nao autorizado! Chave nao encontrada!");
  lcd.setCursor(0, 1);
  lcd.write(" NAO AUTORIZADO ");
  for (byte c = 0; c <= 1; c++) {
    tone(BUZZER, 2900);
    digitalWrite(LED_ACESSO_INVALIDO, HIGH);
    delay(400);
    digitalWrite(LED_ACESSO_INVALIDO, LOW);
    noTone(BUZZER);
  }
}

//##############################################################################################
//##################################Conexão com Banco###########################################
//##############################################################################################
void  conectar() {
  selecionaETH();
  while (!my_conn.is_connected()) {
    Serial.println("Conectando ao Banco...");
    lcd.setCursor(0, 1);
    lcd.write("Conectando......");
    if (my_conn.mysql_connect(servidorSQL, 3306, user, password)) {
      delay(500);
      my_conn.cmd_query("USE arduino_db");
      toqueConexao();
      lcd.setCursor(14, 1);
      lcd.write("OK");
    } else {
      num_fails++;
      Serial.println("Falha ao Conectar!");
      if (num_fails == MAX_FAILED_CONNECTS) {
        Serial.println("Falha ao tentar conectar por 5x... Reiniciando Sistema...");
        lcd.setCursor(0, 1);
        lcd.write("Falha Conexao-BD)");
        delay(1000);
        lcd.setCursor(0, 1);
        lcd.write("Reiniciando Sist");
        tone(BUZZER, 1900, 2000);
        delay(1000);
        soft_reset();
      }
    }
  }
}
//seleciona o SPI para utilização do Shield Ethernet
void selecionaETH() {
  //desabilita o RFID
  digitalWrite(SS, HIGH);
  //habilitar Ethernet
  digitalWrite(ETH_PIN, LOW);
}
//seleciona o SPI para utilização do Shield RFID
void selecionaRFID() {
  //desabilita Ethernet
  digitalWrite(ETH_PIN, HIGH);
  //habilita RFID
  digitalWrite(SS, LOW);
}

//toque que precede a conexão ao banco
void toqueConexao() {
  for (int i = 0; i <= 2500; i += 500) {
    tone(BUZZER, i + 1000);
    delay(400);
  }
  noTone(BUZZER);
}

//reset via comando
void soft_reset() {
  asm volatile("jmp 0");
}


int get_free_memory() {
  extern char __bss_end;
  extern char *__brkval;
  int free_memory;
  if ((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);
  return free_memory;
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
