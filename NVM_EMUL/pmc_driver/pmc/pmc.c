#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/major.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
//#include <asm/special_insns.h>
#include <asm/msr.h>
#include <asm/uaccess.h> /* copy_from/to_user */

static int pmc_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

struct file_operations pmc_fops = {
	.unlocked_ioctl = pmc_ioctl,
	.compat_ioctl = pmc_ioctl,
};

const int PERFCTR0 = 0xc1;
const int PERFEVENTSEL0 = 0x186;
//const int PERFEVENTSEL0 = 0x38;
static int PMC_MAJOR = 256;

int pmc_init_module(void)
{
	int a;
	unsigned long cr4reg;
	int pmc_major;

	if ((pmc_major = register_chrdev(PMC_MAJOR, "pmc", &pmc_fops)) == -EBUSY) {
		printk(KERN_INFO "unable to get major for pmc device\n");
		return -EIO;
	}


	/*
	 * In order to use the rdpmc instruction in user mode, we need to set the
	 * PCE bit of CR4. PCE is 8th bit of cr4, and 256 is 2 << 8
	 */
 	printk(KERN_INFO "<4> Setting the PCE bit \n");

	cr4reg = read_cr4();
	cr4reg |= 0x100; // setting the PCE bit
	write_cr4(cr4reg);

	return 0;
}	

void pmc_exit_module(void) {
	/* Freeing the major number */
	unregister_chrdev(PMC_MAJOR, "pmc");
}	


/* 
 * pmc_clear clears the PMC specified by counter
 * counter = 0 => perfctr0
 * counter = 1 => perfctr1
 * it uses WRMSR to write the values in the counters
 */
static void pmc_clear(int counter) {
	int counterRegister = PERFCTR0 + counter;
	/* clear the old register */

	__asm__ __volatile__("mov %0, %%ecx\n\t"
	        "xor %%edx, %%edx\n\t"
            "xor %%eax, %%eax\n\t"
            "wrmsr\n\t"
	        : /* no outputs */
	        : "m" (counterRegister)
	        : "eax", "ecx", "edx" /* all clobbered */);
}



/* 
 * This function writes the value specified by the arg to the counter
 * indicated by counter 
 */

static void setCounter(int counter, unsigned long arg) 
{
	if ((counter < 0) || (counter > 3)) {
		printk(KERN_INFO "pmc:setCounter illegal value for counter\n");
	} else {
		int selectionRegister = PERFEVENTSEL0 + counter;
		int counterRegister = PERFCTR0 + counter;
		pmc_clear(counter);

		/* set the value */

		__asm__ __volatile__("mov %0, %%ecx\n\t" /* ecx contains the number of the MSR to set */
		        "xor %%edx, %%edx\n\t"/* edx contains the high bits to set the MSR to */
		        "mov %1, %%eax\n\t" /* eax contains the low bits to set the MSR to */
		        "wrmsr\n\t"
		        : /* no outputs */
		        : "m" (selectionRegister), "m" (arg)
		        : "eax", "ecx", "edx" /* clobbered */);
	}
}



static int pmc_ioctl(struct file *f, unsigned int cmd, unsigned long arg) 
{
	int a;
	uint32_t low;
	uint32_t high;
	
	printk(KERN_INFO "pmc_ioctl: command=%d\n", cmd);
	printk(KERN_ALERT "pmc_ioctl: command=%d\n", cmd);

	cmd = cmd%10;
	switch (cmd) {
		case 0:
			/* disable counter */
			setCounter(0, 0);
			pmc_clear(0);
			/* set counter 0 */
			setCounter(0, arg);
			break;
		case 1:
			/* disable counter */
			setCounter(1, 0);
			pmc_clear(1);
			/* set counter 1 */
			setCounter(1, arg);
			break;
		case 2:
			/* disable counter */
			setCounter(2, 0);
			pmc_clear(2);
			/* set counter 1 */
			setCounter(2, arg);
			break;
		case 23:
			/* disable counter */
			setCounter(3, 0);
			pmc_clear(3);
			/* set counter 1 */
			setCounter(3, arg);
			break;

		default:
			printk(KERN_INFO "pmc_ioctl: illegal cmd: %d\n", cmd);
			break;
	}
	return 0;
}


/* Declaration of the init and exit functions */
module_init(pmc_init_module);
module_exit(pmc_exit_module);

