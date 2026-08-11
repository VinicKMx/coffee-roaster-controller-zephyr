cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(roaster_profile_tests)

target_include_directories(app PRIVATE ../../src)
target_sources(app PRIVATE
	../../src/control/profile.c
	test_profile.c
)
