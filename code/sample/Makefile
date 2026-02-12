include $(NVT_PRJCFG_MODEL_CFG)

SUBDIRS := $(dir $(wildcard */Makefile */makefile))

all:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n"; \
		$(MAKE) -C $$n || exit "$$?"; \
		echo "Done!"; \
		echo " "; \
	done

install:
ifneq ($(NVT_SAMPLES_INSTALL),DISABLE)
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n install"; \
		$(MAKE) -C $$n install; \
		echo "Done!"; \
		echo " "; \
	done
endif

clean:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n clean"; \
		$(MAKE) -C $$n clean; \
		echo "Done!"; \
		echo " "; \
	done
