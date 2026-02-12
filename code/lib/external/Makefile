include $(NVT_PRJCFG_MODEL_CFG)

EXTLIB = ${LIBRARY_DIR}/external
SHELL = /bin/bash
INSTALL_DIR =${EXTLIB}/__install
PREFIX="/usr"
#MAKEFILE_LIST = freetype libnl openssl
# openssl for tee
MULTI_CORES ?= $(shell grep -c ^processor /proc/cpuinfo)
MAKEFILE_LIST = openssl libnl zbar

# Bluez and dependencies
ifeq (${BT_STACK},BT_STACK_BLUEZ)
	MAKEFILE_LIST += bluez
endif

TARGET=$(shell ${CC} -dumpmachine)
CROSS_TOOLCHAIN_BIN_PATH2=${CROSS_TOOLCHAIN_PATH}/${TARGET}/bin

XML2 := libxml2-2.9.3
AXTLS := axtls
FUSE := fuse-2.9.4
FREETYPE := freetype-2.3.5
UNWIND := libunwind-1.3.0
SQLITE3 :=sqlite3-3.10.0
FFMPEG := ffmpeg-3.0
LIBNL := libnl-3.2.27
OPENSSL := openssl-1.0.2d
CURL := curl-7.45.0
ZBAR := zbar-0.10
LIBFFI := libffi-3.2.1
NCURSES := ncurses-6.3
GLIB := glib-2.58.3
UTIL-LINUX := util-linux-v2.33.1
ICONV := libiconv-1.16
DBUS := dbus-1.12.20
EXPAT := expat-2.2.10
READLINE := readline-6.2
UDEV := udev-070
LIBICAL := libical-2.0.0
BLUEZ := bluez-5.64
ZBAR := zbar-0.10

CFLAGS="-fPIC -I${INSTALL_DIR}/${PREFIX}/include"
LDFLAGS="-L${INSTALL_DIR}/${PREFIX}/lib -Wl,-rpath-link=${INSTALL_DIR}/${PREFIX}/lib -Wl,-rpath=${INSTALL_DIR}/${PREFIX}/lib" 

check_file_exist = \
if [ ! -f $(1) ]; then \
	echo "file $(1) not exist!!" ; \
	exit 1 ; \
fi


check_dir_exist = \
if [ ! -d $(1) ]; then \
	echo "dir $(1) not exist!!" ; \
	exit 1 ; \
fi


check_tar_exist =  \
if [ ! -f $(1).tar* ]; then \
	echo "tar $(1).tar* not exist!!" ; \
	exit 1 ; \
fi

extract_tar_file = \
if [ -f $(1).tar* ]; then \
	rm -rf $(1); \
	tar -xvf $(1).tar*; \
else \
	exit 1 ; \
fi


check_exist_cp_bin = \
if [ -d $(INSTALL_DIR)/$(1) ]; then \
	ls $(INSTALL_DIR)/$(1)/$(2); \
	if [ $$? -eq 0 ]; then \
		mkdir -p ${ROOTFS_DIR}/rootfs/$(3); \
		cp -arfv $(INSTALL_DIR)/$(1)/$(2) ${ROOTFS_DIR}/rootfs/$(3)/; \
	fi \
fi

check_exist_cp_lib = \
if [ -d $(INSTALL_DIR)/$(1) ]; then \
	ls $(INSTALL_DIR)/$(1)/$(2); \
	if [ $$? -eq 0 ]; then \
		mkdir -p ${ROOTFS_DIR}/rootfs/$(3); \
		cp -arfv $(INSTALL_DIR)/$(1)/$(2) ${ROOTFS_DIR}/rootfs/$(3)/ ; \
	fi \
fi

check_exist_cp_etc = \
if [ -d $(INSTALL_DIR)/$(1) ]; then \
	ls $(INSTALL_DIR)/$(1)/$(2); \
	if [ $$? -eq 0 ]; then \
		mkdir -p ${ROOTFS_DIR}/rootfs/$(3); \
		cp -narv $(INSTALL_DIR)/$(1)/$(2) ${ROOTFS_DIR}/rootfs/$(3)/; \
	fi \
fi


.PHONY: $(MAKEFILE_LIST)
all: $(MAKEFILE_LIST)

list:
	@echo $(MAKEFILE_LIST)

init:
	@if [ ! -e "${INSTALL_DIR}" ]; then \
		mkdir -p ${INSTALL_DIR}; \
	fi
	@if [ -z "${CROSS_COMPILE}" ]; then \
		echo "Please execute \"source build/envsetup.sh\" firstly.  Stop."; exit 1; \
	fi

libxml2: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${XML2});
	@if [ ! -f ${XML2}/.nvt_finish ]; then \
		$(call extract_tar_file, ${XML2}); \
		cd ${XML2}; ./configure --prefix=${INSTALL_DIR} CC=${CC} CXX=${CXX} AR=${AR} --host=${NVT_HOST} --without-python --target=${NVT_HOST}; make -j$(MULTI_CORES); \
	fi
	@cd ${XML2}; make install; cd -;
	@rm -rf ${INSTALL_DIR}/lib/cmake ${INSTALL_DIR}/lib/xml2Conf.sh ${INSTALL_DIR}/lib/pkgconfig
	@touch ${XML2}/.nvt_finish

axtls: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${AXTLS});
	@if [ ! -f ${AXTLS}/.nvt_finish ]; then \
		$(call extract_tar_file, ${AXTLS}); \
		cd ${AXTLS}; make -j$(MULTI_CORES) CC=${CC} AR=${AR}; \
	fi
	@cd ${AXTLS}; make PREFIX=${INSTALL_DIR} install;
	@touch ${AXTLS}/.nvt_finish

fuse: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${FUSE});
	@if [ ! -f ${FUSE}/.nvt_finish ]; then \
		$(call extract_tar_file, ${FUSE}); \
		cd ${FUSE}; ./configure --host=${NVT_HOST} CC=${CC} LD=${LD} AR=${AR}; \
		make -j$(MULTI_CORES); \
	fi
	cd ${FUSE}; make -j$(MULTI_CORES) install DESTDIR=${EXTLIB}/${FUSE}/install; cp -arvf ${EXTLIB}/${FUSE}/install/usr/local/* ${INSTALL_DIR};
	@touch ${FUSE}/.nvt_finish

freetype: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${FREETYPE});
	@if [ ! -f ${FREETYPE}/.nvt_finish ]; then \
		$(call extract_tar_file, ${FREETYPE}); \
		cd ${FREETYPE}; mkdir result; CC=${NVT_HOST}-gcc; ./configure --prefix=${INSTALL_DIR} --host=${NVT_HOST}; \
		make -j$(MULTI_CORES); \
	fi
	@cd ${FREETYPE}; make install;
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig
	@touch ${FREETYPE}/.nvt_finish

libunwind: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${UNWIND});
	@if [ ! -f ${UNWIND}/.nvt_finish ]; then \
		$(call extract_tar_file, ${UNWIND}); \
		cd ${UNWIND}; mkdir result; CC=${NVT_HOST}-gcc; ./configure --prefix=${INSTALL_DIR} --host=${NVT_HOST}; \
		make -j$(MULTI_CORES); \
	fi
	@cd ${UNWIND}; make install;
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig
	@touch ${UNWIND}/.nvt_finish

sqlite3: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${SQLITE3});
	@if [ ! -f ${SQLITE3}/.nvt_finish ]; then \
		$(call extract_tar_file, ${SQLITE3}); \
		cd ${SQLITE3}; make -j$(MULTI_CORES) CC=${CC} AR=${AR} STRIP="${CROSS_COMPILE}strip"; \
	fi
	@cd ${SQLITE3}; make INSTALL_DIR=${INSTALL_DIR} install;
	@touch ${SQLITE3}/.nvt_finish

ffmpeg: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${FFMPEG});
	@if [ ! -f ${FFMPEG}/.nvt_finish ]; then \
		$(call extract_tar_file, ${FFMPEG}); \
		cd ${FFMPEG}; ./configure --prefix=${INSTALL_DIR} --cross-prefix=${NVT_HOST}- --target-os=linux --arch=${ARCH} --target-exec="qemu-mips -cpu 24Kec" --disable-everything --enable-muxer=mpegts --enable-muxer=mp4 --disable-armv5te --disable-armv6t2 --disable-armv6 --disable-asm --disable-encoders --disable-decoders --enable-protocol=file --enable-small --disable-static --enable-shared --enable-cross-compile --disable-zlib --disable-mipsfpu --disable-ffmpeg --disable-ffserver --disable-ffprobe --disable-swscale --disable-swresample --disable-avdevice --disable-avfilter --disable-debug; \
		make -j$(MULTI_CORES); \
	fi
	@cd ${FFMPEG}; make install;
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig
	@touch ${FFMPEG}/.nvt_finish

libnl: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${LIBNL});
	@if [ ! -f ${LIBNL}/.nvt_finish ]; then \
		$(call extract_tar_file, ${LIBNL}); \
		cd ${LIBNL}; ./configure --prefix=${INSTALL_DIR} --host=${NVT_HOST} CC=${CC} LD=${LD} AR=${AR}; \
		make -j$(MULTI_CORES); \
	fi
	@cd ${LIBNL}; make install;
	@cp ${INSTALL_DIR}/lib/libnl-3.so ${INSTALL_DIR}/lib/libnl.so ; cp ${INSTALL_DIR}/lib/libnl-genl-3.so ${INSTALL_DIR}/lib/libnl-genl.so
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig
	@touch ${LIBNL}/.nvt_finish

openssl: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${OPENSSL});
	@if [ ! -f ${OPENSSL}/.nvt_finish ]; then \
		$(call extract_tar_file, ${OPENSSL}); \
		cd ${OPENSSL}; CROSS_COMPILE="" CC=${CC} LD=${LD} AR=${AR} RANLIB=${RANLIB} ./Configure shared linux-generic32 -DHAVE_CRYPTODEV -DUSE_CRYPTODEV_DIGESTS -DOPENSSL_SSL_CLIENT_ENGINE_AUTO --prefix=${INSTALL_DIR} -I$(NVT_DRIVER_DIR)/include; make -j$(MULTI_CORES); \
	fi
	@cd ${OPENSSL}; make install;
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig ${INSTALL_DIR}/lib/engines
	@touch ${OPENSSL}/.nvt_finish

curl: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${CURL});
ifeq ($(NVT_CURL_SSL),NVT_CURL_SSL_OPENSSL)
	@if [ -z `cat ${CURL}/.nvt_finish | grep NVT_CURL_SSL_OPENSSL` ]; then \
		$(call extract_tar_file, ${CURL}); \
		cd ${CURL}; CPPFLAGS="-I${INSTALL_DIR}/include -I${INCLUDE_DIR}" LDFLAGS="-L${LIBRARY_DIR} -L${INSTALL_DIR}/lib -Wl,-rpath-link=$(INSTALL_DIR)/lib" LIBS="-lssl" ./configure --with-ssl --prefix=${INSTALL_DIR} --with-ca-path="/etc/ssl/certs" --host=${NVT_HOST}; make -j$(MULTI_CORES); \
	fi
else
	@if [ -z `cat ${CURL}/.nvt_finish | grep NVT_CURL_SSL_AXTLS` ]; then \
		$(call extract_tar_file, ${CURL}); \
		cd ${CURL}; CPPFLAGS="-I${INSTALL_DIR}/include -I${INCLUDE_DIR}" LDFLAGS="-L${LIBRARY_DIR} -L${INSTALL_DIR}/lib -Wl,-rpath-link=$(INSTALL_DIR)/lib" LIBS="-laxtls" ./configure --without-ssl --with-axtls --enable-ipv6 --prefix=${INSTALL_DIR} --host=${NVT_HOST}; make -j$(MULTI_CORES); \
	fi
endif
	@cd ${CURL}; make install;
	@cd ${INSTALL_DIR}/bin; rm curl-config
	@rm -rf ${INSTALL_DIR}/lib/pkgconfig
	@echo $(NVT_CURL_SSL) > ${CURL}/.nvt_finish

libffi: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${LIBFFI});
	@if [ ! -f ${LIBFFI}/.nvt_finish ]; then \
		$(call extract_tar_file, ${LIBFFI}); \
		cd ${LIBFFI}; CC=${CC} LD=${LD} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} AR=${AR} RANLIB=${RANLIB} \
		./configure --host=${NVT_HOST} --prefix=${INSTALL_DIR}/${PREFIX} --without-PACKAGE; make -j$(MULTI_CORES); \
	fi
	@cd ${LIBFFI}; make install;
	@touch ${LIBFFI}/.nvt_finish

ncurses: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${NCURSES});
	@if [ ! -f ${NCURSES}/.nvt_finish ]; then \
		$(call extract_tar_file, ${NCURSES}); \
		cd ${NCURSES}; CC=${CC} LD=${LD} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} AR=${AR} RANLIB=${RANLIB} \
		./configure --host=${NVT_HOST} --prefix=${INSTALL_DIR}/${PREFIX} --with-shared --disable-db-install --with-termlib; \
	       	make -j$(MULTI_CORES); \
	fi
	@cd ${NCURSES}; make install ;
	@touch ${NCURSES}/.nvt_finish

util-linux: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${UTIL-LINUX}); 
	@if [ ! -f ${UTIL-LINUX}/.nvt_finish ]; then \
		$(call extract_tar_file, ${UTIL-LINUX}) ; \
		cd ${UTIL-LINUX}; ./autogen.sh; \
		CC=${CC} LD=${LD} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} AR=${AR} RANLIB=${RANLIB} \
		./configure --host=${NVT_HOST} --prefix=${INSTALL_DIR}/${PREFIX} \
		--disable-pylibmount --without-python --disable-makeinstall-chown --disable-use-tty-group \
		--disable-rpath --disable-makeinstall-chown --disable-makeinstall-setuid \
		--disable-bash-completion --enable-libmount ; \
		make -j$(MULTI_CORES); \
	fi
	@cd ${UTIL-LINUX}; make install;
	@touch ${UTIL-LINUX}/.nvt_finish

iconv:
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${ICONV});
	@if [ ! -f ${ICONV}/.nvt_finish ]; then \
		$(call extract_tar_file, ${ICONV}); \
		cd ${ICONV}; ./autogen.sh; \
		CC=${CC} LD=${LD} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} AR=${AR} RANLIB=${RANLIB} ./configure --host=${NVT_HOST} --prefix=${INSTALL_DIR}/${PREFIX}; \
		make -j$(MULTI_CORES);\
	fi
	@cd ${ICONV}; make install;
	@touch ${ICONV}/.nvt_finish


glib: init iconv libffi ncurses util-linux
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${GLIB});
	@if [ ! -f ${GLIB}/.nvt_finish ]; then \
		$(call extract_tar_file, ${GLIB}); \
		cd ${GLIB}; ./autogen.sh; CC=${CC} LD=${LD} AR=${AR} CFLAGS=${CFLAGS}" -Wno-error=format-nonliteral" LDFLAGS=${LDFLAGS} RANLIB=${RANLIB} \
		LIBFFI_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/lib/${LIBFFI}/include" \
		LIBFFI_LIBS="-L${INSTALL_DIR}/${PREFIX}/lib -lffi" \
		LIBMOUNT_LIBS="-L${INSTALL_DIR}/${PREFIX}/lib -lmount" \
		LIBMOUNT_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include/libmount" \
		ac_cv_type_long_long=yes glib_cv_stack_grows=no \
		glib_cv_uscore=no ac_cv_func_posix_getpwuid_r=yes \
		ac_cv_func_posix_getgrgid_r=yes \
		./configure \
		--host=${NVT_HOST} --prefix=${INSTALL_DIR}/${PREFIX} --enable-libmount --enable-shared --with-libiconv=gnu --with-pcre=internal; \
		make -j$(MULTI_CORES);\
	fi
	@cd ${GLIB}; make install;
	@touch ${GLIB}/.nvt_finish

expat: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${EXPAT});
	@if [ ! -f ${EXPAT}/.nvt_finish ]; then \
		$(call extract_tar_file, ${EXPAT}); \
		cd ${EXPAT}; ./configure --host=${NVT_HOST} --prefix=${PREFIX}; make -j$(MULTI_CORES); \
	fi
	@cd ${EXPAT}; make install  DESTDIR=${INSTALL_DIR};
	@touch ${EXPAT}/.nvt_finish

dbus: init expat
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${DBUS});
	@if [ ! -f ${DBUS}/.nvt_finish ]; then \
		$(call extract_tar_file, ${DBUS}); \
		cd ${DBUS}; \
		CC=${CC} LD=${LD} AR=${AR} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} RANLIB=${RANLIB} \
		EXPAT_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include" \
		EXPAT_LIBS="-L${INSTALL_DIR}/${PREFIX}/lib -lexpat" \
		./configure --host=${NVT_HOST} --prefix=${PREFIX} --enable-tests=no \
		--sysconfdir="/etc" --localstatedir="/var"; \
		make -j$(MULTI_CORES);\
	fi
	@cd ${DBUS}; make install DESTDIR=${INSTALL_DIR};
	@touch ${DBUS}/.nvt_finish
	@$(call check_exist_cp_bin,${PREFIX}/bin,dbus-daemon,${PREFIX}/bin)

readline: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${READLINE});
	@if [ ! -f ${READLINE}/.nvt_finish ]; then \
		$(call extract_tar_file, ${READLINE}); \
		cd ${READLINE}; ./configure --host=${NVT_HOST} --prefix=${PREFIX};\
		make -j$(MULTI_CORES); \
	fi
	@cd ${READLINE}; make install  DESTDIR=${INSTALL_DIR};
	@touch ${READLINE}/.nvt_finish

libical: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${LIBICAL});
	@if [ ! -f ${LIBICAL}/.nvt_finish ]; then \
		$(call extract_tar_file, ${LIBICAL}); \
		cd ${LIBICAL}; mkdir build;cd build; \
		cmake -DBUILD_SHARED_LIBS="ON" -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/${PREFIX} .. ;\
		make -j$(MULTI_CORES); \
	fi
	@cd ${LIBICAL}/build ; make install;
	@touch ${LIBICAL}/.nvt_finish

udev: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${UDEV});
	@if [ ! -f ${UDEV}/.nvt_finish ]; then \
		$(call extract_tar_file, ${UDEV}); \
		cd ${UDEV}; make -j$(MULTI_CORES) V=true CROSS_COMPILE=${CROSS_COMPILE} PREFIX=${INSTALL_DIR} CXX=${CXX}; \
	fi
	@cd ${UDEV}; make install-bin DESTDIR=${INSTALL_DIR}; if [ -d ${ROOTFS_DIR}/rootfs/etc ]; then make install-config DESTDIR=${ROOTFS_DIR}/rootfs; fi;
	@touch ${UDEV}/.nvt_finish

bluez: glib libical readline udev dbus
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${BLUEZ});
	@if [ ! -f ${BLUEZ}/.nvt_finish ]; then \
		$(call extract_tar_file, ${BLUEZ}); \
		cd ${BLUEZ}; \
		CFLAGS=${CFLAGS} \
		LDFLAGS="-L${INSTALL_DIR}/${PREFIX}/lib -Wl,-rpath-link=${INSTALL_DIR}/${PREFIX}/lib -Wl,-rpath=${INSTALL_DIR}/${PREFIX}/lib -ltinfo -ldbus-1 -licalvcal " \
		./configure \
		GLIB_LIBS="-lgio-2.0 -lgobject-2.0 -lglib-2.0 -L${INSTALL_DIR}/${PREFIX}/lib" \
		GLIB_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include/gio-unix-2.0 -I${INSTALL_DIR}/${PREFIX}/include/glib-2.0 -I${INSTALL_DIR}/${PREFIX}/lib/glib-2.0/include" \
	       	DBUS_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include/dbus-1.0 -I${INSTALL_DIR}/${PREFIX}/lib/dbus-1.0/include -I${INCLUDE_DIR}" \
		DBUS_LIBS="-L${LIBRARY_DIR} -L${INSTALL_DIR}/${PREFIX}/lib -ldbus-1" \
		UDEV_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include -I${INCLUDE_DIR}" \
		UDEV_LIBS="-L${LIBRARY_DIR} -L${INSTALL_DIR}/${PREFIX}/lib" \
		ICAL_CFLAGS="-I${INSTALL_DIR}/${PREFIX}/include -I${INCLUDE_DIR}" \
		ICAL_LIBS="-L${LIBRARY_DIR} -L${INSTALL_DIR}/${PREFIX}/lib" \
		--prefix=${PREFIX} --host=${NVT_HOST} --enable-tools --enable-deprecated --enable-testing --enable-library \
		--with-dbussystembusdir="${INSTALL_DIR}/${PREFIX}/share" --with-dbussessionbusdir="${INSTALL_DIR}/${PREFIX}/share" \
		--sysconfdir="/etc" --localstatedir="/var" ; \
	fi
	@cd ${BLUEZ}; make -j4; make install DESTDIR=${INSTALL_DIR};
	@touch ${BLUEZ}/.nvt_finish;
	@$(call check_exist_cp_bin,${PREFIX}/bin,bluetoothctl,${PREFIX}/bin)
	@$(call check_exist_cp_bin,${PREFIX}/bin,hciconfig,${PREFIX}/bin)
	#@$(call check_exist_cp_bin,${PREFIX}/bin,hcitool,${PREFIX}/bin)
	@$(call check_exist_cp_bin,${PREFIX}/bin,hcidump,${PREFIX}/bin)
	#@$(call check_exist_cp_bin,${PREFIX}/bin,l2ping,${PREFIX}/bin)
	#@$(call check_exist_cp_bin,${PREFIX}/tool,btgatt-client,${PREFIX}/bin)
	#@$(call check_exist_cp_bin,${PREFIX}/tool,btgatt-server,${PREFIX}/bin)
	@$(call check_exist_cp_bin,${PREFIX}/libexec/bluetooth,bluetoothd,${PREFIX}/bin)

zbar: init iconv
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_tar_exist, ${ZBAR});
	@if [ ! -f ${ZBAR}/.nvt_finish ]; then \
		$(call extract_tar_file, ${ZBAR}); \
		cd ${ZBAR}; CROSS_COMPILE="" CC=${CC} LD=${LD} AR=${AR} RANLIB=${RANLIB} ./configure --prefix=${INSTALL_DIR}/${PREFIX} --disable-pthread --disable-video --without-imagemagick --without-qt --without-python --without-gtk --disable-dependency-tracking --host=${NVT_HOST}; make -j${MULTI_CORES}; \
	fi
	cd ${ZBAR}; make install;
	@rm -rf ${INSTALL_DIR}/${PREFIX}/lib/pkgconfig
	@touch ${ZBAR}/.nvt_finish

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@$(call check_exist_cp_etc,var,*,var)
	@$(call check_exist_cp_etc,etc,*,etc)
	@$(call check_exist_cp_etc,${PREFIX}/share/dbus-1,*,${PREFIX}/share/dbus-1)
	@$(call check_exist_cp_lib,${PREFIX}/lib,*.so*,${PREFIX}/lib)
	@$(call check_exist_cp_lib,lib,*.so*,lib)
	@if [ -x ${INSTALL_DIR}/lib ]; then \
		mkdir -p ${LIBRARY_DIR}/output; \
		cp -arvf ${INSTALL_DIR}/lib/*.* ${LIBRARY_DIR}/output; \
	fi
	@if [ -x ${INSTALL_DIR}/${PREFIX}/lib ]; then \
		mkdir -p ${LIBRARY_DIR}/output; \
		cp -arvf ${INSTALL_DIR}/${PREFIX}/lib/*.* ${LIBRARY_DIR}/output; \
	fi	

clean:
	@echo ">>>>>>>>>>>>>>>>>>> Remove >>>>>>>>>>>>>>>>>>>"
	@rm -rf ${INSTALL_DIR}
	@rm -rf ${XML2} ${AXTLS} ${FUSE} ${FREETYPE} ${FFMPEG} ${LIBNL} ${SQLITE3} ${OPENSSL} ${CURL} ${UNWIND} ${LIBP11} \
		${LIBICAL} ${EXPAT} ${UDEV} ${DBUS} ${READLINE} ${BLUEZ} ${GLIB} ${NCURSES} ${ICONV} ${UTIL-LINUX} ${LIBFFI} ${ZBAR}
