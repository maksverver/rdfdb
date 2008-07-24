#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int export_to_ntriples(FILE *fp, rdf_it_t it)
{
    int result;
    const char *subj_uri, *pred_uri, *obj_lexical, *obj_type, *obj_lang;

    while((result = rdf_next(it, &subj_uri, &pred_uri,
                                 &obj_lexical, &obj_type, &obj_lang)) > 0)
    {
        fprintf(fp, "<%s> <%s> ", subj_uri, pred_uri);
        if(obj_type == NULL)
            fprintf(fp, "<%s>", obj_lexical);
        else
        {
            const char *p;

            fputc('"', fp);
            for(p = obj_lexical; *p; ++p)
            {
                if(*p == '\\')
                    fputs("\\\\", fp);
                else
                if(*p == '"')
                    fputs("\\\"", fp);
                else
                if(*p == 0x0A)
                    fputs("\\n", fp);
                else
                if(*p == 0x0D)
                    fputs("\\r", fp);
                else
                if(*p == 0x09)
                    fputs("\\t", fp);
                else
                    fputc(*p, fp);
            }
            fputc('"', fp);
        }
        fprintf(fp, "\n");
    }

    return result;
}

int main()
{
    db_t db = rdf_db_open("test.dat");
    if(db)
    {
        printf("%d\n", rdf_insert(db, "foo", "bar", "baz", "", ""));
        printf("%d\n", rdf_insert(db, "foo", "bar", "123", "xsd:integer", ""));
        printf("%d\n", rdf_insert(db, "foo", "bar", "hallo", "", "nl"));
        printf("%d\n", rdf_insert(db, "foo", "bar", "w\raa\nbar", "", "nl"));

        export_to_ntriples(stdout, rdf_find(db, NULL, NULL, NULL, NULL, NULL));

        rdf_db_close(db);
    }

    return 0;
}
