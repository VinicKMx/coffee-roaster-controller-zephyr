# Architecture

This project is a Zephyr-based coffee roaster controller for a popper-style
roaster. The STM32 NUCLEO-F767ZI is the reference platform for bringing up the
safe control core before any BLE/mobile port.

## Layering

- `application/`: commands, state machine and transport-independent telemetry.
- `control/`: profile interpolation, PID, RoR and filters.
- `safety/`: heater permit, latched faults and interlocks.
- `hardware/`: small Zephyr-facing interfaces for sensor, heater and fan.
- `storage/`: profile/settings/logging backends.

The UI and future BLE transport must send commands to `application/`; they do
not write hardware directly.

## Current Checkpoint

The repository is at the reset/domain-model checkpoint. The heater driver uses
a GPIO time-proportioning scheduler, but the devicetree alias points to the
NUCLEO LED. This lets safety paths be tested before any mains wiring exists.

## Portability

The core files under `application/`, `control/` and `safety/` avoid
STM32-specific includes. A future nRF52840 port should replace hardware and HMI
bindings without rewriting the roast model.
