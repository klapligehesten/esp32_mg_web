#
# Main Makefile. This is basically the same as a component makefile.
#
# This Makefile should, at the very least, just include $(SDK_PATH)/make/component_common.mk. By default, 
# this will take the sources in the src/ directory, compile them and link them into 
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
CFLAGS += -DMG_ENABLE_HTTP_WEBSOCKET=1 -DCS_PLATFORM=3 -DMG_DISABLE_DIRECTORY_LISTING=1 -DMG_DISABLE_DAV=1 -DMG_DISABLE_CGI=1 -DMG_ENABLE_HTTP=1 -DMG_ENABLE_MQTT=0 -DMG_ENABLE_MQTT_BROKER=0

COMPONENT_PRIV_INCLUDEDIRS = . mongoose wifi flash mg fatfs utils gpio config relay_gpio
COMPONENT_SRCDIRS = . mongoose wifi flash mg fatfs utils gpio config relay_gpio

# include $(IDF_PATH)/make/component_common.mk

