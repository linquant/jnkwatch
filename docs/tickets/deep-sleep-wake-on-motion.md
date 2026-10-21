# [IMPORTANT] Veille profonde ESP32 (réveil poignet/minuterie)

**Label:** `important` · **Type:** power · **Statut:** ouvert

## Contexte
La veille actuelle (implémentée) éteint réellement l'AMOLED (DCS 0x28) après
inactivité et ralentit la boucle CPU ; le lien BLE reste actif et le **toucher**
réveille la montre. C'est la « veille légère » choisie par l'utilisateur.

Une **vraie veille profonde** ESP32 (`esp_deep_sleep_start`, ~10 µA) donnerait un
gain batterie bien supérieur, mais **le réveil au toucher y est impossible** : en
deep sleep le CPU et l'I2C sont coupés, et il n'existe **aucune ligne
d'interruption tactile câblée** sur ce PCB — le FT3168 est en polling I2C pur
(confirmé firmware + démo Waveshare + schéma : nets `IMU_INT`/`RTC_INT` présents,
**pas** de `TP_INT`). Les seules sources de réveil disponibles sont donc le
**mouvement IMU (QMI8658 INT)**, l'**alarme RTC (PCF85063)** ou une **minuterie**.

## Objectif
Ajouter un mode veille profonde optionnel (après longue inactivité, ex. plusieurs
minutes) avec réveil au **poignet** et/ou **minuterie**, état minimal conservé en
**mémoire RTC**, reconnexion BLE au réveil.

## Pré-requis / pistes
- **Identifier le GPIO `IMU_INT`** (et `RTC_INT`) sur le schéma
  `docs/hardware/vendor/ESP32-S3-Touch-AMOLED-1.43-Schematic.pdf`, confirmer qu'il
  est RTC-capable (GPIO ≤ 21 sur S3) pour `esp_sleep_enable_ext0/ext1_wakeup`.
- Ajouter le pin à `firmware-esp32/firmware/main/board.h` ; configurer le QMI8658 en
  **wake-on-motion** (implémenter le stub `watch_set_wrist_wake` de
  `device_hal.c`), INT actif → wake.
- Alternative/complément : alarme PCF85063 ou `esp_sleep_enable_timer_wakeup`.
- Sauver l'état utile (face courante, etc.) en `RTC_DATA_ATTR` ; au boot,
  détecter `esp_sleep_get_wakeup_cause()` pour un réveil rapide.
- Accepter/gérer la **reconnexion BLE** (le deep sleep coupe le lien) — d'où le
  choix actuel de ne PAS l'utiliser comme état de repos par défaut (l'utilisateur
  veut rester connecté).
- Décider du déclencheur (timeout long, geste, batterie faible) et de la
  cohabitation avec la veille légère déjà en place (`ui/ui.c` `power_tick`).

## Fichiers
- `firmware-esp32/firmware/main/board.h` (pin IMU_INT / RTC_INT)
- `firmware-esp32/firmware/main/device_hal.c` (`watch_set_wrist_wake`, entrée deep-sleep)
- `firmware-esp32/firmware/main/imu_qmi8658.c` (wake-on-motion), `firmware-esp32/firmware/main/rtc_pcf85063.c` (alarme)
- `firmware-esp32/firmware/main/main.c` (`esp_sleep_*`, wake cause au boot)
- `ui/ui.c` (`power_tick` : seuil/déclencheur veille profonde)
