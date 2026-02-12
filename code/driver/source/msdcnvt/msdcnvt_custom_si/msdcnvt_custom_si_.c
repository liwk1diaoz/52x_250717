#include "msdcnvt_int.h"
#include "msdcnvt_custom_si.h"

//Step 1: Declare custom functions
//Declare Gets Functions
static void get_say_hi(void);
static void get_say_hello(void);
//Declare Sets Functions
static void set_say_hi(void);
static void set_say_hello(void);

//Step 2: Create your function mapping table for 'Get' Command
static void (*custom_get_obj[])(void) = {
	get_say_hi,                   // 0, #define ID_GET_SAY_HI    0 //in AP Side
	get_say_hello,                // 1, #define ID_GET_SAY_HELLO 1 //in AP Side
};

//Step 3: Create your function mapping table for 'Set' Command
static void (*custom_set_obj[])(void) = {
	set_say_hi,                   // 0, #define ID_SET_SAY_HI    0 //in AP Side
	set_say_hello,                // 1, #define ID_SET_SAY_HELLO 1 //in AP Side
};

//Step 4: Provide API for Register Single Direction Functions
BOOL msdcnvtregsi_custom_si(void)
{
	return msdcnvt_add_callback_si(custom_get_obj, sizeof(custom_get_obj) / sizeof(custom_get_obj[0]), custom_set_obj, sizeof(custom_set_obj) / sizeof(custom_set_obj[0]));
}

//Step 5: Start to implement your custom function
static void get_say_hi(void)
{
	UINT32  *p_data    = (UINT32 *)msdcnvt_get_data_addr();
	UINT32   length = msdcnvt_get_trans_size();

	//1.  p_data could be a structure, just the PC side and Firmware side use the
	//    same structure define. such as
	//    STRUCT_DATA *p_data =  (STRUCT_DATA*)msdcnvt_get_data_addr();
	//    then you can check size as if(length!=sizeof(STRUCT_DATA))
	//2.  p_data could be a string. if the string is not fixed length, you have
	//    to remove check size if condition for invalid data. and the string
	//    length have to smaller than length-1

	//length have to equal to PC data size
	//if PC only send 4 bytes data, here length have to be 4
	if (length != sizeof(UINT32)) {
		return;
	}

	DBG_DUMP("@@@@@@ Get Say Hi @@@@@@\r\n");
	*p_data = 12345;
}

static void get_say_hello(void)
{
	UINT32  *p_data    = (UINT32 *)msdcnvt_get_data_addr();
	UINT32   length = msdcnvt_get_trans_size();

	DBG_DUMP("length=%d\r\n", length);

	//1.  p_data could be a structure, just the PC side and Firmware side use the
	//    same structure define. such as
	//    STRUCT_DATA *p_data =  (STRUCT_DATA*)msdcnvt_get_data_addr();
	//    then you can check size as if(length!=sizeof(STRUCT_DATA))
	//2.  p_data could be a string. if the string is not fixed length, you have
	//    to remove check size if condition for invalid data. and the string
	//    length have to < length

	//length have to equal to PC data size
	//if PC only send 4 bytes data, here length have to be 4
	if (length != sizeof(UINT32)) {
		return;
	}

	DBG_DUMP("@@@@@@ Get Say Hello @@@@@@\r\n");
	*p_data = 67890;
}

static void set_say_hi(void)
{
	UINT32  i;
	UINT32 *p_data    = (UINT32 *)msdcnvt_get_data_addr();
	UINT32  length = msdcnvt_get_trans_size();

	DBG_DUMP("###### Set Say Hi ###### \r\n");

	//1.  p_data could be a structure, just the PC side and Firmware side use the
	//    same structure define. such as
	//    STRUCT_DATA *p_data =  (STRUCT_DATA*)msdcnvt_get_data_addr();
	//    then you can check size as if(length!=sizeof(STRUCT_DATA))
	//2.  p_data could be a string.
	if (length) {
		for (i = 0; i < length; i += 4) {
			DBG_ERR("Data[%d]=%d\r\n", i, *p_data);
			p_data++;
		}
	}
}

static void set_say_hello(void)
{
	UINT32  i;
	UINT32 *p_data    = (UINT32 *)msdcnvt_get_data_addr();
	UINT32  length = msdcnvt_get_trans_size();

	DBG_DUMP("###### Set Say Hello ###### \r\n");

	//1.  p_data could be a structure, just the PC side and Firmware side use the
	//    same structure define. such as
	//    STRUCT_DATA *p_data =  (STRUCT_DATA*)msdcnvt_get_data_addr();
	//    then you can check size as if(length!=sizeof(STRUCT_DATA))
	//2.  p_data could be a string.
	if (length) {
		for (i = 0; i < length; i += 4) {
			DBG_ERR("Data[%d]=%d\r\n", i, *p_data);
			p_data++;
		}
	}
}