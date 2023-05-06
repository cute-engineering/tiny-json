#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct json_t json_t;

/* --- Allocation ------------------------------------------------------ */

void *_json_realloc(void *ptr, size_t size);
void _json_free(void *ptr, size_t size);

/* --- Reader ---------------------------------------------------------- */

typedef struct
{
    char const *buf;
    size_t len;
    size_t pos;
} json_reader_t;

json_reader_t json_init(char const *buf, size_t len);

#define json_arr_foreach(o, r)                                     \
    if ((o)->array.len > 0)                                        \
        for (size_t i = 0;                                         \
             i < (o)->array.len && (((r) = (o)->array.buf[i]), 1); \
             ++i)

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
    JSON_KEY_NOT_FOUND,
    JSON_MISSING_COLON,
    JSON_MISSING_QUOTE,
    JSON_MISSING_RBRACE,
    JSON_MISSING_LBRACE,
    JSON_MISSING_RBRACKET,
    JSON_MISSING_LBRACKET,
    JSON_NOT_A_KEY,
    JSON_NOT_AN_OBJECT,

    JSON_UNIMPLEMENTED
} json_error_t;

typedef enum
{
    JSON_ARRAY,
    JSON_BOOL,
    JSON_ERROR,
    JSON_KEY,
    JSON_NULL,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_STRING,
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
void json_free(json_t *self);