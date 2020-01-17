
#include <SPI.h>
#include <Ethernet.h>
#include <RFID.h>
#include <LiquidCrystal.h>

void make_request(String& id);                                     // make a badge request
void time();                                                       // make a time request to be displayed on the lcd

    // CONFIG ethernet
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {192, 168, 1, 141};                                    // static arduino IP
byte server[] = {192, 168, 1, 201};                                // static server IP
EthernetClient client;                                             

    // CONFIG RFID
#define SS_PIN 9
#define RST_PIN 8
RFID rfid(SS_PIN, RST_PIN);             

    // CONFIG LCD
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

    // delay
unsigned long previousMillis = 0, currentMillis = 0;

void setup()
{    Serial.begin(9600);
     SPI.begin(); 
     rfid.init();
     lcd.begin(16, 2);
     
     lcd.setCursor(0, 0);
     lcd.print(F("Device setup.."));
     
         // disable rfid
     pinMode(9, OUTPUT);
     digitalWrite(9, HIGH);
     
         // disable sd card
     pinMode(4, OUTPUT);
     digitalWrite(4, HIGH);
      
         // enable ethernet
     pinMode(10, OUTPUT);
     digitalWrite(10, LOW); 
     
         // setup buzzer (pin 7)
     pinMode(3, OUTPUT);
     digitalWrite(3, HIGH);
     pinMode(7, OUTPUT);
     digitalWrite(7, HIGH);
     
        // acquiring ip without dhcp
     Ethernet.begin(mac, ip);
     delay(1000);
     lcd.setCursor(14, 0);
     lcd.print(F("OK"));
     delay(1000);
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print(F("     ICAS94     "));
     lcd.setCursor(0, 1);
     lcd.print(F("Waiting server.."));
}

void loop()
{   // if a card is read, then get the id from the card and call make_request() for validation 
	if (rfid.isCard())
     {    if (rfid.readCardSerial())
          {    String id = "";
               id += rfid.serNum[0];
               id += rfid.serNum[1];
               id += rfid.serNum[2];
               id += rfid.serNum[3];
               id += rfid.serNum[4];
               //lcd.setCursor(0, 0);
               //lcd.print(id);
               //delay(7000);
               make_request(id);
               lcd.setCursor(0, 0);
               lcd.print(F("     ICAS94     "));
               lcd.setCursor(0, 1);         
          }
     }
	
     currentMillis = millis();
	 
	//	fix the currentMillis overflow problem
    if(currentMillis<previousMillis)
        previousMillis = currentMillis;
		 
	// if a certain amount of time has passed (2 seconds), calculate the time to be displayed on the lcd calling time()
    if(currentMillis-previousMillis >= 2000)
    {   previousMillis = currentMillis;
        time();
        lcd.setCursor(0, 0);
        lcd.print(F("     ICAS94     "));
    }
}

void make_request(String& id)
{   if (client.connect(server, 80))
    {   // enable buzzer
		digitalWrite(7, LOW);
		
		// create the request pointing to the page under the path apache2/htdocs/index.php (in my case)
         String richiesta = "GET /Timb/index.php?code=";
         richiesta += id;
         richiesta += " HTTP/1.0";
         client.println(richiesta); 
         client.println();
         delay(50);
		 
		/*  create the answer (it's not the best solution, but for my little project is perfect): 
			- after the request has been sent to the server, index.php gets dynamically populated with the content of the database
			- all of the strings to be displayed on the lcd will be created with the structure "XXXanswer;" if i have to display an error, or "XXXanswer." if the request was ok
			- i have to look inside the idex.php for the first occurrance of XXX
			- start reading and storing the chars in the 'answer' variable till ';' or '.' is found
		*/
		
         String risposta = ""; 	// answer variable
         int contatore = 0;
         char c = 'a';
         char c_prec = 'a';
         while(true)
         {    if (client.available())
              {    c = client.read();
                   if(contatore >=3)
                        risposta += c;
                   else
                   {    if(c == 'X')
                        {    if(c_prec == 'X')
                                  contatore++;
                             else
                             {    contatore = 1; 
                                  c_prec = c;
                             }    
                        }
                        else if(c != 'X' && contatore < 3)
                        {    contatore = 0;
                             c_prec = c;
                        }
                   }
              }
			  // if there isn't anything left to be read from the server display the message
              if (!client.connected())
              {    client.stop();
                   /* read the last character of the string:
                           - if i read ';' there must have been and error, then the red led is turned on (on pin 6)
                           - if i read '.' everything is ok and the green led is turned on (on pin 5)
                   */
                   risposta.trim();
                   if(risposta.lastIndexOf(';')>0)
                   {   risposta.replace(';', '\0'); 
                       digitalWrite(6, HIGH);
                   }
                   else
                       digitalWrite(5, HIGH);
					   
					// print on the display
                   lcd.clear();
                   lcd.print(risposta.substring(0, risposta.lastIndexOf('.')));
                   lcd.setCursor(0, 1);
                   lcd.print(risposta.substring(risposta.lastIndexOf('.')+1));
                   delay(1500);
				   
				   // turn off the leds and the buzzer
                   digitalWrite(5, LOW);
                   digitalWrite(6, LOW);
                   digitalWrite(7, HIGH);
                   lcd.clear();
                   break;
              }             
         }
    }  
}

void time(void)
{   lcd.setCursor(0, 1);
    if (client.connect(server, 80))
    {    /* same logic of the make_request(), but what i get is the time from server
			setting in the request "time=1" is interpreted from the server like 'give me the actual time'
		 */
	     client.println("GET /Timb/index.php?time=1 HTTP/1.0"); 
         client.println();
         delay(50);
         String risposta = "";
         int contatore = 0;
         char c = 'a';
         char c_prec = 'a';
         while(true)
         {    if (client.available())
              {    c = client.read();
                   if(contatore >=3)
                        risposta += c;
                   else
                   {    if(c == 'X')
                        {    if(c_prec == 'X')
                                  contatore++;
                             else
                             {    contatore = 1; 
                                  c_prec = c;
                             }    
                        }
                        else if(c != 'X' && contatore < 3)
                        {    contatore = 0;
                             c_prec = c;
                        }
                   }
              }  
              if (!client.connected())
              {    client.stop();
                   lcd.print(risposta);
                   break;
              }             
         }
    }
	/* if the server cannot be reached (server down or cable unplugged), the leds start blinkin and the lcd displays 'waiting server'
		until the communication with the server is restored. at that point the time is diplayed
	*/
    else
    {    lcd.print(F("Waiting server.."));
         for(int i=0; i<10; i++)
         {   digitalWrite(5, HIGH);
             digitalWrite(6, HIGH);
             delay(100);
             digitalWrite(5, LOW);
             digitalWrite(6, LOW);
             delay(100);
         }
    }
}


 

