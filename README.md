# horloge-et-Notification
Ce sont les sources liés à l'article sur mon blog byfeel.info , concernant la mise en place d'une horloge avec notification a base d'un WEMOS D1

Collez les deux fichier dans le meme repertoire puis ouvrir avec votre interface IDE Arduino le fichier Horloge_wemos_ota

Variable à definir avant compilation dans votre module

#define NOMMODULE "ESP-Horloge_Nom"   // nom module
#define NTPSERVER "pool.ntp.org"         // Serveur NTP
#define ACTIVBOUTON true              // Si bouton installé
#define ACTIVCAPTLUM true              // Si capteur luminosité installé
#define  BUF_SIZE  60                    // Taille max des notification ( nb de caractéres max )
