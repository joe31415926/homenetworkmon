#!/bin/bash
while :
do
echo 1 | nc -w 0 -u 127.0.0.1 8080
ping -c 1 -w 10 -p 1 -I wlan0 `cat /home/pi/configure/pingip1.txt`
sleep 5.`echo $RANDOM`
done
