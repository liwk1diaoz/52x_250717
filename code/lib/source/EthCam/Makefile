include $(NVT_PRJCFG_MODEL_CFG)
SUBDIRS := $(dir $(shell find . -name Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)

.PHONY: all install clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

install:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n install; \
	done

clean:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean; \
	done
	@rm ../output -rf


