#!/bin/bash
while :
do
echo 3 | nc -w 0 -u 127.0.0.1 8080
ping -c 1 -w 10 -p 3 -I eth0 `cat /home/pi/configure/pingip1.txt`
sleep 5.`echo $RANDOM`
done
