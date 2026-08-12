# Coffee Roaster Controller Zephyr

Safety-oriented embedded coffee roasting controller built on Zephyr for the
NUCLEO-F767ZI reference platform.

The current implementation is an architectural reset checkpoint:

- MAX31865 temperature input is isolated behind `temperature_sensor_read()`.
- Heater output is isolated behind `heater_set_demand()`. The default backend
  is still the NUCLEO LED mock; the hardware target is now a RobotDyn
  16A/600V AC dimmer module using `ZC` and `PSM` pins.
- Safety logic owns final heater permission.
- Application telemetry is transport-independent and emitted as CSV on the
  Zephyr console.

Do not connect heater or mains wiring until the popper wiring, measured load
current, fuse, enclosure, heatsink and safety relay wiring are verified against
`docs/hardware/power-stage.md` and `docs/hardware/robotdyn-dimmer.md`.

## Build

From an initialized Zephyr workspace:

```sh
west build -p always -b nucleo_f767zi -s app
```

To compile the RobotDyn dimmer backend:

```sh
west build -p always -b nucleo_f767zi -s app \
  -- -DEXTRA_CONF_FILE=ci/robotdyn.conf \
     -DEXTRA_DTC_OVERLAY_FILE=ci/robotdyn.overlay
```

## Tests

```sh
west build -p always -b native_sim/native/64 -s app/tests/profile \
  -d build/profile-test
build/profile-test/zephyr/zephyr.exe
```

## Current Hardware Assumptions

- Board: `nucleo_f767zi`
- Temperature sensor: MAX31865 on `spi1`
- Heater actuator target: RobotDyn 16A/600V AC dimmer module
- Safe default actuator: `heater0` devicetree alias mapped to `led0`
- Fan: fixed/full-power concept only; no fan hardware control yet
- STOP input: not wired yet; safety path exists in the model

## Safety Rule

The default physical command is heater off. The firmware must never make the UI,
PID, profile engine or BLE transport the authority that decides whether heating
is permitted.

## Buy Me a Coffee

If this project helped you, you can send a few sats over Lightning:

`maquinalab@walletofsatoshi.com`

<img src="assets/lightning-donation-qr.svg" alt="Lightning donation QR code" width="180">

## License

Dual-licensed under [MIT](LICENSE-MIT) or [Apache-2.0](LICENSE-APACHE), at your
option.
