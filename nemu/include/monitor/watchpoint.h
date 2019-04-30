#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint
{
	int NO;
	char exp[32];
	uint32_t value;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
} WP;


void print_wp();
bool delete_wp(int num);
WP *new_wp(char *exp);
#endif

