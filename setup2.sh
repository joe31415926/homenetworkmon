#!/bin/bash
mkdir -p /home/pi/.ssh
cp -f /boot/personaljoeruff.pub /home/pi/.ssh/authorized_keys
cp -f /boot/weaksecurity /home/pi/.ssh/
chmod 0400 /home/pi/.ssh/weaksecurity
chown pi:pi /home/pi/.ssh/authorized_keys
chown pi:pi /home/pi/.ssh/weaksecurity
chown pi:pi /home/pi/.ssh
mkdir -p /home/pi/ramdisk
chown pi:pi /home/pi/ramdisk
grep -q /home/pi/ramdisk /etc/fstab
if [ $? -ne 0 ]
then echo "tmpfs /home/pi/ramdisk tmpfs size=8000000 0 0" >> /etc/fstab
fi
gcc -O3 -o /home/pi/monping2 /home/pi/homenetworkmon/monping2.c -pthread -lm
chown pi:pi /home/pi/monping2
crontab -u root /home/pi/homenetworkmon/crontab_root2
crontab -u pi /home/pi/homenetworkmon/crontab_pi2
cp -pR /home/pi/homenetworkmon/configure /home/pi/
/usr/bin/raspi-config nonint do_wifi_country US
sudo timedatectl set-timezone US/Pacific
