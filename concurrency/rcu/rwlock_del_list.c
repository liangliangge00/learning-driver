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

DEFINE_RWLOCK(listmutex);
LIST_HEAD(head);

//read
int search(long key, int *result)
{
    struct el *p;
    
    read_lock(&listmutex);
    list_for_each_entry(p, &head, lp) {
        if (p->key == key) {
            *result = p->data;
            read_unlock(&listmutex);
            return 1;
        }
    }

    read_unlock(&listmutex);
    return 0;
}

//write
int delete(long key)
{
    struct el *p;

    write_lock(&listmutex);
    list_for_each_entry(p, &head, lp) {
        if (p->key == key) {
            list_del(&p->lp);
            write_unlock(&listmutex);
            
            kfree(p);
            return 1;
        }
    }
    write_unlock(&listmutex);
    return 0;
}


