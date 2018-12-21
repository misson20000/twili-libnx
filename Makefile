.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET := libtwili
INCLUDES := include

ARCH := -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

LIBS  := -lnx
LIBDIRS  := $(PORTLIBS) $(LIBNX)

OFILES := build/twili.o
INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
      $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
      -I. \
      -iquote $(CURDIR)/include/switch/

CFLAGS := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS += $(INCLUDE) -D__SWITCH__
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

CFILES := src/twili.c

.PHONY: clean all install

all: lib/$(TARGET).a

install: lib/libtwili.a
	install -D -t $(DESTDIR)/lib lib/libtwili.a
	install -D -t $(DESTDIR)/include include/twili.h

build/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

lib/$(TARGET).a: $(OFILES)
	mkdir -p $(@D)
	$(AR) -rc $@ $^

clean:
	@echo clean ...
	@rm -fr lib build
