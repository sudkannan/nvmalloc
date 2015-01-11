sudo rmmod pmc.ko
sudo rm /dev/pmc
make clean && make && sudo make install
sudo insmod pmc.ko
sudo mknod /dev/pmc c 256 0
