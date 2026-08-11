# Wiring

Current firmware wiring:

- `spi1`: MAX31865 RTD frontend.
- `heater0`: devicetree alias currently mapped to NUCLEO `led0` as a mock.

Future wiring must add:

- STOP input.
- SSR driver GPIO.
- HEATER_PERMIT relay/contactor output.
- Display SPI pins.
- Encoder A/B/SW via Zephyr input/gpio-qdec.
- Optional current, mains and zero-cross sensing.
