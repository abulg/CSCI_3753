

#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<linux/slab.h>
#include<linux/uaccess.h>

#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
/* Define device_buffer and other global data structures you will need here */
int opened = 0;
int closed = 0;
struct module *owner;

static char *device_buffer;//[BUFFER_SIZE] = {'\0'};
//static int device_buffer_size;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/* length is the length of the userspace buffer*/
	/* offset will be set to current position of the opened file after read*/
	/* copy_to_user function: source is device_buffer and destination is the userspace buffer *buffer */

	int max_bytes_read;	//max bytes that can be read in the device buffer
	int bytes_to_read;
	int bytes_read;
	max_bytes_read = BUFFER_SIZE - *offset;

	if(max_bytes_read > length){
		bytes_to_read = length;
	}
	else{
		bytes_to_read = max_bytes_read;
	}
	if(bytes_to_read == 0){
		printk(KERN_ALERT "Reached end of device buffer\n");
	}

	bytes_read = bytes_to_read - copy_to_user(buffer, device_buffer + *offset, length);
	printk(KERN_ALERT "Sent %d characters to the user\n", bytes_read);
	*offset += bytes_read;

	return bytes_read;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_from_user function: destination is device_buffer and source is the userspace buffer *buffer */

	int max_bytes_write;	//max bytes that can be read in the device buffer
	int bytes_to_write;
	int bytes_written;
	max_bytes_write = BUFFER_SIZE - *offset;

	if(max_bytes_write > length){
		bytes_to_write = length;
	}
	else{
		bytes_to_write = max_bytes_write;
	}

	bytes_written = bytes_to_write - copy_from_user(device_buffer + *offset, buffer, bytes_to_write);
	printk(KERN_ALERT "Sucessfully written %d bytes\n", bytes_written);
	*offset += bytes_written;
	return bytes_written;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	opened++;
	printk(KERN_ALERT "Char Driver Opened %d times\n", opened);
	return 0;
}

int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	closed++;
	printk(KERN_ALERT "Char Driver Closed %d times\n", closed);
	return 0;
}

loff_t simple_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	/* Update open file position according to the values of offset and whence */
	loff_t newpos = 0;
	switch(whence){
		case SEEK_SET:{	//set seek to position as set by user
			newpos = offset;
			break;
		}
		case SEEK_CUR:{	//set seek to current position + offset
			newpos = pfile->f_pos + offset;
			break;
		}
		case SEEK_END:{	//set seek to offset bytes from the end of device buffer
			newpos = BUFFER_SIZE - offset;
			break;
		}
		default:{

		}
	}

	if(newpos > BUFFER_SIZE){
		newpos = BUFFER_SIZE;
	}
	if(newpos < 0){
		newpos = 0;
	}

	pfile->f_pos = newpos;
	return newpos;
}

struct file_operations simple_char_driver_file_operations = {
	.owner = THIS_MODULE,
	.open = simple_char_driver_open,
	.release = simple_char_driver_close,
	.llseek = simple_char_driver_seek,
	.read = simple_char_driver_read,
	.write = simple_char_driver_write
};


static int __init simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_ALERT "Inside %s function\n", __FUNCTION__);

	device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);	//allocate memory for device buffer on init
	printk("Device buffer address is: %p", device_buffer);
	/* register the device */
	register_chrdev(241, "simple_char_device", &simple_char_driver_file_operations);
	return 0;
}

static void simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	printk(KERN_ALERT "Inside %s function\n", __FUNCTION__);
	kfree(device_buffer);	//free device buffer memory on exit
	/* unregister  the device using the register_chrdev() function. */
	unregister_chrdev(241, "simple_char_device");
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);