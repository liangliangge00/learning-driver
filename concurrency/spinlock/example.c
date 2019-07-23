    ...
    ...
    ...

int xxx_count = 0;

static int xxx_open(struct inode *inode, struct file *filp)
{
    ...
    spin_lock(&xxx_lock);
    if (xxx_count) {
        spin_unlock(&xxx_lock);
        return -EBUSY;
    }
    xxx_count++;
    spin_unlock(&xxx_lock);
    ...
    
    return 0;
}

static int xxx_release(struct inode *inode, struct file *filp)
{
    ...
    spin_lock(&xxx_lock);
    xxx_count--;
    spin_unlock(&xxx_lock);
    
    return 0;
}

    ...
    ...
    ...
