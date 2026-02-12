#!/bin/sh
VOLTAGE=900000
VOLTAGE_MIN=800000

while [ "$VOLTAGE" -ge "$VOLTAGE_MIN" ];
do
	VOLTAGE=$(($VOLTAGE-10000))
	echo "TEST Volt >>>>>>> $VOLTAGE"
	echo $VOLTAGE > /proc/power_ctrl/regulator_ctrl
	cpu_stress 100000
	ret=$?
	if [ $ret == 0 ]; then
		echo pass;
	else
		echo fail;
	fi
done

