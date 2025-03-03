!#/bin/bash
sudo rmmod "$1"
make clean
make module="$1.o"
sudo insmod "$1.ko"

