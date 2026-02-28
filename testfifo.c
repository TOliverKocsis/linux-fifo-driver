// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver K");
MODULE_DESCRIPTION("OliversTestFifo");
MODULE_VERSION("0.1");

static dev_t dev_num;
static struct cdev testfifo_cdev;

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
	.owner   = THIS_MODULE,  // ref count of how many processes using this cdev
	.open    = testfifo_open,
	.release = testfifo_release,
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

	// wire file_operations into the cdev, then register it with our device number
	cdev_init(&testfifo_cdev, &testfifo_fops);
	ret = cdev_add(&testfifo_cdev, dev_num, 1);
	if (ret < 0) {
		pr_err("testfifo: failed to add cdev\n");
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}
	pr_info("testfifo: cdev registered\n");

	// TODO: device class + device_create: makes /dev/testfifo appear automatically when module loads

	return 0;
}

static void __exit testfifo_exit(void)
{
	// tear down in reverse order of init,
	// .owner ref count>0 forbids calling rmmod and this exit
	cdev_del(&testfifo_cdev);

	unregister_chrdev_region(dev_num, 1);
	pr_info("testfifo: unloaded\n");
}

module_init(testfifo_init);
module_exit(testfifo_exit);
