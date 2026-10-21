# [IMPORTANT] Réactiver le chiffrement/bonding BLE

**Label:** `important` · **Type:** security · **Statut:** ouvert

## Contexte
Pour débloquer l'app compagnon (Phase 4), l'exigence de **chiffrement BLE** a été
**retirée** des caractéristiques GATT (commit « strip encryption »). Un couple
bonding NimBLE ↔ Android provoquait une **boucle de ré-appairage** : à chaque
reconnexion, l'un des deux ne conservait pas la clé → nouvelle demande
d'appairage sans fin.

État actuel : toutes les caractéristiques sont en **clair** (aucun flag
`BLE_GATT_CHR_F_*_ENC` dans `firmware/main/ble_server.c`). N'importe quel central
BLE à portée peut lire/écrire les réglages, l'heure, pousser des notifs et
reconfigurer les boutons. Risque faible pour une montre perso, mais réel.

## Objectif
Rétablir une liaison **chiffrée et bondée** qui **persiste** correctement,
**sans** boucle de ré-appairage.

## Pistes
- Diagnostiquer la persistance des clés NimBLE : confirmer que
  `CONFIG_BT_NIMBLE_NVS_PERSIST=y` + `ble_store_config_init()` sauvegardent bien
  le bond (namespace NVS `nimble_bond`), et qu'il survit reconnexion **et** reboot.
- Vérifier l'adresse de la montre : utiliser une adresse **publique stable**
  (efuse MAC) ou une identité résolvable (IRK distribué) pour que le téléphone
  reconnaisse l'appareil bondé et **ne re-pair pas**.
- Revoir le handler `BLE_GAP_EVENT_REPEAT_PAIRING` (il **supprime** le bond
  actuel puis retente — s'il se déclenche à chaque connexion, c'est un symptôme,
  pas la solution).
- Côté Android : gérer proprement `BluetoothDevice.createBond()` /
  `ACTION_BOND_STATE_CHANGED`, et remettre les flags `_ENC` sur les
  caractéristiques (surtout Settings/Time/NotifPush/ActionCfg).
- Tester : appairer une fois, couper/reconnecter plusieurs fois et rebooter la
  montre → **aucune** nouvelle demande d'appairage.

## Fichiers
- `firmware/main/ble_server.c` (flags des caractéristiques + config `ble_hs_cfg.sm_*`)
- `firmware/sdkconfig.defaults` (`CONFIG_BT_NIMBLE_NVS_PERSIST`)
- `docs/ble-protocol.md` (section Security)
- App : `android/app/src/main/java/fr/ponera/jnkwatch/ble/WatchBleClient.kt`
