# homenetworkmon

### On the MacBook Pro

#### With the micro sd card in the MacBook Pro

```
diskUtil unmountDisk /dev/disk2
sudo dd bs=2048000 if=Downloads/2021-10-30-raspios-bullseye-armhf-lite.img of=/dev/disk2                                     
touch /Volumes/boot/ssh
diskUtil unmountDisk /dev/disk2
```

#### With the micro sd card in the Raspberry Pi

remember the default password for the Raspberry Pi is `raspberry`

```
scp ~/.ssh/id_rsa.pub pi@raspberrypi.local:
ssh-add ~/.ssh/id_rsa
ssh -A pi@raspberrypi.local
```

### On the Raspberry Pi

```
mkdir .ssh
mv id_rsa.pub .ssh/authorized_keys
sudo apt -y update
sudo apt -y upgrade
sudo apt -y install git gcc python3-pip
python3 -m pip install matplotlib
git clone git@github.com:joe31415926/homenetworkmon.git
sudo homenetworkmon/setup.sh
```