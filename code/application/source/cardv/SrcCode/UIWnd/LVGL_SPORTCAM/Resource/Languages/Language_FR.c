#include "Resource/Plugin/lv_plugin_common.h"
lv_plugin_string_t lv_plugin_FR_string_table[] = {
	{ NULL, 0 },
	{ "", 0 },			/* LV_PLUGIN_STRING_ID_STRID_NULL_ */ 
	{ "Mode", 4 },			/* LV_PLUGIN_STRING_ID_STRID_MODE */ 
	{ "Taille Im.", 10 },			/* LV_PLUGIN_STRING_ID_STRID_IMGSIZE */ 
	{ "Resolution", 10 },			/* LV_PLUGIN_STRING_ID_STRID_RESOLUTION */ 
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
	{ "Compression", 11 },			/* LV_PLUGIN_STRING_ID_STRID_COMPRESSION */ 
	{ "Qualité", 8 },			/* LV_PLUGIN_STRING_ID_STRID_QUALITY */ 
	{ "Super Forte", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SUPER */ 
	{ "Forte", 5 },			/* LV_PLUGIN_STRING_ID_STRID_FINE */ 
	{ "Normale", 7 },			/* LV_PLUGIN_STRING_ID_STRID_NORMAL */ 
	{ "Économie", 9 },			/* LV_PLUGIN_STRING_ID_STRID_ECONOMY */ 
	{ "Balance Blancs", 14 },			/* LV_PLUGIN_STRING_ID_STRID_WB */ 
	{ "Auto", 4 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO */ 
	{ "Ensoleille", 10 },			/* LV_PLUGIN_STRING_ID_STRID_WB_DAY */ 
	{ "Nuageux", 7 },			/* LV_PLUGIN_STRING_ID_STRID_WB_CLOUDY */ 
	{ "Tungstene", 9 },			/* LV_PLUGIN_STRING_ID_STRID_WB_TUNGSTEN */ 
	{ "Fluor", 5 },			/* LV_PLUGIN_STRING_ID_STRID_WB_FLUORESCENT */ 
	{ "Exposition", 10 },			/* LV_PLUGIN_STRING_ID_STRID_EXPOSURE */ 
	{ "Exposition", 10 },			/* LV_PLUGIN_STRING_ID_STRID_EV */ 
	{ "ISO", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO */ 
	{ "50", 2 },			/* LV_PLUGIN_STRING_ID_STRID_ISO50 */ 
	{ "100", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO100 */ 
	{ "200", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO200 */ 
	{ "400", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO400 */ 
	{ "800", 3 },			/* LV_PLUGIN_STRING_ID_STRID_ISO800 */ 
	{ "1600", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ISO1600 */ 
	{ "3200", 4 },			/* LV_PLUGIN_STRING_ID_STRID_ISO3200 */ 
	{ "MesureExpos", 11 },			/* LV_PLUGIN_STRING_ID_STRID_METERING */ 
	{ "Centrale Pondérée", 19 },			/* LV_PLUGIN_STRING_ID_STRID_METER_CENTER */ 
	{ "Point", 5 },			/* LV_PLUGIN_STRING_ID_STRID_METER_SPOT */ 
	{ "Moyenne des", 11 },			/* LV_PLUGIN_STRING_ID_STRID_METER_AVG */ 
	{ "Mode Capture", 12 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_MODE */ 
	{ "Simple", 6 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_SINGLE */ 
	{ "Continuer", 9 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_BURST */ 
	{ "3 Continuer", 11 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_BURST_3 */ 
	{ "AEB", 3 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_AUTO */ 
	{ "2s temporisateur", 16 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER2S */ 
	{ "5s temporisateur", 16 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER5S */ 
	{ "10s temporisateur", 17 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER10S */ 
	{ "20s temporisateur", 17 },			/* LV_PLUGIN_STRING_ID_STRID_CAP_TIMER20S */ 
	{ "Mode instantané", 16 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHMODE */ 
	{ "Force dessus", 12 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHON */ 
	{ "Force au loin", 13 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHOFF */ 
	{ "Auto", 4 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHAUTO */ 
	{ "Rouge-oeil réduction", 21 },			/* LV_PLUGIN_STRING_ID_STRID_REDEYE */ 
	{ "Av", 2 },			/* LV_PLUGIN_STRING_ID_STRID_AV */ 
	{ "Tv", 2 },			/* LV_PLUGIN_STRING_ID_STRID_TV */ 
	{ "Av / Tv", 7 },			/* LV_PLUGIN_STRING_ID_STRID_AVTV */ 
	{ "Réglage Av:", 12 },			/* LV_PLUGIN_STRING_ID_STRID_AVSETTING */ 
	{ "Réglage Tv:", 12 },			/* LV_PLUGIN_STRING_ID_STRID_TVSETTING */ 
	{ "Réglage Av/Tv:", 15 },			/* LV_PLUGIN_STRING_ID_STRID_AVTVSETTING */ 
	{ "Paysage", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SCENESETTING */ 
	{ "Paysage", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SCENE */ 
	{ "Paysage", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LANDSCAPE */ 
	{ "Scène de nuit", 14 },			/* LV_PLUGIN_STRING_ID_STRID_NIGHTSCENE */ 
	{ "Sports", 6 },			/* LV_PLUGIN_STRING_ID_STRID_SPORTS */ 
	{ "Portrait", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PORTRAIT */ 
	{ "Fleur", 5 },			/* LV_PLUGIN_STRING_ID_STRID_FLOWER */ 
	{ "Mode intelligent", 16 },			/* LV_PLUGIN_STRING_ID_STRID_SMART */ 
	{ "Backlight", 9 },			/* LV_PLUGIN_STRING_ID_STRID_BACKLIGHT */ 
	{ "Color", 5 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR */ 
	{ "Normal", 6 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_FULL */ 
	{ "N/B", 3 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_BW */ 
	{ "Sépia", 6 },			/* LV_PLUGIN_STRING_ID_STRID_COLOR_SEPIA */ 
	{ "Nettete", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SHARPNESS */ 
	{ "Forte", 5 },			/* LV_PLUGIN_STRING_ID_STRID_STRONG */ 
	{ "Douce", 5 },			/* LV_PLUGIN_STRING_ID_STRID_SOFT */ 
	{ "Faible", 6 },			/* LV_PLUGIN_STRING_ID_STRID_LOW */ 
	{ "Moyen", 5 },			/* LV_PLUGIN_STRING_ID_STRID_MED */ 
	{ "Supérieur", 10 },			/* LV_PLUGIN_STRING_ID_STRID_HIGH */ 
	{ "Marque Date", 11 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_STAMP */ 
	{ "Date/Heure", 10 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_TIME */ 
	{ "Date", 4 },			/* LV_PLUGIN_STRING_ID_STRID_DATE */ 
	{ "Reglage Heure", 13 },			/* LV_PLUGIN_STRING_ID_STRID_TIME */ 
	{ "a/m/j", 5 },			/* LV_PLUGIN_STRING_ID_STRID_Y_M_D */ 
	{ "j/m/a", 5 },			/* LV_PLUGIN_STRING_ID_STRID_D_M_Y */ 
	{ "m/j/a", 5 },			/* LV_PLUGIN_STRING_ID_STRID_M_D_Y */ 
	{ "Active", 6 },			/* LV_PLUGIN_STRING_ID_STRID_ON */ 
	{ "Desactive", 9 },			/* LV_PLUGIN_STRING_ID_STRID_OFF */ 
	{ "Zoom Numér.", 12 },			/* LV_PLUGIN_STRING_ID_STRID_DZ */ 
	{ "Bilan Rapide", 12 },			/* LV_PLUGIN_STRING_ID_STRID_QUICK_VIEW */ 
	{ "Saturation", 10 },			/* LV_PLUGIN_STRING_ID_STRID_SATURATION */ 
	{ "Vidéo", 6 },			/* LV_PLUGIN_STRING_ID_STRID_MOVIE */ 
	{ "Taux d'armature", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FRAMERATE */ 
	{ "30 fps", 6 },			/* LV_PLUGIN_STRING_ID_STRID_30FPS */ 
	{ "15 fps", 6 },			/* LV_PLUGIN_STRING_ID_STRID_15FPS */ 
	{ "Vidéo", 6 },			/* LV_PLUGIN_STRING_ID_STRID_VIDEO */ 
	{ "Audio", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUDIO */ 
	{ "Reglages", 8 },			/* LV_PLUGIN_STRING_ID_STRID_SETUP */ 
	{ "Fréquence", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FREQUENCY */ 
	{ "50 Hz", 5 },			/* LV_PLUGIN_STRING_ID_STRID_50HZ */ 
	{ "60 Hz", 5 },			/* LV_PLUGIN_STRING_ID_STRID_60HZ */ 
	{ "Médias choisis", 15 },			/* LV_PLUGIN_STRING_ID_STRID_MEDIA */ 
	{ "Stockage Media", 14 },			/* LV_PLUGIN_STRING_ID_STRID_STORAGE */ 
	{ "Memoire Interne", 15 },			/* LV_PLUGIN_STRING_ID_STRID_INT_FLASH */ 
	{ "Carte SD", 8 },			/* LV_PLUGIN_STRING_ID_STRID_EXT_CARD */ 
	{ "Affichage de logo", 17 },			/* LV_PLUGIN_STRING_ID_STRID_LOGO_DISPLAY */ 
	{ "Ouverture", 9 },			/* LV_PLUGIN_STRING_ID_STRID_OPENING */ 
	{ "Format", 6 },			/* LV_PLUGIN_STRING_ID_STRID_FORMAT */ 
	{ "OK", 2 },			/* LV_PLUGIN_STRING_ID_STRID_OK */ 
	{ "Annuler", 7 },			/* LV_PLUGIN_STRING_ID_STRID_CANCEL */ 
	{ "Bip Sonore", 10 },			/* LV_PLUGIN_STRING_ID_STRID_BEEPER */ 
	{ "#NAME?", 6 },			/* LV_PLUGIN_STRING_ID_STRID_BEEP_LOUD */ 
	{ "Faible", 6 },			/* LV_PLUGIN_STRING_ID_STRID_BEEP_SOFT */ 
	{ "Langues", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LANGUAGE */ 
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
	{ "Mode de PC", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PC_MODE */ 
	{ "Mémoire de masse", 17 },			/* LV_PLUGIN_STRING_ID_STRID_MSDC */ 
	{ "Caméra PC", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PCC */ 
	{ "Pictbridge", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PICTBRIDGE */ 
	{ "Power Charging", 14 },			/* LV_PLUGIN_STRING_ID_STRID_USBCHARGE */ 
	{ "Mode de TV", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TV_MODE */ 
	{ "NTSC", 4 },			/* LV_PLUGIN_STRING_ID_STRID_TV_NTSC */ 
	{ "PAL", 3 },			/* LV_PLUGIN_STRING_ID_STRID_TV_PAL */ 
	{ "LCD Brightness", 14 },			/* LV_PLUGIN_STRING_ID_STRID_BRIGHTNESS */ 
	{ "Arret Auto", 10 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF */ 
	{ "1 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_1MIN */ 
	{ "2 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_2MIN */ 
	{ "3 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_3MIN */ 
	{ "5 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_5MIN */ 
	{ "10 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_AUTO_OFF_10MIN */ 
	{ "1 Min", 5 },			/* LV_PLUGIN_STRING_ID_STRID_1MIN */ 
	{ "2 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_2MIN */ 
	{ "3 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_3MIN */ 
	{ "5 Mins", 6 },			/* LV_PLUGIN_STRING_ID_STRID_5MIN */ 
	{ "10 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_10MIN */ 
	{ "15 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_15MIN */ 
	{ "20 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_20MIN */ 
	{ "25 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_25MIN */ 
	{ "30 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_30MIN */ 
	{ "60 Mins", 7 },			/* LV_PLUGIN_STRING_ID_STRID_60MIN */ 
	{ "Screen Save", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SCREEN_SAVE */ 
	{ "Enregistrement continu", 22 },			/* LV_PLUGIN_STRING_ID_STRID_CYCLIC_REC */ 
	{ "Flash Record", 12 },			/* LV_PLUGIN_STRING_ID_STRID_FLASH_REC */ 
	{ "Golf Shot", 9 },			/* LV_PLUGIN_STRING_ID_STRID_GOLF_REC */ 
	{ "Dual Record", 11 },			/* LV_PLUGIN_STRING_ID_STRID_DUAL_REC */ 
	{ "Intervalle", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TIMELAPSE_REC */ 
	{ "Intervalle", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TIMELAPSE_CAP */ 
	{ "Make Movie by Photo", 19 },			/* LV_PLUGIN_STRING_ID_STRID_MAKE_MOVIE */ 
	{ "En cours…", 11 },			/* LV_PLUGIN_STRING_ID_STRID_PROCESSING */ 
	{ "Keine", 5 },			/* LV_PLUGIN_STRING_ID_STRID_NONE */ 
	{ "Réini.Nb", 9 },			/* LV_PLUGIN_STRING_ID_STRID_RESET_NUM */ 
	{ "Regl. Defaut", 12 },			/* LV_PLUGIN_STRING_ID_STRID_DEFAULT_SETTING */ 
	{ "Oui", 3 },			/* LV_PLUGIN_STRING_ID_STRID_YES */ 
	{ "Non", 3 },			/* LV_PLUGIN_STRING_ID_STRID_NO */ 
	{ "Version", 7 },			/* LV_PLUGIN_STRING_ID_STRID_VERSION */ 
	{ "de lecture", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PLAYBACK */ 
	{ "Ongle du pouce", 14 },			/* LV_PLUGIN_STRING_ID_STRID_THUMBNAIL */ 
	{ "Placez le logo", 14 },			/* LV_PLUGIN_STRING_ID_STRID_SET_LOGO */ 
	{ "Tournez", 7 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE */ 
	{ "Tourne", 6 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_S */ 
	{ "Tourne 90°", 11 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_90 */ 
	{ "Tourne 180°", 12 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_180 */ 
	{ "Tourne 270°", 12 },			/* LV_PLUGIN_STRING_ID_STRID_ROTATE_270 */ 
	{ "Récolte", 8 },			/* LV_PLUGIN_STRING_ID_STRID_CROP */ 
	{ "Emblav", 6 },			/* LV_PLUGIN_STRING_ID_STRID_CROP_S */ 
	{ "Diaporama", 9 },			/* LV_PLUGIN_STRING_ID_STRID_SLIDE_SHOW */ 
	{ "2 Secondes", 10 },			/* LV_PLUGIN_STRING_ID_STRID_2SEC */ 
	{ "3 Secondes", 10 },			/* LV_PLUGIN_STRING_ID_STRID_3SEC */ 
	{ "5 Secondes", 10 },			/* LV_PLUGIN_STRING_ID_STRID_5SEC */ 
	{ "8 Secondes", 10 },			/* LV_PLUGIN_STRING_ID_STRID_8SEC */ 
	{ "10 Secondes", 11 },			/* LV_PLUGIN_STRING_ID_STRID_10SEC */ 
	{ "Protection", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECT */ 
	{ "Protéger Une", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECTONE */ 
	{ "Protége Ttes", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PROTECTALL */ 
	{ "Vérouillé", 11 },			/* LV_PLUGIN_STRING_ID_STRID_LOCK */ 
	{ "Verouillez le courant", 21 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKONE */ 
	{ "Verouiller Tout", 15 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKALL */ 
	{ "Serrure Choisie", 15 },			/* LV_PLUGIN_STRING_ID_STRID_LOCKSELECTED */ 
	{ "Déverrouil", 11 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCK */ 
	{ "Ouverz le courant", 17 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKONE */ 
	{ "Déverrouiller Tout", 19 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKALL */ 
	{ "Ouvrez Choisi", 13 },			/* LV_PLUGIN_STRING_ID_STRID_UNLOCKSELECTED */ 
	{ "DPOF", 4 },			/* LV_PLUGIN_STRING_ID_STRID_DPOF */ 
	{ "Une Image", 9 },			/* LV_PLUGIN_STRING_ID_STRID_ONEIMAGE */ 
	{ "Toutes Images", 13 },			/* LV_PLUGIN_STRING_ID_STRID_ALL_IMAGES */ 
	{ "Cette Image", 11 },			/* LV_PLUGIN_STRING_ID_STRID_THIS_IMAGE */ 
	{ "Cette Image", 11 },			/* LV_PLUGIN_STRING_ID_STRID_THIS_VIDEO */ 
	{ "Select. Images", 14 },			/* LV_PLUGIN_STRING_ID_STRID_SELECT_IMAGES */ 
	{ "Tout Index", 10 },			/* LV_PLUGIN_STRING_ID_STRID_ALL_INDEX */ 
	{ "Toutes", 6 },			/* LV_PLUGIN_STRING_ID_STRID_ALL */ 
	{ "Annuler Tout", 12 },			/* LV_PLUGIN_STRING_ID_STRID_RESETALL */ 
	{ "Quantité", 9 },			/* LV_PLUGIN_STRING_ID_STRID_COPIES */ 
	{ "Retour", 6 },			/* LV_PLUGIN_STRING_ID_STRID_RETURN */ 
	{ "Rogner", 6 },			/* LV_PLUGIN_STRING_ID_STRID_RESIZE */ 
	{ "Redimensionner", 14 },			/* LV_PLUGIN_STRING_ID_STRID_QUALITYCHANGE */ 
	{ "Copie Sur Carte", 15 },			/* LV_PLUGIN_STRING_ID_STRID_COPY_TO_CARD */ 
	{ "Copie De Carte", 14 },			/* LV_PLUGIN_STRING_ID_STRID_COPY_FROM_CARD */ 
	{ "Copy", 4 },			/* LV_PLUGIN_STRING_ID_STRID_COPY */ 
	{ "ÁL'Écart-type", 15 },			/* LV_PLUGIN_STRING_ID_STRID_TOSD */ 
	{ "Á Interne", 10 },			/* LV_PLUGIN_STRING_ID_STRID_TOINTERNAL */ 
	{ "Effacer", 7 },			/* LV_PLUGIN_STRING_ID_STRID_DELETE */ 
	{ "Supprime le courant", 19 },			/* LV_PLUGIN_STRING_ID_STRID_DELETECURRENT */ 
	{ "Effacer tont", 12 },			/* LV_PLUGIN_STRING_ID_STRID_DELETEALL */ 
	{ "Effacement choisi", 17 },			/* LV_PLUGIN_STRING_ID_STRID_DELETESELECTED */ 
	{ "Les données traitées", 22 },			/* LV_PLUGIN_STRING_ID_STRID_DATAPROCESSED */ 
	{ "Sélect.", 8 },			/* LV_PLUGIN_STRING_ID_STRID_SELECT */ 
	{ "Page", 4 },			/* LV_PLUGIN_STRING_ID_STRID_PAGE */ 
	{ "Sélection mode impression", 26 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTMODESEL */ 
	{ "Taille", 6 },			/* LV_PLUGIN_STRING_ID_STRID_SIZE */ 
	{ "Standard", 8 },			/* LV_PLUGIN_STRING_ID_STRID_STANDARD */ 
	{ "Suite", 5 },			/* LV_PLUGIN_STRING_ID_STRID_CONTINUE */ 
	{ "Sans", 4 },			/* LV_PLUGIN_STRING_ID_STRID_WITHOUT */ 
	{ "Avec", 4 },			/* LV_PLUGIN_STRING_ID_STRID_WITH */ 
	{ "Nom Fichier", 11 },			/* LV_PLUGIN_STRING_ID_STRID_NAME */ 
	{ "Aucune Image ", 13 },			/* LV_PLUGIN_STRING_ID_STRID_NO_IMAGE */ 
	{ "Définir", 8 },			/* LV_PLUGIN_STRING_ID_STRID_SET */ 
	{ "Mémoire Interne Complètement", 30 },			/* LV_PLUGIN_STRING_ID_STRID_MEMORY_FULL */ 
	{ "Carte Complètement", 19 },			/* LV_PLUGIN_STRING_ID_STRID_CARD_FULL */ 
	{ "Dossier plein", 13 },			/* LV_PLUGIN_STRING_ID_STRID_FOLDERFULL */ 
	{ "Erreur carte", 12 },			/* LV_PLUGIN_STRING_ID_STRID_CARDERROR */ 
	{ "Erreur memoire", 14 },			/* LV_PLUGIN_STRING_ID_STRID_MEMORYERROR */ 
	{ "Erreur objectif", 15 },			/* LV_PLUGIN_STRING_ID_STRID_LENSERROR */ 
	{ "Carte Protégée", 16 },			/* LV_PLUGIN_STRING_ID_STRID_CARD_LOCKED */ 
	{ "Protégé !", 11 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_PROTECT */ 
	{ "Erreur d'image", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PICTUREERROR */ 
	{ "Pile basse", 10 },			/* LV_PLUGIN_STRING_ID_STRID_BATTERY_LOW */ 
	{ "Un moment", 9 },			/* LV_PLUGIN_STRING_ID_STRID_ONEMOMENT */ 
	{ "Non connecté", 13 },			/* LV_PLUGIN_STRING_ID_STRID_NOCONNECTION */ 
	{ "Transfert", 9 },			/* LV_PLUGIN_STRING_ID_STRID_TRANSFERRING */ 
	{ "Connecté au PC", 15 },			/* LV_PLUGIN_STRING_ID_STRID_CONNECTEDTOPC */ 
	{ "Retirer câble USB", 18 },			/* LV_PLUGIN_STRING_ID_STRID_REMOVEUSBCABLE */ 
	{ "Total", 5 },			/* LV_PLUGIN_STRING_ID_STRID_TOTAL */ 
	{ "Marque Date", 11 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_STAMPING */ 
	{ "Nom Fichier", 11 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_NAME */ 
	{ "No. De Copies", 13 },			/* LV_PLUGIN_STRING_ID_STRID_NO_OF_COPY */ 
	{ "Défaut", 7 },			/* LV_PLUGIN_STRING_ID_STRID_DEFAULT */ 
	{ "Menu", 4 },			/* LV_PLUGIN_STRING_ID_STRID_MENU */ 
	{ "Quitter", 7 },			/* LV_PLUGIN_STRING_ID_STRID_EXIT */ 
	{ "Patientez", 9 },			/* LV_PLUGIN_STRING_ID_STRID_PLEASE_WAIT */ 
	{ "Début: Shutter", 15 },			/* LV_PLUGIN_STRING_ID_STRID_STARTSHUTTER */ 
	{ "Arrét: Shutter", 15 },			/* LV_PLUGIN_STRING_ID_STRID_STOPSHUTTER */ 
	{ "Pas de fichier", 14 },			/* LV_PLUGIN_STRING_ID_STRID_NO_FILE */ 
	{ "Aucune Carte", 12 },			/* LV_PLUGIN_STRING_ID_STRID_NO_CARD */ 
	{ "Aucune image", 12 },			/* LV_PLUGIN_STRING_ID_STRID_NO_PHOTO */ 
	{ "Aucun fichier JPEG", 18 },			/* LV_PLUGIN_STRING_ID_STRID_NOT_JPEG */ 
	{ "Flash not ready", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FLASHNOTREADY */ 
	{ "Update Graph", 12 },			/* LV_PLUGIN_STRING_ID_STRID_UPDATEBG */ 
	{ "Check Version", 13 },			/* LV_PLUGIN_STRING_ID_STRID_CHECKVERSION */ 
	{ "PStore Error", 12 },			/* LV_PLUGIN_STRING_ID_STRID_PSTOREERR */ 
	{ "Jeu.", 4 },			/* LV_PLUGIN_STRING_ID_STRID_PLAY */ 
	{ "Palse", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PAUSE */ 
	{ "Arrêt", 6 },			/* LV_PLUGIN_STRING_ID_STRID_STOP */ 
	{ "Move", 4 },			/* LV_PLUGIN_STRING_ID_STRID_MOVE */ 
	{ "Éccnow", 7 },			/* LV_PLUGIN_STRING_ID_STRID_SAVE */ 
	{ "Aprés", 6 },			/* LV_PLUGIN_STRING_ID_STRID_NEXT */ 
	{ "Changement", 10 },			/* LV_PLUGIN_STRING_ID_STRID_CHANGE */ 
	{ "Volume", 6 },			/* LV_PLUGIN_STRING_ID_STRID_VOLUME */ 
	{ "fixe", 4 },			/* LV_PLUGIN_STRING_ID_STRID_STILL */ 
	{ "Battery Type", 12 },			/* LV_PLUGIN_STRING_ID_STRID_BATTERY_TYPE */ 
	{ "Alkaline", 8 },			/* LV_PLUGIN_STRING_ID_STRID_ALKALINE */ 
	{ "NiMH", 4 },			/* LV_PLUGIN_STRING_ID_STRID_NIMH */ 
	{ "Format memoire\nefface toutes\ndonnees", 38 },			/* LV_PLUGIN_STRING_ID_STRID_DELETE_WARNING */ 
	{ "Sequence No", 11 },			/* LV_PLUGIN_STRING_ID_STRID_SEQUENCE_NO */ 
	{ "Retour reglages\npar defaut", 27 },			/* LV_PLUGIN_STRING_ID_STRID_RESET_WARNING */ 
	{ "Effacer Ceci", 12 },			/* LV_PLUGIN_STRING_ID_STRID_ERASE_THIS */ 
	{ "Efface nefface Images?", 22 },			/* LV_PLUGIN_STRING_ID_STRID_ERASE_ALL */ 
	{ "Images", 6 },			/* LV_PLUGIN_STRING_ID_STRID_IMAGES */ 
	{ "Connecter appareil", 18 },			/* LV_PLUGIN_STRING_ID_STRID_CONNECT_TO_DEVICE */ 
	{ "Appareil connecte", 17 },			/* LV_PLUGIN_STRING_ID_STRID_DEVICE_IS_CONNECTED */ 
	{ "Erreur de lien", 14 },			/* LV_PLUGIN_STRING_ID_STRID_LINK_ERROR */ 
	{ "Erreur USB", 10 },			/* LV_PLUGIN_STRING_ID_STRID_USB_ERROR */ 
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
	{ "TOUTES LES PHOTOS", 17 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_ALL */ 
	{ "Tout Index", 10 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_INDEX */ 
	{ "Imprimer avec DPOF", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_DPOF */ 
	{ "Réglage de Imprimez", 20 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_SETTING */ 
	{ "DPOF Restart", 12 },			/* LV_PLUGIN_STRING_ID_STRID_DPOF_RESTART */ 
	{ "Imprimer", 8 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT */ 
	{ "Commencez Á Imprimer", 21 },			/* LV_PLUGIN_STRING_ID_STRID_START_PRINTING */ 
	{ "Impression..", 12 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTING */ 
	{ "Print Canceled", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_CANCELED */ 
	{ "Imprime Termine", 15 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_FINISHED */ 
	{ "Appuyer OK pour imprimer", 24 },			/* LV_PLUGIN_STRING_ID_STRID_OK_TO_PRINT */ 
	{ "Format Papier", 13 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_SIZE */ 
	{ "L", 1 },			/* LV_PLUGIN_STRING_ID_STRID_L */ 
	{ "2L", 2 },			/* LV_PLUGIN_STRING_ID_STRID_2L */ 
	{ "Carte postale", 13 },			/* LV_PLUGIN_STRING_ID_STRID_POSTCARD */ 
	{ "Carte", 5 },			/* LV_PLUGIN_STRING_ID_STRID_CARD */ 
	{ "100x150", 7 },			/* LV_PLUGIN_STRING_ID_STRID_100X150 */ 
	{ "4\"\"x6\"\"", 11 },			/* LV_PLUGIN_STRING_ID_STRID_4X6 */ 
	{ "8\"\"x10\"\"", 12 },			/* LV_PLUGIN_STRING_ID_STRID_8X10 */ 
	{ "Lettre ", 7 },			/* LV_PLUGIN_STRING_ID_STRID_LETTER */ 
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
	{ "Rouleau L", 9 },			/* LV_PLUGIN_STRING_ID_STRID_L_ROLLS */ 
	{ "Rouleau 2L", 10 },			/* LV_PLUGIN_STRING_ID_STRID_2L_ROLLS */ 
	{ "Roul.10x15", 10 },			/* LV_PLUGIN_STRING_ID_STRID_4_ROLLS */ 
	{ "Rouleau A4", 10 },			/* LV_PLUGIN_STRING_ID_STRID_A4_ROLLS */ 
	{ "Type De Papier", 14 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_TYPE */ 
	{ "Photo", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PHOTO_PAPER */ 
	{ "Plain", 5 },			/* LV_PLUGIN_STRING_ID_STRID_PLAIN_PAPER */ 
	{ "Fast Photo", 10 },			/* LV_PLUGIN_STRING_ID_STRID_FAST_PHOTO */ 
	{ "Type De Dossier", 15 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_TYPE */ 
	{ "Exif/JPEG", 9 },			/* LV_PLUGIN_STRING_ID_STRID_EXIF_JPEG */ 
	{ "Exif", 4 },			/* LV_PLUGIN_STRING_ID_STRID_EXIF */ 
	{ "JPEG", 4 },			/* LV_PLUGIN_STRING_ID_STRID_JPEG */ 
	{ "Impression De La Date", 21 },			/* LV_PLUGIN_STRING_ID_STRID_DATE_PRINT */ 
	{ "Impression De La Nom", 20 },			/* LV_PLUGIN_STRING_ID_STRID_FILENAME_PRINT */ 
	{ "L'Image Optimisent", 18 },			/* LV_PLUGIN_STRING_ID_STRID_IMAGE_OPTIMIZE */ 
	{ "N-UP Layout", 11 },			/* LV_PLUGIN_STRING_ID_STRID_LAYOUT */ 
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
	{ "Index", 5 },			/* LV_PLUGIN_STRING_ID_STRID_INDEX */ 
	{ "Borderless", 10 },			/* LV_PLUGIN_STRING_ID_STRID_1UP_BORDERLESS */ 
	{ "Taille Fixe", 11 },			/* LV_PLUGIN_STRING_ID_STRID_FIXED_SIZE */ 
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
	{ "Emblavage", 9 },			/* LV_PLUGIN_STRING_ID_STRID_CROPPING */ 
	{ "Erreur impression", 17 },			/* LV_PLUGIN_STRING_ID_STRID_PRINT_ERROR */ 
	{ "Printer can be Disconnected.", 28 },			/* LV_PLUGIN_STRING_ID_STRID_PRINTER_DISCONNECTABLE */ 
	{ "Probleme De Papier", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_ERROR */ 
	{ "Impression Annulee", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_OUT */ 
	{ "Il n'y a plus de papier", 23 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_LOAD_ERROR */ 
	{ "Probleme De Papier", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_EJECT_ERROR */ 
	{ "Probleme De Papier", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_MEDIA_ERROR */ 
	{ "Erreur de support", 17 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_JAMMED */ 
	{ "Bourrage du papier", 18 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_NEAR_EMPTY */ 
	{ "Il n'y a presque plus de papier", 31 },			/* LV_PLUGIN_STRING_ID_STRID_PAPER_NOT_SUPPORT */ 
	{ "Probleme D'encre", 16 },			/* LV_PLUGIN_STRING_ID_STRID_INK_ERROR */ 
	{ "Ce type de papier ne convient pas", 33 },			/* LV_PLUGIN_STRING_ID_STRID_INK_EMPTY */ 
	{ "Il n'y a plus d'encre", 21 },			/* LV_PLUGIN_STRING_ID_STRID_INK_LOW */ 
	{ "Probleme D'encre", 16 },			/* LV_PLUGIN_STRING_ID_STRID_INK_WASTE */ 
	{ "Erreur matérielle d'imprimante", 31 },			/* LV_PLUGIN_STRING_ID_STRID_HW_ERROR */ 
	{ "Erreur fatale", 13 },			/* LV_PLUGIN_STRING_ID_STRID_HW_FATAL */ 
	{ "Maintenance requise", 19 },			/* LV_PLUGIN_STRING_ID_STRID_HW_SERVICE_CALL */ 
	{ "Imprimante indisponible", 23 },			/* LV_PLUGIN_STRING_ID_STRID_HW_UNAVAILABLE */ 
	{ "Imprimante occupée", 19 },			/* LV_PLUGIN_STRING_ID_STRID_HW_BUSY */ 
	{ "Erreur de levier", 16 },			/* LV_PLUGIN_STRING_ID_STRID_HW_LEVER */ 
	{ "Position Défectueuse", 21 },			/* LV_PLUGIN_STRING_ID_STRID_HW_COVER_OPEN */ 
	{ "Aucun agent de marquage", 23 },			/* LV_PLUGIN_STRING_ID_STRID_HW_NO_MARKING_AGENT */ 
	{ "de marquage", 11 },			/* LV_PLUGIN_STRING_ID_STRID_HW_INK_COVER_OPEN */ 
	{ "Couvercle d'encre ouvert", 24 },			/* LV_PLUGIN_STRING_ID_STRID_HW_NO_INK_CARTRIDGE */ 
	{ "Erreur De Fichier", 17 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_ERROR */ 
	{ "Info Impr", 9 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_PRINT_INFO */ 
	{ "Erreur de décodage du fichier", 30 },			/* LV_PLUGIN_STRING_ID_STRID_FILE_DECODE_ERROR */ 
	{ "Erreur impression", 17 },			/* LV_PLUGIN_STRING_ID_STRID_UNKNOW_ERROR */ 
	{ "Erreur impression", 17 },			/* LV_PLUGIN_STRING_ID_STRID_XML_SYNTAX_ERROR */ 
	{ "Détection de mouvement", 23 },			/* LV_PLUGIN_STRING_ID_STRID_MOTION_DET */ 
	{ "Détection de visage", 20 },			/* LV_PLUGIN_STRING_ID_STRID_FACE_DET */ 
	{ "Détection de Smiley ", 21 },			/* LV_PLUGIN_STRING_ID_STRID_SMILE_DET */ 
	{ "Stabilisation", 13 },			/* LV_PLUGIN_STRING_ID_STRID_ANTI_SHAKING */ 
	{ "Aucun Film", 10 },			/* LV_PLUGIN_STRING_ID_STRID_NO_MOVIE */ 
	{ "MP3", 3 },			/* LV_PLUGIN_STRING_ID_STRID_MP3PLAY */ 
	{ "Aucun fichier MP3", 17 },			/* LV_PLUGIN_STRING_ID_STRID_NO_MP3FILE */ 
	{ "Rotation de l'image ", 20 },			/* LV_PLUGIN_STRING_ID_STRID_SENSOR_ROTATE */ 
	{ "Disque de vidéo", 16 },			/* LV_PLUGIN_STRING_ID_STRID_RECORD */ 
	{ "Acoustique record", 17 },			/* LV_PLUGIN_STRING_ID_STRID_RECORD_AUDIO */ 
	{ "Arrangement de lampe", 20 },			/* LV_PLUGIN_STRING_ID_STRID_LED_SETTING */ 
	{ "AU REVOIR", 9 },			/* LV_PLUGIN_STRING_ID_STRID_GOOD_BYE */ 
	{ "S'IL VOUS PLAÎT INSERT SD CARD", 31 },			/* LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD */ 
	{ "IR LED", 6 },			/* LV_PLUGIN_STRING_ID_STRID_IR_LED */ 
	{ "microphone", 10 },			/* LV_PLUGIN_STRING_ID_STRID_MICROPHONE */ 
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



