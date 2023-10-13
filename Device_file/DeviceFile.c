#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>

#define Major_Num   262

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mazen Ibrahim");
MODULE_DESCRIPTION("Device Number Practice");

/**
 * @brief This Function is Called when the device file is opened
*/
static int driver_open(struct inode *device_file, struct file* instance)
{
        printk("Dev_nr - open was called");
        return 0;
}

/**
 * @brief This Function is Called when the device file is closed
*/
static int driver_close(struct inode *device_file, struct file* instance)
{
        printk("Dev_nr - close was called");
        return 0;
}

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open  = driver_open,
    .release = driver_close
};

/**
 * @brief This Function is Called when Module load into Kernel
*/
static int __init ModuleInit(void)
{   
    int retVal;

    printk("Device_Number Practice\n");
    
    retVal = register_chrdev(Major_Num,"Device_Number",&fops);
    if( retVal == 0 ) /*This Mean Successfully Take The number*/
    {
        printk("Device_No - Major_No: %d  Minor: %d\n",Major_Num,0);
    } 
    else if ( retVal > 0)
    {
        printk("Device_No - Major_No: %d  Minor: %d\n",retVal>>20 , retVal & 0xffffff);
    }
    else
    {
        printk("Could not register device number\n");
        return -1;
    }

    return 0;


}

/**
 * @brief This Function is Called when Module Removed from Kernel
*/
static void __exit ModuleExit(void)
{    
    unregister_chrdev(Major_Num,"Device_Number");
    printk("Goodbye Device_Number\n");
 
}

module_init(ModuleInit);
module_exit(ModuleExit);