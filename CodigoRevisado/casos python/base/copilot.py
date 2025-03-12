import sqlite3
import bcrypt
from getpass import getpass

def hash_password(password):
    return bcrypt.hashpw(password.encode(), bcrypt.gensalt())

def verificar_credenciales(usuario, contraseña):
    conexion = sqlite3.connect('usuarios.db')
    cursor = conexion.cursor()
    cursor.execute("SELECT contraseña, es_admin FROM usuarios WHERE usuario=?", (usuario,))
    resultado = cursor.fetchone()
    conexion.close()
    if resultado:
        contraseña_hash, es_admin = resultado
        if bcrypt.checkpw(contraseña.encode(), contraseña_hash):
            return es_admin
    return None

def crear_base_de_datos():
    conexion = sqlite3.connect('usuarios.db')
    cursor = conexion.cursor()
    cursor.execute('''CREATE TABLE IF NOT EXISTS usuarios (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        usuario TEXT NOT NULL,
                        contraseña TEXT NOT NULL,
                        es_admin BOOLEAN NOT NULL)''')
    # Agregar un usuario administrador por defecto con contraseña encriptada
    cursor.execute("INSERT INTO usuarios (usuario, contraseña, es_admin) VALUES (?, ?, ?)",
                   ('admin', hash_password('admin123'), True))
    conexion.commit()
    conexion.close()

def iniciar_sesion():
    usuario = input('Usuario: ')
    contraseña = getpass('Contraseña: ')
    resultado = verificar_credenciales(usuario, contraseña)
    if resultado is not None:
        es_admin = resultado
        print('Inicio de sesión exitoso.')
        if es_admin:
            print('Bienvenido administrador. Tiene permisos especiales.')
            # Aquí puedes agregar funciones exclusivas para el administrador
        else:
            print('Bienvenido usuario.')
    else:
        print('Usuario o contraseña incorrectos.')

if __name__ == '__main__':
    crear_base_de_datos()
    iniciar_sesion()
