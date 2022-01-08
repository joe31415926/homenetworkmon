while :
do
ping -c 1 -w 10 -p "4`date +%s%N`" -I eth0 joeruff.com
done
