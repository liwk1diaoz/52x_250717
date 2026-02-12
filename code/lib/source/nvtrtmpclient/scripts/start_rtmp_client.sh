if ! [ $# -eq 3 ] ; then 
    echo "USAGE  : ./start_rtmp_client.sh [client_app] [channel] [rtmpURL]"; 
    echo "example: ./start_rtmp_client.sh nvtrtmpclient_test 0 rtmp://rtmp.url/live &"
    exit; 
fi

app_name=$1
channel=$2
rtmp_url=$3

full_cmd=$(echo $1 $2 $3)

#-------------------------------------------------------------------------------

$full_cmd &

#--------------------------------------------------------------------------------

while [ 1 ]; do
    PID=$(ps|pgrep $app_name);
    net_state=$(netstat -anp 2>/dev/null | grep $PID | awk '{print $6}');
    net_sendQ=$(netstat -anp 2>/dev/null | grep $PID | awk '{print $3}');
    
    if [ "$last_net_sendQ" = "$net_sendQ" ]; then 
        echo "EVENT_SENDQ_SAME"
        killall $app_name
        $full_cmd &
    fi

    if [ "$net_state" =  "CLOSE_WAIT" ]; then 
        echo "EVENT_CLOSE_WAIT"
        killall $app_name
        $full_cmd &
    fi

    last_net_sendQ=$net_sendQ;

    sleep 3;
done

