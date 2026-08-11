# RobotDyn Dimmer

## Module

Current heater actuator target:

```text
RobotDyn-style high-load AC dimmer
16A / 600V class
1 channel
ZC + PSM logic interface
```

The module is a TRIAC phase-control board. `ZC` reports each mains
zero-crossing to the MCU. The firmware then waits a calculated delay and pulses
`PSM` to fire the TRIAC for the remainder of that half-cycle.

References:

- RobotDyn high-load dimmer: https://robotdyn.com/dimmer-module-for-16-24a-600v-high-load-1-channel-3-3v-5v-logic
- RobotDyn AC dimmer connection model: https://github.com/RobotDynOfficial/Documentation/wiki/AC-Light-Dimmer-Module%2C-4-Channel%2C-3.3V_5V-logic%2C-AC-50_60hz%2C-220V_110V

## Firmware Backend

Enable the backend with:

```text
CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER=y
```

The backend expects these devicetree aliases:

```text
heater-zc0
heater-psm0
```

`heater-zc0` must be an input GPIO that supports interrupts. `heater-psm0` must
be an output GPIO connected to the dimmer `PSM` logic pin.

The default mains setting is:

```text
CONFIG_ROASTER_MAINS_HALF_CYCLE_US=8333
```

Use `8333` for 60 Hz mains and `10000` for 50 Hz mains.

## Example Overlay Shape

Do not paste this blindly. Choose pins after the real harness is defined.

```devicetree
/ {
	aliases {
		heater-zc0 = &robotdyn_zc;
		heater-psm0 = &robotdyn_psm;
	};

	robotdyn_inputs {
		compatible = "gpio-keys";

		robotdyn_zc: robotdyn_zc {
			gpios = <&gpiof 15 GPIO_ACTIVE_HIGH>; /* Arduino D2 example */
			label = "RobotDyn ZC";
		};
	};

	robotdyn_outputs {
		compatible = "gpio-leds";

		robotdyn_psm: robotdyn_psm {
			gpios = <&gpioe 13 GPIO_ACTIVE_HIGH>; /* Arduino D3 example */
			label = "RobotDyn PSM";
		};
	};
};
```

## Bring-Up Order

1. Build and run with the LED mock.
2. Enable the RobotDyn backend with only logic-side wiring.
3. Verify `ZC` interrupt rate: about 120 Hz on 60 Hz mains, about 100 Hz on
   50 Hz mains.
4. Verify `PSM` pulse timing with no heater connected.
5. Test with an isolated low-power resistive load.
6. Add safety relay, fuse, enclosure and heatsink before connecting the popper
   heater.

## Limitations

`heater_set_demand()` still accepts `0..1000 permille`, but phase-angle control
is not the same electrical waveform as burst control. The initial mapping is a
timing map suitable for bring-up and must be calibrated with real heater data.

The dimmer does not replace STOP, fuse, original thermal protection, current
sensing or the safety relay.
