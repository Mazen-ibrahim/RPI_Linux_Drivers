#include<linux/module.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include<linux/fs.h> //For Registartion of file
#include<linux/cdev.h>



#define Major_Num   262

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mazen Ibrahim");
MODULE_DESCRIPTION("Device Number Practice");

/*Define Variables for Device and Device Class*/
#define Driver_Name  "Dynamic_Driver"
#define Driver_Class  "Dynamic_Class"
static dev_t device_nr;
static struct class* myClass;
static struct cdev my_device;




/* Create Buffer to Hold Data */
#define Buffer_Size   255
static char Buffer[Buffer_Size];
static int Buffer_pointer;

/**
 * @brief This Function is Called When want to Read from File Created
*/
static ssize_t driver_read(struct file* instance , char* user_Buffer , size_t count , loff_t* offs)
{    
       int to_copy , not_copied , delta;

       /*Get amount of Data to Copy*/
       to_copy = min(count,Buffer_pointer);

       /*Copy Data to User*/
       not_copied = copy_to_user(user_Buffer, Buffer,to_copy);

       /*Calculate delta*/
       delta = to_copy - not_copied;

       return delta;
    

}

/**
 * @brief This Function is called when want to Write to File Created
*/

static ssize_t driver_write(struct file* instance , const char* user_Buffer , size_t count , loff_t* offs)
{
        int to_copy , not_copied , delta;

       /*Get amount of Data to Copy*/
       to_copy = min(count,sizeof(Buffer));

       /*Copy Data to User*/
       not_copied = copy_from_user(Buffer, user_Buffer, to_copy);
       Buffer_pointer = to_copy;

       /*Calculate delta*/
       delta = to_copy - not_copied;

       return delta;

 
}




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
    .release = driver_close,
    .write = driver_write,
    .read = driver_read
};

/**
 * @brief This Function is Called when Module load into Kernel
*/
static int __init ModuleInit(void)
{   
    int retVal;
    printk("Device_Dynamic Practice\n");
     
    /* Allocate Device Number */
    if( alloc_chrdev_region(&device_nr,0,1,Driver_Name) < 0 )
    {

        printk("Device No couldnot allocated\n");
        return -1;
    }

    printk("Sucessfully allocated Device No. with Major: %d  & Minor: %d\n", MAJOR(device_nr),MINOR(device_nr));

    /* Create Device Class */
    if( (myClass = class_create(THIS_MODULE,Driver_Class))  == NULL )
    {
        printk("Device Class cannot Created\n");
        goto ClassErr;     
    }  


    
    /* Create Device File */
    if( device_create(myClass, NULL, device_nr, NULL, Driver_Name) == NULL )
    {
        printk("Cannot create Device File\n");
        goto FileErr;
    }

    /* Initialize device File */
    cdev_init(&my_device , &fops);

    /* Registering Device to Kernel */
    if( cdev_add(&my_device , device_nr, 1) == -1 )
    {
        printk("Registering of Device to Kernel Failed\n");
        goto DeviceErr;
    }

        return 0;

DeviceErr:
    device_destroy(myClass,device_nr);
FileErr:
    class_destroy(myClass);
ClassErr:
   unregister_chrdev(device_nr,Driver_Name);
   return -1;

}

/**
 * @brief This Function is Called when Module Removed from Kernel
*/
static void __exit ModuleExit(void)
{    

    cdev_del(&my_device);
    device_destroy(myClass,device_nr);
    class_destroy(myClass);
    unregister_chrdev(device_nr,Driver_Name);
    printk("Goodbye Device_Number\n");
 
}

module_init(ModuleInit);
module_exit(ModuleExit);