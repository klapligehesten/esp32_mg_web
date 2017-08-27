#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
CFLAGS += -DCS_PLATFORM=3 -DMG_DISABLE_DIRECTORY_LISTING=1 -DMG_DISABLE_DAV=1 -DMG_DISABLE_CGI=1 -DRTOS_SDK

PROJECT_NAME := app-template

include $(IDF_PATH)/make/project.mk

