#include "storage.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* For ANSI Linux */
char *strdup(const char *);


/*
 * SQL script for creating a new database.
 */
static const char * const creation_script =
    "CREATE TABLE Node (id INTEGER PRIMARY KEY, uri TEXT);"
    "CREATE UNIQUE INDEX Node_id ON Node(id);"
    "CREATE UNIQUE INDEX Node_uri ON Node(uri);"

    "CREATE TABLE Literal (id INTEGER PRIMARY KEY, data, type TEXT, language TEXT);"
    "CREATE UNIQUE INDEX Literal_id ON Literal(id);"
    "CREATE UNIQUE INDEX Literal_value ON Literal(type,data,language);"

    "CREATE TABLE Triple (id INTEGER PRIMARY KEY, subject INTEGER, predicate INTEGER, object INTEGER);"
    "CREATE UNIQUE INDEX Triple_id ON Triple(id);"
    "CREATE UNIQUE INDEX Triple_spo ON Triple(subject,predicate,object);"
    "CREATE INDEX Triple_po ON Triple(predicate,object);";


/*
 * SQL statements used.
 */

#define STATEMENTS 8

static const char * const statements[STATEMENTS] = {
#define SQL_FIND_NODE_BY_URI        ( 0)
    "SELECT id FROM Node WHERE uri=?1",

#define SQL_INSERT_NODE             ( 1)
    "INSERT INTO Node (id, uri) VALUES (?1, ?2)",

#define SQL_FIND_LITERAL_BY_VALUE   ( 2)
    "SELECT id FROM Literal WHERE data=?1 AND type=?2 AND language=?3",

#define SQL_INSERT_LITERAL          ( 3)
    "INSERT INTO Literal (id, data, type, language) VALUES (?1, ?2, ?3, ?4)",

#define SQL_NEXT_NODE_ID            ( 4)
    "SELECT IFNULL(MAX(m),0)+1 FROM ("
    "   SELECT MAX(id) AS m FROM Node"
    "   UNION"
    "   SELECT MAX(id) AS m FROM Literal )",

#define SQL_FIND_TRIPLE             ( 5)
    "SELECT id FROM Triple WHERE subject=?1 AND predicate=?2 AND object=?3",

#define SQL_INSERT_TRIPLE           ( 6)
    "INSERT INTO Triple (id, subject, predicate, object) VALUES (?1, ?2, ?3, ?4)",

#define SQL_DROP_TRIPLE             ( 7)
    "DELETE FROM Triple WHERE subject=?1 AND predicate=?2 AND object=?3"

};


/*
 * More type definitions
 */

typedef long long int nid_t;

struct db
{
    sqlite3 *db;
    sqlite3_stmt *stmts[STATEMENTS];
};


/*
 *  Helper functions
 */

static nid_t next_id(db_t db)
{
    nid_t id = 0;
    sqlite3_stmt *stmt = db->stmts[SQL_NEXT_NODE_ID];

    if(sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int64(stmt, 0);
    sqlite3_reset(stmt);

    return id;
}

static nid_t uri_to_id(db_t db, const char *uri)
{
    nid_t id = 0;
    sqlite3_stmt *stmt;

    /* Make sure all paramters are provided */
    if(!uri)
        return 0;

    /* Try to find existing node */
    stmt = db->stmts[SQL_FIND_NODE_BY_URI];
    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);
    if(sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int64(stmt, 0);
    sqlite3_reset(stmt);

    if(id == 0)
    {
        /* Insert new node */
        stmt = db->stmts[SQL_INSERT_NODE];
        sqlite3_bind_int64(stmt, 1, next_id(db));
        sqlite3_bind_text(stmt, 2, uri, -1, SQLITE_STATIC);
        if(sqlite3_step(stmt) == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(db->db);
        sqlite3_reset(stmt);
    }

    return id;
}

static nid_t lit_to_id(
    db_t db, const char *data, const char *type, const char *lang )
{
    nid_t id = 0;
    sqlite3_stmt *stmt;

    /* Make sure all parameters are provided. */
    if(!data || !type || !lang)
        return 0;

    /* Try to find existing literal */
    stmt = db->stmts[SQL_FIND_LITERAL_BY_VALUE];
    sqlite3_bind_text(stmt, 1, data, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, lang, -1, SQLITE_STATIC);
    if(sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int64(stmt, 0);
    sqlite3_reset(stmt);

    if(id == 0)
    {
        /* Not found; insert new literal */
        stmt = db->stmts[SQL_INSERT_LITERAL];
        sqlite3_bind_int64(stmt, 1, next_id(db));
        sqlite3_bind_text(stmt, 2, data, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, type, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, lang, -1, SQLITE_STATIC);
        if(sqlite3_step(stmt) == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(db->db);
        sqlite3_reset(stmt);
    }

    return id;
}

static nid_t tri_to_id(
    db_t db, nid_t subj_id, nid_t pred_id, nid_t obj_id )
{
    nid_t id = 0;
    sqlite3_stmt *stmt;

    /* Make sure all parameters are provided */
    if(!subj_id || !pred_id || !obj_id)
        return 0;

    /* Try to find extisting triple */
    stmt = db->stmts[SQL_FIND_TRIPLE];
    sqlite3_bind_int64(stmt, 1, subj_id);
    sqlite3_bind_int64(stmt, 2, pred_id);
    sqlite3_bind_int64(stmt, 3, obj_id);
    if(sqlite3_step(stmt) == SQLITE_ROW)
    {
        id = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_reset(stmt);

    if(id == 0)
    {
        /* Not found; insert new triple */
        stmt = db->stmts[SQL_INSERT_TRIPLE];
        sqlite3_bind_int64(stmt, 1, next_id(db));
        sqlite3_bind_int64(stmt, 2, subj_id);
        sqlite3_bind_int64(stmt, 3, pred_id);
        sqlite3_bind_int64(stmt, 4, obj_id);
        if(sqlite3_step(stmt) == SQLITE_DONE)
        {
            id = sqlite3_last_insert_rowid(db->db);
        }
        sqlite3_reset(stmt);
    }

    return id;
}


/*
 * API implementation
 */

db_t rdf_db_open(const char *filepath)
{
    int n;

    /* Allocate handle */
    db_t db = (db_t)malloc(sizeof(struct db));
    if(db == NULL)
        return NULL;
    memset(db, 0, sizeof(db_t));

    /* Open database */
    if(sqlite3_open(filepath, &db->db) != SQLITE_OK)
    {
        rdf_db_close(db);
        return NULL;
    }

    /* Create database structure */
    sqlite3_exec(db->db, creation_script, NULL, NULL, NULL);

    /* Prepare statements */
    for(n = 0; n < STATEMENTS; ++n)
    {
        if(sqlite3_prepare(db->db, statements[n], -1, &db->stmts[n], NULL) != SQLITE_OK)
        {
            fprintf( stderr, "rdfdb: INTERNAL ERROR -- "
                             "unable to prepare statement \"%s\"\n", statements[n] );
            rdf_db_close(db);
            return NULL;
        }
    }

    return db;
}

void rdf_db_close(db_t db)
{
    int n;

    /* Clean up prepared statements */
    for(n = 0; n < STATEMENTS; ++n)
        if(db->stmts[n] != NULL)
            sqlite3_finalize(db->stmts[n]);

    /* Close database */
    sqlite3_close(db->db);

    /* Deallocate handle */
    free(db);
}

char *rdf_anon_uri(db_t db)
{
    nid_t id;
    sqlite3_stmt *stmt;
    char uri[32];
    char *result = NULL;

    /* Allocate node identifier for anonymous resource */
    if((id = next_id(db)))
    {
        /* Create URI */
        sprintf(uri, "_:%lld", id);

        /* Insert new anonymous node */
        stmt = db->stmts[SQL_INSERT_NODE];
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, uri, -1, SQLITE_STATIC);
        if(sqlite3_step(stmt) == SQLITE_DONE)
            result = strdup(uri);
        sqlite3_reset(stmt);
    }

    return result;
}

int rdf_insert( db_t db,
                const char *subj_uri,
                const char *pred_uri,
                const char *obj_lexical,
                const char *obj_type,
                const char *obj_lang )
{
    nid_t subj_id, pred_id, obj_id;

    subj_id = uri_to_id(db, subj_uri);
    pred_id = uri_to_id(db, pred_uri);
    if(obj_type == NULL)
    {
        /* Object is a resource */
        obj_id = uri_to_id(db, obj_lexical);
    }
    else
    {
        /* Object is a literal */
        obj_id = lit_to_id(db, obj_lexical, obj_type, obj_lang);
    }

    if( subj_id && pred_id && obj_id &&
        tri_to_id(db, subj_id, pred_id, obj_id) )
    {
        return 0;
    }

    return -1;
}

int rdf_drop( db_t db,
                 const char *subj_uri,
                 const char *pred_uri,
                 const char *obj_lexical,
                 const char *obj_type,
                 const char *obj_lang )
{
    int result = -1;
    nid_t subj_id, pred_id, obj_id;

    if( (subj_id = uri_to_id(db, subj_uri)) &&
        (pred_id = uri_to_id(db, pred_uri)) &&
        (obj_id  = lit_to_id(db, obj_lexical, obj_type, obj_lang)) )
    {
        sqlite3_stmt *stmt = db->stmts[SQL_DROP_TRIPLE];
        sqlite3_bind_int64(stmt, 1, subj_id);
        sqlite3_bind_int64(stmt, 2, pred_id);
        sqlite3_bind_int64(stmt, 3, obj_id);
        if(sqlite3_step(stmt) == SQLITE_DONE)
            result = 0;
        sqlite3_reset(stmt);
    }

    return result;
}

int rdf_exists( db_t db,
                const char *subj_uri,
                const char *pred_uri,
                const char *obj_lexical,
                const char *obj_type,
                const char *obj_lang )
{
    rdf_it_t it;
    int result;

    it = rdf_find(db, subj_uri, pred_uri, obj_lexical, obj_type, obj_lang);
    if(!it)
        return -1;

    result = rdf_next(it, NULL, NULL, NULL, NULL, NULL);
    if(result > 0)
        rdf_cancel(it);
    return result;
}

rdf_it_t rdf_find( db_t db,
                   const char *subj_uri,
                   const char *pred_uri,
                   const char *obj_lexical,
                   const char *obj_type,
                   const char *obj_lang )
{
    sqlite3_stmt *stmt;
    nid_t subj_id = 0, pred_id = 0, obj_id = 0;

    if( (subj_uri && (subj_id = uri_to_id(db, subj_uri)) == 0) ||
        (pred_uri && (pred_id = uri_to_id(db, pred_uri)) == 0) ||
        (obj_lexical && (obj_id = lit_to_id(
            db, obj_lexical, obj_type, obj_lang)) == 0) )
    {
        /* Failed to allocate ids */
        return NULL;
    }

    char buffer[1024] =
        "SELECT SubjectNode.uri   AS subject_uri,"
        "       PredicateNode.uri AS predicate_uri,"
        "       COALESCE(ObjectNode.uri, Literal.data) AS object_lexical,"
        "       Literal.type      AS object_type,"
        "       Literal.language  AS object_language "
        "FROM Triple "
        "LEFT JOIN Node AS SubjectNode   ON SubjectNode.id   = subject "
        "LEFT JOIN Node AS PredicateNode ON PredicateNode.id = predicate "
        "LEFT JOIN Node AS ObjectNode    ON ObjectNode.id    = object "
        "LEFT JOIN Literal               ON Literal.id       = object ";

    if(subj_id || pred_id || obj_id)
    {
        strcat(buffer, "WHERE 1 ");
        if(subj_id)
            sprintf(buffer + strlen(buffer), "AND subject=%lld ", subj_id);
        if(pred_id)
            sprintf(buffer + strlen(buffer), "AND predicate=%lld ", pred_id);
        if(obj_id)
            sprintf(buffer + strlen(buffer), "AND subject=%lld ", obj_id);
    }

    if(sqlite3_prepare(db->db, buffer, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf( stderr, "rdfdb: INTERNAL ERROR -- "
                         "unable to prepare statement \"%s\"\n", buffer );
        return NULL;
    }

    return stmt;
}

int rdf_next( rdf_it_t it,
              const char **subj_uri,
              const char **pred_uri,
              const char **obj_lexical,
              const char **obj_type,
              const char **obj_lang )
{
    /* Attempt to get next row from statement. */
    int result = sqlite3_step(it);
    if(result == SQLITE_ROW)
    {
        /* New row available; extract data */
        if(subj_uri)
            *subj_uri    = sqlite3_column_text(it, 0);
        if(pred_uri)
            *pred_uri    = sqlite3_column_text(it, 1);
        if(obj_lexical)
            *obj_lexical = sqlite3_column_text(it, 2);
        if(obj_type)
            *obj_type    = sqlite3_column_text(it, 3);
        if(obj_lang)
            *obj_lang    = sqlite3_column_text(it, 4);

        return 1;
    }
    else
    {
        /* End of result set, or error occured. */
        sqlite3_finalize(it);

        return (result == SQLITE_DONE) ? 0 : -1;
    }
}

void rdf_cancel(rdf_it_t it)
{
    sqlite3_finalize(it);
}

void rdf_purge(db_t db)
{
    /* Delete unused nodes */
    sqlite3_exec(db->db,
        "DELETE FROM Node WHERE id NOT IN"
        "    ( SELECT subject   FROM triple UNION"
        "      SELECT predicate FROM triple UNION"
        "      SELECT object    FROM triple );",
        NULL, NULL, NULL );

    /* Delete unused literals */
    sqlite3_exec(db->db,
        "DELETE FROM Literal WHERE id NOT IN ( SELECT object FROM triple );",
        NULL, NULL, NULL );

    /* Vacuum database to reclaim freed up space. */
    sqlite3_exec(db->db, "VACUUM;", NULL, NULL, NULL);
}
