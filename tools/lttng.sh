
insmod lib/lttng-lib-ring-buffer.ko
insmod probes/lttng-kretprobes.ko
insmod probes/lttng-kprobes.ko
insmod probes/lttng-ftrace.ko

insmod lttng-clock.ko
insmod lttng-statedump.ko
insmod lttng-tracer.ko

insmod lttng-ring-buffer-client-discard.ko
insmod lttng-ring-buffer-client-mmap-discard.ko
insmod lttng-ring-buffer-client-mmap-overwrite.ko
insmod lttng-ring-buffer-client-overwrite.ko
insmod lttng-ring-buffer-metadata-client.ko
insmod lttng-ring-buffer-metadata-mmap-client.ko

insmod probes/lttng-probe-workqueue.ko
insmod probes/lttng-probe-net.ko
insmod probes/lttng-probe-module.ko
insmod probes/lttng-probe-random.ko
insmod probes/lttng-probe-signal.ko
insmod probes/lttng-probe-scsi.ko
insmod probes/lttng-probe-ext4.ko
insmod probes/lttng-probe-rcu.ko
insmod probes/lttng-probe-statedump.ko
insmod probes/lttng-probe-vmscan.ko
insmod probes/lttng-probe-block.ko
insmod probes/lttng-probe-printk.ko
insmod probes/lttng-probe-sched.ko
insmod probes/lttng-probe-jbd2.ko
insmod probes/lttng-probe-gpio.ko
insmod probes/lttng-probe-i2c.ko
insmod probes/lttng-probe-power.ko
insmod probes/lttng-probe-napi.ko
insmod probes/lttng-probe-writeback.ko
insmod probes/lttng-kretprobes.ko
insmod probes/lttng-probe-udp.ko
insmod probes/lttng-probe-skb.ko
insmod probes/lttng-probe-sunrpc.ko
insmod probes/lttng-probe-irq.ko
insmod probes/lttng-probe-kmem.ko
insmod probes/lttng-probe-timer.ko
insmod probes/lttng-probe-sock.ko
insmod probes/lttng-probe-compaction.ko
