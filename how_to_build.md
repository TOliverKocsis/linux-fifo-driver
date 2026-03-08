1. build from main directory:

$ make
$ cd userspace && make

2. test if can add it to virtual kernel

start the virtual kernel
$ vng --run

insert the character device:
$ sudo insmod /home/oliverk/Documents/kernel_dev/build/testfifo.ko

observer some printouts in ksystem logs:
$ sudo dmesg | tail -5

check if device file exists in dev/:
$ ls -la /dev/testfifo

try to write soemthing into the device, and read it back
$ sudo chmod 666 /dev/testfifo
$ echo "hello" > /dev/testfifo
$ cat /dev/testfifo 

deregister and delete the character module:
$ sudo rmmod testfifo

observer the printout again:
$ sudo dmesg | tail -5

observe file not existing anymore in dev/:
$ ls -la /dev/testfifo

3. run the userspace program for testing:

$ vng --run
$ sudo insmod /home/oliverk/Documents/kernel_dev/build/testfifo.ko
$ sudo /home/oliverk/Documents/kernel_dev/userspace/fifo_userspace_test
