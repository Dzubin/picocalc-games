# PicoCalc Games  (by Anthropic Claude)
#
# Reuses the drivers from ./picocalc-text-starter-main (lcd, audio, southbridge),
# which is picocalc-text-starter by Blair Leduc, plus the Raspberry Pi Pico SDK.
# Build it with the Raspberry Pi Pico VS Code extension, or from a terminal that
# has PICO_SDK_PATH set (see README.md).

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in the Pico SDK (must be before project())
include(pico_sdk_import.cmake)

project(picocalc-games C CXX ASM)

pico_sdk_init()

# Location of the picocalc-text-starter drivers we build against
set(STARTER ${CMAKE_CURRENT_LIST_DIR}/picocalc-text-starter-main)

add_executable(picocalc-games
    games.c
    cards.c
    chooser.c
    settings.c
    cardfly.c
    victory.c
    freecell.c
    freecell_play.c
    klondike.c
    klondike_play.c
    minesweeper.c
    minesweeper_play.c
    mine_cells.c
    ui.c
    platform_pico.c
    ${STARTER}/drivers/lcd.c
    ${STARTER}/drivers/audio.c
    ${STARTER}/drivers/southbridge.c
    ${STARTER}/drivers/font-8x10.c
    ${STARTER}/drivers/font-5x10.c
)

pico_set_program_name(picocalc-games "picocalc-games")

# The program version comes from the VERSION line in games_const.h, so it is
# written down in one place only (a change there re-runs CMake).
file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/games_const.h VERSION_LINE
     REGEX "^#define VERSION ")
string(REGEX REPLACE "^#define VERSION \"V?([^\"]*)\".*" "\\1" PROGRAM_VERSION "${VERSION_LINE}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/games_const.h)
pico_set_program_version(picocalc-games "${PROGRAM_VERSION}")

# Build the audio PIO program (generates audio.pio.h that drivers/audio.c needs)
pico_generate_pio_header(picocalc-games ${STARTER}/drivers/audio.pio)

# No serial console needed; keep USB free so ~ -> BOOTSEL is clean
pico_enable_stdio_uart(picocalc-games 0)
pico_enable_stdio_usb(picocalc-games 0)

target_compile_options(picocalc-games PRIVATE -Wall)

target_include_directories(picocalc-games PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${STARTER}/drivers
)

target_link_libraries(picocalc-games
    pico_stdlib
    pico_multicore
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_pio
    hardware_clocks
    hardware_watchdog
)

# The chip is part of every output file name (games-RP2040.uf2 /
# games-RP2350.uf2, and the matching .elf/.bin/.hex), so a downloaded file
# always says which board it was built for.
if(PICO_PLATFORM MATCHES "^rp2040")
    set(CHIP_NAME RP2040)
elseif(PICO_PLATFORM MATCHES "^rp2350")
    set(CHIP_NAME RP2350)
else()
    set(CHIP_NAME ${PICO_PLATFORM})
endif()
set_target_properties(picocalc-games PROPERTIES OUTPUT_NAME games-${CHIP_NAME})

pico_add_extra_outputs(picocalc-games)

# Copy the finished .uf2 (same name) to the top-level project folder, so the
# RP2040 and RP2350 builds can sit side by side.
add_custom_command(TARGET picocalc-games POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:picocalc-games>/$<TARGET_FILE_BASE_NAME:picocalc-games>.uf2
            ${CMAKE_CURRENT_LIST_DIR}/
)
