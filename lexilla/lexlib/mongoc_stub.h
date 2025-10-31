#pragma once

#include <cstddef>
#include <cstdint>

// Lightweight stubs so the Lexilla sources compile even when the
// real libmongoc/libbson headers are not available (for example in CI).

struct _mongoc_client_t {};
struct _mongoc_collection_t {};
struct _mongoc_cursor_t {};

using mongoc_client_t = _mongoc_client_t;
using mongoc_collection_t = _mongoc_collection_t;
using mongoc_cursor_t = _mongoc_cursor_t;

struct bson_error_t {
    char message[256] = {0};
};

struct bson_t {};

constexpr int MONGOC_QUERY_NONE = 0;

inline void mongoc_init() {}
inline void mongoc_cleanup() {}

inline mongoc_client_t* mongoc_client_new(const char*) { return nullptr; }
inline mongoc_collection_t* mongoc_client_get_collection(mongoc_client_t*, const char*, const char*) { return nullptr; }

inline bson_t* bson_new_from_json(const uint8_t*, int, bson_error_t*) { return nullptr; }

inline bool mongoc_collection_update_one(mongoc_collection_t*, const bson_t*, const bson_t*, void*, void*, bson_error_t*) { return false; }

inline void bson_destroy(bson_t*) {}
inline void mongoc_collection_destroy(mongoc_collection_t*) {}
inline void mongoc_client_destroy(mongoc_client_t*) {}

inline char* bson_as_canonical_extended_json(const bson_t*, size_t*) { return nullptr; }
inline void bson_free(void*) {}

inline mongoc_cursor_t* mongoc_collection_find(mongoc_collection_t*, int, int, int, int, const bson_t*, const bson_t*, const bson_t*) { return nullptr; }
inline bool mongoc_cursor_next(mongoc_cursor_t*, const bson_t**) { return false; }
inline bool mongoc_cursor_error(mongoc_cursor_t*, bson_error_t*) { return false; }
inline void mongoc_cursor_destroy(mongoc_cursor_t*) {}

