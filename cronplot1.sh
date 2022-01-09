#!/bin/bash
while :
do inotifywait -e delete_self /home/pi/ramdisk/log1.txt
   python3 /home/pi/homenetworkmon/plot.py < /home/pi/ramdisk/log1.txt > /home/pi/ramdisk/log1.tmp.rgba
   mv /home/pi/ramdisk/log1.tmp.png /home/pi/ramdisk/log1.rgba
done