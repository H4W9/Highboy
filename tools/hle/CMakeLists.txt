cmake_minimum_required(VERSION 3.16)
project(TentacleOS_HLE C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FIRMWARE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(HLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(HLE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ── Collect firmware component include dirs ───────────────────────────────────
function(collect_firmware_includes TARGET_DIR OUTPUT_VAR)
    set(_includes "")
    file(GLOB_RECURSE _dirs LIST_DIRECTORIES true "${TARGET_DIR}/components/*/include")
    foreach(_dir ${_dirs})
        list(APPEND _includes ${_dir})
    endforeach()
    set(${OUTPUT_VAR} ${_includes} PARENT_SCOPE)
endfunction()
collect_firmware_includes("${FIRMWARE_ROOT}/firmware_c5" C5_INCLUDES)
collect_firmware_includes("${FIRMWARE_ROOT}/firmware_p4" P4_INCLUDES)

function(collect_firmware_sources TARGET_DIR PATTERN OUTPUT_VAR)
    file(GLOB_RECURSE _sources "${TARGET_DIR}/${PATTERN}")
    set(${OUTPUT_VAR} ${_sources} PARENT_SCOPE)
endfunction()

# ── C5 firmware sources ──────────────────────────────────────────────────────
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Core/*.c" C5_CORE_SRC)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "main/*.c" C5_MAIN_SRC)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Drivers/*.c" C5_DRV)
message(STATUS "C5 driver files: ${C5_DRV}")
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Service/spi_bridge/spi_bridge.c" C5_SVC_BRIDGE)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Service/spi_bridge/session_manager.c" C5_SVC_SESSION)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Service/spi_bridge/wifi_dispatcher.c" C5_SVC_WIFI_DISP)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_c5" "components/Service/spi_bridge/bt_dispatcher.c" C5_SVC_BT_DISP)

# ── P4 firmware sources ──────────────────────────────────────────────────────
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Core/*.c" P4_CORE_SRC)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "main/*.c" P4_MAIN_SRC)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Drivers/spi/spi.c" P4_DRV_SPI)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Drivers/buttons_gpio/*.c" P4_DRV_BTN)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Drivers/bq25896/*.c" P4_DRV_BQ)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Drivers/led/led_control.c" P4_DRV_LED)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Drivers/i2c_init/*.c" P4_DRV_I2C)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Service/storage_vfs/vfs_core.c" P4_VFS_CORE)
collect_firmware_sources("${FIRMWARE_ROOT}/firmware_p4" "components/Service/storage_vfs/vfs_auto.c" P4_VFS_AUTO)

# ── Dependencies ─────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git GIT_TAG v9.2.2)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h" CACHE STRING "" FORCE)
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lvgl)

find_package(SDL2 QUIET)

# ── Common compile flags ─────────────────────────────────────────────────────
add_compile_definitions(HLE_BUILD)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    -Wno-sign-compare -Wno-missing-field-initializers -fno-strict-aliasing)

# ── Libraries ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE HLE_SRC "${HLE_SRC_DIR}/*.cpp")
list(FILTER HLE_SRC EXCLUDE REGEX ".*main\\.cpp$")        # main is for interactive only
list(FILTER HLE_SRC EXCLUDE REGEX ".*lv_port.*\\.cpp$")   # lv_port needs display context
add_library(hle_shims STATIC ${HLE_SRC})
target_include_directories(hle_shims PUBLIC ${HLE_INCLUDE_DIR})
target_compile_options(hle_shims PRIVATE -Wno-pmf-conversions)

add_library(firmware_c5 STATIC
    ${C5_CORE_SRC} ${C5_MAIN_SRC} ${C5_DRV}
    ${C5_SVC_BRIDGE} ${C5_SVC_SESSION} ${C5_SVC_WIFI_DISP} ${C5_SVC_BT_DISP}
)
target_include_directories(firmware_c5 SYSTEM PRIVATE ${HLE_INCLUDE_DIR} ${C5_INCLUDES})
target_compile_options(firmware_c5 PRIVATE -Wno-implicit-function-declaration)

add_library(firmware_p4 STATIC
    ${P4_CORE_SRC} ${P4_MAIN_SRC}
    ${P4_DRV_SPI} ${P4_DRV_BTN} ${P4_DRV_BQ} ${P4_DRV_LED} ${P4_DRV_I2C}
    ${P4_VFS_CORE} ${P4_VFS_AUTO}
)
target_include_directories(firmware_p4 SYSTEM PRIVATE ${HLE_INCLUDE_DIR} ${P4_INCLUDES})
target_include_directories(firmware_p4 SYSTEM PRIVATE ${lvgl_SOURCE_DIR})
target_compile_options(firmware_p4 PRIVATE -Wno-implicit-function-declaration)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
add_executable(hle_tests ${TEST_SOURCES})
target_link_libraries(hle_tests PRIVATE hle_shims GTest::gtest_main pthread)
target_include_directories(hle_tests PRIVATE ${HLE_INCLUDE_DIR})
include(GoogleTest)
gtest_discover_tests(hle_tests)

# ── Interactive ──────────────────────────────────────────────────────────────
if(SDL2_FOUND)
    file(GLOB_RECURSE INTERACTIVE_SRC
        "${HLE_SRC_DIR}/main.cpp"
        "${HLE_SRC_DIR}/hal/lv_port_disp_hle.cpp"
        "${HLE_SRC_DIR}/hal/lv_port_indev_hle.cpp"
    )
    add_executable(hle_interactive EXCLUDE_FROM_ALL ${INTERACTIVE_SRC})
    target_include_directories(hle_interactive PRIVATE ${HLE_INCLUDE_DIR})
    target_include_directories(hle_interactive SYSTEM PRIVATE ${lvgl_SOURCE_DIR})
    target_compile_definitions(hle_interactive PRIVATE
        -DLV_CONF_INCLUDE_SIMPLE
        -DLV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h"
        -DLV_LVGL_H_INCLUDE_SIMPLE
    )
    target_link_libraries(hle_interactive PRIVATE hle_shims lvgl SDL2::SDL2 pthread)
    message(STATUS "SDL2 found — hle_interactive enabled")
endif()
