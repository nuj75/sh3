#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos)
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

    result = sprintf(buffer, jiffies / HZ);

    return result;
}

static const struct proc_ops my_proc_ops = {
    .proc_read = proc_read,
};

int proc_start(void)
{
    proc_create("seconds", 0, NULL, &my_proc_ops);

    return 0;
}

int proc_quit(void)
{
    remove_proc_entry("seconds", NULL);
}
module_init(proc_start);
module_exit(proc_exit);
