#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <json-c/json.h>

// Estructura para guardar información de palabras clave
typedef struct KeywordInfo {
    char *keyword;
    int frequency;
    json_object *occurrences;
} KeywordInfo;

// Función para convertir a minúsculas
void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Función para calcular contexto y ocurrencias
void process_line(const char *line, int line_number, char **keywords, int keyword_count, KeywordInfo **keyword_info) {
    char *token;
    char line_copy[4096];
    strcpy(line_copy, line);
    to_lowercase(line_copy);

    char *context;
    int absolute_index = 0;

    token = strtok(line_copy, " ");
    while (token) {
        for (int i = 0; i < keyword_count; i++) {
            if (strstr(token, keywords[i]) != NULL) {
                keyword_info[i]->frequency++;

                // Contexto: 10 palabras antes y después (simplificado)
                context = (char *)malloc(256);
                snprintf(context, 256, "línea %d, índice %d, fragmento: %s", line_number, absolute_index, line);

                // JSON: Añadir ocurrencia
                json_object_array_add(keyword_info[i]->occurrences, json_object_new_string(context));
                free(context);
            }
        }
        absolute_index++;
        token = strtok(NULL, " ");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <archivo> <palabra_clave1> [palabra_clave2...]\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    char **keywords = argv + 2;
    int keyword_count = argc - 2;
    KeywordInfo **keyword_info = malloc(keyword_count * sizeof(KeywordInfo *));

    for (int i = 0; i < keyword_count; i++) {
        keyword_info[i] = malloc(sizeof(KeywordInfo));
        keyword_info[i]->keyword = keywords[i];
        keyword_info[i]->frequency = 0;
        keyword_info[i]->occurrences = json_object_new_array();
    }

    char line[4096];
    int line_number = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        process_line(line, line_number, keywords, keyword_count, keyword_info);
    }

    fclose(file);

    // Exportar resultados a JSON
    json_object *output = json_object_new_object();
    for (int i = 0; i < keyword_count; i++) {
        json_object *keyword_object = json_object_new_object();
        json_object_object_add(keyword_object, "frecuencia", json_object_new_int(keyword_info[i]->frequency));
        json_object_object_add(keyword_object, "ocurrencias", keyword_info[i]->occurrences);
        json_object_object_add(output, keyword_info[i]->keyword, keyword_object);

        free(keyword_info[i]);
    }

    free(keyword_info);

    FILE *json_file = fopen("resultados.json", "w");
    if (json_file) {
        fprintf(json_file, "%s\n", json_object_to_json_string_ext(output, JSON_C_TO_STRING_PRETTY));
        fclose(json_file);
    } else {
        perror("No se pudo escribir el archivo JSON");
    }

    json_object_put(output);

    return 0;
}