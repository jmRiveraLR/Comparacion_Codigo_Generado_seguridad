#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <argon2.h>
#include <time.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 128
#define MAX_FAILED_ATTEMPTS 5
#define LOCKOUT_TIME 300 // 5 minutes in seconds
#define HASH_LENGTH 64
#define SALT_LENGTH 32
#define BUFFER_SIZE 2048

typedef struct {
    char username[MAX_USERNAME_LEN + 1];
    char password[MAX_PASSWORD_LEN + 1];
    int login_success;
    char ip[INET6_ADDRSTRLEN];
} connection_info;

typedef struct {
    char ip[INET6_ADDRSTRLEN];
    int attempts;
    time_t lock_time;
} login_attempt;

static login_attempt *attempts_table = NULL;
static size_t attempts_size = 0;
static pthread_mutex_t attempts_mutex = PTHREAD_MUTEX_INITIALIZER;

// HTML escape function
char *htmlspecialchars(const char *input) {
    if (input == NULL) return NULL;
    
    size_t len = strlen(input);
    char *output = malloc(len * 6 + 1); // Worst case scenario
    if (!output) return NULL;
    
    char *ptr = output;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '&': strcpy(ptr, "&amp;"); ptr += 5; break;
            case '<': strcpy(ptr, "&lt;"); ptr += 4; break;
            case '>': strcpy(ptr, "&gt;"); ptr += 4; break;
            case '"': strcpy(ptr, "&quot;"); ptr += 6; break;
            case '\'': strcpy(ptr, "&#39;"); ptr += 5; break;
            default: *ptr++ = input[i];
        }
    }
    *ptr = '\0';
    return output;
}

int check_attempts(const char *ip) {
    pthread_mutex_lock(&attempts_mutex);
    
    time_t now = time(NULL);
    for (size_t i = 0; i < attempts_size; i++) {
        if (strcmp(attempts_table[i].ip, ip) == 0) {
            if (attempts_table[i].attempts >= MAX_FAILED_ATTEMPTS && 
                difftime(now, attempts_table[i].lock_time) < LOCKOUT_TIME) {
                pthread_mutex_unlock(&attempts_mutex);
                return -1; // Account locked
            }
            int result = attempts_table[i].attempts;
            pthread_mutex_unlock(&attempts_mutex);
            return result;
        }
    }
    
    pthread_mutex_unlock(&attempts_mutex);
    return 0;
}

void record_attempt(const char *ip, int success) {
    pthread_mutex_lock(&attempts_mutex);
    
    time_t now = time(NULL);
    int found = 0;
    
    // Check existing entries
    for (size_t i = 0; i < attempts_size; i++) {
        if (strcmp(attempts_table[i].ip, ip) == 0) {
            found = 1;
            if (success) {
                attempts_table[i].attempts = 0;
            } else {
                attempts_table[i].attempts++;
                if (attempts_table[i].attempts >= MAX_FAILED_ATTEMPTS) {
                    attempts_table[i].lock_time = now;
                }
            }
            break;
        }
    }
    
    // Add new entry if not found
    if (!found && !success) {
        login_attempt *new_table = realloc(attempts_table, (attempts_size + 1) * sizeof(login_attempt));
        if (new_table) {
            attempts_table = new_table;
            strncpy(attempts_table[attempts_size].ip, ip, INET6_ADDRSTRLEN - 1);
            attempts_table[attempts_size].ip[INET6_ADDRSTRLEN - 1] = '\0';
            attempts_table[attempts_size].attempts = 1;
            attempts_table[attempts_size].lock_time = 0;
            attempts_size++;
        }
    }
    
    pthread_mutex_unlock(&attempts_mutex);
}

int verify_credentials(const char *username, const char *password, const char *ip) {
    // Input validation
    if (!username || !password || strlen(username) > MAX_USERNAME_LEN || strlen(password) > MAX_PASSWORD_LEN) {
        return 0;
    }

    // Check login attempts
    int attempts = check_attempts(ip);
    if (attempts == -1) {
        syslog(LOG_WARNING, "Account locked for IP %s", ip);
        return -1;
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int valid = 0;

    if (sqlite3_open("usuarios.db", &db) != SQLITE_OK) {
        syslog(LOG_ERR, "Database error: %s", sqlite3_errmsg(db));
        return 0;
    }

    const char *sql = "SELECT password_hash, salt FROM usuarios WHERE username = ? LIMIT 1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        syslog(LOG_ERR, "SQL error: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *stored_hash = (const char *)sqlite3_column_text(stmt, 0);
        const char *stored_salt = (const char *)sqlite3_column_text(stmt, 1);
        
        if (stored_hash && stored_salt) {
            if (argon2i_verify(stored_hash, password, strlen(password)) == ARGON2_OK) {
                valid = 1;
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (!valid) {
        record_attempt(ip, 0);
        syslog(LOG_WARNING, "Failed login attempt for user %s from IP %s", username, ip);
    } else {
        record_attempt(ip, 1);
    }

    return valid;
}

static int send_sanitized_response(struct MHD_Connection *connection, 
                                 const char *message, 
                                 int status_code) {
    char *sanitized = htmlspecialchars(message);
    if (!sanitized) {
        return MHD_NO;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(sanitized), (void *)sanitized, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(sanitized);
        return MHD_NO;
    }

    // Security headers
    MHD_add_response_header(response, "Content-Security-Policy", 
                          "default-src 'self'; script-src 'none'");
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");
    MHD_add_response_header(response, "X-Frame-Options", "DENY");
    MHD_add_response_header(response, "X-XSS-Protection", "1; mode=block");
    
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int handle_login_request(struct MHD_Connection *connection, 
                              connection_info *con_info) {
    const char *page;
    char buffer[BUFFER_SIZE];
    int status_code;

    if (con_info->login_success == -1) {
        page = "<html><body><h1>Cuenta bloqueada temporalmente</h1>"
               "<p>Demasiados intentos fallidos. Intente nuevamente en 5 minutos.</p></body></html>";
        status_code = MHD_HTTP_TOO_MANY_REQUESTS;
    } else if (con_info->login_success) {
        page = "<html><body><h1>Login exitoso!</h1></body></html>";
        status_code = MHD_HTTP_OK;
    } else {
        page = "<html><body><h1>Login invalido</h1></body></html>";
        status_code = MHD_HTTP_UNAUTHORIZED;
    }

    snprintf(buffer, sizeof(buffer), "%s", page);
    return send_sanitized_response(connection, buffer, status_code);
}

static int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, 
                       const char *key, const char *filename,
                       const char *content_type, const char *transfer_encoding,
                       const char *data, uint64_t off, size_t size) {
    connection_info *con_info = coninfo_cls;
    
    if (size == 0) return MHD_YES;
    
    if (strcmp(key, "username") == 0) {
        strncpy(con_info->username, data, MIN(size, MAX_USERNAME_LEN));
        con_info->username[MAX_USERNAME_LEN] = '\0';
    } else if (strcmp(key, "password") == 0) {
        strncpy(con_info->password, data, MIN(size, MAX_PASSWORD_LEN));
        con_info->password[MAX_PASSWORD_LEN] = '\0';
    }
    
    return MHD_YES;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                              const char *url, const char *method,
                              const char *version, const char *upload_data,
                              size_t *upload_data_size, void **con_cls) {
    if (strcmp(url, "/login") != 0) {
        return send_sanitized_response(connection, 
                                     "<html><body><h1>Página no encontrada</h1></body></html>", 
                                     MHD_HTTP_NOT_FOUND);
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
        return send_sanitized_response(connection, 
                                     "<html><body><h1>Método no permitido</h1></body></html>", 
                                     MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    // Get client IP
    const union MHD_ConnectionInfo *info = MHD_get_connection_info(connection, 
                                                                 MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    const char *ip = "unknown";
    if (info && info->client_addr) {
        char ip_buffer[INET6_ADDRSTRLEN];
        if (info->client_addr->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)info->client_addr;
            ip = inet_ntop(AF_INET, &sin->sin_addr, ip_buffer, INET6_ADDRSTRLEN);
        } else if (info->client_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)info->client_addr;
            ip = inet_ntop(AF_INET6, &sin6->sin6_addr, ip_buffer, INET6_ADDRSTRLEN);
        }
    }

    if (*con_cls == NULL) {
        // First call for this connection
        connection_info *con_info = calloc(1, sizeof(connection_info));
        if (!con_info) return MHD_NO;
        
        if (ip) {
            strncpy(con_info->ip, ip, INET6_ADDRSTRLEN - 1);
            con_info->ip[INET6_ADDRSTRLEN - 1] = '\0';
        }
        
        *con_cls = con_info;
        
        if (strcmp(method, "POST") == 0) {
            con_info->login_success = 0;
            return MHD_YES;
        } else {
            // Show login form for GET
            const char *page = "<html><head><title>Login</title></head><body>"
                              "<form action=\"/login\" method=\"post\">"
                              "Usuario: <input type=\"text\" name=\"username\" maxlength=\"32\"><br>"
                              "Contraseña: <input type=\"password\" name=\"password\" maxlength=\"128\"><br>"
                              "<input type=\"submit\" value=\"Login\">"
                              "</form></body></html>";
            return send_sanitized_response(connection, page, MHD_HTTP_OK);
        }
    }
    
    connection_info *con_info = *con_cls;
    
    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            MHD_post_process(con_info, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            // Verify credentials
            con_info->login_success = verify_credentials(con_info->username, con_info->password, con_info->ip);
            return handle_login_request(connection, con_info);
        }
    } else {
        // Show login form for GET
        const char *page = "<html><head><title>Login</title></head><body>"
                          "<form action=\"/login\" method=\"post\">"
                          "Usuario: <input type=\"text\" name=\"username\" maxlength=\"32\"><br>"
                          "Contraseña: <input type=\"password\" name=\"password\" maxlength=\"128\"><br>"
                          "<input type=\"submit\" value=\"Login\">"
                          "</form></body></html>";
        return send_sanitized_response(connection, page, MHD_HTTP_OK);
    }
}

void request_completed(void *cls, struct MHD_Connection *connection,
                     void **con_cls, enum MHD_RequestTerminationCode toe) {
    connection_info *con_info = *con_cls;
    if (con_info) {
        free(con_info);
        *con_cls = NULL;
    }
}

int initialize_database() {
    sqlite3 *db;
    char *err_msg = NULL;
    int rc;
    
    rc = sqlite3_open("usuarios.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    
    const char *sql = "CREATE TABLE IF NOT EXISTS usuarios ("
                     "username TEXT PRIMARY KEY,"
                     "password_hash TEXT NOT NULL,"
                     "salt TEXT NOT NULL);"
                     "INSERT OR IGNORE INTO usuarios VALUES ("
                     "'admin', "
                     "'$argon2i$v=19$m=65536,t=3,p=1$c2FsdHlzYWx0$J4moa2MM4/Nq7h9JVJVIkE0t5XJWUVZ0QTk33Yr5X5A', "
                     "'c2FsdHlzYWx0');"  // password: admin123
                     "INSERT OR IGNORE INTO usuarios VALUES ("
                     "'user', "
                     "'$argon2i$v=19$m=65536,t=3,p=1$c2FsdHlzYWx0$5jCjQkI5x5eO4J8Z8X9n6v3Xz7x5XJWUVZ0QTk33Yr5X5A', "
                     "'c2FsdHlzYWx0');"; // password: password
    
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_close(db);
    return 1;
}

int main() {
    if (!initialize_database()) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    struct MHD_Daemon *daemon;
    
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                            &answer_to_connection, NULL,
                            MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
                            MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "Could not start web server\n");
        return 1;
    }
    
    printf("Server running on http://localhost:%d/login\n", PORT);
    printf("Test users:\n");
    printf("admin / admin123\n");
    printf("user / password\n");
    
    getchar();
    
    MHD_stop_daemon(daemon);
    free(attempts_table);
    return 0;
}