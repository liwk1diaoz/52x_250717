#!/bin/sh

all_pids=`ls /proc | grep -E '^[0-9]+$'`
PID_PSS_RES=""
PID_NUM_LIST=""

for n in $all_pids; do 
	SIZE=`pmap -x $n | tail -1 | awk -F' ' '{print $3}'`
	PID_PSS_RES="$SIZE $PID_PSS_RES";
	if [ -f /proc/$n/comm ]; then PID_NUM_LIST="$n:$SIZE(`cat /proc/$n/comm 2>&1 /dev/null`) $PID_NUM_LIST"; fi;
done

for n in $PID_PSS_RES; do
	TOTAL=$((n+TOTAL))
done
echo "Your user space memory consumption size(with Proportional shared lib) is $TOTAL" KBytes

echo "PID >>> "$PID_NUM_LIST
echo "Mem usage(KB) >>> "$PID_PSS_RES
