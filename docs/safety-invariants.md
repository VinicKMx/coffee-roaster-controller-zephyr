# Safety Invariants

## SAFE-001

MCU reset never leaves heater commanded on. Boot initializes the heater output
inactive before normal control.

## SAFE-002

Invalid, stale, open, shorted or out-of-range temperature samples force heater
permission off.

## SAFE-003

Temperature above the hard safety limit forces heater permission off and latches
an overtemperature fault.

## SAFE-004

STOP forces heater permission off independently of UI state.

## SAFE-005

FAULT never permits heater on until an explicit recovery command clears latched
faults.

## SAFE-006

When airflow becomes a real requirement, heater permission must require valid
airflow.

## SAFE-007

Watchdog recovery during roast must be recorded as a fault and start safe.

## SAFE-008

Loss of UI cannot change safety decisions.

## SAFE-009

Future BLE disconnect cannot affect the control loop or safety logic.

## SAFE-010

The future safety relay/contactor must be physically fail-safe: unpowered means
heater de-energized.
