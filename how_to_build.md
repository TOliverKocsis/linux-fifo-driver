1. build:
    make

2. test if can add it to virtual kernel

start the virtual kernel
vng --run

insert the character device:
sudo insmod /home/oliverk/Documents/kernel_dev/build/testfifo.ko

observer some printouts in ksystem logs:
sudo dmesg | tail -5

deregister and delete the character module:
sudo rmmod testfifo

observer the printout again:
sudo dmesg | tail -5

