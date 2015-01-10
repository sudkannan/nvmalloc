#include <linux/init.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/major.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/msr.h>
#include <asm/uaccess.h>
#include "ioctl_query.h"

static long pmc_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

struct file_operations pmc_fops = {
	.unlocked_ioctl = pmc_ioctl,
	.compat_ioctl = pmc_ioctl,
};

static const char* module_name = "nvmemul";
const int PERFCTR0 = 0xc1;
const int PERFEVENTSEL0 = 0x186;
static int NVMEMUL_MAJOR = 256;

int pmc_init_module(void)
{
	unsigned long cr4reg;
	int major;

 	printk(KERN_INFO "%s: Loading. Initializing...\n", module_name);
	if ((major = register_chrdev(NVMEMUL_MAJOR, module_name, &pmc_fops)) == -EBUSY) {
		printk(KERN_INFO "%s: Unable to get major for %s device\n", module_name, module_name);
		return -EIO;
	}

	/*
	 * In order to use the rdpmc instruction in user mode, we need to set the
	 * PCE bit of CR4. PCE is 8th bit of cr4, and 256 is 2 << 8
	 */

	cr4reg = read_cr4();
	cr4reg |= 0x100; // setting the PCE bit
	write_cr4(cr4reg);

	return 0;
}	

void pmc_exit_module(void) {
 	printk(KERN_INFO "%s: Unloading. Cleaning up...\n", module_name);
	/* Freeing the major number */
	unregister_chrdev(NVMEMUL_MAJOR, module_name);
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

static long setCounter(int counter, unsigned long arg) 
{
	if ((counter < 0) || (counter > 3)) {
		printk(KERN_INFO "%s: setCounter illegal value 0x%x for counter\n", module_name, counter);
        return -ENXIO;
	} else {
		int selectionRegister = PERFEVENTSEL0 + counter;
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
    return 0;
}


static long pmc_ioctl_setcounter(struct file* f, unsigned int cmd, unsigned long arg)
{
    long ret;
    ioctl_query_setcounter_t q;
    copy_from_user(&q, (ioctl_query_setcounter_t*) arg, sizeof(ioctl_query_setcounter_t));
    /* disable counter */
    if ((ret = setCounter(q.counter_id, 0)) < 0) {
        return ret;
    }
    pmc_clear(q.counter_id);
	/* set counter */
	setCounter(q.counter_id, q.event_id);
    return 0;
}

static long pmc_ioctl_setpci(struct file* f, unsigned int cmd, unsigned long arg)
{
    ioctl_query_setgetpci_t q;
    struct pci_bus *bus = NULL;

    copy_from_user(&q, (ioctl_query_setgetpci_t*) arg, sizeof(ioctl_query_setgetpci_t));
    //FIXME: What if there are multiple PCI buses?
    while ((bus = pci_find_next_bus(bus))) {
        if (q.bus_id == bus->number) {
            pci_bus_write_config_word(bus, PCI_DEVFN(q.device_id, q.function_id), q.offset, (u16) q.val);
            printk(KERN_INFO "%s: setpci bus_id=0x%x device_id=0x%x, function_id=0x%x, val=0x%x\n", module_name, q.bus_id, q.device_id, q.function_id, q.val); 
            return 0;
        }
    }
    return -ENXIO;
}

static long pmc_ioctl_getpci(struct file* f, unsigned int cmd, unsigned long arg)
{
    ioctl_query_setgetpci_t q;
    struct pci_bus *bus = NULL;

    copy_from_user(&q, (ioctl_query_setgetpci_t*) arg, sizeof(ioctl_query_setgetpci_t));
    while ((bus = pci_find_next_bus(bus))) {
        if (q.bus_id == bus->number) {
            unsigned int val = 0;
            pci_bus_read_config_word(bus, PCI_DEVFN(q.device_id, q.function_id), q.offset, (u16*) &val);
            printk(KERN_INFO "%s: getpci bus_id 0x%x device_id 0x%x, function_id 0x%x, offset 0x%x, val 0x%x\n", module_name, q.bus_id, q.device_id, q.function_id, q.offset, val); 
            q.val = val;
            copy_to_user((ioctl_query_setgetpci_t*) arg, &q, sizeof(ioctl_query_setgetpci_t));
            return 0;
        }
    }
    return -ENXIO;
}

static long pmc_ioctl(struct file *f, unsigned int cmd, unsigned long arg) 
{
    int ret = -1;

	printk(KERN_INFO "%s: ioctl command: 0x%x\n", module_name, cmd);
	switch (cmd) {
		case IOCTL_SETCOUNTER:
            ret = pmc_ioctl_setcounter(f, cmd, arg);
            break;
        case IOCTL_SETPCI:
            ret = pmc_ioctl_setpci(f, cmd, arg);
            break;
        case IOCTL_GETPCI:
            ret = pmc_ioctl_getpci(f, cmd, arg);
            break;
		default:
			printk(KERN_INFO "%s: ioctl illegal command: 0x%x\n", module_name, cmd);
			break;
	}
	return ret;
}


/* Declaration of the init and exit functions */
module_init(pmc_init_module);
module_exit(pmc_exit_module);

