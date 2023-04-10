#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct json_t json_t;

/* --- Allocation ------------------------------------------------------ */

void *json_realloc(void *ptr, size_t size);
void json_free(void *ptr, size_t size);

/* --- Reader ---------------------------------------------------------- */

typedef struct
{
    char const *buf;
    size_t len;
    size_t pos;
} json_reader_t;

json_reader_t json_init(char const *buf, size_t len);

/* --- Vec ------------------------------------------------------------- */

typedef struct
{
    json_t *buf;
    size_t len;
    size_t cap;
} json_vec_t;

void vec_append(json_vec_t *self, json_t obj);
void vec_free(json_vec_t *self);

/* --- Parser ---------------------------------------------------------- */

typedef struct json_obj_t json_obj_t;

typedef enum
{
    JSON_MISSING_RBRACE,
    JSON_MISSING_RBRACKET,
    JSON_MISSING_QUOTE,
    JSON_MISSING_COLON,
    JSON_NOT_AN_OBJECT,
    JSON_NOT_A_KEY,
    JSON_KEY_NOT_FOUND,

    JSON_UNIMPLEMENTED
} json_error_t;

typedef enum
{
    JSON_NULL,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_KEY,
    JSON_BOOL,
    JSON_ERROR
} json_type_t;

typedef struct json_t
{
    json_type_t type;
    union
    {
        json_vec_t array;
        char const *string;
        uint64_t number;
    };
} json_t;

#define json_raise(e) \
    (json_t) { .type = JSON_ERROR, .number = (e) }

#define json_ret_if_error(j)  \
    if (j.type == JSON_ERROR) \
    return j

json_t json_parse(json_reader_t *r);

/* --- Utils ------------------------------------------------------ */

json_t json_get(json_t obj, char const *key);