#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
	char *arg = strtok(NULL, " ");
	int num=0;
	if(arg==NULL)
		num = 1;
	else
	{
		sscanf(arg, "%d", &num);
		if(num<1)
			num = 1;
	}	
	int i=0;
	for(;i<num;++i)
		cpu_exec(1);
	return 0;
}

static int cmd_info(char * args)
{
	char *arg=strtok(NULL," ");
	if(!strcmp(arg,"r"))
	{
		int i;
		for(i=0;i<8;++i)
			printf("%s:\t0x%08x\t%08d\n",regsl[i],cpu.gpr[i]._32,cpu.gpr[i]._32);
		printf("eip:\t0x%08x\t%08d\n",cpu.eip,cpu.eip);
		for(i=0;i<8;++i)
			printf("%s:\t0x%08x\t%08d\n",regsw[i],cpu.gpr[i]._16,cpu.gpr[i]._16);
		int j=0;
		for(i=0;i<4;++i)
			for(j=0;j<2;++j)
				printf("%s\t0x%08x\t%08d\n",regsb[i*2+j],cpu.gpr[i]._8[j],cpu.gpr[i]._8[j]);

	}
	else
	{;}
	return 0;
}

static int cmd_x(char *args)
{
	char *arg1=strtok(NULL," ");
	char *arg2=strtok(NULL," ");
	unsigned num=0;
	swaddr_t begin;
	sscanf(arg1,"%d",&num);
	sscanf(arg2,"0x%x",&begin);
	printf("%#08x:",begin);
	int i=0;
	for(i=0;i<num;++i)
	{
		printf("\t0x%08x",swaddr_read(begin,4));
		begin+=4;
		if(i%4==3)
		printf("\n\t");
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args)
{
	bool success=false;
	uint32_t ans=expr(args,&success);
	if(success)
		printf("%d\n",ans);
	else 
		printf("function fault!");
	return 0;
}

static int cmd_w(char *args)
{
	WP *n_wp=new_wp(args);
	printf("watchpoint %d:%s is set successfully.\n",n_wp->NO,n_wp->exp);
	return 0;
}

static int cmd_d(char *args)
{
	char *arg = strtok(NULL, " ");
	int num = 0;
	sscanf(arg, "%d", &num);
	bool ans=delete_wp (num);
	if(ans)
		printf("delete watchpoint %d successfully!\n", num);
	else
		printf("There is no watchpoint whose NO. is %d.\n",num);
	return 0;
}


static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{"si","execute the program step by step, N steps in total",cmd_si},
	{"info","info r means print the state of registers, while info w means print the information of moniting point",cmd_info},
	{"x","x N EXPR, solve the EXPR and view the solution as the beginning address. Then print consistant N bytes from the beginning address.", cmd_x},
	{"p","p EXPR, calculate the value of EXPR",cmd_p},
	{"w","w EXPR, create watchpoint, every time the EXPR is changed, program is paused",cmd_w},
	{"d","d N, delete the watchpoint whose number is N",cmd_d}

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
