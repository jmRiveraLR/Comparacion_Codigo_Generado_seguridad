import mysql.connector
from getpass import getpass  # Para ocultar la entrada de la contraseña
import bcrypt  # Para hashear contraseñas
import os
from dotenv import load_dotenv  # Para cargar variables de entorno

# Cargar variables de entorno desde un archivo .env
load_dotenv()

def conectar_bd():
    try:
        conexion = mysql.connector.connect(
            host=os.getenv("DB_HOST", "localhost"),
            user=os.getenv("DB_USER", "root"),  # Usuario desde variables de entorno
            password=os.getenv("DB_PASSWORD", ""),  # Contraseña desde variables de entorno
            database=os.getenv("DB_NAME", "sistema_login")
        )
        return conexion
    except mysql.connector.Error as err:
        print(f"Error de conexión: {err}")
        return None

def hash_password(password):
    # Genera un hash de la contraseña usando bcrypt
    return bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt())

def verificar_password(password, hashed_password):
    # Verifica si la contraseña coincide con el hash almacenado
    return bcrypt.checkpw(password.encode('utf-8'), hashed_password)

def verificar_usuario(username, password):
    conexion = conectar_bd()
    if conexion:
        cursor = conexion.cursor(dictionary=True)
        query = "SELECT * FROM usuarios WHERE username = %s"
        cursor.execute(query, (username,))
        usuario = cursor.fetchone()
        cursor.close()
        conexion.close()

        if usuario and verificar_password(password, usuario['password'].encode('utf-8')):
            return usuario
    return None

def menu_admin():
    print("Bienvenido, Administrador")
    print("1. Ver todos los usuarios")
    print("2. Agregar nuevo usuario")
    print("3. Salir")
    opcion = input("Seleccione una opción: ")
    if opcion == "1":
        ver_usuarios()
    elif opcion == "2":
        agregar_usuario()
    elif opcion == "3":
        print("Saliendo...")
    else:
        print("Opción no válida")

def ver_usuarios():
    conexion = conectar_bd()
    if conexion:
        cursor = conexion.cursor(dictionary=True)
        cursor.execute("SELECT id, username, es_admin FROM usuarios")  # No seleccionar contraseñas
        usuarios = cursor.fetchall()
        cursor.close()
        conexion.close()
        for usuario in usuarios:
            print(f"ID: {usuario['id']}, Username: {usuario['username']}, Admin: {usuario['es_admin']}")

def agregar_usuario():
    username = input("Ingrese el nombre de usuario: ")
    password = getpass("Ingrese la contraseña: ")  # Oculta la entrada de la contraseña
    es_admin = input("¿Es administrador? (s/n): ").lower() == 's'
    hashed_password = hash_password(password)

    conexion = conectar_bd()
    if conexion:
        cursor = conexion.cursor()
        query = "INSERT INTO usuarios (username, password, es_admin) VALUES (%s, %s, %s)"
        cursor.execute(query, (username, hashed_password.decode('utf-8'), es_admin))
        conexion.commit()
        cursor.close()
        conexion.close()
        print("Usuario agregado con éxito")

def main():
    username = input("Usuario: ")
    password = getpass("Contraseña: ")  # Oculta la entrada de la contraseña

    usuario = verificar_usuario(username, password)

    if usuario:
        print(f"Bienvenido, {usuario['username']}")
        if usuario['es_admin']:
            menu_admin()
        else:
            print("No tienes permisos de administrador.")
    else:
        print("Usuario o contraseña incorrectos")

if __name__ == "__main__":
    main()