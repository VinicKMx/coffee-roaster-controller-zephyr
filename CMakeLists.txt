cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coffee_roaster_controller)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
	src/main.c
	src/application/commands.c
	src/application/roaster.c
	src/application/state_machine.c
	src/application/telemetry.c
	src/control/controller.c
	src/control/filters.c
	src/control/pid.c
	src/control/profile.c
	src/control/ror.c
	src/hardware/fan.c
	src/hardware/heater.c
	src/hardware/temperature.c
	src/safety/faults.c
	src/safety/interlocks.c
	src/safety/safety_manager.c
	src/storage/logger.c
)
