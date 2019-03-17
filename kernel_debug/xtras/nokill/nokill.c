/*
 * nokill.c
 *
 * Prevent a particular task being killed
 * by setting the SIGNAL_UNKILLABLE flag in signal->flags..
 *
 * /// Simple kernel program that either-
 * /// -returns info about all tasks on the task list
 * /// -returns info reg a particular task (ref by PID passed)
 * 
 * Author: Kaiwan N Billimoria <kaiwan@kaiwantech.com>
 * [L]GPL
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>	/* current, jiffies */
#include <linux/fs.h>		/* no_llseek */
#include <linux/slab.h>
#include <asm/uaccess.h>	/* copy_to_user() */
#include <linux/kallsyms.h>

#define	DRVNAME		"nokill"
#define MY_MAJOR  	0    /* 0 => dynamic major number assignment */
#define MAXKBUF_LEN	20*1024 // 20Kb should do.... ?

#ifdef DEBUG
	#define MSG(string, args...) \
		printk(KERN_DEBUG "%s:%s:%d: " string, \
			DRVNAME, __FUNCTION__, __LINE__, ##args)
#else
	#define MSG(string, args...)
#endif

static int dontkillpid=-1;
//static int get_all=1; // TODO- concurrent safety- use atomic_t

static ssize_t taskinfo_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
	struct task_struct *p;
	char *kbuf, tmp[128];
	int numread=0, num=0;

	if (!(kbuf = kzalloc (MAXKBUF_LEN, GFP_KERNEL))) {
		printk (KERN_WARNING "%s: kmalloc failed", DRVNAME);
		return -ENOMEM;
	}

	for_each_process(p) {

		if (p->pid == dontkillpid)
			p->signal->flags |= SIGNAL_UNKILLABLE;

#if 1
		memset (tmp, 0, 128);
		num = snprintf (tmp, 128, "\
%-16s|%7d|%7d|%7d|%7d||0x%08x\n",
			p->comm, p->tgid, p->pid,
			//p->uid, p->euid
			task_uid(p), task_euid(p), p->signal->flags
		);
		strncat (kbuf, tmp, num);
		numread += num;
#endif
#ifndef CONFIG_PREEMPT
		if (unlikely(test_tsk_thread_flag(current, TIF_NEED_RESCHED))) {
			MSG ("the unlikely occured. scheduling...\n");
			schedule();
		}
#endif
	} // for_each_process loop...

//printk("\n\n%s\n", kbuf);

#if 1
	if (copy_to_user (buf, kbuf, numread)) {
		printk (KERN_ALERT "foreach:copy_to_user failed..\n");
		kfree (kbuf);
		return -EFAULT;
	}
#endif
	kfree (kbuf);
	return count;
}

static ssize_t taskinfo_write(struct file *filp, const char __user *buf, 
		size_t count, loff_t *offp)
{
	MSG( "process %s [pid %d], count=%d\n", 
			current->comm, current->pid, count);
	return -ENOSYS;
}


/* Minor-specific open routines */
static struct file_operations taskinfo_fops = {
	.llseek =	no_llseek,
	.read =		taskinfo_read,	// do this with ioctl..better.
	.write =	taskinfo_write,
};

static int taskinfo_open(struct inode * inode, struct file * filp)
{
	MSG( "Device node with minor # %d being used\n", iminor(inode));

	switch (iminor(inode)) {
		case 0:
			filp->f_op = &taskinfo_fops;
			break;
		default:
			return -ENXIO;
	}
	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp); 
		   /* Minor-specific open : jumps to the 
		   suitable entry point - the correct open() call if 
	      	   one has been defined */
	return 0;
}

/* Major-wide open routine */
static struct file_operations taskinfoopen_fops = {
	.open =		taskinfo_open, /* just a means to get at the real open */
};

/*
 * Register the char driver with the kernel.
 *
 *  On 2.6 kernels, we could use the new alloc_register_chrdev() function; 
 *  here, we use the "classic" register_chrdev() API.
 *	result = register_chrdev( MY_MAJOR, "cz", &cz_fops );
 */
static int major=0;
static int __init taskinfo_init_module(void)
{
	int result;

	if (dontkillpid == -1) {
		printk("%s: must pass valid parameter 'dontkillpid=<PID>' for task PID to not kill\n", DRVNAME);
		return -EINVAL;
	}
	
	/*
	 * Register the major, and accept a dynamic number.
	 * The return value is the actual major # assigned.
	 */
	result = register_chrdev (0, DRVNAME, &taskinfoopen_fops);
	if (result < 0) {
		MSG( "register_chrdev() failed\n");
		return result;
	}
	major = result; /* dynamic */
	MSG( "registered:: major=%d\n", major);

	return 0; /* success */
}

static void __exit taskinfo_cleanup_module(void)
{
	unregister_chrdev(major, DRVNAME);
	MSG("Unregistered.\n");
}

module_init(taskinfo_init_module);
module_exit(taskinfo_cleanup_module);

module_param(dontkillpid, int, 0); /* 0 => param won't show up in sysfs, 
				   non-zero are mode (perms) */
MODULE_PARM_DESC(dontkillpid, "PID of task to set to a 'no kill' state.");
MODULE_AUTHOR("Kaiwan");
MODULE_DESCRIPTION("Simple module that returns task info to userspace");
MODULE_LICENSE("GPL");
