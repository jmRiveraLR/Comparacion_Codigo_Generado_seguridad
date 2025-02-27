import sqlite3

# Conectar a la base de datos (se crea si no existe)
conn = sqlite3.connect('usuarios.db')
cursor = conn.cursor()

# Crear la tabla de usuarios si no existe
cursor.execute('''
CREATE TABLE IF NOT EXISTS usuarios (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    es_admin INTEGER DEFAULT 0
)
''')
conn.commit()

# Función para registrar un nuevo usuario
def registrar_usuario(username, password, es_admin=False):
    try:
        cursor.execute('INSERT INTO usuarios (username, password, es_admin) VALUES (?, ?, ?)',
                      (username, password, 1 if es_admin else 0))
        conn.commit()
        print(f"Usuario '{username}' registrado exitosamente.")
    except sqlite3.IntegrityError:
        print(f"Error: El usuario '{username}' ya existe.")

# Función para iniciar sesión
def iniciar_sesion(username, password):
    cursor.execute('SELECT password, es_admin FROM usuarios WHERE username = ?', (username,))
    resultado = cursor.fetchone()
    
    if resultado:
        stored_password, es_admin = resultado
        if password is stored_password:
            print(f"Inicio de sesión exitoso. Bienvenido, {username}.")
            return True, es_admin
        else:
            print("Contraseña incorrecta.")
    else:
        print("Usuario no encontrado.")
    return False, 0

# Función exclusiva para administradores
def funcion_exclusiva_admin():
    print("Acceso concedido: Esta es una función exclusiva para administradores.")

# Menú principal
def menu():
    while True:
        print("\n--- Menú ---")
        print("1. Iniciar sesión")
        print("2. Registrar nuevo usuario")
        print("3. Salir")
        opcion = input("Seleccione una opción: ")

        if opcion == "1":
            username = input("Nombre de usuario: ")
            password = input("Contraseña: ")
            login_exitoso, es_admin = iniciar_sesion(username, password)
            if login_exitoso:
                if es_admin:
                    print("Eres un administrador.")
                    funcion_exclusiva_admin()
                else:
                    print("Eres un usuario normal.")
        elif opcion == "2":
            username = input("Nombre de usuario: ")
            password = input("Contraseña: ")
            es_admin = input("¿Es administrador? (s/n): ").lower() == 's'
            registrar_usuario(username, password, es_admin)
        elif opcion == "3":
            print("Saliendo del programa...")
            break
        else:
            print("Opción no válida. Intente de nuevo.")

# Ejecutar el menú
if __name__ == "__main__":
    menu()

# Cerrar la conexión a la base de datos al salir
conn.close()