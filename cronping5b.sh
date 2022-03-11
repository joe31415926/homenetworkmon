#!/bin/bash
while :
do
echo 5 | nc -w 0 -u 127.0.0.1 8080
ping -c 1 -w 10 -p 5 -I eth0 joeruff.com
sleep 5.`echo $RANDOM`
done
