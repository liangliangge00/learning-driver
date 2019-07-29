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


/**
 *	list_for_first_entry - get the first element from a list
 *	@ptr:	 the list head to take the element from.
 *	@type:	 the type of the struct this is embedded in.
 *	@member: the name of the list_head within the struct.
 */
#define list_first_entry(ptr, type, member)	\
	list_entry((ptr)->next, type, member)


/**
 *	list_for_last_entry - get the last element from a list
 *	@ptr:	 the list head to take the element from.
 *	@type:	 the type of the struct this is embedded in.
 *	@member: the name of the list_head within the struct.
 */
#define list_last_entry(ptr, type, member)	\
	list_entry((ptr)->prev, type, member)


/**
 *	list_for_next_entry - get the next element from in list
 *	@pos:	 the type * to cursor.
 *	@member: the name of the list_head within the struct.
 */
#define list_next_entry(pos, member)	\
	list_entry((pos)->member.next, typeof(*(pos)), member)


/**
 *	list_for_prev_entry - get the prev element from in list
 *	@pos:	 the type * to cursor.
 *	@member: the name of the list_head within the struct.
 */
#define list_prev_entry(pos, member)	\
	list_entry((pos)->member.prev, typeof(*(pos)), member)


/**
 *	list_for_each_entry - iterate over list of given type
 *	@pos:	 the type * to use as a loop cursor.
 *	@head:	 the head for your list.
 *	@member: the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, member)			\
	for(pos = list_first_entry(head, typeof(*pos), member);		\
		&pos->member != (head);						\
		pos = list_next_entry(pos, member))


/**
 *	list_for_each_entry_reverse - iterate backwards over list of given type
 *	@pos:	 the type * to use as a loop cursor.
 *	@head:	 the head for your list.
 *	@member: the name of the list_head within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)		\
	for(pos = list_last_entry(head, typeof(*pos), member);	\
		&pos->member != (head);				\
		pos = list_prev_entry(pos, member))


#endif

