%{
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "serql.h"
#include "pool.h"


/* Global variables */
static pool_t pool;
static char *parse_error;
static struct query *query;

/* Defined in lexfile */
extern char *yytext;
extern int line, col, chars;

#define PALLOC(p,type) (type*)palloc(p,sizeof(type))


void yyerror(const char *string)
{
    int tokenlen = strlen(yytext);

    col   -= tokenlen;
    chars += col;

    if(*yytext)
    {
        parse_error = pprintf(pool,
            "unexpected token '%s' (character %d on line %d, column %d)",
            yytext, chars + 1, line + 1, col + 1 );
    }
    else
    {
        parse_error = pprintf(pool, 
            "unexpected end of input at character %d on line %d, column %d",
            chars + 1, line + 1, col + 1 );
    }
}

int yywrap()
{
    return 1;
}

void assign_subject(struct node_elem *subj, struct graph_expr *expr)
{
    struct path_expr *pe;
    struct graph_expr *ge;

    /* TEMP */
    if(subj == NULL)
        fprintf(stderr, "WARNING: assigning NULL subject!\n");

    for(pe = expr->mandatory; pe; pe = pe->next)
        if(pe->subj == NULL)
            pe->subj = subj;

    for(ge = expr->optional; ge; ge = ge->next)
        for(pe = ge->mandatory; pe; pe = pe->next)
            if(pe->subj == NULL)
                pe->subj = subj;
}

struct graph_expr *merge_paths(struct graph_expr *f, struct graph_expr *g)
{
    struct path_expr **pe;
    struct graph_expr **ge;

    for(pe = &(g->mandatory); *pe; pe = &((*pe)->next)) { };
    for(ge = &(g->optional); *ge; ge = &((*ge)->next)) { };
    *pe = f->mandatory;
    *ge = f->optional;
    f->mandatory = g->mandatory;
    f->optional  = g->optional;

    pfree(pool, g); /* .. optional .. */

    return f;
}



struct query *parse_serql(FILE *fp, pool_t p, char **error)
{
    pool = p;
    parse_error = NULL;
    line = col = chars =0;
    yyrestart(fp);

    if(yyparse() != 0)
    {
        if(error != NULL)
            *error = parse_error;

        return NULL;
    }

    return query;
}

%}

%union {
    char                  *string;
    double                real;
    long long             integer;
    struct value          value;

    struct expression     *expression;
    struct query          *query;
    struct table_query    *table_query;
    struct namespace_decl *namespace_decl;
    struct node_elem      *node_elem;
    struct path_expr      *path_expr;
    struct graph_expr     *graph_expr;
}

%token <string>  FULL_URI QNAME BNODE STRING IDENTIFIER LANGUAGE_TAG
%token <real>    REAL
%token <integer> INTEGER

%token OP_DATATYPE OP_EQ OP_NEQ OP_LT OP_LTEQ OP_GT OP_GTEQ

%token KW_SELECT KW_CONSTRUCT KW_FROM KW_WHERE KW_USING
%token KW_NAMESPACE KW_LOCALNAME KW_AS
%token KW_TRUE KW_FALSE KW_NOT KW_AND KW_OR KW_LIKE KW_LABEL KW_LANG
%token KW_DATATYPE KW_NULL KW_ISRESOURCE KW_ISLITERAL KW_ISBNODE KW_ISURI
%token KW_ANY KW_ALL KW_SORT KW_IN
%token KW_UNION KW_INTERSECT KW_MINUS KW_EXISTS KW_FORALL KW_DISTINCT
%token KW_LIMIT KW_OFFSET


%type <namespace_decl>  NamespaceDecl NamespaceList OptionalNamespaceList
%type <table_query>     TableQuerySet TableQuery SelectQuery
%type <value>           Var Value VarOrValue Literal Edge
%type <expression>      BooleanExpr BooleanElem AndExpr WhereClause OptionalWhereClause
%type <query>           Query
%type <integer>         SignedInteger SetOperator
%type <integer>         OptionalLimitClause OptionalOffsetClause OptionalDistinct
%type <integer>         CompOp
%type <real>            SignedReal
%type <string>          Uri
%type <node_elem>       Node NodeElemList NodeElem
%type <graph_expr>      PathExpr PathExprTail PathExprPath PathExprList GraphPattern OptionalFromClause


%start Query

%%

Uri:                    FULL_URI    { $$ = pstrdup(pool, $1); }
                        | QNAME     { $$ = pstrdup(pool, $1); }
                        | BNODE     { $$ = pstrdup(pool, $1); };

SignedInteger:          INTEGER         { $$ =  $1; }
                        | '+' INTEGER   { $$ = +$2; }
                        | '-' INTEGER   { $$ = -$2; };

SignedReal:             REAL            { $$ =  $1; }
                        | '+' REAL      { $$ = +$2; }
                        | '-' REAL      { $$ = -$2; };

Literal:                STRING {
                            $$.type        = string;
                            $$.lexical     = pstrdup(pool, $1);
                            $$.language    = NULL;
                            $$.datatype    = NULL;
                        }
                        | STRING OP_DATATYPE Uri {
                            $$.type        = string;
                            $$.lexical     = pstrdup(pool, $1);
                            $$.language    = NULL;
                            $$.datatype    = pstrdup(pool, $3);
                        }
                        | STRING LANGUAGE_TAG {
                            $$.type        = string;
                            $$.lexical     = pstrdup(pool, $1);
                            $$.language    = pstrdup(pool, $2);
                            $$.datatype    = NULL;
                        }
                        | SignedInteger {
                            $$.type        = integer;
                            $$.integer     = $1;
                        }
                        | SignedReal {
                            $$.type        = real;
                            $$.real        = $1;
                        };

Var:                    IDENTIFIER {
                            $$.type        = variable;
                            $$.identifier  = pstrdup(pool, $1);
                        };

Value:                  KW_NULL {
                            $$.type        = null;
                        }
                        | Uri {
                            $$.type        = uri;
                            $$.uri         = $1;
                        }
                        | Literal
                        | KW_DATATYPE  '(' Var ')'  { $$.type = UNIMPLEMENTED; }
                        | KW_LANG      '(' Var ')'  { $$.type = UNIMPLEMENTED; }
                        | KW_LABEL     '(' Var ')'  { $$.type = UNIMPLEMENTED; }
                        | KW_NAMESPACE '(' Var ')'  { $$.type = UNIMPLEMENTED; }
                        | KW_LOCALNAME '(' Var ')'  { $$.type = UNIMPLEMENTED; };

VarOrValue:             Var | Value;

CompOp:                 OP_EQ           { $$ =  0; }
                        | OP_NEQ        { $$ =  1; }
                        | OP_LT         { $$ =  2; }
                        | OP_LTEQ       { $$ =  3; }
                        | OP_GT         { $$ = -3; }
                        | OP_GTEQ       { $$ = -2; };

AnyOrAll:               KW_ANY
                        | KW_ALL;

BooleanElem:            '(' BooleanExpr ')' { $$ = $2; }
                        | KW_TRUE {
                            $$ = PALLOC(pool, struct expression);
                            $$->type  = value;
                            $$->value.type    = integer;
                            $$->value.integer = 1;
                        }
                        | KW_FALSE {
                            $$ = PALLOC(pool, struct expression);
                            $$->type  = value;
                            $$->value.type    = integer;
                            $$->value.integer = 0;
                        }
                        | KW_NOT BooleanElem {
                            $$ = PALLOC(pool, struct expression);
                            $$->type  = negation;
                            $$->left  = $2;
                            $$->right = NULL;
                        }
                        | VarOrValue CompOp VarOrValue {
                            $$ = PALLOC(pool, struct expression);
                            $$->left  = PALLOC(pool, struct expression);
                            $$->right = PALLOC(pool, struct expression);
                            $$->left->type = value;
                            $$->right->type = value;
                            if($2 > 0)
                            {
                                $$->left->value  = $1;
                                $$->right->value = $3;
                                $$->type = ( ($2 < 2) ? (($2 == 0) ? equal : unequal)
                                                      : (($2 == 2) ? less : less_or_equal) );
                            }
                            else
                            {
                                $$->left->value  = $3;
                                $$->right->value = $1;
                                $$->type = (-$2 == 2) ? less : less_or_equal;
                            }
                        }
                        | VarOrValue CompOp AnyOrAll '(' TableQuerySet ')' { $$ = NULL; }
                        | VarOrValue KW_LIKE STRING { $$ = NULL; }
                        | VarOrValue KW_IN '(' TableQuerySet ')' { $$ = NULL; }
                        | KW_EXISTS '(' TableQuerySet ')' { $$ = NULL; }
                        | KW_ISRESOURCE '(' Var ')' { $$ = NULL; }
                        | KW_ISURI      '(' Var ')' { $$ = NULL; }
                        | KW_ISBNODE    '(' Var ')' { $$ = NULL; }
                        | KW_ISLITERAL  '(' Var ')' { $$ = NULL; };

AndExpr:                BooleanElem
                        | AndExpr KW_AND BooleanElem {
                            $$ = PALLOC(pool, struct expression);
                            $$->type  = conjunction;
                            $$->left  = $1;
                            $$->right = $3;
                        };

BooleanExpr:            AndExpr
                        | BooleanExpr KW_OR AndExpr {
                            $$ = PALLOC(pool, struct expression);
                            $$->type  = disjunction;
                            $$->left  = $1;
                            $$->right = $3;
                        };

NodeElem:               Literal {
                            $$ = PALLOC(pool, struct node_elem);
                            $$->next  = NULL;
                            $$->value = $1;
                        }
                        | Var {
                            $$ = PALLOC(pool, struct node_elem);
                            $$->next  = NULL;
                            $$->value = $1;
                        }
                        | Uri {
                            $$ = PALLOC(pool, struct node_elem);
                            $$->value.type  = uri;
                            $$->value.uri   = $1;
                        };

NodeElemList:           NodeElem
                        | NodeElemList ',' NodeElem {
                            $$ = $3;
                            $3->next = $1;
                        };

Node:                   '{' '}' {
                            $$ = PALLOC(pool, struct node_elem);
                            $$->next = NULL;
                            $$->value.type       = variable;
                            $$->value.identifier = pprintf(pool, ".%d", chars);
                        }
                        | '{' NodeElemList '}' {
                            $$ = $2;
                        };

Edge:                   Var
                        | Uri {
                            $$.type        = uri;
                            $$.identifier  = $1;
                        };

WhereClause:            KW_WHERE BooleanExpr { $$ = $2; };

OptionalWhereClause:                    { $$ = NULL; }
                        | WhereClause   { $$ = $1;   };

PathExpr:               Node PathExprPath {
                            assign_subject($1, $2);
                            $$ = $2;
                        }
                        | '[' GraphPattern ']' {
                            $$ = PALLOC(pool, struct graph_expr);
                            $$->next      = NULL;
                            $$->mandatory = NULL;
                            $$->optional  = $2;
                            $$->where     = NULL;
                        };

PathExprPath:           PathExprTail
                        | PathExprPath ';' PathExprTail {
                            assign_subject($1->mandatory->subj, $3);
                            $$ = merge_paths($1, $3);
                        }
                        | PathExprPath PathExprTail {
                            assign_subject($1->mandatory->obj, $2);
                            $$ = merge_paths($1, $2);
                        };

PathExprTail:           Edge Node {
                            $$ = PALLOC(pool, struct graph_expr);
                            $$->next      = NULL;
                            $$->mandatory = PALLOC(pool, struct path_expr);
                            $$->optional  = NULL;
                            $$->where     = NULL;
                            $$->mandatory->next = NULL;
                            $$->mandatory->subj = NULL;
                            $$->mandatory->pred = $1;
                            $$->mandatory->obj  = $2;
                        }
                        | '[' PathExprPath OptionalWhereClause ']' {
                            $$ = PALLOC(pool, struct graph_expr);
                            $$->next      = NULL;
                            $$->mandatory = NULL;
                            $$->optional  = $2;
                            $$->where     = $3;
                        };

PathExprList:           PathExpr
                        | PathExprList ',' PathExpr {
                            $$ = merge_paths($1, $3);
                        };

GraphPattern:           PathExprList OptionalWhereClause {
                            $$ = $1;
                            $$->where = $2;
                        };

OptionalAsClause:       | KW_AS STRING;

ProjectionElem:         VarOrValue OptionalAsClause;

Projection:             '*'
                        | ProjectionElem
                        | Projection ',' ProjectionElem;

OptionalDistinct:                       { $$ = 0; }
                        | KW_DISTINCT   { $$ = 1; };

OptionalFromClause:                             { $$ = NULL; }
                        | KW_FROM GraphPattern  { $$ = $2; };

OptionalLimitClause:                        { $$ = -1; }
                        | KW_LIMIT INTEGER  { $$ = $2; };

OptionalOffsetClause:                       { $$ = -1; }
                        | KW_OFFSET INTEGER { $$ = $2; };

SelectQuery:            KW_SELECT OptionalDistinct Projection OptionalFromClause
                        OptionalLimitClause OptionalOffsetClause {
                            $$ = PALLOC(pool, struct table_query);
                            $$->next     = NULL;
                            $$->distinct = $2;
                            /* TODO projection */
                            $$->from     = $4;
                            $$->limit    = $5;
                            $$->offset   = $6;
                        };

SetOperator:            KW_UNION        { $$ = setop_union; }
                        | KW_INTERSECT  { $$ = setop_intersect; }

                        | KW_MINUS      { $$ = setop_minus; };

TableQuery:             SelectQuery { $$ = $1; }
                        | '(' TableQuerySet ')' { $$ = $2; };

TableQuerySet:          TableQuery
                        | TableQuerySet SetOperator TableQuery {
                            $$        = $1;
                            $$->setop = $2;
                            $$->next  = $3;
                        };

NamespaceDecl:          IDENTIFIER OP_EQ FULL_URI {
                            $$ = PALLOC(pool, struct namespace_decl);
                            $$->prefix = pstrdup(pool, $1);
                            $$->uri    = pstrdup(pool, $3);
                        };

NamespaceList:          NamespaceDecl
                        | NamespaceList ',' NamespaceDecl {
                            $$ = $3;
                            $$->next = $1;
                        };

OptionalNamespaceList:  { $$ = NULL; }
                        | KW_USING KW_NAMESPACE NamespaceList { $$ = $3; };

Query:                  TableQuerySet OptionalNamespaceList {
                            $$ = PALLOC(pool, struct query);
                            $$->queries         = $1;
                            $$->namespace_decls = $2;
                            query = $$;
                        }
/* EOF */
