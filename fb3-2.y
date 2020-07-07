
%{
#  include <stdio.h>
#  include <stdlib.h>
#  include "fb3-2.h"
extern FILE* yyin;

%}

%union {
  struct ast *a;
  double d;
  struct symbol *s;		/* which symbol */
  struct symlist *sl;
  int fn;			/* which function */
}

/* declare tokens */
%token <d> NUMBER
%token <s> NAME
%token <fn> FUNC
%token EOL

// LB RB LP RP -> left brace, right brace, left Parenthesis, right Parenthesis
%token IF THEN ELSE WHILE DO DEFINE PRINT INT REAL


%nonassoc <fn> CMP
%right '='
%left '+' '-'
%left '*' '/'
%nonassoc '|' UMINUS

%type <a> exp stmt explist defineMacro
%type <sl> symlist

%start calclist

%%

defineMacro: DEFINE NAME NUMBER     { $$ = newmacro($2, $3);}

stmtlist: stmtlist stmt
	| stmt

stmt:
     INT symlist ';'                { $$ = test($2, integer); }
   | REAL symlist ';'                { $$ = test($2, real); }
   | INT NAME '[' NUMBER ']' ';'        { arrdeclare($2, $4); }
   | exp ';'                       { $$ = $1;}
;

exp: exp CMP exp          { $$ = newcmp($2, $1, $3); }
   | exp '+' exp          { $$ = newast('+', $1,$3); }
   | exp '-' exp          { $$ = newast('-', $1,$3);}
   | exp '*' exp          { $$ = newast('*', $1,$3); }
   | exp '/' exp          { $$ = newast('/', $1,$3); }
   | '|' exp              { $$ = newast('|', $2, NULL); }
   | '(' exp ')'          { $$ = $2; }
   | '-' exp %prec UMINUS { $$ = newast('M', $2, NULL); }
   | NAME '[' NUMBER ']'  { $$ = newarrref($1, $3); }
   | NUMBER               { $$ = newnum($1); }
   | NAME                 { $$ = newref($1); }
   | NAME '=' exp         { $$ = newasgn($1, $3); }
   | NAME '[' NUMBER ']' '=' exp  { $$ = newarrasgn($1, $3, $6); }
   | NAME '(' explist ')' { $$ = newcall($1, $3); }
;

explist: exp
 | exp ',' explist  { $$ = newast('L', $1, $3); }
;
symlist: NAME       { $$ = newsymlist($1, NULL); printf("%s \n", $1->name); }
 | NAME ',' symlist { $$ = newsymlist($1, $3);  printf("%s \n", $1->name);}
;
//   | 'int' FUNC '(' explist ')' '{' stmtlist '}' { $$ = newfunc($2, $4); }
calclist: /* nothing */
   INT NAME '('  ')' '{' stmtlist 'return' NUMBER ';' '}'
  |INT NAME '('  ')' '{' stmtlist '}' {
    	if (curparseState == DeclareState) {
    		printf("main function end, and in DeclareState\n");
    	}
    	// 做個結尾，把sp恢復
	printf("要生出恢復挖stack的code\n");
	char *temp = malloc(sizeof(char) * 80);
	sprintf(temp, "addi sp, sp, %d\n", cur_stk_size);
	strcat(totalCode, temp);
     	printf("========== end =======\n");
     	printf("%s\n",totalCode);
   }

  |defineMacro INT NAME '('  ')' '{' stmtlist '}'{
  	if (curparseState == DeclareState) {
  		printf("main function end, and in DeclareState\n");
  	}
   	printf("========== end =======\n");
   	printf("%s\n",totalCode);
   }
/*
  | calclist stmt ';' {
    if(debug) dumpast($2, 0);
     printf("= %4.4g\n> ", eval($2));
     printf("code %s\n", $2->code);
     printf("=====================\n");
     treefree($2);
    }
  | calclist DEFINE NAME '(' symlist ')' '=' list EOL {
                       dodef($3, $5, $8);
                       printf("Defined %s\n> ", $3->name); }
  | calclist PRINT '(' NAME ')' EOL {
     if ($4->valueType == integer) {
          printf("%s: %0.0f\n>", $4->name, $4->value);
     }
     else {
          printf("%s: %f\n>", $4->name, $4->value);
     }
  }
  | calclist error EOL { yyerrok; printf("> "); }
 ;
 */
%%

void parse(FILE* fileInput)
    {
        yyin= fileInput;
        while(feof(yyin)==0)
        {
        yyparse();
        }
    }
