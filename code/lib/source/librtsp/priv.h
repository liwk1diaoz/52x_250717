#ifndef __PRIV_H_

#define __PRIV_H_

int video_timesync(struct rtp_endpoint *ep, unsigned int curr_time, int time_incr);
void h264_timesync(struct rtp_endpoint *ep, unsigned int curr_time, int time_incr);
void jpeg_timesync(struct rtp_endpoint *ep, unsigned int curr_time, int time_incr);
void gm_check(struct rtp_endpoint *ep);

#endif

