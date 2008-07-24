#include "serql.h"

#define SERQL_PARSE_ERROR 1
#define SERQL_NO_MANDATORY_PATH 2

int annotate(struct graph_expr *graph_expr)
{
    struct graph_expr *ge;
    int r = 0;

    if(graph_expr->mandatory == NULL)
        return SERQL_NO_MANDATORY_PATH;

    for(ge = graph_expr->optional; ge; ge = ge->next)
        if((r = annotate(ge)) != 0)
            break;

    return r;
}

char *create_graph_expression(struct graph_expr *graph_expr)
{
    struct path_expr *pe;
    struct graph_expr *ge;

    /* Output mandatory path expressions */
    for(pe = graph_expr->mandatory; pe; pe->next)
    {
        /* TODO */
    }

    /* Output optional path expressions */
    for(ge = graph_expr->optional; ge; ge = ge->next)
        create_graph_expression(graph_expr);

    return "";  /* TEMP */
}

char *create_rdf_query(pool_t pool, struct query *query)
{
    struct table_query *tq;

    for(tq = query->queries; tq; tq = tq->next)
    {
        /* table query */
        if(tq->next)
        { 
            create_graph_expression(tq->from);

            /* output other members */
        }
    }
    /* output namespaces */

    return "";  /* TEMP */
}

int main()
{
    struct query *query;
    struct pool pool = { NULL };
    char *error;

    query = parse_serql(stdin, &pool, &error);
    if(query)
    {
        fprintf(stdout, "Parsed OK!\n");
        create_rdf_query(&pool, query);
    }
    else
    {
        fprintf(stdout, "Parse error: %s!\n", error);
    }

    pclear(&pool);

    return 0;
}
