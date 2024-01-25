# Component Makefile
#

include $(COMPONENT_PATH)/../openthread_common.mk

## These include paths would be exported to project level
ifeq ("$(CONFIG_TARGET_CUSTOMER)", "das")
COMPONENT_ADD_INCLUDEDIRS += ../subg_openthread/include ../subg_openthread/src/core ../subg_openthread/src ../subg_openthread/examples/platforms
else
COMPONENT_ADD_INCLUDEDIRS += ../openthread/include ../openthread/src/core ../openthread/src ../openthread/examples/platforms
endif

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

## This component's src 
COMPONENT_SRCS := ot_alarm.c \
                  ot_diag.c \
                  ot_crypto.c \
                  ot_entropy.c \
                  ot_settings.c \
                  ot_logging.c \
                  ot_misc.c \
                  ot_radio.c \
                  ot_uart.c \
                  ot_system.c \
                  ot_freertos.c \
                  ot_memory.c \
                  ot_ota_handler.c\
                  soft_source_match_table.c \

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))
COMPONENT_SRCDIRS := .