#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/err.h>

#define DRIVER_NAME "temp_device"
#define DRIVER_CLASS "tempClass"
#define I2C_BUS_AVAILABLE 1
#define SLAVE_DEVICE_NAME "tmp102"
#define SLAVE_ADDRESS 0x48

static struct i2c_adapter *temp_i2c_adapter = NULL;
static struct i2c_client *temp_i2c_client = NULL;
static const struct i2c_device_id device_id[] = {{SLAVE_DEVICE_NAME, 0},{}};
static struct i2c_driver temp_driver = {.driver = {.name = SLAVE_DEVICE_NAME, .owner = THIS_MODULE}};
static struct i2c_board_info i2c_board_info = {I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_ADDRESS)};

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);

static dev_t tempDeviceNr;
static struct class *tempClass;
static struct cdev tempDevice;

static const struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .open = dev_open,
        .release = dev_release,
        .read = dev_read,
        .write = dev_write};

static int dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}


static int __init dev_init(void)
{
    dev_t dev;
    int ret = -1;

    // alloc chardev region (major, minor numbers)
    ret = alloc_chrdev_region(&tempDeviceNr, 0, 1, DRIVER_NAME);
    if (ret < 0) goto out; // alloc failed

    // init char device
    cdev_init(&tempDevice, &fops);

    // register device
    tempDevice.owner = THIS_MODULE;
    ret = cdev_add(&tempDevice, tempDeviceNr, 1);
    if (ret <0) goto unreg_chrdev; // cdev add failed

    // create class in /sys/class
    tempClass = class_create(THIS_MODULE, DRIVER_CLASS);
    if(IS_ERR(tempClass))
    {
        ret = PTR_ERR(tempClass);
        goto cdev_del;
    }
    tempClass->dev_uevent = dev_uevent;

    // populate sysfs with device info
    dev = device_create(tempClass, NULL, tempDeviceNr, NULL, DRIVER_NAME);
    if (IS_ERR(tempClass))
    {
        ret = PTR_ERR(tempClass);
        goto class_del;
    }

    /**
     * 1. get available adapter OR create one using 'i2c_add_adapter'
    */
    temp_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if(IS_ERR(temp_i2c_adapter))
    {
        ret = PTR_ERR(temp_i2c_adapter);
        goto dev_del;
    }

    /**
     * 2. add i2c device
    */
    temp_i2c_client = i2c_new_client_device(temp_i2c_adapter, &i2c_board_info);
    if(IS_ERR(temp_i2c_client))
    {
        ret = PTR_ERR(temp_i2c_client);
        goto dev_del;
    }

    /**
     * 3. add driver to the subsystem
    */
    ret = i2c_add_driver(&temp_driver);
    if (ret < 0) goto i2c_dev_del;

    /**
     * 4. release associated memory of adapter
    */
    i2c_put_adapter(temp_i2c_adapter);

    ret = 0;
    return ret;

i2c_dev_unreg:
    i2c_unregister_device(temp_i2c_client);
i2c_dev_del:
    i2c_del_driver(&temp_driver);
dev_del:
    device_destroy(tempClass, tempDeviceNr);
class_del:
    class_destroy(tempClass);
cdev_del:
    cdev_del(&tempDevice);
unreg_chrdev:
    unregister_chrdev_region(tempDeviceNr, 1);
out:
    return ret;
}

static void __exit dev_exit(void)
{
    i2c_unregister_device(temp_i2c_client);
    i2c_del_driver(&temp_driver);
    device_destroy(tempClass, tempDeviceNr);
    class_destroy(tempClass);
    cdev_del(&tempDevice);
    unregister_chrdev_region(tempDeviceNr, 1);
    return;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    size_t max_len = 30, not_copied;
    uint8_t data_buf[max_len];
    if (count < max_len)
        max_len = count;

    not_copied = copy_from_user(data_buf, buf, max_len);
    if (not_copied == 0)
        printk(KERN_INFO "Copied %zd bytes from user\n", max_len);
    else
        printk(KERN_INFO "Could not copy %zd bytes from user\n", not_copied);

    data_buf[max_len - 1] = 0;
    printk(KERN_INFO "Device write: %s\n", data_buf);
    return count;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int to_copy, not_copied;
    int temp;
    size_t len;
    char out[20];
    u8 b1, b2;

    i2c_smbus_write_byte(temp_i2c_client, 0x00);
    b1 = i2c_smbus_read_byte(temp_i2c_client);
    b2 = i2c_smbus_read_byte(temp_i2c_client);

    temp = (u16)(b1 << 8 | b2 ) >> 4;
    int temp_integer = temp * 625 / 10000;  // Integer part of the temperature
    int temp_decimal = temp * 625 - temp_integer * 10000;  // Decimal part of the temperature

    printk("TEMP: %d.%02d\n", temp_integer, temp_decimal);
    len = sprintf(out, "%d.%02d", temp_integer, temp_decimal);
    to_copy = min(len, count);
    printk("out: %s\n", out);
    not_copied = copy_to_user(buf, out, to_copy);
    printk("READ: %d\n", not_copied);
    return (to_copy - not_copied);
}


static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device open\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device close\n");
    return 0;
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nik Amir Syazwan Bin Abdul Aziz");
MODULE_DESCRIPTION("SH1107 Driver");
MODULE_VERSION("1.0");
