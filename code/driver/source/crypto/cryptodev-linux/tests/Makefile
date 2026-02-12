###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
PWD := $(shell pwd)
OPENSSL_INCLUDE_DIR = $(NVT_DRIVER_DIR)/../lib/external/openssl-1.0.2d/include
ROOTFS_DIR = $(PWD)/../../../target/rootfs-cpio/
LIBRARY_DIR = $(ROOTFS_DIR)/lib

CC    := $(CROSS_COMPILE)gcc
AR    := $(CROSS_COMPILE)ar
LD    := $(CROSS_COMPILE)ld
AS    := $(CROSS_COMPILE)as
STRIP := $(CROSS_COMPILE)strip

CFLAGS += -I.. $(CRYPTODEV_CFLAGS) -Wall -Werror -I$(OPENSSL_INCLUDE_DIR)

#comp_progs := cipher-thread cipher_comp hash_comp hmac_comp
comp_progs := cipher-thread

hostprogs := cipher cipher-aead hmac speed async_cipher async_hmac \
	async_speed sha_speed hashcrypt_speed fullspeed cipher-gcm \
	cipher-aead-srtp $(comp_progs)

example-cipher-objs := cipher.o
example-cipher-aead-objs := cipher-aead.o
example-hmac-objs := hmac.o
example-speed-objs := speed.c
example-fullspeed-objs := fullspeed.c
example-sha-speed-objs := sha_speed.c
example-async-cipher-objs := async_cipher.o
example-async-hmac-objs := async_hmac.o
example-async-speed-objs := async_speed.o
example-hashcrypt-speed-objs := hashcrypt_speed.c

#prefix ?= /usr/local
#execprefix ?= $(prefix)
#bindir = $(execprefix)/bin
bindir = $(ROOTFS_DIR)/gm/bin/

all: $(hostprogs)

check: $(hostprogs)
	./cipher
	./hmac
	./async_cipher
	./async_hmac
	./cipher-aead-srtp
	./cipher-gcm
	./cipher-aead

install:
	install -d $(DESTDIR)/$(bindir)
	for prog in $(hostprogs); do \
		install -m 755 $$prog $(DESTDIR)/$(bindir); \
	done

clean:
	rm -f *.o *~ $(hostprogs)

#${comp_progs}: LDLIBS += -L$(LIBRARY_DIR) -lssl -lcrypto -lpthread
#${comp_progs}: %: %.o openssl_wrapper.o

${comp_progs}: LDLIBS += -L$(LIBRARY_DIR) -lpthread
${comp_progs}: %: %.o

.PHONY: all clean check install

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

modules:
	@echo "nothing to be done for cryptodev tests"

clean:
	@echo "nothing to be done for cryptodev tests"

modules_install:
	@echo "nothing to be done for cryptodev tests"

endif
