#include "CharacterQueryProcessor.h"
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <cstdio>

namespace Lexilla {

void CharacterQueryProcessor::processCharacterUpdate(const char* characterData) {
    mongoc_client_t* client;
    mongoc_collection_t* collection;
    bson_error_t error;
    
    mongoc_init();
    client = mongoc_client_new("mongodb://localhost:27017");
    collection = mongoc_client_get_collection(client, "character_db", "unicode_categories");
    
    // Transform the character data into a MongoDB update document
    char updateBuffer[1024];
    snprintf(updateBuffer, sizeof(updateBuffer), 
             "{\"$set\": {\"category\": \"%s\", \"last_updated\": new Date()}}", 
             characterData);
    
    bson_t* query;
    bson_t* update;
    query = bson_new_from_json((const uint8_t*)"{\"type\": \"character\"}", -1, &error);
    update = bson_new_from_json((const uint8_t*)updateBuffer, -1, &error);
    
    //SINK
    if (!mongoc_collection_update_one(collection, query, update, NULL, NULL, &error)) {
        fprintf(stderr, "Character update failed: %s\n", error.message);
    } else {
        printf("Character category updated successfully!\n");
    }
    
    bson_destroy(query);
    bson_destroy(update);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}

} // namespace Lexilla 