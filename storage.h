#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

/*
    DATA TYPES
*/

struct db;
typedef struct db *db_t;

struct sqlite3_stmt;
typedef struct sqlite3_stmt *rdf_it_t;

/*
    CONSTANTS
*/

extern const char * const uri_type;

/*
    FUNCTION DECLARATIONS
*/

db_t rdf_db_open(const char *filepath);

void rdf_db_close(db_t db);

int rdf_db_initialize(db_t db);

char *rdf_anon_uri( db_t db );

int rdf_insert( db_t db,
                const char *subj_uri,
                const char *pred_uri,
                const char *obj_lexical,
                const char *obj_type,
                const char *obj_lang );

int rdf_drop( db_t db,
                 const char *subj_uri,
                 const char *pred_uri,
                 const char *obj_lexical,
                 const char *obj_type,
                 const char *obj_lang );

int rdf_exists( db_t db,
                const char *subj_uri,
                const char *pred_uri,
                const char *obj_lexical,
                const char *obj_type,
                const char *obj_lang );

rdf_it_t rdf_find( db_t db,
                   const char *subj_uri,
                   const char *pred_uri,
                   const char *obj_lexical,
                   const char *obj_type,
                   const char *obj_lang );

int rdf_next( rdf_it_t it,
              const char **subj_uri,
              const char **pred_uri,
              const char **obj_lexical,
              const char **obj_type,
              const char **obj_lang );

void rdf_cancel(rdf_it_t it);

#endif /* ndef STORAGE_H_INCLUDED */
