#!/bin/bash
while :
do
ping -c 1 -w 10 -p "2`date +%s%N`" -I eth0 `cat /home/pi/configure/pingip1.txt`
sleep 5
done
