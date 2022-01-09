#!/bin/bash
while :
do
ping -c 1 -w 10 -p "1`date +%s%N`" -I eth0 `cat /home/pi/configure/pingip0.txt`
sleep 5
done
