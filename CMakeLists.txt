# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Uncomment if you must specify the board here
set(BOARD nucleo_f446re)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/idein.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(idein_mk_ii)

# Explicitly list source files
set(app_sources
    src/codec.c
    src/main.c
    src/midi.c
    src/idein.c
    src/sensor.c
    src/pot.c
    src/voice.c
    src/ui.c

    # Add other source files here
)

target_sources(app PRIVATE ${app_sources})

# Main Include Directory
target_include_directories(app PRIVATE include)


