#!/bin/bash
while :
do inotifywait -e delete_self /home/pi/ramdisk/log2.txt
   python3 /home/pi/homenetworkmon/plot.py < /home/pi/ramdisk/log2.txt > /home/pi/ramdisk/log2.tmp.rgba
   mv /home/pi/ramdisk/log2.tmp.rgba /home/pi/ramdisk/log2.rgba
done