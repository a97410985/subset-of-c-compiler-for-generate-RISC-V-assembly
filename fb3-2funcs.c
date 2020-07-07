#  include <stdio.h>
#  include <stdlib.h>
#  include <stdarg.h>
#  include <string.h>
#  include <math.h>
#  include "fb3-2.h"

int global_counter = 0;


/* symbol table */
/* hash a symbol */
static unsigned
symhash(char *sym) {
    unsigned int hash = 0;
    unsigned c;

    while (c = *sym++) hash = hash * 9 ^ c;

    return hash;
}

struct symbol *
lookup(char *sym) {
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;        /* how many have we looked at */

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) { return sp; }

        if (!sp->name) {        /* new entry */
            sp->name = strdup(sym);
            sp->valueType = real;
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }

        if (++sp >= symtab + NHASH) sp = symtab; /* try the next entry */
    }
    yyerror("symbol table overflow\n");
    abort(); /* tried them all, table is full */

}


void arrdeclare(struct symbol *s, double size) {
    printf("arrdeclare\n");
    s->mLoc = cur_stk_size;
    s->arrSize = size;
    cur_stk_size += 4 * size;
}

void arrdeclare_v(struct symbol *s, struct symbol *size_s) {
    printf("arrdeclare\n");
    s->mLoc = cur_stk_size;
    s->arrSize = size_s->value;
    cur_stk_size += 4 * size_s->value;
}


struct ast *test(struct symlist *sl, enum ValueType valueType) {
    struct symref *a = malloc(sizeof(struct symref));

    struct symlist *curSl = sl;
    int count = 0;
    while (curSl != NULL) {
        count++;
        curSl->sym->valueType = valueType;
        curSl = curSl->next;
    }
    a->nodetype = 'N';
    a->s = sl->sym;
    a->s->valueType = valueType;
    a->code = "";
    return (struct ast *) a;
}

struct ast *newStmtList(struct ast *stmtList, struct ast *stmt) {
    struct ast *a = malloc(sizeof(struct symref));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'L';
    a->code = "";
    a->place = "";

    char *temp = malloc(sizeof(char) * 80);
    sprintf(temp, "%s%s\n", stmtList->code, stmt->code);
    a->code = temp;
    return a;
}

struct ast *newincrement(struct symbol *s) {
    struct symref *a = malloc(sizeof(struct symref));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'K';
    a->code = "";
    a->place = s->place;
    a->s = s;

    char *temp = malloc(sizeof(char) * 80);
    sprintf(temp, "addi %s, %s, 1\n", s->place, s->place);
    a->code = temp;
    return (struct ast *) a;
}

struct ast *newfor(struct ast *ini_a, struct ast *cmp_a, struct ast *chg_a, struct ast *stmtList) {
    struct ast *a = malloc(sizeof(struct ast));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    printf("new for\n");
    a->nodetype = 'f';
    a->l = NULL;
    a->r = NULL;
    char *temp = malloc(sizeof(char) * 500);
    // 變數初始化
    strcat(temp, ini_a->code);
    // 迴圈開始的標籤
    strcat(temp, "for_loop:\n");
    // 判斷是否要跳離 以及 jump的code
    strcat(temp, cmp_a->code);
    // 迴圈的body
    strcat(temp, stmtList->code);
    // 變數變化
    strcat(temp, chg_a->code);

    // 跳回去開頭
    strcat(temp, "j for_loop\n");


    // 迴圈結束的標籤
    strcat(temp, "end:\n");

    a->code = temp;
    a->place = "";
    printf("new for end\n");

    return a;
}

struct ast *
newast(int nodetype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    printf("new ast\n");
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    a->code = "";
    a->place = "t6";
    // 根據節點類型生成不同的程式碼
    if (nodetype == '+') {
        char *temp = malloc(sizeof(char) * 80);
        printf("a->l->nodetype %c\n", a->l->nodetype);
        printf("->l->place %s\n", a->l->place);
        printf("a->r->nodetype %c\n", a->r->nodetype);
        printf("a->r->place %s\n", a->r->place);


        strcat(temp, a->l->code);
        strcat(temp, a->r->code);
        if (a->l->nodetype == 'K' || a->r->nodetype == 'K') {
            strcat(temp, "addi ");
            if (a->l->nodetype == 'K') {
                char *temp2 = malloc(sizeof(char) * 80);
                sprintf(temp2, "%s, %s, %.0f\n", a->place, a->r->place, ((struct numval *) a->l)->number);
                strcat(temp, temp2);
            } else {
                char *temp2 = malloc(sizeof(char) * 80);
                sprintf(temp2, "%s, %s, %.0f\n", a->place, a->l->place, ((struct numval *) a->r)->number);
                strcat(temp, temp2);
            }
        } else if (a->l->nodetype == 'A' || a->r->nodetype == 'A') {
            char *temp2 = malloc(sizeof(char) * 80);
            if (a->l->nodetype == 'A' && a->r->nodetype == 'A') {
                // 生成的程式碼
                sprintf(temp2, "lw a6, %d(sp)\nlw a7, %d(sp)\nadd t6, a6, a7\n",
                        cur_stk_size - ((struct arrref *) a->l)->index * 4 - 4,
                        cur_stk_size - ((struct arrref *) a->r)->index * 4 - 4);
                strcat(temp, temp2);
            } else if (a->r->nodetype == 'A') {
                // 生成的程式碼
                sprintf(temp2, "lw a7, %d(sp)\nadd t6, %s, a7\n",
                        cur_stk_size - ((struct arrref *) a->r)->index * 4 - 4,
                        a->l->place);
                strcat(temp, temp2);
            }
        } else {
            strcat(temp, "add t6,");
            strcat(temp, a->l->place);
            strcat(temp, ", ");
            strcat(temp, a->r->place);
            strcat(temp, "\n");
        }


        a->code = temp;
    } else if (nodetype == '-') {
        printf("new ast & - : a->l->nodetype : %c\n", a->l->nodetype);
        printf("new ast & - : a->l->nodetype : %c\n", a->r->nodetype);
        char *temp = malloc(sizeof(char) * 80);
        if (a->l->nodetype == 'N' || a->r->nodetype == 'N') {
            if (((struct symref *) a->l)->s->macroTF == 1) {
                strcat(temp, a->l->code);
                strcat(temp, a->r->code);
                char *temp2 = malloc(sizeof(char) * 80);
                sprintf(temp2, "neg %s, %s\n", a->r->place, a->r->place);
                strcat(temp, temp2);
                char *temp3 = malloc(sizeof(char) * 80);
                sprintf(temp3, "addi t6, %s, %.0f\n", a->r->place, ((struct symref *) a->l)->s->value);
                strcat(temp, temp3);

            }
        } else {
            strcat(temp, a->l->code);
            strcat(temp, a->r->code);
            strcat(temp, "sub t6, ");
            strcat(temp, a->l->place);
            strcat(temp, ", ");
            strcat(temp, a->r->place);
            strcat(temp, "\n");
        }

        a->code = temp;
    } else if (nodetype == '*') {
        char *temp = malloc(sizeof(char) * 80);
        strcat(temp, a->l->code);
        strcat(temp, a->r->code);
        strcat(temp, "mul t6, ");
        strcat(temp, a->l->place);
        strcat(temp, ", ");
        strcat(temp, a->r->place);
        strcat(temp, "\n");
        a->code = temp;
    } else if (nodetype == '/') {
        char *temp = malloc(sizeof(char) * 80);
        strcat(temp, a->l->code);
        strcat(temp, a->r->code);
        strcat(temp, "div t6, ");
        strcat(temp, a->l->place);
        strcat(temp, ", ");
        strcat(temp, a->r->place);
        strcat(temp, "\n");
        a->code = temp;
    } else if (nodetype == 'A') {
        // array reference
        printf("array reference in RHS\n");;

    }

    printf("haha ast code : %s\n", a->code);
    return a;
}

struct ast *
newnum(double d) {
    struct numval *a = malloc(sizeof(struct numval));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'K';
    a->number = d;
    a->code = "";
    a->place = "";
    return (struct ast *) a;
}

struct ast *
newcmp(int cmptype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;
    a->code = "";
    a->place = "";
//    ">"     { yylval.fn = 1; return CMP; }
//    "<"     { yylval.fn = 2; return CMP; }
//    "<>"    { yylval.fn = 3; return CMP; }
//    "=="    { yylval.fn = 4; return CMP; }
//    ">="    { yylval.fn = 5; return CMP; }
//    "<="    { yylval.fn = 6; return CMP; }
    if (cmptype == 2) {
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "bge %s, %s, end\n", l->place, r->place);
        a->code = temp;
    }
    return a;
}

struct ast *
newfunc(int functype, struct ast *l) {
    struct fncall *a = malloc(sizeof(struct fncall));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    a->code = "";
    return (struct ast *) a;
}

struct ast *
newcall(struct symbol *s, struct ast *l) {
    struct ufncall *a = malloc(sizeof(struct ufncall));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'C';
    a->l = l;
    a->s = s;
    a->code = "";
    return (struct ast *) a;
}

struct ast *newmacro(struct symbol *s, double v) {
    struct symref *a = malloc(sizeof(struct symref));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'N';
    a->s = s;
    a->s->valueType = real;
    a->s->macroTF = 1;
    s->value = v;
    a->code = "";
    a->place = "";
    return (struct ast *) a;
}

struct ast *newarrref(struct symbol *s, double num) {
    struct arrref *a = malloc(sizeof(struct arrref));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'A';
    a->s = s;
    a->code = "";
    a->place = "a7";
    a->index = num;
    return (struct ast *) a;
}

struct ast *newarrref_v(struct symbol *s, struct symbol* index_s)
{
    struct arrref *a = malloc(sizeof(struct arrref));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'A';
    a->s = s;
    a->code = "";
    a->place = "a7";
    a->index = index_s->name;
    return (struct ast *) a;
}

struct ast *
newref(struct symbol *s) {
    struct symref *a = malloc(sizeof(struct symref));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'N';
    a->s = s;
    a->s->valueType = real;
    a->code = "";
    a->place = s->place;
    return (struct ast *) a;
}

struct ast *newarrasgn(struct symbol *s, double num, struct ast *a) {
    struct arrref *af = malloc(sizeof(struct arrref));
    if (!af) {
        yyerror("out of space");
        exit(0);
    }

    if (curparseState == DeclareState) {
        curparseState = ComputeState;
        if (cur_stk_size != 0) {
            // 要生出挖stack的code
            printf("要生出挖stack的code\n");
            char *temp = malloc(sizeof(char) * 80);
            sprintf(temp, "addi sp, sp, -%d\n", cur_stk_size);
            printf("newarrasgn generate code %s \n", temp);
            //if (generate_TF) strcat(totalCode, temp);
        }
    }

    af->nodetype = 'A';
    af->s = s;
    af->s->place = "a7";
    af->code = "";
    af->place = "a7";
    af->index = num;

    printf("newarrasgn , nodetype %c\n", a->nodetype);

    if (a->nodetype == 'K') { // 賦值運算子右邊是常數
        printf("newarrasgn : 賦值運算子右邊是常數\n");
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "li %s, %.0f\nsw %s, %.0f(sp)\n", s->place, ((struct numval *) a)->number, s->place,
                cur_stk_size - s->mLoc - num * 4 - 4);
        af->code = temp;
        printf("newarrasgn generate code %s \n", af->code);
        //if (generate_TF) strcat(totalCode, af->code);
    } else if (a->nodetype == 'N') { // symbol ref
        printf("NNNNNNNNNNNNNNNN\n");
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "%ssw %s, %.0f(sp)\n", a->code, a->place, cur_stk_size - s->mLoc - num * 4 - 4);
        af->code = temp;
        printf("newarrasgn generate code %s \n", a->code);
    } else if (a->nodetype == 'A') {
        printf("AAAAAAAAAAAAAAAAAa\n");
    } else { // 加減乘除運算的結果
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "%ssw %s, %.0f(sp)\n", a->code, a->place, cur_stk_size - s->mLoc - num * 4 - 4);
        af->code = temp;
        printf("newarrasgn generate code %s \n", a->code);
    }

    return (struct ast *) af;
}


struct ast *newarrasgn_v(struct symbol *s, struct symbol *index_s, struct ast *a) {
    struct arrref *af = malloc(sizeof(struct arrref));
    if (!af) {
        yyerror("out of space");
        exit(0);
    }

    if (curparseState == DeclareState) {
        curparseState = ComputeState;
        if (cur_stk_size != 0) {
            // 要生出挖stack的code
            printf("要生出挖stack的code\n");
            char *temp = malloc(sizeof(char) * 80);
            sprintf(temp, "addi sp, sp, -%d\n", cur_stk_size);
            printf("newarrasgn generate code %s \n", temp);
            //if (generate_TF) strcat(totalCode, temp);
        }
    }

    af->nodetype = 'A';
    af->s = s;
    af->s->place = "a7";
    af->code = "";
    af->place = "a7";
    af->index = index_s->value;

    if (a->nodetype == 'K') { // 賦值運算子右邊是常數
        printf("newarrasgn : 賦值運算子右邊是常數\n");
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "li %s, %.0f\nsw %s, %d(sp)\n", s->place, ((struct numval *) a)->number, s->place,
                cur_stk_size - s->mLoc - 4);
        af->code = temp;
        printf("newarrasgn generate code %s \n", af->code);
        //if (generate_TF) strcat(totalCode, af->code);
    }

    return (struct ast *) af;
}


struct ast *
newasgn(struct symbol *s, struct ast *v) {
    struct symasgn *a = malloc(sizeof(struct symasgn));
    printf("newasgn value's type %c\n", v->nodetype);

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    if (s->valueType == integer) {
        printf("newasgn : symbol valueType is integer\n");
    } else {
        printf("newasgn : symbol valueType is real\n");

    }

    if (curparseState == DeclareState) {
        curparseState = ComputeState;
        if (cur_stk_size != 0) {
            // 要生出挖stack的code
            printf("要生出挖stack的code\n");
            char *temp = malloc(sizeof(char) * 80);
            sprintf(temp, "addi sp, sp, -%d\n", cur_stk_size);
            printf("newarrasgn generate code %s \n", temp);
            //if (generate_TF) strcat(totalCode, temp);
        }
    }

    a->nodetype = '=';
    a->s = s;
    a->v = v;
    a->code = "";
    a->place = s->place;
    // 如果右手邊的value是常數，就用li指令生程式
    if (a->v->nodetype == 'K') {
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "li %s, %.0f\n", s->place, ((struct numval *) v)->number);
        a->code = temp;
        printf("newasgn generate code %s \n", a->code);
        //if (generate_TF) strcat(totalCode, a->code);

    } else if (a->v->nodetype == 'N') {
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "li %s, %s\n", s->place, ((struct symref *) v)->s->place);
        a->code = temp;
        printf("newasgn generate code %s \n", a->code);
        //if (generate_TF) strcat(totalCode, a->code);
    } else if (a->v->nodetype == 'A') { // 陣列參考
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "lw a7, %d(sp)\nadd %s, a7, x0\n", cur_stk_size - ((struct arrref *) v)->index * 4 - 4, s->place);
        a->code = temp;
        printf("newasgn generate code %s \n", a->code);
        //if (generate_TF) strcat(totalCode, a->code);
    } else {// 如果右手邊的value是變數或是運算的結果
        char *temp = malloc(sizeof(char) * 80);
        sprintf(temp, "%sadd %s, t6, x0\n", a->v->code, a->place);
        a->code = temp;
        printf("newasgn generate code %s \n", a->code);
        //if (generate_TF) strcat(totalCode, a->code);

    }
    return (struct ast *) a;
}

struct ast *
newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el) {
    struct flow *a = malloc(sizeof(struct flow));

    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;
    a->code = "";
    return (struct ast *) a;
}

struct symlist *
newsymlist(struct symbol *sym, struct symlist *next) {
    struct symlist *sl = malloc(sizeof(struct symlist));

    if (!sl) {
        yyerror("out of space");
        exit(0);
    }
    char *temp = malloc(sizeof(char) * 80);
    sprintf(temp, "t%d", global_counter++);
    sym->place = temp;
    sl->sym = sym;
    sl->next = next;
    printf("newsymlist %s在%s\n", sym->name, sym->place);
    return sl;
}

void
symlistfree(struct symlist *sl) {
    struct symlist *nsl;

    while (sl) {
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

/* define a function */
void
dodef(struct symbol *name, struct symlist *syms, struct ast *func) {
    if (name->syms) symlistfree(name->syms);
    if (name->func) treefree(name->func);
    name->syms = syms;
    name->func = func;
}

static double callbuiltin(struct fncall *);

static double calluser(struct ufncall *);

double
eval(struct ast *a) {
    double v;


    switch (a->nodetype) {
        /* constant */
        case 'K':
            v = ((struct numval *) a)->number;
            break;

            /* name reference */
        case 'N':
            v = ((struct symref *) a)->s->value;
            break;

            /* assignment */
        case '=': {
            double tempValue = eval(((struct symasgn *) a)->v);
            if (((struct symasgn *) a)->s->valueType == integer) {
                tempValue = floor(tempValue);
            }
            v = ((struct symasgn *) a)->s->value = tempValue;
            break;
        }
            /* expressions */
        case '+':
            v = eval(a->l) + eval(a->r);
            break;
        case '-':
            v = eval(a->l) - eval(a->r);
            break;
        case '*':
            v = eval(a->l) * eval(a->r);
            break;
        case '/':
            v = eval(a->l) / eval(a->r);
            break;
        case '|':
            v = fabs(eval(a->l));
            break;
        case 'M':
            v = -eval(a->l);
            break;

            /* comparisons */
        case '1':
            v = (eval(a->l) > eval(a->r)) ? 1 : 0;
            break;
        case '2':
            v = (eval(a->l) < eval(a->r)) ? 1 : 0;
            break;
        case '3':
            v = (eval(a->l) != eval(a->r)) ? 1 : 0;
            break;
        case '4':
            v = (eval(a->l) == eval(a->r)) ? 1 : 0;
            break;
        case '5':
            v = (eval(a->l) >= eval(a->r)) ? 1 : 0;
            break;
        case '6':
            v = (eval(a->l) <= eval(a->r)) ? 1 : 0;
            break;

            /* control flow */
            /* null if/else/do expressions allowed in the grammar, so check for them */
        case 'I':
            if (eval(((struct flow *) a)->cond) != 0) {
                if (((struct flow *) a)->tl) {
                    v = eval(((struct flow *) a)->tl);
                } else
                    v = 0.0;        /* a default value */
            } else {
                if (((struct flow *) a)->el) {
                    v = eval(((struct flow *) a)->el);
                } else
                    v = 0.0;        /* a default value */
            }
            break;

        case 'W':
            v = 0.0;        /* a default value */

            if (((struct flow *) a)->tl) {
                while (eval(((struct flow *) a)->cond) != 0)
                    v = eval(((struct flow *) a)->tl);
            }
            break;            /* last value is value */

        case 'L':
            eval(a->l);
            v = eval(a->r);
            break;

        case 'F':
            v = callbuiltin((struct fncall *) a);
            break;

        case 'C':
            v = calluser((struct ufncall *) a);
            break;

        default:
            printf("internal error: bad node %c\n", a->nodetype);
    }
    return v;
}

static double
callbuiltin(struct fncall *f) {
    enum bifs functype = f->functype;
    double v = eval(f->l);

    switch (functype) {
        case B_sqrt:
            return sqrt(v);
        case B_exp:
            return exp(v);
        case B_log:
            return log(v);
        case B_print:
            printf("= %4.4g\n", v);
            return v;
        default:
            yyerror("Unknown built-in function %d", functype);
            return 0.0;
    }
}

static double
calluser(struct ufncall *f) {
    struct symbol *fn = f->s;    /* function name */
    struct symlist *sl;        /* dummy arguments */
    struct ast *args = f->l;    /* actual arguments */
    double *oldval, *newval;    /* saved arg values */
    double v;
    int nargs;
    int i;

    if (!fn->func) {
        yyerror("call to undefined function", fn->name);
        return 0;
    }

    /* count the arguments */
    sl = fn->syms;
    for (nargs = 0; sl; sl = sl->next)
        nargs++;

    /* prepare to save them */
    oldval = (double *) malloc(nargs * sizeof(double));
    newval = (double *) malloc(nargs * sizeof(double));
    if (!oldval || !newval) {
        yyerror("Out of space in %s", fn->name);
        return 0.0;
    }

    /* evaluate the arguments */
    for (i = 0; i < nargs; i++) {
        if (!args) {
            yyerror("too few args in call to %s", fn->name);
            free(oldval);
            free(newval);
            return 0;
        }

        if (args->nodetype == 'L') {    /* if this is a list node */
            newval[i] = eval(args->l);
            args = args->r;
        } else {            /* if it's the end of the list */
            newval[i] = eval(args);
            args = NULL;
        }
    }

    /* save old values of dummies, assign new ones */
    sl = fn->syms;
    for (i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;

        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }

    free(newval);

    /* evaluate the function */
    v = eval(fn->func);

    /* put the dummies back */
    sl = fn->syms;
    for (i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;

        s->value = oldval[i];
        sl = sl->next;
    }

    free(oldval);
    return v;
}


void
treefree(struct ast *a) {
    switch (a->nodetype) {

        /* two subtrees */
        case '+':
        case '-':
        case '*':
        case '/':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case 'L':
            treefree(a->r);

            /* one subtree */
        case '|':
        case 'M':
        case 'C':
        case 'F':
            treefree(a->l);

            /* no subtree */
        case 'K':
        case 'N':
            break;

        case '=':
            free(((struct symasgn *) a)->v);
            break;

        case 'I':
        case 'W':
            free(((struct flow *) a)->cond);
            if (((struct flow *) a)->tl) free(((struct flow *) a)->tl);
            if (((struct flow *) a)->el) free(((struct flow *) a)->el);
            break;

        default:
            printf("internal error: free bad node %c\n", a->nodetype);
    }

    free(a); /* always free the node itself */

}

void
yyerror(char *s, ...) {
    va_list ap;
    va_start(ap, s);

    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}

int
main() {
    curparseState = DeclareState;
    cur_stk_size = 0;
    FILE *fileInput;
    fileInput = fopen("test_program.txt", "r");
    if (fileInput == NULL) {
        printf("can't read file\n");
        return -1;
    }
    parse(fileInput);
}

/* debugging: dump out an AST */
int debug = 1;

void
dumpast(struct ast *a, int level) {

    printf("%*s", 2 * level, "");    /* indent to this level */
    level++;

    if (!a) {
        printf("NULL\n");
        return;
    }

    switch (a->nodetype) {
        /* constant */
        case 'K':
            printf("number %4.4g\n", ((struct numval *) a)->number);
            break;

            /* name reference */
        case 'N':
            printf("ref %s\n", ((struct symref *) a)->s->name);
            break;

            /* assignment */
        case '=':
            printf("= %s\n", ((struct symref *) a)->s->name);
            dumpast(((struct symasgn *) a)->v, level);
            return;

            /* expressions */
        case '+':
        case '-':
        case '*':
        case '/':
        case 'L':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
            printf("binop %c\n", a->nodetype);
            dumpast(a->l, level);
            dumpast(a->r, level);
            return;

        case '|':
        case 'M':
            printf("unop %c\n", a->nodetype);
            dumpast(a->l, level);
            return;

        case 'I':
        case 'W':
            printf("flow %c\n", a->nodetype);
            dumpast(((struct flow *) a)->cond, level);
            if (((struct flow *) a)->tl)
                dumpast(((struct flow *) a)->tl, level);
            if (((struct flow *) a)->el)
                dumpast(((struct flow *) a)->el, level);
            return;

        case 'F':
            printf("builtin %d\n", ((struct fncall *) a)->functype);
            dumpast(a->l, level);
            return;

        case 'C':
            printf("call %s\n", ((struct ufncall *) a)->s->name);
            dumpast(a->l, level);
            return;

        default:
            printf("bad %c\n", a->nodetype);
            return;
    }
}
