# Repository setup (maintainer, one-time)

These are the one-time steps to turn this working tree into a public git repository
with **LVGL** and **Unity** as pinned git submodules. Run them from the project root.

> Detected versions to pin: **LVGL v9.2.2**, **Unity v2.6.0**.
> `third_party/stb_image_write.h` is a single public-domain header and stays vendored.
> Copyright holder in `LICENSE`/`NOTICE` is set to `jnkwatch`.

```sh
# 0) sanity: build & tests pass first
cd firmware-esp32 && ./build.sh && ./test.sh

# 1) init the repo
git init
git add .gitignore LICENSE NOTICE README.md CONTRIBUTING.md \
        lv_conf.h CMakeLists.txt build.sh shot.sh test.sh \
        core ui platform sim shot tests docs firmware-esp32 tools
git add third_party/stb_image_write.h

# 2) LVGL as a submodule pinned to v9.2.2
rm -rf lvgl                 # remove the vendored shallow clone
git submodule add https://github.com/lvgl/lvgl.git lvgl
git -C lvgl checkout v9.2.2
git add lvgl .gitmodules

# 3) Unity as a submodule pinned to v2.6.0
rm -rf third_party/unity
git submodule add https://github.com/ThrowTheSwitch/Unity.git third_party/unity
git -C third_party/unity checkout v2.6.0
git add third_party/unity .gitmodules

# 4) first commit
git commit -m "Initial commit: round smartwatch LVGL UI + PC emulator"

# 5) push to your remote
git branch -M main
git remote add origin git@github.com:linquant/jnkwatch.git
git push -u origin main
```

## Verifying a fresh clone

```sh
git clone --recursive git@github.com:linquant/jnkwatch.git
cd watch && cd firmware-esp32 && ./build.sh && ./test.sh
```

## Notes

- Submodules record a specific commit; to move LVGL later:
  `git -C lvgl fetch && git -C lvgl checkout <tag> && git add lvgl && git commit`.
- `docs/screenshots/*.png` are force-included (the global `*.png` ignore is negated for them).
- `ui/fonts/wf_digits_100.c` is a generated file that is intentionally committed; the
  command to regenerate it is in the README.
