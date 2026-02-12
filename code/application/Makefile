include $(NVT_PRJCFG_MODEL_CFG)
include $(BUILD_DIR)/definitions.mk

APP_DIR ?= $(PWD)
APPSUBDIRS := $(subst /,,$(filter $(patsubst %,%/,$(NVT_CFG_APP)), $(subst source/, , $(dir $(wildcard source/*/Makefile source/*/makefile)))))
APP_EXTLIST := $(shell cd $(APP_DIR)/external; make -s list)
LIBSUBDIRS := $(filter-out %external, $(subst /,,$(subst $(LIBRARY_DIR)/source, , $(dir $(wildcard $(LIBRARY_DIR)/source/*/Makefile $(LIBRARY_DIR)/source/*/makefile)))))
LIB_EXTLIST := $(shell cd $(LIBRARY_DIR)/external; make -s list)
BUILD_LIST := $(LIB_EXTLIST:%=nvt@ext@lib@%) $(LIBSUBDIRS:%=nvt@lib@%) $(APP_EXTLIST:%=nvt@ext@app@%) $(APPSUBDIRS)

all:
	@$(call makedep, $(BUILD_LIST))

listpackage:
	@echo -e "\e[1;36mAvailable packages: \e[0m"
	@for n in $(BUILD_LIST); do \
		echo -e "\e[1;33m\t$$n \e[0m"; \
	done;
	@echo -e "\e[1;36mAdd \"DEP_LIBRARIES= package_name\" to your Makefile can support dependent build.\e[0m"

install:
	@$(call makelist, $(addprefix source/, $(APPSUBDIRS)), install)
	@cd $(APP_DIR)/external; make install;

clean:
	@$(call makelist, $(addprefix source/, $(APPSUBDIRS)), clean)
	@cd $(APP_DIR)/external; make clean;
	@rm -rf $(APP_DIR)/output
