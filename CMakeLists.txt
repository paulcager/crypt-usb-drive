cmake_minimum_required(VERSION 3.12)

include(${CMAKE_CURRENT_SOURCE_DIR}/../pico-sdk/lib/tinyusb/hw/bsp/family_support.cmake)
include(pico_sdk_import.cmake)

# include(FreeRTOS_Kernel_import.cmake)

project(cryptusbdrive)

pico_sdk_init()

add_executable(cryptusbdrive
        src/image.c
		src/disk_io.c
		src/main.c
		src/usb_descriptors.c
		src/lwipopts.h src/secrets.h src/state_machine.c src/state_machine.h src/secrets.c src/debug.c src/debug.h)

target_include_directories(cryptusbdrive PRIVATE
        include/
        )

target_compile_definitions(cryptusbdrive PRIVATE
		WIFI_SSID="XXX"
		WIFI_PASSWORD="XXX"
		)

target_compile_options(cryptusbdrive PRIVATE -Wall)

set(DBG_PIN_COUNT=4)

#pico_generate_pio_header(cryptusbdrive ${CMAKE_CURRENT_LIST_DIR}/src/probe.pio)

target_include_directories(cryptusbdrive PRIVATE src)

#target_compile_definitions (cryptusbdrive PRIVATE
#	PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
#)

target_link_libraries(cryptusbdrive PRIVATE
		pico_cyw43_arch_lwip_poll
		pico_lwip_http
        pico_stdlib
		# pico_cyw43_arch_none
        tinyusb_device
        tinyusb_board
)

#pico_set_binary_type(cryptusbdrive copy_to_ram)

pico_add_extra_outputs(cryptusbdrive)
