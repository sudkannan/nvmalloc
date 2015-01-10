sudo rmmod pmc.ko
sudo rm /dev/pmc
make clean && make
sudo insmod pmc.ko
sudo mknod /dev/pmc c 256 0
