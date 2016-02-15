/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

#define CLASS 258
#define ELSE 259
#define FI 260
#define IF 261
#define IN 262
#define INHERITS 263
#define LET 264
#define LOOP 265
#define POOL 266
#define THEN 267
#define WHILE 268
#define CASE 269
#define ESAC 270
#define OF 271
#define DARROW 272
#define NEW 273
#define ISVOID 274
#define STR_CONST 275
#define INT_CONST 276
#define BOOL_CONST 277
#define TYPEID 278
#define OBJECTID 279
#define ASSIGN 280
#define NOT 281
#define LE 282
#define ERROR 283
#define LET_STMT 285

/*
 *  Add Your own definitions here
 */

%}

%s STRING
%s COMMENT1
%s COMMENT2
/*
 * Define names for regular expressions here.
 */

%%

 /*
  *  Nested comments
  */
 /*strings*/
<INITIAL>["] { BEGIN(STRING); string_buf_ptr = string_buf; }
<STRING>\\b { *string_buf_ptr = '\b'; string_buf_ptr++; }
<STRING>\\t { *string_buf_ptr = '\t'; string_buf_ptr++; }
<STRING>\\f { *string_buf_ptr = '\f'; string_buf_ptr++; }
<STRING>\\n { *string_buf_ptr = '\n'; string_buf_ptr++; }
<STRING>\\\n { *string_buf_ptr = '\n'; string_buf_ptr++; }
<STRING>\\
<STRING>[^"\\\0\n]* { unsigned int i; for (i = 0; i < strlen(yytext); i++) { *string_buf_ptr = yytext[i]; string_buf_ptr++;} }
<STRING>["] { *string_buf_ptr='\0'; yylval.symbol = stringtable.add_string(string_buf); BEGIN(INITIAL); return (STR_CONST); }
<STRING><<EOF>> /*report error!*/
<STRING>\n /*report error!*/
<STRING>\0 /*report error!*/

 /*comments*/
<INITIAL>"--" { BEGIN(COMMENT1); }
<COMMENT1>\n { curr_lineno++; BEGIN(INITIAL); }
<COMMENT1>.* 

 /*Operaters*/
"+" { return '+'; }
"-" { return '-'; }
"*" { return '*'; }
"/" { return '/'; }
"~" { return '~'; }
"<" { return '<'; }
"=" { return '='; }
"(" { return '('; }
")" { return ')'; }
"{" { return '{'; }
"}" { return '}'; }
"," { return ','; }
":" { return ':'; }
";" { return ';'; }
"." { return '.'; }
"=>"  { return (DARROW); }
"<-"        { return (ASSIGN); }
"<="        { return (LE); }

 /*new line*/
\n { curr_lineno++; }

 /*space*/
[ \t]+ 

 /*Keywords. Case insensitive*/
(?i:class)  { return (CLASS); }
(?i:else) { return (ELSE); }
(?i:fi) { return (FI); }
(?i:if) { return (IF); }
(?i:in) { return (IN); }
(?i:inherits) { return (INHERITS); }
(?i:let) { return (LET); }
(?i:loop) { return (LOOP); }
(?i:pool) { return (POOL); }
(?i:then) { return (THEN); }
(?i:while)  { return (WHILE); } 
(?i:case) { return (CASE); }
(?i:esac) { return (ESAC); }
(?i:of) { return (OF); }
(?i:new)  { return (NEW); }
(?i:isvoid) { return (ISVOID); }
(?i:not) { return (NOT); }
T(?i:rue) { yylval.boolean = true; return (BOOL_CONST); }
F(?i:alse) { yylval.boolean = false; return (BOOL_CONST); }

 /*integers*/
[0-9]+  { yylval.symbol = inttable.add_string(yytext); return (INT_CONST); }

 /*IDs*/
[A-Z][0-9a-zA-Z_]*  { yylval.symbol = idtable.add_string(yytext); return (TYPEID); }
[a-z][0-9a-zA-Z_]*  { yylval.symbol = idtable.add_string(yytext); return (OBJECTID); }

 /*EOF*/
<<EOF>> { yyterminate(); }
 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */


%%