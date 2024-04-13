// **************************************************
//  Station_base_RX.ino
//  Projet CANSAT 2020-2021
//  5ème Electronique INRACI
//  Hardware : ARDUINO UNO + RFM69
//  4/1/2021
// **************************************************
 
// ******************** LIBRAIRIES ******************
#include <SPI.h>
#include <RH_RF69.h>  // Librairie du module radio RFM69
 
//*********** DEFINITION DES CONSTANTES *************
#define RF69_FREQ 433.0  // Fréquence d'émission de  424 à 510 MHz 
#define RFM69_INT 3
#define RFM69_CS 4
#define RFM69_RST 2
//#define debug_reception
 
//************* DEFINITION DES OBJETS ************
RH_RF69 rfm69(RFM69_CS, RFM69_INT);
 
//************* DEFINITION DES VARIABLES ************
String tableau_reception[40];
int compteur = 0;
int cpt_char = 0;
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];  // Buffer de réception
// FORMAT :Packetnum|Time_ms|TMP36_Temperature|BMP280_Temperature|BMP280_Pression|BMP280_AltitudeApprox|OzoneConcentration;\r\n
uint8_t len;  // Taille du buffer de réception
char *token;                            // Pointeur vers le token extrait
int integerPart;
char *token_start;
float floatPart;
int nb_packet;
int len_;
 
 
unsigned long Time_ms;  // "temps" en milliseconde depuis le dernier reset du uP
 
void setup() {
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // Attente de l'ouverture du moniteur série : !!!!!!!!!!! ATTENTION BLOQUANT !!!!!!!!!!
 
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
 
  // Initialisation et configuration du module radio RFM69
  digitalWrite(RFM69_RST, HIGH);  // Reset manuel
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  if (!rfm69.init())
  {
 
    while (1)  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ATTENTION BLOQUANT !!!!!!!!!!!!!!!!!!!!!!
    {
      Serial.println("RFM69 radio init failed");
      delay(200);
    }
  }
  else
  {
    Serial.println("RFM69 radio init Succes");
  }
 
  if (!rfm69.setFrequency(RF69_FREQ))
  {
 
    while (1)  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ATTENTION BLOQUANT !!!!!!!!!!!!!!!!!!!!!!
    {
      Serial.println("setFrequency failed");
      delay(200);
    }
  }
  else
  {
    Serial.println("setFrequency Succes");
  }
 
  rfm69.setTxPower(20, true);  // Puissance d'émission 14 à 20
 
  // Clé de codage doit être identique dans l'émetteur
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
  rfm69.setEncryptionKey(key);
 
  Time_ms = millis();
}
 
void loop() {
  uint8_t test[] = "TEST TRANSMISSION";
  rfm69.send(test, sizeof(test)); /*envois*/
  rfm69.waitPacketSent();
 
  if (rfm69.available())  // Donnée présente ?
  {
    // Récupération et affichage des données
    len = sizeof(buf);
    if (rfm69.recv(buf, &len))
    {
      if (!len) return;
      buf[len] = 0;
      token = strtok(buf, ",");              // Découper la chaîne en fonction de la virgule comme délimiteur
      integerPart = atoi(token);        // Récupérer le nombre entier avant la virgule
      token = strtok(NULL, ",");           // Récupérer la partie flottante après la virgule
      floatPart = atof(token);
     
      if (++compteur == integerPart)
      {
        Serial.print(floatPart);
        Serial.print(",");
      }
 
      #ifdef debug_reception
      Serial.println((char*)buf);
      #endif
      if (buf[0] == '#')
      {
        Serial.print("RSSI:");
        Serial.print(rfm69.lastRssi(), DEC);
        Serial.print(" ");
        token_start = strtok(buf, "#");
        nb_packet = atoi(token_start);
       
        for(int i; i < nb_packet+1 ;i++)
        {         //remplir le tableau pour detecter les erreurs
          tableau_reception[i] = String("erreur");
        }
        compteur = 0;
        Serial.print("#,");
        Serial.print(nb_packet);
        Serial.print(", ");
      }
 
      if (buf[len - 1] == '$')
      {
        Serial.print("$");
        len_ = sizeof(tableau_reception);
      }
    } else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
 
    if (millis() >= Time_ms + 1500)
    {  //prends une mesure toute les 10ms
      Serial.println(".");
      Time_ms = millis();
    }
  }
}