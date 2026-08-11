# Wiring

Current firmware wiring:

- `spi1`: MAX31865 RTD frontend.
- `heater0`: devicetree alias currently mapped to NUCLEO `led0` as the safe
  mock backend.

RobotDyn dimmer wiring must add:

- `heater-zc0`: RobotDyn `ZC` output to an interrupt-capable GPIO.
- `heater-psm0`: RobotDyn `PSM` input from an MCU GPIO output.
- `HEATER_PERMIT`: relay/contactor output in series with the dimmer AC path.
- STOP input.
- Display SPI pins.
- Encoder A/B/SW via Zephyr input/gpio-qdec.
- Optional current and mains sensing.

The AC load path is documented in `docs/hardware/power-stage.md`. The exact
NUCLEO pins for `ZC` and `PSM` must be selected after the physical enclosure and
low-voltage harness are defined.
