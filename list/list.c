#include "list.h"
#include <stdio.h>

struct task_list {
	int val;
	struct list_head my_list;
};

int main(int argc, char *argv[])
{
	int i;
	struct task_list *pos_ptr;

	struct task_list head_task = {
		.val = 0,
		.my_list = LIST_HEAD_INIT(head_task.my_list),
	};

	struct task_list task[10];

	for (i = 0; i < 10; i++) {
		task[i].val = i+1;
		INIT_LIST_HEAD(&task[i].my_list);
		list_add(&task[i].my_list, &head_task.my_list);
	}

	list_for_each_entry(pos_ptr, &head_task.my_list, my_list) {
		printf(" %d ", pos_ptr->val);	
	}

	printf("\n");

	return 0;
}

