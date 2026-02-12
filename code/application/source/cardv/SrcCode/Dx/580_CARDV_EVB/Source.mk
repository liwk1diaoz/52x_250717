DTS_SENSOR = ./SrcCode/Dx/$(MODEL)/sensor.dts
DTS_APP = ./SrcCode/Dx/$(MODEL)/application.dts
DX_SRC = \
	./SrcCode/Dx/$(MODEL)/DxInput_Key.c \
	./SrcCode/Dx/$(MODEL)/IOCfg.c \
	./SrcCode/Dx/$(MODEL)/DxUsb.c \
	./SrcCode/Dx/$(MODEL)/DxCfg.c \
	./SrcCode/Dx/$(MODEL)/DxDisplay_LCD.c \
	./SrcCode/Dx/$(MODEL)/DxPower_Battery.c \
	./SrcCode/Dx/$(MODEL)/DxPower_DC.c \
	./SrcCode/Dx/$(MODEL)/DxStorage_Card.c \
	./SrcCode/Dx/$(MODEL)/DxStorage_EmbMem.c
	
#	./SrcCode/Dx/$(MODEL)/DxCamera_Sensor.c \
#	./SrcCode/Dx/$(MODEL)/DxCfg.c
#	./SrcCode/Dx/$(MODEL)/DxCmd.c
#	./SrcCode/Dx/$(MODEL)/DxDisplay_LCD.c
#	./SrcCode/Dx/$(MODEL)/DxOutput_LED.c
#	./SrcCode/Dx/$(MODEL)/DxPower_Battery.c
#	./SrcCode/Dx/$(MODEL)/DxSound_Audio.c
#	./SrcCode/Dx/$(MODEL)/DxStorage_Card.c
#	./SrcCode/Dx/$(MODEL)/DxStorage_EmbMem.c
#	./SrcCode/Dx/$(MODEL)/DxWiFi.c
#	./SrcCode/Dx/$(MODEL)/DxCmd.c


