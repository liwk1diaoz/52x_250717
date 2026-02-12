#!/bin/sh
sleep=1
rm -rf /mnt/sd/*_dec/;mkdir -p /mnt/sd/aac_dec/mono /mnt/sd/alaw_dec/mono /mnt/sd/ulaw_dec/mono /mnt/sd/pcm_dec/mono /mnt/sd/aac_dec/stereo /mnt/sd/alaw_dec/stereo /mnt/sd/ulaw_dec/stereo /mnt/sd/pcm_dec/stereo


cd /mnt/sd/
#if [ "1" -eq "0" ];then
#decode g711a mono
./hd_audio_decode_test 0 1 0 /mnt/sd/aac/audio_8000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_8000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 1 /mnt/sd/aac/audio_11025_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_11025_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 2 /mnt/sd/aac/audio_12000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_12000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 3 /mnt/sd/aac/audio_16000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_16000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 4 /mnt/sd/aac/audio_22050_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_22050_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 5 /mnt/sd/aac/audio_24000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_24000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 6 /mnt/sd/aac/audio_32000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_32000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 7 /mnt/sd/aac/audio_44100_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_44100_16_1.pcm
sleep $sleep
./hd_audio_decode_test 0 1 8 /mnt/sd/aac/audio_48000_16_1.aac /mnt/sd/aac_dec/mono/dec_audio_48000_16_1.pcm
sleep $sleep


#decode g711u mono
./hd_audio_decode_test 1 1 0 /mnt/sd/ulaw/audio_8000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_8000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 1 /mnt/sd/ulaw/audio_11025_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_11025_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 2 /mnt/sd/ulaw/audio_12000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_12000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 3 /mnt/sd/ulaw/audio_16000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_16000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 4 /mnt/sd/ulaw/audio_22050_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_22050_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 5 /mnt/sd/ulaw/audio_24000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_24000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 6 /mnt/sd/ulaw/audio_32000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_32000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 7 /mnt/sd/ulaw/audio_44100_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_44100_16_1.pcm
sleep $sleep
./hd_audio_decode_test 1 1 8 /mnt/sd/ulaw/audio_48000_16_1.g711u /mnt/sd/ulaw_dec/mono/dec_audio_48000_16_1.pcm
sleep $sleep

#decode g711a mono
./hd_audio_decode_test 2 1 0 /mnt/sd/alaw/audio_8000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_8000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 1 /mnt/sd/alaw/audio_11025_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_11025_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 2 /mnt/sd/alaw/audio_12000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_12000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 3 /mnt/sd/alaw/audio_16000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_16000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 4 /mnt/sd/alaw/audio_22050_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_22050_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 5 /mnt/sd/alaw/audio_24000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_24000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 6 /mnt/sd/alaw/audio_32000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_32000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 7 /mnt/sd/alaw/audio_44100_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_44100_16_1.pcm
sleep $sleep
./hd_audio_decode_test 2 1 8 /mnt/sd/alaw/audio_48000_16_1.g711a /mnt/sd/alaw_dec/mono/dec_audio_48000_16_1.pcm
sleep $sleep

#decode pcm mono
./hd_audio_decode_test 3 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_8000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_11025_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_12000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_16000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_22050_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_24000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_32000_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_44100_16_1.pcm
sleep $sleep
./hd_audio_decode_test 3 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm /mnt/sd/pcm_dec/mono/dec_audio_48000_16_1.pcm
sleep $sleep
#fi

###########stereo
sleep=1
#decode g711a stereo
./hd_audio_decode_test 0 2 0 /mnt/sd/aac/audio_8000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_8000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 1 /mnt/sd/aac/audio_11025_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_11025_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 2 /mnt/sd/aac/audio_12000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_12000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 3 /mnt/sd/aac/audio_16000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_16000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 4 /mnt/sd/aac/audio_22050_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_22050_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 5 /mnt/sd/aac/audio_24000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_24000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 6 /mnt/sd/aac/audio_32000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_32000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 7 /mnt/sd/aac/audio_44100_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_44100_16_2.pcm
sleep $sleep
./hd_audio_decode_test 0 2 8 /mnt/sd/aac/audio_48000_16_2.aac /mnt/sd/aac_dec/stereo/dec_audio_48000_16_2.pcm
sleep $sleep


#decode g711u stereo
./hd_audio_decode_test 1 2 0 /mnt/sd/ulaw/audio_8000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_8000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 1 /mnt/sd/ulaw/audio_11025_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_11025_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 2 /mnt/sd/ulaw/audio_12000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_12000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 3 /mnt/sd/ulaw/audio_16000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_16000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 4 /mnt/sd/ulaw/audio_22050_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_22050_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 5 /mnt/sd/ulaw/audio_24000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_24000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 6 /mnt/sd/ulaw/audio_32000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_32000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 7 /mnt/sd/ulaw/audio_44100_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_44100_16_2.pcm
sleep $sleep
./hd_audio_decode_test 1 2 8 /mnt/sd/ulaw/audio_48000_16_2.g711u /mnt/sd/ulaw_dec/stereo/dec_audio_48000_16_2.pcm
sleep $sleep

#decode g711a stereo
./hd_audio_decode_test 2 2 0 /mnt/sd/alaw/audio_8000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_8000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 1 /mnt/sd/alaw/audio_11025_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_11025_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 2 /mnt/sd/alaw/audio_12000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_12000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 3 /mnt/sd/alaw/audio_16000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_16000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 4 /mnt/sd/alaw/audio_22050_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_22050_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 5 /mnt/sd/alaw/audio_24000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_24000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 6 /mnt/sd/alaw/audio_32000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_32000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 7 /mnt/sd/alaw/audio_44100_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_44100_16_2.pcm
sleep $sleep
./hd_audio_decode_test 2 2 8 /mnt/sd/alaw/audio_48000_16_2.g711a /mnt/sd/alaw_dec/stereo/dec_audio_48000_16_2.pcm
sleep $sleep

#decode pcm stereo
./hd_audio_decode_test 3 2 0 /mnt/sd/pcm/audio_8000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_8000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 1 /mnt/sd/pcm/audio_11025_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_11025_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 2 /mnt/sd/pcm/audio_12000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_12000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 3 /mnt/sd/pcm/audio_16000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_16000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 4 /mnt/sd/pcm/audio_22050_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_22050_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 5 /mnt/sd/pcm/audio_24000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_24000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 6 /mnt/sd/pcm/audio_32000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_32000_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 7 /mnt/sd/pcm/audio_44100_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_44100_16_2.pcm
sleep $sleep
./hd_audio_decode_test 3 2 8 /mnt/sd/pcm/audio_48000_16_2.pcm /mnt/sd/pcm_dec/stereo/dec_audio_48000_16_2.pcm
sleep $sleep

