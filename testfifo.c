// SPDX-License-Identifier: GPL-2.0
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver K");
MODULE_DESCRIPTION("OliversTestFifo");
MODULE_VERSION("0.1");

static dev_t dev_num;
static struct cdev testfifo_cdev;
static struct class *testfifo_class;
static struct device *testfifo_device;

#define FIFO_BUF_SIZE 4096 // needs to be pw of 2

static char *fifo_buf;
static size_t fifo_head;	// next byte to read
static size_t fifo_tail;	// next byte to write
static size_t fifo_count;	// bytes currently in buffer
static DEFINE_MUTEX(fifo_lock); // protects head, tail, count and buf contents

static ssize_t testfifo_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	//  *file pointer points to the bookeeping info of the open file: which
	//  process instance of the file, what current state user is an
	//  annotation for static analysis, compiler ignores it ppos will not be
	//  used, but we must match kernels .write defitionion to include this
	//  function pointer into the file operations

	// interruptible: if another process holds the lock, sleep until it's
	// free.
	if (mutex_lock_interruptible(&fifo_lock)) {
		// kernel internal restart toke: kernel will decide if should
		// restart or give error to userspace
		return -ERESTARTSYS;
	}

	// write is capped to remainign space in buffer, and for simplicity to
	// the remaining size until wrap around
	// TODO: utest capped write
	size_t available_space = FIFO_BUF_SIZE - fifo_count;
	size_t to_end = FIFO_BUF_SIZE - fifo_tail;
	size_t to_write =
	    min(count, available_space); // min from <linux/mimax.h> pulled in
					 // by <linux/kernel.h>
	to_write = min(to_write, to_end);

	if (to_write == 0) {
		mutex_unlock(&fifo_lock);
		return -ENOSPC;
	}

	// copy_from_user: bytes from userspace virtual address space into
	// kernel buffer returns number of bytes it FAILED to copy (0 = success)
	if (copy_from_user(fifo_buf + fifo_tail, buf, to_write)) {
		// partial/full failure: we do NOT update fifo tail
		mutex_unlock(&fifo_lock);
		return -EFAULT;
	}

	fifo_tail = (fifo_tail + to_write) % FIFO_BUF_SIZE;
	fifo_count += to_write;

	mutex_unlock(&fifo_lock);
	return to_write;
}

/*
 * Read (/drain) count amount of bytes from fifo to userspace buffer
 * Return: bytes transfered from fifo transfered
 */
static ssize_t testfifo_read(struct file *file, char __user *buf, size_t count,
			     loff_t *ppos)
{
	// interruptible: if another process holds the lock, sleep until it's
	// free.
	if (mutex_lock_interruptible(&fifo_lock)) {
		// kernel internal restart toke: kernel will decide if should
		// restart or give error to userspace
		return -ERESTARTSYS;
	}

	// cap amount that can be read, for simplicity sake we cap until wrap
	// around
	size_t to_end = FIFO_BUF_SIZE - fifo_head;
	//  [--h-------t--]
	// *b            *b+size
	//     |-count-|
	size_t read_max = min(count, fifo_count);
	size_t read_amount = min(to_end, read_max);

	if (read_amount == 0) {
		mutex_unlock(&fifo_lock);
		return 0;
	}

	// copy_to_user: bytes from kernel virtual memory to userspace virtual
	// memory
	if (copy_to_user(buf, fifo_buf + fifo_head, read_amount)) {
		mutex_unlock(&fifo_lock);
		return -EFAULT;
	}

	// if copy was succesful we can move head pointer
	fifo_head = (fifo_head + read_amount) % FIFO_BUF_SIZE;
	fifo_count -= read_amount;

	mutex_unlock(&fifo_lock);
	return read_amount;
}

// called when userspace opens /dev/testfifo
static int testfifo_open(struct inode *inode, struct file *file)
{
	pr_info("testfifo: open\n");
	return 0;
}

// called when userspace closes the file descriptor
static int testfifo_release(struct inode *inode, struct file *file)
{
	pr_info("testfifo: release\n");
	return 0;
}

// vtable: maps syscalls to our functions
// .owner prevents the module being unloaded while a file is open
static const struct file_operations testfifo_fops = {
    .owner = THIS_MODULE, // ref count of how many processes using this cdev
    .open = testfifo_open,
    .release = testfifo_release,
    .write = testfifo_write,
    .read = testfifo_read,
};

static int __init testfifo_init(void)
{
	int ret;

	// ask kernel to allocate a device number for us
	ret = alloc_chrdev_region(&dev_num, 0, 1, "testfifo");
	if (ret < 0) {
		pr_err("testfifo: failed to allocate device number\n");
		return ret;
	}
	pr_info("testfifo: allocated %d:%d\n", MAJOR(dev_num), MINOR(dev_num));

	// allocate fixed arrray with kmalloc: pinned in RAM, non-swappable
	// note: kmalloc flags
	fifo_buf = kmalloc(FIFO_BUF_SIZE, GFP_KERNEL);
	if (!fifo_buf) {
		pr_err("testfifo: failed to allocate buffer\n");
		unregister_chrdev_region(dev_num, 1);
		return -ENOMEM;
	}
	pr_info("testfifo: buffer allocated (%d bytes)\n", FIFO_BUF_SIZE);

	// wire file_operations into the cdev, then register it with our device
	// number
	cdev_init(&testfifo_cdev, &testfifo_fops);
	ret = cdev_add(&testfifo_cdev, dev_num, 1);
	if (ret < 0) {
		pr_err("testfifo: failed to add cdev\n");
		kfree(fifo_buf);
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}
	pr_info("testfifo: cdev registered\n");

	// register a class under /sys/class/testfifo
	// udev will see inotify on sysfs and call mknod to make dev/testfifo
	testfifo_class = class_create("testfifo");
	if (IS_ERR(testfifo_class)) {
		pr_err("testfifo: failed to create class\n");
		kfree(fifo_buf);
		cdev_del(&testfifo_cdev);
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(testfifo_class);
	}

	// triggers udev to create /dev/testfifo
	testfifo_device =
	    device_create(testfifo_class, NULL, dev_num, NULL, "testfifo");
	if (IS_ERR(testfifo_device)) {
		pr_err("testfifo: failed to create device\n");
		kfree(fifo_buf);
		class_destroy(testfifo_class);
		cdev_del(&testfifo_cdev);
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(testfifo_device);
	}

	pr_info("testfifo: /dev/testfifo ready\n");
	return 0;
}

static void __exit testfifo_exit(void)
{
	// tear down in reverse order of init,
	// .owner ref count>0 forbids calling rmmod and this exit
	device_destroy(testfifo_class, dev_num);
	class_destroy(testfifo_class);
	kfree(fifo_buf);
	cdev_del(&testfifo_cdev);
	unregister_chrdev_region(dev_num, 1);
	pr_info("testfifo: unloaded\n");
}

module_init(testfifo_init);
module_exit(testfifo_exit);
