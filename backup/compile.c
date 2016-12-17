//
// Created by chaoyan on 2016/11/4.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define   word_max      13      //保留字个数
#define   num_max       14      //整数上限
#define   tab_max       200     //符号表长度
#define   ident_max     20      //标识符长度上限
#define   code_max      300     //目标代码数组长度
#define   string_max    50      //字符串长度上限
#define   stack_max     1000     //运行栈长度上限


typedef char identifier[ident_max];

typedef enum {
    intcon,floatcon,charcon,stringcon,
    plus, minus,times,slash, lss, leq, gtr, geq, neq, eql,
    nul, ident,constsy,intsy, floatsy, charsy, lparent, rparent,
    lbrack,rbrack,lbrace,rbrace,comma,semicolon,becomes,
    voidsy,mainsy,ifsy,elsesy,whilesy,forsy,
    returnsy,scanfsy,printfsy
} symbol;

typedef union{
    int inum;
    float fnum;
}tab_addr;

typedef union{
    int inum;
    float fnum;
    char str[string_max];
}a_entry;

typedef enum{
    ldc, opr, lod, sto, cal, inc, jmp, jpc, red, wrt, init_p, init_main, lda, sta
} fct;
/*
 * ldc 0 a 加载整数常数
 * ldc 1 a 加载字符常数
 * ldc 2 a 加载浮点常数
 * opr 0 返回
 * opr 1 取反
 * opr 2 加法
 * opr 3 减法
 * opr 4 乘法
 * opr 5 除法
 * opr 6 判断奇数操作： 奇1偶0
 * opr 8 判断相等，相等1，不相等0
 * opr 9 判断不相等，不相等1，不想等0
 * opr 10 判断小于
 * opr 11 判断大于等于
 * opr 12 判断大于
 * opr 13 判断小于等于
 * lod l a 取变量放到栈顶
 * lda l a 将数组放到栈顶
 * sto l a 将栈顶内容存入变量
 * sta l a 将栈顶内容写入数组中
 * cal l a 调用函数，层次差为0，函数在符号表中的登记位置是a
 * int 增加栈顶：调函数时分配空间，可以不用的
 * jmp 跳
 * jpc 栈顶是0跳，因为只有0才是假
 * red 读
 * wrt 0 0 写栈顶内容
 * wrt 1 a.str 写字符串
 * */

typedef enum {
    constant, variable, function
} object;

typedef enum {
    non, ints,floats,chars,arrays
} types;

typedef struct {
    fct f;//指令助记符
    int l;//l:层数
    a_entry a;  //float的情况是为了加载浮点常数和常量的情况而设计的
} instruction;

typedef struct{
    types arr_ele_typ;
    int arr_size;
    types return_typ;
    int params;
}sp_ref;

typedef struct{
    types type;
    int inum;
    float fnum;
}stack_entry;


typedef struct {
    identifier name;
    object obj;
    types typ;
    int level;
    /*
     * 对于变量和参数(包括数组)，此处填入在运行栈的相对地址(dx)；
     * 对于函数名，应记录目标代码的入口，即code表中的索引；
     * 对于常量，这里写值。
     */
    tab_addr addr;  //有两种可能的情况：整数和浮点数，其中浮点数仅适用于浮点数常量
    /*
     * 标识符为数组时，填入数组向量登记表中的位置；
     * 标识符为函数时。填入在btab中的位置；
     * 其他情况ref一律为0；
     */
    sp_ref ref;
} entry_tab;

/*全局变量定义区*/

identifier id, tmp, word[word_max];
char ch=' ', line[100], file[50], err_msg[60][50], str[string_max];    //当前读到字符
symbol sym, wordsym[word_max], symset[10]; //当前读到的符号助记符
int int_num, cc, ll, cx, line_count, err, tx, _global_array_marker = 0;
stack_entry s[stack_max];
float float_num;
instruction code[code_max];
FILE *fp;

entry_tab tab[tab_max];

/*函数声明区*/
int parameterlist(int level, int dx);
void constdeclaration(int level);
int variabledeclaration(int level, symbol type, int dx);
int var(int level, symbol type, int dx);
void factor(int level);
void term(int level);
void expression(int level);
void functioncall(int level, int pos);
void condition(int level);
void conditionstatement(int level);
void whilestatement(int level);
void forstatement(int level);
void scanfstatement(int level);
void printfstatement(int level);
void statement(int level);
void statements(int level);
void compositestatements(int level, int dx);
void functiondeclaration(symbol returntype, int level);
void mainfunction(int level);
void program(int level);

void tolowercase(identifier name){
    int i;
    for (i = 0; i < ident_max; i++){
        if (name[i]>='A' && name[i] <= 'Z')
            name[i] = name[i] - 'A' + 'a';
    }
}
int ten_based_pow(int times){
    int result = 10;
    for (;times>1;times--){
        result *= 10;
    }
    if (times == 0){
        return 1;
    }else{
        return result;
    }
}
int isUpperLetter(char ch){
    if (ch >='A' && ch <= 'Z')
        return 1;
    else
        return 0;
}
int isLowerCase(char ch){
    if (ch >='a' && ch <= 'z')
        return 1;
    else
        return 0;
}
int isDigit(char ch){
    if (ch >= '0' && ch <= '9')
        return 1;
    else
        return 0;
}
void error(int errnum){
    //错误的位置需要更高级的判定
    char tmp[30]={'\0'};
    int loc = cc;
    err++;
    /*
    intcon,floatcon,charcon,stringcon,
    plus, minus,times,slash, lss, leq, gtr, geq, neq, eql,
    nul, ident,constsy,intsy, floatsy, charsy, lparent, rparent,
    lbrack,rbrack,lbrace,rbrace,comma,semicolon,becomes,
    voidsy,mainsy,ifsy,elsesy,whilesy,forsy,
    returnsy,scanfsy,printfsy
     */

    switch (sym){
        case ident:
            loc -= strlen(id);
            break;
        case voidsy:
            loc -= 4;
            break;
        case mainsy:
            loc -= 4;
            break;
        case ifsy:
            loc -= 2;
            break;
        case elsesy:
            loc -= 4;
            break;
        case whilesy:
            loc -= 5;
            break;
        case forsy:
            loc -= 3;
            break;
        case returnsy:
            loc -= 6;
            break;
        case scanfsy:
            loc -=5;
            break;
        case printfsy:
            loc -= 6;
            break;
        case constsy:
            loc -= 5;
            break;
        case intsy:
            loc -= 3;
            break;
        case floatsy:
            loc -= 5;
            break;
        case charsy:
            loc -= 4;
            break;
        case stringcon:
            loc -= strlen(str)+2;
            break;
        case charcon:
            loc -= 3;
            break;
        case intcon:
            sprintf(tmp,"%d",int_num);
            loc -= strlen(tmp);
            break;
        case floatcon:
            sprintf(tmp,"%f",float_num);
            loc -= strlen(tmp);
            break;
        default:
            loc -= 1;
            break;

    }
    printf("%-3d %s",line_count,line);
    printf("%*c^ Error: %s\n", loc+3,' ', err_msg[errnum]);
}

void getch(){
    int i;
    if (cc == ll){ //读到文件最后一行
        if (feof(fp)){
            printf("程序结束\n");
            fclose(fp);
        }
        line_count++;
        ll = 0;
        cc = 0;
        for (i = 0; i < 100; i++){
            line[i] = '\0';
        }
        do {
            i = fscanf(fp, "%c", line+ll);
            ll++;
        }while(line[ll-1]!='\n' && i != EOF);   //ll比实际的字符数大1
    }
    ch = line[cc++];
}
void getsym(){
    int i;
    while(ch==' ' || ch =='\t' || ch =='\r' || ch == '\n'){//跳过空白的区域
        getch();
    }
    if (ch=='_' || isUpperLetter(ch)||isLowerCase(ch)){//标识符
        for (i = 0; i < ident_max; i++){
            tmp[i] = '\0';  //清空上一次的痕迹
        }
        i = 0;
        do{
            if (i >= ident_max){
                error(5);
            }else{
                tmp[i++] = ch;
            }
            getch();
        } while (ch=='_' || isUpperLetter(ch) || isLowerCase(ch) || isDigit(ch));
        strcpy(id,tmp);
        tolowercase(id);
        sym = ident;
        for (i = 0; i < word_max; i++){//检查是否是保留字
            if (strcmp(word[i],id)==0){
                sym = wordsym[i];
                break;
            }
        }
    }
    else if (isDigit(ch)){ //数字：无符号整数从0到Nan；0属于整数，且只能有一个0。
        if (ch == '0'){//只有一个0的整数的情况
            int_num = 0;
            sym = intcon;
            getch();
            if (isDigit(ch)){
                error(7);
            }
        } else{
            int_num = 0;
            i = 0;
            sym = intcon;
            do{
                i++;
                int_num = 10*int_num + ch-'0';
                getch();
            }while(isDigit(ch));
            if (i > num_max){
                error(2);
            }
        }
        if (ch == '.'){ //说明是浮点数
            sym = floatcon;
            float_num = int_num; //浮点数整数部分赋值
            int_num = 0;
            i = 0;
            getch();
            if (isDigit(ch)){
                do{
                    i++;
                    int_num = 10*int_num + ch - '0';
                    getch();
                }while(isDigit(ch));
                if (i > num_max){
                    error(2);
                }
                float_num += (float)int_num/ten_based_pow(i);
            }
        }
    }
    else if (ch == '\''){ //字符常量
        sym = charcon;
        getch();
        if (ch =='+' || ch == '-' || ch =='*' || ch == '/' || ch == '_'
            || isUpperLetter(ch) || isLowerCase(ch)|| isDigit(ch)){
            int_num = ch;   //用int表示char
        }else{
            int_num = 0;    //空字符
            error(0);
        }
        getch();
        if (ch == '\''){
            getch();
        }else{
            error(1);
        }
    }
    else if (ch == '\"'){//字符串常量
        sym = stringcon;
        for (i = 0 ; i < string_max; i++){
            str[i] = '\0';
        }
        i = 0;
        getch();
        while(ch != '\"'){
            if (ch>=32&&ch<=126&&ch!=35){
                if (i >= string_max){
                    error(4);
                }else{
                    str[i] = ch;
                    i++;    //字符串长度记录
                }
            }else{
                error(3);
            }
            getch();
        }
        getch();
    }
    else if(ch == '='){
        getch();
        if (ch == '='){
            sym = eql;
            getch();
        }else{
            sym = becomes;
        }
    }
    else if (ch == '<'){
        getch();
        if (ch == '='){
            sym = leq;
            getch();
        }else{
            sym = lss;
        }
    }
    else if (ch == '>'){
        getch();
        if (ch == '='){
            sym = geq;
            getch();
        }else{
            sym = gtr;
        }
    }
    else if(ch == '!'){
        getch();
        if (ch == '='){
            sym = neq;
            getch();
        }else{
            error(6);
        }
    }
    else{
        switch(ch){
            case '+': sym = plus; break;
            case '-': sym = minus; break;
            case '/': sym = slash; break;
            case '*': sym = times; break;
            case '(': sym = lparent; break;
            case ')': sym = rparent; break;
            case '[': sym = lbrack; break;
            case ']': sym = rbrack; break;
            case '{': sym = lbrace; break;
            case '}': sym = rbrace; break;
            case ',': sym = comma; break;
            case ';': sym = semicolon; break;
            default : sym = nul; break;
        }
        getch();

    }
}

void print_symbols() {
    int counter_limit = 500, nums=0, i = 0;
    char *sym_print[] = {
            "intcon","floatcon","charcon","stringcon", "plus",
            "minus","times","slash", "lss", "leq", "gtr", "geq",
            "neq", "eql", "nul", "ident","constsy","intsy", "floatsy",
            "charsy","lparent", "rparent", "lbrack","rbrack","lbrace",
            "rbrace","comma","semicolon","becomes", "voidsy","mainsy",
            "ifsy","elsesy","whilesy","forsy", "returnsy","scanfsy","printfsy"
    };

    while(counter_limit){
        counter_limit--;
        getsym();
        nums++;
        if (i < line_count){
            i = line_count;
            printf ("********** Line %d **********\n", line_count);
        }

        if (sym == intcon){
            printf("%-5d %-15s %d\n",nums, sym_print[sym],int_num);
        }
        else if (sym == charcon){
            printf("%-5d %-15s %c\n",nums, sym_print[sym],int_num);
        }
        else if(sym == floatcon){
            printf("%-5d %-15s %f\n",nums, sym_print[sym],float_num);
        }
        else if (sym == stringcon){
            printf("%-5d %-15s %s\n",nums, sym_print[sym],str);
        }
        else if (sym == ident || sym == constsy || sym == intsy || sym == floatsy ||
                 sym == charsy || sym == voidsy || sym == voidsy || sym == mainsy ||
                 sym == ifsy || sym == elsesy || sym == elsesy || sym == forsy || sym == whilesy ||
                 sym == returnsy || sym == scanfsy|| sym == printfsy){
            printf("%-5d %-15s %s\n",nums, sym_print[sym],id);
        }
        else{
            switch (sym){
                case plus:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"+");
                    break;
                case minus:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"-");
                    break;
                case slash:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"/");
                    break;
                case times:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"*");
                    break;
                case lss:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"<");
                    break;
                case leq:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"<=");
                    break;
                case gtr:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],">");
                    break;
                case geq:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],">=");
                    break;
                case neq:
                    printf("%-5d, %-15s %s\n",nums, sym_print[sym],"!=");
                    break;
                case eql:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"==");
                    break;
                case nul:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"nul");
                    break;
                case lparent:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"(");
                    break;
                case rparent:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],")");
                    break;
                case lbrack:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"[");
                    break;
                case rbrack:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"]");
                    break;
                case lbrace:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"{");
                    break;
                case rbrace:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"}");
                    break;
                case comma:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],",");
                    break;
                case semicolon:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],";");
                    break;
                case becomes:
                    printf("%-5d %-15s %s\n",nums, sym_print[sym],"=");
                    break;
                default:
                    printf("Shouldn't be here!\n");
                    break;

            }
        }

    }
}
void print_table(){
    int i;
    printf("%-20s %-20s %-20s %-20s %-20s %-20s\n", "name", "obj", "typ", "level", "addr", "ref");
    for (i = 0; i < tx; i++){
        if (tab[i].obj == constant){
            if (tab[i].typ == floats)
                printf("%-20s %-20s %-20s %-20d %-20f %-20d\n", tab[i].name,"constant", "floats", tab[i].level, tab[i].addr.fnum, 0);
            else if (tab[i].typ == ints)
                printf("%-20s %-20s %-20s %-20d %-20d %-20d\n", tab[i].name,"constant", "ints", tab[i].level, tab[i].addr.inum, 0);
            else if (tab[i].typ == chars)
                printf("%-20s %-20s %-20s %-20d %-20c %-20d\n", tab[i].name,"constant", "chars", tab[i].level, tab[i].addr.inum, 0);
            else
                printf("unrecognized\n");
        }
        else if(tab[i].obj == variable) {
            if (tab[i].typ == floats)
                printf("%-20s %-20s %-20s %-20d %-20d %-20d\n", tab[i].name,"variable", "floats", tab[i].level, tab[i].addr.inum, 0);
            else if (tab[i].typ == ints)
                printf("%-20s %-20s %-20s %-20d %-20d %-20d\n", tab[i].name,"variable", "ints", tab[i].level, tab[i].addr.inum, 0);
            else if (tab[i].typ == chars)
                printf("%-20s %-20s %-20s %-20d %-20d %-20d\n", tab[i].name,"variable", "chars", tab[i].level, tab[i].addr.inum, 0);
            else if (tab[i].typ == arrays)
                printf("%-20s %-20s %-20s %-20d %-20d %-20s %-20d\n", tab[i].name,"variable", "arrays", tab[i].level, tab[i].addr.inum,
                       (tab[i].ref.arr_ele_typ == ints)? "ints": (tab[i].ref.arr_ele_typ == floats)? "floats" : "chars",tab[i].ref.arr_size );
            else
                printf("unrecognized\n");
        }
        else if (tab[i].obj == function){
            printf("%-20s %-20s %-20s %-20d %-20d %-20d %-20s\n", tab[i].name,"function", "non", tab[i].level, tab[i].addr.inum,
                   tab[i].ref.params, (tab[i].ref.return_typ == ints) ? "ints": (tab[i].ref.return_typ == floats) ? "floats" : (tab[i].ref.return_typ == chars) ? "chars" : "non");
        }
    }
}
void print_pcode(){
    int i;
    char *fun[] = {"ldc", "opr", "lod", "sto", "cal", "inc", "jmp", "jpc", "red", "wrt", "init_p", "init_main", "lda", "sta"};
    for (i = 0; i < cx; i++){
        if (code[i].f==ldc && code[i].l == 2){
            printf("%-10d %-10s %-10d %-10f\n", i, fun[code[i].f], code[i].l, code[i].a.fnum);
        }
        else if (code[i].f==ldc && code[i].l == 1){
            printf("%-10d %-10s %-10d %-10c\n", i, fun[code[i].f], code[i].l, code[i].a.inum);
        }
        else if (code[i].f==wrt && code[i].l == 1){
            printf("%-10d %-10s %-10d %-10s\n", i, fun[code[i].f], code[i].l, code[i].a.str);
        }
        else{
            printf("%-10d %-10s %-10d %-10d\n", i, fun[code[i].f], code[i].l, code[i].a.inum);
        }

    }
}

void skip(int length){
    int i;
    while(1) {
        for (i = 0; i < length; i++) {
            if (sym == symset[i]) { //在跳读结束标识的数组内，返回
                return;
            }
        }
        getsym();
    }
}

void gen(fct f, int l, a_entry a){
    if (cx >= code_max){
        printf("Program too long!\n");
        exit(1);
    }
    else{
        code[cx].f = f;
        code[cx].l = l;
        code[cx].a = a;
        cx++;
    }
}

int position(identifier name){
    int i;
    int marker = 0;

    for (i = tx-1; i>0; i -- ){
        if (tab[i].obj == function){  //出现过一个函数
            marker = 1;
        }
        if (strcmp(tab[i].name, name) == 0){
            if (tab[i].obj != function && marker == 1 && tab[i].level != 0){ //找到的是常量或变量，然而已经跨函数，而且不在外层（即不是全局的）
                continue;   //注意这里是continue！！
            }
            return i;
        }
    }
    return 0;
}

int getfunctionpos(){
    int i;
    for (i = tx -1; i>0 ; i--){
        if (tab[i].obj == function){
            return i;
        }
    }
    return 0;
}

void enter_tab(object objtyp, symbol type, int level, int dx){  //存符号表时全部转为小写
    /* addr:
     * 对于变量和参数(包括数组)，此处填入在运行栈的相对地址(dx)；
     * 对于函数名，应记录目标代码的入口，即code表中的索引，即该子程序开始的地址；
     * 对于普通常量，这里写值。
     */
    /* ref:
     * 标识符为数组时，填入数组向量元素类型和数组长度；
     * 标识符为函数时。参数个数和返回值类型
     * 其他情况ref无意义；
     */
    switch(objtyp){
        case constant:  //常量加入符号表
            tab[tx].obj = objtyp;       //类型是常数
            tolowercase(id);
            strcpy(tab[tx].name, id);   //常数名，全部转换为小写
            tab[tx].level = level;      //常数所在层数
            //tab[tx].ref = 0;
            switch (type){
                case intsy:
                    tab[tx].typ = ints; //常熟类型
                    tab[tx].addr.inum = int_num;    //常数值
                    break;
                case floatsy:
                    tab[tx].typ = floats;
                    tab[tx].addr.fnum = float_num;
                    break;
                case charsy:
                    tab[tx].typ = chars;
                    tab[tx].addr.inum = int_num;
                    break;
                default:
                    printf("How come?\n");
                    break;
            }
            break;
        case variable:
            tab[tx].obj = objtyp;       //类型是变量
            tolowercase(id);
            strcpy(tab[tx].name, id);   //变量名，全部转换为小写
            tab[tx].level = level;      //变量所在层数
            //tab[tx].ref = 0;
            if (_global_array_marker == 1){    //种类是数组
                tab[tx].typ = arrays;
                tab[tx].addr.inum = dx;
                tab[tx].ref.arr_ele_typ = (type == intsy)?ints:(type==floatsy)?floats:chars;
                tab[tx].ref.arr_size = int_num;
            }else {
                switch (type){
                    case intsy:
                        tab[tx].typ = ints; //变量类型
                        tab[tx].addr.inum = dx;    //变量偏移
                        break;
                    case floatsy:
                        tab[tx].typ = floats;
                        tab[tx].addr.inum = dx;
                        break;
                    case charsy:
                        tab[tx].typ = chars;
                        tab[tx].addr.inum = dx;
                        break;
                    default:
                        printf("How come2 ?\n");
                        break;
                }
            }
            break;
        case function:
            tab[tx].obj = objtyp;       //类型是变量
            tolowercase(id);
            strcpy(tab[tx].name, id);   //变量名，全部转换为小写
            tab[tx].level = level;      //变量所在层数
            tab[tx].typ = non;

    }
    tx++;   //符号表头指针上移

}

int parameterlist(int level, int dx){
    symbol type;
    while(1) {
        if (sym == intsy || sym == floatsy || sym == charsy) {
            type = sym;
            getsym();
            if (sym == ident) {
                enter_tab(variable, type, level, dx);
                dx++;
                getsym();
            } else {
                error(50);
                symset[0] = comma, symset[1] = rparent;
                skip(2); //出错后跳到下一个逗号、右括号
            }
        }
        if (sym == comma){
            getsym();
        }else{
            break;
        }
    }
    return dx;
}

void constdeclaration(int level){
    int rate, counter, pos;
    symbol type;
    if (sym == intsy || sym == floatsy || sym == charsy){//const后面跟类型符
        type = sym; //记录常量的类型
        do{
            rate = 0, counter = 0;//正负号计数器和初始化
            getsym();
            if (sym == ident){ //标识符, 常量名
                pos = position(id);
                if (pos!=0&&tab[pos].level==level){   //出现同名变量，且在同一层:同一层意味着作用域相同
                    error(29);
                    symset[0] = comma, symset[1] = semicolon;   //和当前作用域重名
                    skip(2);
                }
                getsym();
                if (sym == becomes){
                    getsym();//下面是整数，实数或字符
                    while (sym == plus || sym == minus){
                        counter++;
                        if (sym == plus){
                            rate = (rate == 0) ? +1 : rate*1;
                        }
                        else{
                            rate = (rate == 0) ? -1 : rate*-1;
                        }
                        getsym();
                    }

                    //整型
                    if (sym == intcon){
                        if (counter  > 1){ //整数前面最多就一个符号
                            error(9);
                        }
                        if (rate != 0){//说明有符号了
                            int_num *= rate;//更新整数数值
                        }
                        if (type == floatsy){//整数转换成浮点数
                            float_num = (float) int_num;
                        }
                        else if (type == charsy){//将整数直接赋给字符，要检查有无越界
                            if (int_num=='+'||int_num=='-'||int_num ==
                                    '*'||int_num == '/'||int_num == '_'||isLowerCase(int_num) ||
                                    isUpperLetter(int_num)||isDigit(int_num)){//字符的范围有限，不许越界
                                printf("%d: Implicit transformation", line_count);//隐式转换
                            }else{
                                int_num = 0;
                                error(11);
                            }
                        }
                        enter_tab(constant, type, level, 0);
                    }
                    else if (sym == charcon){//字符型
                        if (counter > 0){
                            error(9);
                        }

                        if (type == intsy){
                            printf("%d: Implicit transformation", line_count);//隐式转换
                        }
                        if (type == floatsy){//将字符赋给浮点的情况...
                            float_num = (float) int_num;
                        }
                        enter_tab(constant, type, level, 0);
                    }
                    else if (sym == floatcon) { //隐式转换
                        if (counter > 2){
                            error(9);
                        }

                        if (rate!=0){
                            float_num *= rate;
                        }

                        if (type == intsy){
                            printf("%d: Implicit transformation", line_count);//隐式转换
                            int_num = (int)float_num;
                        }
                        if (type == charsy){
                            int_num = (int)float_num;
                            if (int_num=='+'||int_num=='-'||int_num == '*'||int_num == '/'||
                                    int_num == '_'||isLowerCase(int_num) ||
                                    isUpperLetter(int_num)||isDigit(int_num)){//ASC2字符，且无越界
                                printf("%d: Implicit transformation", line_count);//隐式转换
                            }else{
                                int_num = 0;
                                error(11);
                            }
                        }
                        enter_tab(constant, type, level, 0);
                    }
                    else{
                        printf("%s, %d",id, sym);
                        error(7);
                    }
                    getsym();
                }
                else{
                    error(6);
                    symset[0] = comma, symset[1] = semicolon;   //常量未赋值
                    skip(2);
                }
            }else{
                error(15);
                symset[0] = comma, symset[1] = semicolon;   //常量的名称不正确
                skip(2);
            }
        }while(sym == comma);
    }
    else{
        error(8);
        symset[0] = semicolon;  //const说明中没有表明类型，那么只好全部跳过
        skip(1);
    }
}

int variabledeclaration(int level, symbol type, int dx){
    int pos;
    while(1){
        if (sym == ident){
            pos = position(id);
            if (pos!=0&&tab[pos].level==level){   //出现同名变量，且在同一层:同一层意味着作用域相同
                error(29);
                symset[0] = comma, symset[1] = semicolon;   //和当前作用域重名
                skip(2);
            }
            getsym();
            if (sym == lbrack){
                _global_array_marker = 1;
                getsym();
                if (sym != intcon || int_num == 0){
                    error(16);
                    sym = intcon;   //改正错误
                    int_num = 1;    //改正错误
                    symset[0] = rbrack, symset[1] = comma, symset[2] = semicolon;   //和当前作用域重名
                    skip(3);
                }
                enter_tab(variable,type,level,dx);
                _global_array_marker = 0;
                dx += int_num;
                getsym();
                if (sym == rbrack){
                    getsym();
                }else{
                    error(17);
                    symset[0] = rbrack, symset[1] = comma, symset[2] = semicolon;   //和当前作用域重名
                    skip(3);
                }
            }
            else{
                enter_tab(variable,type,level,dx);
                dx++;
            }
        }else{
            error(15);
            symset[0] = comma, symset[1] = semicolon;   //和当前作用域重名
            skip(2);
        }

        if (sym == comma) {
            getsym();
        }else{
            break;
        }
    }

    return dx;

}

int var(int level, symbol type, int dx){
    if (sym == lbrack){ //说明是数组
        _global_array_marker = 1;
        getsym();
        if (sym != intcon || int_num == 0){
            error(16);
            sym = intcon;
            int_num = 1;
            symset[0] = rbrack, symset[1] = comma, symset[2] = semicolon;   //和当前作用域重名
            skip(3);
        }
        enter_tab(variable,type,level,dx);
        _global_array_marker = 0;
        dx += int_num;
        getsym();
        if (sym == rbrack){
            getsym();
            if (sym == comma) {
                getsym();
                return variabledeclaration(level, type, dx);
            }
        }else{
            error(17);
            symset[0] = rbrack, symset[1] = comma, symset[2] = semicolon;   //和当前作用域重名
            skip(3);
        }
    }
    else if (sym == semicolon){
        enter_tab(variable,type,level,dx);
        dx++;   //普通变量dx加1
        return dx;
    }
    else if (sym == comma ){
        enter_tab(variable,type,level,dx);
        dx++;   //普通变量dx加1
        getsym();
        return variabledeclaration(level,type,dx);
    }
    return dx;
}

void factor(int level){
    a_entry tmp;
    int rate = 0, pos;
    while(sym == plus || sym == minus){ //因子前面可能会有多余的符号
        if (rate == 0){
            rate = 1;
        }
        if (sym == minus){
            rate *= -1;
        }
        getsym();
    }

    if (sym == ident){//常量、变量 （含数组）或函数调用
        if (rate != 0){
            //变量前面出现符号，文法是不允许的
            //常量前面出现一到两个符号是可以的
            error(21);
        }
        pos = position(id);
        //返回的pos是在作用域内可见的同名常量、变量或函数
        if (pos == 0){
            error(22);
            symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
                    symset[4]=semicolon;
            skip(5);
        }
        if (tab[pos].obj == constant){
            if (tab[pos].typ == ints){
                tmp.inum = tab[pos].addr.inum;
                gen(ldc,0,tmp); //加载整／字符const
            }
            else if (tab[pos].typ == chars){
                tmp.inum = tab[pos].addr.inum;
                gen(ldc,1,tmp);// 加载字符const
            }
            else if (tab[pos].typ == floats){
                tmp.fnum = tab[pos].addr.fnum;
                gen(ldc,2,tmp);// 加载浮点const
            }
            getsym();
        }
        else if (tab[pos].obj == variable){
            if (tab[pos].typ == arrays){
                getsym();
                if (sym == lbrack){
                    getsym();
                    expression(level);
                    tmp.inum = tab[pos].addr.inum;
                    gen(lda,level-tab[pos].level,tmp);  //加载数组指令
                    if (sym == rbrack){
                        getsym();
                    }
                    else{
                        error(17);
                        symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
                        symset[4]=semicolon;
                        skip(5);
                    }
                }
                else{
                    error(50);  //数组类型引用错误
                    symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
                    symset[4]=semicolon;
                    skip(5);
                }
            }
            else{
                tmp.inum = tab[pos].addr.inum;  //记录偏移位置
                gen(lod,level-tab[pos].level,tmp);
                getsym();
            }
        }
        else if (tab[pos].obj == function){
            if (tab[pos].ref.return_typ == non){    //无返回值的函数不允许出现在因子里
                error(24);
                symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
                symset[4]=semicolon;
                skip(5);
            }
            else{   //有返回值的情况会出现在这里
                getsym();
                functioncall(level, pos);
            }
        }
        else{
            printf("How?\n");
        }

    }
    else if (sym == intcon){
        tmp.inum = int_num;
        gen(ldc,0,tmp);
        getsym();
    }
    else if (sym == floatcon){
        tmp.fnum = float_num;
        gen(ldc,2,tmp);
        getsym();
    }
    else if (sym == charcon){
        tmp.inum = int_num;
        gen(ldc,1,tmp);
        getsym();
    }
    else if (sym == lparent){
        getsym();
        expression(level);
        if (sym == rparent){
            getsym();
        }else{
            error(18);
            symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
            symset[4]=semicolon;
            skip(5);
        }
    }

    if (rate == -1){    //根据多余的正负号个数取反
        tmp.inum = 1;
        gen(opr,0,tmp);
    }
};
void term(int level){
    symbol mulop;
    a_entry tmp;
    factor(level);
    while(sym == times || sym == slash){
        mulop = sym;
        getsym();
        factor(level);
        if (mulop == times){
            tmp.inum = 4;
            gen(opr,0,tmp);
        }else{
            tmp.inum = 5;
            gen(opr,0,tmp);
        }
    }
}
void expression(int level){
    symbol addop;
    a_entry tmp;
    if (sym == plus || sym == minus){
        addop = sym;
        getsym();
        term(level);
        if (addop == minus){
            tmp.inum = 1;
            gen(opr,0,tmp);//取反指令，从pl/0源代码学来的
        }
    }
    else{
        term(level);
    }

    while (sym == plus || sym == minus){
        addop = sym;
        getsym();
        term(level);
        if (addop == plus){
            tmp.inum = 2;
            gen(opr,0,tmp);   //加法
        }
        else{
            tmp.inum = 3;
            gen(opr,0,tmp);   //减法
        }
    };
}

//TODO
void functioncall(int level, int pos){  //pos函数在tab中位置
    a_entry tmp;
    int num_params, counter;
    if (sym == lparent){
        if (tab[pos].ref.params == 0){
            getsym();
        }else{
            num_params = tab[pos].ref.params;
            for(counter = 0; ; ){
                counter++;
                getsym();
                expression(level);
                if (sym != comma){
                    break;
                }
            }
            if (counter != num_params){
                error(28);
            }
        }

        if (sym == rparent){
            tmp.inum = pos;
            gen(cal,level-tab[pos].level,tmp);
            //tmp里面放的是函数在符号表中的登记位置,可能还要负担一些填参数的工作
            //新函数的入口地址，这里level-tab[pos].level好像是有公式的
            getsym();
        }else{
            error(17);
            symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
            symset[4]=semicolon;
            skip(5);
        }
    }
}

void condition(int level){
    symbol relop;
    a_entry tmp;
    expression(level);
    if (sym == lss || sym == leq || sym == gtr || sym == geq || sym == neq || sym == eql){
        //关系运算符
        relop = sym;
        getsym();
        expression(level);
        switch(relop){
            case lss:
                tmp.inum = 10;
                gen(opr,0,tmp);
                break;
            case leq:
                tmp.inum = 13;
                gen(opr,0,tmp);
                break;
            case gtr:
                tmp.inum = 12;
                gen(opr,0,tmp);
                break;
            case geq:
                tmp.inum = 11;
                gen(opr,0,tmp);
                break;
            case neq:
                tmp.inum = 9;
                gen(opr,0,tmp);
                break;
            case eql:
                tmp.inum = 8;
                gen(opr,0,tmp);
                break;
            default:
                error(30);
                break;
        }
    }
}

void conditionstatement(int level){
    a_entry tmp;
    int cx1, cx2;
    if (sym == ifsy){
        getsym();
        if (sym == lparent){
            getsym();
            condition(level);
            /*生成条件条件跳转*/
            if (sym == rparent){
                getsym();
                cx1 = cx; //记住下面这条jpc指令在code中的索引
                tmp.inum = 0;
                gen(jpc,0,tmp);
                statement(level);//条件正确时走这边，里面可能也有if，else邮件由里面的匹配
                if (sym == elsesy){
                    // 有else，那么就要确定前面的if执行完后跳过else的内容
                    cx2 = cx;
                    tmp.inum = 0;
                    gen(jmp,0,tmp);
                    code[cx1].a.inum = cx;

                    getsym();
                    statement(level);
                    code[cx2].a.inum = cx;
                }
                else{
                    //本statement的if执行结束后面没有else
                    code[cx1].a.inum = cx;
                }

            } else{
                error(18);
            }
        }
        else{
            error(50);
            //这里写的可能不好
            symset[0] = semicolon, symset[1] = elsesy;
            skip(2);
            getsym();
        }
    }
}

void whilestatement(int level){
    int cx1, cx2;
    a_entry tmp;
    if (sym == whilesy){
        getsym();
        if (sym == lparent){
            getsym();
            cx1 = cx;   //循环跳回的位置
            condition(level);
            cx2 = cx;//jpc指令在code中的索引
            tmp.inum = 0;
            gen(jpc,0,tmp);
            if (sym == rparent){
                getsym();
                statement(level);
                tmp.inum = cx1;
                gen(jmp,0,tmp); //无条件跳回循环(条件)开始的地方
                code[cx2].a.inum = cx;//jpc跳出循环的位置就是下一个cx的位置
            }else{
                error(17);
                symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                skip(3);
                getsym();
            }
        }else{
            error(50);
            symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
            skip(3);
            getsym();
        }
    }
}

void forstatement(int level){
    a_entry tmp;
    symbol addop;
    int pos, pos_1, pos_2, adder, cx1, cx2;
    if (sym == forsy){
        getsym();
        if (sym == lparent){
            getsym();
            if (sym == ident){
                pos = position(id);
                if (pos == 0){
                    error(22);
                }else{
                    if (tab[pos].obj == variable &&
                            (tab[pos].typ == ints || tab[pos].typ == floats || tab[pos].typ == chars)){
                        getsym();
                        if (sym == becomes){
                            getsym();
                            expression(level);
                            tmp.inum = tab[pos].addr.inum;
                            gen(sto,level-tab[pos].level, tmp); //赋值语句的sto
                            if (sym == semicolon){
                                getsym();
                                cx1 = cx;   //循环跳回
                                condition(level);   //条件
                                cx2 = cx;
                                tmp.inum = 0;
                                gen(jpc,0,tmp);
                                if (sym == semicolon){
                                    getsym();
                                    if (sym == ident){
                                        pos_1 = position(id);
                                        if (tab[pos_1].obj == variable &&
                                              (tab[pos_1].typ == ints || tab[pos_1].typ == floats || tab[pos_1].typ == chars)){
                                            getsym();
                                            if (sym == becomes){
                                                getsym();
                                                if (sym == ident){
                                                    pos_2 = position(id);
                                                    if (tab[pos_2].obj == variable &&
                                                            (tab[pos_2].typ == ints || tab[pos_2].typ == floats || tab[pos_2].typ == chars)){
                                                        getsym();
                                                        if (sym == plus || sym == minus){
                                                            addop = sym;
                                                            getsym();
                                                            if (sym == intcon){
                                                                if (!(int_num > 0)){    //步长需为无符号整数: 大于0
                                                                    error(50);
                                                                    symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                                                    skip(3);
                                                                    getsym();
                                                                    return;
                                                                }
                                                                adder = int_num;
                                                                getsym();
                                                                if (sym == rparent){
                                                                    getsym();
                                                                    statement(level);
                                                                    tmp.inum = tab[pos_2].addr.inum;
                                                                    gen(lod,level-tab[pos_2].level,tmp);
                                                                    tmp.inum = adder;
                                                                    gen(ldc,0,tmp); //步长应该只能是整数
                                                                    if (addop == plus){
                                                                        tmp.inum = 2;
                                                                        gen(opr,0,tmp);
                                                                    }else{
                                                                        tmp.inum = 3;
                                                                        gen(opr,0,tmp);
                                                                    }
                                                                    tmp.inum = tab[pos_1].addr.inum;
                                                                    gen(sto,level-tab[pos_1].level,tmp);    //sto，更新
                                                                    tmp.inum = cx1;
                                                                    gen(jmp,0,tmp);
                                                                    code[cx2].a.inum = cx;
                                                                }
                                                            }
                                                        }else{
                                                            error(50);
                                                            symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                                            skip(3);
                                                            getsym();
                                                        }
                                                    }else{
                                                        error(50);
                                                        symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                                        skip(3);
                                                        getsym();
                                                    }
                                                }else{
                                                    error(50);
                                                    symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                                    skip(3);
                                                    getsym();
                                                }
                                            }else{
                                                error(6);
                                                symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                                skip(3);
                                                getsym();
                                            }
                                        }else{
                                            error(50);
                                            symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                            skip(3);
                                            getsym();
                                        }
                                    }
                                }else{
                                    error(50);
                                    symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                    skip(3);
                                    getsym();
                                }
                            }else{
                                error(50);
                                symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                                skip(3);
                                getsym();
                            }
                        }else{
                            error(6);
                            symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                            skip(3);
                            getsym();
                        }
                    }else{
                        error(50);
                        symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                        skip(3);
                        getsym();
                    }
                }
            }else{
                error(15);
                symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
                skip(3);
                getsym();
            }
        }else{
            error(50);
            symset[0] = semicolon, symset[1] = rparent, symset[2] = lbrace;
            skip(3);
            getsym();
        }
    }
}

void scanfstatement(int level){//TODO 检查
    int pos;
    a_entry tmp;
    if (sym == scanfsy){
        getsym();
        if (sym == lparent){
            do{
                getsym();
                if (sym == ident){
                    pos = position(id);
                    if (pos == 0){
                        error(22);
                        symset[0] = comma, symset[1] = rparent, symset[2] = semicolon;
                        skip(3);
                    }else if (!(tab[pos].obj == variable&&
                            (tab[pos].typ == ints || tab[pos].typ == floats || tab[pos].typ == chars))){
                        error(50);//不是非数组的变量类型
                        symset[0] = comma, symset[1] = rparent, symset[2] = semicolon;
                        skip(3);
                    }
                    else{
                        tmp.inum = tab[pos].addr.inum;
                        gen(red,level-tab[pos].level,tmp);
                        getsym();
                    }
                }else{
                    error(15);
                    symset[0] = comma, symset[1] = rparent, symset[2] = semicolon;
                    skip(3);
                }
            }while(sym == comma);
            if (sym == rparent){
                getsym();
            }else{
                error(17);
                symset[0] = semicolon;
                skip(1);
            }
        }

    }
}

void printfstatement(int level){    //TODO 检查
    a_entry tmp;
    if (sym == printfsy){
        getsym();
        if (sym == lparent){
            getsym();
            if (sym == stringcon){
                strcpy(tmp.str,str);
                gen(wrt,1,tmp);
                getsym();
                if (sym == comma){
                    getsym();
                    expression(level);
                    tmp.inum = 0;
                    gen(wrt,0,tmp);
                }
            }
            else{
                expression(level);
                tmp.inum = 0;
                gen(wrt,0,tmp);
            }
        }
    }

    if (sym == rparent){
        getsym();
    }else{
        error(17);
        symset[0] = semicolon;
        skip(1);
    }
}

void statement(int level){
    int pos, dx;
    a_entry tmp;
    if (sym == ifsy){
        conditionstatement(level);
    }
    else if (sym == whilesy){
        whilestatement(level);
    }
    else if (sym == forsy){
        forstatement(level);
    }
    else if (sym == lbrace){
        getsym();
        statements(level);
        if (sym == rbrace){
            getsym();
        }else{
            error(19);
        }
    }
    else if(sym == ident){  //TODO 赋值语句或函数调用语句
        pos = position(id);
        if (pos != 0){
            if (tab[pos].obj==variable){    //赋值语句
                if (tab[pos].typ == arrays){    //数组赋值
                    getsym();
                    if (sym == lbrack){
                        getsym();
                        expression(level);  //此时栈顶是数组下标
                        if (sym == rbrack){
                            getsym();
                            if (sym == becomes){
                                getsym();
                                expression(level);  //此时栈顶是要存入的值
                                tmp.inum = tab[pos].addr.inum;
                                gen(sta,level-tab[pos].level,tmp);
                            } else{
                                error(50);
                                symset[0]=semicolon;
                                skip(1);
                            }
                        }
                        else{
                            error(17);
                            symset[0]=plus,symset[1]=minus,symset[2]=times,symset[3]=slash,
                            symset[4]=semicolon;
                            skip(5);
                        }
                    }else{
                        error(50);
                        symset[0]=semicolon;
                        skip(1);
                    }
                }else{  //普通变量赋值
                    getsym();
                    if (sym == becomes){
                        getsym();
                        expression(level);
                        tmp.inum = tab[pos].addr.inum;
                        gen(sto,level-tab[pos].level,tmp);
                    }else{
                        error(50);
                        symset[0]=semicolon;
                        skip(1);
                    }
                }
            }
            else if (tab[pos].obj == function){ //TODO 调用函数
                getsym();
                functioncall(level,pos);
            }
            else if (tab[pos].obj == constant){
                error(25);
                symset[0]=semicolon;
                skip(1);
            }
        }else{
            error(22);
            symset[0]=semicolon;
            skip(1);
        }

        if (sym == semicolon){
            getsym();
        }else{
            error(13);
        }
    }
    else if (sym == scanfsy){
        scanfstatement(level);
        if (sym == semicolon){
            getsym();
        }else{
            error(13);
        }
    }
    else if (sym == printfsy){
        printfstatement(level);
        if (sym == semicolon){
            getsym();
        }else{
            error(13);
        }
    }
    else if (sym == returnsy){
        //TODO 检查
        getsym();
        pos = getfunctionpos();
        if (sym == lparent){
            if (tab[pos].ref.return_typ == non){
                error(26);  //该函数应该没有返回值，报错
                symset[0] = semicolon;
                skip(1);
            }else{
                getsym();
                expression(level);//返回值应该会存放在栈顶
                tmp.inum = 0;
                gen(opr,0,tmp);
                if (sym == rparent){
                    getsym();
                }else{
                    error(17);
                    symset[0] = semicolon;
                    skip(1);
                }
            }
        }else{
            if (tab[pos].ref.return_typ != non){    //==
                error(27);  //该函数应该有返回值,报错
                symset[0] = semicolon;
                skip(1);
            }else{
                tmp.inum = 0;
                gen(opr,0,tmp); //此时栈顶不会有返回值
            }
        }

        if (sym == semicolon){
            getsym();
        }else{
            error(13);
            symset[0] = semicolon;
            skip(1);
            getsym();
        }
    }
    else if (sym == semicolon){
        getsym();
    }
    else{
        error(50);
        symset[0] = semicolon;  //跳到下一个语句的结束位置，然后再读一个新的标识
        skip(1);
        getsym();
    }
}

void statements(int level){
 while(sym != rbrace){  //TODO 解决函数体为空的情况
     statement(level);
 }//右大括号，语句的结束
}

void compositestatements(int level, int dx){
    a_entry tmp;
    symbol type;
    while (sym == constsy){    //复合语句中常量定义模块
        getsym();
        constdeclaration(level);
        if (sym != semicolon){
            error(13);
            symset[0] = semicolon;
            skip(1);
            getsym();
        }else{
            getsym();
        }
    }
    while (sym == intsy || sym == floatsy || sym == charsy){    //复合语句中变量定义模块commencement
        type = sym;
        getsym();
        dx = variabledeclaration(level,type,dx);
        if (sym == semicolon){
            getsym();
        }else{
            error(13);
            symset[0] = semicolon;
            skip(1);
            getsym();
        }
    }
    statements(level);
    //这个是为了以防没有return
    tmp.inum = 0;
    gen(opr,0,tmp);

}

void functiondeclaration(symbol returntype, int level){
    //tx0是当前函数在符号表的登记入口
    int tx0 = tx -1, dx = 5, num_of_params;
    a_entry tmp;
    if (sym == lparent){
        getsym();
        dx = parameterlist(level, dx);
        num_of_params = dx - 5;
        /*
         * C0的子函数貌似不用设置jmp了。在最外面一层设置一下jmp跳到main里面去即可。
         * 需要设置addr为pcode开始的地方
         * */

        tab[tx0].ref.params = num_of_params;
        tab[tx0].ref.return_typ = (returntype == intsy) ? ints: (returntype == floatsy) ? floats : (returntype == charsy) ? chars: non;
        tab[tx0].addr.inum= cx;//此时cx处还没有指令，下面的gen将会生成
        tmp.inum = 0;
        gen(jmp,0,tmp);
        code[cx-1].a.inum = cx;

        if (sym == rparent){
            getsym();
            if (sym == lbrace){
                getsym();
            }else{
                error(50);
                symset[0] = semicolon;
                skip(1);
                getsym();
            }
            compositestatements(level,dx);
            if (sym == rbrace){
                getsym();
            }else{
                error(19);
                symset[0] = semicolon;
                skip(1);
                getsym();
            }

        }else{
            error(18);
            symset[0] = lbrace, symset[1] = semicolon;
            skip(2);
            getsym();
        }
    }else{
        error(50);
        symset[0] = rparent, symset[1] = lbrace, symset[2] = semicolon;
        skip(3);
        getsym();
    }
}

void mainfunction(int level){
//tx0是当前函数在符号表的登记入口
    int tx0 = tx -1, dx = 5;
    a_entry tmp;
    if (sym == lparent){
        getsym();

        tab[tx0].ref.params = 0;
        tab[tx0].ref.return_typ = non;
        tab[tx0].addr.inum= cx;//此时cx处还没有指令，下面的gen将会生成
        tmp.inum = 0;
        gen(jmp,0,tmp);
        code[cx-1].a.inum = cx;  //下一条执行的语句就是code还未生成的!!注意，如果函数体为空，那么需要人为生成一条return命令。

        if (sym == rparent){
            getsym();
            if (sym == lbrace){
                getsym();
            }else{
                error(50);
                symset[0] = semicolon;
                skip(1);
                getsym();
            }
            compositestatements(level,dx);
            if (sym == rbrace){
                getsym();
            }else{
                error(19);
                symset[0] = semicolon;
                skip(1);
                getsym();
            }

        }else{
            error(18);
            symset[0] = lbrace, symset[1] = semicolon;
            skip(2);
            getsym();
        }
    }else{
        error(50);
        symset[0] = rparent, symset[1] = lbrace, symset[2] = semicolon;
        skip(3);
        getsym();
    }
}

void program(int level){    //从最外层开始
    a_entry tmp;
    int tx0, pos;
    symbol type;
    int dx = 5; //保留前5个: 返回值, 静态链 ,动态链 ,返回地址, 函数在符号表中登记位置

    tx0 = tx-1; //program在符号表的入口，其实就是0
    tab[tx0].ref.params = 0;
    tab[tx0].ref.return_typ = non;
    tab[tx0].addr.inum = cx;
    tmp.inum = 0;
    gen(jmp,0,tmp);   //最开始的跳转，跳到main开始的地方

    if (sym != constsy && sym != intsy && sym != floatsy && sym != charsy && sym != voidsy){    //program接受的标识符
        error(50);
    }

    while (sym == constsy){//常量定义
        getsym();
        constdeclaration(level);
        if (sym != semicolon){
            error(13);
            symset[0] = semicolon;
            skip(1);
            getsym();
        }else{
            getsym();
        }
    }
    while (sym == intsy || sym == floatsy || sym == charsy){
        //变量和有返回值函数的定义；如果发现了函数，就不允许变量定义了
        type = sym;
        getsym();
        if (sym == ident){//标识符：变量的名字或者函数的名字
            pos = position(id);
            if (pos!=0&&tab[pos].level==level){   //出现同名变量，且在同一层:同一层意味着作用域相同
                error(29);
            }
            getsym();
            if (sym == lparent){    //说明这是个有返回值函数说明
                /*填最外层program的参数*/
                enter_tab(function,nul,level,0);
                functiondeclaration(type, level+1);//从'('开始
                break;  //出来后应该是下一个函数
            }
            else{   //变量定义，接下来的sym可能是分号，逗号或左方括号, var要与variable做好区分
                dx = var(level,type,dx);//变量名字已经有了，如果是数组的话可能还会有方括号
                if (sym == semicolon){
                    getsym();
                }
                else{
                    error(13);
                }
            }
        }
    }

    while(sym == voidsy || sym == intsy || sym == floatsy || sym == charsy){
        type = sym;
        //函数定义
        getsym();
        if (sym == mainsy){
            //主函数
            if (type != voidsy){
                error(20);
            }
            code[tab[tx0].addr.inum].a.inum = cx;    //现在更新第一条跳转语句的位置,tx应该就是0
            tmp.inum = 0;
            gen(init_p,0,tmp);      //全局变量、program返回地址等
            tmp.inum = tx;
            gen(init_main,0,tmp); //加载主函数
            enter_tab(function,nul,level,0);
            getsym();
            mainfunction(level+1); //main函数稍微有点特殊，所以用一个单独的函数
            break;//main函数后面不需要出现任何其他东西
        }
        else{
            pos = position(id);
            if (pos!=0&&tab[pos].level==level){   //出现同名变量，且在同一层:同一层意味着作用域相同
                error(29);
            }
            enter_tab(function,nul,level,0);
            getsym();
            functiondeclaration(type, level+1);//从'('开始
        }
    }
}

/**
 *
 * 运行栈：
 *
 * 操作数 <-- t
 * 局部变量
 * 参数
 * POS
 * DL
 * SL
 * RA
 * RL <-- b
 */

int counter_t_inc(int pos){ //返回的是一函数运行栈大小
    int i;
    for (i=pos+1 ; i<tx ; i++){
        if ( i >=tx || tab[i].obj == function ){    //找到下一个函数的位置
            break;
        }
    }

    if (i == pos+1){
        return 5;
    }
    else if (tab[i-1].obj == constant){
        return 5 + tab[pos].ref.params;
    }
    else if (tab[i-1].obj == variable && tab[i-1].typ != arrays){
        return tab[i-1].addr.inum + 1;
    }
    else{
        return tab[i-1].addr.inum+tab[i-1].ref.arr_size;    //这里array不用+1了
    }

    /*
    if (tab[i].obj == function){    //说明查询的函数没有参数，也没有常数，也没有变量
        return 5;   //C语言数组从0开始
    }
    else if (tab[i].obj == constant){
        return 5 + tab[pos].ref.params;
    }
    else if (tab[i].obj == variable && tab[i].typ != arrays){
        return tab[i].addr.inum + 1;
    }
    else{
        return tab[i].addr.inum+tab[i].ref.arr_size + 1;
    }
    */
}

int base(int l, int base){
    while(l > 0){
        base = s[base+2].inum;   //base+2:静态链位置
        l--;
    }
    return base;
}

void interpret(){
    int p, b, t, i, j,tmp; //p:pcode指针, b:基地址指针, t: 栈顶指针
    instruction ins;

    printf("Program starts\n");
    t = -1;
    b = 0;
    p = 0;
    s[0].type = ints, s[0].inum = 0;   //RL
    s[1].type = ints, s[1].inum = 0;   //RA
    s[2].type = ints, s[2].inum = 0;   //SL
    s[3].type = ints, s[3].inum = 0;   //DL
    s[4].type = ints, s[4].inum = 0;   //POS，这里是program在tab中位置
    do{
        ins = code[p];
        p++;
        switch(ins.f){
            case jmp:
                p = ins.a.inum;
                break;
            case init_p:
                tmp = t+6;  //局部变量区第一个位置
                t += counter_t_inc(s[b+4].inum);    //b+4是pos

                for (i=s[b+4].inum+1 ; i<tx ; i++){
                    if ( i >=tx || tab[i].obj == function ){
                        break;
                    }
                    if (tab[i].obj == variable){   //参数、变量包括函数，都填入类型信息
                        if (tab[i].typ != arrays){
                            s[tmp].type = tab[i].typ;
                            tmp++;
                        }
                        else{
                            j = tab[i].ref.arr_size;
                            while(j>0){
                                j--;
                                s[tmp].type = tab[i].ref.arr_ele_typ;
                                tmp++;
                            }
                        }
                    }
                }

                if (tmp!=t+1){
                    printf("ASSERT FALUSE init_p\n");
                }
                break;
            case init_main:
                s[t+1].type = ints, s[t+1].inum = 0;
                s[t+2].type = ints, s[t+2].inum = 0;
                s[t+3].type = ints, s[t+3].inum = 0;
                s[t+4].type = ints, s[t+4].inum = b;
                s[t+5].type = ints, s[t+5].inum = ins.a.inum;
                tmp = t + 6;
                b = t+1;
                p = tab[s[b+4].inum].addr.inum;
                t = b + counter_t_inc(ins.a.inum) - 1;

                for (i=s[b+4].inum+1 ; i<tx ; i++){
                    if ( i >=tx || tab[i].obj == function ){
                        break;
                    }
                    if (tab[i].obj == variable){   //参数、变量包括函数，都填入类型信息
                        if (tab[i].typ != arrays){
                            s[tmp].type = tab[i].typ;
                            tmp++;
                        }
                        else{
                            j = tab[i].ref.arr_size;
                            while(j>0){
                                j--;
                                s[tmp].type = tab[i].ref.arr_ele_typ;
                                tmp++;
                            }
                        }
                    }
                }

                if (tmp!=t+1){
                    printf("ASSERT FALUSE init_main\n");
                }
                break;
            case cal:   //cal l a : l是层次差， a是函数在符号表登记位置
                i = b+counter_t_inc(s[b+4].inum)-1;//当前最后一个局部变量的位置
                //s[b+4]是当前函数pos内容; i+1,i+2...t 是操作数栈的内容
                //i指向最后一个变量

                for (j = t-1; j >= i; j-- ){
                    s[j+counter_t_inc(ins.a.inum)+1] = s[j+1];  //新的操作数栈，这种移法不会破坏原栈的内容:struct内部所有信息都会复制的吧？！
                }
//                for (j = i; j < t; j++){
//                    s[j+counter_t_inc(ins.a.inum)+1] = s[j+1];  //新的操作数栈
//                }
                t = t+counter_t_inc(ins.a.inum); //这里已经指向操作数栈栈顶-->压栈

                s[i+1].type = ints, s[i+1].inum = 0;                 //返回值
                s[i+2].type = ints, s[i+2].inum = p;                 //返回地址
                s[i+3].type = ints, s[i+3].inum = base(ins.l, b);    //静态链，这个地方应该都是0...
                s[i+4].type = ints, s[i+4].inum = b;                 //动态链，返回的时候有用
                s[i+5].type = ints, s[i+5].inum = ins.a.inum;        //符号表中函数位置
                b = i + 1;                  //基地址更新
                p = tab[s[b+4].inum].addr.inum;     //指令指向函数开始位置

                tmp = b+5;
                for (i=s[b+4].inum+1 ; i<tx ; i++){ //将局部变量的类型登记
                    if ( i >=tx || tab[i].obj == function ){
                        break;
                    }
                    if (tab[i].obj == variable){   //参数、变量包括函数，都填入类型信息
                        if (tab[i].typ != arrays){
                            s[tmp].type = tab[i].typ;
                            tmp++;
                        }
                        else{
                            j = tab[i].ref.arr_size;
                            while(j>0){
                                j--;
                                s[tmp].type = tab[i].ref.arr_ele_typ;
                                tmp++;
                            }
                        }
                    }
                }

                //接下来要写入参数，并将栈顶的临时实参清掉
                if (tab[s[b+4].inum].ref.params !=0){
                    for(i = tab[s[b+4].inum].ref.params; i > 0; i-- ){

                        if (s[b+4+i].type == s[t].type){
                            s[b+4+i] = s[t];
                        }else{
                            if (s[b+4+i].type == chars){    //TODO: double check
                                if (s[t].type == floats){
                                    s[b+4+i].inum = (int)s[t].fnum;
                                }else{
                                    s[b+4+i].inum = s[t].inum;
                                }
                            }
                            else if (s[b+4+i].type == ints){
                                if (s[t].type == floats){
                                    s[b+4+i].inum = (int)s[t].fnum;
                                }else{
                                    s[b+4+i].inum = s[t].inum;
                                }
                            }
                            else if (s[b+4+i].type == floats){
                                if (s[t].type == floats){
                                    s[b+4+i].inum = s[t].fnum;
                                }else{
                                    s[b+4+i].inum = (float)s[t].inum;
                                }
                            }
                        }

                        //s[b+4+i]=s[t];  //移动时也会把类型信息写入
                        t--;
                    }

                }
                break;
            case lod:
                t++;
                s[t] = s[base(ins.l, b)+ins.a.inum];
                break;
            case lda:   //加载数组元素指令，需要用到栈顶中计算出的下标值
                if (s[t].type == floats){
                    s[t].type = ints;
                    s[t].inum = (int) s[t].fnum;
                }
                s[t] = s[base(ins.l,b)+ins.a.inum+s[t].inum];
                break;
            case ldc:
                t++;
                if (ins.l == 0){    //加载整数常量
                    s[t].type = ints;
                    s[t].inum = ins.a.inum;
                }
                else if (ins.l == 1){   //加载字符常量
                    s[t].type = chars;
                    s[t].inum = ins.a.inum;
                }
                else if (ins.l == 2){   //加载浮点常量
                    s[t].type = floats;
                    s[t].fnum = ins.a.fnum;
                }
                break;
            case sto:
                if (s[base(ins.l, b)+ins.a.inum].type == s[t].type){
                    s[base(ins.l, b)+ins.a.inum] = s[t];
                }else{
                    if (s[base(ins.l, b)+ins.a.inum].type == chars){    //TODO: 检查越界，和隐式转换的提示
                        if (s[t].type == floats){
                            s[base(ins.l, b)+ins.a.inum].inum = (int)s[t].fnum;
                        }else{
                            s[base(ins.l, b)+ins.a.inum].inum = s[t].inum;
                        }
                    }
                    else if (s[base(ins.l, b)+ins.a.inum].type == ints){
                        if (s[t].type == floats){
                            s[base(ins.l, b)+ins.a.inum].inum = (int)s[t].fnum;
                        }else{
                            s[base(ins.l, b)+ins.a.inum].inum = s[t].inum;
                        }
                    }
                    else if (s[base(ins.l, b)+ins.a.inum].type == floats){
                        if (s[t].type == floats){
                            s[base(ins.l, b)+ins.a.inum].fnum = s[t].fnum;
                        }else{
                            s[base(ins.l, b)+ins.a.inum].fnum = (float)s[t].inum;
                        }
                    }
                }
                t--;
                break;
            case sta:
                if (s[t-1].type == floats){
                    s[t-1].type = ints;
                    s[t-1].inum = (int)s[t-1].fnum;
                }

                if (s[base(ins.l,b)+ins.a.inum+s[t-1].inum].type == s[t].type){
                    s[base(ins.l,b)+ins.a.inum+s[t-1].inum] = s[t];
                }else{
                    if (s[base(ins.l,b)+ins.a.inum+s[t-1].inum].type == chars){    //TODO: 检查越界，和隐式转换的提示
                        if (s[t].type == floats){
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].inum = (int)s[t].fnum;
                        }else{
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].inum = s[t].inum;
                        }
                    }
                    else if (s[base(ins.l,b)+ins.a.inum+s[t-1].inum].type == ints){
                        if (s[t].type == floats){
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].inum = (int)s[t].fnum;
                        }else{
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].inum = s[t].inum;
                        }
                    }
                    else if (s[base(ins.l,b)+ins.a.inum+s[t-1].inum].type == floats){
                        if (s[t].type == floats){
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].fnum = s[t].fnum;
                        }else{
                            s[base(ins.l,b)+ins.a.inum+s[t-1].inum].fnum = (float)s[t].inum;
                        }
                    }
                }
                t = t-2;
                break;
            case jpc:
                if (s[t].type == ints || s[t].type == chars){
                    if (s[t].inum == 0){
                        p = ins.a.inum;
                    }
                }else{
                    if ((int)s[t].fnum == 0){   //需要隐式转换的情况: float->int
                        p = ins.a.inum;
                    }
                }
                t--;
                break;
            case red:
                if (s[base(ins.l, b) + ins.a.inum].type == ints){
                    scanf("%d",&s[base(ins.l, b) + ins.a.inum].inum);
                }
                else if (s[base(ins.l, b) + ins.a.inum].type == chars){
                    scanf(" %c",&s[base(ins.l, b) + ins.a.inum].inum);   //经试验这样写可以的
                }
                else if (s[base(ins.l, b) + ins.a.inum].type == floats){
                    scanf("%f",&s[base(ins.l, b) + ins.a.inum].fnum);
                }
                break;
            case wrt:
                if (ins.l == 1){
                    printf("%s\n",ins.a.str);   //打印字符串常量
                }else{
                    if (s[t].type == ints){
                        printf("%d\n",s[t].inum);
                    }
                    else if (s[t].type == chars){
                        printf("%c\n",s[t].inum);
                    }
                    else if (s[t].type == floats){
                        printf("%f\n",s[t].fnum);
                    }
                    t--;
                }
                break;
            case opr:
                switch (ins.a.inum){
                    case 0: //TODO 返回
                        p = s[b+1].inum; //返回地址
                        tmp = s[b+3].inum; //动态链保存
                        j = s[b+4].inum; //函数登记位置

                        i = b+counter_t_inc(j)-1;  //之前索引写错了，but how？是因为用的是ins.a.inum而不是真正的s[b+4]
                        //s[b+4]是当前函数pos内容; i+1,i+2...t 是操作数栈的内容
                        //i指向最后一个变量
                        for ( ;i < t;i++){
                            s[i-counter_t_inc(j)+1] = s[i+1];  //新的操作数栈
                        }

                        t = i-counter_t_inc(j); //这里已经指向操作数栈栈顶
                        b = tmp;

                        if (tab[j].ref.return_typ!=non){    //有返回值，需要对返回值的类型进行审查:返回值一定在栈顶
                            if (s[t].type != tab[j].ref.return_typ){
                                if (s[t].type == ints && tab[j].ref.return_typ == chars){
                                    s[t].type = chars;
                                }
                                else if (s[t].type == ints && tab[j].ref.return_typ == floats){
                                    s[t].type = floats;
                                    s[t].fnum = (float)s[t].inum;
                                }
                                else if (s[t].type == chars && tab[j].ref.return_typ == ints){
                                    s[t].type = ints;
                                }
                                else if (s[t].type == chars && tab[j].ref.return_typ == floats){
                                    s[t].type = floats;
                                    s[t].fnum = (float)s[t].inum;
                                }
                                else if (s[t].type == floats && tab[j].ref.return_typ == ints){
                                    s[t].type = ints;
                                    s[t].inum = (int)s[t].fnum;
                                }
                                else if (s[t].type == floats && tab[j].ref.return_typ == chars){
                                    s[t].type = chars;
                                    s[t].inum = (int)s[t].fnum;
                                }
                            }
                        }
                        break;
                    case 1:
                        s[t].inum *= -1;
                        s[t].fnum *= -1;
                        break;
                    case 2:
                        t = t-1;

                        if (s[t].type == ints && s[t+1].type == ints){
                            s[t].inum += s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == chars){
                            s[t].inum += s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum + s[t+1].fnum;
                        }
                        else if (s[t].type == chars && s[t+1].type == ints){
                            s[t].type = ints;
                            s[t].inum += s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == chars){
                            s[t].inum += s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum + s[t+1].fnum;
                        }
                        else if (s[t].type == floats && s[t+1].type == ints){
                            s[t].fnum += (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == chars){
                            s[t].fnum += (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].fnum += s[t+1].fnum;
                        }

                        break;
                    case 3:
                        t = t-1;

                        if (s[t].type == ints && s[t+1].type == ints){
                            s[t].inum -= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == chars){
                            s[t].inum -= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum - s[t+1].fnum;
                        }
                        else if (s[t].type == chars && s[t+1].type == ints){
                            s[t].type = ints;
                            s[t].inum -= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == chars){
                            s[t].inum -= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum - s[t+1].fnum;
                        }
                        else if (s[t].type == floats && s[t+1].type == ints){
                            s[t].fnum -= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == chars){
                            s[t].fnum -= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].fnum -= s[t+1].fnum;
                        }

                        break;
                    case 4:
                        t = t-1;

                        if (s[t].type == ints && s[t+1].type == ints){
                            s[t].inum *= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == chars){
                            s[t].inum *= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum * s[t+1].fnum;
                        }
                        else if (s[t].type == chars && s[t+1].type == ints){
                            s[t].type = ints;
                            s[t].inum *= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == chars){
                            s[t].inum *= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum * s[t+1].fnum;
                        }
                        else if (s[t].type == floats && s[t+1].type == ints){
                            s[t].fnum *= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == chars){
                            s[t].fnum *= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].fnum *= s[t+1].fnum;
                        }

                        break;
                    case 5:
                        t = t -1;

                        if (s[t].type == ints && s[t+1].type == ints){
                            s[t].inum /= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == chars){
                            s[t].inum /= s[t+1].inum;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum / s[t+1].fnum;
                        }
                        else if (s[t].type == chars && s[t+1].type == ints){
                            s[t].type = ints;
                            s[t].inum /= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == chars){
                            s[t].inum /= s[t+1].inum;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = floats;
                            s[t].fnum = (float)s[t].inum / s[t+1].fnum;
                        }
                        else if (s[t].type == floats && s[t+1].type == ints){
                            s[t].fnum /= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == chars){
                            s[t].fnum /= (float)s[t+1].inum;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].fnum /= s[t+1].fnum;
                        }
                        break;
                    case 6://偶置0，奇置1
                        if (s[t].type == floats){
                            s[t].type = ints;
                            if ((int)s[t].fnum % 2==0){
                                s[t].inum=1;
                            }else{
                                s[t].inum=0;
                            }
                        }else{
                            s[t].type = ints;
                            if (s[t].inum % 2==0){
                                s[t].inum=1;
                            }else{
                                s[t].inum=0;
                            }
                        }
                        break;
                    case 8:
                        t =t-1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum == s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum == s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum == s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum == s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum == (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum == s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    case 9:
                        t =t -1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum != s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum != s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum != s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum != s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum != (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum != s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    case 10:
                        t =t -1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum < s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum < s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum < s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum < s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum < (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum < s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    case 11:
                        t =t -1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum >= s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum >= s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum >= s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum >= s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum >= (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum >= s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    case 12:
                        t = t-1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum > s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum > s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum > s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum > s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum > (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum > s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    case 13:
                        t = t-1;
                        if (s[t].type == ints && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].inum = (s[t].inum <= s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == ints && s[t+1].type == floats){
                            s[t].inum = ((float)s[t].inum <= s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == chars && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].inum <= s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == chars && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = ((float)s[t].inum <= s[t+1].fnum) ? 1: 0;
                        }
                        else if (s[t].type == floats && (s[t+1].type == ints || s[t+1].type == chars)){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum <= (float)s[t+1].inum) ? 1: 0;
                        }
                        else if (s[t].type == floats && s[t+1].type == floats){
                            s[t].type = ints;
                            s[t].inum = (s[t].fnum <= s[t+1].fnum) ? 1: 0;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }while(p != 0);
}

int main(){
    strcpy(word[0],"const");    strcpy(word[1],"int");
    strcpy(word[2],"float");    strcpy(word[3],"char");
    strcpy(word[4],"void");     strcpy(word[5],"main");
    strcpy(word[6],"if");       strcpy(word[7],"else");
    strcpy(word[8],"while");    strcpy(word[9],"for");
    strcpy(word[10],"scanf");   strcpy(word[11],"printf");
    strcpy(word[12],"return");

    wordsym[0] = constsy;       wordsym[1] = intsy;
    wordsym[2] = floatsy;       wordsym[3] = charsy;
    wordsym[4] = voidsy;        wordsym[5] = mainsy;
    wordsym[6] = ifsy;          wordsym[7] = elsesy;
    wordsym[8] = whilesy;       wordsym[9] = forsy;
    wordsym[10] = scanfsy;      wordsym[11] = printfsy;
    wordsym[12] = returnsy;

    strcpy(err_msg[0],"Invalid character");
    strcpy(err_msg[1],"Expected a \'");
    strcpy(err_msg[2],"Decimals exceed limit");
    strcpy(err_msg[3],"Expected ch(32), ch(33), ch(35)-ch(126)");
    strcpy(err_msg[4],"String exceeds limit");
    strcpy(err_msg[5],"Ident name exceeds limit");
    strcpy(err_msg[6],"Expected a =");
    strcpy(err_msg[7],"Invalid decimal");
    strcpy(err_msg[8],"Expected a type");
    strcpy(err_msg[9],"Operators +/- too many");
    strcpy(err_msg[10],"Implicit transformation");
    strcpy(err_msg[11],"Exceeds ASC2 bounds");
    strcpy(err_msg[12],"How could you assign a char to a float?");
    strcpy(err_msg[13],"Expected a ;");
    strcpy(err_msg[14],"Cannot declare variables here");
    strcpy(err_msg[15],"Expected an identifier");
    strcpy(err_msg[16],"Expected an unsigned integer");
    strcpy(err_msg[17],"Expected a ]");
    strcpy(err_msg[18],"Expected a )");
    strcpy(err_msg[19],"Expected a }");
    strcpy(err_msg[20],"Main function should not return");
    strcpy(err_msg[21],"Excessive operators");
    strcpy(err_msg[22],"Undeclared identifier");
    strcpy(err_msg[23],"Array out of bounds");
    strcpy(err_msg[24],"Function has no return values");
    strcpy(err_msg[25],"Cannot assign values to constant");
    strcpy(err_msg[26],"Function has no return value");
    strcpy(err_msg[27],"Function has return value");
    strcpy(err_msg[28],"Parameters mismatch");
    strcpy(err_msg[29],"Duplication detected");
    strcpy(err_msg[30],"Illegal relation operator");

    strcpy(err_msg[50],"Invalid Syntax");

    printf("请输入文件路径\n");
    scanf("%s",file);
    if(!(fp = fopen(file,"r"))){
        printf("打开文件失败\n");
        return 0;
    }

    tx = 0;
    err = 0;
    cx = 0;

    strcpy(id,"0program");
    enter_tab(function,nul,-1,0);
    getsym();
    program(0); //程序的全局常数、变量都算第0层的
    if (err > 0){
        printf("Errors: %d\n",err);
        return 0;
    }else{
        print_table();
        print_pcode();
        interpret();
    }
}
