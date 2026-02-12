kill -9 $(ps|grep start_rtmp_client.sh|grep -v grep|awk '{print $1}')
killall $1
