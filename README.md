# homenetworkmon

### On the MacBook Pro

#### With the micro sd card in the MacBook Pro

```
diskUtil unmountDisk /dev/disk2
sudo dd bs=2048000 if=Downloads/2021-10-30-raspios-bullseye-armhf-lite.img of=/dev/disk2                                     
touch /Volumes/boot/ssh
cp ~/.ssh/personaljoeruff.pub /Volumes/boot/
cp ~/.ssh/weaksecurity /Volumes/boot/
cp wpa_supplicant.conf /Volumes/boot/
diskUtil unmountDisk /dev/disk2
```

#### With the micro sd card in the Raspberry Pi

remember the default password for the Raspberry Pi is `raspberry`

```
ssh-add ~/.ssh/personaljoeruff.pem
ssh -A pi@raspberrypi.local
```

### On the Raspberry Pi

```
sudo apt -y update
sudo apt -y upgrade
sudo apt -y install git gcc python3-pip libopenjp2-7 libatlas-base-dev inotify-tools
python3 -m pip install matplotlib
git clone git@github.com:joe31415926/homenetworkmon.git
sudo homenetworkmon/setup.sh
```
