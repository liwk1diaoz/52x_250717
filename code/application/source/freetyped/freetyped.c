#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nvtipc.h"
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <nvtcfg_define.h>
#include <nvtcodec.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define WIDTH   1000
#define HEIGHT  100

#define PID_FILE "/var/run/freetyped.pid"

#define MAX_CHANNEL 1

unsigned short                  dimage[HEIGHT * WIDTH];

/* origin is the upper left corner */
FT_Library                      library = NULL;
FT_Face                         face[MAX_CHANNEL] = { 0 };
FT_GlyphSlot                    slot;
FT_Matrix                       matrix;                 /* transformation matrix */
FT_Vector                       pen;                    /* untransformed origin  */
FT_Error                        error;

static int create_pid_file(void)
{
	int fd;
	pid_t   my_pid;

	unlink(PID_FILE);
	fd = open(PID_FILE, O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("freetyped can't create %s\n", PID_FILE);
		return -1;
	}

	my_pid = getpid();
	if (write(fd, &my_pid, sizeof(pid_t)) <= 0) {
		printf("freetyped can't write %s\n", PID_FILE);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static int init_ftype(char *font_file, int font_size, double angle)
{
	FT_Error error;
	int n;

	if (!font_file) {
		printf("freetyped : invalid ftype parameter\n");
		return -1;
	}

	error = FT_Init_FreeType(&library);                /* initialize library */
	/* error handling omitted */

	for (n = 0 ; n < MAX_CHANNEL ; ++n) {
		error = FT_New_Face(library, font_file, 0, &face[n]);   /* create face object */
		if (error) {
			printf("freetyped : FT_New_Face() fail with %d\r\n", error);
		}
	}
	/* error handling omitted */

	/* use 50pt at 100dpi */
	for (n = 0 ; n < MAX_CHANNEL ; ++n) {
		error = FT_Set_Char_Size(face[n], font_size * 64, 0, 100, 0);                  /* set character size */
		/* error handling omitted */
		if (error) {
			printf("freetyped : FT_Set_Char_Size() fail with %d\r\n", error);
		}
	}

	/* set up matrix */
	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

	return (int)error;
}

/* Replace this function with something useful. */
static void draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y, unsigned short *buf)
{
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;

	for (i = x, p = 0; i < x_max; i++, p++) {
		for (j = y, q = 0; j < y_max; j++, q++) {
			if (i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT) {
				continue;
			}

			if (bitmap->buffer[q * bitmap->width + p])
				buf[j * WIDTH + i] = (((unsigned short)240 << 8) | (unsigned short)15);
			else
				buf[j * WIDTH + i] = 0x00;
		}
	}
}

static int config_buffer(void)
{
	if (NvtCodec_AllocPingPongBuf(VDS_PHASE_PS, 0, WIDTH, HEIGHT)) {
		printf("fail to allocate buffer for PS image 0\r\n");
		return -1;
	}
        if (NvtCodec_AllocPingPongBuf(VDS_PHASE_BTN, 0, WIDTH, HEIGHT)) {
                printf("fail to allocate buffer for BTN image 0\r\n");
                return -1;
        }
        if (NvtCodec_AllocPingPongBuf(VDS_PHASE_EXT, 0, WIDTH, HEIGHT)) {
                printf("fail to allocate buffer for EXT image 0\r\n");
                return -1;
        }

	return 0;
}

static void config_btn_osd(CodecOSDIMAGE *image, unsigned int id, unsigned int layer, unsigned int region, unsigned int vid, unsigned int enable, VDS_FMT fmt, unsigned int x, unsigned int y, unsigned int buf)
{
	memset(image, 0, sizeof(CodecOSDIMAGE));
	image->id                 = id;
	image->phase             = VDS_PHASE_BTN;
	image->data.btn.layer     = layer;
	image->data.btn.region    = region;
	image->data.btn.vid       = vid;
	image->data.btn.en        = enable;
	image->data.btn.fmt       = fmt;
	image->data.btn.x         = x;
	image->data.btn.y         = y;
	image->data.btn.w         = WIDTH;
	image->data.btn.h         = HEIGHT;
        image->data.btn.alpha     = 255;
	image->data.btn.addr      = buf;
}

static void config_ext_osd(CodecOSDIMAGE *image, unsigned int id, unsigned int vid, unsigned int enable, VDS_FMT fmt, unsigned int x, unsigned int y, unsigned int buf)
{
	memset(image, 0, sizeof(CodecOSDIMAGE));
	image->id                  = id;
	image->phase               = VDS_PHASE_EXT;
	image->data.ext.vid        = vid;
	image->data.ext.fmt        = fmt;
	image->data.ext.en         = enable;
	image->data.ext.x          = x;
	image->data.ext.y          = y;
	image->data.ext.w          = WIDTH;
	image->data.ext.h          = HEIGHT;
	image->data.ext.alpha      = 128;
	image->data.ext.addr       = buf;
}

static void config_ps_osd(CodecOSDIMAGE *image, unsigned int id, unsigned int vid, VDS_FMT fmt, unsigned int enable, unsigned int x, unsigned int y, unsigned int buf)
{
	memset(image, 0, sizeof(CodecOSDIMAGE));
	image->id                 = id;
	image->phase              = VDS_PHASE_PS;
	image->data.ps.vid        = vid;
	image->data.ps.fmt        = fmt;
	image->data.ps.en         = enable;
	image->data.ps.x[0]       = x;
	image->data.ps.y[0]       = y;
	image->data.ps.w          = WIDTH;
	image->data.ps.h          = HEIGHT;
	image->data.ps.addr       = buf;
}

static int create_datetime_image(char *prefix, unsigned short *buf)
{
	char                 new_date_time[50], osd_string[100];
	int                  n, num_dt, font_size = 20, width = 0, height = 0;
	time_t               tmp_time;
	struct tm            *timep;

	if(!buf){
		printf("buf is null\r\n");
		return -1;
	}

	memset(new_date_time, 0, sizeof(new_date_time));
	time(&tmp_time);
	timep = localtime(&tmp_time);
	if (timep) {
		sprintf(new_date_time, "%d/%.2d/%.2d - %.2d:%.2d:%.2d", 1900 + timep->tm_year, 1 + timep->tm_mon, timep->tm_mday, timep->tm_hour, timep->tm_min, timep->tm_sec);
	} else {
		printf("freetyped : localtime() fail\r\n");
		return -1;
	}

	if(prefix){
		if(strlen(prefix) < sizeof(osd_string))
			strcpy(osd_string, prefix);
		else{
			printf("size of prefix(%d) > size of osd string(%d)\r\n", strlen(prefix), sizeof(osd_string));
			return -1;
		}
	}

	if((strlen(prefix) + strlen(new_date_time)) < sizeof(osd_string))
		strcat(osd_string, new_date_time);
	else{
		printf("size of prefix(%d) + size of timestamp(%d) > size of osd string(%d)\r\n", strlen(prefix), strlen(new_date_time), sizeof(osd_string));
		return -1;
	}

	slot = face[0]->glyph;

	memset(buf, 0, WIDTH * HEIGHT * 2);

	pen.x = 0;
	pen.y = 640;

	num_dt = strlen(osd_string);
	for (n = 0; n < num_dt; n++) {
		/* set transformation */
		FT_Set_Transform(face[0], &matrix, &pen);

		/* load glyph image into the slot (erase previous one) */
		error = FT_Load_Char(face[0], osd_string[n], FT_LOAD_RENDER);
		if (error) {
			printf("FT_Load_Char(%c) fail with %d\n", osd_string[n], error);
			return -1;
		}

		/* now, draw to our target surface (convert position) */
		draw_bitmap(&slot->bitmap, slot->bitmap_left, HEIGHT - slot->bitmap_top, buf);

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;

		height = slot->bitmap_top + (font_size - (slot->bitmap_top % font_size));
		width  = slot->bitmap_left + slot->bitmap.width;
		width  = width  + (font_size - (width % font_size));
		width += (4 - (width % 4));
	}

	if (width > WIDTH || height > HEIGHT) {
		printf("freetyped : image w(%d) h(%d) > max w(%d) h(%d)\r\n", width, height, WIDTH, HEIGHT);
		return -1;
	}

	return 0;
}

static int draw_ps_osd(void)
{
	CodecOSDIMAGE ipc_image;

	if(create_datetime_image("PS : ", (unsigned short*)dimage))
		return -1;

	config_ps_osd(&ipc_image, 0, 0, VDS_FMT_PICTURE_ARGB4444, 1, 0, 0, (unsigned int)dimage);
	if (NvtCodec_AddVdsOSD(&ipc_image)) {
		printf("freetyped fail to add ps image 0\r\n");
		return -1;
	}

	return 0;
}

static int draw_btn_osd(void)
{
        CodecOSDIMAGE ipc_image;

        if(create_datetime_image("BTN : ", (unsigned short*)dimage))
                return -1;

	config_btn_osd(&ipc_image, 0, 1, 1, 2, 1, VDS_FMT_PICTURE_ARGB4444, 0, 100, (unsigned int)dimage);
	if (NvtCodec_AddVdsOSD(&ipc_image)) {
		printf("freetyped fail to add btn image 0\r\n");
		return -1;
	}

        return 0;
}

static int draw_ext_osd(void)
{
        CodecOSDIMAGE ipc_image;

        if(create_datetime_image("EXT : ", (unsigned short*)dimage))
                return -1;

	config_ext_osd(&ipc_image, 0, 2, 1, VDS_FMT_PICTURE_ARGB4444, 0, 200, (unsigned int)dimage);
        if (NvtCodec_AddVdsOSD(&ipc_image)) {
                printf("freetyped fail to add ext image 0\r\n");
                return -1;
        }

        return 0;
}

int main(int argc, char  **argv)
{
	char                *font_file;
	int                  font_size = 40, n;

	if (create_pid_file()) {
		return -1;
	}

	font_file = argv[1];          
	if (init_ftype(font_file, font_size, 0)) {
		return -1;
	}

	//********** initialize videosprite **********//
	if (NvtCodec_InitVds(	WIDTH * HEIGHT * 4 + //PS ping pong buffer
				WIDTH * HEIGHT * 4 + //BTN ping pong buffer
				WIDTH * HEIGHT * 4   //EXT ping pong buffer
	)) {
		printf("fail to initialize videosprite\r\n");
		return -1;
	}

	if (config_buffer()) {
		printf("fail to config scale buffer\r\n");
		return -1;
	}

	while (1) {

		//********** Draw PS Image **********//
		if (draw_ps_osd()) 
			goto loop;

		//********** Draw BTN Image **********//
		if (draw_btn_osd()) 
			goto loop;

		//********** Draw EXT Image **********//
		if (draw_ext_osd()) 
			goto loop;

		//********** flush osd setting **********//
		if (NvtCodec_SwapVdsBtnExtOSD(2)) {
			printf("freetyped fail to swap btn ext osd vid 0\r\n");
			goto loop;
		}
		if (NvtCodec_SwapVdsPsOSD(0)) {
			printf("freetyped fail to swap ps osd vid 0\r\n");
			goto loop;
		}
loop:
		sleep(1);
	}

        //********** Tear down videosprite **********//
        NvtCodec_UninitVds();

	for (n = 0 ; n < MAX_CHANNEL ; ++n) {
		FT_Done_Face(face[n]);
	}
	FT_Done_FreeType(library);

	return 0;
}

