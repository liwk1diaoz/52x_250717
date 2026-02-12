
MODULE_DIR=/lib/modules/$(uname -r)/extra/bt/rtl8821CS_20220330_LINUX_BT_DRIVER_RTL8821C_COEX_v5555/uart/
BT_UART=ttyS4

# P_GPIO2 = 0x20 + 2 = 34
BT_DIS_PIN=34
DELAY=0.1


# Reload hci_uart module
killall -9 rtk_hciattach

sleep ${DELAY}

lsmod | grep hci_uart && rmmod hci_uart.ko

insmod ${MODULE_DIR}/hci_uart.ko

sleep ${DELAY}


# Check BS_DIS pin
if [ ! -d /sys/devices/gpiochip0/gpio/gpio${BT_DIS_PIN} ]; then
	echo ${BT_DIS_PIN} > /sys/class/gpio/export
	sleep ${DELAY}
fi


# Config BS_DIS pin
echo "out" > /sys/devices/gpiochip0/gpio/gpio${BT_DIS_PIN}/direction

echo 0 > /sys/devices/gpiochip0/gpio/gpio${BT_DIS_PIN}/value

sleep ${DELAY}

echo 1 > /sys/devices/gpiochip0/gpio/gpio${BT_DIS_PIN}/value


# Load BT firmware
rtk_hciattach -n -s 115200 ${BT_UART} rtk_h5 &


# Start Bluez daemons
if [ -L $0 ] ; then
	DIR=$(dirname $(readlink -f $0)) ;
else
	DIR=$(dirname $0) ;
fi ;

# if using dbus api please run init_bluetoothd.sh
#${DIR}/init_bluetoothd.sh