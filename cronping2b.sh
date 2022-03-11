#!/bin/bash
while :
do
echo 2 | nc -w 0 -u 127.0.0.1 8080
ping -c 1 -w 10 -p 2 -I eth0 `cat /home/pi/configure/pingip0.txt`
sleep 5.`echo $RANDOM`
done
