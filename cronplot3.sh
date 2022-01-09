#!/bin/bash
while :
do inotifywait -e delete_self /home/pi/ramdisk/log3.txt
   python3 /home/pi/homenetworkmon/plot.py < /home/pi/ramdisk/log3.txt > /home/pi/ramdisk/log3.tmp.rgba
   mv /home/pi/ramdisk/log3.tmp.rgba /home/pi/ramdisk/log3.rgba
done