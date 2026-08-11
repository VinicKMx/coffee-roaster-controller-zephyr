# Power Stage

## Current Decision

Use a RobotDyn-style 16A/600V high-load AC dimmer module as the current heater
actuator. This is a TRIAC dimmer with a zero-cross output and a TRIAC gate
trigger input, not an industrial zero-cross SSR.

The firmware keeps the actuator behind `heater_set_demand()` so the rest of the
controller remains independent from phase-angle timing.

## Required Chain

```text
MAINS -> FUSE -> ORIGINAL THERMAL PROTECTION -> SAFETY RELAY -> DIMMER -> HEATER
```

The dimmer is not the only safety element. A relay or contactor must remove
power independently when STOP, FAULT, reset or firmware health requires heater
shutdown.

## RobotDyn Interface

The module logic side uses:

- `VCC`: 3.3 V or 5 V logic supply, depending on the exact module.
- `GND`: logic ground.
- `ZC`: zero-cross signal into an interrupt-capable MCU GPIO.
- `PSM`: MCU output that fires the TRIAC gate after each zero-cross.

The RobotDyn high-load documentation describes the module as a 16A continuous,
600V TRIAC dimmer with optocoupler isolation and calls out heatsinking, fusing
and enclosure requirements. Verify the exact board received before wiring mains
because clones may differ.

References:

- RobotDyn high-load dimmer: https://robotdyn.com/dimmer-module-for-16-24a-600v-high-load-1-channel-3-3v-5v-logic
- RobotDyn AC dimmer connection model: https://github.com/RobotDynOfficial/Documentation/wiki/AC-Light-Dimmer-Module%2C-4-Channel%2C-3.3V_5V-logic%2C-AC-50_60hz%2C-220V_110V

## Validation Before Heater Connection

Before connecting the popper heater:

- Confirm line voltage, heater resistance and RMS current.
- Confirm the fan is not accidentally controlled by the heater dimmer.
- Install the correct fuse and strain relief.
- Mount the dimmer in an enclosure with protected heatsink.
- Verify `PSM` timing with a low-power test load and oscilloscope.
- Verify STOP and safety relay remove heater power independently of `PSM`.

## Firmware Interface

Firmware exposes:

```c
heater_set_demand(uint16_t permille);
heater_force_off();
```

The control engine never knows whether this becomes an LED mock, RobotDyn
phase-angle dimmer, burst scheduler or future board-specific actuator.
