%{
#include "serql.h"
#include "serql.tab.h"
#include <string.h>
#define _GNU_SOURCE
#include <stdlib.h>

int line, col, chars;

%}

%%

\<([a-z][0-9a-z+.-]*:)[0-9a-z;/?:@&=+$._!~*'()%-]+(#[0-9a-z;/?:@&=+$\\.\\-_!~*'()%]*)?> {
                                            col += yyleng;
                                            yylval.string = yytext + 1;
                                            yylval.string[strlen(yylval.string) - 1] = '\0';
                                            return FULL_URI;
                                        }

@[a-z]{0,3}(-[a-z0-9]{1,8})*            {
                                            col += yyleng;
                                            yylval.string = yytext + 1;
                                            return LANGUAGE_TAG;
                                        }

[0-9]*\.[0-9]+                          {
                                            col += yyleng;
                                            yylval.real = atof(yytext);
                                            return REAL;
                                        }

[0-9]+                                  {
                                            col += yyleng;
                                            yylval.integer = atoll(yytext);
                                            return INTEGER;
                                        }

\"(\\["trn\\]|[^"\t\r\n\\])*\"          {
                                            col += yyleng;
                                            yylval.string = yytext;
                                            return STRING;
                                        }


\^\^                                    col += yyleng; return OP_DATATYPE;
=                                       col += yyleng; return OP_EQ;
!=                                      col += yyleng; return OP_NEQ;
\<                                      col += yyleng; return OP_LT;
\<=                                     col += yyleng; return OP_LTEQ;
>                                       col += yyleng; return OP_GT;
>=                                      col += yyleng; return OP_GTEQ;

SELECT                                  col += yyleng; return KW_SELECT;
CONSTRUCT                               col += yyleng; return KW_CONSTRUCT;
FROM                                    col += yyleng; return KW_FROM;
WHERE                                   col += yyleng; return KW_WHERE;
USING                                   col += yyleng; return KW_USING;
NAMESPACE                               col += yyleng; return KW_NAMESPACE;
LOCALNAME                               col += yyleng; return KW_LOCALNAME;
AS                                      col += yyleng; return KW_AS;
TRUE                                    col += yyleng; return KW_TRUE;
FALSE                                   col += yyleng; return KW_FALSE;
NOT                                     col += yyleng; return KW_NOT;
AND                                     col += yyleng; return KW_AND;
OR                                      col += yyleng; return KW_OR;
LIKE                                    col += yyleng; return KW_LIKE;
LABEL                                   col += yyleng; return KW_LABEL;
LANG                                    col += yyleng; return KW_LANG;
DATATYPE                                col += yyleng; return KW_DATATYPE;
NULL                                    col += yyleng; return KW_NULL;
ISRESOURCE                              col += yyleng; return KW_ISRESOURCE;
ISLITERAL                               col += yyleng; return KW_ISLITERAL;
ISBNODE                                 col += yyleng; return KW_ISBNODE;
ISURI                                   col += yyleng; return KW_ISURI;
ANY                                     col += yyleng; return KW_ANY;
ALL                                     col += yyleng; return KW_ALL;
SORT                                    col += yyleng; return KW_SORT;
IN                                      col += yyleng; return KW_IN;
UNION                                   col += yyleng; return KW_UNION;
INTERSECT                               col += yyleng; return KW_INTERSECT;
MINUS                                   col += yyleng; return KW_MINUS;
EXISTS                                  col += yyleng; return KW_EXISTS;
FORALL                                  col += yyleng; return KW_FORALL;
DISTINCT                                col += yyleng; return KW_DISTINCT;
LIMIT                                   col += yyleng; return KW_LIMIT;
OFFSET                                  col += yyleng; return KW_OFFSET;

(([a-z][a-z0-9._-]*)|(_[a-z0-9._-]+))   {
                                            col += yyleng;
                                            yylval.string = yytext;
                                            return IDENTIFIER;
                                        }

(([a-z][a-z0-9._-]*)|(_[a-z0-9._-]+)):[a-z0-9._-]+  {
                                            col += yyleng;
                                            yylval.string = yytext;                                            col += yyleng;
                                            yylval.string = yytext;

                                            return QNAME;
                                        }

_:[a-z0-9._-]+                          {
                                            col += yyleng;
                                            yylval.string = yytext;
                                            return BNODE;
                                        }

\r?\n                                   {
                                            chars   += col + yyleng;
                                            line    += 1;
                                            col      = 0;
                                        }

[\t ]+                                  col += yyleng;

.                                       ++col; return (int) yytext[0];
