    ...
    ...
    ...

//write
struct foo {
    struct list_head list;
    int a;
    int b;
    int c;
};

LIST_HEAD(head);

    ...

p = kmalloc(sizeof(*p), GFP_KERNEL);
p->a = 1;
p->b = 2;
p->c = 3;
list_add_rcu(&p->list, &head);

//read

rcu_read_lock();
list_for_each_entry_rcu(p, head, list) {
    do_something_with(p->a, p->b, p->c);
}
rcu_read_unlock();

