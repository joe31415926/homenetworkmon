#!/bin/bash
while :
do
ping -c 1 -w 10 -p "3`date +%s%N`" -I eth0 `cat /home/pi/ramdisk/ip`
sleep $((30 + $RANDOM % 60))
done
