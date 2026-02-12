MODULE_NAME = libheif
# DIRs

RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
BUILD_DIR = build_heif
BUILD_LIB_DIR = $(BUILD_DIR)/lib
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)
HEIF_LIB = heif
HEIF_LIB_SRC = $(HEIF_LIB)/srcs
HEIF_PATCH = heifnvt_001.patch

.PHONY: all clean clean_src download

ifeq ("$(wildcard */*/*/*.cpp)","")

all:
	@echo "please download heif source by 'make download' , nothing to be done for '$(OUTPUT_NAME)'"

download:
	@git clone https://github.com/nokiatech/heif.git
	@patch -i $(HEIF_PATCH) -p 4
	@echo "download complete!"

else

all:
	@echo Creating library $(MODULE_NAME) ...
	@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR) ; fi
	@cd $(BUILD_DIR) && cmake ../$(HEIF_LIB_SRC) -DBUILD_ONLY_STATIC_LIB=1 -DWITHOUT_EXAMPLE=1 -DCMAKE_TOOLCHAIN_FILE=../cross_compiler_for_arm.cmake && cmake --build .

download:
	@echo "heif source already exist , remove source by 'rm -rf heif'"

endif

clean_src:
	@rm -rf $(HEIF_LIB)

clean:
	@rm -rf $(BUILD_DIR)
	@if [ -e $(OUTPUT_DIR)/libheif_static.a ] ; then rm $(OUTPUT_DIR)/libheif*.a ; fi

install:
	@if [ -d $(BUILD_LIB_DIR) ]; then mkdir -p $(OUTPUT_DIR); cp -avf $(BUILD_LIB_DIR)/*.a $(OUTPUT_DIR); fi

