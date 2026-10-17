# LicheePi Nano OS

Buildroot-based Linux image for the Lichee Pi Nano (Allwinner F1C100s, 32 MB RAM)
with SPI-NAND (W25N01GV) and a 480x272 RGB LCD.

## Layout
- `br2-external/` – Buildroot external tree (configs, board files, packages)
- `docker/` – `Dockerfile.base` (full build), `Dockerfile.image` (fast rebuild)
- `myapp/` – LVGL application (planned)
- `docs/`, `scripts/`

## Origin & license
Derived from https://github.com/unframework/licheepi-nano-buildroot.
See `LICENSE` (check the license text and keep upstream notices).
