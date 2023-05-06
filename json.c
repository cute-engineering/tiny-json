#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>

#include "json.h"

/* --- Utils ----------------------------------------------------------- */

static uint64_t str_to_number(char const *str, size_t len)
{
    uint64_t result = 0;

    for (size_t i = 0; i < len; i++)
    {
        result = result * 10 + (str[i] - '0');
    }

    return result;
}

json_t json_get(json_t obj, char const *key)
{
    if (obj.type != JSON_OBJECT)
    {
        return json_raise(JSON_NOT_AN_OBJECT);
    }

    for (size_t i = 0; i < obj.array.len; i += 2)
    {
        json_t k = obj.array.buf[i];
        json_t v = obj.array.buf[i + 1];

        if (k.type != JSON_KEY)
        {
            return json_raise(JSON_NOT_A_KEY);
        }

        if (strncmp(k.string, key, strlen(key)) == 0)
        {
            return v;
        }
    }

    return json_raise(JSON_KEY_NOT_FOUND);
}

void json_free(json_t *self)
{
    if (self == NULL)
    {
        return;
    }

    switch (self->type)
    {
    case JSON_OBJECT:
    case JSON_ARRAY:
    {
        for (size_t i = 0; i < self->array.len; i++)
        {
            json_free(&self->array.buf[i]);
        }

        _json_free(self->array.buf, self->array.cap * sizeof(json_t));

        self->array.buf = NULL;
        self->array.len = 0;
        self->array.cap = 0;
        break;
    }

    case JSON_BOOL:
    case JSON_ERROR:
    case JSON_NULL:
    case JSON_NUMBER:
    {
        break;
    }

    case JSON_STRING:
    case JSON_KEY:
    {
        _json_free((void *)self->string, strlen(self->string) + 1);
    }
    }
}

/* --- Reader ---------------------------------------------------------- */

static bool reader_eof(json_reader_t *r)
{
    return r->pos >= r->len;
}

static char reader_peek(json_reader_t *r)
{
    if (r->pos >= r->len)
    {
        return '\0';
    }

    return r->buf[r->pos];
}

static void reader_skip_ws(json_reader_t *r)
{
    while (!reader_eof(r) && isspace(reader_peek(r)))
    {
        r->pos++;
    }
}

static bool reader_skip(json_reader_t *r, char c)
{
    if (reader_peek(r) == c)
    {
        r->pos++;
        return true;
    }

    return false;
}

static bool reader_skip_chars(json_reader_t *r, char const *chars)
{
    size_t delta = r->len - r->pos;

    if (delta < strlen(chars))
    {
        return false;
    }

    if (strncmp(r->buf + r->pos, chars, strlen(chars)) == 0)
    {
        r->pos += strlen(chars);
        return true;
    }

    return false;
}

json_reader_t json_init(char const *buf, size_t len)
{
    return (json_reader_t){.buf = buf, .len = len, .pos = 0};
}

/* --- Parser ---------------------------------------------------------- */

static json_t json_str(char const *str, size_t len)
{
    char *result = _json_realloc(NULL, len + 1);
    memcpy(result, str, len);
    result[len] = '\0';

    return (json_t){.type = JSON_STRING, .string = result};
}

static json_t json_parse_str(json_reader_t *r)
{
    if (!reader_skip(r, '"'))
    {
        return json_raise(JSON_MISSING_QUOTE);
    }

    size_t start = r->pos;

    while (!reader_eof(r) && reader_peek(r) != '"')
    {
        r->pos++;
    }

    if (reader_eof(r))
    {
        return json_raise(JSON_MISSING_QUOTE);
    }

    size_t len = r->pos - start;

    r->pos++;

    return json_str(r->buf + start, len);
}

static json_t json_parse_object(json_reader_t *r)
{
    json_vec_t obj = {0};
    reader_skip(r, '{');

    while (!reader_eof(r))
    {
        reader_skip_ws(r);

        if (reader_peek(r) == '}')
        {
            break;
        }

        json_t key = json_parse_str(r);
        key.type = JSON_KEY;

        json_ret_if_error(key);
        reader_skip_ws(r);

        if (!reader_skip(r, ':'))
        {
            return json_raise(JSON_MISSING_COLON);
        }

        json_t value = json_parse(r);

        vec_append(&obj, key);
        vec_append(&obj, value);

        reader_skip_ws(r);

        if (!reader_skip(r, ','))
        {
            break;
        }
    }

    if (!reader_skip(r, '}'))
    {
        return json_raise(JSON_MISSING_LBRACE);
    }

    return (json_t){.type = JSON_OBJECT, .array = obj};
}

static json_t json_parse_number(json_reader_t *r)
{
    size_t start = r->pos;

    while (!reader_eof(r) && isdigit(reader_peek(r)))
    {
        r->pos++;
    }

    size_t len = r->pos - start;

    return (json_t){.type = JSON_NUMBER, .number = str_to_number(r->buf + start, len)};
}

json_t json_parse_array(json_reader_t *r)
{
    json_vec_t arr = {0};

    reader_skip(r, '[');

    while (!reader_eof(r))
    {
        reader_skip_ws(r);

        if (reader_peek(r) == ']')
        {
            break;
        }

        json_t value = json_parse(r);
        vec_append(&arr, value);

        reader_skip_ws(r);
        if (!reader_skip(r, ','))
        {
            break;
        }
    }

    if (!reader_skip(r, ']'))
    {
        return json_raise(JSON_MISSING_LBRACKET);
    }

    return (json_t){.type = JSON_ARRAY, .array = arr};
}

json_t json_parse(json_reader_t *r)
{
    reader_skip_ws(r);

    if (reader_peek(r) == '{')
    {
        return json_parse_object(r);
    }
    else if (reader_peek(r) == '"')
    {
        return json_parse_str(r);
    }
    else if (reader_peek(r) == '[')
    {
        return json_parse_array(r);
    }
    else if (reader_skip_chars(r, "true"))
    {
        return (json_t){.type = JSON_BOOL, .number = 1};
    }
    else if (reader_skip_chars(r, "false"))
    {
        return (json_t){.type = JSON_BOOL, .number = 0};
    }
    else if (reader_skip_chars(r, "null"))
    {
        return (json_t){.type = JSON_NULL};
    }
    else if (isdigit(reader_peek(r)))
    {
        return json_parse_number(r);
    }

    return json_raise(JSON_UNIMPLEMENTED);
}
/* --- Vec ------------------------------------------------------------- */

void vec_append(json_vec_t *self, json_t obj)
{
    if (self->len == self->cap)
    {
        size_t n = (self->cap == 0) ? 1 : self->cap << 1;
        self->buf = _json_realloc(self->buf, n * sizeof(json_t));
        self->cap = n;
    }

    self->buf[self->len++] = obj;
}