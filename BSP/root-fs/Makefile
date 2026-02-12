NVT_FS_MTD_CFG_DEFAULT := ${ROOTFS_DIR}/mtd_cfg.txt
NVT_FS_MTD_CFG := ${CONFIG_DIR}/cfg_gen/mtd_cfg.txt
COPYSO_CHK_BIN_PATH =bin sbin usr/bin usr/sbin usr/local/bin usr/local/sbin var/www var/www/onvif var/www/cgi-bin
COPYSO_SCRIPT = $(LINUX_BUILD_TOP)/build/copyso.sh
IMGNAME_RAMDISK="rootfs.ramdisk.bin"
IMGNAME_TARBZ2="rootfs.tar.bz2"
IMGNAME_UBIFS="rootfs.ubifs.bin"
IMGNAME_UBIFS_RW="rootfs_1.rw.ubifs.bin"
IMGNAME_SQUASHFS="rootfs.squash.bin"
IMGNAME_SQUASH_UBI="rootfs.squash_ubi.bin"
IMGNAME_SQUASHFS1="rootfs_1.squash.bin"
IMGNAME_JFFS2="rootfs.jffs2.bin"
IMGNAME_JFFS2_RW="rootfs_1.rw.jffs2.bin"
IMGNAME_EXT4="rootfs.ext4.bin"
IMGNAME_EXT4_RW="rootfs.rw.ext4.bin"
IMGNAME_FAT_CACHE="rootfs_1.cache.vfat.bin"
PARTITION_LABEL_UBIFS = rootfs
PARTITION_LABEL_UBIFS_RW = rootfs1
MODELCFG_PROFILE_PATH="${ROOTFS_DIR}/rootfs/etc/profile_prjcfg"
CLEAR_FILE_LIST="rootfs/etc/profile_prjcfg rootfs/linuxrc"
GEN_MODULES_DEP_BB_FILE = $(LINUX_BUILD_TOP)/build/nvt-tools/gen_modules_dep_bb
DTBNAME = nvt-na51055-evb.dtb

# disable python tool generate __pycache__ directory
export PYTHONDONTWRITEBYTECODE=1

KERVER ?= $(NVT_LINUX_VER)
include $(NVT_PRJCFG_MODEL_CFG)
-include $(NVT_FS_MTD_CFG_DEFAULT)
-include $(NVT_FS_MTD_CFG)

ifeq ($(NVT_BINARY_FILE_STRIP), yes)
STRIPDIRS = ${ROOTFS_DIR}/rootfs/bin/* ${ROOTFS_DIR}/rootfs/sbin/* ${ROOTFS_DIR}/rootfs/user/bin/* ${ROOTFS_DIR}/rootfs/usr/sbin/*
STRIPLIBS = ${ROOTFS_DIR}/rootfs/lib/* ${ROOTFS_DIR}/rootfs/usr/lib/*
endif

# for sdk_maker, use $SUDO_PWD to solving flow break on emmc-building
define gen_rootfs_init
	echo -e "\e[1;44mRootfs are generating.....\e[0m"
	if [ -z $(LINUX_BUILD_TOP) ]; then \
		echo -e "\r\nERROR :Please source build/envsetup.sh in NA51000_Linux firstly to have auto copyso function\r\n"; \
		exit 1; \
	fi
	if [ ! -e $(ROOTFS_DIR)/bin ]; then \
		mkdir $(ROOTFS_DIR)/bin; \
	else \
		if [ -d $(ROOTFS_DIR)/bin/mnt ]; then \
			if [ -z $(SUDO_PWD) ]; then \
				sudo umount $(ROOTFS_DIR)/bin/mnt; \
			else \
				echo "$(SUDO_PWD)" | sudo -S umount $(ROOTFS_DIR)/bin/mnt; \
			fi;\
		fi; \
		rm -rf $(ROOTFS_DIR)/bin/*; \
        fi
	if [ ! -z $(LINUX_BUILD_TOP) ]; then \
		sh $(COPYSO_SCRIPT) "$(ROOTFS_DIR)/rootfs" $(COPYSO_CHK_BIN_PATH); \
	fi
    if [ -f "$(GEN_MODULES_DEP_BB_FILE)" ]; then \
        echo "$(GEN_MODULES_DEP_BB_FILE) $(ROOTFS_DIR)/lib/modules/$(KERVER)"; \
        $(GEN_MODULES_DEP_BB_FILE) $(ROOTFS_DIR)/rootfs/lib/modules/$(KERVER); \
    else \
        echo "WRANING: can not find $(GEN_MODULES_DEP_BB_FILE)"; \
    fi

endef

define gen_rootfs_etc_link
	if [ ! -z $(NVT_ROOTFS_ETC) ]; then \
		cd $(ROOTFS_DIR)/rootfs; rm -rf etc; ln -s etc_Model/etc_$(NVT_ROOTFS_ETC) etc; \
	else \
		MODEL=`echo $(NVT_PRJCFG_MODEL_CFG) | awk -F'/' '{print $$(NF-1)}' | awk -F'cfg_' '{print $$NF}'`; \
		cd $(ROOTFS_DIR)/rootfs; rm -rf etc; ln -s etc_Model/etc_$$MODEL etc; \
		echo -e "\e[1;32mYour model is $$MODEL\e[0m"; \
	fi
endef

# function: gen_rootfs_ubinize_cfg_param
# $(1) : ubi fs file, e.g. $(IMGNAME_UBIFS).raw
# $(2) : output cfg file, e.g. bin/ubinize.cfg
# $(3) : vol_size, e.g. ROOTFS_UBI_MAX_LEB_COUNT * ROOTFS_UBI_ERASE_BLK_SIZE
gen_rootfs_ubinize_cfg_param = \
	echo -e "[ubifs]"           > $(2); \
	echo -e "mode=ubi"         >> $(2); \
	echo -e "image=$(1)"       >> $(2); \
	echo -e "vol_id=0"         >> $(2); \
	echo -e "vol_size=$(3)"    >> $(2); \
	echo -e "vol_type=dynamic" >> $(2); \
	echo -e "vol_name=rootfs"  >> $(2);

define gen_rootfs_tmp
	rm -rf $(ROOTFS_DIR)/rootfs.tmp;
	mkdir $(ROOTFS_DIR)/rootfs.tmp; cd $(ROOTFS_DIR)/rootfs; cp -arf `ls | grep -v etc` $(ROOTFS_DIR)/rootfs.tmp;
	@# Generate etc folder
	@# List all etc folder files
	$(eval etc_files_list=`cd $(ROOTFS_DIR)/rootfs/etc; find .`)
	cd $(ROOTFS_DIR)/rootfs/etc/; \
	for file in $(etc_files_list); do \
		if [ -L $$file ]; then \
			mkdir -p $(ROOTFS_DIR)/rootfs.tmp/etc/`dirname $$file`; \
			cp -rf $$file $(ROOTFS_DIR)/rootfs.tmp/etc/$$file; \
		elif [ -d $$file ]; then \
			mkdir -p $(ROOTFS_DIR)/rootfs.tmp/etc/$$file; \
		else \
			mkdir -p $(ROOTFS_DIR)/rootfs.tmp/etc/`dirname $$file`; \
			cp -f $$file $(ROOTFS_DIR)/rootfs.tmp/etc/$$file; \
		fi \
	done
	if [ "$(NVT_ROOTFS_RW_PART_EN)" != "NVT_ROOTFS_RW_PART_EN_OFF" ] || [ -z "$(NVT_ROOTFS_RW_PART_EN)" ]; then \
		if [[ ( "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_RAMDISK" && "$(EMBMEM)" != "EMBMEM_NONE" ) \
			|| ( "$(EMBMEM)" != "EMBMEM_SPI_NOR" && "$(EMBMEM)" != "EMBMEM_NONE" && "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ) \
			|| ( "$(NVT_ROOTFS_RW_PART_EN)" == "NVT_ROOTFS_RW_PART_EN_ON" ) ]]; then \
			rm -rf $(ROOTFS_DIR)/rootfs.tmp/lib/modules/*; \
			find $(ROOTFS_DIR)/rootfs.tmp/usr/bin/ -type f -exec rm {} \;; \
			find $(ROOTFS_DIR)/rootfs.tmp/usr/lib/ -type f -exec rm {} \;; \
		fi; \
	fi
endef

define gen_selected_rootfs_ro
	echo -e "\e[1;32m$(NVT_ROOTFS_TYPE).....\e[0m"
	if [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_NAND_UBI" ]; then \
		./verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS) -b $(ROOTFS_UBI_MAX_LEB_COUNT) && \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
		cd $(ROOTFS_DIR); mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_MAX_LEB_COUNT) -o bin/$(IMGNAME_UBIFS).raw -r rootfs.tmp; \
		$(call gen_rootfs_ubinize_cfg_param,bin/$(IMGNAME_UBIFS).raw,bin/ubinize.cfg,$$(($(ROOTFS_UBI_MAX_LEB_COUNT) * $(ROOTFS_UBI_ERASE_BLK_SIZE)))) \
		ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(IMGNAME_UBIFS).ubi.raw bin/ubinize.cfg; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_UBIFS).ubi.raw --packsum-dst=bin/$(IMGNAME_UBIFS) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ] && [ "$(EMBMEM)" == "EMBMEM_SPI_NAND" ]; then \
		cd $(ROOTFS_DIR); mksquashfs rootfs.tmp bin/$(IMGNAME_SQUASH_UBI).raw -comp $(ROOTFS_SQ_COMPRESS_MODE) -b $(ROOTFS_SQ_BLK_SIZE) -root-owned; \
		$(call gen_rootfs_ubinize_cfg_param,bin/$(IMGNAME_SQUASH_UBI).raw,bin/ubinize.cfg,$$(($(ROOTFS_UBI_MAX_LEB_COUNT) * $(ROOTFS_UBI_ERASE_BLK_SIZE))))\
		ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(IMGNAME_SQUASH_UBI).ubi.raw bin/ubinize.cfg; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_SQUASH_UBI).ubi.raw --packsum-dst=bin/$(IMGNAME_SQUASH_UBI) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ]; then \
		cd $(ROOTFS_DIR); mksquashfs rootfs.tmp bin/$(IMGNAME_SQUASHFS).raw -comp $(ROOTFS_SQ_COMPRESS_MODE) -b $(ROOTFS_SQ_BLK_SIZE) -root-owned; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_SQUASHFS).raw --packsum-dst=bin/$(IMGNAME_SQUASHFS) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_NAND_JFFS2" ]; then \
		cd $(ROOTFS_DIR); mkfs.jffs2 -n -s $(ROOTFS_JFFS2_PAGE_SIZE) -e $(ROOTFS_JFFS2_BLK_SIZE) -r rootfs.tmp -U -X $(ROOTFS_JFFS2_COMPRESS_MODE) -p $(ROOTFS_JFFS2_SIZE) -o bin/$(IMGNAME_JFFS2).raw; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_JFFS2).raw --packsum-dst=bin/$(IMGNAME_JFFS2) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_NOR_JFFS2" ]; then \
		cd $(ROOTFS_DIR); mkfs.jffs2 -n -s $(ROOTFS_JFFS2_PAGE_SIZE) -e $(ROOTFS_JFFS2_BLK_SIZE) -r rootfs.tmp -U -X $(ROOTFS_JFFS2_COMPRESS_MODE) -o bin/$(IMGNAME_JFFS2).raw; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_JFFS2).raw --packsum-dst=bin/$(IMGNAME_JFFS2) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_EXT4" ]; then \
		cd $(ROOTFS_DIR); make_ext4fs -s -l $(ROOTFS_EXT4_SIZE) bin/$(IMGNAME_EXT4).raw rootfs.tmp; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_EXT4).raw --packsum-dst=bin/$(IMGNAME_EXT4) --packsum-type=0x9; \
	else \
		cd $(ROOTFS_DIR)/rootfs.tmp; find . | cpio -o -H newc > $(ROOTFS_DIR)/bin/initramfs.cpio; \
		if [ "$(NVT_LINUX_COMPRESS)" == "NVT_LINUX_COMPRESS_GZ" ] || [ "$(NVT_LINUX_COMPRESS)" == "NVT_LINUX_COMPRESS_NONE" ]; then \
			$(BUILD_DIR)/nvt-tools/bfc c gz $(ROOTFS_DIR)/bin/initramfs.cpio $(ROOTFS_DIR)/bin/$(IMGNAME_RAMDISK).raw 0 0 -align; \
		else \
			if [ ! -x $( command -v lz4) ]; then \
				echo 'Error: lz4 is not installed.'; \
				exit 1; \
			fi; \
			lz4 -l $(ROOTFS_DIR)/bin/initramfs.cpio $(ROOTFS_DIR)/bin/initramfs.cpio.lz4; \
			$(ROOTFS_DIR)/tools/mkimage -A arm -T ramdisk -C none -n 'NVT initramfs' -d $(ROOTFS_DIR)/bin/initramfs.cpio.lz4 $(ROOTFS_DIR)/bin/$(IMGNAME_RAMDISK).raw; \
		fi; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=$(ROOTFS_DIR)/bin/$(IMGNAME_RAMDISK).raw --packsum-dst=$(ROOTFS_DIR)/bin/$(IMGNAME_RAMDISK) --packsum-type=0x9; \
	fi
	@# tar.bz2 generate
	rm -rf $(ROOTFS_DIR)/bin/$(IMGNAME_TARBZ2); cd $(ROOTFS_DIR)/rootfs.tmp; tar -jcf $(ROOTFS_DIR)/bin/$(IMGNAME_TARBZ2) .
endef

define rm_rootfs_tmp_dir
	rm -rf $(ROOTFS_DIR)/rootfs.tmp
endef

define gen_selected_rootfs_rw
	if [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_NAND_UBI" ]; then \
		./verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS_RW) -b $(ROOTFS_UBI_RW_MAX_LEB_COUNT) && \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
		cd $(ROOTFS_DIR); mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_RW_MAX_LEB_COUNT) -o bin/$(IMGNAME_UBIFS_RW).raw -r rootfs.tmp; \
		$(call gen_rootfs_ubinize_cfg_param,bin/$(IMGNAME_UBIFS_RW).raw,bin/ubinize_rw.cfg,$$(($(ROOTFS_UBI_RW_MAX_LEB_COUNT) * $(ROOTFS_UBI_ERASE_BLK_SIZE)))) \
		ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(IMGNAME_UBIFS_RW).ubi.raw bin/ubinize_rw.cfg; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_UBIFS_RW).ubi.raw --packsum-dst=bin/$(IMGNAME_UBIFS_RW) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_NAND_JFFS2" ]; then \
		cd $(ROOTFS_DIR); mkfs.jffs2 -n -s $(ROOTFS_JFFS2_PAGE_SIZE) -e $(ROOTFS_JFFS2_BLK_SIZE) -r rootfs.tmp -U -X $(ROOTFS_JFFS2_COMPRESS_MODE) -p $(ROOTFS_JFFS2_RW_SIZE) -o bin/$(IMGNAME_JFFS2_RW).raw; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_JFFS2_RW).raw --packsum-dst=bin/$(IMGNAME_JFFS2_RW) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_EXT4" ]; then \
		cd $(ROOTFS_DIR); $(BUILD_DIR)/gen_vfat_image.sh bin/$(IMGNAME_FAT_CACHE).raw tools/rootfs_recovery `expr $(ROOTFS_FAT_CACHE_SIZE) / 1024 / 1024`; \
		$(BUILD_DIR)/nvt-tools/reduce_fat_bin.py bin/$(IMGNAME_FAT_CACHE).raw bin/$(IMGNAME_FAT_CACHE).raw.sparse; \
		$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_FAT_CACHE).raw.sparse --packsum-dst=bin/$(IMGNAME_FAT_CACHE) --packsum-type=0x9; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ] && [ "$(EMBMEM)" != "EMBMEM_NONE" ]; then \
		if [ "$(EMBMEM)" == "EMBMEM_EMMC" ]; then \
			cd $(ROOTFS_DIR); make_ext4fs -s -l $(ROOTFS_EXT4_SIZE) bin/$(IMGNAME_EXT4_RW).raw rootfs.tmp; \
			$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_EXT4_RW).raw --packsum-dst=bin/$(IMGNAME_EXT4_RW) --packsum-type=0x9; \
		elif [ "$(EMBMEM)" == "EMBMEM_SPI_NAND" ]; then \
			./verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS_RW) -b $(ROOTFS_UBI_RW_MAX_LEB_COUNT) && \
			if [ $$? -ne 0 ]; then \
				exit 1; \
			fi; \
			cd $(ROOTFS_DIR); mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_RW_MAX_LEB_COUNT) -o bin/$(IMGNAME_UBIFS_RW).raw -r rootfs.tmp; \
			$(call gen_rootfs_ubinize_cfg_param,bin/$(IMGNAME_UBIFS_RW).raw,bin/ubinize_rw.cfg,$$(($(ROOTFS_UBI_RW_MAX_LEB_COUNT) * $(ROOTFS_UBI_ERASE_BLK_SIZE)))) \
			ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(IMGNAME_UBIFS_RW).ubi.raw bin/ubinize_rw.cfg; \
			$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_UBIFS_RW).ubi.raw --packsum-dst=bin/$(IMGNAME_UBIFS_RW) --packsum-type=0x9; \
		fi; \
	elif [ "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_RAMDISK" ] && [ "$(EMBMEM)" != "EMBMEM_NONE" ]; then \
		if [ "$(EMBMEM)" == "EMBMEM_SPI_NAND" ]; then \
			./verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS_RW) -b $(ROOTFS_UBI_RW_MAX_LEB_COUNT); \
			if [ $$? -ne 0 ]; then \
				exit 1; \
			fi; \
			cd $(ROOTFS_DIR); mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_RW_MAX_LEB_COUNT) -o bin/$(IMGNAME_UBIFS_RW).raw -r rootfs.tmp; \
			$(call gen_rootfs_ubinize_cfg_param,bin/$(IMGNAME_UBIFS_RW).raw,bin/ubinize_rw.cfg,$$(($(ROOTFS_UBI_RW_MAX_LEB_COUNT) * $(ROOTFS_UBI_ERASE_BLK_SIZE)))) \
			ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(IMGNAME_UBIFS_RW).ubi.raw bin/ubinize_rw.cfg; \
			$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_UBIFS_RW).ubi.raw --packsum-dst=bin/$(IMGNAME_UBIFS_RW) --packsum-type=0x9; \
		elif [ "$(EMBMEM)" == "EMBMEM_EMMC" ]; then \
			cd $(ROOTFS_DIR); make_ext4fs -s -l $(ROOTFS_EXT4_SIZE) bin/$(IMGNAME_EXT4_RW).raw rootfs.tmp; \
			$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_EXT4_RW).raw --packsum-dst=bin/$(IMGNAME_EXT4_RW) --packsum-type=0x9; \
		else \
			cd $(ROOTFS_DIR); mksquashfs rootfs.tmp bin/$(IMGNAME_SQUASHFS1).raw -comp $(ROOTFS_SQ_COMPRESS_MODE) -b $(ROOTFS_SQ_BLK_SIZE) -root-owned; \
			$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(IMGNAME_SQUASHFS1).raw --packsum-dst=bin/$(IMGNAME_SQUASHFS1) --packsum-type=0x9; \
		fi; \
	else \
		continue; \
	fi
endef

define rm_rootfs_tmp_files
	rm -rf $(ROOTFS_DIR)/rootfs.tmp/*;
	if [[ ( "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_RAMDISK" && "$(EMBMEM)" != "EMBMEM_NONE" ) \
		|| ( "$(EMBMEM)" == "EMBMEM_SPI_NAND" && "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ) \
		|| ( "$(EMBMEM)" == "EMBMEM_EMMC" && "$(NVT_ROOTFS_TYPE)" == "NVT_ROOTFS_TYPE_SQUASH" ) ]]; then \
		cp -r $(ROOTFS_DIR)/rootfs/lib/modules/* $(ROOTFS_DIR)/rootfs.tmp; \
		mkdir -p $(ROOTFS_DIR)/rootfs.tmp/usr; \
		cp -arf $(ROOTFS_DIR)/rootfs/usr/bin $(ROOTFS_DIR)/rootfs.tmp/usr; \
		cp -arf $(ROOTFS_DIR)/rootfs/usr/lib $(ROOTFS_DIR)/rootfs.tmp/usr; \
	fi
endef

define strip_binary_files
	if [ "${STRIPDIRS}" != "" ]; then \
		STRIPFILES=`file ${STRIPDIRS} 2> /dev/null | grep "not stripped" | awk 'BEGIN {FS=":"}; {print $$1}'`; \
		for target_file in $$STRIPFILES; do \
			chmod 755 $$target_file; \
			${STRIP} $$target_file; \
		done; \
	fi
	if [ "${STRIPLIBS}" != "" ]; then \
		STRIPFILES=`for lib in ${STRIPLIBS}; do \
				find $$lib -type f -exec file {} \; 2> /dev/null | grep "not stripped" | awk 'BEGIN {FS=":"}; {print $$1}'; \
			done;`; \
		for target_file in $$STRIPFILES; do \
			chmod 755 $$target_file; \
			${STRIP} -g --strip-unneeded $$target_file; \
		done; \
	fi
endef

define install_rootfs
	echo -e "\e[1;44mRootfs are installing.....\e[0m"
	cd $(ROOTFS_DIR)/bin/; cp rootfs*.bin $(LINUX_BUILD_TOP)/output/; \
	RAW_IMAGE_LIST=`ls *.raw 2>/dev/null`; \
	for file in $$RAW_IMAGE_LIST; do \
		mkdir -p $(LINUX_BUILD_TOP)/output/raw; \
		if [ -f $$file ]; then cp $$file $(LINUX_BUILD_TOP)/output/raw; fi; \
	done;
endef

define gen_prjcfg_profile_init
	echo -e "\e[1;44m To do rootfs/etc/profile_prjcfg is generating .....\e[0m"
	if [ -z $(LINUX_BUILD_TOP) ]; then \
		echo -e "\r\nERROR :Please source build/envsetup.sh in NA51000_Linux firstly to have auto copyso function\r\n"; \
		exit 1; \
	fi
	echo "export MODEL=`echo ${NVT_PRJCFG_MODEL_CFG} | awk -F'ModelConfig_' '{print $$NF}' | awk -F'.txt' '{print $$1}'`" > $(MODELCFG_PROFILE_PATH);
	grep '^[^#][[:alnum:]_]*[[:blank:]]*=[[:blank:]]*[[:alnum:][:blank:]_]*' ${NVT_PRJCFG_MODEL_CFG}  | sed 's/\([[:alnum:]_]*\)[[:blank:]]*=[[:blank:]]*\([[:alnum:][:blank:]_\.$$\(\)\-]*\)/export \1="\2"/g' >> $(MODELCFG_PROFILE_PATH);
	sed -i '/BIN_NAME/d' $(MODELCFG_PROFILE_PATH);
#	echo "export LD_PRELOAD='libnvtlibc.so'" >> $(MODELCFG_PROFILE_PATH);
endef

define clean_specific_files_list
	if [ -z $(LINUX_BUILD_TOP) ]; then \
		echo -e "\r\nERROR :Please source build/envsetup.sh in NA51000_Linux firstly to have auto copyso function\r\n"; \
		exit 1; \
	fi
	cd ${ROOTFS_DIR}; \
	for n in $(CLEAR_FILE_LIST); do \
		rm -rf $$n; \
	done;
endef

define clean_rootfs
	rm -rf ${ROOTFS_DIR}/bin
	rm -rf ${ROOTFS_DIR}/rootfs/bin
	rm -rf ${ROOTFS_DIR}/rootfs/sbin
	rm -rf ${ROOTFS_DIR}/rootfs/lib/*
	rm -rf ${ROOTFS_DIR}/rootfs/usr/bin
	rm -rf ${ROOTFS_DIR}/rootfs/usr/local
	rm -rf ${ROOTFS_DIR}/rootfs/usr/sbin
	rm -rf ${ROOTFS_DIR}/rootfs/usr/lib/*
	rm -rf ${ROOTFS_DIR}/rootfs/var/www/cgi-bin/*
	rm -rf ${ROOTFS_DIR}/rootfs/etc
	rm -rf ${ROOTFS_DIR}/rootfs/man
	rm -rf ${ROOTFS_DIR}/rootfs/share
	if [ -d $(LINUX_BUILD_TOP)/output ]; then cd $(LINUX_BUILD_TOP)/output; rm -rf $(IMGNAME_RAMDISK) $(IMGNAME_UBIFS) $(IMGNAME_EXT4_RW) $(IMGNAME_SQUASH_UBI) $(IMGNAME_TARBZ2); fi
endef

all:
	@$(call gen_rootfs_init)

	@$(call gen_rootfs_etc_link)

	@$(call gen_prjcfg_profile_init)

	@$(call strip_binary_files)

	@$(call gen_rootfs_tmp)

	@$(call gen_selected_rootfs_ro)

	@$(call rm_rootfs_tmp_files)
ifneq ($(NVT_ROOTFS_RW_PART_EN), NVT_ROOTFS_RW_PART_EN_OFF)
	@$(call gen_selected_rootfs_rw)
endif
	@$(call rm_rootfs_tmp_dir)

gen_etc:
	@$(call gen_rootfs_etc_link)

install:
	@$(call install_rootfs)

clean:
	@$(call clean_specific_files_list)
	@$(call clean_rootfs)
	@$(call rm_rootfs_tmp)
