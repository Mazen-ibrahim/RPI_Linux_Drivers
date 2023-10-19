#include<linux/module.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include<linux/fs.h>  
#include<linux/cdev.h>
#include<linux/gpio.h>
#include<linux/delay.h>


/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mazen Ibrahim");
MODULE_DESCRIPTION("LCD Driver");


/* Define Buffers and Pins Needed */
static char lcd_buffer[17];

static char *PIN_NAMES[]=
{
   "Enable_PIN",
   "Register_PIN",
   "Data_PIN_0",
   "Data_PIN_1",
   "Data_PIN_2",
   "Data_PIN_3",
   "Data_PIN_4",
   "Data_PIN_5",
   "Data_PIN_6",
   "Data_PIN_7",

};

unsigned int LCD_PINS[10] = 
{ 
   2,  //Enable Pin 
   3,  //Register Select Pin
   4,  //Data Pin 0
   17, //Data Pin 1
   27, //Data Pin 2
   22, //Data Pin 3
   10, //Data Pin 4
   9,  //Data Pin 5
   11, //Data Pin 6
   0,  //Data Pin 7 

};



#define ENABLE_PIN    LCD_PINS[0]
#define REGISTER_PIN  LCD_PINS[1]


/*Define Variables for Device and Device Class*/
#define Driver_Name  "LCD_Driver"
#define Driver_Class  "LCD_Class"
static dev_t device_nr;
static struct class* myClass;
static struct cdev my_device;


/**
 * @brief generate Pulse on Enable Pin
*/
void lcd_enable(void)
{
  gpio_set_value(ENABLE_PIN,1);
  msleep(5);
  gpio_set_value(ENABLE_PIN,0);

}


/**
 * @brief function used to send one byte of data to lcd
 * @param Data: data to sent
*/
void lcd_send_byte(unsigned char data)
{
  for(int i = 0 ; i<8 ; i++)    
    gpio_set_value( LCD_PINS[i+2], ( data & (1<<i) )>>i );
  
  lcd_enable();
  msleep(5);
  
}

/**
 * @brief send command to lcd
 * @param data: command to send
*/
void lcd_send_command(unsigned char data)
{  
    gpio_set_value(REGISTER_PIN,0);  //mean send command to lcd
    lcd_send_byte(data);
}

/**
 * @brief send data to lcd
 * @param data: data to send
*/
void lcd_send_data(unsigned char data)
{
    gpio_set_value(REGISTER_PIN,1);  //mean send data to lcd
    lcd_send_byte(data);
}


/**
 * @brief send array of bytes to lcd
 * @param pointer to array of char  
*/
void lcd_send_bytes(unsigned char *data)
{
    int i = 0 ;
    while(data[i] != '\0')
    {
        lcd_send_byte(data[i]);
        i++;
    }  
}


/**
 * @brief Initialize lcd
*/

void lcd_Init(void)
{ 
    lcd_send_command(0x30);  //set lcd for 8 bit mode
    lcd_send_command(0xf);   //set display on , cursor on , cursor blinking
    lcd_send_command(0x1);   //Clear lcd display
}

/**
 * @brief This Function is called when want to Write to File Created
*/

static ssize_t driver_write(struct file* instance , const char* user_Buffer , size_t count , loff_t* offs)
{
       int to_copy , not_copied , delta;
       

       /*Get amount of Data to Copy*/
       to_copy = min(count,sizeof(lcd_buffer));

       /*Copy Data to User*/
       not_copied = copy_from_user(lcd_buffer, user_Buffer, to_copy);

       /*Calculate delta*/
       delta = to_copy - not_copied;

       /* send new data to lcd */
       if(delta == 0)
       {
            lcd_send_command(0x1);
            for(int i = 0 ;i < to_copy ; i++)
                lcd_send_data(lcd_buffer[i]);
       }
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
    int retVal , i;
    printk("LCD_Driver Practice\n");
     
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
    
    /* Request Pins for LCD */
    printk("LCD_Driver Initialize\n");
    for(  i = 0 ; i<10 ; i++ )
    {
        if(gpio_request(LCD_PINS[i],PIN_NAMES[i]))
        {
            printk("Request LCD Pin: %s failed\n",PIN_NAMES[i]);
            goto LCD_INIT_ERR;
        }
    }

    /* Set Pins Direction for LCD */
    printk("LCD_Driver Set Pins Direction\n");
    for( i = 0 ; i<10 ; i++)
    {
        if(gpio_direction_output(LCD_PINS[i],0))
        {
            printk("Set LCD DIR: %s failed\n",PIN_NAMES[i]);
            goto LCD_DIR_ERR;
        }
    }


    /*Initialize LCD*/
    lcd_Init();

    /*Send Data to lcd*/
    lcd_send_bytes("Hello World");

    return 0;


LCD_DIR_ERR:
    i = 9; 
LCD_INIT_ERR:
   for(  ; i>=0 ; i--)
    {
        gpio_free(LCD_PINS[i]);
    } 

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
    //Clear lcd
    lcd_send_command(0x1);
    for(int i = 0 ; i<10 ; i++)
    {
        gpio_set_value(LCD_PINS[i],0);
        gpio_free(LCD_PINS[i]);
    } 

    cdev_del(&my_device);
    device_destroy(myClass,device_nr);
    class_destroy(myClass);
    unregister_chrdev(device_nr,Driver_Name);
    printk("Goodbye Device_Number\n");
 
}

module_init(ModuleInit);
module_exit(ModuleExit);