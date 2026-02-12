// conventions mode:
typedef enum {
    MODE_GE_THOMAS = 1,     // EM4095
    MODE_IEEE802            // EM4100
}manchester_decode_mode;


int em4100_data_reduction(char *origin_data, int len, char *manchester_data);
int em4100_manchester_decoding(char *manchester_data, int mode, char *decode_buf);
int em4100_search_header(char *decode_buf, char *packet_buf, char *header);
int em4100_getID_Data(char *packet_buf, int *id, int *data);

