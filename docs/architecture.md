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

The repository is at the reset/domain-model checkpoint with the heater
abstraction in place. The safe default backend maps heater demand to the NUCLEO
LED. The selected real actuator is a RobotDyn 16A/600V AC dimmer module, driven
as a hardware-layer detail through zero-cross input and TRIAC gate output
aliases.

The application, PID/profile code and safety manager still exchange normalized
heater demand in `0..1000 permille`; they do not know whether the hardware
backend is an LED mock, a RobotDyn phase-angle dimmer or a future power board.

## Portability

The core files under `application/`, `control/` and `safety/` avoid
STM32-specific includes. A future nRF52840 port should replace hardware and HMI
bindings without rewriting the roast model.
