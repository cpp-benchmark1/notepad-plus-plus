#include "CharacterSearchEngine.h"

#if defined(__has_include)
#  if __has_include(<mongoc/mongoc.h>)
#    include <mongoc/mongoc.h>
#    include <bson/bson.h>
#  else
#    include "mongoc_stub.h"
#  endif
#else
#  include "mongoc_stub.h"
#endif
#include <cstdio>
#include <string>

namespace Lexilla {

void CharacterSearchEngine::searchCharacterCategories(const char* searchCriteria) {
    mongoc_client_t* client;
    mongoc_collection_t* collection;
    bson_error_t error;
    
    mongoc_init();
    client = mongoc_client_new("mongodb://localhost:27017");
    collection = mongoc_client_get_collection(client, "character_db", "unicode_categories");
    
    // First transformation: Convert search criteria to MongoDB query format
    std::string queryStr = "{\"$or\": [";
    queryStr += "{\"category\": {\"$regex\": \"" + std::string(searchCriteria) + "\", \"$options\": \"i\"}},";
    queryStr += "{\"description\": {\"$regex\": \"" + std::string(searchCriteria) + "\", \"$options\": \"i\"}}";
    queryStr += "]}";
    
    // Second transformation: Add sorting and limit to the query
    std::string finalQuery = queryStr + ", {\"sort\": {\"category\": 1}, \"limit\": 100}";
    
    bson_t* query;
    query = bson_new_from_json((const uint8_t*)finalQuery.c_str(), -1, &error);
    
    if (!query) {
        fprintf(stderr, "Error parsing search query: %s\n", error.message);
        return;
    }

    // Print the query being executed
    char* query_str = bson_as_canonical_extended_json(query, NULL);
    printf("Executing character search with query:\n");
    printf("Query: %s\n", query_str);
    bson_free(query_str);
    
    //SINK
    mongoc_cursor_t* cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
    
    // Process matching documents
    const bson_t* doc;
    int count = 0;
    while (mongoc_cursor_next(cursor, &doc)) {
        char* doc_str = bson_as_canonical_extended_json(doc, NULL);
        printf("Found character %d: %s\n", ++count, doc_str);
        bson_free(doc_str);
    }

    if (count == 0) {
        printf("No characters found matching the criteria.\n");
    } else {
        printf("Total characters found: %d\n", count);
    }

    if (mongoc_cursor_error(cursor, &error)) {
        fprintf(stderr, "Search error: %s\n", error.message);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}

} // namespace Lexilla 