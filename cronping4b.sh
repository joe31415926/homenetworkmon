#!/bin/bash
while :
do
echo 4 | nc -n -u -w 1 127.0.0.1 8080
ping -c 1 -w 10 -p 4 -I eth0 joeruff.com
sleep 5
done
