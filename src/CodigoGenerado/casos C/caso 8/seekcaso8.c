#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <jansson.h>

#define MAX_LINE_LENGTH 4096
#define CONTEXT_WORDS 10
#define HASH_TABLE_SIZE 1024

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

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % HASH_TABLE_SIZE;
}

HashTable *create_hash_table() {
    HashTable *table = malloc(sizeof(HashTable));
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->entries[i] = NULL;
    }
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

void add_occurrence(KeywordEntry *entry, long line_number, long file_offset, const char *line, 
                    const char *context_before, const char *context_after) {
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

void add_keyword_occurrence(HashTable *table, const char *keyword, long line_number, 
                           long file_offset, const char *line, const char *context_before, 
                           const char *context_after) {
    unsigned long slot = hash_function(keyword);
    KeywordEntry *entry = table->entries[slot];

    if (entry == NULL) {
        entry = create_keyword_entry(keyword);
        table->entries[slot] = entry;
    } else {
        // Handle collisions (simple linear probing)
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

char **split_words(const char *line, int *word_count) {
    char **words = NULL;
    int capacity = 0;
    int count = 0;
    const char *p = line;

    while (*p) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        const char *start = p;
        while (*p && !isspace(*p)) p++;
        int len = p - start;

        if (count >= capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            words = realloc(words, sizeof(char*) * capacity);
        }

        words[count] = malloc(len + 1);
        strncpy(words[count], start, len);
        words[count][len] = '\0';
        count++;
    }

    *word_count = count;
    return words;
}

void free_words(char **words, int word_count) {
    for (int i = 0; i < word_count; i++) {
        free(words[i]);
    }
    free(words);
}

char *get_context_before(char **words, int current_word, int word_count) {
    int start = current_word - CONTEXT_WORDS;
    if (start < 0) start = 0;
    int len = current_word - start;

    char *context = malloc(1);
    context[0] = '\0';
    size_t total_len = 0;

    for (int i = start; i < current_word; i++) {
        total_len += strlen(words[i]) + 1;
        context = realloc(context, total_len);
        if (i > start) strcat(context, " ");
        strcat(context, words[i]);
    }

    return context;
}

char *get_context_after(char **words, int current_word, int word_count) {
    int end = current_word + 1 + CONTEXT_WORDS;
    if (end > word_count) end = word_count;
    int len = end - (current_word + 1);

    char *context = malloc(1);
    context[0] = '\0';
    size_t total_len = 0;

    for (int i = current_word + 1; i < end; i++) {
        total_len += strlen(words[i]) + 1;
        context = realloc(context, total_len);
        if (i > current_word + 1) strcat(context, " ");
        strcat(context, words[i]);
    }

    return context;
}

void process_file(const char *filename, HashTable *table, char **keywords, int keyword_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
    long line_number = 0;
    long file_offset = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len-1] == '\n') {
            line[line_len-1] = '\0'; // Remove newline
        }

        int word_count = 0;
        char **words = split_words(line, &word_count);

        for (int i = 0; i < word_count; i++) {
            // Normalize word (lowercase, remove punctuation)
            char *word = words[i];
            size_t len = strlen(word);
            for (size_t j = 0; j < len; j++) {
                if (ispunct(word[j])) {
                    memmove(&word[j], &word[j+1], len - j);
                    len--;
                    j--;
                } else {
                    word[j] = tolower(word[j]);
                }
            }

            if (len == 0) continue;

            // Check if word is one of our keywords
            for (int k = 0; k < keyword_count; k++) {
                if (strcmp(word, keywords[k]) == 0) {
                    char *context_before = get_context_before(words, i, word_count);
                    char *context_after = get_context_after(words, i, word_count);
                    
                    add_keyword_occurrence(table, keywords[k], line_number, file_offset, 
                                          line, context_before, context_after);
                    
                    free(context_before);
                    free(context_after);
                    break;
                }
            }
        }

        free_words(words, word_count);
        file_offset += line_len;
    }

    fclose(file);
}

json_t *hash_table_to_json(HashTable *table) {
    json_t *root = json_object();

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        KeywordEntry *entry = table->entries[i];
        if (entry != NULL) {
            json_t *keyword_obj = json_object();
            json_object_set_new(keyword_obj, "count", json_integer(entry->count));

            json_t *occurrences_array = json_array();
            for (int j = 0; j < entry->occurrences_size; j++) {
                Occurrence *occ = &entry->occurrences[j];
                json_t *occ_obj = json_object();
                json_object_set_new(occ_obj, "line_number", json_integer(occ->line_number));
                json_object_set_new(occ_obj, "file_offset", json_integer(occ->file_offset));
                json_object_set_new(occ_obj, "line", json_string(occ->line));
                if (occ->context_before) {
                    json_object_set_new(occ_obj, "context_before", json_string(occ->context_before));
                }
                if (occ->context_after) {
                    json_object_set_new(occ_obj, "context_after", json_string(occ->context_after));
                }
                json_array_append_new(occurrences_array, occ_obj);
            }

            json_object_set_new(keyword_obj, "occurrences", occurrences_array);
            json_object_set_new(root, entry->keyword, keyword_obj);
        }
    }

    return root;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <filename> <keyword1> [keyword2 ...]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int keyword_count = argc - 2;
    char **keywords = malloc(sizeof(char*) * keyword_count);

    // Normalize keywords (lowercase)
    for (int i = 0; i < keyword_count; i++) {
        keywords[i] = strdup(argv[i + 2]);
        for (char *p = keywords[i]; *p; p++) {
            *p = tolower(*p);
        }
    }

    HashTable *table = create_hash_table();
    process_file(filename, table, keywords, keyword_count);

    // Convert to JSON and print
    json_t *root = hash_table_to_json(table);
    char *json_str = json_dumps(root, JSON_INDENT(2));
    printf("%s\n", json_str);

    // Cleanup
    free(json_str);
    json_decref(root);
    for (int i = 0; i < keyword_count; i++) {
        free(keywords[i]);
    }
    free(keywords);
    free_hash_table(table);

    return 0;
}