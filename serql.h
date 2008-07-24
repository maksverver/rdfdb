#ifndef SERQL_H_INCLUDED
#define SERQL_H_INCLUDED

#include <stdio.h>
#include "pool.h"

struct namespace_decl {
    struct namespace_decl *next;

    char *prefix, *uri;
};

struct table_query {
    enum { setop_intersect, setop_union, setop_minus } setop;

    struct table_query *next;

    struct graph_expr *from;
    char              distinct;
    long long int     limit, offset;
};

struct query {
    struct table_query *queries;
    struct namespace_decl *namespace_decls;
};

struct value {
    enum { null, string, integer, real, uri, variable, UNIMPLEMENTED } type;

    union {
        long long       integer;
        double          real;
        struct {
            char        *lexical;
            char        *language;
            char        *datatype;
        };
        char            *uri;
        char            *identifier;
    };
};

struct identifier {
    struct identifier *next;

    char *identifier;
};

struct expression {
    enum { value, negation, conjunction, disjunction,
           equal, unequal, less, less_or_equal } type;

    struct value      value;
    struct expression *left, *right;
};

struct node_elem {
    struct node_elem *next;

    struct value value;
};

struct path_expr {
    struct path_expr *next;

    struct node_elem      *subj;
    struct value          pred;
    struct node_elem      *obj;
};

struct graph_expr {
    struct graph_expr *next;

    struct path_expr   *mandatory;
    struct graph_expr  *optional;
    struct expression  *where;

    struct identifier *identifiers;
};

struct query *parse_serql(FILE *fp, pool_t pool, char **error);

#endif /* ndef SERQL_H_INCLUDED */
