#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>
asmlinkage long sys_cs3753_add(int num1, int num2, int __user *result){
	int user_result = 0;	
	//printk(KERN_ALERT "Local Variables initialized\n");
	//printk(KERN_ALERT "Got parameter %d and %d\n", num1, num2);

	user_result = num1 + num2;
	printk(KERN_ALERT "Sum of %d and %d is %d\n", num1, num2, user_result);
	put_user(user_result, result);
	//printk(KERN_ALERT "Put result pointer = %p\n", result);
	return 0;
}

