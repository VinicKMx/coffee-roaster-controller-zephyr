# Coffee Roaster Controller Zephyr

Safety-oriented embedded coffee roasting controller built on Zephyr for the
NUCLEO-F767ZI reference platform.

The current implementation is an architectural reset checkpoint:

- MAX31865 temperature input is isolated behind `temperature_sensor_read()`.
- Heater output is isolated behind `heater_set_demand()` and is mapped to the
  NUCLEO LED as a safe mock until the popper power stage is characterized.
- Safety logic owns final heater permission.
- Application telemetry is transport-independent and emitted as CSV on the
  Zephyr console.

Do not connect the heater or mains wiring to this firmware until
`docs/hardware/popper-characterization.md` and `docs/hardware/power-stage.md`
are completed for the actual popper.

## Build

From a Zephyr workspace:

```sh
west build -b nucleo_f767zi
```

For a pristine rebuild:

```sh
west build -p always -b nucleo_f767zi
```

With the local workspace found under `../projetos`:

```sh
cd /home/vinicius/Documents/projetos/zephyr-dev/workspaces/blueglyph
west build -p always -b nucleo_f767zi \
  -s /home/vinicius/Documents/projetos-pessoais/coffee-roaster-controller-zephyr \
  -d /tmp/coffee-roaster-build \
  -- -DZEPHYR_SDK_INSTALL_DIR=/home/vinicius/Documents/projetos/zephyr-dev/sdk/zephyr-sdk-0.16.8
```

That workspace is Zephyr 3.7.0. SDK 0.17.2 failed in the Zephyr Picolibc
integration, while SDK 0.16.8 built successfully.

## Tests

```sh
cd /home/vinicius/Documents/projetos/zephyr-dev/workspaces/blueglyph
west build -p always -b native_sim/native/64 \
  -s /home/vinicius/Documents/projetos-pessoais/coffee-roaster-controller-zephyr/tests/profile \
  -d /tmp/coffee-roaster-test-profile \
  -- -DZEPHYR_SDK_INSTALL_DIR=/home/vinicius/Documents/projetos/zephyr-dev/sdk/zephyr-sdk-0.16.8
/tmp/coffee-roaster-test-profile/zephyr/zephyr.exe
```

## Current Hardware Assumptions

- Board: `nucleo_f767zi`
- Temperature sensor: MAX31865 on `spi1`
- Heater actuator: `heater0` devicetree alias, currently mapped to `led0`
- Fan: fixed/full-power concept only; no fan hardware control yet
- STOP input: not wired yet; safety path exists in the model

## Safety Rule

The default physical command is heater off. The firmware must never make the UI,
PID, profile engine or BLE transport the authority that decides whether heating
is permitted.
