#include <osg.h>

#if 0

#define __ALIGN_CEIL_128(a)       ((a+127) & ~0x07f)

static int draw_cross_argb1555(unsigned short *buf)
{
	int i, j;

	if(buf == NULL){
		printk(KERN_ERR "buf is NULL\n");
		return -1;
	}
	
	//draw vertical bar
	for(i = 25 ; i < 75 ; ++i){
		for(j = 0 ; j < 100 ; ++j){
			buf[i + j*100] = 0xfc00;
		}
	}
	
	//draw horizontal bar
	for(i = 0 ; i < 100 ; ++i){
		for(j = 25 ; j < 75 ; ++j){
			buf[i + j*100] = 0xfc00;
		}
	}
	
	return 0;
}

static int draw_cross_argb4444(unsigned short *buf)
{
	int i, j;

	if(buf == NULL){
		printk(KERN_ERR "buf is NULL\n");
		return -1;
	}
	
	//draw vertical bar
	for(i = 25 ; i < 75 ; ++i){
		for(j = 0 ; j < 100 ; ++j){
			buf[i + j*100] = 0xf0f0;
		}
	}
	
	//draw horizontal bar
	for(i = 0 ; i < 100 ; ++i){
		for(j = 25 ; j < 75 ; ++j){
			buf[i + j*100] = 0xf0f0;
		}
	}
	
	return 0;
}

static int draw_cross_argb8888(unsigned int *buf)
{
	int i, j;

	if(buf == NULL){
		printk(KERN_ERR "buf is NULL\n");
		return -1;
	}
	
	//draw vertical bar
	for(i = 25 ; i < 75 ; ++i){
		for(j = 0 ; j < 100 ; ++j){
			buf[i + j*100] = 0xff0000ff;
		}
	}
	
	//draw horizontal bar
	for(i = 0 ; i < 100 ; ++i){
		for(j = 25 ; j < 75 ; ++j){
			buf[i + j*100] = 0xff0000ff;
		}
	}
	
	return 0;
}

static int draw_cross(void *buf, VDO_PXLFMT fmt)
{
	if(fmt == OSG_PXLFMT_ARGB1555){
		return draw_cross_argb1555(buf);
	}else if(fmt == OSG_PXLFMT_ARGB4444){
		return draw_cross_argb4444(buf);
	}else if(fmt == OSG_PXLFMT_ARGB8888){
		return draw_cross_argb8888(buf);
	}else{
		printk(KERN_ERR "invalid osg format(%x)\n", fmt);
		return -1;
	}
}

static int setup_osg_stamp(UINT32 path)
{
	OSG osg;
	OSG_STAMP stamp[3] = {0};
	unsigned char *osg_buffer = NULL, *temp = NULL;
	int ret = -1;

	stamp[0].buf.type        = OSG_BUF_TYPE_PING_PONG;
	stamp[0].buf.size        = __ALIGN_CEIL_128(100*100*2*2); //should be 128 aligned
	stamp[0].buf.ddr_id      = 0;
	stamp[0].img.fmt         = OSG_PXLFMT_ARGB1555;
	stamp[0].img.dim.w       = 100;
	stamp[0].img.dim.h       = 100;
	stamp[0].attr.alpha      = 255;
	stamp[0].attr.position.x = 0;
	stamp[0].attr.position.y = 0;
	stamp[0].attr.layer      = 0;
	stamp[0].attr.region     = 0;

	stamp[1].buf.type        = OSG_BUF_TYPE_SINGLE;
	stamp[1].buf.size        = __ALIGN_CEIL_128(100*100*2); //should be 128 aligned
	stamp[1].buf.ddr_id      = 0;
	stamp[1].img.fmt         = OSG_PXLFMT_ARGB4444;
	stamp[1].img.dim.w       = 100;
	stamp[1].img.dim.h       = 100;
	stamp[1].attr.alpha      = 255;
	stamp[1].attr.position.x = 128;
	stamp[1].attr.position.y = 128;
	stamp[1].attr.layer      = 0;
	stamp[1].attr.region     = 1;

	stamp[2].buf.type        = OSG_BUF_TYPE_PING_PONG;
	stamp[2].buf.size        = __ALIGN_CEIL_128(100*100*4*2); //should be 128 aligned
	stamp[2].buf.ddr_id      = 0;
	stamp[2].img.fmt         = OSG_PXLFMT_ARGB8888;
	stamp[2].img.dim.w       = 100;
	stamp[2].img.dim.h       = 100;
	stamp[2].attr.alpha      = 255;
	stamp[2].attr.position.x = 256;
	stamp[2].attr.position.y = 256;
	stamp[2].attr.layer      = 0;
	stamp[2].attr.region     = 2;

	osg_buffer = kmalloc(stamp[0].buf.size + stamp[1].buf.size + stamp[2].buf.size, GFP_KERNEL);
	if(osg_buffer == NULL){
		printk(KERN_ERR "fail to allocate %d bytes for osg buffer\n", stamp[0].buf.size + stamp[1].buf.size + stamp[2].buf.size);
		return -1;
	}

	stamp[0].buf.p_addr = (uintptr_t)osg_buffer; //should be 128 aligned
	stamp[1].buf.p_addr = (stamp[0].buf.p_addr + stamp[0].buf.size); //should be 128 aligned
	stamp[2].buf.p_addr = (stamp[1].buf.p_addr + stamp[1].buf.size); //should be 128 aligned

	osg.num = 3;
	osg.stamp = stamp;

	if(vds_set_early_osg(path, &osg)){
		printk(KERN_ERR "vds_set_early_osg() fail\n");
		goto out;
	}

	temp = kmalloc(100*100*4, GFP_KERNEL);
	if(temp == NULL){
		printk(KERN_ERR "fail to allocate %d bytes for temp osg buffer\n", 100*100*4);
		goto out;
	}

	memset(temp, 0, 100*100*4);
	if(draw_cross(temp, OSG_PXLFMT_ARGB1555)){
		printk(KERN_ERR "fail to draw a argb1555 cross\n");
		goto out;
	}
	if(vds_update_early_osg(path, 0, (uintptr_t)temp)){
		printk(KERN_ERR "vds_update_early_osg(0, 0) fail\n");
		goto out;
	}
	
	memset(temp, 0, 100*100*4);
	if(draw_cross(temp, OSG_PXLFMT_ARGB4444)){
		printk(KERN_ERR "fail to draw a argb4444 cross\n");
		goto out;
	}
	if(vds_update_early_osg(path, 1, (uintptr_t)temp)){
		printk(KERN_ERR "vds_update_early_osg(0, 1) fail\n");
		goto out;
	}

	memset(temp, 0, 100*100*4);
	if(draw_cross(temp, OSG_PXLFMT_ARGB8888)){
		printk(KERN_ERR "fail to draw a argb8888 cross\n");
		goto out;
	}
	if(vds_update_early_osg(path, 2, (uintptr_t)temp)){
		printk(KERN_ERR "vds_update_early_osg(0, 2) fail\n");
		goto out;
	}
	
	ret = 0;

out:

	if(temp){
		kfree(temp);
	}

	return ret;
}

// setup 3 red, green and blue cross on encoding pathID(0) and jpeg pathID(2)
// All have 100x100 dimension
// Two of them use ping pong buffer and the remaining one uses single buffer
// the 1st cross is argb1555 and the 2nd cross is argb4444 and the 3rd cross is argb8888
// the osg_buffer should be manually freed when built-in fastboot ends. This demo code doesn't free osg_buffer
// It's recommended to allocate osg_buffer from hdal common pool and return osg_buffer back to hdal common pool when hdal initializes
int builtin_osg_demo_init(void)
{
	if(setup_osg_stamp(0)){
		printk(KERN_ERR "fail to setup h26x stamp\n");
		return -1;
	}

	if(setup_osg_stamp(2)){
		printk(KERN_ERR "fail to setup jpeg stamp\n");
		return -1;
	}

	return 0;
}
#else //__KERNEL__
int builtin_osg_demo_init(void)
{
	return 0;
}
#endif //__KERNEL__
