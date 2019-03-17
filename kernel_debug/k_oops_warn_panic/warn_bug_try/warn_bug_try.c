/*
 * warn_bug_try.c
 * Simple demo for WARN(), BUG() macros.
 * And panic() if you wish ;-)
 * Kaiwan NB, kaiwanTECH
 */
#include <linux/init.h>
#include <linux/module.h>
#include <asm-generic/bug.h>

MODULE_LICENSE("Dual BSD/GPL");

static int __init warn_bug_try_init(void)
{
	int n=5;
	printk(KERN_ALERT "Hello, world\n");
#if 1
	WARN((1==1), "%d: WARN try :)", n);
#else
	panic("Yea panic-king..\n");
#endif
	return 0;
}

static void __exit warn_bug_try_exit(void)
{
	printk(KERN_ALERT "Goodbye, invoking BUG now...\n");
#if 1
	BUG();  /* Are u sure!? BUG() calls panic() internally,
	           you _will_ need to restart. */
#endif
}

module_init(warn_bug_try_init);
module_exit(warn_bug_try_exit);
