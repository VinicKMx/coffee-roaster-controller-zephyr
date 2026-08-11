# Power Stage

## Current Decision

Use an industrial zero-cross AC SSR with DC input for the first functional
heater control. Do not build a custom triac phase-angle stage for this phase.

## Required Chain

```text
MAINS -> FUSE -> ORIGINAL THERMAL PROTECTION -> SAFETY RELAY -> SSR -> HEATER
```

The SSR is not the only safety element because SSRs can fail short. A relay or
contactor must remove power independently.

## Low-Voltage Driver

The STM32 GPIO must drive a low-voltage transistor/MOSFET stage, not assume the
SSR input can be powered directly by the MCU pin.

## Firmware Interface

Firmware exposes:

```c
heater_set_demand(uint16_t permille);
heater_force_off();
```

The control engine never knows whether this becomes time windows, mains-cycle
burst control or a future board-specific actuator.
