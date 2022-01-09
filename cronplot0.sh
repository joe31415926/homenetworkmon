#!/bin/bash
while :
do inotifywait -e delete_self /home/pi/ramdisk/log0.txt
   python3 /home/pi/homenetworkmon/plot.py < /home/pi/ramdisk/log0.txt > /home/pi/ramdisk/log0.tmp.rgba
   mv /home/pi/ramdisk/log0.tmp.png /home/pi/ramdisk/log0.rgba
done