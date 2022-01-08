#!/usr/bin/bash
mkdir -p /home/pi/ramdisk
chown pi:pi /home/pi/ramdisk
grep -q /home/pi/ramdisk /etc/fstab
if [ $? -ne 0 ]
then echo "tmpfs /home/pi/ramdisk tmpfs size=8000000 0 0" >> /etc/fstab
fi
gcc -O3 -o /home/pi/monping /home/pi/homenetworkmon/monping.c
chown pi:pi /home/pi/monping
crontab -u root /home/pi/homenetworkmon/crontab_root
crontab -u pi /home/pi/homenetworkmon/crontab_pi
cp -pR /home/pi/homenetworkmon/configure /home/pi/
