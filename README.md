# horloge-et-Notification
Ce sont les sources liés à l'article sur mon blog [byfeel.info](https://byfeel.infp) , concernant la mise en place d'une horloge avec notification a base d'un module esp8266 comme les wemos ( d1, D2 ou mini ) ou compatibles

## installation
Copiez le contenu du dossier , selon la version souhaité sur votre poste et lancez votre interface Arduino.

Variable à definir avant compilation dans votre module

Selon les versions utilisés , vous aurez des variables à définir.

Plus de detail dans l'article DIY Horloge et Notification sur mon blog [pour la version 2](https://byfeel.info/diy-i-notifheure-ou-comment-mettre-en-place-une-horloge-connectee-avec-notification/) et [pour la version 3 ](https://byfeel.info/notifheure-v3-diy/)

## Historique version :

<<<<<<< HEAD
**V3.1.2** : Correctifs et support nouveau script php pour jeedom  
**V3.1.1** : Mise a jour bug dht  
**V3.1** : Nouvelle version ( Novembre 2018 )
=======
#define ACTIVCAPTLUM true              // Si capteur luminosité installé

#define  BUF_SIZE  60                    // Taille max des notification ( nb de caractéres max )

Plus de detail dans l'article DIY Horloge et Notification sur mon blog http://byfeel.info

Historique version :

V3.1.4 : bug fix ( pause 0 S )

V3.1.3 : bug fix + mode info et fix

V3.1.2 : Correctifs et support nouveau script php pour jeedom

V3.1.1 : Mise a jour bug dht

V3.1 : Nouvelle version ( Novembre 2018 )
>>>>>>> 008014a6660d6b4f42b310471b4f8c6a99df8b7c


--------------------------------------------
Si pas intéressé par fonction avançés interface web - V2.6 suffit


**V2.6** : Ajout affichage Horloge ( font personalisé ) - pour un affichage plus PRO.

**V2.5** : Enregistrement des options en EEPROM , pour sauvegarde aprés coupure de courant
Gestion de deux ZONES

!!!!!!!!!!!!!!!!!!!!

**V2.45** : Utilisation des bibliotheque V3 de parola et MAX72xx  !!!!!!!!

!!!!!!!


**V2.4** : Remontée d'info dans jeedom

**V2.3** : Ajout de la gestion de la luminosité et des animations depuis jeedom

**V2.2** : Animation PAC Man

**V2.1** : Gestion des clicks - Ajouts des animations

**V2** : Ajout support bouton / refonte du code

**V1.3** : Ajout interface WEB

**V1.2** : Ajout du support de notification

**V1** : Debut projet / affichage Horloge avec synchro NTP
