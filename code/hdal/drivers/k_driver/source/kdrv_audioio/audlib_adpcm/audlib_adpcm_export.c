#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(audlib_adpcm_encode_mono);
EXPORT_SYMBOL(audlib_adpcm_encode_stereo);
EXPORT_SYMBOL(audlib_adpcm_decode_mono);
EXPORT_SYMBOL(audlib_adpcm_decode_stereo);
EXPORT_SYMBOL(audlib_adpcm_encode_packet_mono);
EXPORT_SYMBOL(audlib_adpcm_encode_packet_stereo);
EXPORT_SYMBOL(audlib_adpcm_decode_packet_mono);
EXPORT_SYMBOL(audlib_adpcm_decode_packet_stereo);


#endif
