#!/bin/bash
while :
do inotifywait -e delete_self /home/pi/ramdisk/log4.txt
   python3 /home/pi/homenetworkmon/plot.py < /home/pi/ramdisk/log4.txt > /home/pi/ramdisk/log4.tmp.rgba
   mv /home/pi/ramdisk/log4.tmp.png /home/pi/ramdisk/log4.rgba
done