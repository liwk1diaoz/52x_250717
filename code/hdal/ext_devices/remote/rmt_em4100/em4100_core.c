
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "em4100.h"
#include "em4100_core.h"
#include "rmt_dbg.h"


// TODO: glitch problem
int em4100_data_reduction(char *origin_data, int len, char *manchester_data)
{
    int i, j, k;
    int one_cnt = 0, zero_cnt = 0;
    int out_buf_cnt = 0;
    char buf[65];

    DBG_FUNC( "%s was called\n", __func__ );

    // Read RFID tag data
    for (i = 0; i < len; i++) {
        k = 1 << 7;
        for (j = 0; j < 8;j++) {
            origin_data[i] & k ? one_cnt++ : zero_cnt++;
            //printk("l:%x\n", l);
            k = k >> 1;
			if (one_cnt && zero_cnt) {
				if (one_cnt > zero_cnt) { 	// bit is '1'
					if (one_cnt < 36) {
                        manchester_data[out_buf_cnt] = '1';
                        out_buf_cnt++;
					} else {				// if > 360 ms, bit '1' *2
                        manchester_data[out_buf_cnt] = '1';
                        out_buf_cnt++;
                        manchester_data[out_buf_cnt] = '1';
                        out_buf_cnt++;
					}
					one_cnt = 0;
				} else { 					// bit is '0'
					if (zero_cnt < 36) {
                        manchester_data[out_buf_cnt] = '0';
                        out_buf_cnt++;
					} else {				// if > 360 ms, bit '0' *2
                        manchester_data[out_buf_cnt] = '0';
                        out_buf_cnt++;
                        manchester_data[out_buf_cnt] = '0';
                        out_buf_cnt++;
					}
					zero_cnt = 0;
				}
			}
            if (out_buf_cnt >= (len / 3) -1 ) {
                DBG_IND("out_buf_cnt: %d is full\n", out_buf_cnt);
                return -1;
            }
        }
    }
    manchester_data[out_buf_cnt] = '\0';

    //DBG_IND("reduction data buf:\n%s\n", manchester_data);
    DBG_IND("reduction data buf:\n");
    DBG_IND("len: %d\n", out_buf_cnt);
    j = 0;
    while(out_buf_cnt) {
        if (out_buf_cnt < 64) {
            for(i = 0; i < out_buf_cnt ; i++){
                buf[i] = manchester_data[i+j];
            }
            out_buf_cnt = 0;
        } else {
            for(i = 0; i < 64 ; i++){
                buf[i] = manchester_data[i+j];
            }
            j += i;
            out_buf_cnt -= 64;
        }
        buf[i] = '\0';
        DBG_IND("%s\n", buf);
    }

    return 0;
}

int em4100_manchester_decoding(char *manchester_data, int mode,
                                                    char *decode_buf)
{
    int i, cnt = 0;
    int buf_cnt = 0;
    u8 shift_flg = 0, end_flg = 0;
    char buf[65];

    DBG_FUNC( "%s was called\n", __func__ );
    DBG_IND("decoding mode: %d\n", mode);
    for (i = 0; i < 3072; i+=2) {
        if( manchester_data[i] == '0') {
            if (manchester_data[i + 1] == '1') {
                if (mode == MODE_GE_THOMAS) {
                    decode_buf[cnt] = '0';
                    cnt++;
                } else if (mode == MODE_IEEE802) {
                    decode_buf[cnt] = '1';
                    cnt++;
                } else {
                    DBG_IND("Mode setting is error!\n");
                }
            } else if (manchester_data[i + 1] == '0') {
                DBG_IND("decoding: '00' is error!\n");
                if (shift_flg == 0) {
                    DBG_IND("Try shift 1 bit: \n");
                    shift_flg = 1;
                    i = 1;
                    cnt = 0;
                    continue;
                }
                break;
            } else if (manchester_data[i + 1] == '\0') {
                end_flg = 1;
                decode_buf[cnt] = '\0';
                break;
            } else {
                DBG_IND("Data error!\n");
                break;
            }
        } else if (manchester_data[i] == '1'){
            if(manchester_data[i + 1] == '0') {
                if(mode == MODE_GE_THOMAS) {
                    decode_buf[cnt] = '1';
                    cnt++;
                } else if (mode == MODE_IEEE802) {
                    decode_buf[cnt] = '0';
                    cnt++;
                } else {
                    DBG_IND("Mode setting is error!\n");
                }
            } else if (manchester_data[i + 1] == '1' && shift_flg == 0) {
                DBG_IND("decoding: '11' is error!\n");
                if (shift_flg == 0) {
                    DBG_IND("Try shift 1 bit: \n");
                    shift_flg = 1;
                    i = 1;
                    cnt = 0;
                    continue;
                }
                break;
            } else if (manchester_data[i + 1] == '\0') {
                end_flg = 1;
                decode_buf[cnt] = '\0';
                break;
            } else {
                DBG_IND("Data error!\n");
                break;
            }
        } else if (manchester_data[i] == '\0' || manchester_data[i + 1] == '\0'){
            end_flg = 1;
            decode_buf[cnt] = '\0';
            break;
        } else {
            DBG_IND("Data error!\n");
            break;
        }
    }
    if (end_flg) {
        //DBG_IND("Decoding buf:\n%s\n", decode_buf);
        DBG_IND("Decoding buf:\n");
        DBG_IND("len: %d\n", cnt);
        while(cnt) {
            if (cnt < 64) {
                for(i = 0; i < cnt ; i++){
                    buf[i] = decode_buf[i + buf_cnt];
                }
                cnt = 0;
            } else {
                for(i = 0; i < 64 ; i++){
                    buf[i] = decode_buf[i + buf_cnt];
                }
                buf_cnt += i;
                cnt -= 64;
            }
            buf[i] = '\0';
            DBG_IND("%s\n", buf);
        }
    }
    return 0;
}

int em4100_search_header(char *decode_buf, char *packet_buf, char *header)
{
    char *loc;
    int i ;

    DBG_FUNC( "%s was called\n", __func__ );

    if (header == NULL){
        header = "111111111";   // default header : '1' * 9
    }

    loc = strstr(decode_buf, header);
    if (loc == NULL) {
        DBG_IND("search fail.\n");
        return 0;
    } else {
        DBG_IND("header position: %u\n", loc - decode_buf);
    }

    for (i = 0; i < 55; i++) {
        packet_buf[i] = decode_buf[loc - decode_buf + i + 9];
    }
    packet_buf[55] = '\0';
    DBG_IND("Tag data : %s\n", packet_buf);

    return 0;
}

int em4100_getID_Data(char *packet_buf, int *id, int *data)
{
    int i, buf = 0;
    int parity = 0;         // even parity
    int PC[4] = {0,0,0,0};  // 4 column Parity
    int parity_column_cnt = 0;
    uint tag_id, tag_data;

    DBG_FUNC( "%s was called\n", __func__ );

    for (i = 0; i < 54;i++) {
        if (i >= 50) {      // column Parity chk
            if ( (packet_buf[i] == '1' && PC[parity_column_cnt] % 2 == 1) ||
                    (packet_buf[i] == '0' && PC[parity_column_cnt] % 2 == 0)) {
                DBG_IND("PC%d chk done.\n", parity_column_cnt);
            } else {
                DBG_IND("PC%d chk error!\n", parity_column_cnt);
                *id = 0;
                *data = 0;
                return 0;
            }
            parity_column_cnt++;
        } else {
            if (i % 5 < 4 && packet_buf[i] == '1') {
                PC[i % 5]++;
            }

            if (i % 5 < 4) {
                if (packet_buf[i] == '1') {
                    buf = buf << 1;
                    buf++;
                    parity++;
                } else {
                    buf = buf << 1;
                }
            } else if (i % 5 == 4){     // Even parity chk
                if ( (packet_buf[i] == '1' && parity % 2 == 1) ||
                        (packet_buf[i] == '0' && parity % 2 == 0)) {
                    DBG_IND("Parity %d chk done.\n", i / 5);
                } else {
                    DBG_IND("Parity %d chk error!\n", i / 5);
                    *id = 0;
                    *data = 0;
                    return 0;
                }
                parity = 0;
            }

            if (i == 9) {
                tag_id = buf;
                *id = tag_id;
                buf = 0;
            }
        }
    }
    tag_data = buf;
    *data = tag_data;

    DBG_IND("Get ID : 0x%x\n", *id);
    DBG_IND("Data   : 0x%x\n", *data);

    return 0;
}


