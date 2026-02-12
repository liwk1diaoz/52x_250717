MODULE_NAME = liblvgl
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51055
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source
RTOS_LWIP_DIR = $(KERNELDIR)/lib/third_party/lwip/src
RTOS_LWIP_SRC_DIR = $(RTOS_LWIP_DIR)



.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)

EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I../../include \
	-I$(KDRV_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(NVT_HDAL_DIR)/vendor/media/include \

#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -DSOCKLEN_T=socklen_t -D__LINUX_USER__ -D_TODO_=0
C_PREDEFINED    = -D_NVT_CONSOLE_
C_CFLAGS       = $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED)
LD_FLAGS	= -fPIC -shared
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
	lvgl/src/lv_font/lv_font.c \
	lvgl/src/lv_font/lv_font_fmt_txt.c \
	lvgl/src/lv_font/lv_font_loader.c \
	lvgl/src/lv_font/lv_font_montserrat_12.c \
	lvgl/src/lv_font/lv_font_montserrat_14.c \
	lvgl/src/lv_font/lv_font_montserrat_16.c \
	lvgl/src/lv_font/lv_font_montserrat_18.c \
	lvgl/src/lv_font/lv_font_montserrat_20.c \
	lvgl/src/lv_font/lv_font_montserrat_22.c \
	lvgl/src/lv_font/lv_font_montserrat_24.c \
	lvgl/src/lv_font/lv_font_montserrat_26.c \
	lvgl/src/lv_font/lv_font_montserrat_28.c \
	lvgl/src/lv_font/lv_font_montserrat_30.c \
	lvgl/src/lv_font/lv_font_montserrat_32.c \
	lvgl/src/lv_font/lv_font_montserrat_34.c \
	lvgl/src/lv_font/lv_font_montserrat_36.c \
	lvgl/src/lv_font/lv_font_montserrat_38.c \
	lvgl/src/lv_font/lv_font_montserrat_40.c \
	lvgl/src/lv_font/lv_font_montserrat_42.c \
	lvgl/src/lv_font/lv_font_montserrat_44.c \
	lvgl/src/lv_font/lv_font_montserrat_46.c \
	lvgl/src/lv_font/lv_font_montserrat_48.c \
	lvgl/src/lv_font/lv_font_montserrat_12_subpx.c \
	lvgl/src/lv_font/lv_font_montserrat_28_compressed.c \
	lvgl/src/lv_font/lv_font_unscii_8.c \
	lvgl/src/lv_font/lv_font_dejavu_16_persian_hebrew.c \
	lvgl/src/lv_core/lv_group.c \
	lvgl/src/lv_core/lv_indev.c \
	lvgl/src/lv_core/lv_disp.c \
	lvgl/src/lv_core/lv_obj.c \
	lvgl/src/lv_core/lv_refr.c \
	lvgl/src/lv_core/lv_style.c \
	lvgl/src/lv_draw/lv_draw_mask.c \
	lvgl/src/lv_draw/lv_draw_blend.c \
	lvgl/src/lv_draw/lv_draw_rect.c \
	lvgl/src/lv_draw/lv_draw_label.c \
	lvgl/src/lv_draw/lv_draw_line.c \
	lvgl/src/lv_draw/lv_draw_img.c \
	lvgl/src/lv_draw/lv_draw_arc.c \
	lvgl/src/lv_draw/lv_draw_triangle.c \
	lvgl/src/lv_draw/lv_img_decoder.c \
	lvgl/src/lv_draw/lv_img_cache.c \
	lvgl/src/lv_draw/lv_img_buf.c \
	lvgl/src/lv_widgets/lv_arc.c \
	lvgl/src/lv_widgets/lv_bar.c \
	lvgl/src/lv_widgets/lv_checkbox.c \
	lvgl/src/lv_widgets/lv_cpicker.c \
	lvgl/src/lv_widgets/lv_dropdown.c \
	lvgl/src/lv_widgets/lv_keyboard.c \
	lvgl/src/lv_widgets/lv_line.c \
	lvgl/src/lv_widgets/lv_msgbox.c \
	lvgl/src/lv_widgets/lv_spinner.c \
	lvgl/src/lv_widgets/lv_roller.c \
	lvgl/src/lv_widgets/lv_table.c \
	lvgl/src/lv_widgets/lv_tabview.c \
	lvgl/src/lv_widgets/lv_tileview.c \
	lvgl/src/lv_widgets/lv_btn.c \
	lvgl/src/lv_widgets/lv_calendar.c \
	lvgl/src/lv_widgets/lv_chart.c \
	lvgl/src/lv_widgets/lv_canvas.c \
	lvgl/src/lv_widgets/lv_gauge.c \
	lvgl/src/lv_widgets/lv_label.c \
	lvgl/src/lv_widgets/lv_list.c \
	lvgl/src/lv_widgets/lv_slider.c \
	lvgl/src/lv_widgets/lv_textarea.c \
	lvgl/src/lv_widgets/lv_spinbox.c \
	lvgl/src/lv_widgets/lv_btnmatrix.c \
	lvgl/src/lv_widgets/lv_cont.c \
	lvgl/src/lv_widgets/lv_img.c \
	lvgl/src/lv_widgets/lv_imgbtn.c \
	lvgl/src/lv_widgets/lv_led.c \
	lvgl/src/lv_widgets/lv_linemeter.c \
	lvgl/src/lv_widgets/lv_page.c \
	lvgl/src/lv_widgets/lv_switch.c \
	lvgl/src/lv_widgets/lv_win.c \
	lvgl/src/lv_widgets/lv_objmask.c \
	lvgl/src/lv_themes/lv_theme.c \
	lvgl/src/lv_themes/lv_theme_material.c \
	lvgl/src/lv_themes/lv_theme_mono.c \
	lvgl/src/lv_themes/lv_theme_empty.c \
	lvgl/src/lv_themes/lv_theme_template.c \
	lvgl/src/lv_misc/lv_area.c \
	lvgl/src/lv_misc/lv_task.c \
	lvgl/src/lv_misc/lv_fs.c \
	lvgl/src/lv_misc/lv_anim.c \
	lvgl/src/lv_misc/lv_mem.c \
	lvgl/src/lv_misc/lv_ll.c \
	lvgl/src/lv_misc/lv_color.c \
	lvgl/src/lv_misc/lv_txt.c \
	lvgl/src/lv_misc/lv_txt_ap.c \
	lvgl/src/lv_misc/lv_math.c \
	lvgl/src/lv_misc/lv_log.c \
	lvgl/src/lv_misc/lv_gc.c \
	lvgl/src/lv_misc/lv_utils.c \
	lvgl/src/lv_misc/lv_async.c \
	lvgl/src/lv_misc/lv_printf.c \
	lvgl/src/lv_misc/lv_bidi.c \
	lvgl/src/lv_misc/lv_debug.c \
	lvgl/src/lv_hal/lv_hal_disp.c \
	lvgl/src/lv_hal/lv_hal_indev.c \
	lvgl/src/lv_hal/lv_hal_tick.c \
	lvgl/src/lv_gpu/lv_gpu_nvt_dma2d.c \


OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)
	@$(AR) rcs $(MODULE_NAME).a $(OBJ)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) $< -c -o $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
	@rm -f $(OUTPUT_DIR)/liblvgl.*
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.so* *.a $(OUTPUT_DIR)
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = $(MODULE_NAME).a
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek \
	-I../../include \
	-I$(NVT_HDAL_DIR)/vendor/media/include \
	-I$(NVT_HDAL_DIR)/include \

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format -D_TODO_=0 -Wno-inline

SRC = \
	lvgl/src/lv_font/lv_font.c \
	lvgl/src/lv_font/lv_font_fmt_txt.c \
	lvgl/src/lv_font/lv_font_loader.c \
	lvgl/src/lv_font/lv_font_montserrat_12.c \
	lvgl/src/lv_font/lv_font_montserrat_14.c \
	lvgl/src/lv_font/lv_font_montserrat_16.c \
	lvgl/src/lv_font/lv_font_montserrat_18.c \
	lvgl/src/lv_font/lv_font_montserrat_20.c \
	lvgl/src/lv_font/lv_font_montserrat_22.c \
	lvgl/src/lv_font/lv_font_montserrat_24.c \
	lvgl/src/lv_font/lv_font_montserrat_26.c \
	lvgl/src/lv_font/lv_font_montserrat_28.c \
	lvgl/src/lv_font/lv_font_montserrat_30.c \
	lvgl/src/lv_font/lv_font_montserrat_32.c \
	lvgl/src/lv_font/lv_font_montserrat_34.c \
	lvgl/src/lv_font/lv_font_montserrat_36.c \
	lvgl/src/lv_font/lv_font_montserrat_38.c \
	lvgl/src/lv_font/lv_font_montserrat_40.c \
	lvgl/src/lv_font/lv_font_montserrat_42.c \
	lvgl/src/lv_font/lv_font_montserrat_44.c \
	lvgl/src/lv_font/lv_font_montserrat_46.c \
	lvgl/src/lv_font/lv_font_montserrat_48.c \
	lvgl/src/lv_font/lv_font_montserrat_12_subpx.c \
	lvgl/src/lv_font/lv_font_montserrat_28_compressed.c \
	lvgl/src/lv_font/lv_font_unscii_8.c \
	lvgl/src/lv_font/lv_font_dejavu_16_persian_hebrew.c \
	lvgl/src/lv_core/lv_group.c \
	lvgl/src/lv_core/lv_indev.c \
	lvgl/src/lv_core/lv_disp.c \
	lvgl/src/lv_core/lv_obj.c \
	lvgl/src/lv_core/lv_refr.c \
	lvgl/src/lv_core/lv_style.c \
	lvgl/src/lv_draw/lv_draw_mask.c \
	lvgl/src/lv_draw/lv_draw_blend.c \
	lvgl/src/lv_draw/lv_draw_rect.c \
	lvgl/src/lv_draw/lv_draw_label.c \
	lvgl/src/lv_draw/lv_draw_line.c \
	lvgl/src/lv_draw/lv_draw_img.c \
	lvgl/src/lv_draw/lv_draw_arc.c \
	lvgl/src/lv_draw/lv_draw_triangle.c \
	lvgl/src/lv_draw/lv_img_decoder.c \
	lvgl/src/lv_draw/lv_img_cache.c \
	lvgl/src/lv_draw/lv_img_buf.c \
	lvgl/src/lv_widgets/lv_arc.c \
	lvgl/src/lv_widgets/lv_bar.c \
	lvgl/src/lv_widgets/lv_checkbox.c \
	lvgl/src/lv_widgets/lv_cpicker.c \
	lvgl/src/lv_widgets/lv_dropdown.c \
	lvgl/src/lv_widgets/lv_keyboard.c \
	lvgl/src/lv_widgets/lv_line.c \
	lvgl/src/lv_widgets/lv_msgbox.c \
	lvgl/src/lv_widgets/lv_spinner.c \
	lvgl/src/lv_widgets/lv_roller.c \
	lvgl/src/lv_widgets/lv_table.c \
	lvgl/src/lv_widgets/lv_tabview.c \
	lvgl/src/lv_widgets/lv_tileview.c \
	lvgl/src/lv_widgets/lv_btn.c \
	lvgl/src/lv_widgets/lv_calendar.c \
	lvgl/src/lv_widgets/lv_chart.c \
	lvgl/src/lv_widgets/lv_canvas.c \
	lvgl/src/lv_widgets/lv_gauge.c \
	lvgl/src/lv_widgets/lv_label.c \
	lvgl/src/lv_widgets/lv_list.c \
	lvgl/src/lv_widgets/lv_slider.c \
	lvgl/src/lv_widgets/lv_textarea.c \
	lvgl/src/lv_widgets/lv_spinbox.c \
	lvgl/src/lv_widgets/lv_btnmatrix.c \
	lvgl/src/lv_widgets/lv_cont.c \
	lvgl/src/lv_widgets/lv_img.c \
	lvgl/src/lv_widgets/lv_imgbtn.c \
	lvgl/src/lv_widgets/lv_led.c \
	lvgl/src/lv_widgets/lv_linemeter.c \
	lvgl/src/lv_widgets/lv_page.c \
	lvgl/src/lv_widgets/lv_switch.c \
	lvgl/src/lv_widgets/lv_win.c \
	lvgl/src/lv_widgets/lv_objmask.c \
	lvgl/src/lv_themes/lv_theme.c \
	lvgl/src/lv_themes/lv_theme_material.c \
	lvgl/src/lv_themes/lv_theme_mono.c \
	lvgl/src/lv_themes/lv_theme_empty.c \
	lvgl/src/lv_themes/lv_theme_template.c \
	lvgl/src/lv_misc/lv_area.c \
	lvgl/src/lv_misc/lv_task.c \
	lvgl/src/lv_misc/lv_fs.c \
	lvgl/src/lv_misc/lv_anim.c \
	lvgl/src/lv_misc/lv_mem.c \
	lvgl/src/lv_misc/lv_ll.c \
	lvgl/src/lv_misc/lv_color.c \
	lvgl/src/lv_misc/lv_txt.c \
	lvgl/src/lv_misc/lv_txt_ap.c \
	lvgl/src/lv_misc/lv_math.c \
	lvgl/src/lv_misc/lv_log.c \
	lvgl/src/lv_misc/lv_gc.c \
	lvgl/src/lv_misc/lv_utils.c \
	lvgl/src/lv_misc/lv_async.c \
	lvgl/src/lv_misc/lv_printf.c \
	lvgl/src/lv_misc/lv_bidi.c \
	lvgl/src/lv_misc/lv_debug.c \
	lvgl/src/lv_hal/lv_hal_disp.c \
	lvgl/src/lv_hal/lv_hal_indev.c \
	lvgl/src/lv_hal/lv_hal_tick.c \
	lvgl/src/lv_gpu/lv_gpu_nvt_dma2d.c \
	
copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all : $(OUTPUT_NAME)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

$(OUTPUT_NAME): $(OBJ)
	@echo Creating library $* ...
	@$(AR) rcsD $(OUTPUT_NAME) $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
endif

install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)

endif

