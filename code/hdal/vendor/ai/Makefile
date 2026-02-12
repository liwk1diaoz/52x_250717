MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))

NVT_VENDOR_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_KDRV_PATH=$(NVT_VENDOR_AI_PATH)/drivers/k_driver/source
NVT_KFLOW_PATH=$(NVT_VENDOR_AI_PATH)/drivers/k_flow/source
NVT_SOURCE_PATH=$(NVT_VENDOR_AI_PATH)/source
NVT_SOURCE_PUB_PATH=$(NVT_VENDOR_AI_PATH)/source_pub
NVT_SAMPLE_PATH=$(NVT_HDAL_DIR)/samples
NVT_TEST_CASE_PATH=$(NVT_HDAL_DIR)/test_cases

all:
	@$(MAKE) -C $(NVT_KDRV_PATH) -j$(JOBS)
	#@$(MAKE) -C $(NVT_KFLOW_PATH) -j$(JOBS)
	#@$(MAKE) -C $(NVT_SOURCE_PATH) -j$(JOBS)
	#@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) -j$(JOBS)
	#@$(MAKE) -C $(NVT_SAMPLE_PATH) -j$(JOBS)
	@rm -rf $(NVT_SAMPLE_PATH)/ai_auto_test
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fc_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_pvdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_pvdcnn_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_ftg_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_bindcpu_test
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_multi_in
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream_2dev
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream_2net
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_bin_file
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_cycle_time
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_linux_cpu_loading
	@rm -rf $(NVT_SAMPLE_PATH)/alg_pdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_preproc_sample
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_fdcnn_pvdcnn_sample_stream_ipc
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_fdcnn_sample_stream_ipc
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_pdcnn_sample_stream_ipc
	
install:
	@$(MAKE) -C $(NVT_KDRV_PATH) modules_install
	#@$(MAKE) -C $(NVT_KFLOW_PATH) modules_install
	#@$(MAKE) -C $(NVT_SOURCE_PATH) install
	#@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) install

clean:
	@$(MAKE) -C $(NVT_KDRV_PATH) clean
	#@$(MAKE) -C $(NVT_KFLOW_PATH) clean
	#@$(MAKE) -C $(NVT_SOURCE_PATH) clean
	#@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) clean
	#@$(MAKE) -C $(NVT_SAMPLE_PATH) clean
	@rm -rf $(NVT_SAMPLE_PATH)/ai_auto_test
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fc_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_pvdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_pvdcnn_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_fdcnn_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_ftg_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_bindcpu_test
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_multi_in
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream_2dev
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_app_user_sample_stream_2net
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_bin_file
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_cycle_time
	@rm -rf $(NVT_SAMPLE_PATH)/alg_net_test_linux_cpu_loading
	@rm -rf $(NVT_SAMPLE_PATH)/alg_pdcnn_sample
	@rm -rf $(NVT_SAMPLE_PATH)/alg_preproc_sample
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_fdcnn_pvdcnn_sample_stream_ipc
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_fdcnn_sample_stream_ipc
	@rm -rf $(NVT_TEST_CASE_PATH)/alg_pdcnn_sample_stream_ipc

codesize:
	@$(MAKE) -C $(NVT_KDRV_PATH) codesize
	#@$(MAKE) -C $(NVT_KFLOW_PATH) codesize
	#@$(MAKE) -C $(NVT_SOURCE_PATH) codesize
	#@$(MAKE) -C $(NVT_SOURCE_PUB_PATH) codesize