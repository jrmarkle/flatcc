#include <stdio.h>
#include <stdlib.h>

#include "mystruct_builder.h"
#include "mystruct_json_parser.h"
#include "mystruct_json_printer.h"
#include "mystruct_reader.h"

Vec3Fixed_t make()
{
    Vec3Fixed_t s;
    s.x[0] = 12;
    s.x[1] = 34;
    s.x[2] = 56;
    return s;
}

void check(Vec3Fixed_t s)
{
    if (s.x[0] != 12 || s.x[1] != 34 || s.x[2] != 56)
    {
        printf("struct did not match\n");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    {
        const Vec3Fixed_t s = make();
        check(s);
    }
    {
        void *bufferB;

        {
            Vec3Fixed_t s1 = make();

            flatcc_builder_t builder;
            flatcc_builder_init(&builder);
            Vec3Wrapper_create_as_root(&builder, &s1);

            size_t size = flatcc_builder_get_buffer_size(&builder);
            assert(size > 0);

            void *bufferA = flatcc_builder_finalize_buffer(&builder, &size);
            assert(NULL != bufferA);

            bufferB = malloc(size);
            memcpy(bufferB, bufferA, size);

            free(bufferA);
            flatcc_builder_clear(&builder);
        }

        {
            const Vec3Wrapper_table_t s2 = Vec3Wrapper_as_root(bufferB);
            const Vec3Fixed_struct_t s2i = Vec3Wrapper_inside_get(s2);
            check(*s2i);
        }

        free(bufferB);
    }

    {
        char *jsonFileBuffer;
        size_t jsonFileSize;
        FILE *jsonFile;

        {
            Vec3Fixed_t s1 = make();

            flatcc_builder_t builderA;
            flatcc_builder_init(&builderA);
            Vec3Wrapper_create_as_root(&builderA, &s1);

            size_t size = flatcc_builder_get_buffer_size(&builderA);
            assert(size > 0);

            void *buffer = flatcc_builder_finalize_buffer(&builderA, &size);
            assert(NULL != buffer);

            flatcc_builder_clear(&builderA);

            jsonFile = open_memstream(&jsonFileBuffer, &jsonFileSize);

            flatcc_json_printer_t printer;
            {
                const int err = flatcc_json_printer_init(&printer, jsonFile);
                assert(0 == err);
            }

            const int jsonSize = Vec3Wrapper_print_json_as_root(&printer, buffer, size, NULL);
            assert(jsonSize > 0);

            fflush(jsonFile);
            free(buffer);

            assert(jsonSize == jsonFileSize);
        }

        printf("size %zu\n", jsonFileSize);
        printf("%s\n", jsonFileBuffer);

        {
            flatcc_builder_t builderB;
            flatcc_builder_init(&builderB);

            flatcc_json_parser_t parser;
            const int err = mystruct_parse_json(&builderB, &parser, jsonFileBuffer, jsonFileSize, 0);
            if (0 != err)
            {
                printf("mystruct_parse_json returned error: %s\n", flatcc_json_parser_error_string(err));
                printf("parser.end:       %p\n", parser.end);
                printf("parser.error_loc: %p\n", parser.error_loc);
                printf("parser.line: %d\n", parser.line);
                printf("parser.pos: %d\n", parser.pos);
                exit(EXIT_FAILURE);
            }

            size_t size;
            void *buffer = flatcc_builder_finalize_buffer(&builderB, NULL);

            const Vec3Wrapper_table_t parsed = Vec3Wrapper_as_root(buffer);
            check(*Vec3Wrapper_inside_get(parsed));
        }

        fclose(jsonFile);
    }

    return EXIT_SUCCESS;
}
