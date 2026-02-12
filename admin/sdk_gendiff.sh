rev_start=$1
rev_end=$2

function get_file_list () {
	file_list=`git diff --name-only $rev_start...$rev_end $1 | sort`
	echo $file_list;
}

file_list_lib=`get_file_list lib`
file_list_app=`get_file_list application`
file_list_linux=`get_file_list linux-kernel`
file_list_supplement=`get_file_list linux-supplement`
file_list_include=`get_file_list include`
file_list_rootfs=`get_file_list root-fs`
file_list_sample=`get_file_list sample`
file_list_uboot=`get_file_list u-boot`
