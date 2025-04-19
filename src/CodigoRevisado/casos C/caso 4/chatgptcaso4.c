#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uthash.h"

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

void escape_json_string(const char *src, char *dest, size_t maxlen) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di + 6 < maxlen; si++) {
        if (src[si] == '\"' || src[si] == '\\') {
            dest[di++] = '\\';
        }
        dest[di++] = src[si];
    }
    dest[di] = '\0';
}

void add_context(KeywordInfo *kw_info, size_t line_number, size_t abs_index, const char *context_text) {
    Context *ctx = malloc(sizeof(Context));
    if (!ctx) exit(EXIT_FAILURE);
    ctx->line_number = line_number;
    ctx->absolute_index = abs_index;
    ctx->context_text = strdup(context_text);
    if (!ctx->context_text) exit(EXIT_FAILURE);
    ctx->next = kw_info->contexts;
    kw_info->contexts = ctx;
}

void to_lower_str(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

char *build_context(char **words, int total, int center, int before, int after) {
    int start = center - before < 0 ? 0 : center - before;
    int end = center + after >= total ? total - 1 : center + after;

    size_t buf_size = 1024;
    char *context = calloc(buf_size, sizeof(char));
    if (!context) exit(EXIT_FAILURE);

    size_t len = 0;
    for (int i = start; i <= end; ++i) {
        int written = snprintf(context + len, buf_size - len, "%s%s", words[i], (i < end ? " " : ""));
        if (written < 0 || (size_t)written >= buf_size - len) break;
        len += written;
    }

    return context;
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) return 0;
    return S_ISREG(path_stat.st_mode);
}

void export_json(const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Error al crear JSON");
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
            char escaped[1024];
            escape_json_string(ctx->context_text, escaped, sizeof(escaped));
            fprintf(out, "      {\"line\": %zu, \"index\": %zu, \"context\": \"%s\"}%s\n",
                    ctx->line_number, ctx->absolute_index, escaped,
                    ctx->next ? "," : "");
            ctx = ctx->next;
        }

        fprintf(out, "    ]\n");
        fprintf(out, "  }%s\n", tmp->hh.next ? "," : "");
    }

    fprintf(out, "}\n");
    fclose(out);
}

void process_line(char *line, size_t line_number, size_t *abs_index) {
    char *tokens[2048];
    int token_count = 0;
    char *copy = strdup(line);
    if (!copy) exit(EXIT_FAILURE);
    char *saveptr = NULL;
    char *token = strtok_r(copy, " \t\n\r", &saveptr);

    while (token && token_count < 2048) {
        tokens[token_count++] = token;
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }

    for (int i = 0; i < token_count; ++i) {
        char word[256];
        snprintf(word, sizeof(word), "%s", tokens[i]);
        to_lower_str(word);

        KeywordInfo *kw_info;
        HASH_FIND_STR(keywords_table, word, kw_info);
        if (kw_info) {
            kw_info->frequency++;
            char *ctx_text = build_context(tokens, token_count, i, CONTEXT_WORDS, CONTEXT_WORDS);
            add_context(kw_info, line_number, *abs_index + (tokens[i] - copy), ctx_text);
            free(ctx_text);
        }
    }

    *abs_index += strlen(line);
    free(copy);
}

void add_keyword(const char *keyword) {
    KeywordInfo *kw = malloc(sizeof(KeywordInfo));
    if (!kw) exit(EXIT_FAILURE);
    kw->keyword = strdup(keyword);
    if (!kw->keyword) exit(EXIT_FAILURE);
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
        return EXIT_FAILURE;
    }

    if (!is_regular_file(argv[1])) {
        fprintf(stderr, "Archivo de entrada inválido o no es regular.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}
