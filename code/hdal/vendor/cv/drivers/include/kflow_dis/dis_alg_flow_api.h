#ifndef __DIS_ALG_FLOW_API_H_
#define __DIS_ALG_FLOW_API_H_

//#define DEBUG_KDRV_DIS_ALG


#ifdef DEBUG_KDRV_DIS_ALG
int exam_kflow_dis_task_init(unsigned char argc, char **p_argv);
int exam_kflow_dis_task_get(unsigned char argc, char **p_argv);
int exam_kflow_dis_task_reset(unsigned char argc, char **p_argv);
int exam_kflow_dis_task_uninit(unsigned char argc, char **p_argv);
#endif

int exam_kflow_dis_read_version(unsigned char argc, char **pargv);
int exam_kflow_dis_dump_global_mv_start(unsigned char argc, char **pargv);
int exam_kflow_dis_dump_global_mv_end(unsigned char argc, char **pargv);
int exam_kflow_dis_dump_param_in(unsigned char argc, char **pargv);
int exam_kflow_dis_dump_buffsize(unsigned char argc, char **pargv);




#endif
