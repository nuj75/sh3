#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

/*
 5.15.0-87-generic
 No LSB modules are available.
 Distributor ID: Ubuntu
 Description: Ubuntu 22.04.3 LTS
 Release: 22.04
 Codename: jammy
*/

static unsigned long start_jiffies; // Global variable to store the start time

/* Function prototypes */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);
int proc_start(void);
void proc_exit(void);

ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
    int result = 0;
    char buffer[128];
    static int complete = 0;

    if (complete)
    {
        complete = 0;
        return 0;
    }

    complete = 1;

    unsigned long elapsed = (jiffies - start_jiffies) / HZ;      // Calculate elapsed seconds since the module was loaded
    result = snprintf(buffer, sizeof(buffer), "%lu\n", elapsed); // Format elapsed seconds into the buffer
    copy_to_user(usr_buf, buffer, result);

    return result;
}

/* Define the proc file operations for the /proc/seconds entry */
static const struct proc_ops my_proc_ops = {
    .proc_read = proc_read,
};

/*
 * proc_start - called when the module is loaded
 *
 * This function records the current value of jiffies to measure elapsed
 * time later and creates the /proc/seconds entry with the specified
 * proc operations.
 */
int proc_start(void)
{
    start_jiffies = jiffies; // Record module load time

    proc_create("seconds", 0, NULL, &my_proc_ops); // Create the /proc/seconds entry
    printk(KERN_INFO "/proc/seconds created\n");
    return 0;
}

/*
 * proc_exit - called when the module is removed
 *
 * This function removes the /proc/seconds entry to clean up the kernel
 * before the module is unloaded.
 */
void proc_exit(void)
{
    remove_proc_entry("seconds", NULL);
    printk(KERN_INFO "/proc/seconds removed\n");
}

module_init(proc_start);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello Module");
MODULE_AUTHOR("SGG");
