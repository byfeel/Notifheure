
//**************************
// Hotloge / notification
// OTA + Interface Web + Connecté
//  Mai 2018
// Byfeel
// *************************

#define Ver 2.13
#define NOMMODULE "ESP-Horloge_nom"   // nom module
#define NTPSERVER "fr.pool.ntp.org"         // Serveur NTP
#define ACTIVBOUTON true              // Si bouton installé
#define ACTIVCAPTLUM true              // Si capteur luminosité installé
#define  BUF_SIZE  60                    // Taille max des notification ( nb de caractéres max )

// Parametrage matrice  ( Pin Arduino ou est branché la matrice )
#define MAX_DEVICES 8 // ( nombre de matr ice )
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6

// ******************************//
//******** Bibliotheque *********//
//******************************//
//***** Gestion reseau
#include <EthernetClient.h>
#include <Ethernet.h>
#include <Dhcp.h>
#include <EthernetServer.h>
#include <Dns.h>
#include <EthernetUdp.h>
//****** Gestion WIFI
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// WIFI Manager , afin de gerer la connexion au WIFI de façon plus intuitive
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
// ***** OTA
///includes necessaires au fonctionnement de l'OTA :
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// **** Matrice
// gestion de l'affichage matricielle
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "Parola_Fonts_data.h"
// **** Temps
// librairie temps
#include <TimeLib.h>
#include <NtpClientLib.h>
#if (ACTIVBOUTON)
//librairies click bouton
#include <ClickButton.h>
#endif

//**************************
//**** Varaible & service ***
// *************************

ESP8266WebServer server(80);         // serveur WEB sur port 80

// definition du numero de LED interne 
#define led 2 // led built IN

// Bouton
#if (ACTIVBOUTON)
#define bouton  0 // GPIO0 ( pullup integré - D8 pour R1 ou D3 pour R2 )
ClickButton boutonClick(bouton, LOW, CLICKBTN_PULLUP);
#endif

// variable pour stocker la valeur du photoresistor
#if (ACTIVCAPTLUM) 
int sensorValue;
#endif


// initialisation de la matrice
MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES); 

//variable systemes
char Notif[BUF_SIZE];
boolean DisSec = true;     // on-off secondes
boolean Alert=false;       
boolean AutoIn=true;      // Auto / manu intensite
boolean TimeOn=true;      // ON - off Affichage horloge
int Intensite=5;
const long interval = 10000;  // interval pour mesure luminosite réglé sur 5 s
unsigned long previousMillis=0 ;
int clic=0;
int clicstate=1;
String message="";
String type="";

//*****************************************
// UTF8 - Ascii etendu
//*****************************************
uint8_t utf8Ascii(uint8_t ascii)
// Convert a single Character from UTF8 to Extended ASCII according to ISO 8859-1,
// also called ISO Latin-1. Codes 128-159 contain the Microsoft Windows Latin-1
// extended characters:
// - codes 0..127 are identical in ASCII and UTF-8
// - codes 160..191 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC2 then second byte identical to the extended ASCII code.
// - codes 192..255 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC3 then second byte differs only in the first two bits to extended ASCII code.
// - codes 128..159 in Windows-1252 are different, but usually only the €-symbol will be needed from this range.
//                 + The euro symbol is 0x80 in Windows-1252, 0xa4 in ISO-8859-15, and 0xe2 0x82 0xac in UTF-8.
//
// Modified from original code at http://playground.arduino.cc/Main/Utf8ascii
// Extended ASCII encoding should match the characters at http://www.ascii-code.com/
//
// Return "0" if a byte has to be ignored.
{
  static uint8_t cPrev;
  uint8_t c = '\0';

  if (ascii < 0x7f)   // Standard ASCII-set 0..0x7F, no conversion
  {
    cPrev = '\0';
    c = ascii;
  }
  else
  {
    switch (cPrev)  // Conversion depending on preceding UTF8-character
    {
    case 0xC2: c = ascii;  break;
    case 0xC3: c = ascii | 0xC0;  break;
    case 0x82: if (ascii==0xAC) c = 0x80; // Euro symbol special case
    }
    cPrev = ascii;   // save last char
  }
  return(c);
}

void utf8Ascii(char* s)
// In place conversion UTF-8 string to Extended ASCII
// The extended ASCII string is always shorter.
{
  uint8_t c, k = 0;
  char *cp = s;

  while (*s != '\0')
  {
    c = utf8Ascii(*s++);
    if (c != '\0')
      *cp++ = c;
  }
  *cp = '\0';   // terminate the new string
}

//*****************************************

// **************************
// Parametre NTP
// Serveur NTP
// **************************

// const char ntpserver[] = "pool.ntp.org";
// TimeZone
int8_t timeZone = 1;
// Le fuseau utilise les horaires été / hiver
bool DLS = true;
boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event


void configModeCallback (WiFiManager *myWiFiManager) {
  P.print("Choisir AP..");
  delay(3000);
  P.print(WiFi.softAPIP().toString());
    delay(3000);
    P.print(myWiFiManager->getConfigPortalSSID());
  //Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  //Serial.println(myWiFiManager->getConfigPortalSSID());
}


//void processNtpEvent (NTPSyncEvent_t ntpEvent) {
 void processSyncEvent (NTPSyncEvent_t ntpEvent) {
  if (ntpEvent) {
    if (ntpEvent == noResponse)
      P.print ("Serveur NTP injoignable");
    else if (ntpEvent == invalidAddress)
      P.print ("Adresse du serveur NTP invalide");
  } else {
     if ( year() != 1970 or year() != 2036 ) {
             NTP.setInterval (1200);
            }
    
  }
  
}
// **************************************


#if (ACTIVCAPTLUM)
// fonction reglage auto luminosite
void luminosite() {
  sensorValue = analogRead(A0); // read analog input pin 0
  Intensite =round((sensorValue*1.5)/100);
  if (Intensite < 1 ) Intensite = 0 ;
  if (Intensite > 15 ) Intensite = MAX_INTENSITY;
}
#endif

// ******************
// Gestion page WEB
//******************

void handleRoot(){ 
  if ( server.hasArg("SEC") || server.hasArg("LUM") || server.hasArg("HOR")) {
    handleSubmit();
  } 
  else {
    server.send ( 200, "text/html", getPage() );
  }  
}
 
void handleSubmit() {
  String SEC = server.arg("SEC");
  String LUM = server.arg("LUM");
  String HOR = server.arg("HOR");
  if ( LUM =="manu" ) {
            AutoIn=false;
            Intensite=server.arg("INT").toInt();
    } else if ( LUM == "auto") { AutoIn=true;}
    
  if ( HOR =="off" ) {
          TimeOn=false;
      } else if ( HOR =="on")  TimeOn=true;
  
  if ( SEC == "on" ) {
    DisSec=true;
    server.send ( 200, "text/html", getPage() );
  } else if ( SEC == "off" ) {
    DisSec=false;
    server.send ( 200, "text/html", getPage() );
  } else {
   server.send ( 200, "text/html", getPage() );
  }
  
  if ( server.arg("RAZ") =="1" ) {
     P.print("reboot");
     delay(3000);
     ESP.reset();
     
  }
}

// *****************************
// ANIMATION
// SPRITE DEFINITION (PAC MAn )
// *****************************

const uint8_t F_PMAN1 = 6;
 const uint8_t W_PMAN1 = 8;
static uint8_t  pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
static uint8_t pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
};

//***********************

//***********************
// Boucle SETUP
// *********************

void setup() {
   //demarrage Display
   P.begin();
   P.setFont(ExtASCII);   // chargement caractere etendue
   P.setSpriteData(pacman1, W_PMAN1, F_PMAN1,pacman2, W_PMAN2, F_PMAN2);   // chargement animation en memoire
   
    //******** WiFiManager ************
    WiFiManager wifiManager;

     //Si besoin de fixer une adresse IP
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    
    //Forcer à effacer les donnees WIFI dans l'eprom , permet de changer d'AP à chaque demmarrage ou effacer les infos d'une AP dans la memoire ( a valider , lors du premier lancement  )
    //wifiManager.resetSettings();
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
     wifiManager.setAPCallback(configModeCallback);
    
    //Recupere les identifiants   ssid et Mot de passe dans l'eprom  et essayes de se connecter
    //Si pas de connexion possible , il demarre un nouveau point d'accés avec comme nom , celui definit dans la commande autoconnect ( ici : AutoconnectAP )
    // wifiManager.autoConnect("AutoConnectAP");
    //Si rien indiqué le nom par defaut est  ESP + ChipID
    //wifiManager.autoConnect();
      if(!wifiManager.autoConnect("AP_ESP")) {
              P.print("erreur AP");
              delay(3000);
              //reset and try again, or maybe put it to deep sleep
              ESP.reset();
              delay(5000);
        } 
    
// ****** Fin config WIFI Manager ************



 //******* OTA ***************
// Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(NOMMODULE);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    digitalWrite(led, HIGH); // allume led au debut du transfert
      P.print("Update ..."); 
  });
  ArduinoOTA.onEnd([]() {
     P.print("Reboot ..."); 
    digitalWrite(led, LOW); // eteint a la fin de la mise a jour
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (MAX_DEVICES<5) sprintf(Notif,"Up %u%%", (progress / (total / 100)));
    else sprintf(Notif,"Upload  %u%%", (progress / (total / 100)));
     P.print(Notif); 
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();
  //********* Fin OTA ***************
   
 

  // on attend d'etre connecte au WiFi avant de continuer
  P.print("Wait ..."); 
  while (WiFi.status() != WL_CONNECTED) {
  }
  // on affiche l'adresse IP attribuee pour le serveur DSN et on affecte le nom reseau
   WiFi.hostname(NOMMODULE);
  P.print("Wifi Ready ...");
  delay(400);
  P.print("");
  //*************************
  
  // on definit les points d'entree (les URL a saisir dans le navigateur web) : Racine du site est géré par la fonction handleRoot
  server.on("/", handleRoot);

 server.on("/Notification", [](){
    // on recupere les parametre dans l'url dans la partie /Notification?msg="notification a affiocher"&type="PAC"
     if ( server.hasArg("msg")) {
        message=server.arg("msg");
        message.toCharArray(Notif,BUF_SIZE);
        if (server.arg("type")) {
              type=server.arg("type");
              P.setIntensity(MAX_INTENSITY); // intensité au max pour les notifications
               }
     Alert=true;
     P.print("");
    // on repond au client
    server.send(200, "text/plain", "message :" + message + " & Animation : "+server.arg("type"));
     }
    });
  

  // on demarre le serveur web 
  server.begin();


    //******* Service NTP ********
    NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });

    
// Démarrage du processus NTP  
NTP.begin(NTPSERVER, timeZone, DLS);
// Interval de synchronisation en seconde , 10 s au depart pour forcer une mise a jour rapide
NTP.setInterval (10);

  
  // Parametrage led interne pour indiquer fin de demmarrage
  pinMode(led, OUTPUT);     // Initialise la broche "led" comme une sortie 
  digitalWrite(led, LOW);

#if (ACTIVBOUTON)
  // Setting click button : Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
 boutonClick.debounceTime   = 20;   // Debounce timer in ms
  boutonClick.multiclickTime = 250;  // Time limit for multi clicks
 boutonClick.longClickTime  = 1000; // time until "held-down clicks" register
#endif

  message = " Systeme OK - Adresse ip de votre Module : ";
  message += WiFi.localIP().toString();
  message.toCharArray(Notif,BUF_SIZE);
 Alert=true;
}
// ********* fin SETUP


 // ********** Boucle loop *******

void loop() {
  // ***********  Gestion bouton
 #if (ACTIVBOUTON)
  // etat bouton
    boutonClick.Update();
  // Sauvegarde de la valeur dans la variable click
  if (boutonClick.clicks != 0) clic = boutonClick.clicks;

  switch (clic) {
    case 1:  // Simple clic - boucle : Manuel intensite à 0 / Manu Intensite MAX / Auto
        if (clicstate==1) {
          AutoIn=false;
          Intensite = 0; 
          P.print("Min");
          }
          if (clicstate==2) {
            Intensite = 15;
            P.print("Max");
          }
          if (clicstate==3) { 
            AutoIn=true;
            P.print("Auto");
            clicstate=0;
          }
          ++clicstate;
      delay(400);
      clic=0;
      break;
    case 2: // double clic - ON / OFF horloge
      TimeOn =!TimeOn;
      if (TimeOn) P.print("Time On"); else P.print("Time Off");
      delay(400);
      clic=0;
      break;
         case 3: // double clic - Affiche ou non les secondes
      DisSec =!DisSec;
      clic=0;
      break;
       case -1: // simple clic mais long sur dernier  - diminue
      --Intensite;
      if (Intensite<0) Intensite=0;
      P.print(Intensite);
      delay(400);
      clic=0;
      break;
       case -2: // double clic mais long sur dernier  - Augmente
       ++Intensite;
      if (Intensite>15) Intensite=MAX_INTENSITY;
      P.print(Intensite);
      delay(400);
      clic=0;
      break;
    default:
      // default is optional
      break;  
  }
#endif
// ******** Fin gestion bouton

// ******** Gestion luminosite
 #if (ACTIVCAPTLUM)
  // Si gestion Auto luminositée activé
  if (AutoIn) {
  if( millis() - previousMillis >= interval) {
    previousMillis = millis();
  luminosite();
  }
  }
  #endif
// ********** Fin gestion luminosite
  
// ********* Gestion Reseau : Pages WEB , NTP et OTA
// ********* Service NTP
   if (syncEventTriggered) {
        processSyncEvent (ntpEvent);
        syncEventTriggered = false;
    }
    
//  ****** Page WEb : a chaque iteration, la fonction handleClient traite les requetes 
  server.handleClient();

// ******* OTA
// Surveillance des demandes de mise a jour en OTA
  ArduinoOTA.handle();
  
// ******** Fin gestion reseau

// Gestion Affichage
  if (P.displayAnimate())
  {
    // Si une demande de notification est arrivée
  if (Alert) {
    // Type de demande 
    if (type=="PAC" ) {    // animation PAC MAn
      P.displayText("Notif", PA_LEFT, 40, 1, PA_SPRITE, PA_SPRITE);
      type="";
    }
    else if (type=="BLINDS" ) {  // animation de type volet
       P.displayText("", PA_CENTER, 40, 1, PA_BLINDS, PA_BLINDS);
      type="";
    }
     else if (type=="OPENING" ) {  // animation Ouverture
       P.displayText("Notif", PA_CENTER, 60, 100, PA_OPENING_CURSOR, PA_OPENING_CURSOR);
      type="";
    }
    else {
         P.displayText(Notif, PA_LEFT, 40, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        Alert=false;
    }
  }
  // Sinon affichage de l'heure
  else {
      // Affichage heure
      if (TimeOn) {
       P.setIntensity(Intensite);
        digitalClockDisplay().toCharArray(Notif,BUF_SIZE);
       // P.print(clic); //Debug pour afficher valeur 
      P.displayText(Notif, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        } else  
        {
        P.setIntensity(0); 
        P.print(".");  // Affichage d'un point si horloge etaind , peut etre remplaçé par "". Si on ne veut pas d'affichage
        }  
  }
}
}
// ********* Fin boucle



// Fonction clockformat ( ajoute les zeros devant les unités )
String digitalClockDisplay()
    {
          message=printDigits(hour());
          message+=":";
          message+=printDigits(minute());
          if ( DisSec ) { 
            message+=":";
            message+=printDigits(second());
            }
  return message;
  }

String printDigits(int digits)
{
  String Digital;
  // Digital=":";
  if (digits < 10) Digital+="0";
  Digital+=digits;
  return Digital;
}


//************************************
// Page WEB Accueil
// ***********************************
String getPage(){
  String page = "<html lang=fr-FR><head><meta http-equiv='content-type' content='text/html; charset=iso-8859-15' /><meta http-equiv='refresh' content='120'/>";
  page += "<title>Horloge / Notification - Byfeel.info</title>";
  page += "<style> table.steelBlueCols { border: 3px solid #555555;background-color: #555555;width: 95%;text-align: left;border-collapse: collapse;}";
  page +="table.steelBlueCols td, table.steelBlueCols th { border: 1px solid #555555;padding: 4px 9px;}";
  page +="table.steelBlueCols tbody td {font-size: 1em;font-weight: bold;color: #FFFFFF;} table.steelBlueCols td:nth-child(even) {background: #398AA4;}";
  page +="fieldset { padding:0 20px 20px 20px; margin-bottom:10px;border:1px solid #398AA4;width:95%;} div.bloc {float:left;width:450px}";
  //page +="table.steelBlueCols tfoot .links a{display: inline-block;background: #FFFFFF;color: #398AA4;padding: 2px 8px;border-radius: 5px;"
  page +="}</style>";
  page += "</head><body><h1><span style='background-color: #398AA4; color: #ffffff; padding: 0 5px;'>Horloge - Notification</span></h1>";
  page += "<h3><span style='background-color: #398AA4; color: #ffffff; padding: 0 5px;'> Heure : ";
  page += printDigits(hour())+":"+printDigits(minute())+":"+printDigits(second());
  page += " - Date : "+printDigits(day())+"/"+printDigits(month())+"/"+year()+"</span></h3>";
   page += "<div class='bloc'><form action='/Notification' method='GET'>";
  page +="<fieldset><legend>Envoyer une Notification :</legend>";
  page +="<label for='msg'>Message </label><br />";
  page += "<INPUT type='text' name='msg' id='msg'maxlength='59' style='width:400px;' placeholder='Votre Message - 60 caracteres max -'/>";
   page +="<label for='type'>type : </label>";
   page +="<select name='type' id='type'>";
   page +="<option value=''>Defaut</option>";
  page +="<option value='PAC'>PAC MAN</option>";
  page +="<option value='BLINDS'>BLINDS</option>";
page +="<option value='OPENING'>Opening</option>";
  page +="</select>";
  page +="</fieldset>";
  page += "<INPUT type='submit' value='Envoyer Message' /></div>";
  page += "</form></div>";
  page +="<div style='clear:both;'></div>";
  page += "<div class='bloc'><h3>Systeme</h3>";
  page += "<table class='steelBlueCols'><tbody>";
  page += "<tr><td>Version </td><td>";
  page += Ver;
  page +="</td></tr>";
  page += "<tr><td>Mise en Marche </td><td>"+ NTP.getUptimeString() + "</td></tr>";
  page += "<tr><td>Serveur NTP </td><td>";
  page += NTPSERVER;
  page +="</td></tr>";
  page += "<tr><td>Premiere synchro NTP </td><td>"+NTP.getTimeDateString(NTP.getFirstSync())+"</td></tr>";
  page += "<tr><td>Derniere synchro NTP </td><td>"+NTP.getTimeDateString(NTP.getLastNTPSync())+"</td></tr>";
  page +="</tbody></table></div>";
  page += "<div class='bloc'><h3>Resau</h3>";
  page += "<table class='steelBlueCols'><tbody>";
  page += "<tr><td>mac adress : </td><td>";
  page += WiFi.macAddress().c_str();
  page +="</td></tr>";
  page += "<tr><td>IP</td><td>"+ WiFi.localIP().toString() + "</td></tr>";
  page += "<tr><td>Masque </td><td>"+ WiFi.subnetMask().toString() + "</td></tr>";
  page += "<tr><td>Passerelle</td><td>"+ WiFi.gatewayIP().toString() + "</td></tr>";
  page += "<tr><td>DNS primaire</td><td>"+ WiFi.dnsIP().toString() + "</td></tr>";
  page += "<tr><td>DNS secondaire</td><td>"+ WiFi.dnsIP(1).toString() + "</td></tr>";
  page +="</tbody></table></div>";
  page +="<div class='bloc'><h3>Wifi</h3>";
  page += "<table class='steelBlueCols'><tbody>";
  page += "<tr><td>Hostname</td><td>"+ WiFi.hostname() + "</td></tr>";
  page += "<tr><td>SSID</td><td>"+ WiFi.SSID() + "</td></tr>";
  page += "<tr><td>Signal WIFI</td><td>";
  page += WiFi.RSSI();
  page +=" dBm</td></tr>";
  page += "<tr><td>BSSID </td><td>";
  page += WiFi.BSSIDstr().c_str();
  page +="</td></tr>";
  page +="</tbody></table></div>";
  page +="<div style='clear:both;'></div>";
  page += "<div class='bloc'><h3>Parametres :</h3>";
  page += "<form action='/' method='POST'>";
  page +="<fieldset><legend>Affichage des Secondes :</legend>";
  page += "<INPUT type='radio' name='SEC' value='on' id='on'";
  if (DisSec==true) page +=" checked ";
  page += "><label for='on'>ON</label>";
  page +="<INPUT type='radio' name='SEC' value='off' id='off'";
  if (DisSec==false) page +=" checked ";
  page += "><label for='off'>OFF</label></fieldset>";
  page +="<fieldset><legend>Affichage Horloge :</legend>";
  page += "<INPUT type='radio' name='HOR' value='on' id='horon'";
  if (TimeOn==true) page +=" checked ";
  page += "><label for='horon'>ON</label>";
  page +="<INPUT type='radio' name='HOR' value='off' id='horoff'";
  if (TimeOn==false) page +=" checked ";
  page += "><label for='horoff'>OFF</label></fieldset>";
  page +="<fieldset><legend>Gestion de la luminosit&eacute;e ( "; 
  page +=Intensite;
  page +=" ) : </legend>";
  page += "<INPUT type='radio' name='LUM' value='auto' id='auto'";
  if (AutoIn==true) page +=" checked ";
  page += "><label for='auto'>AUTO</label>";
  page +="<INPUT type='radio' name='LUM' value='manu' id='manu'";
  if (AutoIn==false) page +=" checked ";
  page += "><label for='manu'>Manuel</label></P>";
  if (AutoIn==false) {
  page +="<p>Valeur luminosit&eacute;e : ";
  page +=Intensite;
  page +="</p>";
  page += "<p><input id='slider1' type='range' min='0' max='15' step='1' name='INT' list='tickmarks' value='";
  page +=Intensite;
  page +="'/>";
  page +="<datalist id='tickmarks'>";
  page +="<option value='0' label='0'>";
  page +="<option value='1'>";
  page +="<option value='2'>";
  page +="<option value='3'>";
  page +="<option value='4'>";
  page +="<option value='5' label='5'>";
  page +="<option value='6'>";
  page +="<option value='7'>";
  page +="<option value='8'>";
  page +="<option value='9'>";
  page +="<option value='10' label='10'>";
  page +="<option value='11'>";
  page +="<option value='12'>";
  page +="<option value='13'>";
  page +="<option value='14'>";
  page +="<option value='15' label='15'>";
  page +="</datalist></p>";
  };
  page +="</fieldset>";
  page +="<fieldset><legend>Redemarrage Module :</legend>";
  page += "<INPUT type='radio' name='RAZ' value='1' id='reset'";
  page += "><label for='reset'>Redemmarage</label>";
   page += "</fieldset>";
   page +="</fieldset>";
  page += "<INPUT type='submit' value='Actualiser'name='parametres'/></div>";
  page += "</form>";
  page += "<br><br>";
  page += "</body></html>";
  return page;
}

