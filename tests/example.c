#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

void *_json_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void _json_free(void *ptr, __attribute__((unused)) size_t size)
{
    free(ptr);
}

int main(void)
{
    char const *json = "{\"foo\": \"bar\", \"baz\": 42, \"qux\": [1, 2, 3]}";
    json_reader_t r = json_init(json, strlen(json));
    json_t obj = json_parse(&r);

    if (obj.type == JSON_ERROR)
    {
        printf("Expected object, got %lu\n", obj.number);
        return 1;
    }

    json_t foo = json_get(obj, "foo");
    json_t baz = json_get(obj, "baz");
    json_t qux = json_get(obj, "qux");

    printf("foo = %s\n", foo.string);
    printf("baz = %lu\n", baz.number);

    for (size_t i = 0; i < qux.array.len; i++)
    {
        printf("qux[%lu] = %lu\n", i, qux.array.buf[i].number);
    }

    json_free(&obj);
}
