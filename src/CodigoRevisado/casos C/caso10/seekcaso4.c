#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <jansson.h>
#include <limits.h>
#include <errno.h>
#define MAX_LINE_LENGTH (10 * 1024 * 1024)
#define MAX_REASONABLE_FILE_SIZE (1 * 1024 * 1024 * 1024)
#define CONTEXT_WORDS 10
#define HASH_TABLE_SIZE 1024
#define SAFE_STR_COPY(dest, src, size) do { strncpy(dest, src, size - 1); dest[size - 1] = '\0'; } while (0)
typedef struct {
    long line_number;
    long file_offset;
    char *line;
    char *context_before;
    char *context_after;
} Occurrence;
typedef struct {
    char *keyword;
    int count;
    Occurrence *occurrences;
    int occurrences_capacity;
    int occurrences_size;
} KeywordEntry;
typedef struct {
    KeywordEntry *entries[HASH_TABLE_SIZE];
} HashTable;
unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE;
}
HashTable *create_hash_table() {
    HashTable *table = malloc(sizeof(HashTable));
    for (int i = 0; i < HASH_TABLE_SIZE; i++) table->entries[i] = NULL;
    return table;
}
KeywordEntry *create_keyword_entry(const char *keyword) {
    KeywordEntry *entry = malloc(sizeof(KeywordEntry));
    entry->keyword = strdup(keyword);
    entry->count = 0;
    entry->occurrences_capacity = 10;
    entry->occurrences_size = 0;
    entry->occurrences = malloc(sizeof(Occurrence) * entry->occurrences_capacity);
    return entry;
}
void add_occurrence(KeywordEntry *entry, long line_number, long file_offset, const char *line, const char *context_before, const char *context_after) {
    if (entry->occurrences_size >= entry->occurrences_capacity) {
        entry->occurrences_capacity *= 2;
        entry->occurrences = realloc(entry->occurrences, sizeof(Occurrence) * entry->occurrences_capacity);
    }
    Occurrence *occ = &entry->occurrences[entry->occurrences_size++];
    occ->line_number = line_number;
    occ->file_offset = file_offset;
    occ->line = strdup(line);
    occ->context_before = context_before ? strdup(context_before) : NULL;
    occ->context_after = context_after ? strdup(context_after) : NULL;
}
void add_keyword_occurrence(HashTable *table, const char *keyword, long line_number, long file_offset, const char *line, const char *context_before, const char *context_after) {
    unsigned long slot = hash_function(keyword);
    KeywordEntry *entry = table->entries[slot];
    if (entry == NULL) {
        entry = create_keyword_entry(keyword);
        table->entries[slot] = entry;
    } else {
        while (entry != NULL && strcmp(entry->keyword, keyword) != 0) {
            slot = (slot + 1) % HASH_TABLE_SIZE;
            entry = table->entries[slot];
        }
        if (entry == NULL) {
            entry = create_keyword_entry(keyword);
            table->entries[slot] = entry;
        }
    }
    entry->count++;
    add_occurrence(entry, line_number, file_offset, line, context_before, context_after);
}
int is_safe_filename(const char *filename) {
    if (!filename || strlen(filename) > PATH_MAX) return 0;
    if (strstr(filename, "../") || strchr(filename, ';') || strchr(filename, '|') || strchr(filename, '&')) return 0;
    return 1;
}
FILE *safe_fopen(const char *filename) {
    if (!is_safe_filename(filename)) { fprintf(stderr, "[!] Nombre de archivo no seguro: %s\n", filename); return NULL; }
    int fd = open(filename, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) { perror("[!] Error al abrir archivo"); return NULL; }
    struct stat st;
    if (fstat(fd, &st) == -1) { perror("[!] Error en stat"); close(fd); return NULL; }
    if (!S_ISREG(st.st_mode)) { fprintf(stderr, "[!] No es un archivo regular\n"); close(fd); return NULL; }
    if (st.st_size > MAX_REASONABLE_FILE_SIZE) { fprintf(stderr, "[!] Archivo demasiado grande (>1GB)\n"); close(fd); return NULL; }
    FILE *file = fdopen(fd, "r");
    if (!file) { perror("[!] Error en fdopen"); close(fd); }
    return file;
}
char **split_words_safe(const char *line, int *word_count) {
    if (!line || !word_count) return NULL;
    char **words = NULL;
    int capacity = 16;
    int count = 0;
    words = malloc(capacity * sizeof(char *));
    if (!words) return NULL;
    const char *p = line;
    while (*p) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;
        const char *start = p;
        while (*p && !isspace(*p)) p++;
        int len = p - start;
        if (count >= capacity) {
            capacity *= 2;
            char **new_words = realloc(words, capacity * sizeof(char *));
            if (!new_words) goto error;
            words = new_words;
        }
        words[count] = malloc(len + 1);
        if (!words[count]) goto error;
        SAFE_STR_COPY(words[count], start, len + 1);
        count++;
    }
    *word_count = count;
    return words;
error:
    for (int i = 0; i < count; i++) free(words[i]);
    free(words);
    return NULL;
}
char *get_context_safe(char **words, int start, int end, int total_words) {
    if (!words || start < 0 || end > total_words || start >= end) return NULL;
    size_t total_len = 0;
    for (int i = start; i < end; i++) total_len += strlen(words[i]) + 1;
    char *context = malloc(total_len + 1);
    if (!context) return NULL;
    context[0] = '\0';
    for (int i = start; i < end; i++) {
        if (i > start) strncat(context, " ", total_len - strlen(context));
        strncat(context, words[i], total_len - strlen(context));
    }
    return context;
}
void process_file_safe(const char *filename, HashTable *table, char **keywords, int keyword_count) {
    FILE *file = safe_fopen(filename);
    if (!file) exit(EXIT_FAILURE);
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    long line_number = 0;
    long file_offset = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        line_number++;
        if (read > MAX_LINE_LENGTH) { fprintf(stderr, "[!] Línea %ld demasiado larga\n", line_number); continue; }
        if (line[read - 1] == '\n') line[read - 1] = '\0';
        int word_count = 0;
        char **words = split_words_safe(line, &word_count);
        if (!words) continue;
        for (int i = 0; i < word_count; i++) {
            for (char *p = words[i]; *p; p++) {
                if (ispunct(*p)) { memmove(p, p + 1, strlen(p)); p--; } 
                else { *p = tolower(*p); }
            }
            for (int k = 0; k < keyword_count; k++) {
                if (strcmp(words[i], keywords[k]) == 0) {
                    char *context_before = get_context_safe(words, (i - CONTEXT_WORDS > 0) ? i - CONTEXT_WORDS : 0, i, word_count);
                    char *context_after = get_context_safe(words, i + 1, (i + 1 + CONTEXT_WORDS < word_count) ? i + 1 + CONTEXT_WORDS : word_count, word_count);
                    add_keyword_occurrence(table, keywords[k], line_number, file_offset, line, context_before, context_after);
                    free(context_before);
                    free(context_after);
                    break;
                }
            }
        }
        for (int i = 0; i < word_count; i++) free(words[i]);
        free(words);
        file_offset += read;
    }
    free(line);
    fclose(file);
}
json_t *hash_table_to_json(HashTable *table) {
    json_t *root = json_object();
    if (!root) return NULL;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        KeywordEntry *entry = table->entries[i];
        if (!entry) continue;
        json_t *keyword_obj = json_object();
        json_object_set_new(keyword_obj, "count", json_integer(entry->count));
        json_t *occurrences = json_array();
        for (int j = 0; j < entry->occurrences_size; j++) {
            json_t *occ = json_object();
            json_object_set_new(occ, "line", json_string(entry->occurrences[j].line));
            json_object_set_new(occ, "offset", json_integer(entry->occurrences[j].file_offset));
            json_array_append_new(occurrences, occ);
        }
        json_object_set_new(keyword_obj, "occurrences", occurrences);
        json_object_set_new(root, entry->keyword, keyword_obj);
    }
    return root;
}
void free_hash_table(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        KeywordEntry *entry = table->entries[i];
        if (entry != NULL) {
            free(entry->keyword);
            for (int j = 0; j < entry->occurrences_size; j++) {
                free(entry->occurrences[j].line);
                free(entry->occurrences[j].context_before);
                free(entry->occurrences[j].context_after);
            }
            free(entry->occurrences);
            free(entry);
        }
    }
    free(table);
}
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Uso: %s <archivo> <palabra1> [palabra2...]\n", argv[0]); return EXIT_FAILURE; }
    if (!is_safe_filename(argv[1])) { fprintf(stderr, "[!] Nombre de archivo no permitido\n"); return EXIT_FAILURE; }
    HashTable *table = create_hash_table();
    char **keywords = malloc((argc - 2) * sizeof(char *));
    for (int i = 2; i < argc; i++) {
        keywords[i - 2] = strdup(argv[i]);
        for (char *p = keywords[i - 2]; *p; p++) *p = tolower(*p);
    }
    process_file_safe(argv[1], table, keywords, argc - 2);
    json_t *json = hash_table_to_json(table);
    char *json_str = json_dumps(json, JSON_INDENT(2));
    printf("%s\n", json_str);
    free(json_str);
    json_decref(json);
    free_hash_table(table);
    for (int i = 0; i < argc - 2; i++) free(keywords[i]);
    free(keywords);
    return EXIT_SUCCESS;
}