/**
   Protótipo de Fechadura Eletrônica DINT
*/
#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
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

/* Setup for the Connector/Arduino */
Connector my_conn;        // The Connector/Arduino reference

//Usuário Banco
char user[] = "arduino2";
char password[] = "123456";
const char PORTA_ATUACAO[] = "PORTA X";
char SELECT_RFID[100] = "SELECT id, nome FROM arduino_db.usuarios WHERE(chave like '%s');";
//INSERT de acionamento da porta
char INSERT_HISTORICO[100] = "INSERT INTO arduino_db.historico (porta, data_hora, id_usuario) VALUES ('%s', now(), %d);";
char QUERY_AUX[100] = "";

//usuario resgatando do banco via SELECT
long id_usuario = 0;
//char* chave_acesso;
char* nome_usuario;
char ip[16];

unsigned long millisAnteriorRFID = 0;
unsigned long millisAtualRFID = 0;
unsigned long millisAnteriorConex = 0;
unsigned long millisAtualConex = 0;

//Tempo de execução de leitura do shield RFID
long tempoLeituraRIFD = 600;
long tempoLeituraStop = 300;
long tempoLedRFID = 500;
long tempoVerificaConex = 300000 * 12;

//Para efeito de simulação Entrada RFID sera o ACIONAMENTO DO PINO 4
const int botao_RFID = 23;

//Led de leitura efetuada do cartão RFID
const int ledLeituraRFID = 24;
//Led de Liberação de acesso;
const int ledLibAcessoRFID = 26;
//Led indicado de Acesso Inválido
const int ledAcessoInvalido = 22;
//rele de acionamento da porta
const int portaRele = 21;

//BUZZER;
const int BUZZER = 7;

//Display LCD
const int pinoPwmLcd = 6;



//controle de excecução
int parado = 0;
//contador de falhas de conexão
int num_fails = 0;

#define MAX_FAILED_CONNECTS 5
#define ABERTURA_PORTA 3000
#define RST 5
#define SS 53
#define ETH_PIN 10

//módulo de TAGS de 13,56MHz
MFRC522 mfrc522(SS, RST);

//LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(31, 30, 32, 33, 34, 35);

void setup() {

  lcd.begin(16, 2);
  pinMode(pinoPwmLcd, OUTPUT);
  //contraste do LCD;
  analogWrite(pinoPwmLcd, 100);
  lcd.setCursor(0, 0);
  lcd.write("-+#DINT--PMES#+-");

  pinMode(ledLeituraRFID, OUTPUT);
  pinMode(ledLibAcessoRFID, OUTPUT);
  pinMode(ledAcessoInvalido, OUTPUT);
  pinMode(botao_RFID, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(portaRele, OUTPUT);

  digitalWrite(ledAcessoInvalido, HIGH);
  digitalWrite(ledLeituraRFID, HIGH);
  digitalWrite(ledLibAcessoRFID, HIGH);

  Ethernet.begin(macAddr, meuIP, meuDNS, gw, mask);
  Serial.begin(115200);
  Serial.println("Inicializando...");
  Serial.println("Meu IP: ");
  Serial.println(Ethernet.localIP());
  lcd.setCursor(0, 1);
  lcd.write("Iniciando.....");
  while (!Serial) ;
  delay(2000);
  digitalWrite(ledAcessoInvalido, LOW);
  digitalWrite(ledLeituraRFID, LOW);
  digitalWrite(ledLibAcessoRFID, LOW);

  //ativar resistores pull-up internos
  digitalWrite(botao_RFID, HIGH);
  digitalWrite(portaRele, LOW);

  lcd.setCursor(14, 1);
  lcd.write("OK");
  delay(1000);

  //lcd.clear();
  lcd.setCursor(0, 1);
  //lcd.write("0123456789abcdef");
  lcd.write("Conectando......");
  //conecta ao banco de dados
  conectar();
  lcd.setCursor(14, 1);
  lcd.write("OK");

  selecionaRFID();
  SPI.begin();
  mfrc522.PCD_Init();   // Inicia MFRC522
  delay(1000);
}

void loop() {
  selecionaRFID();
  lcd.setCursor(0, 0);
  lcd.write("-+#DINT--PMES#+-");
  lcd.setCursor(0, 1);
  //lcd.write("0123456789abcdef");
  lcd.write("Aprox. o Cartao!");

  millisAtualRFID = millis();
  millisAtualConex = millis();

  /*
    //Evitar overflow do millis()
    if (millisAtualRFID < millisAnteriorRFID) {
      millisAnteriorRFID = millisAtualRFID;
    }
    if (millisAtualConex < millisAnteriorConex) {
      millisAnteriorConex = millisAtualConex;
    }
  */

  //Período de leitura do RFID
  if (millisAtualRFID - millisAnteriorRFID >= tempoLedRFID) {
    //marco o tempo o qual realizei a tarefa
    millisAnteriorRFID = millisAtualRFID;
    if (digitalRead(ledLeituraRFID) == LOW) {
      digitalWrite(ledLeituraRFID, HIGH);
    } else {
      digitalWrite(ledLeituraRFID, LOW);
    }

    if (mfrc522.PICC_IsNewCardPresent()) {
      if (mfrc522.PICC_ReadCardSerial()) {
        /*
                // Show some details of the PICC (that is: the tag/card)
                Serial.print(F("Card UID:"));
                //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
                Serial.println();
                Serial.print(F("PICC type: "));
                MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
                Serial.println(mfrc522.PICC_GetTypeName(piccType));

        */

        Serial.println("Cartao detectado!");
        //Mostra UID na serial
        Serial.print("UID da tag :");
        String conteudo = "";
        char chave[11];
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
          conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
          conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
          chave[i] = toupper(HEX);
        }
        Serial.println("");
        //paro e encerro o RFID e Habilito do Ethernet
        mfrc522.PICC_HaltA();
        //mfrc522.PCD_StopCrypto1();

        selecionaETH();

        Serial.println("conteudo:");
        Serial.println(conteudo);
        Serial.println("");

        
        //Eliminar o espaço inicial (iniciando de 1)
        for (byte i = 0; i < 12; i++) {
          chave[i] = toupper(conteudo[i]);
          //Serial.print(chave[i]);
        }

        if (my_conn.is_connected()) {
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
              id_usuario = atol(row->values[0]);
              nome_usuario = row->values[1];


              Serial.println(nome_usuario);
              //my_conn.show_results();

              //Faço uma espécie de preparedStatement para execução da Query do Histórico;
              sprintf(QUERY_AUX, INSERT_HISTORICO, "Porta Z", id_usuario);
              //inserção na tabela de hitórico!
              my_conn.cmd_query(QUERY_AUX);


              row = NULL;

              digitalWrite(ledLibAcessoRFID, HIGH);
              Serial.println("Entrada Liberada!");

              tone(BUZZER, 2900, 300);
              //delay de abertura
              //digitalWrite(portaRele, LOW);

              lcd.setCursor(0, 1);
              lcd.write("   BEM VINDO!   ");
              delay(1000);
              lcd.setCursor(0, 1);
              lcd.write("                ");
              lcd.setCursor(0, 1);
              lcd.write(nome_usuario);
              delay(ABERTURA_PORTA);

              digitalWrite(ledLibAcessoRFID, LOW);
              //digitalWrite(portaRele, HIGH);
            } else {
              Serial.println("Acesso nao autorizado! Chave nao encontrada!");

              lcd.setCursor(0, 1);
              lcd.write(" NAO AUTORIZADO ");
              for (byte c = 0; c <= 1; c++) {
                tone(BUZZER, 2900);
                digitalWrite(ledAcessoInvalido, HIGH);
                delay(400);
                digitalWrite(ledAcessoInvalido, LOW);
                noTone(BUZZER);
              }
            }

            //Libera o buffer de linhas
            my_conn.free_row_buffer();
            //Libera o buffer de colunas;
            my_conn.free_columns_buffer();
            my_conn.clear_ok_packet();

          } while (row != NULL);


          //Serial.println("Memoria Livre (DEPOIS CONSULTA):");
          //Serial.println(get_free_memory());
          num_fails = 0;
          
        } else {
          //tento conexão ao banco
          conectar();
        }
      }
    }
  }

  //Verifico se é tempo de checar a conexão com o banco!
  if (millisAtualConex - millisAnteriorConex >= tempoVerificaConex) {
    millisAnteriorConex = millisAtualConex;
    //conectar();
  }


  /*
    if (!digitalRead(botao_STOP)) {
      delay(500);
      if (parado == 1) {
        digitalWrite(ledAcessoInvalido, LOW);
        Serial.println("RODANDO!");
        parado = 0;
      } else {
        digitalWrite(ledAcessoInvalido, HIGH);
        Serial.println("STOP ATIVADO!");
        parado = 1;
      }
    }
  */
}

//realiza a conexão, tenta 5 vezes, senão reinicia o serviço!
void  conectar() {
  selecionaETH();
  while (!my_conn.is_connected()) {
    Serial.println("Conectando ao Banco...");
    if (my_conn.mysql_connect(servidorSQL, 3306, user, password)) {
      delay(500);
      Serial.println("Conectado com sucesso!");
      my_conn.cmd_query("USE arduino_db");
      toqueConexao();
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


void selecionaETH() {
  //desabilita o RFID
  digitalWrite(SS, HIGH);
  //habilitar Ethernet
  digitalWrite(ETH_PIN, LOW);

}

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

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
