# horloge-et-Notification
Ce sont les sources liés à l'article sur mon blog [byfeel.info](https://byfeel.infp) , concernant la mise en place d'une horloge avec notification a base d'un module esp8266 comme les wemos ( d1, D2 ou mini ) ou compatibles

## installation
Copiez le contenu du dossier __notifheure__ ,  sur votre poste et lancez votre interface Arduino , en double cliquant sur le fichier __notifheure.ino__

Variable à définir avant compilation dans votre module :     

      // *************************
      //**************************
      //****** CONFIG SKETCH *****
      //**************************
      //**************************
      //****************************************************
      // En fonction de vos matrices , si probléme         *
      // d'affichage ( inversé , effet miroir , etc .....) *
      // ***************************************************
      // matrix   - decocher selon config matrix    ********     
      #define HARDWARE_TYPE MD_MAX72XX::FC16_HW        //***
      //#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW    //***
      //#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW //***
      //#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW   //***
      // ***************************************************
      //****************************************************
      #define MAX_DEVICES 8 // nombre de matrice  
      // PIN de raccordement matrice ( si different )
      #define CLK_PIN   D5
      #define DATA_PIN  D7
      #define CS_PIN    D6
      // Options ( modifie si different ou laisser valeur par défaut )
      // Ne pas supprimer ou ne pas modifier , meme si options absentes
      #define PINbouton1 0 // Bouton 1   --- GPIO0 ( pullup integré - D8 pour R1 ou ( D3 pour R2 ou wemos mini )
      #define PINbouton2 2 // Bouton 2   -- GPIO2  ( d9 por wemos r1 ou d3 pour wemos mini
      #define DHTTYPE DHTesp::DHT22  // si DHT22 ou DHT11 si DHT11
      //#define DHTTYPE DHTesp::DHT11  // si DHT11
      #define dhtpin 16 // GPIO16  egale a D2 sur WEMOS D1R1  ou D0 pour les autres ( a verifier selon esp )
      #define LEDPin 5  // GPIO 5  D15 pour wemos R1  ou D1 pour wemos mini


Selon les versions utilisés , vous aurez des variables à définir.

Plus de detail dans l'article DIY Horloge et Notification sur mon blog [pour la version 2](https://byfeel.info/diy-i-notifheure-ou-comment-mettre-en-place-une-horloge-connectee-avec-notification/) et [pour la version 3 ](https://byfeel.info/notifheure-v3-diy/)

## Historique version :


Plus de detail dans l'article DIY Horloge et Notification sur mon blog http://byfeel.info

### Release :    
**V3.3** : Ajout fonction Minuteur / correctif Bug et ajout option OFF HORLOGE ( noir complet )  
**V3.2** : Gestion des effets pour le type INFO et FIX , Historisation des messages , corrections .....  
**V3.1.2** : Correctifs et support nouveau script php pour jeedom  
**V3.1.1** : Mise a jour bug dht    
**V3.1** : Nouvelle version ( Novembre 2018 )  
**V3.1.4** : bug fix ( pause 0 S )  
**V3.1.3** : bug fix + mode info et fix  
**V3.1.2** : Correctifs et support nouveau script php pour jeedom  
**V3.1.1** : Mise a jour bug dht  
**V3.1** : Nouvelle version ( Novembre 2018 )

-------------------------------------------- Version 3  
Si pas intéressé par fonction avançés interface web - V2.6 suffit


**V2.6** : Ajout affichage Horloge ( font personalisé ) - pour un affichage plus PRO.  
**V2.5** : Enregistrement des options en EEPROM , pour sauvegarde aprés coupure de courant
Gestion de deux ZONES  
**V2.45** : Utilisation des bibliotheque V3 de parola et MAX72xx    
**V2.4** : Remontée d'info dans jeedom  
**V2.3** : Ajout de la gestion de la luminosité et des animations depuis jeedom  
**V2.2** : Animation PAC Man  
**V2.1** : Gestion des clicks - Ajouts des animations  
**V2** : Ajout support bouton / refonte du code

_____________________________________ Version 2  
**V1.3** : Ajout interface WEB  
**V1.2** : Ajout du support de notification  
**V1** : Debut projet / affichage Horloge avec synchro NTP
