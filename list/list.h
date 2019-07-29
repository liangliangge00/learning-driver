#ifndef __LIST_H
#define __LIST_H



struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name)	{ &(name), &(name)}

void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

#define offsetof(type, member) ((unsigned long) &((type *)0)->member)

#define container_of(ptr, type, member)	({		\
	void *__mptr = (void *)(ptr);		\
	((type *)(__mptr - offsetof(type, member)));})


#define list_entry(ptr, type, member)	\
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member)	\
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member)	\
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member)			\
	for(pos = list_first_entry(head, typeof(*pos), member);		\
		&pos->member != (head);						\
		pos = list_next_entry(pos, member))

#endif
