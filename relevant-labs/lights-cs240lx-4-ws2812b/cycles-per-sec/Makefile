# list out the driver program source 
TEST_SRC = cycles-per-sec.c

# pick one of your attachment points (first or last) consistently.
BOOTLOADER = my-install

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
# SRC := stop-inline.c
# STAFF_OBJS = staff-hc-sr04.o

OPT_LEVEL = -O3

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

LIBS = $(CS140E_2023_PATH)/lib/libgcc.a

DEPS = ./Makefile
SUPPORT_OBJS := $(SRC:.c=.o)
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template

