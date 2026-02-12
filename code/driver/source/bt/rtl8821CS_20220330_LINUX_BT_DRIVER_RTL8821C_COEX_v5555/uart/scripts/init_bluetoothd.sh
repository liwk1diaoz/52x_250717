

DBUS_DIR=/var/run/dbus

if [ ! -d ${DBUS_DIR} ]; then \
	mkdir -p /var/run/dbus \
else
	if [ -e ${DBUS_DIR}/pid ]; then
		rm ${DBUS_DIR}/pid
	fi
fi

killall -9 dbus-daemon

killall -9 bluetoothd

cat /etc/group | grep messagebus 

if [ ! $? = 0 ]; then
	echo messagebus:x:1000: >> /etc/group
fi


cat /etc/group | grep bluetooth

if [ ! $? = 0 ]; then
	echo bluetooth:x:1001: >> /etc/group
fi


cat /etc/passwd | grep messagebus

if [ ! $? = 0 ]; then
	echo messagebus::1000:1000:messagebus:/messagebus: >> /etc/passwd
fi


cat /etc/passwd | grep bluetooth

if [ ! $? = 0 ]; then
	echo bluetooth::1001:1001:bluetooth:/bluetooth: >> /etc/passwd
fi

dbus-daemon --system

bluetoothd -n -d &


