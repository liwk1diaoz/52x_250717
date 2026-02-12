CFLAGS = 
TIME = time

all: rundhrystone rundhrystoneR rundhrystoneO rundhrystoneRO

dhrystone: dhrystone.c
	$(CC) dhrystone.c -o dhrystone

rundhrystone: dhrystone
	@echo 'Running dhrystone (Level 1 optimization, without registers)'
	@echo "=> dhrystone"


dhrystoneR: dhrystone.c
	$(CC) -DREG=register dhrystone.c -o dhrystoneR

rundhrystoneR: dhrystoneR
	@echo 'Running dhrystone (Level 1 optimization, with registers)'
	@echo "=> dhrystoneR"


dhrystoneO: dhrystone.c
	$(CC) -O dhrystone.c -o dhrystoneO

rundhrystoneO: dhrystoneO
	@echo 'Running dhrystone (Level 2 optimization, without registers)'
	@echo "=> dhrystoneO"


dhrystoneRO: dhrystone.c
	$(CC) -O -DREG=register dhrystone.c -o dhrystoneRO

rundhrystoneRO: dhrystoneRO
	@echo 'Running dhrystone (Level 2 optimization, with registers)'
	@echo "=> dhrystoneRO"


clean:
	(set nonomatch;rm -f dhrystone dhrystoneR dhrystoneO dhrystoneRO *.o)

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@mkdir -p $(APP_DIR)/output
	@cp -avf dhrystone dhrystoneR dhrystoneO dhrystoneRO $(APP_DIR)/output
	@cp -avf dhrystone dhrystoneR dhrystoneO dhrystoneRO $(ROOTFS_DIR)/rootfs/usr/bin
	@echo "Running below instructions:"
	@echo "=> dhrystone"
	@echo "=> dhrystoneR"
	@echo "=> dhrystoneO"
	@echo "=> dhrystoneRO"
