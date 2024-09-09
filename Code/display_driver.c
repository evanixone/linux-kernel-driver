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
#include <linux/delay.h>
#include "font.h"

#define DRIVER_NAME "display_device"
#define DRIVER_CLASS "displayClass"
#define I2C_BUS_AVAILABLE 1
#define SLAVE_DEVICE_NAME "sh1107"
#define SLAVE_ADDRESS 0x3c
#define SH1107_MAX_SEG 128
#define SH1107_MAX_LINE 15

//SH1107 Functionality
static int SH1107_DisplayInit(void);
static void SH1107_Fill(unsigned char data);
static void SH1107_Write(bool is_cmd, unsigned char data);
static void SH1107_SetCursor( uint8_t lineNo, uint8_t cursorPos);
static void SH1107_String(unsigned char *str);
static void SH1107_PrintChar( unsigned char c );
static void SH1107_GoToNextLine( void );
static void SH1107_ReturnToFirstSegment( void );
static int I2C_Write(unsigned char *buf, unsigned int len);
static int I2C_Read(void);

//Variable to store Line Number and Cursor Position.
static uint8_t SH1107_LineNum   = 0;
static uint8_t SH1107_CursorPos = 0;
static uint8_t SH1107_FontSize  = SH1107_DEF_FONT_SIZE;
static uint8_t SH1107_DisplayBuffer[SH1107_MAX_SEG];

//Character Device Variables
static dev_t displayDeviceNr;
static struct class *displayClass;
static struct cdev displayDevice;

//I2C Device Variables
static struct i2c_adapter *display_i2c_adapter = NULL;
static struct i2c_client *display_i2c_client = NULL;
static const struct i2c_device_id device_id[] = {
    {SLAVE_DEVICE_NAME, 1}, {}};
static struct i2c_driver display_driver = {
    .driver = {.name = SLAVE_DEVICE_NAME, .owner = THIS_MODULE},
    };
static struct i2c_board_info i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_ADDRESS)
    };

//Character Device Function
static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);


static const struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .open = dev_open,
        .release = dev_release,
        .read = dev_read,
        .write = dev_write};

//SH1107 Functions
static int SH1107_DisplayInit(void)
{
    msleep(100);
    /**
     * Commands to initialize the SH1107 OLED Display
    */
    
    SH1107_Write(true, 0xAE); // Entire Display OFF
    SH1107_Write(true, 0xA0); // Segment Re-Map
    SH1107_Write(true, 0xC0); // Common Output Scan Direction
    SH1107_Write(true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
    SH1107_Write(true, 0xA6); // Set Memory addressing Mode Horizontal
    SH1107_Write(true, 0xAF); // Display ON in normal mode

    SH1107_Fill(0x00);
    return 0;
}

static void SH1107_Fill(unsigned char data)
{
    unsigned int pages = 16;
    unsigned int columns = 128;
    unsigned char commandData;
    
    for (unsigned int page = 0; page < pages; page++) {
        commandData = 0xB0 | page; // Set page command
        SH1107_Write(true, commandData);
        SH1107_Write(true, 0x00); // Set column lower nibble
        SH1107_Write(true, 0x10); // Set column higher nibble
        for (unsigned int column = 0; column < columns; column++) {
            SH1107_Write(false, data); // Write data to display
        }
    }
}


static void SH1107_Write(bool is_cmd, unsigned char data)
{
    unsigned char buf[2] = {0};
    int ret;

    if( is_cmd == true )
    {
      buf[0] = 0x00;
    }
    else
    {
      buf[0] = 0x40;
    }

    buf[1] = data;

    ret = I2C_Write(buf, 2);
}

static void SH1107_SetCursor( uint8_t lineNo, uint8_t cursorPos )
{
    unsigned char commandData;
    if((lineNo <= SH1107_MAX_LINE) && (cursorPos < SH1107_MAX_SEG))
    {
        SH1107_LineNum   = lineNo;
        SH1107_CursorPos = cursorPos;
        
        commandData = 0xB0 | lineNo;
        SH1107_Write(true, commandData);
        
        commandData = 0xD3;
        SH1107_Write(true, commandData);
        commandData = 0x00 | cursorPos;
        SH1107_Write(true, cursorPos);
    }
}

static void SH1107_String(unsigned char *str)
{
    while(*str)
    {
        SH1107_PrintChar(*str++);
    }
}

static void SH1107_PrintChar(unsigned char c)
{
    uint8_t data_byte;
    uint8_t temp = 0;

    if( (( SH1107_CursorPos + SH1107_FontSize ) >= SH1107_MAX_SEG ) || ( c == '\n' ))
    {
        SH1107_GoToNextLine();
    }
    else if (c == '\b')
    {
        c -= 0x20;
        uint8_t tempCursorPos = SH1107_CursorPos;
        SH1107_ReturnToFirstSegment();
        SH1107_ReturnToFirstSegment();
        for(int i = 0; i < tempCursorPos - SH1107_FontSize - 2; i ++){
            SH1107_Write(false, SH1107_DisplayBuffer[i]);
            SH1107_CursorPos++;
        }
        SH1107_Write(false, 0x00);
        SH1107_CursorPos++;
        return;
    }
    
    if( c != '\n' )
    {
        c -= 0x20;
        do
        {
          data_byte= SH1107_font[c][temp];
          SH1107_DisplayBuffer[SH1107_CursorPos] = data_byte;

          SH1107_Write(false, data_byte);
          SH1107_CursorPos++;
          
          temp++;
          
        } while ( temp < SH1107_FontSize);
        SH1107_Write(false, 0x00);
        SH1107_CursorPos++;
    }
}

static void SH1107_ReturnToFirstSegment( void )
{
    unsigned int columns = 128 - SH1107_CursorPos;
    
    for (unsigned int column = 0; column < columns; column++) {
            SH1107_Write(false, 0x00);
        }    
    SH1107_SetCursor(SH1107_LineNum,0);
}

static void SH1107_GoToNextLine( void )
{
    SH1107_LineNum++;
    SH1107_LineNum = (SH1107_LineNum & SH1107_MAX_LINE);
    SH1107_ReturnToFirstSegment();
}

//I2C Read and Write Function

static int I2C_Write(unsigned char *buf, unsigned int len)
{
    int ret = i2c_master_send(display_i2c_client, buf, len);
    return ret;
}

 
static int I2C_Read( void )
{
  int ret = i2c_smbus_read_byte(display_i2c_client);
  return ret;
}

//Device Main Drivers Function
static int dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init dev_init(void)
{
    struct device *dev;
    int ret = -1;
    
    /**
     * CHARACTER DEVICE CONFIGURATION
     */ 
    ret = alloc_chrdev_region(&displayDeviceNr, 2, 2, DRIVER_NAME);
    if (ret < 0) goto out;

    cdev_init(&displayDevice, &fops);

    displayDevice.owner = THIS_MODULE;
    ret = cdev_add(&displayDevice, displayDeviceNr, 2);
    if (ret <0) goto unreg_chrdev;

    displayClass = class_create(THIS_MODULE, DRIVER_CLASS);
    if(IS_ERR(displayClass))
    {
        ret = PTR_ERR(displayClass);
        goto cdev_del;
    }
    displayClass->dev_uevent = dev_uevent;

    dev = device_create(displayClass, NULL, displayDeviceNr, NULL, DRIVER_NAME);
    if (IS_ERR(displayClass))
    {
        ret = PTR_ERR(displayClass);
        goto class_del;
    }
    
    /**
     * I2C DEVICE CONFIGURATION
     */ 
     
    display_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if(IS_ERR(display_i2c_adapter))
    {
        ret = PTR_ERR(display_i2c_adapter);
        goto dev_del;
    }

    display_i2c_client = i2c_new_client_device(display_i2c_adapter, &i2c_board_info);
    if(IS_ERR(display_i2c_client))
    {
        ret = PTR_ERR(display_i2c_client);
        goto i2c_dev_unreg;
    }

    ret = i2c_add_driver(&display_driver);
    if (ret < 0) goto i2c_dev_del;

    i2c_put_adapter(display_i2c_adapter);

    ret = 0;
    
    /**
     * SH1107 Initialisation
     */
     
    SH1107_DisplayInit();
    SH1107_SetCursor(0,0);
      
    return ret;

i2c_dev_unreg:
    i2c_unregister_device(display_i2c_client);
i2c_dev_del:
    i2c_del_driver(&display_driver);
dev_del:
    device_destroy(displayClass, displayDeviceNr);
class_del:
    class_destroy(displayClass);
cdev_del:
    cdev_del(&displayDevice);
unreg_chrdev:
    unregister_chrdev_region(displayDeviceNr, 1);
out:
    return ret;
}

static void __exit dev_exit(void)
{
    SH1107_Fill(0x00);

    SH1107_Write(true, 0xAE); // Entire Display OFF

    pr_info("OLED Removed!!!\n");
    i2c_unregister_device(display_i2c_client);
    i2c_del_driver(&display_driver);
    device_destroy(displayClass, displayDeviceNr);
    class_destroy(displayClass);
    cdev_del(&displayDevice);
    unregister_chrdev_region(displayDeviceNr, 1);
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

    data_buf[max_len - 1] = '\0';
    printk(KERN_INFO "Device write: %s\n", data_buf);
    SH1107_String(data_buf);

    return count;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int to_copy, not_copied;
    size_t len;
    char out[20];
    u8 b1;
    
    b1 = I2C_Read();
    
    len = sprintf(out, "%d", b1);
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
MODULE_DESCRIPTION("TMP102 Driver");
MODULE_VERSION("1.0");
