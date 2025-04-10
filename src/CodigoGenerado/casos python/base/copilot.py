import mysql.connector 

# Conexión a la base de datos
db = mysql.connector.connect(
    host="tu_host",
    user="tu_usuario",
    password="tu_contraseña",
    database="tu_base_de_datos"
)

cursor = db.cursor()

def login(username, password):
    cursor.execute("SELECT username, password, is_admin FROM users WHERE username=%s", (username,))
    user = cursor.fetchone()

    if user and user[1] == password:
        print("Login exitoso!")
        if user[2]:  # user[2] is the is_admin field
            print("Bienvenido, administrador!")
            admin_functions()
        else:
            print("Bienvenido, usuario!")
    else:
        print("Usuario o contraseña incorrectos.")

def admin_functions():
    print("Acceso a funciones exclusivas para administradores.")
    # Aquí puedes agregar las funciones exclusivas para administradores
    # por ejemplo: gestionar usuarios, visualizar informes, etc.

if __name__ == "__main__":
    username = input("Ingresa tu nombre de usuario: ")
    password = input("Ingresa tu contraseña: ")
    login(username, password)
