#include<linux/module.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include<linux/fs.h>  
#include<linux/cdev.h>
#include<linux/gpio.h>
#include<linux/pwm.h>


//must load pwm module to kernel in boot to allow use pwm API

#define Major_Num    262
/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mazen Ibrahim");
MODULE_DESCRIPTION("PWM Driver");

/*Define Variables for Device and Device Class*/
#define Driver_Name   "PWM_Driver"
#define Driver_Class  "PWM_Class"
static dev_t device_nr;
static struct class* myClass;
static struct cdev my_device;


/* Define Variables for PWM0 */
struct pwm_device *pwm_0 = NULL;
u32 pwm_on_time = 500000000;


/**
 * @brief This Function is called when want to Write to File Created
*/

static ssize_t driver_write(struct file* instance , const char* user_Buffer , size_t count , loff_t* offs)
{
       int to_copy , not_copied , delta;
       
       /*Define Buffer*/
       char value;

       /*Get amount of Data to Copy*/
       to_copy = min(count,sizeof(value));

       /*Copy Data to User*/
       not_copied = copy_from_user(&value, user_Buffer, to_copy);

       /*Calculate delta*/
       delta = to_copy - not_copied;

       if( value < 'a' || value > 'j')
           printk("Invalid Input\n");
       else
           pwm_config(pwm_0,100000000*(value - 'a'),1000000000);

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


    pwm_0 = pwm_request(0,"PWM_0");
    if( pwm_0 == NULL )
    {
        printk("Could not request PWM0\n");
        goto DeviceErr;
    }

    pwm_config(pwm_0,pwm_on_time,1000000000);
    pwm_enable(pwm_0);

    
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
    pwm_disable(pwm_0);
    pwm_free(pwm_0);
    cdev_del(&my_device);
    device_destroy(myClass,device_nr);
    class_destroy(myClass);
    unregister_chrdev(device_nr,Driver_Name);
    printk("Goodbye Device_Number\n");
 
}

module_init(ModuleInit);
module_exit(ModuleExit);