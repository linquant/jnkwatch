# [IMPORTANT] Authentifier les images OTA

**Label:** `important` · **Type:** security · **Statut:** ouvert

## Contexte
L'OTA par BLE (mise à jour firmware sans fil, caractéristiques OtaCtrl `6a4e010A`
+ OtaData `6a4e010B`, voir `docs/ble-protocol.md` et `firmware/main/ota_update.c`)
a été livrée en **v1 non authentifiée**, de façon cohérente avec le lien BLE
actuellement **non chiffré** (voir [[reenable-ble-encryption]]).

État actuel : la montre vérifie l'**intégrité** de l'image (SHA-256 annoncé au
START + comparé en flux dans `core/ota_proto.c`, puis validation d'en-tête par
`esp_ota_end`), mais **pas son authenticité**. N'importe quel central BLE à
portée peut donc pousser un firmware arbitraire et le faire démarrer. C'est
strictement plus grave que l'exposition de configuration déjà notée dans le
ticket de chiffrement : ici l'attaquant obtient l'**exécution de code**.

Atténuation partielle en place : le double-slot + rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`)
évite le bricking, mais n'empêche pas l'exécution d'un firmware malveillant qui,
lui, « se confirme » volontairement.

## Objectif
N'accepter et ne booter que des images **signées par une clé de confiance**.

## Pistes (par ordre de coût croissant)
1. **Signature applicative vérifiée dans `ota_proto`** (réversible, pas d'eFuse) :
   l'image embarque une signature (Ed25519 ou RSA) ; la clé publique est en dur
   dans le firmware ; `wf_ota_commit` ne réussit que si la signature de l'image
   reçue est valide, avant `esp_ota_set_boot_partition`. Faiblesse : un firmware
   déjà flashé pourrait retirer la vérif (pas de racine de confiance matérielle).
2. **Secure Boot v2 (RSA)** : le bootloader ne démarre qu'une image signée. Le
   support est déjà `*_SUPPORTED/*_PREFERRED` dans sdkconfig mais **non activé**.
   ⚠️ **Irréversible** sur silicium réel (burn d'eFuses) — faire un *dry-run*
   (`CONFIG_SECURE_BOOT` en mode build-only) et tester la chaîne de signature
   AVANT d'activer sur une vraie montre.
3. **Lier à la reprise du chiffrement BLE** ([[reenable-ble-encryption]]) : exiger
   un lien bondé/chiffré pour autoriser le transfert OTA réduit la surface
   (plus d'écriture OTA par un inconnu à portée), sans garantir l'authenticité de
   l'image elle-même — à combiner avec (1) ou (2).

## Recommandation
(1) à court terme (indépendant du chiffrement, réversible), puis (2) quand la
chaîne de build signée est prête. Documenter le risque résiduel tant que rien
n'est en place.
