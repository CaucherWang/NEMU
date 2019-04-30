#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_list[NR_WP];
static WP *head, *free_;

void init_wp_list() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_list[i].NO = i;
		wp_list[i].next = &wp_list[i + 1];
	}
	wp_list[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_list;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char * exp)
{
	assert(free_ != NULL);
	
	WP *temp = free_;
	free_ = free_->next;
	temp->next = NULL;

	bool success = false;
	strcpy(temp->exp, exp);
	temp->value = expr(temp->exp, &success);
	assert(success);

	if(head==NULL)
		head = temp;
	else
	{
		WP *p = head;
		while(p->next)
			p = p->next;
		p->next = temp;
	}

	return temp;
}

void free_wp(WP *wp)
{
	WP *p = head;
	while(p->next!=wp)
		p = p->next;
	p->next = wp->next;
	
	wp->next = free_;
	free_ = wp;

}

void print_wp()
{
	if(head==NULL)
		printf("NO WATCHPOINT!\n");
	WP *p = head;
	while(p)
	{
		printf("watch point %d: %s\n", p->NO, p->exp);
		printf("\t The value now is %d\n", p->value);
		p = p->next;
	}
}

bool delete_wp(int num)
{
	WP *p = head;
	while(p)
	{
		printf("p->num=%d\n",p->NO);
		if (p->NO == num)
		{
			free_wp(p);
			break;
		}
	}
	if(p==NULL)
		return false;
	else
		return true;
}

int test_change()
{
	WP *p = head;
	while(p)
	{
		bool success = false;
		uint32_t n_value = expr(p->exp, &success);
		assert(success);
		if(n_value!=p->value)	
		{
			printf("watchpoint %d:%s is changed\n", p->NO, p->exp);
			printf("The old value is %d\n", p->value);
			printf("The new value is %d\n", n_value);
			p->value = n_value;
			return 1;
		}
	}
	return 0;
}
