	...
	...
	...

static ssize_t xxx_write(struct file *filp, const char *buffer, size_t count, loff_t *ppos)
{
	...
	DECLARE_WAITQUEUE(wait, current);	/* Define wait queue element */
	add_wait_queue(&xxx_wait, &wait);	/* add a new element to wait queue */

	/* wait for the devices buffer to be writable */
	do {
		avail = device_writable(...);
		if (avail < 0) {
			if (filp->f_flags & O_NONBLOCK) {	/* no block process */
				ret = -EAGAIN;		/*try again*/
				goto out;
			}
			/* block process */
			__set_current_state(TASK_INTERRUPTIBLE);	/* change process status */
			schedule();					/* scheduling other processes */
			if (signal_pending(current)) {	/* if it is a signal wake up */
				ret = -ERESTARTSYS;
				goto out;
			}
		}
	} while (avail < 0);

	/* devices buffer to be writable */
	device_write(...);
out:
	remove_wait_queue(&xxx_wait, &wait);	/* remove element for the wait queue */
	set_current_state(TASK_RUNNING);		/* change process state to TASK_RUNNING */
	return ret;
}


