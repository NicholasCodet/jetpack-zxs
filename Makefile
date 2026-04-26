# Minimal devkitPro/libgba Makefile for this project.
# It builds jetpack_gba.gba from sources under src/.

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

LIBGBA_INC := $(DEVKITPRO)/libgba/include
LIBGBA_LIB := $(DEVKITPRO)/libgba/lib

ifeq ($(wildcard $(DEVKITARM)),)
$(error "devkitARM not found at $(DEVKITARM)")
endif
ifeq ($(wildcard $(LIBGBA_INC)),)
$(error "libgba headers not found at $(LIBGBA_INC)")
endif
ifeq ($(wildcard $(LIBGBA_LIB)/libgba.a),)
$(error "libgba library not found at $(LIBGBA_LIB)/libgba.a")
endif

TARGET := roms/jetpack-zxs
BUILD := build
SOURCES := src

ARCH := -mthumb -mthumb-interwork

CFLAGS := -g -Wall -O2 $(ARCH) -I$(LIBGBA_INC) -I$(PROJECT_ROOT)/include
CXXFLAGS := $(CFLAGS)
LDFLAGS := -g $(ARCH)
LD := $(DEVKITARM)/bin/arm-none-eabi-gcc
LIBS := -L$(LIBGBA_LIB) -lgba

include $(DEVKITARM)/gba_rules

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@mkdir -p $@ $(dir $(TARGET))
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba

else

$(OUTPUT).gba: $(OFILES)

endif
