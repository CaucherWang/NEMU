#### 改写寄存器CPU_state

- 思路：因为32位寄存器，16位寄存器，8位寄存器是共用内存的，所以一定是要用union来代替struct来定义gpr,之后为了定义几个寄存器的名字，我考虑在外层再套一个union,让几个寄存器的名字和gpr[i]地位等同，gpr是8个32位的变量拼成的数组，我想不妨再用一个整齐对齐的结构体来做外层union的另一部分，这个结构体也是8*32位的，结构体中每一个变量类型均为uint32_t,与数组相对应，变量名即为各个寄存器的名字，这样cpu.gpr[0]._32等价于cpu.eax，经测试，寄存器改写完成。其实主要体现为CPU内部寄存器共用内存这一事实。

- 遇到的困难：

  - 刚开始不明白寄存器的名字是要自己定义，还是直接用reg.h中上方以寄存器名字命名的常量
  - 开始不知道只需新定义32位寄存器名字即可

#### 完成简易调试器的三个函数

- 寄存器改写完成之后，NEMU已经可以开始运行了，根据手册上的指导，我尝试了NEMU的c,q,help指令，对NEMU的运行模式有了一个初步的理解。
- 查阅了man文件，了解了必备的三个函数，其基本功能如下
  - readline(char *),读取命令行提示符char *后面的字符串
  - sscanf(char *s1,char *s2,argument 3),按照s2规范的格式读取s1中的内容到s3中
  - strtok(char *s1,char *s2),把s1按照s2字符集进行分割，并返回第一个非空字符串；如果s1为NULL，s2则读取上次分割的下一子串

#### 单步执行函数cmd_si

- 思路：首先根据PA手册的指导和对代码框架图的理解，找到了调试器的文件ui.c，大致阅读了一下，找到了眼熟的c,q,help指令，开始阅读这三个指令的函数。cmd_c(),cmd_q(),两个函数只有一句话，而help函数有一些内容，在help函数实现中，我看到了一个没见过的定义，就是cmd_table数组，查阅了文件中的定义，发现这个就是指令集，之后补充的调试器的指令也应该是从这里补充上的，相应的函数也应在这之前声明。
- 实现：分有参数和无参数两种情况进行讨论，最终得到程序执行步数变量，传入cpu_exec(uint32_t)函数，运行指定步数。其中有参数需要解析一下字符串。
- 困难：对于cpu_exec()函数源代码的理解到现在还不完全清楚，开始直接将执行步数传入该函数，发现一个数字可以正常运行，超过一位数就无法运行，这种意外却不是字符串解析的问题（我在VS上单独调试过）。我换了一种方式，既然个位数可以正常识别，那就单步执行步数次，结果可以正常运行。不过两位数传递过去为什么不能执行，仍需在之后继续理解源代码或许才可以解决。

#### 寄存器状态函数cmd_info

- 思路：输出寄存器存储状态其实不是很难的问题，如果对源代码有了一定理解。直接访问cpu全局变量，可以直接利用union访问寄存器状态。PPT上的效果图只输出了32位寄存器，不过为了全面，我把所有位的寄存器都输出了，十六进制和十进制；名称上利用了全局变量regsl[],regsw[],regsb[]
- 困难：刚开始觉得顺序和手册上的寄存器顺序不一致，后来改了reg.h中32位寄存器顺序，发现编译都无法通过，于是放弃了这个想法

#### 扫描内存函数cmd_x

- 思路：第一个问题就是解析字符串了，这个函数有两个参数，需要先用字符串分割函数分开，然后读取；第二个问题就是对模拟内存的理解，根据手册提示，应使用swaddr_read()函数进行内存读取。不过按照调用顺序依次查看内存读取函数，发现len参数其实应该统一设成4，避免无符号数算术右移丢失数据，之后起始地址依次+4，因为观察到返回的是32位无符号数，那显然是4个字节，就往前推进四个字节。
- 困难：开始忽视了对模拟内存的理解，绕了很大的弯路，不过后来经同学提醒，解决了这个问题；还有就是对内存源代码的理解还不十分到位，没有完全理解，在之后的PA完成过程中应多多注意。


### 一、表达式求值

#### 1. 对框架代码的理解

##### a) 正则表达式及<regex.h>内置函数

- int regcomp(regex_t *preg, const char *regex, int cflags);
  匹配规则编译函数，编译结果放在数组preg里，regex_t是一个内置类型，专门放置正则表达式编译结果。参数regex是一个字符串，它代表将要被编译的正则表达式；参数cflags决定了正则表达式该如何被处理的细节。 编译成功，函数将返回0，否则返回非0值。 
- int regexec(const    regex_t    *preg,    const    char *string, size_t nmatch,regmatch_t pmatch[], int eflags);
  typedef struct {
  ​    regoff_t rm_so;
  ​    regoff_t rm_eo;
  } regmatch_t;
  在regcomp()函数成功地编译了正则表达式之后，可以调用regexec()函数进行模式匹配。参数preg是编译后的正则表达式结果，参数string是将要进行匹配的字符串，参数nmatch和pmatch则用于把匹配结果返回给调用程序，参数eflags决定了匹配的细节。 匹配成功返回0，否则返回非0值。
  在调用函数regexec()进行模式匹配的过程中，可能在字符串string中会有多处与给定的正则表达式相匹配，参数pmatch就是用来保存这些匹配位置的，而参数nmatch则告诉函数regexec()最多可以把多少个匹配结果填充到pmatch数组中。当regexec()函数成功返回时，从string+pmatch[i].rm_so到string+pmatch[i].rm_eo是第i个匹配的字符串。

##### b) expr.c

- token_type：自定义的一些符号常量，常量值为256之后是因为不会跟一些ASCII码值冲突造成误解。（要在这里补一些自定义的符号常量）
- rules：对于正则表达式的一些元字符的对应规则，类型为结构数组，每个元素都是一个结构，包含正则表达式和自定义值的符号常量。（要在这里补一些正则表达式对应规则，因为最后是按顺序遍历对应规则，所以优先级高的符号应该放在前面）
- NR_REGEX：有几条规则
- re[NR_REGEX]：<regex.h>内置类型数组，存放各种token编译结果的数组。
- init_regex()：对每个token对应规则进行编译，如果token的表达都不符合规定，会直接报错。成功的话，编译结果放在re[]里。
- make_token(char * e)：对输入的字符串进行分段解析，从头开始，直到字符串结尾结束。对于每个当前位置position，都遍历一遍规则进行正则表达式匹配，得到的匹配结果必须是从position这个位置开始符合要求的第一个子串，之后需要我用一个switch结构把匹配字符串中相应的各个子串按不同的token放置在tokens数组里。
- tokens[32]：一个结构数组，按照token在表达式中出现的顺序，每个元素中包含token的类型及子字符串。
- nr_token：一个整型变量，记录着共有几个token。
- uint32_t expr(char *e, bool *success)函数：计算表达式e的值并返回，即主功能函数。其首先通过make_token函数把e拆解成若干token,并且存进全局变量的tokens数组里，然后用eval函数进行表达式求值。

#### 2. 具体实现

##### a) 正则表达式匹配规则的扩展

- 许多token_types和rules在框架代码中没有写出来，需要我先了解正则表达式之后对它进行扩充。

  ```C
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
  ```

- 其中符号常量从256开始是因为ASCII码是7位二进制码，1-255有效，为了不和用字符型ASCII的类型冲突，所以从256开始编码。

- 这里有一个小小的顺序问题需要注意，因为对于正则表达式规则的匹配是从规则表从前到后进行匹配，所以十六进制数字的匹配必须在十进制数字的前面，因为十进制数字的匹配会把十六进制数字前的0x中的‘0’读取而改变了本来的含义；同理，不等于!=也必须在非 ! 前面。

- 注意到这里的'*'被解释为乘号，但是类型中是存在间接引用的，同理'-'被解释为减号，但是类型中是存在负号的，之后将在其它函数中将他们区分开。

##### b) 区分同形多意操作符

- 我们在expr()函数里对乘号和间接引用，减号和负号进行区分。

- 如果*是tokens数组里的第一个token，或者星号前面的token不是操作数和右括号，那星号就一定是间接引用的含义，减号也是同理，证明略，代码如下。

  ```C
  	int i;
  	for(i = 0; i < nr_token; i ++) 
  	{
      if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
          tokens[i].type = DEREF;
  	if(tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type !=NUM && tokens[i-1].type!=HEXNUM && tokens[i-1].type!=REGNAME && tokens[i-1].type!=')')) ) 
          tokens[i].type = NEG;
  	} 
  ```

##### c) 将对应到的规则写入tokens数组

- 对于操作数，我们有必要保留标志性的字符串，对于运算符和括号，我们只需记录它的类型即可。

- 特别注意空格不应该写入token数组，因为它不具备任何实际含义，也没有对其进行特别处理的语句，所以将其丢弃即可。

- 对于十六进制数，我们只保留数字部分；对于寄存器名称，我们只保留字母部分。

- 还有对于需要写入字符串的操作数部分，必须要在结尾额外加一个’\0‘，因为tokens数组是全局变量，这就意味着计算两个表达式时，计算完第一个表达式其tokens内容不会被丢弃，而是仍然被保存，如果对于每个token第二个表达式的字符串长度都大于第一个表达式，那不会有问题；但是如果某个token第二个表达式字符串长度小于第一个表达式，那这个字符串的后面还保留着上一个表达式该位置token的一部分，会造成错乱。

- 以下代码限于篇幅限制，只列出有代表性的一小部分。

  ```C
  switch(rules[i].token_type) {
  					case NOTYPE:
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
  					case '+':
  						tokens[nr_token].type = '+';
  						++nr_token;
  						break;
  					case NOTEQUAL:
  						tokens[nr_token].type=NOTEQUAL;
  						++nr_token;
  						break;
  				}
  ```

  

##### d) 表达式求值函数：

- 当e被成功编译之后，uint32_t eval(unsigned p,unsigned q)函数以`递归方式`求解字符串表达式e拆解成的若干token，按不同的方式对token进行处理。p,q为所求表达式的第一个token和最后一个token

- 如果p>q，显然有错误

- 如果p=q，那么当前token必然是一个数字或寄存器，十进制或十六进制，将字符串解析成十进制数字返回即可。

  ```C
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
  
  ```

  - 功能展示：

    ![1556603708512](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1556603708512.png)

    ![1556603778783](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1556603778783.png)

##### e) 解析转化函数：

- 字符串转化成十进制或十六进制是十分简单的，这里不再解释代码。读取寄存器转化函数内容如下。

```C
uint32_t read_reg(char *s)
  {
  	int i;
  	if(strlen(s)==3)			//名字有3个字母的寄存器
  	{
  		for (i = 0; i < 8; ++i)
  			if(!strcmp(regsl[i],s))	 //和寄存器名字字符串数组逐个比较
  				return  cpu.gpr[i]._32;  //返回寄存器中存储的值
  		return 0;
  	}
  	else						//名字有2个字母的寄存器
  	{
  		for(i=0;i<8;++i)
  			if(!strcmp(regsw[i],s))
  				return (uint32_t)cpu.gpr[i]._16;//为匹配返回值类型，先进行类型转换
  		for(i=0;i<8;++i)
  			if(!strcmp(regsb[i],s))
  				return (uint32_t)cpu.gpr[i]._8[0];
  		return 0;
  	}
  }
```


​      

  - 如果p<q，先检验表达式的括号是否合法，这里的合法包括两个方面，第一是本身括号匹配没有问题，第二是整个表达式被一对括号包住。以下是检验括号合法性的函数，用到了栈的思想，用一个整型变量counter来模拟不需要保存值的栈。
  
    ```C
    bool check_parentheses(unsigned p,unsigned q)
    {
    	if(tokens[p].type!='('||tokens[q].type!=')')	//检查表达式是否被一堆括号包住
    		return false;
    	unsigned counter=0;		//检测到的左括号数量
        unsigned i = p+1;
      for (; i < q;++i)
        {
            if(tokens[i].type=='(')
                ++counter;
            else if(tokens[i].type==')')
            {
                if(!counter)	//右括号之前没有左括号进行匹配
    			return false;
    			--counter;
    		}
        }
        if(!counter)	//所有左括号都被匹配掉
            return true;
        else
            return false;
    }
    ```
    
  - 如果表达式的括号是合法的，那么计算被括号包住的里面的值。
  
  - 如果表达式的括号不是合法的，那么再次检验这个表达式的括号匹配是否正常。括号匹配检验函数和括号合法函数有点类似，这里不再赘述。如果连括号匹配都是错误的，那么触发assert(0)，程序异常中断。
  
  - 如果括号匹配是没有问题的，只是最外层没有一对括号包住，那么就可以开始寻找这个表达式的dominant_operator。所谓dominant_operator，就是这个表达式最后一步计算的运算符，它处在优先级最低位，结合性最低位，并且不能是被括号包住的运算符，满足这三点，就可以是dominant_operator.

##### f) dominant_operator函数

```c
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
	for(i=0;i<j;++i)	if(tokens[candidate[i]].type==NEG||tokens[candidate[i]].type=='!'||tokens[candidate[i]].type==DEREF)
			return candidate[i];
	return 0;
}
```

- 函数思路如下：所谓表达式，其实就是一系列运算符、操作数、括号的集合，我们要找的dominant_operator实际上是一个特定的运算符。那对于所有token来说，先排除掉操作数和括号，然后用一个函数检验每个运算符是不是包含在一对括号里面。验证完这两点，就可以写入candidate数组里，等待优先级和结合性的检验。
- 优先级检验：越是优先级低的运算符，越应该先进行检验；没有优先级低的运算符，才会去考虑优先级高的运算符，所以对candidate数组里的运算符，检验顺序为：|| > && > (==,!=) > (+,-) > ( *, /) > 单目运算符
- 结合性检验：对于结合性从左到右的，要从candidate数组的后面向前进行检验，因为candidate数组后面的符号是表达式后面的token，也是后面的token会成为dominant_operator，而不是前面的；同理，结合性从右到左的，就要从前到后对candidate数组进行检验，因为更靠近左面的token会成为dominant_operator。

##### g) 检验操作符是否被一对括号包含

- 检验某个位置的操作符是否被一对括号包含我额外提供了一个函数check_if_parentheses()

  ```C
  // 判断某个位置的一个符号是不是被一个括号包围着
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
  ```

- 算法的思路如下：对于每一个左括号记录其位置，每当其和其对应的右括号匹配成功时，就去检验操作符的位置是否在这一对括号中，是的话就返回true，不是的话继续检验，直到所有括号被匹配成功，此时返回false，说明该操作符没有被一对括号包围着。

#### 3. 遇到的问题

- 做的时候遇到了一个问题：我的dominant_operator函数总是编译不通过，报这样一个错。
  
    ```
    error: control reaches end of non-void function [-Werror=return-type]
     }
    
    ```
  
    经我查证这是说没有写返回值导致的，但实际上我返回了值。经我仔细研究，原因在于不是所有情况都有返回值，其实正常应该是warning，但NEMU的ccl把所有warning全变成了error,只能在最后补一个return 0消除这种异常，虽然这种情况不会发生，因为运算符异常会在解析表达式make_token时就已经触发了。
  
- 之后程序还是异常，可编译，但运行没有任何反应，明明到了调用函数的一步，却连dominant_operator()函数的第一句指令（调试用的printf("here\n")语句)也不能执行输出，因此我请教过助教，助教教我利用top指令查看资源管理器，发现NEMU的CPU已经占满了，说明并不是程序卡住了，而是哪里正在进行大量计算，由于Linux环境下无法进一步调试，我就把必要的函数复制粘贴到Windows的Visual Studio 2017编译器的环境下，在这个环境里，我发现dominant_operator()函数的一个数组索引错了，导致程序存在危险。我改好之后程序正常运行。因此我推测可能Linux操作系统对函数的安全性要求很高，对于危险的函数可能不会正常运行，所以才导致了无法调试。
  
- 算十六进制转化时，我手滑在一个表达式里多加了一个s[i]，这是一个很小的错误，但是不太容易发现，好在测试时选取了大量的表达式，发现了问题。
  
- NOTYPE类型开始我也写进tokens数组，发现带空格的表达式偶尔会出问题，经我肉眼逻辑分析，发现了这个BUG。
  
- make_token函数开始也有数组索引错误，但是因为系统有自己的蓝字正则表达式匹配，我误以为匹配是正确的，将重心放在了之后的函数，还在VS2017下进行调试发现没有问题才反应过来正则匹配可能出问题了。就此，因为一个十分简单而愚蠢的错误浪费了2个小时的时间，非常痛心。
  
- make_token函数开始我没有把字符串尾结束符单独考虑，在大量的测试中我发现单独的某个表达式第一次计算没有问题，但是接着一个很长的表达式后面就会算错，之后才想到共用tokens全局变量不完全覆盖这样一个问题。
  
- 正则规则匹配中，|是正则匹配的元字符，我开始没注意到这点，后来发现||运算错误，才开始考虑这个问题，经查证，改正了这个BUG。
  
- 还遇到了加入运算符却忘记加入判断条件等诸多问题，这里不再一一赘述了。
  
- 可以吸取经验的是，大量的对功能测试会帮助我们找到更多的BUG，避免很多想当然的情况。



### 二、监视点

#### 1. 对框架代码的理解

##### a)  “池”的数据结构

- 经过简要查资料，池意味着一块固定大小的内存区，在程序运行开始时就已经分配好了，之后与之相关的一些变量会固定的从这个内存区中取用或释放，保证其总量是不变的，不会溢出。

##### b）对现有符号的理解

- WP：自定义结构类型，代表一个监视点，也是监视点链表中的一个基本单元。

- wp_list：这就是本文件中的池，预分配了NR_WP个WP结构大小的连续区域，是已占用和未占用的监视点的集合。

- head：已占用的监视点链表的表头。
- free_：池中空闲状态监视点也被用一个链表串起来，free是它们的表头。
- init_wp_list()：把池中的监视点结构初始化，按照数组顺序进行编号，这个编号之后不会变化，同时用free_作表头的链表进行串接起来。

##### c）对蓝框问题的回答

- 问题：框架代码中定义 `wp_pool` 等变量的时候使用了关键字 `static` , `static` 在此处的含义是什么? 为什么要在此处使用它?

- static是局部变量的意思，是说wp_pool等变量只可在watchpoint.c中被使用，而不可以在其他文件中使用。目的是为了便于维护池，池占用了一块固定内存，如果被任意使用，会导致分配混乱浪费有限的内存，所以把有关池的所有操作都放在这一个文件里。

  

#### 2. 具体实现

##### a） 对WP结构的扩充

- 我对WP结构类型中补充了一个保存表达式字符串的变量exp，和一个保存当前表达式值的变量value。

##### b） 为新监视点分配空间

- 有了表达式求值的函数，这里的实现就把重心放在了链表的操作上。

- 从free_的头部摘下一个空余监视点，之后在head的表尾上接上这个新监视点。

  ```C
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
  ```

##### c） 释放监视点

- 释放特定编号的监视点是一个ui指令，所以我写了两个函数，一个负责和ui接口，参数是一个整型变量；另一个函数负责池中的链表操作。

- 拆下来的监视点安插在free_的表头。

  ```C
  bool delete_wp(int num)
  {
  	WP *p = head;
  	while(p)
  	{
  		if (p->NO == num)
  		{
  			free_wp(p);
  			break;
  		}
  		p=p->next;
  	}
  	if(p==NULL)
  		return false;
  	else
  		return true;
  }
  ```

  ```C
  void free_wp(WP *wp)
  {
  	WP *p = head;
  	while(p->next!=wp)
  		p = p->next;
  	p->next = wp->next;
  	
  	wp->next = free_;
  	free_ = wp;
  }
  
  ```

##### d） 打印监视点

- 为了满足info w的功能需求，补充一个打印监视点的函数。

```c
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
```

##### e） 检测监视点表达式值的变化

- 基本函数实现之后，可以简单地把相应的cmd函数写出来。之后我找到了cpu-exec.c文件，发现已经提示了在哪个部分进行监视点检测，就是程序每执行一次就要进行一次检测，那么现在需要一个监视点检测函数。

- 这个函数实现的思路很简单，只需要把监视点存储的表达式再求一次值，检查它和监视点中存储的值是否相等即可。相等不做改变，不等就将NEMU_state变量设置为STOP，中断执行。

  ```C
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
  		p=p->next;
  	}
  	return 0;
  }
  ```

#### 3. 遇到的问题

- test_changer()函数的返回值如果设置为bool，编译器就会说它不认识bool，改成int就可以了，但是原因我还没太明白，因为其它函数用bool作为返回值没什么问题。

- 链表操作开始有两处忘了迭代，运行结果一直不停，没有办法调试，但是检查了一下源代码，很快就发现了。

- 开始发现监视点发生变化时是可以监测到并且打印输出的，但是程序并不会中断，是因为之前实现cmd_si()函数时，n次执行exe_cpu(1)，所以让nemu_state发生变化没有用。那么改成exe_cpu(n)，就会起到作用。那么如何解决之前执行步骤不超过10的问题呢，我们找到了这个宏定义

  把这个宏定义值修改为int最大正数值，就可以在正常情况下运行啦！



#### 2. shell 命令统计代码行数

- PA1结束之后，共有代码4206行

  ```shell
  find /home/caucher/programming-assignment/nemu/ -name "*.c" -or -name "*.h" |xargs cat|wc -l
  ```

  - find -name 命令查找到nemu文件夹下所有以.c或.h文件
  - |运算符把前一个命令得到的结果传送到后一个命令作为输入
  - xargs把find找到的所有路径一个一个接到cat后面作参数
  - 没有[option]的cat操作符会把接收到的所有文件依次连接传送给wc
  - wc是统计命令,-l是统计目标文件的行数

- 除去空行，代码行数为3430行

  ```shell
  find /home/caucher/programming-assignment/nemu/ -name "*.c" -or -name "*.h" |xargs cat|grep -v ^$|wc -l
  ```

  - grep是匹配命令，-v是除了匹配字符串行的所有行， ^$表示空行

- 通过git log找到PA1开始日期之前的commit值95f219a7fa169130544dd984ea7fe64a4f581ad0

- 通过git checkout回到过去的分支，再统计一次行数，得到结果3018

- 所以我在PA1中写了412行代码。

- 之后再次用git checkout回到返回前的代码。

  


#### 3. gcc编译选项

##### a） gcc中的 `-Wall` 和 `-Werror` 有什么作用? 为什么要使用 `-Wall` 和 `-Werror`

- -Wall是要显示所有的警告
- -Werror是要把所有的警告信息都变成error显示出来（这就是困扰我很久的罪魁祸首）

### 执行第一个程序：

#### 概述：

- 要执行程序，其实就是可执行文件的指令从内存中取出，进行翻译，对应相应的函数进行操作，返回指令长度，移动PC，周而复始进行循环。那么其中一个重要的步骤就是对指令码进行解读，解读示意图如PA手册。

  ![1558937465503](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1558937465503.png)

![1558937490083](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1558937490083.png)

#### 从内存中取出指令：

- 取指函数instr_fetch(eip,len):实际上就是一个读内存的函数，第一个参数代表一个内存地址，这里传递了PC的值，即当前指令所在地址，然后返回地址指向的指令（无符号32位整数）,长度为len个字节，len最大为4。

#### 译码：

- <u>对opcode操作码的解读</u>：在i386手册中，每一条指令都有一个唯一的操作码对应，对于不同的操作数类型，操作数大小，一般都有不同的opcode。这是操作指令的核心部分。在nemu中，这些opcode集中体现在，opcode_table数组中，数组每个元素代表一个函数指针，这个函数代表一个指令，这些函数有着统一的逻辑格式，即以PC为输入，以int为输出。

- <u>对指令集的扩展</u>：
  - **转义码**：opcode操作码只占据一个字节，而一个字节最多编码256条指令，所以要实现更多指令就要延伸op字段。在nemu中，如果0x0f为指令的第一个字节，那么代表真正的opcode藏在第二个字节中，需要在_2byte_opcode_table数组中填写真正的指令函数。
  - **指令组**：x86还会利用M0dR/M字节中的reg/opcode位进行指令的区分。同一个opcode不同指令构成的集合称为指令组，经我在i386查找验证，对于同一个指令组，操作数类型及长度都相同，只有操作不一致。在NEMU中，体现为code_table中的make_group,一个group里面包含6个指令，opcode是相同的，后面的/digit决定它们在group的第几个位置。reg/opcode共三位，所以每个指令组有8个指令。
  
- <u>操作数</u>：
  - **寄存器操作数**：i386手册中的操作码后面跟着一个`/r` 表示后面跟一个 `ModR/M` 字节, 并且 `ModR/M` 字节中的 `reg/opcode` 域解释成通用寄存器的编码, 用来表示其中一个操作数，有相应的编码表。
  
  - **operand-size prefix**：由于16位与32位操作数的opcode是一样的，为了区分，如果指令的第一个字节是0x66，表示操作数是16位，否则表示操作数是32位的。

  - 内存操作数： `r/m` 表示的究竟是寄存器还是内存, 这是由 `ModR/M` 字节的 `mod` 域决定的: 当 `mod` 域取值为 `3` 的时候, `r/m` 表示的是寄存器; 否则 `r/m` 表示的是内存。表示内存的时候又有多种寻址方式, 具体信息参考i386手册中的表格17-3.内存操作数有多种寻址方式，在nemu中由于内存操作数的译码函数都已经写好，不需要对此深入理解。
  
  - **无符号立即数与有符号立即数**：在相应的x86指令对应的域中。
  
  - 在nemu中，decode函数会将操作数完善成一个Operand结构类型变量，存储该操作数是何种类型，大小。对于寄存器操作数，会存储其寄存器编号；对于内存操作数，会存储内存地址；对于有/无符号立即数，会存储它的值。任何类型的操作数都会存储它的值。还会构造一个Operands结构类型变量，存储一条指令中的所有操作数、opcode，以及对于同样的opcode，用is_Data_size_16来记录是否存在operand-size prefix，方便接口。
  
  -  ```C
    typedef struct {
    uint32_t type;
    size_t size;
    union {
    	uint32_t reg;
    	hwaddr_t addr;
    	uint32_t imm;
    	int32_t simm;
    };
    uint32_t val;
    char str[OP_STR_SIZE];
    } Operand;
    typedef struct {
    	uint32_t opcode;
    	bool is_data_size_16;
    	Operand src, dest, src2;
    } Operands;
    ```
  
- <u>译码函数的实现</u>：

  - decode函数以指向跳过opcode的eip为输入，以该指令剩余的指令长度为返回值，不同操作类型有不同的decode函数。
  - decode_si函数有一个需要实现，实际上就是从内存特定位置读出一条机器指令的立即数。eip此时所处的位置就是立即数的机器码所在位置，那么只需要从eip位置处读出一个DATA_TYPE的内容。另外，要求将机器码解释为一个32位有符号数，前面我们提到过，instr_fetch(eip,length)函数返回的是一个无符号数，那就需要一个强制类型转换。


#### 指令的执行：

- <u>操作数的调用</u>(template_start.h)：

  - **寄存器操作数**：REG(index)宏替换可以利用寄存器编号访问寄存器保存的值，REG_NAME(index)宏替换可以利用寄存器名字（枚举常量）访问寄存器值。宏替换后的函数在reg.h中有定义，实现很简单，这里省略。
  - **内存操作数**：MEM_R(addr)宏替换可以利用地址读取内存中的值，实际上是利用swaddr_read函数访问虚拟地址；MEM_W(addr, data)宏替换可以利用地址设置内存中的值，实际上是利用swaddr_write函数访问虚拟地址。
  - **数据的写入**：write_operand函数非常方便，会判断传入操作数的类型自行判断如何写入，第一个参数是需要写入的操作数Operand，第二个参数是数据。
  - **最高位的读取**：MSB(n)，宏替换函数，利用右移得到数据二进制最高位是0还是1。

- <u>操作数基本信息的获取</u>(template_start.h):

  由于指令函数实现一般是由模板来实现的，对于同一种操作的不同操作数大小只需写一个函数，那就需要让实现函数确定当前操作数的大小是多少。宏定义中一些宏代表当前操作数的大小信息：

  - 后缀：SUFFIX
  - 数据大小：DATA_BYTE
  - 数据对应的无符号整数类型：DATA_TYPE
  - 数据对应的有符号整数类型：DATA_TYPE_S

- <u>指令函数的声明</u>(all_instr.h+*.h)

  - all_instr.h会包含所有的*.h文件
  - *.h中包含了同一条指令的所有函数的声明

- <u>指令函数的定义</u>(*-template.h+.c)

  - *-template.h中对每一种操作类型的函数进行定义，实际上是宏替换后进行定义的，即make_instr_helper(name)。实际上是将每种操作补充后缀之后利用idex(eip,译码函数，执行函数)函数统一进行定义。idex函数包括两个部分，一个是译码，一个是执行。**由于译码函数绝大部分已经写好，而且命名系统已经成型，这就要求我们在为操作命名时必须符合decode函数的命名，否则会识别错误或无法识别**。执行函数即do_execute是需要我进行实现的，对于所有操作类型和大小统一使用一个执行函数是因为操作数类型和大小可以从宏定义中获取，又有方便统一的写入函数，这为执行函数的统一提供了条件。
  - *.c文件：同样提供了函数的定义，对于那些16位和32位共享一个opcode的操作码，命名时用_v进行代替，实际操作数大小信息存储在ops_decoded结构变量中，那就需要利用宏替换函数make_helper_v把后缀进行更换，而替换后相应函数的声明和实现正在-template.h中，其包含在.c文件之前，那就可以进行调用。

- <u>指令函数调用思路(以sub为例)</u>：

  - sub.h对所有函数进行声明(b,v为后缀)  ->  all_instr.h文件包含sub.h文件  ->  exec文件包含all_instr.h文件，则exec中包含sub所有函数的声明  ->   exec文件中对某一特定sub函数(b,v为后缀)进行调用   
  - 对于b为后缀及其它函数，直接在sub-template.h找到定义  ->  调用特定的idex函数  ->  调用译码函数(decode_)  ->  调用执行函数(do_execute)  ->  执行完毕返回指令长度  ->  返回exec文件执行结束
  - 对于v为后缀及其它函数，在sub.c文件中找到定义  ->  调用sub-template.h文件中的w,l为后缀的函数的定义  ->  调用特定的idex函数  ->  调用译码函数(decode_)  ->  调用执行函数(do_execute)  ->  执行完毕返回指令长度  ->  返回exec文件执行结束

- <u>对于concat、#和##</u>：
  
  - #是C语言用于宏定义中，将宏定义的参数转化为字符串的一个运算符。
  - ##是C语言用于宏定义中，将宏定义的参数直接拼接拼接在一起的一个运算符。
  - comcat_temp(x,y)宏定义函数，将两个参数拼接起来。**<u>由于带有##的宏的实参如果是宏的话，这个宏不会展开</u>**，解决方式就是用这样的二级宏，在第一次宏展开的时候(concat(x,y))不设计#，所以可以正常把参数进行宏展开，之后二次宏展开的时候(concat_temp(x,y))，再把已经展开的参数进行拼接，就得到了答案。
  - concatn函数，将n个参数按顺序拼接起来。
- str(x)宏函数可以将参数x进行宏展开（如果是宏的话），然后将其变成字符串。所用的策略仍是刚才使用的二级展开，这里不再赘述。
  
- <u>指令输出函数</u>：

  在执行指令结束之后，还要输出以提示用户，框架代码提供了三种模板输出函数

  - print_asm_template1():只有源操作数
  - print_asm_template2():有源操作数和目的操作数
  - print_asm_template3():有两个源操作数和目的操作数
  - print_asm():类似printf的用法，用于输出特殊指令


- <u>指令函数的实现</u>：

  在PA2中最重要的工作就是实现指令函数

  - 首先对于每个instr，建立三个文件.h,.c,-template.h，分类入各个文件夹中。

  - .h文件编写非常简单，按照正常.h文件的规范进行宏定义，然后把所有b,v为结尾的函数声明用make_helper宏替换编写进即可。

  - .c文件编写也很简单，对于可能的每种DATA_BYTE(1,2,4)，对-template.h进行包含，然后对所有v为后缀的函数用make_helper_v函数进行定义，还要包含helper.h，里面有一些宏替换的定义。

  - 最后是-template.h。

    - 首先包含template-start.h和template-end.h，然后对instr进行宏替换，准备好所有的宏定义。
  - 利用make_instr_helper()函数对以b,w,l为后缀的所有函数提供定义
    - 最后实现do_execute()函数格式是统一的。大抵分两步，执行操作以及对于需要改变标志位的修改其标志位。具体实现将分指令逐个讨论。

  ***为了结构清晰，我们先解释指令操作，再对标志位的改变算法进行讨论。***

- <u>sub指令</u>：用目的操作数减去源操作数，再写入目的操作数的值中。

- <u>call指令</u>：

  call指令的实现比较特殊，因为对于不同类型的操作数，指令长度不同（操作数为立即数指令长度为立即数size，操作数为r/m则并非如此），而指令长度对于函数实现有作用，所以只能放弃优秀的idex函数和instr函数，对于不同类型的操作数，自己编写make_helper函数。

  对于call指令的实现分两种一种是PC32的方式，一种是32的方式，无论哪种方式，都可以分成三步，第一步用esp分配空间，第二步将返回地址存入内存栈中，第三步是更改eip，使其指向调用的位置。

  - 首先将栈指针移出一个DATA_BYTE的长度，用以存放返回地址。
  - 由于没有使用idex函数，那就要用concat自行译码，注意传入的eip应该跳过opcode，记录返回值。
  - eip+len+1就是返回地址，1代表opcode，len代表剩下的指令长度。
  - PC32方式需要eip加上源操作数的值，32方式需要eip被赋值为源操作数的值。

  *此处注意DATA_BYTE表示指令中操作数的大小，而不一定等于指令所占字节数，所以在确定返回地址的时候，要利用decode函数的返回值确定指令所占字节数；在确定esp为返回地址留出空间是就用DATA_BYTE。*

- <u>push指令</u>：机器码50-57分别代表push指令把8个不同的寄存器压入栈帧，其实push类似call指令操作的前两步，实现起来非常简单，不再赘述。

- <u>test指令</u>：实际操作就是把源操作数和目的操作数取与，然后更改标志位。

- <u>je指令</u>：实现je的时候发现此时出现第一个opcode是0x0f，那么此时就要在第二个字节的opcode表进行补充。je要做的就是检验ZF，如果为1的话，eip要加上源操作数的值以实现跳转。由于所有指令在执行完毕之后都会统一进行eip向下移动，所以在实现指令跳转函数的时候要保留出一部分，具体来说，就是1+len，len是decode函数的返回值，对于操作数为立即数的指令来说，就是DATA_BYTE。

- <u>ret</u><u>指令</u>：ret指令首先要将eip恢复为栈指针中所保存的地址，然后栈空间进行回收。

- <u>jmp指令</u>：jmp指令分两种，操作数是立即数的话直接让eip加上这个立即数的值，操作数是r/m的话，就要让eip等于r/m中存储的值，也就是源操作数的值。

- <u>cmp指令</u>：cmp指令的本质就是sub，不再赘述。

- <u>add指令</u>：add指令直接将两个操作数相加。

- <u>leave指令</u>：leave指令分成三步，首先栈指针回到被调用函数栈顶，然后ebp指针得到旧值，回到caller函数的顶部，然后esp指针再向上释放一个地址的空间。

- <u>jbe指令</u>：jbe指令相比于je指令除了验证ZF以外，增加验证CF，二者有一个是1，就可以跳转。

- <u>lea指令</u>：lea指令需要取出源操作数的地址然后写进目的操作数。

对于一部分指令，会改变eflags寄存器中的标志位，而目前还没有实现eflags，可以在reg.h中将CPU_state结构中寄存器名字后面接上一个以虚拟地址为类型的变量eip模拟PC，一组标志位构成的32位结构体模拟elfags。

另外，这里给出所有影响标志位eflags寄存器的指令，提醒在后面指令实现中，改变eflags.

![1558591842161](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1558591842161.png)

各个标志位是如何被影响的，在i386手册中也有所体现。

![1558962625742](C:\Users\64451\AppData\Roaming\Typora\typora-user-images\1558962625742.png)

- SF是通过验证结果的最高位来设置的，可以使用MSB(result)。
- PF标志位要算低8位中是否有偶数个1，本来想用lab1 bits.c中位运算掩码的方法二分计算，后来想想没必要，没有bits.c那么多限制，直接用循环和一个标志变量实现就OK了。
- ZF的设置方式就不再多说了。
- CF和OF的检验方式因命令而异。

- <u>sub指令和cmp指令</u>：
  - CF：如果被减数是小于减数的，设置CF
  - OF：如果被减数和减数的符号相同，并且和结果不同，设置OF
- <u>add指令</u>：
  - CF：如果结果是小于任何一个操作数的，设置CF
  - OF：和sub一致
- test指令，CF与OF均设置为0.

### 插入断点

- 按照PA手册上的建议，对expr.c进行修改，加入一个token，补充一个对变量名token的求值函数。

- 阅读elf.c源代码：buf读了可执行文件的4096个字节，并把文件头部指针给了elf，sh_size计算出所有节的大小，之后加载节头部表到指针sh处。之后加载strtab到shstrtab指针中，根据PA手册讲，strtab就是把符号名连续排列，那么只需要一个char类型的指针，最终读取到了symtab和strtab，nr_symtab表示符号表条目个数

- 以下是elf.c中摘取的一些有用的数据结构和宏定义

- ```c
  #define STT_OBJECT  1       /* Symbol is a data object */
  /* Symbol table entry.  */
  
  typedef struct
  {
  Elf32_Word    st_name;        /* Symbol name (string tbl index) */
  Elf32_Addr    st_value;       /* Symbol value */
  Elf32_Word    st_size;        /* Symbol size */
  unsigned char st_info;        /* Symbol type and binding */
  unsigned char st_other;       /* Symbol visibility */
  Elf32_Section st_shndx;       /* Section index */
  } Elf32_Sym;
  
  /* Section header.  */
  typedef struct
  {
  Elf32_Word    sh_name;        /* Section name (string tbl index) */
  Elf32_Word    sh_type;        /* Section type */
  Elf32_Word    sh_flags;       /* Section flags */
  Elf32_Addr    sh_addr;        /* Section virtual addr at execution */
  Elf32_Off sh_offset;      /* Section file offset */
  Elf32_Word    sh_size;        /* Section size in bytes */
  Elf32_Word    sh_link;        /* Link to another section */
  Elf32_Word    sh_info;        /* Additional section information */
  Elf32_Word    sh_addralign;       /* Section alignment */
  Elf32_Word    sh_entsize;     /* Entry size if section holds table */
  } Elf32_Shdr;
  
  typedef struct
  {
  unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
  Elf32_Half    e_type;         /* Object file type */
  Elf32_Half    e_machine;      /* Architecture */
  Elf32_Word    e_version;      /* Object file version */
  Elf32_Addr    e_entry;        /* Entry point virtual address */
  Elf32_Off e_phoff;        /* Program header table file offset */
  Elf32_Off e_shoff;        /* Section header table file offset */
  Elf32_Word    e_flags;        /* Processor-specific flags */
  Elf32_Half    e_ehsize;       /* ELF header size in bytes */
  Elf32_Half    e_phentsize;        /* Program header table entry size */
  Elf32_Half    e_phnum;        /* Program header table entry count */
  Elf32_Half    e_shentsize;        /* Section header table entry size */
  Elf32_Half    e_shnum;        /* Section header table entry count */
  Elf32_Half    e_shstrndx;     /* Section header string table index */
  } Elf32_Ehdr;
  ```

- 我注意到elf.c中重要的变量都加了static声明，不希望其它文件引用这些变量，为了保证代码的健壮性，我不改掉这里的static，而是在elf.c中编写接口函数，方便expr函数调用。

- 首先检查符号表中每一个符号的类型是否为STT_OBJECT，strtab表示字符串表的首地址，字符串表数组中的元素的st_name属性表示字符串表中的偏移，相加就是符号表中对应元素在字符串表中的地址，将它和变量名相比较，如果相同，就返回st_value，表示变量地址。

### 打印栈帧链

- 打印栈帧链，首先按照PA手册定义一个PartOfStackFrame结构，这个结构对应一个current_frame变量，然后定义一个current_addr表示当前所在函数的首地址。
- 现将current_frame的返回地址设为当前eip，然后利用elf.c文件中的get_tmp函数获得当前指令所处于哪个函数，如果处于main，那么直接退出；否则，prev_ebp设为当前ebp所指向的内存内容，ret_addr设为ebp+4所指向的内存内容，即当前指令返回上一级函数时指令的地址，接着向上读取4个参数，输出5个参数。
- 之后要将current_addr设为prev_ebp的值，然后进入循环重复上述过程，直到current_addr值为0，代表结束。
