    ...
    ...
    ...

struct foo {
    struct list_head list;
    int a;
    int b;
    int c;
};
LIST_HEAD(head);

    ...
    ...
    ...

p = search(head, key);
if (p == NULL) {
    /*Take appropriate action,unlock,and return*/
    ...
}
q = kmalloc(sizeof(*p), GFP_KERNEL);
if (q == NULL) {
    return -ENOMEM;
}
*q = *p;
q->b = 2;
q->c = 3;
list_replace_rcu(&p->list, &q->list);
synchronize_rcu();
kfree(p);

