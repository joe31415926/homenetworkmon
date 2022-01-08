#!/bin/bash
while :
do
/usr/bin/curl -o /home/pi/ramdisk/ip_tmp http://joeruff.com/ip
sleep 1
if [ -f /home/pi/ramdisk/ip_tmp ]
then
mv /home/pi/ramdisk/ip_tmp /home/pi/ramdisk/ip
sleep 60
fi
done
