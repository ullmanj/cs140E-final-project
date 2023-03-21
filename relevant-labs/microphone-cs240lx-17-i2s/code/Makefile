# list out the driver program source
# PROGS = $(wildcard tests/*.c)
# PROGS = tests/1-i2s-test.c
# PROGS = tests/2-i2s-dump.c
# PROGS = tests/3-i2s-audiovis.c
# PROGS = tests/4-fft-test.c
# PROGS = tests/5-fft-time.c
# PROGS = tests/6-fft-freqout.c
# PROGS = tests/7-fft-freqviz.c
# PROGS = tests/8-fft-buckets.c
# PROGS = tests/10-server.c
PROGS = tests/11-client.c

OPT_LEVEL = -O3

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
COMMON_SRC = i2s.c fft.c
COMMON_SRC += $(CS140E_2023_PATH)/../relevant-labs/lights-cs240lx-4-ws2812b/code/neopixel.c
COMMON_SRC += $(CS140E_2023_PATH)/../relevant-labs/16-nrf-networking/code/nrf-public.c
COMMON_SRC += $(CS140E_2023_PATH)/../relevant-labs/16-nrf-networking/code/nrf-driver.c
COMMON_SRC += $(CS140E_2023_PATH)/../relevant-labs/16-nrf-networking/code/nrf-hw-support.c

STAFF_OBJS := $(CS140E_2023_PATH)/libpi/staff-objs/new-spi.o

LIBS += ./libgcc.a

# define this if you need to give the device for your pi
TTYUSB = 

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
# SUPPORT_OBJS := $(SRC:.c=.o)
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template
