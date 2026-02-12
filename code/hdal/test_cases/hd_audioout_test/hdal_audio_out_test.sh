#!/bin/sh

# ./hd_audio_out_test <drv_out:1~4> <drv_mono:0~1> <vol:0~160> <usr_mode:1~2> <usr_sampling_rate:0~8>  <file_path> 

cd /mnt/sd/
if [ "1" -eq "0" ];then
#SPK out test <drv_mono> is don't care
##mono
echo "=====>SPK: mono 8k <====="
./hd_audioout_test 1 1  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>SPK: mono 11.025k <====="
./hd_audioout_test 1 1  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>SPK: mono 12k <====="
./hd_audioout_test 1 1  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>SPK: mono 16k <====="
./hd_audioout_test 1 1  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>SPK: mono 22.05k <====="
./hd_audioout_test 1 1  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>SPK: mono 24k <====="
./hd_audioout_test 1 1 120 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>SPK: mono 32k <====="
./hd_audioout_test 1 1 100 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>SPK: mono 44.1k <====="
./hd_audioout_test 1 1  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>SPK: mono 48k <====="
./hd_audioout_test 1 1  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm
##stereo
echo "=====>SPK: stereo 8k <====="
./hd_audioout_test 1 1  80 2 0 /mnt/sd/pcm/audio_8000_16_2.pcm
echo "=====>SPK: stereo 11.025k <====="
./hd_audioout_test 1 1  60 2 1 /mnt/sd/pcm/audio_11025_16_2.pcm
echo "=====>SPK: stereo 12k <====="
./hd_audioout_test 1 1  50 2 2 /mnt/sd/pcm/audio_12000_16_2.pcm
echo "=====>SPK: stereo 16k <====="
./hd_audioout_test 1 1  80 2 3 /mnt/sd/pcm/audio_16000_16_2.pcm
echo "=====>SPK: stereo 22.05k <====="
./hd_audioout_test 1 1  80 2 4 /mnt/sd/pcm/audio_22050_16_2.pcm
echo "=====>SPK: stereo 24k <====="
./hd_audioout_test 1 1 120 2 5 /mnt/sd/pcm/audio_24000_16_2.pcm
echo "=====>SPK: stereo 32k <====="
./hd_audioout_test 1 1 100 2 6 /mnt/sd/pcm/audio_32000_16_2.pcm
echo "=====>SPK: stereo 44.1k <====="
./hd_audioout_test 1 1  80 2 7 /mnt/sd/pcm/audio_44100_16_2.pcm
echo "=====>SPK: stereo 48k <====="
./hd_audioout_test 1 1  80 2 8 /mnt/sd/pcm/audio_48000_16_2.pcm

#LINE out
##mono, <drv_mono> decided which pin(LEFT/RIGHT) is the output
###LEFT
echo "=====>LINE: mono 8k LEFT <====="
./hd_audioout_test 2 0  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>LINE: mono 11.025k LEFT <====="
./hd_audioout_test 2 0  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>LINE: mono 12k LEFT <====="
./hd_audioout_test 2 0  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>LINE: mono 16k LEFT <====="
./hd_audioout_test 2 0  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>LINE: mono 22.05k LEFT <====="
./hd_audioout_test 2 0  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>LINE: mono 24k LEFT <====="
./hd_audioout_test 2 0 120 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>LINE: mono 32k LEFT <====="
./hd_audioout_test 2 0 120 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>LINE: mono 44.1k LEFT <====="
./hd_audioout_test 2 0  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>LINE: mono 48k LEFT <====="
./hd_audioout_test 2 0  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm
###RIGHT
echo "=====>LINE: mono 8k RIGHT <====="
./hd_audioout_test 2 1  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>LINE: mono 11.025k RIGHT <====="
./hd_audioout_test 2 1  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>LINE: mono 12k RIGHT <====="
./hd_audioout_test 2 1  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>LINE: mono 16k RIGHT <====="
./hd_audioout_test 2 1  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>LINE: mono 22.05k RIGHT <====="
./hd_audioout_test 2 1  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>LINE: mono 24k RIGHT <====="
./hd_audioout_test 2 1 120 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>LINE: mono 32k RIGHT <====="
./hd_audioout_test 2 1 120 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>LINE: mono 44.1k RIGHT <====="
./hd_audioout_test 2 1  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>LINE: mono 48k RIGHT <====="
./hd_audioout_test 2 1  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm

##stereo, <drv_mono> is don't care, all the pin would be the output
echo "=====>LINE: stereo 8k <====="
./hd_audioout_test 2 0  80 2 0 /mnt/sd/pcm/audio_8000_16_2.pcm
echo "=====>LINE: stereo 11.025k <====="
./hd_audioout_test 2 0  60 2 1 /mnt/sd/pcm/audio_11025_16_2.pcm
echo "=====>LINE: stereo 12k <====="
./hd_audioout_test 2 0  50 2 2 /mnt/sd/pcm/audio_12000_16_2.pcm
echo "=====>LINE: stereo 16k <====="
./hd_audioout_test 2 0  100 2 3 /mnt/sd/pcm/audio_16000_16_2.pcm
echo "=====>LINE: stereo 22.05k <====="
./hd_audioout_test 2 0  120 2 4 /mnt/sd/pcm/audio_22050_16_2.pcm
echo "=====>LINE: stereo 24k <====="
./hd_audioout_test 2 0 100 2 5 /mnt/sd/pcm/audio_24000_16_2.pcm
echo "=====>LINE: stereo 32k <====="
./hd_audioout_test 2 0 100 2 6 /mnt/sd/pcm/audio_32000_16_2.pcm
echo "=====>LINE: stereo 44.1k <====="
./hd_audioout_test 2 0  120 2 7 /mnt/sd/pcm/audio_44100_16_2.pcm
echo "=====>LINE: stereo 48k <====="
./hd_audioout_test 2 0  120 2 8 /mnt/sd/pcm/audio_48000_16_2.pcm

#ALL out(SPK + LINE)
##mono
###LINE-LEFT
echo "=====>ALL: mono 8k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>ALL: mono 11.025k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>ALL: mono 12k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>ALL: mono 16k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>ALL: mono 22.05k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>ALL: mono 24k LINE-LEFT <====="
./hd_audioout_test 3 0 120 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>ALL: mono 32k LINE-LEFT <====="
./hd_audioout_test 3 0 100 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>ALL: mono 44.1k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>ALL: mono 48k LINE-LEFT <====="
./hd_audioout_test 3 0  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm
###LINE-RIGHT
echo "=====>ALL: mono 8k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>ALL: mono 11.025k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>ALL: mono 12k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>ALL: mono 16k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>ALL: mono 22.05k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>ALL: mono 24k LINE-RIGHT <====="
./hd_audioout_test 3 1 120 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>ALL: mono 32k LINE-RIGHT <====="
./hd_audioout_test 3 1 100 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>ALL: mono 44.1k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>ALL: mono 48k LINE-RIGHT <====="
./hd_audioout_test 3 1  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm
##stereo, <drv_mono> is don't care, all the pin would be the output
echo "=====>ALL: stereo 8k <====="
./hd_audioout_test 3 0  80 2 0 /mnt/sd/pcm/audio_8000_16_2.pcm
echo "=====>ALL: stereo 11.025k <====="
./hd_audioout_test 3 0  60 2 1 /mnt/sd/pcm/audio_11025_16_2.pcm
echo "=====>ALL: stereo 12k <====="
./hd_audioout_test 3 0  50 2 2 /mnt/sd/pcm/audio_12000_16_2.pcm
echo "=====>ALL: stereo 16k <====="
./hd_audioout_test 3 0  80 2 3 /mnt/sd/pcm/audio_16000_16_2.pcm
echo "=====>ALL: stereo 22.05k <====="
./hd_audioout_test 3 0  80 2 4 /mnt/sd/pcm/audio_22050_16_2.pcm
echo "=====>ALL: stereo 24k <====="
./hd_audioout_test 3 0 100 2 5 /mnt/sd/pcm/audio_24000_16_2.pcm
echo "=====>ALL: stereo 32k <====="
./hd_audioout_test 3 0 100 2 6 /mnt/sd/pcm/audio_32000_16_2.pcm
echo "=====>ALL: stereo 44.1k <====="
./hd_audioout_test 3 0  60 2 7 /mnt/sd/pcm/audio_44100_16_2.pcm
echo "=====>ALL: stereo 48k <====="
./hd_audioout_test 3 0  60 2 8 /mnt/sd/pcm/audio_48000_16_2.pcm
fi

#I2S out, only nvt_aud_emu support audio out, <vol:0~100> 
##mono, <drv_mono> decided which pin(LEFT/RIGHT) is the output
###LEFT
echo "=====>I2S: mono 8k LEFT <====="
./hd_audioout_test 4 0  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>I2S: mono 11.025k LEFT <====="
./hd_audioout_test 4 0  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>I2S: mono 12k LEFT <====="
./hd_audioout_test 4 0  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>I2S: mono 16k LEFT <====="
./hd_audioout_test 4 0  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>I2S: mono 22.05k LEFT <====="
./hd_audioout_test 4 0  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>I2S: mono 24k LEFT <====="
./hd_audioout_test 4 0 100 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>I2S: mono 32k LEFT <====="
./hd_audioout_test 4 0 100 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>I2S: mono 44.1k LEFT <====="
./hd_audioout_test 4 0  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>I2S: mono 48k LEFT <====="
./hd_audioout_test 4 0  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm
###RIGHT
echo "=====>I2S: mono 8k RIGHT <====="
./hd_audioout_test 4 1  80 1 0 /mnt/sd/pcm/audio_8000_16_1.pcm
echo "=====>I2S: mono 11.025k RIGHT <====="
./hd_audioout_test 4 1  80 1 1 /mnt/sd/pcm/audio_11025_16_1.pcm
echo "=====>I2S: mono 12k RIGHT <====="
./hd_audioout_test 4 1  80 1 2 /mnt/sd/pcm/audio_12000_16_1.pcm
echo "=====>I2S: mono 16k RIGHT <====="
./hd_audioout_test 4 1  80 1 3 /mnt/sd/pcm/audio_16000_16_1.pcm
echo "=====>I2S: mono 22.05k RIGHT <====="
./hd_audioout_test 4 1  80 1 4 /mnt/sd/pcm/audio_22050_16_1.pcm
echo "=====>I2S: mono 24k RIGHT <====="
./hd_audioout_test 4 1 100 1 5 /mnt/sd/pcm/audio_24000_16_1.pcm
echo "=====>I2S: mono 32k RIGHT <====="
./hd_audioout_test 4 1 100 1 6 /mnt/sd/pcm/audio_32000_16_1.pcm
echo "=====>I2S: mono 44.1k RIGHT <====="
./hd_audioout_test 4 1  80 1 7 /mnt/sd/pcm/audio_44100_16_1.pcm
echo "=====>I2S: mono 48k RIGHT <====="
./hd_audioout_test 4 1  80 1 8 /mnt/sd/pcm/audio_48000_16_1.pcm

##stereo, <drv_mono> is don't care, all the pin would be the output
echo "=====>I2S: stereo 8k <====="
./hd_audioout_test 4 0  80 2 0 /mnt/sd/pcm/audio_8000_16_2.pcm
echo "=====>I2S: stereo 11.025k <====="
./hd_audioout_test 4 0  60 2 1 /mnt/sd/pcm/audio_11025_16_2.pcm
echo "=====>I2S: stereo 12k <====="
./hd_audioout_test 4 0  50 2 2 /mnt/sd/pcm/audio_12000_16_2.pcm
echo "=====>I2S: stereo 16k <====="
./hd_audioout_test 4 0  100 2 3 /mnt/sd/pcm/audio_16000_16_2.pcm
echo "=====>I2S: stereo 22.05k <====="
./hd_audioout_test 4 0  100 2 4 /mnt/sd/pcm/audio_22050_16_2.pcm
echo "=====>I2S: stereo 24k <====="
./hd_audioout_test 4 0 100 2 5 /mnt/sd/pcm/audio_24000_16_2.pcm
echo "=====>I2S: stereo 32k <====="
./hd_audioout_test 4 0 100 2 6 /mnt/sd/pcm/audio_32000_16_2.pcm
echo "=====>I2S: stereo 44.1k <====="
./hd_audioout_test 4 0  100 2 7 /mnt/sd/pcm/audio_44100_16_2.pcm
echo "=====>I2S: stereo 48k <====="
./hd_audioout_test 4 0  100 2 8 /mnt/sd/pcm/audio_48000_16_2.pcm
