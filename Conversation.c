#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAXLEN 100 // 最大记号长度
#define KEYNUM 8 // 关键字个数
#define SYMNUM 50 // 符号表大小
#define QUADNUM 100 // 四元式序列大小
#define CODESIZE 1000 // 目标代码大小

// 记号类别
enum TokenType {
    ID, // 标识符
    KEY, // 关键字
    NUM, // 数字常量
    OP, // 运算符
    DEL, // 界符
    ERR // 错误
};

// 关键字表
char *keywords[KEYNUM] = {"int", "char", "if", "else", "while", "return", "main", "void"};

// 符号表项结构体
struct Symbol {
    char name[MAXLEN]; // 符号名
    enum TokenType type; // 符号类别（标识符或关键字）
    int value; // 符号值（数字常量或变量地址）
};

// 四元式结构体
struct Quadruple {
    char op[MAXLEN]; // 操作符
    char arg1[MAXLEN]; // 第一个操作数
    char arg2[MAXLEN]; // 第二个操作数
    char result[MAXLEN]; // 结果
};

// 全局变量声明
char ch; // 当前字符
char token[MAXLEN]; // 当前记号
int pos = 0; // 记号位置指针
enum TokenType type; // 记号类别

struct Symbol symtab[SYMNUM]; // 符号表数组
int symnum = 0; // 符号表大小

struct Quadruple quadtab[QUADNUM]; // 四元式序列数组
int quadnum = 0; // 四元式序列大小

char code[CODESIZE]; // 目标代码字符串
int codepos = 0; // 目标代码位置指针

FILE *fp; // 源程序文件指针

// 函数声明
void lexicalAnalysis(); // 词法分析函数，获取下一个记号并存入全局变量token和type中
void syntaxAnalysis(); // 语法分析函数，分析源程序的语法结构并生成四元式序列
void semanticAnalysis(); // 语义分析函数，检查源程序的语义正确性并填充符号表和四元式序列中的值和地址信息
void codeGeneration(); // 目标代码生成函数，根据四元式序列和符号表生成目标代码并输出到文件中

void program(); // 程序分析函数，对应产生式<程序> ::= <声明序列><语句序列>
void declarationList(); // 声明序列分析函数，对应产生式<声明序列> ::= <声明><声明序列>|ε
void declaration(); // 声明分析函数，对应产生式<声明> ::= <类型><标识符>;
void type(); // 类型分析函数，对应产生式<类型> ::= int|char|void
void statementList(); // 语句序列分析函数，对应产生式<语句序列> ::= <语句><语句序列>|ε
void statement(); // 语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>
void assignStatement(); // 赋值语句分析函数，对应产生式<赋值语句> ::= <标识符>=<表达式>;
void expression(char *place); // 表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数place用于存储表达式的结果位置（临时变量或标识符）
void term(char *place); // 项分析函数，对应产生式<项> ::= <因子>{*<因子>|/<因子>|%<因子>}，参数place用于存储项的结果位置（临时变量或标识符）
void factor(char *place); // 因子分析函数，对应产生式<因子> ::= <标识符>|<常量>|(<表达式>)，参数place用于存储因子的结果位置（临时变量或标识符）
void conditionStatement(); // 条件语句分析函数，对应产生式<条件语句> ::= if(<条件>)<语句>{else<语句>}
void condition(char *trueLabel, char *falseLabel); // 条件分析函数，对应产生式<条件> ::= <表达式><关系运算符><表达式>，参数trueLabel和falseLabel用于存储条件为真和为假时的跳转标号
void loopStatement(); // 循环语句分析函数，对应产生式<循环语句> ::= while(<条件>)<语句>
void returnStatement(); // 返回语句分析函数，对应产生式<返回语句> ::= return;|return(<表达式>);

void error(char *msg); // 错误处理函数，打印错误信息并退出程序
int isKeyword(char *token); // 判断是否为关键字，是则返回其序号，否则返回-1
int isLetter(char ch); // 判断是否为字母或下划线
int isOperator(char ch); // 判断是否为运算符，是则返回其序号，否则返回-1
int isDelimiter(char ch); // 判断是否为界符，是则返回其序号，否则返回-1
void printToken(enum TokenType type, char *token); // 打印记号信息
int lookupSymbol(char *name); // 查找符号表，返回符号在表中的位置，如果不存在则返回-1
void insertSymbol(char *name, enum TokenType type, int value); // 插入符号表，如果已存在则报错
void updateSymbol(int index, int value); // 更新符号表中的值
char *newTemp(); // 生成一个新的临时变量名
char *newLabel(); // 生成一个新的标号名
void emitQuad(char *op, char *arg1, char *arg2, char *result); // 生成一个四元式并加入到四元式序列中
void backpatch(char *label, int quadpos); // 回填跳转标号到指定的四元式位置
void printQuad(struct Quadruple quad); // 打印四元式信息
void printQuadList(); // 打印四元式序列信息
void printSymbol(struct Symbol sym); // 打印符号表项信息
void printSymbolList(); // 打印符号表信息
void emitCode(char *code); // 生成目标代码并加入到目标代码字符串中
void printCode(); // 打印目标代码信息

// 词法分析函数，获取下一个记号并存入全局变量token和type中
void lexicalAnalysis() {
    while ((ch = fgetc(fp)) != EOF) { // 读取文件直到结束
        if (isspace(ch)) { // 跳过空白字符
            continue;
        } else if (isLetter(ch)) { // 处理标识符或关键字
            token[pos++] = ch; // 加入记号
            while (isLetter(ch = fgetc(fp)) || isdigit(ch)) { // 读取后续字符直到非字母或数字
                token[pos++] = ch; // 加入记号
            }
            ungetc(ch, fp); // 将多读的字符退回文件流中
            token[pos] = '\0'; // 添加字符串结束标志
            pos = 0; // 重置位置指针

            int index = isKeyword(token); // 判断是否为关键字
            if (index != -1) { // 是关键字
                type = KEY; // 设置类别为关键字
                printToken(type, token); // 打印记号信息（可选）
                return; // 返回记号信息给语法分析器
            } else { // 不是关键字
                type = ID; // 设置类别为标识符
                printToken(type, token); // 打印记号信息（可选）
                return; // 返回记号信息给语法分析器
            }
        } else if (isdigit(ch)) { // 处理数字常量
            token[pos++] = ch; // 加入记号
            while (isdigit(ch = fgetc(fp))) { // 读取后续字符直到非数字
                token[pos++] = ch; // 加入记号
            }
            ungetc(ch, fp); // 将多读的字符退回文件流中
            token[pos] = '\0'; // 添加字符串结束标志
            pos = 0; // 重置位置指针

            type = NUM; // 设置类别为数字常量
            printToken(type, token); // 打印记号信息（可选）
            return; // 返回记号信息给语法分析器
        } else if (isOperator(ch) != -1) { // 处理运算符
            token[pos++] = ch; // 加入记号
            char next = fgetc(fp); // 读取下一个字符
            if ((ch == '<' || ch == '>' || ch == '=' || ch == '!' || ch == '&' || ch == '|') && next == ch) { // 处理双字符运算符
                token[pos++] = next; // 加入记号
                token[pos] = '\0'; // 添加字符串结束标志
                pos = 0; // 重置位置指针

                type = OP; // 设置类别为运算符
                printToken(type, token); // 打印记号信息（可选）
                return; // 返回记号信息给语法分析器
            } else { // 处理单字符运算符
                ungetc(next, fp); // 将多读的字符退回文件流中
                token[pos] = '\0'; // 添加字符串结束标志
                pos = 0; // 重置位置指针

                type = OP; // 设置类别为运算符
                printToken(type, token); // 打印记号信息（可选）
                return; // 返回记号信息给语法分析器
            }
        } else if (isDelimiter(ch) != -1) { // 处理界符
            token[pos++] = ch; // 加入记号
            token[pos] = '\0'; // 添加字符串结束标志
            pos = 0; // 重置位置指针

            type = DEL; // 设置类别为界符
            printToken(type, token); // 打印记号信息（可选）
            return; // 返回记号信息给语法分析器
        } else { // 处理错误字符
            token[pos++] = ch; // 加入记号
            token[pos] = '\0'; // 添加字符串结束标志
            pos = 0; // 重置位置指针

            type = ERR; // 设置类别为错误
            printToken(type, token); // 打印记号信息（可选）
            error("Invalid character"); // 报错并退出程序
        }
    }
    strcpy(token, "EOF"); // 文件结束，设置记号为EOF
    type = ERR; // 设置类别为错误（用于表示文件结束）
}

// 语法分析函数，分析源程序的语法结构并生成四元式序列
void syntaxAnalysis() {
    lexicalAnalysis(); // 获取第一个记号，开始语法分析过程
    program(); // 调用程序分析函数，对应产生式<程序> ::= <声明序列><语句序列>
    if (strcmp(token, "EOF") != 0) { // 如果还有剩余的记号，说明语法错误
        error("Syntax error"); 
    }
}

// 程序分析函数，对应产生式<程序> ::= <声明序列><语句序列>
void program() {
    declarationList(); // 调用声明序列分析函数，对应产生式<声明序列> ::= <声明><声明序列>|ε
    statementList(); // 调用语句序列分析函数，对应产生式<语句序列> ::= <语句><语句序列>|ε
}

// 声明序列分析函数，对应产生式<声明序列> ::= <声明><声明序列>|ε
void declarationList() {
    if (type == KEY && (strcmp(token, "int") == 0 || strcmp(token, "char") == 0 || strcmp(token, "void") == 0)) { // 如果当前记号是类型关键字，说明有声明
        declaration(); // 调用声明分析函数，对应产生式<声明> ::= <类型><标识符>;
        declarationList(); // 递归调用声明序列分析函数，对应产生式<声明序列> ::= <声明><声明序列>
    } else { // 如果当前记号不是类型关键字，说明没有声明，对应产生式<声明序列> ::= ε
        return; // 直接返回
    }
}

// 声明分析函数，对应产生式<声明> ::= <类型><标识符>;
void declaration() {
    type(); // 调用类型分析函数，对应产生式<类型> ::= int|char|void
    char t[MAXLEN]; // 用于存储类型信息
    strcpy(t, token); // 复制类型信息到t中
    lexicalAnalysis(); // 获取下一个记号
    if (type == ID) { // 如果当前记号是标识符，说明是合法的声明
        char n[MAXLEN]; // 用于存储标识符名
        strcpy(n, token); // 复制标识符名到n中
        lexicalAnalysis(); // 获取下一个记号
        if (type == DEL && strcmp(token, ";") == 0) { // 如果当前记号是分号，说明是合法的声明结束
            insertSymbol(n, ID, 0); // 将标识符插入到符号表中，初始值为0
            emitQuad("DEC", t, "", n); // 生成一个DEC四元式，表示为该标识符分配空间
            lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
        } else { // 如果当前记号不是分号，说明是语法错误
            error("Missing ;");
        }
    } else { // 如果当前记号不是标识符，说明是语法错误
        error("Missing identifier");
    }
}

// 类型分析函数，对应产生式<类型> ::= int|char|void
void type() {
    if (type == KEY && (strcmp(token, "int") == 0 || strcmp(token, "char") == 0 || strcmp(token, "void") == 0)) { // 如果当前记号是类型关键字，说明是合法的类型
        return; // 直接返回
    } else { // 如果当前记号不是类型关键字，说明是语法错误
        error("Invalid type");
    }
}

// 语句序列分析函数，对应产生式<语句序列> ::= <语句><语句序列>|ε
void statementList() {
    if ((type == ID) || (type == KEY && (strcmp(token, "if") == 0 || strcmp(token, "while") == 0 || strcmp(token, "return") == 0))) { // 如果当前记号是标识符或if、while、return关键字，说明有语句
        statement(); // 调用语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>
        statementList(); // 递归调用语句序列分析函数，对应产生式<语句序列> ::= <语句><语句序列>
    } else { // 如果当前记号不是标识符或if、while、return关键字，说明没有语句，对应产生式<语句序列> ::= ε
        return; // 直接返回
    }
}

// 语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>
void statement() {
    if (type == ID) { // 如果当前记号是标识符，说明是赋值语句
        assignStatement(); // 调用赋值语句分析函数，对应产生式<赋值语句> ::= <标识符>=<表达式>;
    } else if (type == KEY && strcmp(token, "if") == 0) { // 如果当前记号是if关键字，说明是条件语句
        conditionStatement(); // 调用条件语句分析函数，对应产生式<条件语句> ::= if(<条件>)<语句>{else<语句>}
    } else if (type == KEY && strcmp(token, "while") == 0) { // 如果当前记号是while关键字，说明是循环语句
        loopStatement(); // 调用循环语句分析函数，对应产生式<循环语句> ::= while(<条件>)<语句>
    } else if (type == KEY && strcmp(token, "return") == 0) { // 如果当前记号是return关键字，说明是返回语句
        returnStatement(); // 调用返回语句分析函数，对应产生式<返回语句> ::= return;|return(<表达式>);
    } else { // 如果当前记号不是以上任何一种情况，说明是语法错误
        error("Invalid statement");
    }
}

// 赋值语句分析函数，对应产生式<赋值语句> ::= <标识符>=<表达式>;
void assignStatement() {
    char n[MAXLEN]; // 用于存储标识符名
    strcpy(n, token); // 复制标识符名到n中
    int index = lookupSymbol(n); // 查找符号表中是否有该标识符
    if (index == -1) { // 如果没有找到，说明该标识符未声明，报错并退出程序
        error("Undeclared identifier");
    }
    lexicalAnalysis(); // 获取下一个记号
    if (type == OP && strcmp(token, "=") == 0) { // 如果当前记号是等号，说明是合法的赋值语句
        lexicalAnalysis(); // 获取下一个记号
        char e[MAXLEN]; // 用于存储表达式的结果位置（临时变量或标识符）
        expression(e); // 调用表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数e用于存储表达式的结果位置（临时变量或标识符）
        if (type == DEL && strcmp(token, ";") == 0) { // 如果当前记号是分号，说明是合法的赋值语句结束
            emitQuad("=", e, "", n); // 生成一个赋值四元式，表示将表达式的结果赋给标识符
            updateSymbol(index, atoi(e)); // 更新符号表中的值（如果表达式的结果是一个数字常量）
            lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
        } else { // 如果当前记号不是分号，说明是语法错误
            error("Missing ;");
        }
    } else { // 如果当前记号不是等号，说明是语法错误
        error("Missing =");
    }
}

// 表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数place用于存储表达式的结果位置（临时变量或标识符）
void expression(char *place) {
    char t1[MAXLEN]; // 用于存储第一个项的结果位置（临时变量或标识符）
    term(t1); // 调用项分析函数，对应产生式<项> ::= <因子>{*<因子>|/<因子>|%<因子>}，参数t1用于存储第一个项的结果位置（临时变量或标识符）
    while (type == OP && (strcmp(token, "+") == 0 || strcmp(token, "-") == 0)) { // 如果当前记号是加号或减号，说明有后续的项
        char op[MAXLEN]; // 用于存储操作符
        strcpy(op, token); // 复制操作符到op中
        lexicalAnalysis(); // 获取下一个记号
        char t2[MAXLEN]; // 用于存储第二个项的结果位置（临时变量或标识符）
        term(t2); // 调用项分析函数，对应产生式<项> ::= <因子>{*<因子>|/<因子>|%<因子>}，参数t2用于存储第二个项的结果位置（临时变量或标识符）
        char t3[MAXLEN]; // 用于存储两个项的运算结果位置（临时变量）
        strcpy(t3, newTemp()); // 生成一个新的临时变量名并复制到t3中
        emitQuad(op, t1, t2, t3); // 生成一个四元式，表示将两个项进行运算并将结果存入临时变量
        strcpy(t1, t3); // 将临时变量作为下一次运算的第一个操作数
    }
    strcpy(place, t1); // 将最终的表达式结果位置复制到place中
}

// 项分析函数，对应产生式<项> ::= <因子>{*<因子>|/<因子>|%<因子>}，参数place用于存储项的结果位置（临时变量或标识符）
void term(char *place) {
    char f1[MAXLEN]; // 用于存储第一个因子的结果位置（临时变量或标识符）
    factor(f1); // 调用因子分析函数，对应产生式<因子> ::= <标识符>|<常量>|(<表达式>)，参数f1用于存储第一个因子的结果位置（临时变量或标识符）
    while (type == OP && (strcmp(token, "*") == 0 || strcmp(token, "/") == 0 || strcmp(token, "%") == 0)) { // 如果当前记号是乘号、除号或取余号，说明有后续的因子
        char op[MAXLEN]; // 用于存储操作符
        strcpy(op, token); // 复制操作符到op中
        lexicalAnalysis(); // 获取下一个记号
        char f2[MAXLEN]; // 用于存储第二个因子的结果位置（临时变量或标识符）
        factor(f2); // 调用因子分析函数，对应产生式<因子> ::= <标识符>|<常量>|(<表达式>)，参数f2用于存储第二个因子的结果位置（临时变量或标识符）
        char f3[MAXLEN]; // 用于存储两个因子的运算结果位置（临时变量）
        strcpy(f3, newTemp()); // 生成一个新的临时变量名并复制到f3中
        emitQuad(op, f1, f2, f3); // 生成一个四元式，表示将两个因子进行运算并将结果存入临时变量
        strcpy(f1, f3); // 将临时变量作为下一次运算的第一个操作数
    }
    strcpy(place, f1); // 将最终的项结果位置复制到place中
}

// 因子分析函数，对应产生式<因子> ::= <标识符>|<常量>|(<表达式>)，参数place用于存储因子的结果位置（临时变量或标识符）
void factor(char *place) {
    if (type == ID) { // 如果当前记号是标识符，说明是合法的因子
        char n[MAXLEN]; // 用于存储标识符名
        strcpy(n, token); // 复制标识符名到n中
        int index = lookupSymbol(n); // 查找符号表中是否有该标识符
        if (index == -1) { // 如果没有找到，说明该标识符未声明，报错并退出程序
            error("Undeclared identifier");
        }
        strcpy(place, n); // 将标识符名复制到place中
        lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
    } else if (type == NUM) { // 如果当前记号是数字常量，说明是合法的因子
        char n[MAXLEN]; // 用于存储数字常量
        strcpy(n, token); // 复制数字常量到n中
        strcpy(place, n); // 将数字常量复制到place中
        lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
    } else if (type == DEL && strcmp(token, "(") == 0) { // 如果当前记号是左括号，说明是合法的因子，表示一个括号内的表达式
        lexicalAnalysis(); // 获取下一个记号
        expression(place); // 调用表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数place用于存储表达式的结果位置（临时变量或标识符）
        if (type == DEL && strcmp(token, ")") == 0) { // 如果当前记号是右括号，说明是合法的因子结束
            lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
        } else { // 如果当前记号不是右括号，说明是语法错误
            error("Missing )");
        }
    } else { // 如果当前记号不是以上任何一种情况，说明是语法错误
        error("Invalid factor");
    }
}

// 条件语句分析函数，对应产生式<条件语句> ::= if(<条件>)<语句>{else<语句>}
void conditionStatement() {
    if (type == KEY && strcmp(token, "if") == 0) { // 如果当前记号是if关键字，说明是合法的条件语句开始
        lexicalAnalysis(); // 获取下一个记号
        if (type == DEL && strcmp(token, "(") == 0) { // 如果当前记号是左括号，说明是合法的条件语句开始
            lexicalAnalysis(); // 获取下一个记号

            char trueLabel[MAXLEN]; // 用于存储条件为真时的跳转标号
            char falseLabel[MAXLEN]; // 用于存储条件为假时的跳转标号
            condition(trueLabel, falseLabel); // 调用条件分析函数，对应产生式<条件> ::= <表达式><关系运算符><表达式>，参数trueLabel和falseLabel用于存储条件为真和为假时的跳转标号

            if (type == DEL && strcmp(token, ")") == 0) { // 如果当前记号是右括号，说明是合法的条件语句开始
                lexicalAnalysis(); // 获取下一个记号

                backpatch(trueLabel, quadnum); // 回填条件为真时的跳转标号到下一条四元式位置（即if语句块的开始位置）
                statement(); // 调用语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>

                if (type == KEY && strcmp(token, "else") == 0) { // 如果当前记号是else关键字，说明有else语句块
                    char nextLabel[MAXLEN]; // 用于存储跳过else语句块的跳转标号
                    strcpy(nextLabel, newLabel()); // 生成一个新的标号名并复制到nextLabel中
                    emitQuad("JMP", "", "", nextLabel); // 生成一个无条件跳转四元式，表示跳过else语句块
                    backpatch(falseLabel, quadnum); // 回填条件为假时的跳转标号到下一条四元式位置（即else语句块的开始位置）
                    lexicalAnalysis(); // 获取下一个记号
                    statement(); // 调用语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>
                    backpatch(nextLabel, quadnum); // 回填跳过else语句块的跳转标号到下一条四元式位置（即条件语句结束后的位置）
                } else { // 如果当前记号不是else关键字，说明没有else语句块
                    backpatch(falseLabel, quadnum); // 回填条件为假时的跳转标号到下一条四元式位置（即条件语句结束后的位置）
                }
            } else { // 如果当前记号不是右括号，说明是语法错误
                error("Missing )");
            }
        } else { // 如果当前记号不是左括号，说明是语法错误
            error("Missing (");
        }
    } else { // 如果当前记号不是if关键字，说明是语法错误
        error("Missing if");
    }
}

// 条件分析函数，对应产生式<条件> ::= <表达式><关系运算符><表达式>，参数trueLabel和falseLabel用于存储条件为真和为假时的跳转标号
void condition(char *trueLabel, char *falseLabel) {
    char e1[MAXLEN]; // 用于存储第一个表达式的结果位置（临时变量或标识符）
    expression(e1); // 调用表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数e1用于存储第一个表达式的结果位置（临时变量或标识符）
    if (type == OP && (strcmp(token, "<") == 0 || strcmp(token, "<=") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">=") == 0 || strcmp(token, "==") == 0 || strcmp(token, "!=") == 0)) { // 如果当前记号是关系运算符，说明是合法的条件
        char op[MAXLEN]; // 用于存储关系运算符
        strcpy(op, token); // 复制关系运算符到op中
        lexicalAnalysis(); // 获取下一个记号
        char e2[MAXLEN]; // 用于存储第二个表达式的结果位置（临时变量或标识符）
        expression(e2); // 调用表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数e2用于存储第二个表达式的结果位置（临时变量或标识符）

        strcpy(trueLabel, newLabel()); // 生成一个新的标号名并复制到trueLabel中
        strcpy(falseLabel, newLabel()); // 生成一个新的标号名并复制到falseLabel中

        emitQuad(op, e1, e2, trueLabel); // 生成一个条件跳转四元式，表示如果两个表达式满足关系运算则跳转到trueLabel
        emitQuad("JMP", "", "", falseLabel); // 生成一个无条件跳转四元式，表示否则跳转到falseLabel
    } else { // 如果当前记号不是关系运算符，说明是语法错误
        error("Invalid relation operator");
    }
}

// 循环语句分析函数，对应产生式<循环语句> ::= while(<条件>)<语句>
void loopStatement() {
    if (type == KEY && strcmp(token, "while") == 0) { // 如果当前记号是while关键字，说明是合法的循环语句开始
        lexicalAnalysis(); // 获取下一个记号
        if (type == DEL && strcmp(token, "(") == 0) { // 如果当前记号是左括号，说明是合法的循环语句开始
            lexicalAnalysis(); // 获取下一个记号

            char beginLabel[MAXLEN]; // 用于存储循环开始时的跳转标号
            strcpy(beginLabel, newLabel()); // 生成一个新的标号名并复制到beginLabel中
            backpatch(beginLabel, quadnum); // 回填循环开始时的跳转标号到下一条四元式位置（即条件判断的位置）

            char trueLabel[MAXLEN]; // 用于存储条件为真时的跳转标号
            char falseLabel[MAXLEN]; // 用于存储条件为假时的跳转标号
            condition(trueLabel, falseLabel); // 调用条件分析函数，对应产生式<条件> ::= <表达式><关系运算符><表达式>，参数trueLabel和falseLabel用于存储条件为真和为假时的跳转标号

            if (type == DEL && strcmp(token, ")") == 0) { // 如果当前记号是右括号，说明是合法的循环语句开始
                lexicalAnalysis(); // 获取下一个记号

                backpatch(trueLabel, quadnum); // 回填条件为真时的跳转标号到下一条四元式位置（即while语句块的开始位置）
                statement(); // 调用语句分析函数，对应产生式<语句> ::= <赋值语句>|<条件语句>|<循环语句>|<返回语句>
                emitQuad("JMP", "", "", beginLabel); // 生成一个无条件跳转四元式，表示跳回到循环开始时的位置（即条件判断的位置）
                backpatch(falseLabel, quadnum); // 回填条件为假时的跳转标号到下一条四元式位置（即循环语句结束后的位置）
            } else { // 如果当前记号不是右括号，说明是语法错误
                error("Missing )");
            }
        } else { // 如果当前记号不是左括号，说明是语法错误
            error("Missing (");
        }
    } else { // 如果当前记号不是while关键字，说明是语法错误
        error("Missing while");
    }
}

// 返回语句分析函数，对应产生式<返回语句> ::= return;|return(<表达式>);
void returnStatement() {
    if (type == KEY && strcmp(token, "return") == 0) { // 如果当前记号是return关键字，说明是合法的返回语句开始
        lexicalAnalysis(); // 获取下一个记号
        if (type == DEL && strcmp(token, ";") == 0) { // 如果当前记号是分号，说明是合法的返回语句结束，对应产生式return;
            emitQuad("RET", "", "", ""); // 生成一个RET四元式，表示返回主函数
            lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
        } else if (type == DEL && strcmp(token, "(") == 0) { // 如果当前记号是左括号，说明是合法的返回语句开始，对应产生式return(<表达式>);
            lexicalAnalysis(); // 获取下一个记号
            char e[MAXLEN]; // 用于存储表达式的结果位置（临时变量或标识符）
            expression(e); // 调用表达式分析函数，对应产生式<表达式> ::= <项>{+<项>|-<项>}，参数e用于存储表达式的结果位置（临时变量或标识符）
            if (type == DEL && strcmp(token, ")") == 0) { // 如果当前记号是右括号，说明是合法的返回语句开始
                lexicalAnalysis(); // 获取下一个记号
                if (type == DEL && strcmp(token, ";") == 0) { // 如果当前记号是分号，说明是合法的返回语句结束
                    emitQuad("RET", e, "", ""); // 生成一个RET四元式，表示返回表达式的结果
                    lexicalAnalysis(); // 获取下一个记号，为后续的语法分析做准备
                } else { // 如果当前记号不是分号，说明是语法错误
                    error("Missing ;");
                }
            } else { // 如果当前记号不是右括号，说明是语法错误
                error("Missing )");
            }
        } else { // 如果当前记号不是分号或左括号，说明是语法错误
            error("Invalid return statement");
        }
    } else { // 如果当前记号不是return关键字，说明是语法错误
        error("Missing return");
    }
}

// 错误处理函数，打印错误信息并退出程序
void error(char *msg) {
    printf("Error: %s\n", msg);
    exit(1);
}

// 判断是否为关键字，是则返回其序号，否则返回-1
int isKeyword(char *token) {
    for (int i = 0; i < KEYNUM; i++) {
        if (strcmp(token, keywords[i]) == 0) {
            return i;
        }
    }
    return -1;
}

// 判断是否为字母或下划线
int isLetter(char ch) {
    return isalpha(ch) || ch == '_';
}

// 判断是否为运算符，是则返回其序号，否则返回-1
int isOperator(char ch) {
    char ops[] = "+-*/%<>=!&|"; // 运算符集合
    for (int i = 0; i < strlen(ops); i++) {
        if (ch == ops[i]) {
            return i;
        }
    }
    return -1;
}

// 判断是否为界符，是则返回其序号，否则返回-1
int isDelimiter(char ch) {
    char dels[] = "(),;{}"; // 界符集合
    for (int i = 0; i < strlen(dels); i++) {
        if (ch == dels[i]) {
            return i;
        }
    }
    return -1;
}

// 打印记号信息（可选）
void printToken(enum TokenType type, char *token) {
    switch (type) {
        case ID:
            printf("<ID, %s>\n", token);
            break;
        case KEY:
            printf("<KEY, %s>\n", token);
            break;
        case NUM:
            printf("<NUM, %s>\n", token);
            break;
        case OP:
            printf("<OP, %s>\n", token);
            break;
        case DEL:
            printf("<DEL, %s>\n", token);
            break;
        case ERR:
            printf("<ERR, %s>\n", token);
            break;
        default:
            break;
    }
}

// 查找符号表，返回符号在表中的位置，如果不存在则返回-1
int lookupSymbol(char *name) {
    for (int i = 0; i < symnum; i++) {
        if (strcmp(name, symtab[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

// 插入符号表，如果已存在则报错
void insertSymbol(char *name, enum TokenType type, int value) {
    int index = lookupSymbol(name); // 查找符号表中是否有该标识符
    if (index != -1) { // 如果已存在，报错并退出程序
        error("Duplicate declaration");
    } else { // 如果不存在，插入到符号表中
        strcpy(symtab[symnum].name, name); // 复制标识符名到符号表中
        symtab[symnum].type = type; // 设置标识符类别
        symtab[symnum].value = value; // 设置标识符值
        symnum++; // 增加符号表大小
    }
}

// 更新符号表中的值
void updateSymbol(int index, int value) {
    if (index >= 0 && index < symnum) { // 如果索引合法，更新符号表中的值
        symtab[index].value = value;
    } else { // 如果索引不合法，报错并退出程序
        error("Invalid symbol index");
    }
}

// 生成一个新的临时变量名
char *newTemp() {
    static int count = 0; // 用于记录临时变量的个数
    char *temp = (char *)malloc(MAXLEN * sizeof(char)); // 分配内存空间
    sprintf(temp, "t%d", count++); // 生成临时变量名，如t0, t1, t2, ...
    return temp;
}

// 生成一个新的标号名
char *newLabel() {
    static int count = 0; // 用于记录标号的个数
    char *label = (char *)malloc(MAXLEN * sizeof(char)); // 分配内存空间
    sprintf(label, "L%d", count++); // 生成标号名，如L0, L1, L2, ...
    return label;
}

// 生成一个四元式并加入到四元式序列中
void emitQuad(char *op, char *arg1, char *arg2, char *result) {
    strcpy(quadtab[quadnum].op, op); // 复制操作符到四元式中
    strcpy(quadtab[quadnum].arg1, arg1); // 复制第一个操作数到四元式中
    strcpy(quadtab[quadnum].arg2, arg2); // 复制第二个操作数到四元式中
    strcpy(quadtab[quadnum].result, result); // 复制结果到四元式中
    quadnum++; // 增加四元式序列大小
}

// 回填跳转标号到指定的四元式位置
void backpatch(char *label, int quadpos) {
    if (quadpos >= 0 && quadpos < quadnum) { // 如果位置合法，回填跳转标号到四元式的结果字段中
        strcpy(quadtab[quadpos].result, label);
    } else { // 如果位置不合法，报错并退出程序
        error("Invalid quadruple position");
    }
}

// 打印四元式信息（可选）
void printQuad(struct Quadruple quad) {
    printf("(%s, %s, %s, %s)\n", quad.op, quad.arg1, quad.arg2, quad.result);
}

// 打印四元式序列信息（可选）
void printQuadList() {
    printf("Quadruple list:\n");
    for (int i = 0; i < quadnum; i++) {
        printf("%d: ", i);
        printQuad(quadtab[i]);
    }
}

// 打印符号表项信息（可选）
void printSymbol(struct Symbol sym) {
    printf("<%s, %d, %d>\n", sym.name, sym.type, sym.value);
}

// 打印符号表信息（可选）
void printSymbolList() {
    printf("Symbol list:\n");
    for (int i = 0; i < symnum; i++) {
        printSymbol(symtab[i]);
    }
}

// 语义分析函数，检查源程序的语义正确性并填充符号表和四元式序列中的值和地址信息
void semanticAnalysis() {
    // 遍历四元式序列，对每个四元式进行语义检查和处理
    for (int i = 0; i < quadnum; i++) {
        struct Quadruple quad = quadtab[i]; // 获取当前四元式
        if (strcmp(quad.op, "DEC") == 0) { // 如果是DEC四元式，表示为标识符分配空间
            int index = lookupSymbol(quad.result); // 查找符号表中是否有该标识符
            if (index != -1) { // 如果找到了，设置其地址为当前的偏移量
                symtab[index].address = offset;
                offset += 4; // 增加偏移量（这里假设每个变量占4个字节）
            } else { // 如果没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "=") == 0) { // 如果是赋值四元式，表示将表达式的结果赋给标识符
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数（表达式的结果）
            int index2 = lookupSymbol(quad.result); // 查找符号表中是否有结果（标识符）
            if (index1 != -1 && index2 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type) { // 如果类型匹配，更新符号表中的值
                    symtab[index2].value = symtab[index1].value;
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "+") == 0 || strcmp(quad.op, "-") == 0 || strcmp(quad.op, "*") == 0 || strcmp(quad.op, "/") == 0 || strcmp(quad.op, "%") == 0) { // 如果是算术运算四元式，表示将两个操作数进行运算并将结果存入临时变量
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数
            int index2 = lookupSymbol(quad.arg2); // 查找符号表中是否有第二个操作数
            int index3 = lookupSymbol(quad.result); // 查找符号表中是否有结果（临时变量）
            if (index1 != -1 && index2 != -1 && index3 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type && symtab[index2].type == symtab[index3].type) { // 如果类型匹配，根据不同的运算符进行计算并更新符号表中的值
                    switch (quad.op[0]) {
                        case '+':
                            symtab[index3].value = symtab[index1].value + symtab[index2].value;
                            break;
                        case '-':
                            symtab[index3].value = symtab[index1].value - symtab[index2].value;
                            break;
                        case '*':
                            symtab[index3].value = symtab[index1].value * symtab[index2].value;
                            break;
                        case '/':
                            if (symtab[index2].value != 0) { // 检查除数是否为零
                                symtab[index3].value = symtab[index1].value / symtab[index2].value;
                            } else { // 如果除数为零，说明是语义错误
                                error("Divide by zero");
                            }
                            break;
                        case '%':
                            if (symtab[index2].value != 0) { // 检查除数是否为零
                                symtab[index3].value = symtab[index1].value % symtab[index2].value;
                            } else { // 如果除数为零，说明是语义错误
                                error("Divide by zero");
                            }
                            break;
                        default:
                            break;
                    }
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "<") == 0 || strcmp(quad.op, "<=") == 0 || strcmp(quad.op, ">") == 0 || strcmp(quad.op, ">=") == 0 || strcmp(quad.op, "==") == 0 || strcmp(quad.op, "!=") == 0) { // 如果是关系运算四元式，表示将两个操作数进行比较并根据结果跳转到指定的标号
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数
            int index2 = lookupSymbol(quad.arg2); // 查找符号表中是否有第二个操作数
            if (index1 != -1 && index2 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type) { // 如果类型匹配，根据不同的运算符进行比较并设置条件标志位
                    switch (quad.op[0]) {
                        case '<':
                            flag = symtab[index1].value < symtab[index2].value;
                            break;
                        case '>':
                            flag = symtab[index1].value > symtab[index2].value;
                            break;
                        case '=':
                            flag = symtab[index1].value == symtab[index2].value;
                            break;
                        case '!':
                            flag = symtab[index1].value != symtab[index2].value;
                            break;
                        default:
                            break;
                    }
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "JMP") == 0) { // 如果是无条件跳转四元式，表示跳转到指定的标号
            // 不需要进行语义检查和处理，直接跳转即可
        } else if (strcmp(quad.op, "RET") == 0) { // 如果是返回四元式，表示返回主函数或返回表达式的结果
            int index = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数（表达式的结果）
            if (index != -1) { // 如果找到了，检查其类型是否为int或char
                if (symtab[index].type == ID || symtab[index].type == NUM) { // 如果类型为int或char，返回其值即可
                    return symtab[index].value;
                } else { // 如果类型不为int或char，说明是语义错误
                    error("Invalid return type");
                }
            } else { // 如果没有找到，说明是返回主函数，不需要进行语义检查和处理
                return 0;
            }
        } else { // 如果是其他情况，说明是语法错误（不应该出现）
            error("Invalid quadruple");
        }
    }
}

// 目标代码生成函数，根据四元式序列和符号表生成目标代码并输出到文件中
void codeGeneration() {
    FILE *fp = fopen("target.txt", "w"); // 打开目标代码文件
    if (fp == NULL) { // 如果打开失败，报错并退出程序
        error("Cannot open target file");
    }
    // 遍历四元式序列，对每个四元式生成对应的目标代码
    for (int i = 0; i < quadnum; i++) {
        struct Quadruple quad = quadtab[i]; // 获取当前四元式
        if (strcmp(quad.op, "DEC") == 0) { // 如果是DEC四元式，表示为标识符分配空间
            int index = lookupSymbol(quad.result); // 查找符号表中是否有该标识符
            if (index != -1) { // 如果找到了，生成一条SUB指令，表示从栈顶减去4个字节（这里假设每个变量占4个字节）
                fprintf(fp, "SUB $sp, $sp, 4\n");
            } else { // 如果没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "=") == 0) { // 如果是赋值四元式，表示将表达式的结果赋给标识符
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数（表达式的结果）
            int index2 = lookupSymbol(quad.result); // 查找符号表中是否有结果（标识符）
            if (index1 != -1 && index2 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type) { // 如果类型匹配，生成一条LW指令和一条SW指令，表示将表达式的结果从内存中加载到寄存器中，然后再存回到标识符的地址中
                    fprintf(fp, "LW $t0, %d($sp)\n", symtab[index1].address);
                    fprintf(fp, "SW $t0, %d($sp)\n", symtab[index2].address);
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "+") == 0 || strcmp(quad.op, "-") == 0 || strcmp(quad.op, "*") == 0 || strcmp(quad.op, "/") == 0 || strcmp(quad.op, "%") == 0) { // 如果是算术运算四元式，表示将两个操作数进行运算并将结果存入临时变量
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数
            int index2 = lookupSymbol(quad.arg2); // 查找符号表中是否有第二个操作数
            int index3 = lookupSymbol(quad.result); // 查找符号表中是否有结果（临时变量）
            if (index1 != -1 && index2 != -1 && index3 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type && symtab[index2].type == symtab[index3].type) { // 如果类型匹配，生成两条LW指令和一条对应的算术指令，表示将两个操作数从内存中加载到寄存器中，然后进行运算并将结果存入另一个寄存器中
                    fprintf(fp, "LW $t0, %d($sp)\n", symtab[index1].address);
                    fprintf(fp, "LW $t1, %d($sp)\n", symtab[index2].address);
                    switch (quad.op[0]) {
                        case '+':
                            fprintf(fp, "ADD $t2, $t0, $t1\n");
                            break;
                        case '-':
                            fprintf(fp, "SUB $t2, $t0, $t1\n");
                            break;
                        case '*':
                            fprintf(fp, "MUL $t2, $t0, $t1\n");
                            break;
                        case '/':
                            fprintf(fp, "DIV $t2, $t0, $t1\n");
                            break;
                        case '%':
                            fprintf(fp, "REM $t2, $t0, $t1\n");
                            break;
                        default:
                            break;
                    }
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "<") == 0 || strcmp(quad.op, "<=") == 0 || strcmp(quad.op, ">") == 0 || strcmp(quad.op, ">=") == 0 || strcmp(quad.op, "==") == 0 || strcmp(quad.op, "!=") == 0) { // 如果是关系运算四元式，表示将两个操作数进行比较并根据结果跳转到指定的标号
            int index1 = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数
            int index2 = lookupSymbol(quad.arg2); // 查找符号表中是否有第二个操作数
            if (index1 != -1 && index2 != -1) { // 如果都找到了，检查它们的类型是否匹配
                if (symtab[index1].type == symtab[index2].type) { // 如果类型匹配，生成两条LW指令和一条对应的分支指令，表示将两个操作数从内存中加载到寄存器中，然后进行比较并根据结果跳转到指定的标号
                    fprintf(fp, "LW $t0, %d($sp)\n", symtab[index1].address);
                    fprintf(fp, "LW $t1, %d($sp)\n", symtab[index2].address);
                    switch (quad.op[0]) {
                        case '<':
                            if (quad.op[1] == '=') {
                                fprintf(fp, "BLE $t0, $t1, %s\n", quad.result);
                            } else {
                                fprintf(fp, "BLT $t0, $t1, %s\n", quad.result);
                            }
                            break;
                        case '>':
                            if (quad.op[1] == '=') {
                                fprintf(fp, "BGE $t0, $t1, %s\n", quad.result);
                            } else {
                                fprintf(fp, "BGT $t0, $t1, %s\n", quad.result);
                            }
                            break;
                        case '=':
                            fprintf(fp, "BEQ $t0, $t1, %s\n", quad.result);
                            break;
                        case '!':
                            fprintf(fp, "BNE $t0, $t1, %s\n", quad.result);
                            break;
                        default:
                            break;
                    }
                } else { // 如果类型不匹配，说明是语义错误
                    error("Type mismatch");
                }
            } else { // 如果有一个没有找到，说明是语义错误
                error("Undeclared identifier");
            }
        } else if (strcmp(quad.op, "JMP") == 0) { // 如果是无条件跳转四元式，表示跳转到指定的标号
            fprintf(fp, "J %s\n", quad.result); // 生成一条J指令，表示跳转到指定的标号
        } else if (strcmp(quad.op, "RET") == 0) { // 如果是返回四元式，表示返回主函数或返回表达式的结果
            int index = lookupSymbol(quad.arg1); // 查找符号表中是否有第一个操作数（表达式的结果）
            if (index != -1) { // 如果找到了，检查其类型是否为int或char
                if (symtab[index].type == ID || symtab[index].type == NUM) { // 如果类型为int或char，生成一条LW指令和一条JR指令，表示将表达式的结果从内存中加载到寄存器中，然后返回主函数
                    fprintf(fp, "LW $v0, %d($sp)\n", symtab[index].address);
                    fprintf(fp, "JR $ra\n");
                } else { // 如果类型不为int或char，说明是语义错误
                    error("Invalid return type");
                }
            } else { // 如果没有找到，说明是返回主函数，生成一条JR指令，表示返回主函数
                fprintf(fp, "JR $ra\n");
            }
        } else { // 如果是其他情况，说明是语法错误（不应该出现）
            error("Invalid quadruple");
        }
    }
    fclose(fp); // 关闭目标代码文件
}


// 主函数，打开源程序文件并调用词法分析、语法分析、语义分析和目标代码生成函数
int main(int argc, char *argv[]) {
    if (argc < 2) { // 如果没有指定源程序文件名，报错并退出程序
        error("Missing source file name");
    }
    fp = fopen(argv[1], "r"); // 打开源程序文件
    if (fp == NULL) { // 如果打开失败，报错并退出程序
        error("Cannot open source file");
    }
    syntaxAnalysis(); // 调用语法分析函数，分析源程序的语法结构并生成四元式序列
    semanticAnalysis(); // 调用语义分析函数，检查源程序的语义正确性并填充符号表和四元式序列中的值和地址信息
    codeGeneration(); // 调用目标代码生成函数，根据四元式序列和符号表生成目标代码并输出到文件中
    fclose(fp); // 关闭源程序文件
    return 0;
}



