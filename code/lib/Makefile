include $(NVT_PRJCFG_MODEL_CFG)
include $(BUILD_DIR)/definitions.mk

APPSUBDIRS := $(filter $(NVT_CFG_APP) external, $(subst /,,$(subst $(APP_DIR)/, , $(dir $(wildcard $(APP_DIR)/*/Makefile $(APP_DIR)/*/makefile)))))
APP_EXTLIST := $(shell cd $(APP_DIR)/external; make -s list)
LIBSUBDIRS := $(filter-out %external, $(subst /,,$(subst $(LIBRARY_DIR)/source, , $(dir $(wildcard $(LIBRARY_DIR)/source/*/Makefile $(LIBRARY_DIR)/source/*/makefile)))))
LIB_EXTLIST := $(shell cd $(LIBRARY_DIR)/external; make -s list;)
BUILD_LIST := $(LIB_EXTLIST:%=nvt@ext@lib@%) $(APP_EXTLIST:%=nvt@ext@app@%) $(LIBSUBDIRS:%=nvt@lib@%)

all:
	@$(call makedep, $(BUILD_LIST))

install:
	@$(call makelist, $(addprefix source/, $(LIBSUBDIRS)), install)
	@cd $(LIBRARY_DIR)/external; make install;

clean:
	@$(call makelist, $(addprefix source/, $(LIBSUBDIRS)), clean)
	@cd $(LIBRARY_DIR)/external; make clean;
	@rm -rf $(LIBRARY_DIR)/output
	@rm -vf `find -maxdepth 1 -type 'l' -o -type 'f' | grep -v Makefile`
