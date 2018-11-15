
//**************************
// NOTIFEUR
// OTA + Interface Web + API ( box domotique )
// Gestion Zone + Gestion memoire ( Spiffs - eeprom )
// Websockets & Mdns
//  -------------------------
// Copyright Byfeel 2017-2018
// *************************
String Ver="3.1.2b";
// *************************
//**************************
//****** CONFIG SKETCH *****
//**************************
//**************************
// matrix
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4  // nombre de matrice  
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6
// Options ( modifie si different ou laisser valeur par défaut )
// Ne pas supprime
#define PINbouton1 0 // Bouton 1   --- GPIO0 ( pullup integré - D8 pour R1 ou D3 pour R2 )
#define PINbouton2 2 // Bouton 2
#define DHTTYPE DHTesp::DHT22  // si DHT22
//#define DHTTYPE DHTesp::DHT11  // si DHT11
#define dhtpin 16 // GPIO16  egale a D2 sur WEMOS D1R1  ou D0 pour les autres ( a verifier selon esp )
#define LEDPin 5


//***************************
// admin page config
//***************************

const char* www_username = "admin";
const char* www_password = "notif";

 
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
#include <WebSocketsServer.h>

//****** Client WEB
#include <ESP8266HTTPClient.h>
// WIFI Manager , afin de gerer la connexion au WIFI de façon plus intuitive
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
// ***** OTA
///includes necessaires au fonctionnement de l'OTA :
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// **** Bibliotheque Matrice LED
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#include "Parola_Fonts_data.h"
#include "Font_Byfeel.h"
// **** Temps
// librairie temps
#include <TimeLib.h>
#include <NtpClientLib.h>

//********** eeprom / SPIFFS
//#include <EEPROM.h>
#include <FS.h>   //Include SPIFFS
#include <ArduinoJson.h>
//*************************
// Options
//*************************
// bibliotheque temperature
#include <DHTesp.h>


//librairies click bouton
#include <ClickButton.h>
// ******************************//



// Hardware SPI connection
//MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Si probleme avec detection auto SPI 
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


//**************************
//**** Service et Variable  ***
// *************************
// init dht
DHTesp dht;
      

// Structure fichier config
struct Config {
  char HOSTNAME[10];
  char Name[12];
  char NTPSERVER[30];
  int timeZone;                // 
  bool DLS;                   // Le fuseau utilise les horaires été / hiver
  bool DEBUG;
  bool BOUTON_1;
  bool BOUTON_2;
  bool LED;
  bool multiZone;
  bool DispRight;
  byte LZTime;
  int SPEED_TIME;
  int PAUSE_TIME;
  int interval_lux;   // temps en secondes pour mesure luminosité
  int interval_dht;  // temps en secondes pour mesure DHT si present
  int interval_debug;
  int interval_ddj;
  byte btnclic[3][4];
  bool JEEDOM;
  char URL_Update[130];
  char URL_Action1[130];
  char URL_Action2[130];
  char URL_Action3[130];
};

const char *filename = "/config/config.json";  // fichier config
Config config;                         // <- global configuration object

//Init JSON
//StaticJsonBuffer<1600> jsonBuffer; 
DynamicJsonBuffer jsonBuffer(500);
JsonObject& JSONRoot = jsonBuffer.createObject();
JsonObject& JSONSystem = JSONRoot.createNestedObject("system");
JsonObject& JSONOptions = JSONRoot.createNestedObject("Options");
JsonObject& JSONDht = JSONRoot.createNestedObject("dht");

// service WEB
ESP8266WebServer server(80);         // serveur WEB sur port 80
WebSocketsServer webSocket = WebSocketsServer(81);

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
//nom fichier charge
File fsUploadFile;
void handleFileUpload();

HTTPClient http; // init client WEB

// fonction pour boutons
ClickButton bouton1(PINbouton1, LOW, CLICKBTN_PULLUP);
ClickButton bouton2(PINbouton2, LOW, CLICKBTN_PULLUP);

// variable systeme
// -- luminosté
int sensorValue;
boolean PhotoCell=false;
byte Intensite;    // valeur par defaut au premier demmarrage
byte BkIntensite;

//interval
unsigned long interval = 20000;  // interval pour mesure luminosite réglé sur 15 s ( 10 s min )
unsigned long previousMillis=0 ;

unsigned long intervaldebug = 45000;  // interval pourt debug ( 10 s min )
unsigned long previousMillisdebug=0 ;

unsigned long previousMillisdht=0 ;
unsigned long intervaldht= 300000;  // tempo dht 120s ( 2 mn )

unsigned long previousMillisddj=0 ;
unsigned long intervalddj= 600000;  // tempo date ( 10 mn )
 
// - Debug
String InfoDebugBoucle;
String InfoDebugtInit;

// - Options
bool AutoIn;     // Auto / manu intensite
bool TimeOn;    // ON - off Affichage horloge
bool DisSec; 
byte dj=0;
bool msgTemp;
bool raz;

// boutons
byte clic=0;
byte clic2=0;
byte clicstate=1;
byte ledState = 1;

// affichage
#define  BUF_SIZE  60       // Taille max des notification ( nb de caractéres max )
char Notif[BUF_SIZE];
char Horloge[BUF_SIZE];
char txtAnim[20];
boolean Alert=false;
boolean FinMsg=false;
String message="";    
String type="";
String jours[8]={"Erreur","Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi"};
String mois[13]={"Erreur","Janvier","Février","Mars","Avril","Mai","Juin","Juillet","Août","Septembre","Octobre","Novembre","Décembre"};
byte maxdisplay = MAX_DEVICES;
byte NZtime , NZmsg;
bool Fnotif=false;

// variable DHT
bool DHTsensor=false;
  float humidity;
  float temperature;
  float heatIndex;
  float dewPoint;
  byte per;
 
  //String comfortStatus="";     
   char date_stamp[18];
   
//variable systemes
byte valEPROM;
byte adminEPROM;
int httpCode;
byte NTPcount=0;
String JeedomFile="/jeedom.json";
boolean Reboot = false;
boolean UptoBox=false;
bool OTAerreur=false;

//**********************//
//interaction Jeedom
boolean JEEDOM=false;                
String ipBox="x.x.x.x";
String portBox="80";
String ReqUpBox;
String UpUrlBox;
boolean Up=false;
boolean URLOK=false;

//new




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

// ****************************
// ****** Fonctions ***********
//*****************************

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
/*

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event
*/

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



void processSyncEvent (NTPSyncEvent_t ntpEvent) {
  if (ntpEvent) {
    //if (ntpEvent == noResponse) P.print ("Serveur NTP injoignable");
    // else if (ntpEvent == invalidAddress) P.print ("Adresse du serveur NTP invalide");
  } else {

    // dateNum2 = datenum(dateStr2,'dd-mmm-yyyy HH:MM:SS');
     if ( NTPcount>14 ) {
             NTP.setInterval (1200);
            }
         else ++NTPcount;
  }
  
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event


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



// **************************************
// Fonction Notif'heure
// **************************************
void NotifMsg(String Msg,byte lum,bool Info,bool U2A=true)
  {
     P.setFont(NZmsg,ExtASCII);
     Alert=true;
     Msg.toCharArray(Notif,BUF_SIZE);
  if (U2A) utf8Ascii(Notif);
  if (Info) {
    //P.displayZoneText(0,Notif,PA_CENTER,config.SPEED_TIME, config.PAUSE_TIME,PA_PRINT,PA_NO_EFFECT);
   fx_center=true;
    fx_in=1;
    fx_out=4;
    } else { 
      P.setTextAlignment(NZmsg,PA_LEFT) ;
     // P.displayZoneText(0, Notif, PA_LEFT, config.SPEED_TIME, config.PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
     fx_in=3;
     fx_out=3;
     }
  }


// Construction fichier sauvegarde
String JsonConfig(bool NC=false) {
  String configjson;
    // Allocate the memory pool on the stack
  // Don't forget to change the capacity to match your JSON document.
  // Use https://arduinojson.org/assistant/ to compute the capacity.
  StaticJsonBuffer<1600> jsonBuffercfg;

  // Parse the root object
  JsonObject &rootcfg = jsonBuffercfg.createObject();
if (config.DEBUG) Serial.println("Mise a jour valeur Options : SEC="+String(DisSec) +" HOR ="+String(TimeOn)+" LUM="+String(AutoIn));
  // Set the values
  rootcfg["Hostname"] = config.HOSTNAME;
  rootcfg["Name"] = config.Name;
  rootcfg["NTPserver"] = config.NTPSERVER;
  rootcfg["timeZone"] =  config.timeZone;
  rootcfg["DLS"] = config.DLS;             
  rootcfg["DEBUG"] = config.DEBUG;  
  rootcfg["BOUTON1"] = config.BOUTON_1;  
  rootcfg["BOUTON2"] =config.BOUTON_2; 
  rootcfg["LED"] =config.LED; 
  rootcfg["multizone"] = config.multiZone; 
  rootcfg["DispRight"] = config.DispRight;
  rootcfg["LZTime"] = config.LZTime;
  rootcfg["speed_time"] = config.SPEED_TIME;
  rootcfg["pause_time"] =  config.PAUSE_TIME;  
  rootcfg["interval_lux"] = config.interval_lux;
  rootcfg["interval_dht"] =config.interval_dht; 
  rootcfg["interval_debug"] =config.interval_debug; 
  rootcfg["AutoIn"] = AutoIn; 
  rootcfg["TimeOn"] = TimeOn; 
  rootcfg["DisSec"] = DisSec; 
  rootcfg["intensite"]=Intensite;
  rootcfg["ddht"] = msgTemp;
  rootcfg["maxdisplay"] = MAX_DEVICES;
  if (raz) rootcfg["raz"] =raz ; 
JsonArray& btn1 = rootcfg.createNestedArray("btn1");
btn1.add(config.btnclic[1][1]);
btn1.add(config.btnclic[1][2]);
btn1.add(config.btnclic[1][3]);

JsonArray& btn2 = rootcfg.createNestedArray("btn2");
btn2.add(config.btnclic[2][1]);
btn2.add(config.btnclic[2][2]);
btn2.add(config.btnclic[2][3]);
  rootcfg["txtAnim"] =txtAnim;
rootcfg["URL_Update"]=config.URL_Update;
  rootcfg["URL_Action1"]=config.URL_Action1;
  rootcfg["URL_Action2"]=config.URL_Action2;
  rootcfg["URL_Action3"]=config.URL_Action3;
  rootcfg["JEEDOM"] = config.JEEDOM ;

if (NC) {
   Notif_date(date_stamp);
   rootcfg["date"] = date_stamp;
}
rootcfg.printTo(configjson);
return configjson;

}

// Chargement fichier config
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File file = SPIFFS.open(filename, "r");
   if (!file) {
     InfoDebugtInit=InfoDebugtInit+" Fichier config Jeedom absent -";
  }

  size_t size = file.size();
  if (size > 1024) {
     InfoDebugtInit=InfoDebugtInit+" Fichier config trop grand -";
  }

  // allocation buffer pour charger fichier config
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);

  StaticJsonBuffer<1600> jsonBufferConfig;
  JsonObject& rootcfg = jsonBufferConfig.parseObject(buf.get());

  if (!rootcfg.success()) {
     InfoDebugtInit=InfoDebugtInit+" Erreur lecture fichier config - Valeur par defaut ----";
  }

  // Initialisation des variables systémes pour configuration , si non présente valeur par defaut affecté
  strlcpy(config.HOSTNAME, rootcfg["Hostname"] | "Notif",sizeof(config.HOSTNAME));
  strlcpy(config.Name, rootcfg["Name"] | "new",sizeof(config.Name));    
  strlcpy(config.NTPSERVER,rootcfg["NTPserver"] | "pool.ntp.org",sizeof(config.NTPSERVER));  
  maxdisplay =  rootcfg["maxdisplay"] | MAX_DEVICES;  
  config.timeZone = rootcfg["timeZone"] | 1;
  config.DLS = rootcfg["DLS"] | true;              
  config.DEBUG = rootcfg["DEBUG"] | false;  
  config.BOUTON_1 = rootcfg["BOUTON1"] | false;  
  config.BOUTON_2 = rootcfg["BOUTON2"] | false; 
  config.LED = rootcfg["LED"] | false; 
  config.multiZone = rootcfg["multizone"] | false; 
  config.DispRight = rootcfg["DispRight"] | false;
  config.LZTime =  rootcfg["LZTime"] | round(maxdisplay/2);
  config.SPEED_TIME = rootcfg["speed_time"] | 35;
  config.PAUSE_TIME = rootcfg["pause_time"] | 3;  
  config.interval_lux = rootcfg["interval_lux"] | 15; 
  config.interval_dht = rootcfg["interval_dht"] | 10; 
  config.interval_debug = rootcfg["interval_debug"] | 40; 
  JsonArray& btn1 = rootcfg["btn1"];
  config.btnclic[1][1] = btn1[0] | 1;
  config.btnclic[1][2] = btn1[1] | 2;
  config.btnclic[1][3] = btn1[2] | 3;
  JsonArray& btn2 = rootcfg["btn2"];
  config.btnclic[2][1] = btn2[0] | 4;
  config.btnclic[2][2] = btn2[1] | 5;
  config.btnclic[2][3] = btn2[2] | 6;
  AutoIn = rootcfg["AutoIn"] | true; 
  TimeOn = rootcfg["TimeOn"] | true; 
  DisSec = rootcfg["DisSec"] | true; 
  Intensite = rootcfg["intensite"] | 1;
  
  raz = rootcfg["raz"] | false; 
  msgTemp = rootcfg["ddht"] | false;
  dj =  rootcfg["dj"] | 0;
  strlcpy(txtAnim, rootcfg["txtAnim"] | "Notif",sizeof(txtAnim));
  config.JEEDOM = rootcfg["JEEDOM"] | false;
  strlcpy(config.URL_Update, rootcfg["URL_Update"] | "",sizeof(config.URL_Update));
  strlcpy(config.URL_Action1, rootcfg["URL_Action1"] | "",sizeof(config.URL_Action1));
  strlcpy(config.URL_Action2, rootcfg["URL_Action2"] | "",sizeof(config.URL_Action2));
  strlcpy(config.URL_Action3, rootcfg["URL_Action3"] | "",sizeof(config.URL_Action3));

  // Close the file (File's destructor doesn't close the file)
  InfoDebugtInit=InfoDebugtInit+" valeur test : "+String( config.btnclic[2][3]);
  file.close();
}


// sauvegarde fichier config
void saveConfiguration(const char *filename, const Config &config) {
 File file = SPIFFS.open(filename, "w");
  if (!file) {
   if (config.DEBUG)  Serial.println("Impossible d'ecrire fichier de config");
   return;
    }
file.print(JsonConfig());
 // Close the file (File's destructor doesn't close the file)
  file.close();
}



void ToBox(char* Url) {
  String URL=Url;
  if (config.DEBUG) Serial.println("valeur de URL dans tobox : "+URL);
   http.begin(URL);
   httpCode = http.GET();
   http.end();
   //if (httpCode != 200 ) return false;
   //else return true;
}


// fonction reglage auto luminosite
void luminosite() {
  byte bkint = Intensite; // memorise la valeur d'entree
  sensorValue = analogRead(A0); // read analog input pin 0
  Intensite =round((sensorValue*1.5)/100);
  if (Intensite < 1 ) Intensite = 0 ;
  if (Intensite > 15 ) Intensite = MAX_INTENSITY;
  //if (bkint != Intensite )  ToJeedom( idLum ,Intensite);
  //if (bkint != Intensite )  ToJeedom( idLum ,Intensite);
}

void GetTemp() {
  String Statusdht;
  int ModelDHT;
  Statusdht=dht.getStatusString();
  ModelDHT=dht.getModel();
   temperature = dht.getTemperature();
    DHTsensor=true;
    humidity= dht.getHumidity();
   heatIndex = dht.computeHeatIndex(temperature, humidity, false);
   dewPoint = dht.computeDewPoint(temperature,humidity,false);
   //float cr = dht.getComfortRatio(cf,temperature,humidity);
    per = dht.computePerception(temperature, humidity,false);
    Notif_date(date_stamp);

  if (isnan(humidity) || isnan(temperature)) {
    DHTsensor=false;
    temperature = 0;
    humidity= 0;
    heatIndex = 0;
    dewPoint = 0;
    per = 0;
  }


   // Mise a jour des valeurs dht dans JSON
    //DHTsensor
    String Modele;
    switch(ModelDHT) {
      case 1 : Modele="DHT11";
      break;
      case 2 : Modele="DHT22";
      break;
      default : Modele="Inconnu";
      break;
    }
  
  if (config.DEBUG) Serial.println("Valeur Sensor DHT :"+String(DHTsensor));
  JSONDht["T"] = temperature;
  JSONDht["H"] = humidity;
  JSONDht["Hi"] = String(heatIndex);
  JSONDht["P"] = String(dewPoint);
  JSONDht["per"] = per;
  JSONDht["Modele"] =Modele;
  JSONDht["Status"] = dht.getStatusString();
  JSONSystem["dhtsensor"] = DHTsensor;
  JSONDht["dht_date"] = date_stamp;
if (config.DEBUG) Serial.println(" T:" + String(temperature) + "°C  H:" + String(humidity) + "%  I:" + String(heatIndex) + " D:" + String(dewPoint) + "  et per :"+ String(per));
}


// Fonction DEbug
void DebugInfo() {
  Serial.println("*******************************");
  Serial.println("**** Info Debug ***************");
  Serial.println("*******************************");
  Serial.println("Valeur Luminosite : "+String(Intensite)+" Mode Auto :"+AutoIn);
  if (PhotoCell ) Serial.println("Valeur capteur photocell : "+String(sensorValue));
  Serial.println(" Debug boucle : "+InfoDebugBoucle);
  Serial.print(" Valeur ValEprom : "+ String(valEPROM) +" : " );
  Serial.println(valEPROM,BIN);
  Serial.print(" Valeur adminEprom : "+ String(adminEPROM) +" : " );
  Serial.println(adminEPROM,BIN);
 
}


// fonction flashlight ( falsh DEL avant notif )
void flashlight() {
  int F=0;
  for (int i=1;i<8;i++) {
          digitalWrite(LEDPin,F);
          delay(100);
          F=!F;
  }
   digitalWrite(LEDPin,HIGH);
   Fnotif=false;
}


//*************************************************
//************* Gestion du serveur WEB ************
//*************************************************


// Traitement des notifications
void handleNotif(){
String InfoNotif="Erreur";
 BkIntensite=Intensite; 
// on recupere les parametre dans l'url dans la partie /Notification?msg="notification a affiocher"&type="PAC"
     if ( server.hasArg("msg")) {
          message=server.arg("msg");
           if (message!="") {
              if (server.arg("type")) type=server.arg("type");
              if (server.hasArg("intnotif") && server.arg("intnotif").length() > 0 ) 
                {
                   Intensite = server.arg("intnotif").toInt();
                  if (Intensite < 1 ) Intensite = 0 ;
                   if (Intensite > 14 ) Intensite = MAX_INTENSITY;
                 } 
              if (server.arg("flash")=="true" || server.arg("flash")=="1") Fnotif=true;
              if ( server.hasArg("txt"))   { 
                server.arg("txt").toCharArray(txtAnim,sizeof(txtAnim));
                JSONOptions["txtAnim"] = txtAnim;
                saveConfiguration(filename, config);
              }
              P.setIntensity(Intensite); // regle intensite notif
              InfoNotif="ok"; 
              NotifMsg(message,Intensite,false,false); // envoie de la notif pour traitement
              }
              else InfoNotif="vide";         
     }
     server.send(200, "text/plane",InfoNotif);
}

void handleOptions() {
String result="ok";
    if ( server.hasArg("SEC")) { 
        String SEC = server.arg("SEC");
         if ( (SEC == "on" || SEC =="1") && !DisSec ) Option(&DisSec,true);
          else if ( (SEC == "off" || SEC=="0") && DisSec ) Option(&DisSec,false);
            else result="idem";
          }
     else if ( server.hasArg("HOR")) { 
        String HOR = server.arg("HOR");
         if ( (HOR == "on" || HOR=="1") && !TimeOn) Option(&TimeOn,true);
         else if ( (HOR == "off" || HOR=="0") && TimeOn) Option(&TimeOn,false);
         else result="idem";
    }
      else if ( server.hasArg("LUM")) {
          String LUM = server.arg("LUM");
         if ( (LUM == "auto" || LUM == "1") && !AutoIn) Option(&AutoIn,true);
         else if ( (LUM == "manu" || LUM == "0") && AutoIn) Option(&AutoIn,false);
         else result="idem";
          }
     else if ( server.hasArg("INT") && !AutoIn) { 
           Intensite=server.arg("INT").toInt();
            Option(&AutoIn,false);
         } 
              else if ( server.hasArg("LED") && config.LED) { 
                  if (server.arg("LED")=="1" || server.arg("LED")=="on" ) ledState = 0;
                  else ledState=1;
                  digitalWrite(LEDPin,ledState); // On-Off led
                  JSONOptions["LEDstate"] = !ledState;
         }
      else result="Erreur";
  server.send ( 200, "text/html",result);
    webSocket.broadcastTXT("Update");
}


// fonction getInfo
void handleInfo() {
    String InfoSystem;
    // mise a jour infos system
  String UptimeNotif;
  UptimeNotif = NTP.getUptimeString();
  JSONSystem["uptime"] = UptimeNotif;
  JSONSystem["RSSI"] = WiFi.RSSI();
  JSONSystem["LastSynchroNTP"] = NTP.getTimeDateString(NTP.getLastNTPSync());
  
  JSONRoot.printTo(InfoSystem);
  //JSONDht.printTo(InfoSystem);
  if (config.DEBUG) { 
    Serial.println("Requete getinfo");
    Serial.println(InfoSystem);
  }
  server.send(200, "application/json",InfoSystem);
}

// envoie IP module
void handleIP() {
  server.send(200, "text/html",WiFi.localIP().toString());
}


// fonction edition
void handleEdit() {
String result="ok";
//  config.HOSTNAME = server.arg("inputhost") | config.HOSTNAME;
  Serial.println("page config");
  config.DLS=server.arg("dls");
  Serial.println("dls : "+ config.DLS);
 server.send ( 200, "text/html",result);
}



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String InfoJson;
    switch(type) {
        case WStype_DISCONNECTED:
           if (config.DEBUG) Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
        {
                IPAddress ip = webSocket.remoteIP(num);
                if (config.DEBUG)  Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
        // send message to client
        //webSocket.sendTXT(num, "ok");
            break;
        }
        case WStype_TEXT:
        {
           if (config.DEBUG) Serial.printf("[%u] get Text: %s\n", num, payload);
          int c=0,set=0;
          String s((const __FlashStringHelper*) payload);
          c = s.indexOf("DEBUG");
          set = s.indexOf("SetOptions");
          if (config.DEBUG) Serial.println(" chaine paylod : "+s);
          //envoi info systeme
          if ( s=="GetInfo") {
            String UptimeNotif;
            UptimeNotif = NTP.getUptimeString();
            JSONSystem["uptime"] = UptimeNotif;
            JSONSystem["RSSI"] = WiFi.RSSI();
           JSONSystem["LastSynchroNTP"] = NTP.getTimeDateString(NTP.getLastNTPSync());
            JSONRoot.printTo(InfoJson);
            // send message to client
             webSocket.sendTXT(num, InfoJson);
          }
            //envoi liste MDNS
          else if ( s=="MDNS") {
              int n = MDNS.queryService("notifeur", "tcp"); // Send out query for esp tcp services
              String ReponseHTML="";
              if (n == 0) {
                    ReponseHTML += "<a href='#' class='list-group-item list-group-item-action'>Aucun Notif'Heure trouvé sur le réseau</a>";
               } else {
               for (int i = 0; i < n; ++i) {
                 ReponseHTML += "<a href='http://";
                  ReponseHTML += MDNS.IP(i).toString();
                  ReponseHTML +="' class='list-group-item list-group-item-action'>"+String(i+1)+" - ";
                  ReponseHTML += MDNS.hostname(i);
                  ReponseHTML += " ( ";
                  ReponseHTML += MDNS.IP(i).toString();
                  ReponseHTML += " )</a>";
                 }
                 webSocket.sendTXT(num, ReponseHTML);
              }
            
          }
          // envoie fichier de config
          else if ( s=="Config") {
            String reponse=JsonConfig(true);
            webSocket.sendTXT(num,reponse);
          }

          else if ( s=="REBOOT") {
            webSocket.sendTXT(num,"demande de Reboot ok");
            delay(500);
            ESP.restart();
          }

           else if ( s=="RESET") {
           raz=true;
           saveConfiguration(filename, config);
            delay(1000);
            ESP.restart();
          }
          
          
          // Reponse sauvegarde config
           else if ( c > 10 ) {
            DynamicJsonBuffer jsonBuffercfg;
            JsonObject& jsoncfg = jsonBuffercfg.parseObject(s);

           strlcpy(config.HOSTNAME,  jsoncfg["Hostname"],sizeof(config.HOSTNAME));
            strlcpy(config.Name, jsoncfg["Name"] ,sizeof(config.Name));    
            strlcpy(config.NTPSERVER,jsoncfg["NTPserver"],sizeof(config.NTPSERVER));    

             config.timeZone = jsoncfg["timeZone"];
            config.DLS = jsoncfg["DLS"];
            config.DEBUG = jsoncfg["DEBUG"];
            config.BOUTON_1 = jsoncfg["BOUTON1"] ;
            config.BOUTON_2 = jsoncfg["BOUTON2"] ;
             config.LED = jsoncfg["LED"];
             config.multiZone = jsoncfg["multizone"] ;
             config.LZTime = jsoncfg["LZTime"] ;
             config.DispRight = jsoncfg["DispRight"] ;
            config.SPEED_TIME = jsoncfg["speed_time"];
            config.PAUSE_TIME = jsoncfg["pause_time"];
            config.interval_lux = jsoncfg["interval_lux"];
            config.interval_dht = jsoncfg["interval_dht"];
            config.interval_debug = jsoncfg["interval_debug"];
            config.btnclic[1][1] = jsoncfg["btn1"][0];
            config.btnclic[1][2] = jsoncfg["btn1"][1];
            config.btnclic[1][3] = jsoncfg["btn1"][2];
            config.btnclic[2][1] = jsoncfg["btn2"][0];
            config.btnclic[2][2] = jsoncfg["btn2"][1];
            config.btnclic[2][3] = jsoncfg["btn2"][2];

            config.JEEDOM = jsoncfg["JEEDOM"];
            strlcpy(config.URL_Update,jsoncfg["URL_Update"],sizeof(config.URL_Update));
              strlcpy(config.URL_Action1,jsoncfg["URL_Action1"],sizeof(config.URL_Action1));
              strlcpy(config.URL_Action2,jsoncfg["URL_Action2"],sizeof(config.URL_Action2));
              strlcpy(config.URL_Action3,jsoncfg["URL_Action3"],sizeof(config.URL_Action3));
            
             saveConfiguration(filename, config);
            
             
            webSocket.sendTXT(num,"ok fichier config sauvegardé de :  "+ String(config.HOSTNAME));
          }

           else if ( set >= 3 ) {
            String O;
            bool val;
            String reponsetxt="";
            DynamicJsonBuffer jsonBuffercfg;
            JsonObject& jsoncfg = jsonBuffercfg.parseObject(s);
            
            O=jsoncfg["option"] | ""; 
            val= jsoncfg["valeur"] | true; 
            msgTemp = jsoncfg["ddht"] | msgTemp;
           
            JSONOptions["ddht"] = msgTemp;

            if (O=="SEC") {
                      if (DisSec != val ) { 
                          Option(&DisSec,val);
                          reponsetxt = "-Ok- Options Secondes Mis à jour , Nouvelle Valeur : "+String(val);
                          }
            }
              else if (O=="HOR")
                          {
                          if (TimeOn != val ) { 
                          Option(&TimeOn,val);
                          reponsetxt = "-Ok- Options Affichage Horloge Mis à jour , Nouvelle Valeur : "+String(val);
                          }
            }           
 
      
            else if (O=="LUM")  {
                   if (AutoIn != val ) { 
                          Option(&AutoIn,val);
                          if (val==true ) reponsetxt = "-Ok- Options Mode Auto/manuel mis a jour , mode Auto activé";
                          else reponsetxt = "-Ok- Options Mode Auto/manuel mis a jour , mode Manuel activé avec Intensité à :"+String(Intensite);
                          }
            }
               else if (O=="INT" && !AutoIn)  {
                          //String in= jsoncfg["intensite"] | String(Intensite); 
                          //Intensite =in.toInt();
                          Intensite = jsoncfg["intensite"] | Intensite; 
                          Option(&AutoIn,false);
                           reponsetxt = "-Ok- Options Mode Manuel , Intensité = "+String(Intensite);
                          }
            
          if (reponsetxt!="")  webSocket.sendTXT(num,reponsetxt);
          }
          

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        }

    }
}
//********** fin serveur web


// ***************************
// Fonction gestion des Options
// TimeOn , DisSec / AutoIn
//*****************************

void Option(bool *Poption, bool etat) {
// afectation de la valeur à l'option ( via pointeur )
*Poption = etat;

           if ( Poption == &DisSec  ) {
                  JSONOptions["SEC"] = DisSec;

           }
          else if ( Poption == &AutoIn )  {
                JSONOptions["LUM"] = AutoIn;
              if (etat) NotifMsg("Auto",Intensite,true);
               if ( !etat ) {  
                    if ( Intensite == 0 ) message="Min";
                     else if ( Intensite == 15 ) message="Max";
                    else message="LUM :"+String(Intensite);
                    NotifMsg(message,Intensite,true);
                      P.setIntensity(Intensite); 
                      JSONOptions["INT"] = Intensite;
   
               }
           }
           else if ( Poption == &TimeOn  )  {
                   JSONOptions["HOR"] = TimeOn;
                    if (!etat) 
                    {
                      NotifMsg("OFF",Intensite,true);
                    }
           }

  saveConfiguration(filename, config);
  if (config.DEBUG) {
    int i;
    String Up=config.URL_Update;
    i=strlen(config.URL_Update);
    Serial.println("taille de updtae :"+String(i) + "valeur de Upadte "+Up);
  }
  if (config.JEEDOM && strlen(config.URL_Update)>10 ) ToBox(config.URL_Update);


}


// **********************************************************
//**********************************************************
// ******************* Boucle SETUP ************************
// **********************************************************

void setup(void)
{

 //SPIFFS.format();
 //demarrage file system
  if (SPIFFS.begin()) {
    InfoDebugtInit=InfoDebugtInit+" Demarrage fileSystem -";
 // saveConfig();
  InfoDebugtInit=InfoDebugtInit+" Chargement configuration -";
  loadConfiguration(filename, config);
  
  } else {
    InfoDebugtInit=InfoDebugtInit+" Erreur fileSystem -";
 
  }

if (config.interval_lux > 9 ) interval = config.interval_lux*1000;   // secondes
if (config.interval_dht >= 1) intervaldht = config.interval_dht*60*1000;  // minutes
if (config.interval_debug > 9 ) intervaldebug = config.interval_debug*1000;  //secondes
BkIntensite=Intensite;
  
if (config.DEBUG)  { 
      Serial.begin(115200);
      Serial.println("**********");
      Serial.println("info init : "+InfoDebugtInit);
      Serial.println("Composants optionnels actif :");
      Serial.println("bouton1 :"+String(config.BOUTON_1));
      Serial.println("bouton2 :"+String(config.BOUTON_2));
      Serial.println("clic bouton2-1 :"+String(config.btnclic[2][1]));
      Serial.println("Jeedom :"+String(JEEDOM));
      Serial.println("parametre NTP :");
     // Serial.println(timezone);
      Serial.println(config.NTPSERVER);
      Serial.println("DLS :"+String(config.DLS));
      Serial.println("interval :");
      Serial.println("DHT :"+String(config.interval_dht));
      Serial.println("Debug :"+String(config.interval_debug));
      Serial.println("speed time :"+String(config.SPEED_TIME));
      Serial.println("pause time :"+String(config.PAUSE_TIME));
      Serial.println("Options :");
      Serial.println("Luminosité :"+String(Intensite));
      Serial.println("Secondes :"+String(DisSec));
      Serial.println("AUTO / manuel :"+String(AutoIn));
      Serial.println("TimeOn :"+String(TimeOn));
      Serial.println("MultiZone :"+String(config.multiZone));
    
}


//init demmarrage
sensorValue = analogRead(A0); // init du sensor luminosite




 // Initialize temperature sensor
 // dht.setup(dhtPin, DHTType);
 int pin(dhtpin);
 
 dht.setup(pin,DHTTYPE);
  //dht.begin();
  delay(2000);


P.begin();  // init matrice
P.print("Start ...");

//******** WiFiManager ************
    WiFiManager wifiManager;

     //Si besoin de fixer une adresse IP
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    
    //Forcer à effacer les donnees WIFI dans l'eprom , permet de changer d'AP à chaque demmarrage ou effacer les infos d'une AP dans la memoire ( a valider , lors du premier lancement  )
    if (raz) wifiManager.resetSettings();
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
     wifiManager.setAPCallback(configModeCallback);
    
    //Recupere les identifiants   ssid et Mot de passe dans l'eprom  et essayes de se connecter
    //Si pas de connexion possible , il demarre un nouveau point d'accés avec comme nom , celui definit dans la commande autoconnect ( ici : AutoconnectAP )
    // wifiManager.autoConnect("AutoConnectAP");
    //Si rien indiqué le nom par defaut est  ESP + ChipID
    //wifiManager.autoConnect();
      if(!wifiManager.autoConnect("WIFI-Notifheure")) {
              P.print("erreur AP");
              delay(2000);
              //reset and try again, or maybe put it to deep sleep
              ESP.reset();
              delay(2000);
        } 
    
// ****** Fin config WIFI Manager ************

 // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (config.DEBUG) Serial.print(".");
  }

if (raz) {
  P.print( "RAZ Config");
  SPIFFS.remove(filename);
  delay(400);
  ESP.restart();
}
// Set Hostname.
  //String hostname(config.HOSTNAME);
String  hostname = config.Name;
  hostname += config.HOSTNAME;
  hostname +="-"+String(ESP.getChipId(), HEX);


  // on affiche l'adresse IP attribuee pour le serveur DSN et on affecte le nom reseau
   WiFi.hostname(hostname);
  //*************************
  if (config.DEBUG) WiFi.printDiag(Serial);
  

P.print( "service");
//******* OTA ***************
// Port defaults to 8266
// ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
ArduinoOTA.setHostname((const char *)hostname.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
          else  type = "SPIFFS";
    if ((type)=="SPIFFS") SPIFFS.end();   // NOTE: demontage disque SPIFFS pour mise a jour over the Air
    P.begin();
     P.setFont(ExtASCII);
     P.print("OTA");
     delay(500);
    P.print(type);
    delay(500);
     //Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    P.print("Reboot ..."); 
     
     delay(1000);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    if (MAX_DEVICES<5) sprintf(Notif,"Up %u%%", (progress / (total / 100)));
    else sprintf(Notif,"Upload  %u%%", (progress / (total / 100)));
    P.print(Notif); 
  });
  ArduinoOTA.onError([](ota_error_t error) {
      P.print("ER ..."); 
      delay(1000);
     OTAerreur=true;
   //if (config.DEBUG) Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();

//******* FIN OTA ***************

// Websocket
webSocket.begin();
webSocket.onEvent(webSocketEvent);

//ftp server
  //  ftpSrv.begin("esp8266","pass");   // demarrage serveur ftp avec user = esp8266 et mdp = password
  
// ********************************************
// *********** Page WEB pour reponse **********
// ********************************************
// on definit les points d'entree (les URL a saisir dans le navigateur web) : Racine du site est géré par la fonction handleRoot
//Initialize Webserver
  server.on("/Notification",handleNotif); //Rpour gerer led
  server.on("/Options",handleOptions); //page gestion Options
  server.on("/getInfo",handleInfo); //Rpour récupérer info module ,si pas possible de se servir des websockets
  server.on("/getIP",handleIP); //envoie IP module
  //server.on("/SetAdmin",handleAdmin); // Page gestion Admin
  server.on("/edit",handleEdit); //Requete test
  server.serveStatic("/main.css", SPIFFS, "/config/main.css");
  server.on("/editconfig.html", []() {
       if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
       if (!handleFileRead("/editconfig.html")) {
      server.send(404, "text/plain", "FileNotFound");
    }
     });
     server.on("/config/upload.htm", []() {
       if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
       if (!handleFileRead("/config/upload.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
     });
  // server upload file
  server.on("/Upload", HTTP_GET, []() {
       if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    if (!handleFileRead("/config/upload.html")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  server.on("/Upload", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  }); 
  
  // on demarre le serveur web 
  server.begin();


// Set up mDNS responder:
String  mdnsName = config.Name;
  mdnsName += config.HOSTNAME; 
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(mdnsName.c_str())) {
    if (config.DEBUG) Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  if (config.DEBUG) Serial.println("mDNS responder started");
  //mdns.register("fishtank", { description="Top Fishtank", service="http", port=80, location='Living Room' })
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  MDNS.addService("notifeur", "tcp", 8888); // Announce notifeur service port 8888 TCP


//***************************
//******* Service NTP ********
   NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
    });

// Démarrage du processus NTP  

NTP.begin(config.NTPSERVER, config.timeZone, config.DLS);
NTP.setInterval(60);

    
//************ Initialisation *********
P.print("init");
delay(200);
// *********  des Zones


if (config.multiZone) {
  P.begin(2);
  if (config.LZTime <1 || config.LZTime>(maxdisplay-1)) config.LZTime=round(maxdisplay/2);
  //config.LZTime=3;
  //config.DispRight=false;
  if (config.DispRight) { 
          NZtime=0;
          NZmsg=1;
           P.setZone(0, 0 , config.LZTime-1);
           P.setZone(1,config.LZTime,maxdisplay-1);
  }
  else {
        NZtime=1;
        NZmsg=0;
         P.setZone(0, 0 , maxdisplay-config.LZTime-1);
         P.setZone(1, maxdisplay-config.LZTime, maxdisplay-1);
  }
  P.displayZoneText(NZtime, Horloge, PA_CENTER, 0,0, PA_PRINT, PA_NO_EFFECT);
} else {
  P.begin(1);
  P.setZone(0,0,maxdisplay-1);
  NZtime=0;
  NZmsg=0;
  
}
P.displayZoneText(NZmsg, "", PA_LEFT, config.SPEED_TIME, config.PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

P.setSpriteData(pacman1, W_PMAN1, F_PMAN1,pacman2, W_PMAN2, F_PMAN2);   // chargement animation en memoire

// Verif des options Physiques ( photocell , dht , bouton ... si presentes
 // ------- Capteur Photocell 
   if (sensorValue >20) PhotoCell=true;
   if (config.DEBUG) Serial.println("Verif presence photocell "+String(sensorValue)+" - Capteur Photocell = "+PhotoCell);
// DHT
  GetTemp();
// led
  pinMode(LEDPin,OUTPUT);
  digitalWrite(LEDPin,ledState);
//************************************


// *********************************
// Construction JSON
//**********************************
JSONOptions["SEC"] = DisSec;
JSONOptions["HOR"] = TimeOn;
JSONOptions["LUM"] = AutoIn;
JSONOptions["INT"] = Intensite;
JSONOptions["ddht"] = msgTemp;
JSONOptions["txtAnim"] = txtAnim;
JSONOptions["LEDstate"] = !ledState;

//boutons
  JsonArray& btnclic = JSONSystem.createNestedArray("btnclic");
  btnclic.add(config.btnclic[1][1]);
  btnclic.add(config.btnclic[1][2]);
  btnclic.add(config.btnclic[1][3]);
  btnclic.add(config.btnclic[2][1]);
  btnclic.add(config.btnclic[2][2]);
  btnclic.add(config.btnclic[2][3]);
// system
JSONSystem["version"] = Ver;
JSONSystem["hostname"] = WiFi.hostname();
JSONSystem["mdnsname"] = mdnsName;
JSONSystem["lieu"] = config.Name;
JSONSystem["SSID"] = WiFi.SSID();
JSONSystem["RSSI"] = WiFi.RSSI();
JSONSystem["BSSID"] =WiFi.BSSIDstr();
JSONSystem["channel"] = WiFi.channel();
JSONSystem["DNS"] = WiFi.dnsIP().toString();
JSONSystem["DNS2"] =  WiFi.dnsIP(1).toString();
JSONSystem["passerelle"] = WiFi.gatewayIP().toString();
JSONSystem["IP"] = WiFi.localIP().toString();
JSONSystem["masque"] = WiFi.subnetMask().toString();
JSONSystem["MAC"] = WiFi.macAddress();
JSONSystem["LastSynchroNTP"] = NTP.getTimeDateString(NTP.getLastNTPSync());
JSONSystem["FirstSynchroNTP"] = NTP.getTimeDateString(NTP.getFirstSync());
JSONSystem["NTP"] = config.NTPSERVER;
JSONSystem["uptime"] = NTP.getUptimeString();
JSONSystem["Debug"] = config.DEBUG;
JSONSystem["Photocell"] = PhotoCell;
JSONSystem["Bouton1"] = config.BOUTON_1;
JSONSystem["Bouton2"] = config.BOUTON_2;
JSONSystem["multizone"] = config.multiZone;
JSONSystem["display"] = MAX_DEVICES;
JSONSystem["LED"] = config.LED;

// box
JSONSystem["box"] = config.JEEDOM;

  
// Fin de Setup
  message = " Système OK - Adresse ip : ";
  message += WiFi.localIP().toString();
  NotifMsg(message,Intensite,false);
  if (config.DEBUG)  { 
    Serial.println("fin initialisation"); 
    Serial.print("valeur auto "+String(AutoIn));
    Serial.print(" ,valeur affciahge Secondes "+String(DisSec));
    Serial.print(" ,valeur Affichage horloge "+String(TimeOn));
    Serial.println("");
}



}

//***********************************
//***********************************
//********* Boucle loop *************
//***********************************
//**********************************

void loop(void)
{
  // synchro flasher  ":"
 static uint32_t lastTime = 0; 
 static bool flasher = false;
 static byte CPTReboot=0;
 static bool dj =false;

 // Tempo debug  // reboot
   if (config.DEBUG) {
      if( millis() - previousMillisdebug >= intervaldebug) {
      previousMillisdebug = millis();   
      // afficahge debug
      DebugInfo(); 
       if (Reboot) {
      ++CPTReboot;
       NotifMsg("Reboot",15,true);
    }
    if (CPTReboot>1 && Reboot) ESP.restart();
      } 
   }

// Tempo temperature
  if( millis() - previousMillisdht >= intervaldht) {
     previousMillisdht = millis();   
    GetTemp();
    if ( msgTemp) { 
    
          char MSGt[BUF_SIZE];
          char str_temp[6];
          char str_hum[6];
          dtostrf(temperature, 3, 1, str_temp);
          dtostrf(humidity, 3, 0, str_hum);
          sprintf(MSGt,"Temp : %s °C - Hum : %s%%", str_temp,str_hum);
          message=String(MSGt);
      //message = "Temp : "+String(temperature)+" °C";
       NotifMsg(message,Intensite,false);
       
       }
    }  

// Tempo date
  if( millis() - previousMillisddj >= intervalddj) {
     previousMillisddj = millis();
     dj=1;
     /*String jour=ddj();
      if (TimeOn) NotifMsg(jour,Intensite,false);*/
    } 


// Temporisation luminosite 
  if( millis() - previousMillis >= interval) {
    previousMillis = millis();
   
    // ******** Gestion luminosite
    if (TimeOn) {   // Si Horloge activé
    if (AutoIn) luminosite();  // gestion luminosité auto si activé
    }

  }
// ********** Fin boucle tempo


JSONOptions["INT"] = Intensite;  // mise a jour intensité dans JSON


// ***********  Gestion bouton
if (config.BOUTON_1) {
  // etat bouton
   bouton1.Update();
   // Sauvegarde de la valeur dans la variable click
  if (bouton1.clicks != 0) { 
    clic = bouton1.clicks;
     if (config.DEBUG) Serial.println("Valeur bouton 1 ="+String(config.BOUTON_1)+" , Valeur du clic ="+String(clic));
  BoutonAction(1,clic); 
  clic=0; 
  }
}
if (config.BOUTON_2) {
 // etat bouton 2
   bouton2.Update();
    if (bouton2.clicks != 0) { 
    clic2 = bouton2.clicks;
     if (config.DEBUG) Serial.println("Valeur bouton 2 ="+String(config.BOUTON_2)+" , Valeur du clic ="+String(clic2));
      BoutonAction(2,clic2); 
      clic2=0;
  }
 
}
// ******** Fin gestion bouton


 
// ********* Gestion Reseau : Pages WEB , NTP et OTA
// ********* Service NTP
 if (syncEventTriggered) {
        processSyncEvent (ntpEvent);
        syncEventTriggered = false;
    }
    
//  ****** Page WEb : a chaque iteration, la fonction handleClient traite les requetes 
  server.handleClient();
  webSocket.loop();

// ******* OTA
// Surveillance des demandes de mise a jour en OTA
  ArduinoOTA.handle();
  if (OTAerreur) ESP.restart();
// ******** Fin gestion reseau

//********** flasher - toutes les secondes
if (millis() - lastTime >= 1000)
  {
    lastTime = millis();
    flasher = !flasher;
  }


if (P.displayAnimate())
{

    if (Alert) { 
      if (Fnotif && config.LED) flashlight();
      if (P.getZoneStatus(NZmsg))
      {
      P.setFont(NZmsg,ExtASCII);   // chargement caractere etendue
       // Type de demande 
    if (type=="PAC" ) {    // animation PAC MAn
      P.displayZoneText(NZmsg,txtAnim, PA_LEFT, 40, 1, PA_SPRITE, PA_SPRITE);
      type="";
    }
    else if (type=="BLINDS" ) {  // animation de type volet
       P.displayZoneText(NZmsg,"", PA_CENTER, 40, 1, PA_BLINDS, PA_BLINDS);
      type="";
    }
     else if (type=="OPENING" ) {  // animation Ouverture
       P.displayZoneText(NZmsg,txtAnim, PA_CENTER, 60, 100, PA_OPENING_CURSOR, PA_OPENING_CURSOR);
      type="";
    }
    else {
      P.displayClear(NZmsg);
      P.displayZoneText(NZmsg, Notif, PA_LEFT,config.SPEED_TIME, config.PAUSE_TIME*1000, effect[fx_in], effect[fx_out]);
      if (fx_center) { 
        P.setTextAlignment(NZmsg,PA_CENTER);
       fx_center=false;
       }
       else  FinMsg=true;
       Alert=false;
    }
    }
    } // fin alert
    else {
    if (!config.multiZone) DispZoneTime(flasher);
    }
   //P.displayReset(0);


 if (config.multiZone) DispZoneTime(flasher);
                        
} // fin display animate


} // fin loop

//********************** fin boucle

void DispZoneTime(bool flasher) {
                         P.setFont(NZtime, numeric7Seg_Byfeel); 
                          if (dj==1)  {
                            ddj_time(Notif);
                            P.displayZoneText(NZtime, Notif, PA_CENTER, 0, 3000, PA_PRINT, PA_NO_EFFECT);
                            dj=0;
                          } else {
                          digitalClockDisplay(Horloge,flasher);
                          P.displayZoneText(NZtime, Horloge, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
                          }
}


// Fonction clockformat ( ajoute les zeros devant les unités )
void digitalClockDisplay(char *heure,bool flash)
    {
   // Si affichage de l'heure      
  if (TimeOn) { 
       if (FinMsg) { 
        Intensite=BkIntensite;
        FinMsg=false;
     }
     P.setIntensity(Intensite);
  if ( DisSec ) 
  {  // si affichage des Secondes  00:00 (secondes en petit )
    char secondes[2];  // pour affichage des secondes
   sprintf(secondes,"%02d",second());
    secondes[0]=secondes[0]+23;  // permutation codes AScci 71 à 80
    secondes[1]=secondes[1]+23;
    sprintf(heure,"%02d:%02d %c%c",hour(),minute(),secondes[0],secondes[1]);
    //sprintf(heure,"%02d%c%02d%c%c",hour(),(flash ? ':' : ' '),minute(),secondes[0],secondes[1]);
  }
  // Affichage 00:00  avec : Clignottant ( flasher )
   else sprintf(heure,"%02d%c%02d",hour(),(flash ? ':' : ' '),minute());
   if (config.DEBUG) InfoDebugBoucle=heure;
  }
  else  // sinon point clignottant uniquement
    {
      if (config.DEBUG) InfoDebugBoucle="Horloge désactivé";
       P.setIntensity(0);
      sprintf(heure,"%c",(flash ? '.' : ' '));
    }
  }


String ddj() {
  String jour = jours[weekday()]+" , "+String(day())+" "+mois[month()]+" "+String(year());
  return jour;
}

void ddj_time(char *jour) {
 sprintf(jour,"%02d-%02d-%2d",day(),month(),year()-2000);
}

void Notif_date(char *jour) {
 sprintf(jour,"%02d-%02d-%4d  %02d:%02d:%02d",day(),month(),year(),hour(),minute(),second());
}


void BoutonAction(byte btn , byte btnclic ) {
int actionClick;
if (btnclic < 4 ) actionClick=config.btnclic[btn][btnclic];
else actionClick=btnclic;
  
switch (actionClick) {
    
    case 1: //  Affiche ou non les secondes
          Option(&DisSec,!DisSec);
         // webSocket.sendTXT(num,"test secondes");
      break;
      case 2: //ON / OFF horloge
            Option(&TimeOn,!TimeOn);
      break;
      case 3: // boucle : Manuel intensite à 0 / Manu Intensite MAX / Auto
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
          //ToJeedom(idLum , Intensite);
          P.setIntensity(Intensite);
          ++clicstate;
          break;
       case 4:  // ON off LED
          if (config.LED) {
          ledState = !ledState;
          digitalWrite(LEDPin,ledState); // envoie info à led
          JSONOptions["LEDstate"] = !ledState;
          }   
          break;
      case 5 : //Action 1
            ToBox(config.URL_Action1);
      break;
      case 6 : //Action 2
            ToBox(config.URL_Action2);
      break;
        case 7 : //Action 3
            ToBox(config.URL_Action3);
      break;
       case 255: // simple clic mais long sur dernier  - diminue
      --Intensite;
      if (Intensite<0) Intensite=0;
      NotifMsg("LUM :"+String(Intensite),Intensite,true);
      break;
       case 254: // double clic mais long sur dernier  - Augmente
       ++Intensite;
      if (Intensite>15) Intensite=MAX_INTENSITY;
      NotifMsg("LUM :"+String(Intensite),Intensite,true);
      break;
    default:
      break;  
  }
  webSocket.broadcastTXT("Update");
}


//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool handleFileRead(String path) {
  
    if (path.endsWith("/")) {
    path += "index.html";
  }
  if (config.DEBUG) Serial.println("handleFileRead: " + path);

  String contentType = getContentType(path);
   if (config.DEBUG)  Serial.println("content type " + contentType );
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
}
return false;
}

void handleFileUpload() {
 if (config.DEBUG)  Serial.println("fonction file upload");
  HTTPUpload& upload = server.upload();
   if (config.DEBUG)  Serial.println(upload.status);
  if (upload.status == UPLOAD_FILE_START) {
    String filecfg = upload.filename;
    if (filecfg =="config.json") {
     filecfg = filename;
      if (config.DEBUG)  Serial.print("handleFileUpload Name: "); Serial.println(filecfg);
    fsUploadFile = SPIFFS.open(filecfg, "w");
    }

    filecfg = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      Reboot=true;
    }
     if (config.DEBUG)  Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}
