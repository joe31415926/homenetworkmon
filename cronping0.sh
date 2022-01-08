#!/bin/bash
while :
do
ping -c 1 -w 10 -p "0`date +%s%N`" -I wlan0 `cat /home/pi/configure/pingip0.txt`
sleep $((30 + $RANDOM % 60))
done
