#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256,NUM,EQUAL,HEXNUM,REGNAME,NOTEQUAL,DEREF,NEG,AND,OR

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},		// spaces
	{"0x[0-9,a-f]+",HEXNUM},	// one hexadecimal number
	{"[0-9]+",NUM},			// one decimal number
	{"\\$[a-z]{2,3}",REGNAME},	// a register name
	{"\\(",'('},			// left parenthesis
	{"\\)",')'},			// right parenthesis
	{"\\*",'*'},			// multiply
	{"\\/",'/'},			// divide
	{"\\+", '+'},			// plus
	{"\\-",'-'},			// minus
	{"==", EQUAL},			// equal
	{"!=",NOTEQUAL},		//not equal
	{"&&",AND},			//logical and
	{"\\|\\|",OR},			//logical or
	{"!",'!'}			//logical not
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret != 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */


				switch(rules[i].token_type) {
					case NOTYPE:
						break;
					case NUM:
						if(substr_len>31)
							assert(0);
						tokens[nr_token].type = NUM;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len]='\0';
						++nr_token;
						break;
					case HEXNUM:
						if(substr_len>31)
							assert(0);
						tokens[nr_token].type = HEXNUM;
						strncpy(tokens[nr_token].str, substr_start+2, substr_len-2);
						tokens[nr_token].str[substr_len-2]='\0';
						++nr_token;
						break;
					case REGNAME:
						tokens[nr_token].type = REGNAME;
						strncpy(tokens[nr_token].str, substr_start+ 1,substr_len-1);
						tokens[nr_token].str[substr_len-1]='\0';
						++nr_token;
						break;
					case '(':
						tokens[nr_token].type = '(';
						++nr_token;
						break;
					case ')':
						tokens[nr_token].type = ')';
						++nr_token;
						break;
					case '+':
						tokens[nr_token].type = '+';
						++nr_token;
						break;
					case '-':
						tokens[nr_token].type = '-';
						++nr_token;
						break;
					case '*':
						tokens[nr_token].type = '*';
						++nr_token;
						break;
					case '/':
						tokens[nr_token].type = '/';
						++nr_token;
						break;
					case EQUAL:
						tokens[nr_token].type = EQUAL;
						++nr_token;
						break;

					case NOTEQUAL:
						tokens[nr_token].type=NOTEQUAL;
						++nr_token;
						break;
					case AND:
						tokens[nr_token].type=AND;
						++nr_token;
						break;
					case OR:
						tokens[nr_token].type=OR;
						++nr_token;
						break;
					case '!':
						tokens[nr_token].type='!';
						++nr_token;
						break;
					default:
						panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

uint32_t trans2num10(char * s)
{
	unsigned len=strlen(s);
	unsigned i=0;
	uint32_t ans=0;
	for(;i<len;++i)
		ans=ans*10+(s[i]-'0');
	return ans;
}

uint32_t trans2num16(char *s)
{
	unsigned len=strlen(s);
	unsigned i=0;
	uint32_t ans=0;
	for(;i<len;++i)
		if(s[i]>='0'&&s[i]<='9')
			ans=ans*16+(s[i]-'0');
		else if(s[i]>='a'&&s[i]<='f')
			ans=ans*16+(s[i]+10-'a');
		else if(s[i]>='A'&&s[i]<='F')
			ans=ans*16+(s[i]+10-'A');
	return ans;	
}

uint32_t read_reg(char *s)
{
	int i;
	if(strlen(s)==3)
	{
		for (i = 0; i < 8; ++i)
			if(!strcmp(regsl[i],s))
				return  cpu.gpr[i]._32;
		return 0;
	}
	else
	{
		for(i=0;i<8;++i)
			if(!strcmp(regsw[i],s))
				return (uint32_t)cpu.gpr[i]._16;
		for(i=0;i<8;++i)
			if(!strcmp(regsb[i],s))
				return (uint32_t)cpu.gpr[i]._8[0];
		return 0;
	}
	
}

/*不仅查明表达式中括号是否匹配，还要看整个表达式是不是被一个括号括住*/
bool check_parentheses(unsigned p,unsigned q)
{
	if(tokens[p].type!='('||tokens[q].type!=')')
		return false;
	unsigned counter=0;
    unsigned i = p+1;
    for (; i < q;++i)
    {
        if(tokens[i].type=='(')
            ++counter;
        else if(tokens[i].type==')')
        {
            if(!counter)
			return false;
			--counter;
		}
    }
    if(!counter)
        return true;
    else
        return false;
}

/*仅仅查明表达式中括号是否匹配*/
bool check_only_parentheses(unsigned p,unsigned q)
{
    unsigned counter=0;
    unsigned i = p;
    for (; i <= q;++i)
    {
        if(tokens[i].type=='(')
            ++counter;
        else if(tokens[i].type==')')
        {
	    if (!counter)
		return false;
	    --counter;
        }
    }
    if(!counter)
        return true;
    else
        return false;
}

// 判断一个符号是不是被一个括号包围着
bool check_if_parentheses(int p, int q, int a)
{
	int stack[32];
	int stack_top = -1;
	int i ;
	for (i = p; i <= q; ++i)
	{
		if (tokens[i].type == '(')
		{
			stack_top++;
			stack[stack_top] = i;
		}
		else if (tokens[i].type == ')')
		{
			if (a >= stack[stack_top] && a <= i)	return true;
			--stack_top;
		}
	}
	return false;
}

unsigned dominant_operator(int p, int q)
{
	int i=p;
	unsigned candidate[32];
	int j;
	for(j=0;j<32;++j)
		candidate[j]=-1;
	j=0;
	for(;i<=q;++i)
	{
		if(tokens[i].type==NUM || tokens[i].type==HEXNUM || tokens[i].type=='(' || tokens[i].type==')' || tokens[i].type == REGNAME)
			continue;
		if (check_if_parentheses(p, q, i))	continue;
		candidate[j++]=i;
	}
	//对于可能成为返回值的进行优先级和结合性排序
	for(i=j-1;i>=0;--i)
		if(tokens[candidate[i]].type==OR)
			return candidate[i];
	for(i=j-1;i>=0;--i)
		if(tokens[candidate[i]].type==AND)
			return candidate[i];
	for(i=j-1;i>=0;--i)
		if(tokens[candidate[i]].type==EQUAL||tokens[candidate[i]].type==NOTEQUAL)
			return candidate[i];
	for(i=j-1;i>=0;--i)
		if(tokens[candidate[i]].type=='+'||tokens[candidate[i]].type=='-')
			return candidate[i];
	for(i=j-1;i>=0;--i)
		if(tokens[candidate[i]].type=='*'||tokens[candidate[i]].type=='/')
			return candidate[i];
	for(i=0;i<j;++i)
		if(tokens[candidate[i]].type==NEG||tokens[candidate[i]].type=='!'||tokens[candidate[i]].type==DEREF)
			return candidate[i];
	return 0;
}

uint32_t eval(int p,int q)
{
    if(p > q) 
        assert(0);
    else if(p == q) 
    { 
	if(tokens[p].type==NUM)
		return trans2num10(tokens[p].str);
	else if(tokens[p].type==HEXNUM)
		return trans2num16(tokens[p].str);
	else if(tokens[p].type==REGNAME)
		return read_reg(tokens[p].str);
	else
		assert(0);
	/* Single token.
         * For now this token should be a number. 
         * Return the value of the number.
         */
	}
	else if(check_parentheses(p, q) == true) 
        return eval(p + 1, q - 1); 
        else 
	{
	assert(check_only_parentheses(p,q)==true);
        unsigned op = dominant_operator(p,q);
	if(tokens[op].type==NEG || tokens[op].type==DEREF || tokens[op].type=='!' )
	  {
	    uint32_t val=eval(op+1,q);
	    switch(tokens[op].type)
	    {
	    case NEG:return -val;
	    case DEREF: return swaddr_read(val, 4);
	    case '!': return !val;
            default: assert(0);
    	    }
	  }
	else
	  {
            uint32_t val1 = eval(p, op - 1);
            uint32_t val2 = eval(op + 1, q);
            switch(tokens[op].type) 
	    {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': return val1 / val2;
	    case EQUAL:return val1 == val2;
	    case NOTEQUAL:return val1 != val2;
	    case AND: return val1 && val2;
	    case OR: return val1 || val2;
            default: assert(0);
    	    }
	  }
	}
	return 0;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	--nr_token;
	*success=true;
	int i;
	for(i = 0; i < nr_token; i ++) 
	{
    if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
        tokens[i].type = DEREF;
	if(tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
        tokens[i].type = NEG;
	} 
	return eval(0,nr_token);
	/* TODO: Insert codes to evaluate the expression. */
}

