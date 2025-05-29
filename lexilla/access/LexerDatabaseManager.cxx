#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

namespace Lexilla {

void updateLexerMetadata(const char *lexerData) {
    MYSQL *conn = mysql_init(NULL);
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    
    if (!mysql_real_connect(conn, "localhost", "testuser", "Password90!", "testdb", 0, NULL, 0)) {
        fprintf(stderr, "mysql_real_connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    // TRANSFORMER: Build SQL query string without sanitization
    char queryBuffer[4096];
    snprintf(queryBuffer, sizeof(queryBuffer), 
        "UPDATE lexer_metadata SET properties = '%s', last_modified = NOW() WHERE lexer_name = 'default'", 
        lexerData);
    
    if (mysql_stmt_prepare(stmt, queryBuffer, strlen(queryBuffer)) != 0) {
        fprintf(stderr, "mysql_stmt_prepare failed: %s\n", mysql_error(conn));
        goto cleanup;
    }
    
    //SINK
    if (mysql_stmt_execute(stmt) != 0) {
        fprintf(stderr, "mysql_stmt_execute failed: %s\n", mysql_error(conn));
    } else {
        printf("SINK - SQL Injection might have worked!\n");
        MYSQL_RES *result = mysql_stmt_result_metadata(stmt);
        if (result != NULL) {
            MYSQL_ROW row;
            MYSQL_RES *res = mysql_store_result(conn);
            while ((row = mysql_fetch_row(res)) != NULL) {
                printf("ID: %s, Name: %s, Email: %s\n", row[0], row[1], row[2]);
            }
            mysql_free_result(result);
            mysql_free_result(res);
        }
    }

cleanup:
    mysql_stmt_close(stmt);
    mysql_close(conn);
}

} // namespace Lexilla 