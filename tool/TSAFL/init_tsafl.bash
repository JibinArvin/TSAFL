#! /bin/bash

echo core >/proc/sys/kernel/core_pattern
pushd /sys/devices/system/cpu
echo performance | tee cpu*/cpufreq/scaling_governor
popd
echo 'kernel.sched_rt_runtime_us=-1' >> /etc/sysctl.conf
sysctl -w kernel.sched_rt_runtime_us=-1