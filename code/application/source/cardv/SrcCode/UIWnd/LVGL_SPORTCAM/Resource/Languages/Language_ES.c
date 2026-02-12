#include "Resource/Plugin/lv_plugin_common.h"
lv_plugin_string_t lv_plugin_ES_string_table[] = {
	{ NULL, 0 },
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_NULL_ */ 
	{ "Modo", 4 },			/* LV_PLUGIN_STRING_ID_STRID_MODE */ 
	{ "Tam. Imág.", 11 },			/* LV_PLUGIN_STRING_ID_STRID_IMGSIZE */ 
	{ "Resolución", 11 },			/* LV_PLUGIN_STRING_ID_STRID_RESOLUTION */ 
	{ "12M", 3 },			/* LV_PLUGIN_STRING_ID_STRID_12M */ 
	{ "10M", 3 },			/* LV_PLUGIN_STRING_ID_STRID_10M */ 
	{ "9M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_9M */ 
	{ "8M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_8M */ 
	{ "7M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_7M */ 
	{ "6M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_6M */ 
	{ "5M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_5M */ 
	{ "4M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_4M */ 
	{ "3M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_3M */ 
	{ "2MHD", 4 },			/* LV_PLUGIN_STRING_ID_STRID_2MHD */ 
	{ "2M", 2 },			/* LV_PLUGIN_STRING_ID_STRID_2M */ 
	{ "1.3M", 4 },			/* LV_PLUGIN_STRING_ID_STRID_1M */ 
	{ "VGA", 3 },			/* LV_PLUGIN_STRING_ID_STRID_VGA */ 
	{ "QVGA", 4 },			/* LV_PLUGIN_STRING_ID_STRID_QVGA */ 
	{ "D1", 2 },			/* LV_PLUGIN_STRING_ID_STRID_D1 */ 
	{ "720P", 4 },			/* LV_PLUGIN_STRING_ID_STRID_720P */ 
	{ "1080P", 5 },			/* LV_PLUGIN_STRING_ID_STRID_1080P */ 
	{ "1080FHD", 7 },			/* LV_PLUGIN_STRING_ID_STRID_1080FHD */ 
	{ "12M 4032x3024", 13 },			/* LV_PLUGIN_STRING_ID_STRID_12MWXH */ 
	{ "10M 3648x2736", 13 },			/* LV_PLUGIN_STRING_ID_STRID_10MWXH */ 
	{ "9M 3472x2604", 12 },			/* LV_PLUGIN_STRING_ID_STRID_9MWXH */ 
	{ "8M 3264x2448", 12 },			/* LV_PLUGIN_STRING_ID_STRID_8MWXH */ 
	{ "7M 3072x2304", 12 },			/* LV_PLUGIN_STRING_ID_STRID_7MWXH */ 
	{ "6M 2816x2112", 12 },			/* LV_PLUGIN_STRING_ID_STRID_6MWXH */ 
	{ "5M 2592x1944", 12 },			/* LV_PLUGIN_STRING_ID_STRID_5MWXH */ 
	{ "4M 2272x1704", 12 },			/* LV_PLUGIN_STRING_ID_STRID_4MWXH */ 
	{ "3M 2048x1536", 12 },			/* LV_PLUGIN_STRING_ID_STRID_3MWXH */ 
	{ "2MHD 1920x1080", 14 },			/* LV_PLUGIN_STRING_ID_STRID_2MHDWXH */ 
	{ "2M 1600x1200", 12 },			/* LV_PLUGIN_STRING_ID_STRID_2MWXH */ 
	{ "1.3M 1280x960", 13 },			/* LV_PLUGIN_STRING_ID_STRID_1MWXH */ 
	{ "WVGA 848x480", 12 },			/* LV_PLUGIN_STRING_ID_STRID_WVGAWXH */ 
	{ "VGA 640x480", 11 },			/* LV_PLUGIN_STRING_ID_STRID_VGAWXH */ 
	{ "QVGA 320x240", 12 },			/* LV_PLUGIN_STRING_ID_STRID_QVGAWXH */ 
	{ "1080P60,720x480P30", 18 },			/* LV_PLUGIN_STRING_ID_STRID_1080P60_D1P30 */ 
	{ "D1 720x480", 10 },			/* LV_PLUGIN_STRING_ID_STRID_D1WXH */ 
	{ "720P 1280x720", 13 },			/* LV_PLUGIN_STRING_ID_STRID_720PWXH */ 
	{ "1080P 1440x1080", 15 },			/* LV_PLUGIN_STRING_ID_STRID_1080PWXH */ 
	{ "1080FHD 1920x1080", 17 },			/* LV_PLUGIN_STRING_ID_STRID_1080FHDWXH */ 
	{ "Compresión", 11 },			/* LV_PLUGIN_STRING_ID_STRID_COMPRESSION */ 
	{ "Calidad", 7 },			/* LV_PLUGIN_STRING_ID_STRID_QUALITY */ 
	{ "Calidad Extra", 13 },			/* LV_PLUGIN_STRING_ID_STRID_SUPER */ 
	{ "Alta", 4 },			/* LV_PLUGIN_STRING_ID_STRID_FINE */ 
	{ "Normal", 6 },			/* LV_PLUGIN_STRING_ID_STRID_NORMAL */ 
	{ "Económ.", 8 },			/* LV_PLUGIN_STRING_ID_STRID_ECONOMY */ 
	{ "Balance Blanco", 14 },			/* LV_PLUGIN_STRING_ID_STRID_WB */ 
	{ "Auto", 4 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO */ 
	{ "Sol", 3 },			/* LV_PLUGIN_STRING_ID_STRID_WB_DAY */ 
	{ "Nubes", 5 },			/* LV_PLUGIN_STRING_ID_STRID_WB_CLOUDY */ 
	{ "Tungsteno", 9 },			/* LV_PLUGIN_STRING_ID_STRID_WB_TUNGSTEN */ 
	{ "Fluoro", 6 },			/* LV_PLUGIN_STRING_ID_STRID_WB_FLUORESCENT */ 
	{ "Exposición", 11 },			/* LV_PLUGIN_STRING_ID_STRID_EXPOSURE */ 
	{ "Exposición", 11 },			/* LV_PLUGIN_STRING_ID_STRID_EV */ 
	{ "ISO", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO */ 
	{ "50", 2 },			/* LV_PLUGIN_STRING_ID_STRID_ISO50 */ 
	{ "100", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO100 */ 
	{ "200", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO200 */ 
	{ "400", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO400 */ 
	{ "800", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO800 */ 
	{ "1600", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ISO1600 */ 
	{ "3200", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ISO3200 */ 
	{ "Medición AE", 12 },			/* LV_PLUGIN_STRING_ID_STRID_METERING */ 
	{ "Ponderada Centro", 16 },			/* LV_PLUGIN_STRING_ID_STRID_METER_CENTER */ 
	{ "Puntual", 7 },			/* LV_PLUGIN_STRING_ID_STRID_METER_SPOT */ 
	{ "Promedio", 8 },			/* LV_PLUGIN_STRING_ID_STRID_METER_AVG */ 
	{ "Modo grabación", 15 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_MODE */ 
	{ "único", 6 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_SINGLE */ 
	{ "continuo", 8 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_BURST */ 
	{ "3 continuo", 10 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_BURST_3 */ 
	{ "AEB", 3 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_AUTO */ 
	{ "2s contador de tiempo", 21 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER2S */ 
	{ "5s contador de tiempo", 21 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER5S */ 
	{ "10s contador de tiempo", 22 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER10S */ 
	{ "20s contador de tiempo", 22 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER20S */ 
	{ "Modo de destello", 16 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHMODE */ 
	{ "Fuerza encendido", 16 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHON */ 
	{ "Fuerza apagado", 14 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHOFF */ 
	{ "Auto", 4 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHAUTO */ 
	{ "Rojo-ojo reducción", 19 },			/* LV_PLUGIN_STRING_ID_STRID_REDEYE */ 
	{ "Va", 2 },			/* LV_PLUGIN_STRING_ID_STRID_AV */ 
	{ "Vt", 2 },			/* LV_PLUGIN_STRING_ID_STRID_TV */ 
	{ "Va/Vt", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AVTV */ 
	{ "Ajuste Va:", 10 },			/* LV_PLUGIN_STRING_ID_STRID_AVSETTING */ 
	{ "Ajuste Vt:", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TVSETTING */ 
	{ "Ajuste Va/Vt:", 13 },			/* LV_PLUGIN_STRING_ID_STRID_AVTVSETTING */ 
	{ "Paisaje", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SCENESETTING */ 
	{ "Paisaje", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SCENE */ 
	{ "Paisaje", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LANDSCAPE */ 
	{ "Escena de la noche", 18 },			/* LV_PLUGIN_STRING_ID_STRID_NIGHTSCENE */ 
	{ "Deportes", 8 },			/* LV_PLUGIN_STRING_ID_STRID_SPORTS */ 
	{ "Retrato", 7 },			/* LV_PLUGIN_STRING_ID_STRID_PORTRAIT */ 
	{ "Flor", 4 },			/* LV_PLUGIN_STRING_ID_STRID_FLOWER */ 
	{ "Modo inteligente", 16 },			/* LV_PLUGIN_STRING_ID_STRID_SMART */ 
	{ "Backlight", 9 },			/* LV_PLUGIN_STRING_ID_STRID_BACKLIGHT */ 
	{ "Color", 5 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR */ 
	{ "Normal", 6 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_FULL */ 
	{ "B/N", 3 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_BW */ 
	{ "Sepia", 5 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_SEPIA */ 
	{ "Definición", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SHARPNESS */ 
	{ "Alta", 4 },			/* LV_PLUGIN_STRING_ID_STRID_STRONG */ 
	{ "Baja", 4 },			/* LV_PLUGIN_STRING_ID_STRID_SOFT */ 
	{ "Baja", 4 },			/* LV_PLUGIN_STRING_ID_STRID_LOW */ 
	{ "Media", 5 },			/* LV_PLUGIN_STRING_ID_STRID_MED */ 
	{ "Alta", 4 },			/* LV_PLUGIN_STRING_ID_STRID_HIGH */ 
	{ "Impresión Fecha", 16 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_STAMP */ 
	{ "Fecha/Hora", 10 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_TIME */ 
	{ "Fecha", 5 },			/* LV_PLUGIN_STRING_ID_STRID_DATE */ 
	{ "Configurar Hora", 15 },			/* LV_PLUGIN_STRING_ID_STRID_TIME */ 
	{ "a/m/d", 5 },			/* LV_PLUGIN_STRING_ID_STRID_Y_M_D */ 
	{ "d/m/a", 5 },			/* LV_PLUGIN_STRING_ID_STRID_D_M_Y */ 
	{ "m/d/a", 5 },			/* LV_PLUGIN_STRING_ID_STRID_M_D_Y */ 
	{ "Encendido", 9 },			/* LV_PLUGIN_STRING_ID_STRID_ON */ 
	{ "Apagado", 7 },			/* LV_PLUGIN_STRING_ID_STRID_OFF */ 
	{ "Zoom Digital", 12 },			/* LV_PLUGIN_STRING_ID_STRID_DZ */ 
	{ "Revisión Rápida", 17 },			/* LV_PLUGIN_STRING_ID_STRID_QUICK_VIEW */ 
	{ "Saturación", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SATURATION */ 
	{ "Vídeo", 6 },			/* LV_PLUGIN_STRING_ID_STRID_MOVIE */ 
	{ "Tarifa del macro", 16 },			/* LV_PLUGIN_STRING_ID_STRID_FRAMERATE */ 
	{ "30 fps", 6 },			/* LV_PLUGIN_STRING_ID_STRID_30FPS */ 
	{ "15 fps", 6 },			/* LV_PLUGIN_STRING_ID_STRID_15FPS */ 
	{ "Vídeo", 6 },			/* LV_PLUGIN_STRING_ID_STRID_VIDEO */ 
	{ "Sonido", 6 },			/* LV_PLUGIN_STRING_ID_STRID_AUDIO */ 
	{ "Configuración", 14 },			/* LV_PLUGIN_STRING_ID_STRID_SETUP */ 
	{ "Frecuencia", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FREQUENCY */ 
	{ "50 Hz", 5 },			/* LV_PLUGIN_STRING_ID_STRID_50HZ */ 
	{ "60 Hz", 5 },			/* LV_PLUGIN_STRING_ID_STRID_60HZ */ 
	{ "Medios selectos", 15 },			/* LV_PLUGIN_STRING_ID_STRID_MEDIA */ 
	{ "Medio de almacenamiento", 23 },			/* LV_PLUGIN_STRING_ID_STRID_STORAGE */ 
	{ "Memoria Interna", 15 },			/* LV_PLUGIN_STRING_ID_STRID_INT_FLASH */ 
	{ "Tarjeta SD", 10 },			/* LV_PLUGIN_STRING_ID_STRID_EXT_CARD */ 
	{ "Exhibición de la insignia", 26 },			/* LV_PLUGIN_STRING_ID_STRID_LOGO_DISPLAY */ 
	{ "Apertura", 8 },			/* LV_PLUGIN_STRING_ID_STRID_OPENING */ 
	{ "Formato", 7 },			/* LV_PLUGIN_STRING_ID_STRID_FORMAT */ 
	{ "OK", 2 },			/* LV_PLUGIN_STRING_ID_STRID_OK */ 
	{ "Cancelar", 8 },			/* LV_PLUGIN_STRING_ID_STRID_CANCEL */ 
	{ "Sonido Bip", 10 },			/* LV_PLUGIN_STRING_ID_STRID_BEEPER */ 
	{ "Alto", 4 },			/* LV_PLUGIN_STRING_ID_STRID_BEEP_LOUD */ 
	{ "Suave", 5 },			/* LV_PLUGIN_STRING_ID_STRID_BEEP_SOFT */ 
	{ "Idioma", 6 },			/* LV_PLUGIN_STRING_ID_STRID_LANGUAGE */ 
	{ "English", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_EN */ 
	{ "Français", 9 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_FR */ 
	{ "Español", 8 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_ES */ 
	{ "Deutsch", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_DE */ 
	{ "Italiano", 8 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_IT */ 
	{ "繁體中文", 12 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_TC */ 
	{ "简体中文", 12 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_SC */ 
	{ "日本語", 9 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_JP */ 
	{ "Português", 10 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_PO */ 
	{ "Русский", 14 },			/* LV_PLUGIN_STRING_ID_STRID_LANG_RU */ 
	{ "USB", 3 },			/* LV_PLUGIN_STRING_ID_STRID_USB */ 
	{ "Modo de la PC", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PC_MODE */ 
	{ "Memoria de masa", 15 },			/* LV_PLUGIN_STRING_ID_STRID_MSDC */ 
	{ "Cámara PC", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PCC */ 
	{ "Traspasar Fotos", 15 },			/* LV_PLUGIN_STRING_ID_STRID_PICTBRIDGE */ 
	{ "Power Charging", 14 },			/* LV_PLUGIN_STRING_ID_STRID_USBCHARGE */ 
	{ "Modo de la TV", 13 },			/* LV_PLUGIN_STRING_ID_STRID_TV_MODE */ 
	{ "NTSC", 4 },			/* LV_PLUGIN_STRING_ID_STRID_TV_NTSC */ 
	{ "PAL", 3 },			/* LV_PLUGIN_STRING_ID_STRID_TV_PAL */ 
	{ "Luminosidad LCD", 15 },			/* LV_PLUGIN_STRING_ID_STRID_BRIGHTNESS */ 
	{ "Desconexión Automática", 24 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF */ 
	{ "1 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_1MIN */ 
	{ "2 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_2MIN */ 
	{ "3 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_3MIN */ 
	{ "5 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_5MIN */ 
	{ "10 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_10MIN */ 
	{ "1 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_1MIN */ 
	{ "2 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_2MIN */ 
	{ "3 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_3MIN */ 
	{ "5 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_5MIN */ 
	{ "10 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_10MIN */ 
	{ "15 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_15MIN */ 
	{ "20 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_20MIN */ 
	{ "25 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_25MIN */ 
	{ "30 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_30MIN */ 
	{ "60 Min", 6 },			/* LV_PLUGIN_STRING_ID_STRID_60MIN */ 
	{ "Screen Save", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SCREEN_SAVE */ 
	{ "Grabación continua", 19 },			/* LV_PLUGIN_STRING_ID_STRID_CYCLIC_REC */ 
	{ "Flash Record", 12 },			/* LV_PLUGIN_STRING_ID_STRID_FLASH_REC */ 
	{ "Golf Shot", 9 },			/* LV_PLUGIN_STRING_ID_STRID_GOLF_REC */ 
	{ "Dual Record", 11 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_REC */ 
	{ "Lapso de tiempo", 15 },			/* LV_PLUGIN_STRING_ID_STRID_TIMELAPSE_REC */ 
	{ "Lapso de tiempo", 15 },			/* LV_PLUGIN_STRING_ID_STRID_TIMELAPSE_CAP */ 
	{ "Make Movie by Photo", 19 },			/* LV_PLUGIN_STRING_ID_STRID_MAKE_MOVIE */ 
	{ "Procesando…", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PROCESSING */ 
	{ "Ningunos", 8 },			/* LV_PLUGIN_STRING_ID_STRID_NONE */ 
	{ "Rest. Núm.", 11 },			/* LV_PLUGIN_STRING_ID_STRID_RESET_NUM */ 
	{ "Configuración por defecto", 26 },			/* LV_PLUGIN_STRING_ID_STRID_DEFAULT_SETTING */ 
	{ "Sí", 3 },			/* LV_PLUGIN_STRING_ID_STRID_YES */ 
	{ "No", 2 },			/* LV_PLUGIN_STRING_ID_STRID_NO */ 
	{ "Versión", 8 },			/* LV_PLUGIN_STRING_ID_STRID_VERSION */ 
	{ "playback", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PLAYBACK */ 
	{ "Thumbnail", 9 },			/* LV_PLUGIN_STRING_ID_STRID_THUMBNAIL */ 
	{ "Fije la insignia", 16 },			/* LV_PLUGIN_STRING_ID_STRID_SET_LOGO */ 
	{ "Rote", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE */ 
	{ "Rote", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_S */ 
	{ "Rote 90°", 9 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_90 */ 
	{ "Rote 180°", 10 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_180 */ 
	{ "Rote 270°", 10 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_270 */ 
	{ "Cosecha", 7 },			/* LV_PLUGIN_STRING_ID_STRID_CROP */ 
	{ "Cultiv", 6 },			/* LV_PLUGIN_STRING_ID_STRID_CROP_S */ 
	{ "Ver Diapositivas", 16 },			/* LV_PLUGIN_STRING_ID_STRID_SLIDE_SHOW */ 
	{ "2 Segundos", 10 },			/* LV_PLUGIN_STRING_ID_STRID_2SEC */ 
	{ "3 Segundos", 10 },			/* LV_PLUGIN_STRING_ID_STRID_3SEC */ 
	{ "5 Segundos", 10 },			/* LV_PLUGIN_STRING_ID_STRID_5SEC */ 
	{ "8 Segundos", 10 },			/* LV_PLUGIN_STRING_ID_STRID_8SEC */ 
	{ "10 Segundos", 11 },			/* LV_PLUGIN_STRING_ID_STRID_10SEC */ 
	{ "Proteger", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECT */ 
	{ "Proteger Una", 12 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECTONE */ 
	{ "Prot. Todas", 11 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECTALL */ 
	{ "Bloquear", 8 },			/* LV_PLUGIN_STRING_ID_STRID_LOCK */ 
	{ "Trabe el actual", 15 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKONE */ 
	{ "Blog. Todo", 10 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKALL */ 
	{ "Blog. Seleccionado", 18 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKSELECTED */ 
	{ "Desbloq.", 8 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCK */ 
	{ "Abra el actual", 14 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKONE */ 
	{ "Desblog. Todo", 13 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKALL */ 
	{ "Abra Seleccionado", 17 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKSELECTED */ 
	{ "DPOF", 4 },			/* LV_PLUGIN_STRING_ID_STRID_DPOF */ 
	{ "Una Imagen", 10 },			/* LV_PLUGIN_STRING_ID_STRID_ONEIMAGE */ 
	{ "Todas las imágenes", 19 },			/* LV_PLUGIN_STRING_ID_STRID_ALL_IMAGES */ 
	{ "Esta Imagen", 11 },			/* LV_PLUGIN_STRING_ID_STRID_THIS_IMAGE */ 
	{ "Esta Imagen", 11 },			/* LV_PLUGIN_STRING_ID_STRID_THIS_VIDEO */ 
	{ "Seleccionar Imagen", 18 },			/* LV_PLUGIN_STRING_ID_STRID_SELECT_IMAGES */ 
	{ "Todo el índice", 15 },			/* LV_PLUGIN_STRING_ID_STRID_ALL_INDEX */ 
	{ "Todas", 5 },			/* LV_PLUGIN_STRING_ID_STRID_ALL */ 
	{ "Formate. Todo", 13 },			/* LV_PLUGIN_STRING_ID_STRID_RESETALL */ 
	{ "Cantidad", 8 },			/* LV_PLUGIN_STRING_ID_STRID_COPIES */ 
	{ "Volver", 6 },			/* LV_PLUGIN_STRING_ID_STRID_RETURN */ 
	{ "Cambio Dimens.", 14 },			/* LV_PLUGIN_STRING_ID_STRID_RESIZE */ 
	{ "Cambio Calidad", 14 },			/* LV_PLUGIN_STRING_ID_STRID_QUALITYCHANGE */ 
	{ "Copia a tarjeta", 15 },			/* LV_PLUGIN_STRING_ID_STRID_COPY_TO_CARD */ 
	{ "Copia de tarjeta", 16 },			/* LV_PLUGIN_STRING_ID_STRID_COPY_FROM_CARD */ 
	{ "Copy", 4 },			/* LV_PLUGIN_STRING_ID_STRID_COPY */ 
	{ "Al Sd", 5 },			/* LV_PLUGIN_STRING_ID_STRID_TOSD */ 
	{ "A Interno", 9 },			/* LV_PLUGIN_STRING_ID_STRID_TOINTERNAL */ 
	{ "Borrar", 6 },			/* LV_PLUGIN_STRING_ID_STRID_DELETE */ 
	{ "Suprime el actual", 17 },			/* LV_PLUGIN_STRING_ID_STRID_DELETECURRENT */ 
	{ "Borrar todo", 11 },			/* LV_PLUGIN_STRING_ID_STRID_DELETEALL */ 
	{ "Borrar selecta", 14 },			/* LV_PLUGIN_STRING_ID_STRID_DELETESELECTED */ 
	{ "Los datos ya procesados", 23 },			/* LV_PLUGIN_STRING_ID_STRID_DATAPROCESSED */ 
	{ "Selecc.", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SELECT */ 
	{ "Página", 7 },			/* LV_PLUGIN_STRING_ID_STRID_PAGE */ 
	{ "Selecc. modo impr.", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTMODESEL */ 
	{ "Tamaño", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SIZE */ 
	{ "Estándar", 9 },			/* LV_PLUGIN_STRING_ID_STRID_STANDARD */ 
	{ "Continuar", 9 },			/* LV_PLUGIN_STRING_ID_STRID_CONTINUE */ 
	{ "Sin", 3 },			/* LV_PLUGIN_STRING_ID_STRID_WITHOUT */ 
	{ "Con", 3 },			/* LV_PLUGIN_STRING_ID_STRID_WITH */ 
	{ "Nombre de archivo", 17 },			/* LV_PLUGIN_STRING_ID_STRID_NAME */ 
	{ "Ninguna Imagen", 14 },			/* LV_PLUGIN_STRING_ID_STRID_NO_IMAGE */ 
	{ "Definir", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SET */ 
	{ "Memoria Interna Completo", 24 },			/* LV_PLUGIN_STRING_ID_STRID_MEMORY_FULL */ 
	{ "Tarjeta Completo", 16 },			/* LV_PLUGIN_STRING_ID_STRID_CARD_FULL */ 
	{ "Carpeta llena", 13 },			/* LV_PLUGIN_STRING_ID_STRID_FOLDERFULL */ 
	{ "Error de tarjeta", 16 },			/* LV_PLUGIN_STRING_ID_STRID_CARDERROR */ 
	{ "Error de memoria", 16 },			/* LV_PLUGIN_STRING_ID_STRID_MEMORYERROR */ 
	{ "Error en el objetivo", 20 },			/* LV_PLUGIN_STRING_ID_STRID_LENSERROR */ 
	{ "Tarjeta Protegió", 17 },			/* LV_PLUGIN_STRING_ID_STRID_CARD_LOCKED */ 
	{ "Protegido!", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_PROTECT */ 
	{ "Error de imagen", 15 },			/* LV_PLUGIN_STRING_ID_STRID_PICTUREERROR */ 
	{ "Batería Baja", 13 },			/* LV_PLUGIN_STRING_ID_STRID_BATTERY_LOW */ 
	{ "Un momento", 10 },			/* LV_PLUGIN_STRING_ID_STRID_ONEMOMENT */ 
	{ "Sin conexión", 13 },			/* LV_PLUGIN_STRING_ID_STRID_NOCONNECTION */ 
	{ "Transfiriendo", 13 },			/* LV_PLUGIN_STRING_ID_STRID_TRANSFERRING */ 
	{ "Conexión al PC", 15 },			/* LV_PLUGIN_STRING_ID_STRID_CONNECTEDTOPC */ 
	{ "Quitar cable USB", 16 },			/* LV_PLUGIN_STRING_ID_STRID_REMOVEUSBCABLE */ 
	{ "Total", 5 },			/* LV_PLUGIN_STRING_ID_STRID_TOTAL */ 
	{ "Impresión fecha", 16 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_STAMPING */ 
	{ "Nombre de archivo", 17 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_NAME */ 
	{ "No. de copia", 12 },			/* LV_PLUGIN_STRING_ID_STRID_NO_OF_COPY */ 
	{ "Predeterminado", 14 },			/* LV_PLUGIN_STRING_ID_STRID_DEFAULT */ 
	{ "Menú", 5 },			/* LV_PLUGIN_STRING_ID_STRID_MENU */ 
	{ "Salir", 5 },			/* LV_PLUGIN_STRING_ID_STRID_EXIT */ 
	{ "Por favor espere", 16 },			/* LV_PLUGIN_STRING_ID_STRID_PLEASE_WAIT */ 
	{ "Comienzo: Shutter", 17 },			/* LV_PLUGIN_STRING_ID_STRID_STARTSHUTTER */ 
	{ "Parada: Shutter", 15 },			/* LV_PLUGIN_STRING_ID_STRID_STOPSHUTTER */ 
	{ "Sin archivo", 11 },			/* LV_PLUGIN_STRING_ID_STRID_NO_FILE */ 
	{ "Ninguna Tarjeta", 15 },			/* LV_PLUGIN_STRING_ID_STRID_NO_CARD */ 
	{ "Sin imagen", 10 },			/* LV_PLUGIN_STRING_ID_STRID_NO_PHOTO */ 
	{ "No hay archivo JPEG", 19 },			/* LV_PLUGIN_STRING_ID_STRID_NOT_JPEG */ 
	{ "Flash not ready", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHNOTREADY */ 
	{ "Update Graph", 12 },			/* LV_PLUGIN_STRING_ID_STRID_UPDATEBG */ 
	{ "Check Version", 13 },			/* LV_PLUGIN_STRING_ID_STRID_CHECKVERSION */ 
	{ "PStore Error", 12 },			/* LV_PLUGIN_STRING_ID_STRID_PSTOREERR */ 
	{ "Juego", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PLAY */ 
	{ "Palse", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PAUSE */ 
	{ "Stop", 4 },			/* LV_PLUGIN_STRING_ID_STRID_STOP */ 
	{ "Mover", 5 },			/* LV_PLUGIN_STRING_ID_STRID_MOVE */ 
	{ "Except", 6 },			/* LV_PLUGIN_STRING_ID_STRID_SAVE */ 
	{ "Después", 8 },			/* LV_PLUGIN_STRING_ID_STRID_NEXT */ 
	{ "Cambio", 6 },			/* LV_PLUGIN_STRING_ID_STRID_CHANGE */ 
	{ "Volumen", 7 },			/* LV_PLUGIN_STRING_ID_STRID_VOLUME */ 
	{ "inmóvil", 8 },			/* LV_PLUGIN_STRING_ID_STRID_STILL */ 
	{ "Battery Type", 12 },			/* LV_PLUGIN_STRING_ID_STRID_BATTERY_TYPE */ 
	{ "Alkaline", 8 },			/* LV_PLUGIN_STRING_ID_STRID_ALKALINE */ 
	{ "NiMH", 4 },			/* LV_PLUGIN_STRING_ID_STRID_NIMH */ 
	{ "Formatear memoria\nSe borrarán todos los datos", 47 },			/* LV_PLUGIN_STRING_ID_STRID_DELETE_WARNING */ 
	{ "Num. Secuencia", 14 },			/* LV_PLUGIN_STRING_ID_STRID_SEQUENCE_NO */ 
	{ "Reconfigurar el menú\npor defecto", 34 },			/* LV_PLUGIN_STRING_ID_STRID_RESET_WARNING */ 
	{ "Borrar Ésta", 12 },			/* LV_PLUGIN_STRING_ID_STRID_ERASE_THIS */ 
	{ "Borrar ndados imágenes?", 24 },			/* LV_PLUGIN_STRING_ID_STRID_ERASE_ALL */ 
	{ "imágenes", 9 },			/* LV_PLUGIN_STRING_ID_STRID_IMAGES */ 
	{ "Por favor, conectar\nal dispositivo", 35 },			/* LV_PLUGIN_STRING_ID_STRID_CONNECT_TO_DEVICE */ 
	{ "Dispositivo\nconectado", 22 },			/* LV_PLUGIN_STRING_ID_STRID_DEVICE_IS_CONNECTED */ 
	{ "Error de vínculo", 17 },			/* LV_PLUGIN_STRING_ID_STRID_LINK_ERROR */ 
	{ "Error USB.", 10 },			/* LV_PLUGIN_STRING_ID_STRID_USB_ERROR */ 
	{ "a\\", 3 },			/* LV_PLUGIN_STRING_ID_STRID_DUMMY */ 
	{ "-1/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M0P3 */ 
	{ "-2/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M0P6 */ 
	{ "-1.0", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M1P0 */ 
	{ "-4/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M1P3 */ 
	{ "-5/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M1P6 */ 
	{ "-2.0", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_M2P0 */ 
	{ "+0.0", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P0P0 */ 
	{ "+1/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P0P3 */ 
	{ "+2/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P0P6 */ 
	{ "+1.0", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P1P0 */ 
	{ "+4/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P1P3 */ 
	{ "+5/3", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P1P6 */ 
	{ "+2.0", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EV_P2P0 */ 
	{ "TODAS LAS FOTOS", 15 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_ALL */ 
	{ "Todo el índice", 15 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_INDEX */ 
	{ "Impr. con DPOF", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_DPOF */ 
	{ "Ajustes el impresión", 21 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_SETTING */ 
	{ "Reinicie DPOF", 13 },			/* LV_PLUGIN_STRING_ID_STRID_DPOF_RESTART */ 
	{ "Imprimir", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT */ 
	{ "Comience A Imprimir", 19 },			/* LV_PLUGIN_STRING_ID_STRID_START_PRINTING */ 
	{ "Imprimiendo..", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTING */ 
	{ "Impresión cancelada", 20 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_CANCELED */ 
	{ "Impresión Terminada", 20 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_FINISHED */ 
	{ "Pulse OK para imprimir", 22 },			/* LV_PLUGIN_STRING_ID_STRID_OK_TO_PRINT */ 
	{ "Tamaño Del Papel", 17 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_SIZE */ 
	{ "L", 1 },			/* LV_PLUGIN_STRING_ID_STRID_L */ 
	{ "2L", 2 },			/* LV_PLUGIN_STRING_ID_STRID_2L */ 
	{ "Postal", 6 },			/* LV_PLUGIN_STRING_ID_STRID_POSTCARD */ 
	{ "Tarjeta", 7 },			/* LV_PLUGIN_STRING_ID_STRID_CARD */ 
	{ "100x150", 7 },			/* LV_PLUGIN_STRING_ID_STRID_100X150 */ 
	{ "4\"\"x6\"\"", 11 },			/* LV_PLUGIN_STRING_ID_STRID_4X6 */ 
	{ "8\"\"x10\"\"", 12 },			/* LV_PLUGIN_STRING_ID_STRID_8X10 */ 
	{ "Carta", 5 },			/* LV_PLUGIN_STRING_ID_STRID_LETTER */ 
	{ "11\"\"x17\"\"", 13 },			/* LV_PLUGIN_STRING_ID_STRID_11X17 */ 
	{ "A0", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A0 */ 
	{ "A1", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A1 */ 
	{ "A2", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A2 */ 
	{ "A3", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A3 */ 
	{ "A4", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A4 */ 
	{ "A5", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A5 */ 
	{ "A6", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A6 */ 
	{ "A7", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A7 */ 
	{ "A8", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A8 */ 
	{ "A9", 2 },			/* LV_PLUGIN_STRING_ID_STRID_A9 */ 
	{ "B0", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B0 */ 
	{ "B1", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B1 */ 
	{ "B2", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B2 */ 
	{ "B3", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B3 */ 
	{ "B4", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B4 */ 
	{ "B5", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B5 */ 
	{ "B6", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B6 */ 
	{ "B7", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B7 */ 
	{ "B8", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B8 */ 
	{ "B9", 2 },			/* LV_PLUGIN_STRING_ID_STRID_B9 */ 
	{ "Rollo L", 7 },			/* LV_PLUGIN_STRING_ID_STRID_L_ROLLS */ 
	{ "Rollo 2L", 8 },			/* LV_PLUGIN_STRING_ID_STRID_2L_ROLLS */ 
	{ "Rollo 4L", 8 },			/* LV_PLUGIN_STRING_ID_STRID_4_ROLLS */ 
	{ "Rollo A4", 8 },			/* LV_PLUGIN_STRING_ID_STRID_A4_ROLLS */ 
	{ "Tipo De Papel", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_TYPE */ 
	{ "Photo", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PHOTO_PAPER */ 
	{ "Plain", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PLAIN_PAPER */ 
	{ "Fast Photo", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FAST_PHOTO */ 
	{ "Tipo De Archivo", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_TYPE */ 
	{ "Exif/JPEG", 9 },			/* LV_PLUGIN_STRING_ID_STRID_EXIF_JPEG */ 
	{ "Exif", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EXIF */ 
	{ "JPEG", 4 },			/* LV_PLUGIN_STRING_ID_STRID_JPEG */ 
	{ "Impresión De La Fecha", 22 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_PRINT */ 
	{ "Impresión De La Nombre", 23 },			/* LV_PLUGIN_STRING_ID_STRID_FILENAME_PRINT */ 
	{ "La Imagen Optimiza", 18 },			/* LV_PLUGIN_STRING_ID_STRID_IMAGE_OPTIMIZE */ 
	{ "N-diseño superior", 18 },			/* LV_PLUGIN_STRING_ID_STRID_LAYOUT */ 
	{ "1-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_1UP */ 
	{ "2-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_2UP */ 
	{ "3-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_3UP */ 
	{ "4-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_4UP */ 
	{ "5-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_5UP */ 
	{ "6-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_6UP */ 
	{ "7-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_7UP */ 
	{ "8-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_8UP */ 
	{ "9-UP", 4 },			/* LV_PLUGIN_STRING_ID_STRID_9UP */ 
	{ "10-UP", 5 },			/* LV_PLUGIN_STRING_ID_STRID_10UP */ 
	{ "250-UP", 6 },			/* LV_PLUGIN_STRING_ID_STRID_250UP */ 
	{ "índice", 7 },			/* LV_PLUGIN_STRING_ID_STRID_INDEX */ 
	{ "Randlos", 7 },			/* LV_PLUGIN_STRING_ID_STRID_1UP_BORDERLESS */ 
	{ "Tamaño Fijo", 12 },			/* LV_PLUGIN_STRING_ID_STRID_FIXED_SIZE */ 
	{ "2.5\"\"x3.25\"\"", 16 },			/* LV_PLUGIN_STRING_ID_STRID_25X325 */ 
	{ "3.5\"\"x5\"\"", 13 },			/* LV_PLUGIN_STRING_ID_STRID_35X5 */ 
	{ "254x178", 7 },			/* LV_PLUGIN_STRING_ID_STRID_254X178 */ 
	{ "110x74", 6 },			/* LV_PLUGIN_STRING_ID_STRID_110X74 */ 
	{ "89x55", 5 },			/* LV_PLUGIN_STRING_ID_STRID_89X55 */ 
	{ "6x8", 3 },			/* LV_PLUGIN_STRING_ID_STRID_6X8 */ 
	{ "7x10", 4 },			/* LV_PLUGIN_STRING_ID_STRID_7X10 */ 
	{ "9x13", 4 },			/* LV_PLUGIN_STRING_ID_STRID_9X13 */ 
	{ "13x18", 5 },			/* LV_PLUGIN_STRING_ID_STRID_13X18 */ 
	{ "15x21", 5 },			/* LV_PLUGIN_STRING_ID_STRID_15X21 */ 
	{ "18x24", 5 },			/* LV_PLUGIN_STRING_ID_STRID_18X24 */ 
	{ "Cultivo", 7 },			/* LV_PLUGIN_STRING_ID_STRID_CROPPING */ 
	{ "Error de impresión", 19 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_ERROR */ 
	{ "La impresora se puede desconectar.", 34 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTER_DISCONNECTABLE */ 
	{ "Error De Papel", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_ERROR */ 
	{ "Papel Vacío", 12 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_OUT */ 
	{ "No se ha cargado papel", 22 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_LOAD_ERROR */ 
	{ "Error De Papel", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_EJECT_ERROR */ 
	{ "Error De Papel", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_MEDIA_ERROR */ 
	{ "Papel atascado", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_JAMMED */ 
	{ "Poco papel", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_NEAR_EMPTY */ 
	{ "Tipo de papel no compatible", 27 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_NOT_SUPPORT */ 
	{ "Error De Tinta", 14 },			/* LV_PLUGIN_STRING_ID_STRID_INK_ERROR */ 
	{ "Tinta vacía", 12 },			/* LV_PLUGIN_STRING_ID_STRID_INK_EMPTY */ 
	{ "Poca tinta", 10 },			/* LV_PLUGIN_STRING_ID_STRID_INK_LOW */ 
	{ "Error De Tinta", 14 },			/* LV_PLUGIN_STRING_ID_STRID_INK_WASTE */ 
	{ "Error en hardware de la impresora", 33 },			/* LV_PLUGIN_STRING_ID_STRID_HW_ERROR */ 
	{ "Error grave", 11 },			/* LV_PLUGIN_STRING_ID_STRID_HW_FATAL */ 
	{ "Llame al servicio técnico", 26 },			/* LV_PLUGIN_STRING_ID_STRID_HW_SERVICE_CALL */ 
	{ "Impresora no disponible", 23 },			/* LV_PLUGIN_STRING_ID_STRID_HW_UNAVAILABLE */ 
	{ "Impresora ocupada", 17 },			/* LV_PLUGIN_STRING_ID_STRID_HW_BUSY */ 
	{ "Error de la palanca", 19 },			/* LV_PLUGIN_STRING_ID_STRID_HW_LEVER */ 
	{ "Cubierta abierta", 16 },			/* LV_PLUGIN_STRING_ID_STRID_HW_COVER_OPEN */ 
	{ "No hay agente de marca", 22 },			/* LV_PLUGIN_STRING_ID_STRID_HW_NO_MARKING_AGENT */ 
	{ "Cubierta para tinta abierta", 27 },			/* LV_PLUGIN_STRING_ID_STRID_HW_INK_COVER_OPEN */ 
	{ "No hay cartucho de tinta", 24 },			/* LV_PLUGIN_STRING_ID_STRID_HW_NO_INK_CARTRIDGE */ 
	{ "Error De Archivo", 16 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_ERROR */ 
	{ "Inf. Impr.", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_PRINT_INFO */ 
	{ "Error de decodif. de archivo", 28 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_DECODE_ERROR */ 
	{ "Error de impresión", 19 },			/* LV_PLUGIN_STRING_ID_STRID_UNKNOW_ERROR */ 
	{ "Error de impresión", 19 },			/* LV_PLUGIN_STRING_ID_STRID_XML_SYNTAX_ERROR */ 
	{ "Detección de movimiento", 24 },			/* LV_PLUGIN_STRING_ID_STRID_MOTION_DET */ 
	{ "Detección de la cara", 21 },			/* LV_PLUGIN_STRING_ID_STRID_FACE_DET */ 
	{ "Detección de sonrisas", 22 },			/* LV_PLUGIN_STRING_ID_STRID_SMILE_DET */ 
	{ "Estabilización", 15 },			/* LV_PLUGIN_STRING_ID_STRID_ANTI_SHAKING */ 
	{ "Ninguna película", 17 },			/* LV_PLUGIN_STRING_ID_STRID_NO_MOVIE */ 
	{ "MP3", 3 },			/* LV_PLUGIN_STRING_ID_STRID_MP3PLAY */ 
	{ "N de archivos MP3", 17 },			/* LV_PLUGIN_STRING_ID_STRID_NO_MP3FILE */ 
	{ "Rotación de imagen ", 20 },			/* LV_PLUGIN_STRING_ID_STRID_SENSOR_ROTATE */ 
	{ "Expediente del vídeo", 21 },			/* LV_PLUGIN_STRING_ID_STRID_RECORD */ 
	{ "Audio de registro", 17 },			/* LV_PLUGIN_STRING_ID_STRID_RECORD_AUDIO */ 
	{ "Ajuste de la lámpara", 21 },			/* LV_PLUGIN_STRING_ID_STRID_LED_SETTING */ 
	{ "ADIÓS", 6 },			/* LV_PLUGIN_STRING_ID_STRID_GOOD_BYE */ 
	{ "Inserte tarjeta SD", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD */ 
	{ "IR LED", 6 },			/* LV_PLUGIN_STRING_ID_STRID_IR_LED */ 
	{ "micrófono", 10 },			/* LV_PLUGIN_STRING_ID_STRID_MICROPHONE */ 
	{ "Custer size wrong.\nPlease format", 33 },			/* LV_PLUGIN_STRING_ID_STRID_CLUSTER_WRONG */ 
	{ "Need Class6 or Higher SD Card", 29 },			/* LV_PLUGIN_STRING_ID_STRID_SD_CLASS6 */ 
	{ "Need Class4 or Higher SD Card", 29 },			/* LV_PLUGIN_STRING_ID_STRID_SD_CLASS4 */ 
	{ "HDR", 3 },			/* LV_PLUGIN_STRING_ID_STRID_HDR */ 
	{ "Time Lapse", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TIME_LPASE */ 
	{ "100 ms", 6 },			/* LV_PLUGIN_STRING_ID_STRID_100MS */ 
	{ "200 ms", 6 },			/* LV_PLUGIN_STRING_ID_STRID_200MS */ 
	{ "500 ms", 6 },			/* LV_PLUGIN_STRING_ID_STRID_500MS */ 
	{ "_", 1 },			/* LV_PLUGIN_STRING_ID_STRID_UNDERSCORE */ 
	{ "WiFi", 4 },			/* LV_PLUGIN_STRING_ID_STRID_WIFI */ 
	{ "WiFi_OFF", 8 },			/* LV_PLUGIN_STRING_ID_STRID_WIFI_OFF */ 
	{ "Refresh", 7 },			/* LV_PLUGIN_STRING_ID_STRID_REFRESH */ 
	{ "AP mode", 7 },			/* LV_PLUGIN_STRING_ID_STRID_WIFI_AP_MODE */ 
	{ "Client mode", 11 },			/* LV_PLUGIN_STRING_ID_STRID_WIFI_CLIENT_MODE */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FILL_ASCII */ 
	{ "Switch Mode", 11 },			/* LV_PLUGIN_STRING_ID_STRID_MODE_SWITCH */ 
	{ "2880x2160P24", 12 },			/* LV_PLUGIN_STRING_ID_STRID_2880X2160P24 */ 
	{ "2560x1440P30", 12 },			/* LV_PLUGIN_STRING_ID_STRID_2560X1440P30 */ 
	{ "2304x1296P30", 12 },			/* LV_PLUGIN_STRING_ID_STRID_2304X1296P30 */ 
	{ "1080P96", 7 },			/* LV_PLUGIN_STRING_ID_STRID_1080P96 */ 
	{ "1080P60", 7 },			/* LV_PLUGIN_STRING_ID_STRID_1080P60 */ 
	{ "1080P_DUAL", 10 },			/* LV_PLUGIN_STRING_ID_STRID_1080P_DUAL */ 
	{ "720P120", 7 },			/* LV_PLUGIN_STRING_ID_STRID_720P120WXH */ 
	{ "WDR", 3 },			/* LV_PLUGIN_STRING_ID_STRID_WDR */ 
	{ "RSC", 3 },			/* LV_PLUGIN_STRING_ID_STRID_RSC */ 
	{ "G Sensor", 8 },			/* LV_PLUGIN_STRING_ID_STRID_G_SENSOR */ 
	{ "1 Sec", 5 },			/* LV_PLUGIN_STRING_ID_STRID_1SEC */ 
	{ "30 Sec", 6 },			/* LV_PLUGIN_STRING_ID_STRID_30SEC */ 
	{ "1 Hour", 6 },			/* LV_PLUGIN_STRING_ID_STRID_1HOUR */ 
	{ "2 Hour", 6 },			/* LV_PLUGIN_STRING_ID_STRID_2HOUR */ 
	{ "3 Hour", 6 },			/* LV_PLUGIN_STRING_ID_STRID_3HOUR */ 
	{ "1 Day", 5 },			/* LV_PLUGIN_STRING_ID_STRID_1DAY */ 
	{ "IR Cut", 6 },			/* LV_PLUGIN_STRING_ID_STRID_IRCUT */ 
	{ "Dual Cam Display", 16 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_CAM */ 
	{ "Front", 5 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT */ 
	{ "Behind", 6 },			/* LV_PLUGIN_STRING_ID_STRID_BEHIND */ 
	{ "Both", 4 },			/* LV_PLUGIN_STRING_ID_STRID_BOTH */ 
	{ "PTZ", 3 },			/* LV_PLUGIN_STRING_ID_STRID_PTZ */ 
	{ "Auto Urgent Protect", 19 },			/* LV_PLUGIN_STRING_ID_STRID_URGENT_PROTECT_AUTO */ 
	{ "Manual Urgent Protect", 21 },			/* LV_PLUGIN_STRING_ID_STRID_URGENT_PROTECT_MANUAL */ 
	{ "PIM", 3 },			/* LV_PLUGIN_STRING_ID_STRID_PIM */ 
	{ "FCW", 3 },			/* LV_PLUGIN_STRING_ID_STRID_FCW */ 
	{ "LDWS", 4 },			/* LV_PLUGIN_STRING_ID_STRID_LDWS */ 
	{ "DDD", 3 },			/* LV_PLUGIN_STRING_ID_STRID_DDD */ 
	{ "ADAS", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ADAS */ 
	{ "File Recovery", 13 },			/* LV_PLUGIN_STRING_ID_STRID_REC_RECOVERY */ 
	{ "Self Timer", 10 },			/* LV_PLUGIN_STRING_ID_STRID_SELFTIMER */ 
	{ "Portrial", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PORTRIAL */ 
	{ "Landscpe", 8 },			/* LV_PLUGIN_STRING_ID_STRID_LANDSCPE */ 
	{ "0", 1 },			/* LV_PLUGIN_STRING_ID_STRID_0 */ 
	{ "1", 1 },			/* LV_PLUGIN_STRING_ID_STRID_1 */ 
	{ "2", 1 },			/* LV_PLUGIN_STRING_ID_STRID_2 */ 
	{ "3", 1 },			/* LV_PLUGIN_STRING_ID_STRID_3 */ 
	{ "4", 1 },			/* LV_PLUGIN_STRING_ID_STRID_4 */ 
	{ "5", 1 },			/* LV_PLUGIN_STRING_ID_STRID_5 */ 
	{ "6", 1 },			/* LV_PLUGIN_STRING_ID_STRID_6 */ 
	{ "7", 1 },			/* LV_PLUGIN_STRING_ID_STRID_7 */ 
	{ "8", 1 },			/* LV_PLUGIN_STRING_ID_STRID_8 */ 
	{ "9", 1 },			/* LV_PLUGIN_STRING_ID_STRID_9 */ 
	{ "10", 2 },			/* LV_PLUGIN_STRING_ID_STRID_10 */ 
	{ "Cloud", 5 },			/* LV_PLUGIN_STRING_ID_STRID_CLOUD */ 
	{ "Firmware Update", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FW_UPDATE */ 
	{ "UVC", 3 },			/* LV_PLUGIN_STRING_ID_STRID_UVC */ 
	{ "Resume", 6 },			/* LV_PLUGIN_STRING_ID_STRID_RESUME */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_MCTF */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_EDGE */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_NR */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_WIFI_ETH */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ISO6400 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ISO12800 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2880X2160P50 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_3840X2160P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2880X2160P24 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2704X2032P60 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2560X1440P80 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2560X1440P60 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2560X1440P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_2304X1296P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1920X1080P120 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1920X1080P96 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1920X1080P60 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1280X720P240 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1280X720P120 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1280X720P60 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_1280X720P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_640X480P240 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_640X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_320X240P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_2560X1440P30_1280X720P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_2560X1440P30_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_2304X1296P30_1280X720P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_1080P30_1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_1920X1080P30_1280X720P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_1920X1080P30_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_1920X1080P30_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_1920X1080P30_1280X720P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_2560X1440P30_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_2304X1296P30_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_1920X1080P60_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_1920X1080P60_640X360P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_1920X1080P30_848X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CLONE_2048X2048P30_480X480P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_BOTH2 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_SIDE */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_BURST_30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_5MWXH_USR */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_CODEC */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_MJPG */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_H264 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_H265 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_2880X2160P50 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_3840X2160P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_FRONT_848X480P60 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_TRI_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_TRI_2560X1440P30_1920X1080P30_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_QUAD_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_3840X2160P30_1920X1080P30 */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ETHCAM_RESTART_REC */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ETHCAM_STOP_REC */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_SEND */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_START */ 
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_FINISH */ 
	{ "40M", 3 },			/* LV_PLUGIN_STRING_ID_STRID_40M */
};



