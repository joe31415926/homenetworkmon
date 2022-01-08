#!/bin/bash
while [ ! -f /home/pi/ramdisk/ip ]
do
sleep 1
done

while :
do
/usr/bin/ssh -N -R 9100:localhost:22 -o StrictHostKeyChecking=no -o ServerAliveInterval=5 -o ServerAliveCountMax=3 -i /home/pi/.ssh/weaksecurity ubuntu@`cat /home/pi/ramdisk/ip`
sleep 5
done
