import socket
import ssl
import os
import tarfile

# Configuración del socket
HOST = '0.0.0.0'  # Escuchar en todas las interfaces
PORT = 12345      # Puerto de escucha
CERT_FILE = 'cert.pem'
KEY_FILE = 'key.pem'

def validar_archivo_tar(archivo):
    """Valida si el archivo es un .tar.gz seguro."""
    if not tarfile.is_tarfile(archivo):
        raise ValueError("El archivo no es un .tar.gz válido.")

def extraer_archivo_seguro(ruta_archivo):
    """Extrae el archivo .tar.gz en una ruta segura."""
    with tarfile.open(ruta_archivo, 'r:gz') as tar:
        for member in tar.getmembers():
            # Validar Path Traversal
            if not os.path.abspath(member.name).startswith(os.getcwd()):
                raise ValueError("Se detectó intento de Path Traversal.")
        tar.extractall(path=os.getcwd())
        print("Archivo extraído de manera segura.")

def iniciar_servidor():
    """Configura y ejecuta el servidor seguro."""
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servidor:
        servidor.bind((HOST, PORT))
        servidor.listen(5)
        print(f"Servidor seguro escuchando en {HOST}:{PORT}...")

        with context.wrap_socket(servidor, server_side=True) as servidor_tls:
            while True:
                conn, addr = servidor_tls.accept()
                print(f"Conexión segura establecida con {addr}")

                try:
                    # Autenticación básica (ejemplo: esperar token secreto)
                    conn.sendall(b"Envía tu token de autenticación:")
                    token = conn.recv(4096).decode()
                    if token.strip() != "mi_token_secreto":
                        conn.sendall(b"Autenticación fallida.")
                        conn.close()
                        continue

                    conn.sendall(b"Autenticación exitosa. Envía el archivo.")
                    archivo = b''
                    while True:
                        datos = conn.recv(4096)
                        if not datos:
                            break
                        archivo += datos

                    # Procesar el archivo recibido
                    ruta_destino = "archivo_recibido.tar.gz"
                    with open(ruta_destino, 'wb') as f:
                        f.write(archivo)
                    validar_archivo_tar(ruta_destino)
                    extraer_archivo_seguro(ruta_destino)

                except Exception as e:
                    print(f"Error: {e}")
                finally:
                    conn.close()

if __name__ == "__main__":
    iniciar_servidor()