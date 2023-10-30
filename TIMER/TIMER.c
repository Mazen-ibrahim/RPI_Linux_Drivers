#include<linux/module.h>
#include<linux/fs.h>  
#include<linux/cdev.h>
#include<linux/gpio.h>
#include<linux/timer.h>
#include<linux/jiffies.h>


#define LED_PIN      4
#define Major_Num    262

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mazen Ibrahim");
MODULE_DESCRIPTION("TIMER Driver");

/* Define Variables for Device and Device Class */
#define Driver_Name  "GPIO_Driver"
#define Driver_Class  "GPIO_Class"
static dev_t device_nr;
static struct class* myClass;
static struct cdev my_device;


/* Initialize Variable for Timer */
static struct timer_list timer ;

/* Timer CallBack Function */
void timer_notify_function(struct timer_list *data)
{
    static unsigned char state = 0;
    state ^= 1<<0;
    gpio_set_value(LED_PIN,state);
}



/**
 * @brief This Function is Called When want to Read from File Created
*/
static ssize_t driver_read(struct file* instance , char* user_Buffer , size_t count , loff_t* offs)
{    
       int to_copy , not_copied , delta;
        
       /*Define Buffer*/
       char temp[3] = "\n";

       /*Get amount of Data to Copy*/
       to_copy = min(count,sizeof(temp));

       /*Copy Data to User*/
       not_copied = copy_to_user(user_Buffer, temp,to_copy);

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
       
       /*Define Buffer*/
       char temp[3] = "\n";

       /*Get amount of Data to Copy*/
       to_copy = min(count,sizeof(temp));

       /*Copy Data to User*/
       not_copied = copy_from_user(temp, user_Buffer, to_copy);

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
    
    /* Request Pin 4 for RPI */
    if( gpio_request(LED_PIN,"LED_PIN") )
    {
       printk("Request GPIO LED Pin is Failed\n");
       goto DeviceErr;
        
    }

    /* Set Pin Direction */
    if( gpio_direction_output(LED_PIN,0) )
    {
        printk("Failed to Set LED Pin Direction\n");
        goto LEDERR;
    }

    /* Turn on Led */
    gpio_set_value(LED_PIN,1);

    /* Initialize Timer*/
    timer_setup(&timer,timer_notify_function,0);
    mod_timer( &timer, jiffies + msecs_to_jiffies(1000) );


    return 0;



LEDERR:
    gpio_free(LED_PIN);
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
    del_timer(&timer);
    gpio_set_value(LED_PIN,0);
    gpio_free(LED_PIN);
    cdev_del(&my_device);
    device_destroy(myClass,device_nr);
    class_destroy(myClass);
    unregister_chrdev(device_nr,Driver_Name);
    printk("Goodbye Device_Number\n");
 
}

module_init(ModuleInit);
module_exit(ModuleExit);