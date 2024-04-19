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
#define RF69_FREQ 434.2  // Fréquence d'émission de  424 à 510 MHz 
#define RFM69_INT 3
#define RFM69_CS 4
#define RFM69_RST 2
//#define debug_reception
#define VERTICAN_run 0
#define VERTICAN_format_file 1
#define VERTICAN_extract_file 2
#define VERTICAN_save_on_flash 3
#define VERTICAN_no_backup_on_flash 4
#define VERTICAN_backup_on_radio 5
  
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
int type_de_reception = 0;
String donne_de_reception;
String Radiopacket;
 
 
unsigned long Time_ms;  // "temps" en milliseconde depuis le dernier reset du uP

void setup() 
{
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

String rfm69Reception() 
{
  uint8_t len;
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  
  if (rfm69.available()) // Vérifier si des données sont disponibles
  {
    len = sizeof(buf);
    if (rfm69.recv(buf, &len)) 
    {
      // Assurer qu'il y a des données dans le tampon
      if (len > 0) 
      {
        //Serial.print("donnée dans le tampon RF :");
       // Serial.println((char*)buf);  

        // Convertir le tampon en une chaîne de caractères
        buf[len] = '\0'; // Assurer que le tampon est terminé par un caractère nul pour être une chaîne valide
        return String((char*)buf);
      }
    }
  }
  // Si aucune donnée n'est disponible, retourner une chaîne vide
  return "";
}

char commandeReception() 
{
  static String commandBuffer = ""; // Variable statique pour stocker la commande en cours de saisie
  char receivedChar;

  String receivedData = rfm69Reception(); // Stocker la donnée reçue dans une variable de type String

  if (strstr(receivedData.c_str(), "format") != NULL) // Recherche du mot "format" dans le tampon 
      {
        Serial.println("Le mot 'format' a été recu en RF!");
        if (confirmFormat())  { return VERTICAN_format_file; } 
      }

    if (strstr(receivedData.c_str(), "extract") != NULL)  // Recherche du mot "extract" dans le tampon
      {
        Serial.println("Le mot 'extract' a été recu en RF!");
        return VERTICAN_extract_file;
      }

      if (strstr(receivedData.c_str(), "save") != NULL)  // Recherche du mot "save" dans le tampon
      {
        Serial.println("Le mot 'save' a été recu en RF!");
        return VERTICAN_save_on_flash;
      }

      if (strstr(receivedData.c_str(), "noflash") != NULL)  // Recherche du mot "noflash" dans le tampon
      {
        Serial.println("Le mot 'noflash' a été recu en RF!");
        return VERTICAN_no_backup_on_flash;
      }

      if (strstr(receivedData.c_str(), "radio") != NULL)  // Recherche du mot "radio" dans le tampon
      {
        Serial.println("Le mot 'radio' a été recu en RF!");
        return VERTICAN_backup_on_radio;
      }
  
  while (Serial.available() > 0 || commandBuffer.length() > 0 ) 
  {
    while (Serial.available() > 0)  // Lire les caractères disponibles depuis le port série
    {
      receivedChar = Serial.read();
  
      if (receivedChar == ' ' && commandBuffer.length() == 0)   // Ignorer les caractères d'espacement supplémentaires
      {
        continue;
      }

      if (receivedChar == '\r' || receivedChar == '\n')         // Si le caractère est un retour chariot ou un retour à la ligne
      {
        if (commandBuffer.length() > 0) // Traiter la commande // Vérifier si la commande est non vide
        { 
          commandBuffer.trim(); // Supprimer les espaces avant et après la commande
          if (commandBuffer == "format") 
          {
            if (confirmFormat()) 
            {
              commandBuffer = ""; // Réinitialiser le tampon de commande
              return VERTICAN_format_file;
            } 
          } 
          else if (commandBuffer == "extract")
          {
            Serial.println("data extract file");
            commandBuffer = ""; // Réinitialiser le tampon de commande
            return VERTICAN_extract_file;
            //extractData();
          }  
          else if (commandBuffer == "save")
          {
            commandBuffer = ""; // Réinitialiser le tampon de commande
            return VERTICAN_save_on_flash;
          }  
          else if (commandBuffer == "noflash")
          {
            commandBuffer = ""; // Réinitialiser le tampon de commande
            return VERTICAN_no_backup_on_flash;
          } 
          else if (commandBuffer == "radio")
          {
            commandBuffer = ""; // Réinitialiser le tampon de commande
            return VERTICAN_backup_on_radio;
          }
          else 
          {
            Serial.println("Invalid command. Type 'format' to format the flash memory or 'extract' to extract data.");
          }
        }
        commandBuffer = "";             // Réinitialiser le tampon de commande
      } else 
      {
        commandBuffer += receivedChar;  // Ajouter le caractère au tampon de commande
      }
    }
  }
  return VERTICAN_run;
}

bool confirmFormat()
{
  Serial.println("Are you sure you want to format the flash memory? This will erase all data. (yes/no)");
  String response = ""; // Variable pour stocker la réponse

  while (true) {
    // Lire les caractères série jusqu'à ce qu'un retour chariot ou un retour à la ligne soit détecté
    while (Serial.available() > 0) {
      char receivedChar = Serial.read();

      // Ignorer les caractères d'espacement supplémentaires
      if (receivedChar == ' ' && response.length() == 0) {
        continue;
      }

      // Si le caractère est un retour chariot ou un retour à la ligne
      if (receivedChar == '\r' || receivedChar == '\n') {
        // Vérifier si la réponse est non vide
        if (response.length() > 0) {
          response.trim(); // Supprimer les espaces avant et après la réponse
          if (response.equalsIgnoreCase("yes")) 
          {
            return true;
          } 
          else if (response.equalsIgnoreCase("no")) 
          {
            Serial.println("Format canceled.");
            return false;
          } else {
            Serial.println("Invalid response. Please enter 'yes' or 'no'.");
          }
        }
        // Réinitialiser la réponse
        response = "";
      } else {
        // Ajouter le caractère à la réponse
        response += receivedChar;
      }
    }
  }
}

void send_radio_msg(String message)
{
  rfm69.send((uint8_t *)(message.c_str()), message.length()); //permet de retrouver le status via la radio
  rfm69.waitPacketSent();
}

void loop()
{
  
  
  switch (commandeReception()) 
  {
    case VERTICAN_format_file: send_radio_msg("format");            break;
    case VERTICAN_extract_file: send_radio_msg("extract");          break;
    case VERTICAN_backup_on_radio: send_radio_msg("radio");         break;
    case VERTICAN_no_backup_on_flash: send_radio_msg("noflash");    break;
    case VERTICAN_save_on_flash: send_radio_msg("save");            break;
    //default: VERTICAN_run:     break;
  }


  
  if (rfm69.available())  // Donnée présente ?
  {
    //todo buzzer
    // Récupération et affichage des données
    len = sizeof(buf);
    if (rfm69.recv(buf, &len))
    {
      if (!len) return;
      Time_ms = millis();
      buf[len] = 0; //on place un 0 dans le dernier element du tableau pour signifier la fin du tableau
      #ifdef debug_reception
        Serial.println((char*)buf);
      #endif

      if(buf[0] == '?')
      {
       // send_radio_msg("reception_ok");
        type_de_reception = 2;
      }

      if (buf[0] == '#')
      {
        type_de_reception = 1;
        Serial.print("\nRSSI:");
        Serial.print(rfm69.lastRssi(), DEC);
        Serial.print(" ");
        token_start = strtok(buf, "#");
        nb_packet = atoi(token_start);
       
        for(int i; i < nb_packet+1 ;i++)  //remplir le tableau pour detecter les erreurs
        {         
          tableau_reception[i] = String("erreur");
        }
        compteur = 0;
        Serial.print("#,");
        Serial.print(nb_packet);
        Serial.print(", ");
      }
      
    } else
    {
      Serial.println("Receive failed");
    }


  switch (type_de_reception){
    case 2:
     if(buf[0] != '!')
      { 
      Serial.print((char*)buf);
        if(buf[0] == '$')
        {
        //Serial.print("\n");
        }
      }
      if (strstr(buf, "something") != NULL)
      {
        type_de_reception = 1;
        send_radio_msg("e");
        delay(1);
        send_radio_msg("e");
        delay(1);
        send_radio_msg("e");
        delay(1);
        send_radio_msg("e");
        delay(1);
      }
    break;
    case 1: 
      token = strtok(buf, ",");              // Découper la chaîne en fonction de la virgule comme délimiteur
      integerPart = atoi(token);        // Récupérer le nombre entier avant la virgule
      token = strtok(NULL, ",");           // Récupérer la partie flottante après la virgule
      floatPart = atof(token);

      
      
     // Serial.print("\n avt integerPart :"); Serial.print(integerPart);
      //Serial.print(" compteur :"); Serial.print(compteur);
     
      if (compteur == integerPart)
        {
          Serial.print(floatPart);
          Serial.print(",");
        }
      else 
      {
        
        if(compteur!=1)
        {
          Serial.print("x");
          Serial.print(",");
        }
        compteur=integerPart;
        
      }
      
      if (buf[len - 1] == '$')
        {
          Serial.print("$\n");
          compteur=0;
        }
        compteur++;
      break;
    }
  }
  else
  {
 
    if (millis() >= Time_ms + 1500)
    {  
      Serial.println("_");
      Time_ms = millis();
    }
  }
}