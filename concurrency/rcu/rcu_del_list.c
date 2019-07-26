    ...
    ...
    ...

struct el {
    struct list_head lp;
    long key;
    spinlock_t mutex;
    int data;
    //other data fields
};

DEFINE_SPINLOCK(listmutex);
LIST_HEAD(head);

//read
int search(long key, int *result)
{
    struct el *p;

    rcu_read_lock();
    list_for_each_entry_rcu(p, &head, lp) {
        if (p->key == key) {
            *result = p->data;
            rcu_read_unlock();
            return 1;
        }
    }
    rcu_read_unlock();
    return 0;
}

//write
int delete(long key)
{
    struct el *p;

    spin_lock(&listmutex);
    list_for_each_entry(p, &head, lp) {
        if (p->key == key) {
            list_del_rcu(&p->lp);
            spin_unlock(&listmutex);
            synchronize_rcu();
            kfree(p);
            return 1;
        }
    }
    spin_unlock(&listmutex);
    return 0;
}



