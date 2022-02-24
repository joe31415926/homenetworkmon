#!/bin/bash
while :
do
echo 0 | nc -n -u -w 1 127.0.0.1 8080
ping -c 1 -w 10 -p 0 -I wlan0 `cat /home/pi/configure/pingip0.txt`
sleep 5
done
