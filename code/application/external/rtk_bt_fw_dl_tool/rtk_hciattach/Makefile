CFLAGS := -Wall -g
CC := $(CROSS_COMPILE)gcc
OBJS := hciattach.o hciattach_rtk.o hciattach_h4.o rtb_fwc.o

all: $(OBJS)
	$(CROSS_COMPILE)gcc -o rtk_hciattach $(OBJS)

%.o: %.c
	$(CROSS_COMPILE)gcc -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS)  rtk_hciattach

