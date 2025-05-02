#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define PORT 8080
#define MAX_INPUT_LEN 100
#define SALT_LEN 32

// Estructura para conexión segura
struct connection_info {
    char username[MAX_INPUT_LEN + 1];
    char password[MAX_INPUT_LEN + 1]; 
    int login_success;
};

// Genera hash seguro de contraseña con salt
void generate_hash(const char *password, const unsigned char *salt, char *output) {
    PKCS5_PBKDF2_HMAC(password, strlen(password),
                     salt, SALT_LEN,
                     100000,  // Iteraciones
                     EVP_sha256(),
                     32,      // Longitud del hash
                     (unsigned char *)output);
}

// Verifica credenciales de forma segura
int check_credentials(const char *username, const char *password) {
    PGconn *conn = PQconnectdb("dbname=usuarios user=postgres password=tu_password host=localhost");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Error de conexión\n");
        PQfinish(conn);
        return 0;
    }

    // Consulta preparada para evitar SQLi
    const char *paramValues[1] = {username};
    PGresult *res = PQexecParams(conn,
                               "SELECT salt, password_hash FROM usuarios WHERE username = $1",
                               1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        PQfinish(conn);
        return 0;
    }

    // Verificación segura del hash
    unsigned char *salt = (unsigned char *)PQgetvalue(res, 0, 0);
    char *stored_hash = PQgetvalue(res, 0, 1);
    char computed_hash[32];
    
    generate_hash(password, salt, computed_hash);
    
    int result = (memcmp(stored_hash, computed_hash, 32) == 0);
    
    PQclear(res);
    PQfinish(conn);
    return result;
}

// Callback seguro para POST
static int post_iterator(void *cls, enum MHD_ValueKind kind, 
                         const char *key, const char *filename,
                         const char *content_type, const char *transfer_encoding,
                         const char *data, uint64_t off, size_t size) {
    struct connection_info *con_info = cls;
    
    if (size > MAX_INPUT_LEN) return MHD_NO;

    if (strcmp(key, "username") == 0) {
        strncpy(con_info->username, data, size);
        con_info->username[size] = '\0';
    } 
    else if (strcmp(key, "password") == 0) {
        strncpy(con_info->password, data, size);
        con_info->password[size] = '\0';
    }
    
    return MHD_YES;
}

// Escapa HTML para prevenir XSS
char *escape_html(const char *input) {
    char *output = malloc(strlen(input) * 6 + 1);
    char *ptr = output;
    
    for (; *input; input++) {
        switch(*input) {
            case '&': strcpy(ptr, "&amp;"); ptr += 5; break;
            case '<': strcpy(ptr, "&lt;");  ptr += 4; break;
            case '>': strcpy(ptr, "&gt;");  ptr += 4; break;
            default: *ptr++ = *input;
        }
    }
    *ptr = '\0';
    return output;
}

// Respuesta HTTP segura
static int send_response(struct MHD_Connection *connection, const char *message, int status_code) {
    char *escaped = escape_html(message);
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(escaped), escaped, MHD_RESPMEM_MUST_FREE);
    
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// Callback principal mejorado
static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    
    if (strcmp(url, "/login") != 0) {
        return send_response(connection, "Ruta no encontrada", MHD_HTTP_NOT_FOUND);
    }

    if (strcmp(method, "GET") == 0) {
        const char *form = "<form method=\"post\">[...]</form>";
        return send_response(connection, form, MHD_HTTP_OK);
    }
    else if (strcmp(method, "POST") == 0) {
        struct connection_info *con_info = *con_cls;
        
        if (!con_info) {
            con_info = calloc(1, sizeof(struct connection_info));
            *con_cls = con_info;
            return MHD_YES;
        }

        if (*upload_data_size != 0) {
            MHD_post_process(con_info, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }

        con_info->login_success = check_credentials(con_info->username, con_info->password);
        
        const char *msg = con_info->login_success ? 
            "Login exitoso" : "Credenciales inválidas";
        
        int ret = send_response(connection, msg, MHD_HTTP_OK);
        
        free(con_info);
        *con_cls = NULL;
        return ret;
    }
    
    return send_response(connection, "Método no permitido", MHD_HTTP_METHOD_NOT_ALLOWED);
}

int main() {
    struct MHD_Daemon *daemon;
    
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS,
                             PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_HTTPS_MEM_KEY, "server.key",
                             MHD_OPTION_HTTPS_MEM_CERT, "server.crt",
                             MHD_OPTION_END);
    
    if (!daemon) {
        fprintf(stderr, "Error al iniciar servidor\n");
        return 1;
    }

    printf("Servidor seguro en https://localhost:%d\n", PORT);
    getchar();
    
    MHD_stop_daemon(daemon);
    return 0;
}