# list out the driver program source 
# PROGS = 2-neopix.c   0-timing-check.c # simple.c
# PROGS = 1-blink.c
PROGS = 0-timing-check.c
# PROGS = 2-neopix.c

OPT_LEVEL = -O1 # changed from -03

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
COMMON_SRC := neopixel.c

# define this if you need to give the device for your pi
TTYUSB = 

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
# COMMON_OBJS := $(SRC:.c=.o)
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template
