#include "list.h"
#include <stdio.h>

struct task_list {
	int val;
	struct list_head my_list;
};

int main(int argc, char *argv[])
{
	int i;
	struct task_list *pos_ptr;	/* cursor */
	struct list_head head_task;	/* list head */
	struct task_list task[10];	/* ten elements for list*/

	INIT_LIST_HEAD(&head_task);	/* init head */
	for (i = 0; i < 10; i++) {
		task[i].val = i+1;
		INIT_LIST_HEAD(&task[i].my_list);
		list_add(&task[i].my_list, &head_task);
	}
/*
	list_for_each_entry_reverse(pos_ptr, &head_task.my_list, my_list) {
		printf(" %d ", pos_ptr->val);	
	}
*/
	list_for_each_entry(pos_ptr, &head_task, my_list) {
		printf(" %d ", pos_ptr->val);	
	}

	printf("\n");

	return 0;
}

