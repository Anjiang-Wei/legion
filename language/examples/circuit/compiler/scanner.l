%{
//#define YYSTYPE char *
#include "y.tab.h"
int cur_line = 1;
void yyerror(const char *msg);
void unrecognized_char(char c);
void unterminate_string();
// #define _DUPTEXT {yylval = strdup(yytext);}
%}

%option nounput

/* note \042 is '"' */
/* * is IDENTIFIER */
WHITESPACE          ([ \t\r\a]+)
SINGLE_COMMENT1     ("//"[^\n]*)
SINGLE_COMMENT2     ("#"[^\n]*)
IDENTIFIER          ([_a-zA-Z][_a-zA-Z0-9]*)
OPERATOR            ([-+*/%=,;!?:<>(){}\.\[\]])
INTEGER             ([0-9]+)
UNTERM_STRING       (\042[^\042\n]*)
STRING              (\042[^\042\n]*\042)

%%

\n                  { cur_line++; }
{WHITESPACE}        { /* ignore every whitespace */ }
{SINGLE_COMMENT1}   { /* skip for single line comment */    }
{SINGLE_COMMENT2}   { /* skip for single line comment */    }

{OPERATOR}          { return yytext[0]; }
"Task"              { return T_Task; }
"Region"            { return T_Region; }
"Layout"            { return T_Layout; }
"InstanceLimit"     { return T_Instance; }
"CollectMemory"     { return T_Collect; }
"IndexTaskMap"      { return T_IndexTaskMap; }
"SingleTaskMap"     { return T_SingleTaskMap; }

"F_order"           { return T_Positive_Dimension; }
"C_order"           { return T_Reverse_Dimension; }
"AOS"               { return T_AOS; }
"SOA"               { return T_SOA; }
"Compact"           { return T_Compact; }
"Align"             { return T_Align; }
"Exact"             { return T_Exact; }

"size"              { return T_Size; }
"split"             { return T_Split; }
"merge"             { return T_Merge; }
"swap"              { return T_Swap; }
"slice"             { return T_Slice; }
"reverse"           { return T_Reverse; }
"balance_split"     { return T_Balance_split; }
"auto_split"        { return T_Auto_split; }
"greedy_split"      { return T_Greedy_split; }
"volume"            { return T_Volume; }
"has"               { return T_Has; }
"len"               { return T_Len; }
"ipoint"            { return T_TaskIPoint; }
"ispace"            { return T_TaskISpace; }
"parent"            { return T_TaskParent; }
"processor"         { return T_TaskProcessor; }

"CPU"               { return T_CPU; }
"GPU"               { return T_GPU; }
"IO"                { return T_IO; }
"PY"                { return T_PY; }
"PROC"              { return T_PROC; }
"OMP"               { return T_OMP; }

"SYSMEM"            { return T_SYSMEM; }
"FBMEM"             { return T_FBMEM; }
"RDMEM"             { return T_RDMEM; }
"ZCMEM"             { return T_ZCMEM; }
"SOCKMEM"           { return T_SOCKMEM; }


"Machine"           { yylval.string = strdup("Machine"); return T_Identifier; }

"int"               { return T_Int; }
"bool"              { return T_Bool; }
"IPoint"            { return T_IPoint; }
"ISpace"            { return T_ISpace; }
"MSpace"            { return T_MSpace; }

"tuple"             { return T_Tuple; }
"for"               { return T_For; }
"in"                { return T_In; }

"True"              { return T_True; }
"False"             { return T_False; }

"def"               { return T_Def; }
"return"            { return T_Return; }

"print"             { return T_Print; }

"<="                { return T_Le; }
">="                { return T_Ge; }
"=="                { return T_Eq; }
"!="                { return T_Ne; }
"&&"                { return T_And; }
"||"                { return T_Or; }

{INTEGER}           { yylval.intVal = atoi(yytext); return T_IntConstant; }
{STRING}            { yylval.string = strndup(yytext+1, strlen(yytext)-2); return T_StringConstant; }
{IDENTIFIER}        { yylval.string = strdup(yytext); return T_Identifier; }

{UNTERM_STRING}     { unterminate_string(); }
.                   { unrecognized_char(yytext[0]); }

%%

int yywrap(void) { 
    return 1;
}

void unrecognized_char(char c) {
    char buf[32] = "Unrecognized character: ?";
    buf[24] = c;
    yyerror(buf);
}

void unterminate_string() {
    yyerror("Unterminate string constant");
}

void yyerror(const char *msg) {
    fprintf(stderr, "Error at line %d:\n\t%s\n", cur_line, msg);
    exit(-1);
}
