#include        <linux/init.h>          /* Needed for __init/__exit */
#include        <linux/module.h>        /* Needed for module_init/module_exit/MODULE_* */
#include        <linux/fs.h>            /* Needed for alloc_chrdev_region/unregister_chrdev_region */
#include        <linux/cdev.h>          /* Needed for cdev_init/cdev_add/cdev_del */
#include        <linux/uaccess.h>       /* Needed for copy_from_user */
#include        <linux/string.h>        /* Needed for strlen */
#include        "lkcamp_ioctl.h"

#define BUFFER_SIZE 2040

static dev_t lkcamp_dev;
static struct cdev lkcamp_cdev;

static char crypto_buffer[BUFFER_SIZE];
static size_t data_length = 0; // how many bytes are in the buffer
static char crypto_offset = 3; //caesar cipher default offset



static ssize_t lkcamp_read(struct file *file, char __user *buf, size_t size,
                loff_t *ppos)
{
    char temp[BUFFER_SIZE];

    if (*ppos >= data_length) {
        return 0; // EOF
    }

    if (size > data_length - *ppos) {
        size = data_length - *ppos;
    }

    for (int i = 0; i < size; i++) {
        temp[i] = (crypto_buffer[*ppos + i] - crypto_offset);
    }

    if (copy_to_user(buf, temp, size)) {
        return -EFAULT;
    }

    *ppos += size;
    return size;
}

static ssize_t lkcamp_write(struct file *file, const char __user *buf,
                size_t size, loff_t *ppos)
{
    if (size > BUFFER_SIZE) {
        size = BUFFER_SIZE;
    }

    if (copy_from_user(crypto_buffer, buf, size)) {
        return -EFAULT;
    }

    for (int i = 0; i < size; i++) {
        crypto_buffer[i] = (crypto_buffer[i] + crypto_offset);
    }
    data_length = size;
    return size;
}

static long lkcamp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned char key;

    switch (cmd) {
        case LKCAMP_SET_KEY:
            if (copy_from_user(&key, (unsigned char __user *)arg, sizeof(key))) {
                return -EFAULT;
            }
            crypto_offset = key;
            break;

        case LKCAMP_GET_KEY:
            key = crypto_offset;
            if (copy_to_user((unsigned char __user *)arg, &key, sizeof(key))) {
                return -EFAULT;
            }
            break;

        default:
            return -ENOTTY;
    }

    return 0;
}

static struct file_operations lkcamp_fops = {
        .read = lkcamp_read,
        .write = lkcamp_write,
        .unlocked_ioctl = lkcamp_ioctl,
        .llseek = default_llseek,
};

static int __init cipher_init(void)
{
        int ret;

        ret = alloc_chrdev_region(&lkcamp_dev, 0, 1, "cipher");
        if (ret)
                pr_err("Failed to allocate device number\n");

        cdev_init(&lkcamp_cdev, &lkcamp_fops);

        ret = cdev_add(&lkcamp_cdev, lkcamp_dev, 1);
        if (ret)
                pr_err("Char device registration failed\n");

        return 0; // A non 0 return means init_module failed; module can't be loaded.
}

static void __exit cipher_exit(void)
{
        cdev_del(&lkcamp_cdev);
        unregister_chrdev_region(lkcamp_dev, 1);
        pr_info("Cipher driver unloaded\n");
}

module_init(cipher_init);
module_exit(cipher_exit);

MODULE_AUTHOR("Your Name <your.email@domain.com>");
MODULE_DESCRIPTION("A simple cipher driver");
MODULE_LICENSE("GPL");