#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ROOT "/mnt/sd/"

int create_first_loop(char *model){
	
	char fileName[256] = {0};
	char buffer[256] = {0};
    FILE* file = NULL;
	
	if(model == NULL){
		printf("model is NULL\n");
		return -1;
	}
	
	sprintf(fileName, ROOT "ai2_test_item/99/para/first_loop/%s.txt", model);
	file = fopen(fileName, "w"); /* should check the result */
	if(file == NULL){
		printf("fail to open %s\n", fileName);
		return -1;
	}
	
	strcpy(buffer, "###FIRST_LOOP_DATA\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_path] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[invalid_net] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_buf_opt_method] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_buf_opt_ddr_id] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_buf_opt_invalid] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_job_opt_method] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_job_opt_wait_ms] = -1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[net_job_opt_schd_parm] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###SECOND_LOOP_DATA\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[second_loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###THIRD_LOOP_DATA\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[third_loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###FOURTH_LOOP_DATA\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[fourth_loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###MODEL_CFG_PATH\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[model_cfg_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "[model_cfg_path] = /mnt/sd/ai2_test_item/configs/para/model_cfg/%s.txt\n", model);
	fwrite (buffer , 1, strlen(buffer), file);

    fclose(file);
	
	return 0;
}

int create_image_list(char *model){
	
	char fileName[256] = {0};
	char buffer[256] = {0};
    FILE* file = NULL;
	
	if(model == NULL){
		printf("model is NULL\n");
		return -1;
	}
	
	sprintf(fileName, ROOT "ai2_test_item/configs/para/image_lists/%s.txt", model);
	file = fopen(fileName, "w"); /* should check the result */
	if(file == NULL){
		printf("fail to open %s\n", fileName);
		return -1;
	}

	strcpy(buffer, "1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "/mnt/sd/ai2_test_item/configs/para/in_cfg/%s.txt\n", model);
	fwrite (buffer , 1, strlen(buffer), file);

    fclose(file);
	
	return 0;
}

int create_in_cfg(char *model, char **input_image, int input_image_num){

	char fileName[256] = {0};
	char buffer[256] = {0};
    FILE* file = NULL;
	int i;
	
	if(model == NULL || input_image == NULL){
		printf("model(%p) or input_image is NULL\n", model);
		return -1;
	}
	
	sprintf(fileName, ROOT "ai2_test_item/configs/para/in_cfg/%s.txt", model);
	file = fopen(fileName, "w"); /* should check the result */
	if(file == NULL){
		printf("fail to open %s\n", fileName);
		return -1;
	}

	sprintf(buffer, "%d\n", input_image_num);
	fwrite (buffer , 1, strlen(buffer), file);
	
	for (i = 0; i < input_image_num; i++) {	
		sprintf(buffer, "%s\n", input_image[i]);
		fwrite (buffer , 1, strlen(buffer), file);
	}
	
    fclose(file);
	
	return 0;
}

int create_model_cfg(char *model_path, char *model){
	
	char fileName[256] = {0};
	char buffer[256] = {0};
    FILE* file = NULL;
	
	if(model_path == NULL || model == NULL){
		printf("model_path(%p) or model(%p) is NULL\n", model_path, model);
		return -1;
	}
	
	sprintf(fileName, ROOT "ai2_test_item/configs/para/model_cfg/%s.txt", model);
	file = fopen(fileName, "w"); /* should check the result */
	if(file == NULL){
		printf("fail to open %s\n", fileName);
		return -1;
	}

	strcpy(buffer, "###MODEL_CONFIG\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "[model_filename] = %s\n", model_path);
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[diff_filename] = \n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[invalid] = 0\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[in_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###IN_CONFIG_PATH\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "[in_file_path] = /mnt/sd/ai2_test_item/configs/para/image_lists/%s.txt\n", model);
	fwrite (buffer , 1, strlen(buffer), file);

    fclose(file);
	
	return 0;
}

int main(int argc, char **argv)
{
	char *model = NULL, *model_path = NULL, *output_dir = NULL, **input_image = NULL;
	char fileName[256] = {0};
	char buffer[256] = {0};
    FILE* file = NULL;
	int input_image_num = argc - 4;
	int i;

	printf("input_image_num = %d\r\n", input_image_num);

	if(argc < 5){
		printf("incorrect argument\n");
		printf("usage : ai_generate_config modelName modelPath outputDir input1Info input2Info...\n");
		return -1;
	}
	
	model = argv[1];
	model_path = argv[2];
	output_dir = argv[3];

	input_image = (char **)malloc(sizeof(char *)*input_image_num);

	for (i = 0; i < input_image_num; i++) {
		input_image[i] = argv[4+i];

		printf("input_image[%d] = %s\r\n", i, input_image[i]);
	}
	
	printf("model = %s\n", model);
	printf("model_path = %s\n", model_path);
	printf("output_dir = %s\n", output_dir);

	system("mkdir " ROOT "ai2_test_item");
	system("mkdir " ROOT "ai2_test_item/99");
	system("mkdir " ROOT "ai2_test_item/99/para");
	system("mkdir " ROOT "ai2_test_item/99/para/first_loop");
	system("mkdir " ROOT "ai2_test_item/99/para/global");
	system("mkdir " ROOT "ai2_test_item/configs");
	system("mkdir " ROOT "ai2_test_item/configs/para");
	system("mkdir " ROOT "ai2_test_item/configs/para/image_lists");
	system("mkdir " ROOT "ai2_test_item/configs/para/in_cfg");
	system("mkdir " ROOT "ai2_test_item/configs/para/model_cfg");
	
	sprintf(fileName, ROOT "ai2_test_item/99/para/global/global.txt");
	file = fopen(fileName, "w"); /* should check the result */
	if(file == NULL){
		printf("fail to open %s\n", fileName);
		return -1;
	}
	
	strcpy(buffer, "###GLOBAL_DATA\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[sequential] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[sequential_loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[timeout] = 600\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "[generate_golden_output] = %s/%s\n", output_dir, model);
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "###FIRST_LOOP\n");
	fwrite (buffer , 1, strlen(buffer), file);
	strcpy(buffer, "[first_loop_count] = 1\n");
	fwrite (buffer , 1, strlen(buffer), file);
	sprintf(buffer, "[first_loop_count_path] = /mnt/sd/para/first_loop/%s.txt\n", model);
	fwrite (buffer , 1, strlen(buffer), file);

    fclose(file);
	
	if(create_first_loop(model)){
		printf("fail to create first loop txt for %s\n", model);
		return -1;
	}
	
	if(create_image_list(model)){
		printf("fail to create image list txt for %s\n", model);
		return -1;
	}
	
	if(create_in_cfg(model, input_image, input_image_num)){
		printf("fail to create in cfg txt\n");
		return -1;
	}
	
	if(create_model_cfg(model_path, model)){
		printf("fail to create model cfg txt for %s\n", model);
		return -1;
	}

	free(input_image);
	
	return 0;
}
