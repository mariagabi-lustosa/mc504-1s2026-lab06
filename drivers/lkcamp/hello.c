// SPDX-License-Identifier: GPL-2.0-only
/*
*  hello.c - The simplest kernel module.
*/
#include        <linux/init.h>          /* Needed for __init/__exit */
#include        <linux/module.h>        /* Needed for module_init/module_exit/MODULE_* */
#include        <linux/fs.h>            /* Needed for alloc_chrdev_region/unregister_chrdev_region */
#include        <linux/cdev.h>          /* Needed for cdev_init/cdev_add/cdev_del */
#include        <linux/uaccess.h>       /* Needed for copy_from_user */
#include        <linux/string.h>        /* Needed for strlen */

int myint = -1;
module_param(myint, int, 0);

static dev_t lkcamp_dev;
static struct cdev lkcamp_cdev;

enum driver_state {
        STATUS_OFF = 0,
        STATUS_ON  = 1,
};

static enum driver_state status = STATUS_OFF;
static const char *status_strings[] = {"OFF\n", "ON\n"};

static ssize_t lkcamp_read(struct file *file, char __user *buf, size_t size,
                loff_t *ppos)
{
        return simple_read_from_buffer(buf, size, ppos, status_strings[status],
                        strlen(status_strings[status]));
}

static ssize_t lkcamp_write(struct file *file, const char __user *buf,
                size_t size, loff_t *ppos)
{
        char value;

        if (copy_from_user(&value, buf, 1))
                return -EFAULT;

        if (value == '0')
                status = STATUS_OFF;
        else if (value == '1')
                status = STATUS_ON;
        else
                return -EINVAL;

        return 1;
}

static struct file_operations lkcamp_fops = {
        .read = lkcamp_read,
        .write = lkcamp_write,
};

static int __init hello_init(void)
{
        int ret;

        pr_info("Hello world\n");
        pr_info("I received the following argument: myint=%d\n", myint);
        pr_info("Initialize with the value: %d\n", myint);

        ret = alloc_chrdev_region(&lkcamp_dev, 0, 1, "lkcamp");
        if (ret)
                pr_err("Failed to allocate device number\n");

        cdev_init(&lkcamp_cdev, &lkcamp_fops);

        ret = cdev_add(&lkcamp_cdev, lkcamp_dev, 1);
        if (ret)
                pr_err("Char device registration failed\n");

        return 0; // A non 0 return means init_module failed; module can't be loaded.
}

static void __exit hello_exit(void)
{
        cdev_del(&lkcamp_cdev);
        unregister_chrdev_region(lkcamp_dev, 1);
        pr_info("Goodbye world\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Your Name <your.email@domain.com>");
MODULE_DESCRIPTION("A simple hello / goodbye driver");
MODULE_LICENSE("GPL");