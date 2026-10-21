# Contributing

Thanks for your interest! This project is a round-smartwatch LVGL UI with a PC emulator.
Contributions are welcome via pull requests.

## Development setup

```sh
git clone --recursive <repo-url>
cd watch
sudo apt install cmake build-essential libsdl2-dev
./build.sh
./test.sh
```

If you cloned without `--recursive`:
```sh
git submodule update --init --recursive
```

## Golden rule: test-driven, logic in `firmware-esp32/core/`

- Put **pure logic** (no LVGL, no platform calls) in `firmware-esp32/core/` and unit-test it in `firmware-esp32/tests/`.
- `firmware-esp32/ui/` should only turn `core/` results into widgets.
- `platform/` is the hardware abstraction layer (HAL); keep device specifics behind it.

Workflow: **write a failing test first**, make it pass, then wire the UI and confirm with
a screenshot.

```sh
./test.sh                       # Unity + CTest
./shot.sh out.png <face> <teinte>   # render one face headless, then inspect out.png
./build/watch_sim               # interactive check (mouse = touch)
```

## Adding a new face

Each face is a self-contained module `ui/face_<name>.{c,h}` exposing:

```c
void face_<name>_create(lv_obj_t *parent);   /* build widgets into the tile */
void face_<name>_tick(void);                  /* per-frame update (optional)  */
void face_<name>_apply_accent(void);          /* re-tint on teinte change     */
```

Then wire it in `ui/ui.c`: add a tile in the tileview, call `create` in `ui_init`,
`tick` in `ui_tick`, and `apply_accent` in `ui_apply_accent`. Add the source to
`SHARED_SRC` in `CMakeLists.txt`.

Layout tips for the round 466×466 screen:
- Use the derived constants in `ui/geometry.h` (`UI_CX`, `UI_SAFE_R`, `UI_CHORD_HALF`)
  instead of hardcoding pixels; keep content inside the safe circle (~210 px radius).
- Reuse the accent system via `theme_accent()` and implement `apply_accent`.

## Style

- C11, 4-space indent, no tabs. Match the surrounding code.
- Keep functions small; prefer `static` file-local helpers.
- Comments explain **why**, not what.
- Run `./test.sh` and `./build.sh` (0 warnings) before opening a PR.

## Commit / PR

- Small, focused commits with clear messages.
- Describe what you changed and how you verified it (tests, screenshots).
- By contributing, you agree your contributions are licensed under Apache-2.0.
