#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "uthash.h"

#define MAX_LINE_LEN 8192
#define CONTEXT_WORDS 10

typedef struct Context {
    size_t line_number;
    size_t absolute_index;
    char *context_text;
    struct Context *next;
} Context;

typedef struct KeywordInfo {
    char *keyword;
    size_t frequency;
    Context *contexts;
    UT_hash_handle hh;
} KeywordInfo;

KeywordInfo *keywords_table = NULL;

void add_context(KeywordInfo *kw_info, size_t line_number, size_t abs_index, const char *context_text) {
    Context *ctx = malloc(sizeof(Context));
    ctx->line_number = line_number;
    ctx->absolute_index = abs_index;
    ctx->context_text = strdup(context_text);
    ctx->next = kw_info->contexts;
    kw_info->contexts = ctx;
}

void to_lower_str(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

void tokenize_line(const char *line, char **tokens, int *token_count) {
    char *copy = strdup(line);
    char *token = strtok(copy, " \t\n\r");
    *token_count = 0;

    while (token && *token_count < MAX_LINE_LEN) {
        tokens[(*token_count)++] = token;
        token = strtok(NULL, " \t\n\r");
    }
}

char *build_context(char **words, int total, int center, int before, int after) {
    int start = center - before < 0 ? 0 : center - before;
    int end = center + after >= total ? total - 1 : center + after;
    size_t length = 0;

    for (int i = start; i <= end; ++i) {
        length += strlen(words[i]) + 1;
    }

    char *context = malloc(length + 1);
    context[0] = '\0';

    for (int i = start; i <= end; ++i) {
        strcat(context, words[i]);
        if (i != end) strcat(context, " ");
    }

    return context;
}

void process_line(char *line, size_t line_number, size_t *abs_index) {
    char *tokens[MAX_LINE_LEN];
    int token_count = 0;
    tokenize_line(line, tokens, &token_count);

    for (int i = 0; i < token_count; ++i) {
        char word[256];
        strncpy(word, tokens[i], 255);
        word[255] = '\0';
        to_lower_str(word);

        KeywordInfo *kw_info;
        HASH_FIND_STR(keywords_table, word, kw_info);
        if (kw_info) {
            kw_info->frequency++;

            char *ctx_text = build_context(tokens, token_count, i, CONTEXT_WORDS, CONTEXT_WORDS);
            add_context(kw_info, line_number, *abs_index + (tokens[i] - line), ctx_text);
            free(ctx_text);
        }
    }

    *abs_index += strlen(line);
}

void export_json(const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Error creating JSON file");
        return;
    }

    fprintf(out, "{\n");

    KeywordInfo *kw, *tmp;
    HASH_ITER(hh, keywords_table, kw, tmp) {
        fprintf(out, "  \"%s\": {\n", kw->keyword);
        fprintf(out, "    \"frequency\": %zu,\n", kw->frequency);
        fprintf(out, "    \"contexts\": [\n");

        Context *ctx = kw->contexts;
        while (ctx) {
            fprintf(out,
                    "      {\"line\": %zu, \"index\": %zu, \"context\": \"%s\"}%s\n",
                    ctx->line_number,
                    ctx->absolute_index,
                    ctx->context_text,
                    ctx->next ? "," : "");
            ctx = ctx->next;
        }

        fprintf(out, "    ]\n");
        fprintf(out, "  }%s\n", tmp->hh.next ? "," : "");
    }

    fprintf(out, "}\n");
    fclose(out);
}

void add_keyword(const char *keyword) {
    KeywordInfo *kw = malloc(sizeof(KeywordInfo));
    kw->keyword = strdup(keyword);
    to_lower_str(kw->keyword);
    kw->frequency = 0;
    kw->contexts = NULL;
    HASH_ADD_KEYPTR(hh, keywords_table, kw->keyword, strlen(kw->keyword), kw);
}

void free_all() {
    KeywordInfo *kw, *tmp;
    HASH_ITER(hh, keywords_table, kw, tmp) {
        Context *ctx = kw->contexts;
        while (ctx) {
            Context *tmp_ctx = ctx;
            ctx = ctx->next;
            free(tmp_ctx->context_text);
            free(tmp_ctx);
        }
        HASH_DEL(keywords_table, kw);
        free(kw->keyword);
        free(kw);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s archivo.txt salida.json palabra1 palabra2 ...\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    const char *output_json = argv[2];
    for (int i = 3; i < argc; i++) {
        add_keyword(argv[i]);
    }

    char *line = NULL;
    size_t len = 0;
    size_t line_number = 1;
    size_t abs_index = 0;

    while (getline(&line, &len, file) != -1) {
        process_line(line, line_number++, &abs_index);
    }

    fclose(file);
    free(line);

    export_json(output_json);
    free_all();

    return 0;
}

// Compilar: gcc -o buscador buscador.c
