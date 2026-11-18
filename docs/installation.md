# Installation et première mise en service

## 1. Préparer la carte microSD

Formatez-la en **FAT32**, puis insérez-la dans le lecteur. L'appareil crée lui-même les dossiers `sessions/` et `mesures/`. Sans carte SD, aucune session ne peut démarrer : la traçabilité serait impossible.

## 2. Installer le firmware

### Option A : firmware précompilé (le plus simple)

1. Branchez l'ESP32 en USB sur un ordinateur, avec Chrome ou Edge.
2. Ouvrez **https://espressif.github.io/esptool-js/**, cliquez sur *Connect* et choisissez le port de la carte.
3. Cliquez sur *Erase Flash*, puis attendez la fin.
4. Dans *Flash Address* mettez **0x0**, choisissez le fichier `firmware/bin/compteur-up-v1.2.0-esp32-merged.bin`, puis cliquez sur *Program*.
5. Débranchez et rebranchez la carte.

### Option B : compiler avec l'IDE Arduino (2.x)

1. *Fichier → Préférences → URL de gestionnaire de cartes supplémentaires* : ajoutez
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. *Outils → Carte → Gestionnaire de cartes* : installez **esp32 by Espressif Systems, version 3.x** (testé avec 3.0.7).
3. *Croquis → Inclure une bibliothèque → Gérer les bibliothèques* : installez
   - **OneWire** (Paul Stoffregen)
   - **DallasTemperature** (Miles Burton)
   - **U8g2** (olikraus)
   - **RTClib** (Adafruit), en acceptant l'installation d'*Adafruit BusIO*
   - **ArduinoJson** (Benoit Blanchon), **version 7**
4. Ouvrez `firmware/compteur_up/compteur_up.ino`.
5. *Outils* :
   - Carte : **ESP32 Dev Module**
   - Partition Scheme : **Huge APP (3MB No OTA/1MB SPIFFS)**. Indispensable : le firmware ne tient pas dans la partition par défaut.
6. Téléversez.

### Option C : PlatformIO

`cd firmware && pio run -t upload` (voir `platformio.ini`).

## 3. Première connexion

À la première mise sous tension, aucun Wi-Fi n'est configuré. L'appareil crée donc son propre réseau :

- réseau **`CompteurUP`**, mot de passe **`pasteur2026`** ;
- ouvrez **http://192.168.4.1** sur le téléphone.

Ensuite :

1. Onglet **Réglages** → mot de passe administrateur **`pasteur`** → *Ouvrir*.
2. Renseignez le **Wi-Fi de l'atelier** et changez le **mot de passe administrateur**.
3. Cliquez sur *Enregistrer*, puis sur *Redémarrer*.
4. L'écran affiche maintenant l'adresse IP. La page est aussi accessible à **http://compteur-up.local** (selon le téléphone).

> Les mots de passe par défaut (`pasteur2026` pour le réseau de secours, `pasteur` pour l'administration) se changent dans `config.h` et dans les Réglages. Le réseau de secours ne s'active que si le Wi-Fi de l'atelier est introuvable au démarrage.

## 4. Heure

Avec Internet, l'heure se règle seule (NTP, heure de Paris, été/hiver automatique) et elle est recopiée dans l'horloge DS3231. Sans Internet, utilisez *Réglages → Régler sur l'heure du téléphone*.

**Aucune session ne démarre tant que l'heure n'est pas valide**, pour que les relevés soient correctement datés.

## 5. Notifications sur le téléphone (ntfy)

1. Installez l'application **ntfy** (Android ou iPhone).
2. Inventez un **sujet long et difficile à deviner**, par exemple `pasteur-atelier-7h3k9q2x`. Sur ntfy.sh, les sujets sont publics : quiconque connaît le nom peut lire les messages.
3. Dans l'application : *S'abonner* → ce sujet.
4. Dans *Réglages* de l'appareil : serveur `https://ntfy.sh`, le même sujet. Cochez *Alarmes* et *Fin de lot*, et *Chaque décalage* si vous le souhaitez.
5. Cliquez sur *Enregistrer*, puis sur *Tester ntfy*.

Un serveur ntfy auto-hébergé fonctionne aussi : en `http://` sur le réseau local, ou en `https://` avec un certificat Let's Encrypt.

## 6. Vérifications avant la première utilisation

- Les 4 sondes s'affichent (onglet *Tableau de bord → Sondes*). Une sonde « absente » signale un problème de câblage ou de résistance.
- *Réglages → Tester le buzzer*.
- Appuyez sur le gros bouton : sans session en cours, le journal affiche « Aucune session ni mesure en cours ».
- Appuyez sur **OK** : le menu s'ouvre ; **▲ ▼** déplacent la sélection ; **OK long** revient. Si le menu bouge tout seul, une résistance de 10 kΩ manque sur l'une des broches 34, 35 ou 39.
- Faites l'**étalonnage des sondes**, puis un **essai à blanc** (voir [utilisation.md](utilisation.md)).
