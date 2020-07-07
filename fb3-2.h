enum ValueType {
    integer, real
};

/* symbol table */
struct symbol {        /* a variable name */
    char *name;
    double value;
    enum ValueType valueType;
    struct ast *func;    /* stmt for the function */
    struct symlist *syms; /* list of dummy args */
    char* place;
    int mLoc;
    int arrSize;
    int macroTF;
};

enum parseState {DeclareState, ComputeState};
enum parseState curparseState;
char totalCode[1000];
int cur_stk_size;


/* simple symtab of fixed size */
#define NHASH 9997
struct symbol symtab[NHASH];

struct symbol *lookup(char *);

/* list of symbols, for an argument list */
struct symlist {
    struct symbol *sym;
    struct symlist *next;
};

struct ast *test(struct symlist *sl, enum ValueType valueType);

struct symlist *newsymlist(struct symbol *sym, struct symlist *next);

void symlistfree(struct symlist *sl);

/* node types
 *  + - * / |
 *  0-7 comparison ops, bit coded 04 equal, 02 less, 01 greater
 *  M unary minus
 *  L statement list
 *  I IF statement
 *  W WHILE statement
 *  N symbol ref
 *  = assignment
 *  S list of symbols
 *  F built in function call
 *  C user function call
 */

enum bifs {            /* built-in functions */
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print
};

/* nodes in the Abstract Syntax Tree */
/* all have common initial nodetype */

struct ast {
    int nodetype;
    char *code;
    char *place;
    struct ast *l;
    struct ast *r;
};

struct fncall {            /* built-in function */
    int nodetype;            /* type F */
    char *code;
    char *place;
    struct ast *l;
    enum bifs functype;
};

struct ufncall {        /* user function */
    int nodetype;            /* type C */
    char *code;
    char *place;
    struct ast *l;        /* list of arguments */
    struct symbol *s;
};

struct flow {
    int nodetype;            /* type I or W */
    char *code;
    char *place;
    struct ast *cond;        /* condition */
    struct ast *tl;        /* then or do list */
    struct ast *el;        /* optional else list */
};

struct numval {
    int nodetype;            /* type K */
    char *code;
    char *place;
    double number;
};

struct symref {
    int nodetype;            /* type N */
    char *code;
    char *place;
    struct symbol *s;
};

struct arrref {
    int nodetype;
    char* code;
    char* place;
    struct symbol* s;
    int index;
};

struct symasgn {
    int nodetype;            /* type = */
    const char *code;
    char *place;
    struct symbol *s;
    struct ast *v;        /* value */
};

/* build an AST */
struct ast *newast(int nodetype, struct ast *l, struct ast *r);

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r);

struct ast *newfunc(int functype, struct ast *l);

struct ast *newcall(struct symbol *s, struct ast *l);

struct ast *newref(struct symbol *s);

struct ast *newarrref(struct symbol *s, double num);

struct ast *newarrref_v(struct symbol *s, struct symbol* index_s);

struct ast *newarrasgn(struct symbol* s, double num, struct ast* a);

struct ast *newarrasgn_v(struct symbol* s, struct symbol* index_s, struct ast* a);

struct ast *newasgn(struct symbol *s, struct ast *v);

struct ast *newnum(double d);

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *tr);

struct ast *newfor(struct ast* ini_a, struct ast* cmp_a, struct ast* chg_a, struct ast* stmtList);

struct ast *newincrement(struct symbol *s);

struct ast *newStmtList(struct ast* stmtList, struct ast* stmt);

void arrdeclare(struct symbol* s, double size);
void arrdeclare_v(struct symbol* s, struct symbol* size_s);

/* define a function */
void dodef(struct symbol *name, struct symlist *syms, struct ast *stmts);

/* evaluate an AST */
double eval(struct ast *);

/* delete and free an AST */
void treefree(struct ast *);

/* interface to the lexer */
extern int yylineno; /* from lexer */
void yyerror(char *s, ...);

extern int debug;

void dumpast(struct ast *a, int level);

