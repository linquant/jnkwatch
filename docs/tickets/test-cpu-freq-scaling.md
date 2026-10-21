# [IMPORTANT] Tester la baisse de fréquence CPU (DFS) pour l'énergie

**Label:** `important` · **Type:** power · **Statut:** ouvert

## Contexte
On a activé `esp_pm` avec `esp_pm_configure()` dans
`firmware-esp32/firmware/main/main.c` (`power_mgmt_init()`), aujourd'hui réglé sur
`max_freq_mhz = 240`, `min_freq_mhz = 40`, `light_sleep_enable = true`.
La **fréquence dynamique (DFS)** fait descendre le CPU vers `min_freq` quand rien
ne tient de verrou PM — c'est un levier d'économie **complémentaire** au light-sleep
et à l'extinction de l'écran, et qui agit **même sur USB** (contrairement au
light-sleep, bridé tant que le câble data est branché).

## Objectif
Mesurer l'impact d'une fréquence min/max plus basse sur la **conso**, la
**fluidité de l'UI** (LVGL), la **latence tactile** et la **stabilité BLE**, puis
choisir les valeurs.

## À tester
- Faire varier `min_freq_mhz` : 80 → 40 → (avec horloge RTC) 10 MHz.
- Essayer un `max_freq_mhz` réduit (ex. 160) et juger la fluidité des animations
  (aura, aiguilles) et des transferts QSPI de l'écran.
- Vérifier que les drivers qui prennent un verrou PM (SPI/QSPI écran, BLE) forcent
  bien la remontée en fréquence pendant leurs transferts (pas de saccade / pas de
  déco BLE).
- Comparer la conso **écran allumé** vs **écran éteint** à chaque réglage
  (wattmètre USB / alim externe).
- Idéalement : profiter d'une **batterie** pour mesurer aussi le gain light-sleep
  (voir le gotcha USB dans le mémo power management).

## Fichiers
- `firmware-esp32/firmware/main/main.c` (`power_mgmt_init` : `esp_pm_config_t` min/max freq)
- `firmware/sdkconfig.defaults` (options PM / tickless idle déjà activées)

## Lié
- `docs/tickets/deep-sleep-wake-on-motion.md`
- Écran éteint + light-sleep + paramètres de connexion BLE (déjà en place).
