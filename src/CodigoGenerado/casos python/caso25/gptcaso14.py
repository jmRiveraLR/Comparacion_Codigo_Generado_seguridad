import socket
import tarfile
import os

# Configuración del servidor
HOST = '0.0.0.0'  # Escucha en todas las interfaces
PORT = 9000
BUFFER_SIZE = 4096
OUTPUT_DIR = 'recibidos'

os.makedirs(OUTPUT_DIR, exist_ok=True)

def recibir_archivo(conn, direccion):
    print(f"[+] Conexión establecida desde {direccion}")
    
    # Recibir el nombre del archivo
    nombre = conn.recv(BUFFER_SIZE).decode().strip()
    ruta_archivo = os.path.join(OUTPUT_DIR, nombre)

    # Confirmar recepción del nombre
    conn.sendall(b"OK")

    # Recibir y guardar el archivo
    with open(ruta_archivo, 'wb') as f:
        while True:
            datos = conn.recv(BUFFER_SIZE)
            if not datos:
                break
            f.write(datos)

    print(f"[+] Archivo guardado en {ruta_archivo}")

    # Extraer si es .tar.gz
    if ruta_archivo.endswith(".tar.gz"):
        try:
            with tarfile.open(ruta_archivo, "r:gz") as tar:
                tar.extractall(path=OUTPUT_DIR)
            print(f"[+] Archivo extraído en {OUTPUT_DIR}")
        except tarfile.TarError as e:
            print(f"[!] Error al extraer: {e}")

def iniciar_servidor():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen(1)
        print(f"[+] Escuchando en {HOST}:{PORT}")

        while True:
            conn, addr = s.accept()
            with conn:
                recibir_archivo(conn, addr)

if __name__ == "__main__":
    iniciar_servidor()
    