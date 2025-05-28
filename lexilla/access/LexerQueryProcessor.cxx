#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

namespace Lexilla {

void processLexerQuery(const char *queryData, size_t length) {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "testuser", "Password90!", "testdb", 0, NULL, 0)) {
        fprintf(stderr, "mysql_real_connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }
    
    // TRANSFORMER: Build search query without sanitization
    char searchQuery[4096];
    snprintf(searchQuery, sizeof(searchQuery),
        "SELECT * FROM lexer_config WHERE config_name LIKE '%%%s%%' OR description LIKE '%%%s%%'",
        queryData, queryData);
    
    //SINK
    if (mysql_real_query(conn, searchQuery, strlen(searchQuery)) != 0) { 
        fprintf(stderr, "mysql_real_query failed: %s\n", mysql_error(conn));
    } else {
        printf("SINK - SQL Injection might have worked!\n");
        MYSQL_RES *result = mysql_store_result(conn);
        if (result != NULL) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)) != NULL) {
                printf("ID: %s, Name: %s, Email: %s\n", row[0], row[1], row[2]);
            }
            mysql_free_result(result);
        }
    }
    
    mysql_close(conn);
}

} // namespace Lexilla 