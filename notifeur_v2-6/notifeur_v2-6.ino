
//**************************
// **** i-Notif'Heure
// Hotloge / notification
// OTA + Interface Web + Connecté
// Gestion Zone + enregistrement eeprom
//  Juin 2018
// Byfeel
// *************************

String Ver="2.631";
#define NTPSERVER "pool.ntp.org"     // Serveur NTP
#define ACTIVBOUTON true               // Si bouton installé
#define ACTIVCAPTLUM true               // Si capteur luminosité installé
#define  BUF_SIZE  60                   // Taille max des notification ( nb de caractéres max )

#define NOMMODULE "NotifHeure_Salon"      // nom module

// Options horloge à définir avant programmation
boolean AutoIn=true;      // Auto / manu intensite
boolean TimeOn=true;      // ON - off Affichage horloge
boolean DisSec=false;    // on-off secondesboolean Alert=false;  

#define DEBUG false

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
//****** Serveur WEB
#include <ESP8266WebServer.h>
//****** Client WEB
#include <ESP8266HTTPClient.h>
// WIFI Manager , afin de gerer la connexion au WIFI de façon plus intuitive
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
// ***** OTA
///includes necessaires au fonctionnement de l'OTA :
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// **** Bibliotheque Matrice LED
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Parola_Fonts_data.h"
#include "Font_Byfeel.h"
// **** Temps
// librairie temps
#include <TimeLib.h>
#include <NtpClientLib.h>
//********** eeprom
#include <EEPROM.h>
//*************************
#if (ACTIVBOUTON)
//librairies click bouton
#include <ClickButton.h>
#endif
// ******************************//


// Definir le nombre de module
//la taille de la zone time si plus de deux modules
// et les PINS ou sont branches les modules
// ***************************************
// ***** Doit etre verifié avant chaque compilation
//**********************************************
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8   // nombre de matrice    
#define MAX_ZONES   1   // 1 ou 2 zones
#if (MAX_ZONES == 1 ) 
#define ZONE_TIME (MAX_DEVICES)
#else
#define ZONE_TIME  4    // taille de l'horloge Si Zone = 2
#endif
#define ZONE_MSG (MAX_DEVICES-ZONE_TIME)
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6

// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Si probleme avec detection auto SPI 
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define SPEED_TIME  30
#define PAUSE_TIME  1000

//**********************//
//interaction Jeedom
#define JEEDOM true                  // Activation interaction avec Jeedom ( veuillez renseigner tous les champs )
String ApiKey   = "LV2Vtr5jjyieep7XmcfXm3YXi9V2EYXq"; // cle API Jeedom  ex :mVuQikcXm620321DMYZiCsLj0wamT0HsCcrBCuXRxntJDO1kn
String IPJeedom = "192.168.8.120";
String PortJeedom = "80";
// ID equipement a modifier     
 //id SDJ
//String idHor  = "10821";
//String idLum ="10822";
//String idSec = "10823";
//String idAuto ="10834";
// id salon
String idHor  = "10844";
String idLum ="10839";
String idSec = "10848";
String idAuto ="10842";



//base URL jeedom Virtuel

String BaseUrlJeedom ="http://"+IPJeedom+":"+PortJeedom+"/core/api/jeeApi.php?apikey=" + ApiKey + "&type=virtual&id="; 


//**************************
//**** Varaible & service ***
// *************************

ESP8266WebServer server(80);         // serveur WEB sur port 80

#if (JEEDOM) 
HTTPClient http; // init client WEB
#endif

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



//variable systemes
char Notif[BUF_SIZE];
char Horloge[BUF_SIZE];
boolean Alert=false;
byte Intensite=5;
byte BkIntensite;
const long interval = 20000;  // interval pour mesure luminosite réglé sur 20 s
unsigned long previousMillis=0 ;
byte clic=0;
byte clicstate=1;
String message="";
String type="";
int httpCode=200;
byte NTPcount=0;
byte valEPROM;

//******** fx *******
textEffect_t  effect[] =
{
  PA_RANDOM,
  PA_PRINT,
  PA_SCAN_HORIZ,
  PA_SCROLL_LEFT,
  PA_NO_EFFECT,
  PA_WIPE,
  PA_SCAN_VERTX,
  PA_SCROLL_UP_LEFT,
  PA_SCROLL_UP,
  PA_FADE,
  PA_OPENING_CURSOR,
  PA_GROW_UP,
  PA_SCROLL_UP_RIGHT,
  PA_BLINDS,
  PA_SPRITE,
  PA_CLOSING,
  PA_GROW_DOWN,
  PA_SCAN_VERT,
  PA_SCROLL_DOWN_LEFT,
  PA_WIPE_CURSOR,
  PA_SCAN_HORIZX,
  PA_DISSOLVE,
  PA_MESH,
  PA_OPENING,
  PA_CLOSING_CURSOR,
  PA_SCROLL_DOWN_RIGHT,
  PA_SCROLL_RIGHT,
  PA_SLICE,
  PA_SCROLL_DOWN,
};

byte fx_in=3;
byte fx_out=3;
bool fx_center;


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
   case 0x82: if (ascii==0xAC )   c = 0x80; // Euro symbol special case

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
    //if (ntpEvent == noResponse) P.print ("Serveur NTP injoignable");
  } else {
    // dateNum2 = datenum(dateStr2,'dd-mmm-yyyy HH:MM:SS');
     if ( NTPcount>14 ) {
             NTP.setInterval (1200);
            }
         else ++NTPcount;
  }
  
}

// **************************************

#if (ACTIVCAPTLUM)
// fonction reglage auto luminosite
void luminosite() {
  byte bkint = Intensite;
  sensorValue = analogRead(A0); // read analog input pin 0
  Intensite =round((sensorValue*1.5)/100);
  if (Intensite < 1 ) Intensite = 0 ;
  if (Intensite > 15 ) Intensite = MAX_INTENSITY;
  if (bkint != Intensite )  ToJeedom( idLum ,Intensite);
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
            if (server.arg("INT") !="") Intensite=server.arg("INT").toInt();
      ToJeedom(idLum , Intensite);
      Option(&AutoIn,false);
    } else if ( LUM == "auto") 
        { 
          Option(&AutoIn,true);
          }
    
  if ( HOR =="off" ) Option(&TimeOn,false);
  else if ( HOR =="on")  Option(&TimeOn,true);
  
  if ( SEC == "on" ) Option(&DisSec,true);
  else if ( SEC == "off" ) Option(&DisSec,false);
  
  if ( server.arg("RAZ") =="1" ) {
     P.print("reboot");
     delay(3000);
     ESP.reset();
     
  }
  server.send ( 200, "text/html", getPage() );
}

// *****************************
// ANIMATION
// SPRITE DEFINITION (PAC MAn )
// *****************************

const uint8_t F_PMAN1 = 6;
 const uint8_t W_PMAN1 = 8;
static  uint8_t pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
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


// ***************************
// Fonction gestion des Options
// TimeOn , DisSec / AutoIn
//*****************************

void Option(boolean *Poption, boolean valeur) {
String *Pid = NULL ;
int value=valeur;
// afectation de la valeur à l'option ( via pointeur )
*Poption = valeur;
 byte UpVal = valEPROM;
           if ( Poption == &DisSec  ) {
           Pid = &idSec ;
            bitWrite(valEPROM, 4, valeur);
           }
          else if ( Poption == &AutoIn )  {
           Pid = &idAuto ;
           if (!bitRead(UpVal,6) && valeur ) NotifMsg("Auto",Intensite,true);
            bitWrite(valEPROM, 6, valeur);
               if ( !valeur ) {  
                    if ( Intensite == 0 ) message="Min";
                     else if ( Intensite == 15 ) message="Max";
                    else message="LUM :"+String(Intensite);
                    NotifMsg(message,Intensite,true);
                    valEPROM=valEPROM&240;  // masque sur les 4 derniers bits
                    valEPROM+=Intensite;
                if (DEBUG) {
                  Serial.println(" Menu Option");
                  Serial.println("valeur enregistrement EPROM " + String(valEPROM) );
                }
               }
           }
           else if ( Poption == &TimeOn  )  {
                   Pid = &idHor ;
                    bitWrite(valEPROM, 5, valeur);
                    if (!valeur) 
                    {
                      NotifMsg("OFF",Intensite,true);
                    }
           }
          if (UpVal != valEPROM ) {
            // enregistre en EEprom les options si changement
           bitWrite(valEPROM, 7, true);
           EEPROM.write(100,valEPROM);
           EEPROM.commit();
          }
   // envoie a jeedom des modifications effectués en fonction de l 'option
   ToJeedom(*Pid,value);
}

void NotifMsg(String Msg,byte lum,boolean Info)
  {
     P.setFont(0,ExtASCII);
     Alert=true;
     Msg.toCharArray(Notif,BUF_SIZE);
  if (Info) {
    //P.displayZoneText(0,Notif,PA_CENTER,SPEED_TIME, PAUSE_TIME,PA_PRINT,PA_NO_EFFECT);
   fx_center=true;
    fx_in=1;
    fx_out=4;
    }
    else 
    { 
      P.setTextAlignment(0,PA_LEFT) ;
     // P.displayZoneText(0, Notif, PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
     fx_in=3;
     fx_out=3;
     }
    
  }

//***********************
// Boucle SETUP
// *********************
void setup(void)
{
 //init demmarrage
P.begin();
if (DEBUG) {
Serial.begin(9600);
}
EEPROM.begin(512);
P.print("Start ...");

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

P.print( "OTA ...");
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
    P.begin();
    P.setFont(0,ExtASCII);
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
  while (WiFi.status() != WL_CONNECTED) {
  }
  // on affiche l'adresse IP attribuee pour le serveur DSN et on affecte le nom reseau
   WiFi.hostname(NOMMODULE);
  //*************************
  
  // on definit les points d'entree (les URL a saisir dans le navigateur web) : Racine du site est géré par la fonction handleRoot
  server.on("/", handleRoot);

 server.on("/Notification", [](){
    // on recupere les parametre dans l'url dans la partie /Notification?msg="notification a affiocher"&type="PAC"
     if ( server.hasArg("msg")) {
      BkIntensite=Intensite;
       message=server.arg("msg");
        if (message ) {
       // message.toCharArray(Notif,BUF_SIZE);
        if (server.hasArg("intnotif") && server.arg("intnotif").length() > 0 ) {
          Intensite = server.arg("intnotif").toInt();
          if (Intensite < 1 ) Intensite = 0 ;
          if (Intensite > 14 ) Intensite = MAX_INTENSITY;
          P.setIntensity(Intensite); // intensité pour les notifs si pas de valeur Intensité par defaut
        } 
        if (server.arg("type")) {
              type=server.arg("type");
               }

    // on repond au client
         NotifMsg(message,Intensite,false);
    server.send(200, "text/plain", "message :" + message + " & Animation : "+server.arg("type") + " & Intensite : "+server.arg("intnotif"));
        }
     }
    });
  

  // on demarre le serveur web 
  server.begin();

//******* Service NTP ********
    NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });

P.print("init");    
// Démarrage du processus NTP  
NTP.begin(NTPSERVER, timeZone, DLS);
// Interval de synchronisation en seconde , 10 s au depart pour forcer une mise a jour rapide
NTP.setInterval (10);

//************************************
// init systéme - lecture Eeprom si existe et envoie info à jeedom
valEPROM = EEPROM.read(100);
if (bitRead(valEPROM, 7))  // Si Enregistrement existe
      {
    DisSec=bitRead(valEPROM, 4);
    TimeOn=bitRead(valEPROM, 5);
    AutoIn=bitRead(valEPROM, 6);
    if (!AutoIn) Intensite=valEPROM&15;  // masque sur les 4 derniers bits , pour recuperer valeur intensite mode manuel
    if (DEBUG) {
      Serial.println(" Valeur lu en Memoire : ");
      Serial.println(" valeur intensite : "+String(Intensite));
    }
  }

ToJeedom(idSec,DisSec);
ToJeedom(idHor,TimeOn);
ToJeedom(idAuto,AutoIn);
// *********************************

//************ Initialisation des Zones
 P.begin(MAX_ZONES);
  P.setSpriteData(pacman1, W_PMAN1, F_PMAN1,pacman2, W_PMAN2, F_PMAN2);   // chargement animation en memoire
  P.setInvert(false);
  
 
  #if (MAX_ZONES >1 && ZONE_MSG >0 )
  P.setZone(0, 0, (ZONE_MSG-1));
  P.setZone(1, ZONE_MSG,(MAX_DEVICES -1));
  //P.setZone(1, 1,3);
  #else
  P.setZone(0, 0, MAX_DEVICES);
  #endif


P.displayZoneText(0, "", PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
if (MAX_ZONES > 1 ) P.displayZoneText(1, Horloge, PA_CENTER, 0,0, PA_PRINT, PA_NO_EFFECT);
  
// Fin de Setup
  message = " Systeme OK - Adresse ip : ";
  message += WiFi.localIP().toString();
  //message.toCharArray(Notif,BUF_SIZE);
 //Alert=true;
 NotifMsg(message,Intensite,false);
  
}


//***********************************
//***********************************
//********* Boucle loop *************
//***********************************
//**********************************

void loop(void)
{
 static uint32_t lastTime = 0; // millis() memory
 static bool flasher = false;  // seconds passing flasher

// ******** Gestion luminosite

  // Si Horloge activé
  if (TimeOn) {
  // Si gestion Auto luminositée activé
  if (AutoIn) {
     #if (ACTIVCAPTLUM)
  if( millis() - previousMillis >= interval) {
    previousMillis = millis();
  luminosite();
  }
    #endif
  }
  else {
  P.setIntensity(Intensite);
  }
  }
    if (DEBUG) Serial.print("valeur luminosité "+ String(Intensite));

// ********** Fin gestion luminosite

// ***********  Gestion bouton
 #if (ACTIVBOUTON)
  // etat bouton
    boutonClick.Update();
  // Sauvegarde de la valeur dans la variable click
  if (boutonClick.clicks != 0) clic = boutonClick.clicks;
  if (DEBUG  && clic ) Serial.print("valeur clic :  "+ String(clic));
  switch (clic) {
    case 1: // double clic - Affiche ou non les secondes
          Option(&DisSec,!DisSec);
          clic=0;
      break;
      case 2: // double clic - ON / OFF horloge
            Option(&TimeOn,!TimeOn);
            clic=0;
      break;
      case 3: // Simple clic - boucle : Manuel intensite à 0 / Manu Intensite MAX / Auto
        if (clicstate==1) {
          Intensite = 0; 
          Option(&AutoIn,false);    
          }
          if (clicstate==2) {
            Intensite = 15;
            Option(&AutoIn,false);
          }
          if (clicstate==3) { 
             Option(&AutoIn,true);
            clicstate=0;
          }
          ToJeedom(idLum , Intensite);
          P.setIntensity(Intensite);
          ++clicstate;
          clic=0;
          break;
       case 255: // simple clic mais long sur dernier  - diminue
      --Intensite;
      if (Intensite<0) Intensite=0;
      NotifMsg("LUM :"+String(Intensite),Intensite,true);
      clic=0;
      break;
       case 254: // double clic mais long sur dernier  - Augmente
       ++Intensite;
      if (Intensite>15) Intensite=MAX_INTENSITY;
      NotifMsg("LUM :"+String(Intensite),Intensite,true);
      clic=0;
      break;
    default:
      // default is optional
      break;  
  }
#endif
// ******** Fin gestion bouton


 
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

//********** flasher
if (millis() - lastTime >= 1000)
  {
    lastTime = millis();
    flasher = !flasher;
  }


if (P.displayAnimate())
{

    if (Alert) {
      if (P.getZoneStatus(0))
      {
      P.setFont(0,ExtASCII);   // chargement caractere etendue
       // Type de demande 
    if (type=="PAC" ) {    // animation PAC MAn
      P.displayZoneText(0,"Notif", PA_LEFT, 40, 1, PA_SPRITE, PA_SPRITE);
      type="";
    }
    else if (type=="BLINDS" ) {  // animation de type volet
       P.displayZoneText(0,"", PA_CENTER, 40, 1, PA_BLINDS, PA_BLINDS);
      type="";
    }
     else if (type=="OPENING" ) {  // animation Ouverture
       P.displayZoneText(0,"Notif", PA_CENTER, 60, 100, PA_OPENING_CURSOR, PA_OPENING_CURSOR);
      type="";
    }
    else {
      P.displayClear(0);
      P.displayZoneText(0, Notif, PA_LEFT,SPEED_TIME, PAUSE_TIME, effect[fx_in], effect[fx_out]);
       if (fx_center) { 
        P.setTextAlignment(0,PA_CENTER);
       fx_center=false;
       }
       Intensite=BkIntensite; // reviens a intensite avant notif
      Alert=false;
    }

    }
    }
    else {
    if (MAX_ZONES == 1) { 
                          P.setFont(0, numeric7Seg_Byfeel); 
                          digitalClockDisplay(Notif,flasher);
                          P.displayZoneText(0, Notif, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    }
    }

  
 if (MAX_ZONES > 1) {
                         if (P.getZoneStatus(0)) P.displayClear(0);
                         P.setFont(1, numeric7Seg_Byfeel);
                         digitalClockDisplay(Horloge,flasher);
                         P.displayZoneText(1, Horloge, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
                
 }
  
  // Check for individual zone completion. Note that we check the status of the zone rather than use the
  // return status of the displayAnimate() method as the other unused zones are completed, but we
  // still need to call it to run the animations.
  //while (!P.getZoneStatus(MAX_ZONES-1))

}  // fin displayanimate

}
//********************** fin boucle


// Fonction envoie requete vers Jeedom
void ToJeedom( String id , byte valeur){
  #if (JEEDOM)  
        String url;
        String value;
        if (id=="idSec" || id=="idHor") {
          if (valeur==0) value="off";
          else value="on";
        } 
        else if ( id=="idAuto" ) {
            if (valeur==0) value="manu";
          else value="auto";
        }
        else {
          value=valeur;
        }

        url = BaseUrlJeedom;
        url +=id; 
        url +="&value="+value;
  http.begin(url);
  httpCode = http.GET();
  http.end();
  #endif
}

// Fonction clockformat ( ajoute les zeros devant les unités )
void digitalClockDisplay(char *heure,bool flash)
    {
   // Si affichage de l'heure      
  if (TimeOn) { 
     P.setIntensity(Intensite);
  if ( DisSec ) 
  {  // si affichage des Secondes  00:00 (secondes en petit )
    char secondes[2];  // pour affichage des secondes
   sprintf(secondes,"%02d",second());
    secondes[0]=secondes[0]+23;  // permutation codes AScci 71 à 80
    secondes[1]=secondes[1]+23;
  if (DEBUG) { 
      Serial.println("debug des secondes : ");
      Serial.println(second());
      Serial.println(secondes[0]);
      Serial.println(secondes[1]);
  } 
    sprintf(heure,"%02d:%02d %c%c",hour(),minute(),secondes[0],secondes[1]);
    //sprintf(heure,"%02d%c%02d%c%c",hour(),(flash ? ':' : ' '),minute(),secondes[0],secondes[1]);
  }
  // Affichage 00:00  avec : Clignottant ( flasher )
   else sprintf(heure,"%02d%c%02d",hour(),(flash ? ':' : ' '),minute());
  }
  else  // sinon point clignottant uniquement
    {
       P.setIntensity(0);
      sprintf(heure,"%c",(flash ? '.' : ' '));
    }
  }



  //************************************
// Page WEB Accueil
// ***********************************
String getPage(){
  char ddj[30];
  sprintf(ddj,"%02d:%02d:%02d - Date : %02d/%02d/%4d",hour(),minute(),second(),day(),month(),year());
  String page = "<html lang=fr-FR><head><meta http-equiv='content-type' content='text/html;charset=ISO-8859-15' /><meta http-equiv='refresh' content='120'/>";
  page += "<title>i-Notif'Heure - Byfeel.info</title>";
  page += "<style> table.steelBlueCols { border: 3px solid #555555;background-color: #555555;width: 95%;text-align: left;border-collapse: collapse;}";
  page +="table.steelBlueCols td, table.steelBlueCols th { border: 1px solid #555555;padding: 4px 9px;}";
  page +="table.steelBlueCols tbody td {font-size: 1em;font-weight: bold;color: #FFFFFF;} table.steelBlueCols td:nth-child(even) {background: #398AA4;}";
  page +="fieldset { padding:0 20px 20px 20px; margin-bottom:10px;border:1px solid #398AA4;width:95%;} div.bloc {float:left;width:450px}";
  //page +="table.steelBlueCols tfoot .links a{display: inline-block;background: #FFFFFF;color: #398AA4;padding: 2px 8px;border-radius: 5px;"
  page +="}</style>";
  page += "</head><body><h1><span style='background-color: #398AA4; color: #ffffff; padding: 0 5px;'>i-Notif\'Heure</span></h1>";
  page += "<h3><span style='background-color: #398AA4; color: #ffffff; padding: 0 5px;'> Heure : ";
  page += ddj;
  page += "</span></h3>";
   page += "<div class='bloc'><form action='/Notification' method='GET' accept-charset='ISO-8859-1'>";
  page +="<fieldset><legend>Envoyer une Notification :</legend>";
  page +="<label for='msg'>Message </label><br />";
  page += "<INPUT type='text' name='msg' id='msg'maxlength='59' style='width:400px;' placeholder='Votre Message - "+String(BUF_SIZE) +" caracteres max -'/>";
   page +="<label for='type'>type : </label>";
   page +="<select name='type' id='type'>";
   page +="<option value=''>Defaut</option>";
  page +="<option value='PAC'>PAC MAN</option>";
  page +="<option value='BLINDS'>BLINDS</option>";
  page +="<option value='OPENING'>Opening</option>";
   page +="</select><br />";
  page +="<label for='intnotif'>Intensite : </label><br />";
  page += "<p><input id='sliderINT' type='range' min='0' max='15' step='1' name='intnotif' list='tickmarks' />";
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

