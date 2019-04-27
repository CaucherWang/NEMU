#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256,NUM,EQUAL,HEXNUM,REGNAME,NOTEQUAL,DEREF,NEG

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
	{"[0-9]+",NUM},			// one decimal number
	{"0x[0-9,a-f]+",HEXNUM},	// one hexadecimal number
	{"$[a-z]{2,3}",REGNAME},	// a register name
	{"\\(",'('},			// left parenthesis
	{"\\)",')'},			// right parenthesis
	{"\\*",'*'},			// multiply
	{"\\/",'/'},			// divide
	{"\\+", '+'},			// plus
	{"\\-",'-'},			// minus
	{"==", EQUAL},			// equal
	{"!=",NOTEQUAL},		//not equal
	{"!",'!'}
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
						tokens[i].type = NOTYPE;
						++nr_token;
						break;
					case NUM:
						if(substr_len>31)
							assert(0);
						tokens[i].type = NUM;
						strncpy(tokens[i].str, substr_start, substr_len);
						++nr_token;
						break;
					case HEXNUM:
						if(substr_len>31)
							assert(0);
						tokens[i].type = HEXNUM;
						strncpy(tokens[i].str, substr_start+2, substr_len-2);
						++nr_token;
						break;
					case REGNAME:
						tokens[i].type = REGNAME;
						strncpy(tokens[i].str, substr_start + 1, substr_len - 1);
						++nr_token;
						break;
					case '(':
						tokens[i].type = '(';
						++nr_token;
						break;
					case ')':
						tokens[i].type = ')';
						++nr_token;
						break;
					case '+':
						tokens[i].type = '+';
						++nr_token;
						break;
					case '-':
						tokens[i].type = '-';
						++nr_token;
						break;
					case '*':
						tokens[i].type = '*';
						++nr_token;
						break;
					case '/':
						tokens[i].type = '/';
						++nr_token;
						break;

					case EQUAL:
						tokens[i].type = EQUAL;
						++nr_token;
						break;

					case NOTEQUAL:
						tokens[i].type=NOTEQUAL;
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
			ans=ans*16+(s[i]+10+s[i]-'a');
		else if(s[i]>='A'&&s[i]<='F')
			ans=ans*16+(s[i]+10+s[i]-'A');
	return ans;	
}
/*
uint32_t read_reg(char *s)
{
	int i;
	if(strlen(s)==3)
	{
		for (i = 0; i < 8; ++i)
			if(!strcmp(regsl[i],s))
				return  cpu.gpr[i]._32;
	}
	else
	{
		for(i=0;i<8;++i)
			if(!strcmp(regsw[i],s))
				return (uint32_t)cpu.gpr[i]._16;
		for(i=0;i<8;++i)
			if(!strcmp(regsb[i],s))
				return (uint32_t)cpu.gpr[i]._8[0];
	}
	
}*/

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
		if(tokens[i].type!='+'&&tokens[i].type!='-'&&tokens[i].type!='*'&&tokens[i].type!='/')
			continue;

		unsigned left=i-1,right=i+1;
		bool flag=false;
		while(1)
		{
			while(left>=p && tokens[left].type!='('){--left;}
			while(right<=q && tokens[right].type!=')'){++right;}
			if(left<p||right>q)	break;
			flag=check_parentheses(left,right);
			if(flag)	break;
		}
		if(flag)	continue;
		candidate[j++]=i;
	}
	bool max_priority=0;
	for(i=0;i<j;++i)
		if(tokens[candidate[i]].type=='*'||tokens[candidate[i]].type=='/')
			max_priority=1;
	if(max_priority)
	{
		for(i=j-1;i>=0;--i)
			if(tokens[candidate[i]].type=='+'||tokens[candidate[i]].type=='-')
				return candidate[i];
		return candidate[j - 1];
	}
	else return candidate[j-1];
}

uint32_t eval(unsigned p,unsigned q)
{
	if(p > q) {
        assert(0);
    }
    else if(p == q) { 
		if(tokens[p].type==NUM)
			return trans2num10(tokens[p].str);
		//else if(tokens[p].type==HEXNUM)
			//return trans2num16(tokens[p].str);
		//else if(tokens[p].type==REGNAME)
		//	return read_reg(tokens[p].str);
		else
			assert(0);
		/* Single token.
         * For now this token should be a number. 
         * Return the value of the number.
         */
	}
	//else if(check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses. 
         * If that is the case, just throw away the parentheses.
         */
        //return eval(p + 1, q - 1); 
    //}
    else {
	assert(check_only_parentheses(p,q)==true);
        unsigned op = dominant_operator(p,q);
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);

        switch(tokens[op].type) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': return val1 / val2;
	    case EQUAL:return val1 == val2;
	    case NOTEQUAL:return val1 != val2;
            default: assert(0);
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
	/* i;
	for(i = 0; i < nr_token; i ++) 
	{
    if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
        tokens[i].type = DEREF;
	if(tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
        tokens[i].type = NEG;
	} */
	return eval(0,nr_token);
	/* TODO: Insert codes to evaluate the expression. */
}

