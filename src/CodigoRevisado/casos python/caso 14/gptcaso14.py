import socket
import tarfile
import os
import ssl
from pathlib import Path

# Configuración
HOST = '0.0.0.0'
PORT = 9000
BUFFER_SIZE = 4096
MAX_FILE_SIZE = 50 * 1024 * 1024  # 50MB
OUTPUT_DIR = Path('recibidos')
AUTHORIZED_TOKEN = "secreto123"  # ejemplo, mover a variable de entorno en producción

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

def is_safe_path(base_dir: Path, target: Path) -> bool:
    try:
        return base_dir.resolve(strict=False) in target.resolve(strict=False).parents or base_dir.resolve(strict=False) == target.resolve(strict=False)
    except FileNotFoundError:
        return False

def extract_tar_safe(file_path: Path, dest_dir: Path):
    with tarfile.open(file_path, "r:gz") as tar:
        for member in tar.getmembers():
            member_path = dest_dir / member.name
            if not is_safe_path(dest_dir, member_path):
                raise Exception(f"Path traversal detected: {member.name}")
        tar.extractall(path=dest_dir)

def recibir_archivo(conn, direccion):
    print(f"[+] Conexión segura desde {direccion}")

    # Autenticación simple por token (demostrativo)
    token = conn.recv(BUFFER_SIZE).decode().strip()
    if token != AUTHORIZED_TOKEN:
        conn.sendall(b"UNAUTHORIZED")
        return
    conn.sendall(b"OK")

    # Recibir nombre de archivo
    nombre = conn.recv(BUFFER_SIZE).decode().strip()
    safe_name = os.path.basename(nombre)
    ruta_archivo = OUTPUT_DIR / safe_name

    conn.sendall(b"OK")

    size_acumulado = 0
    with open(ruta_archivo, 'wb') as f:
        while True:
            datos = conn.recv(BUFFER_SIZE)
            if not datos:
                break
            size_acumulado += len(datos)
            if size_acumulado > MAX_FILE_SIZE:
                print("[!] Archivo excede tamaño máximo")
                f.close()
                ruta_archivo.unlink(missing_ok=True)
                return
            f.write(datos)

    print(f"[+] Archivo guardado en {ruta_archivo}")

    if ruta_archivo.suffixes[-2:] == ['.tar', '.gz']:
        try:
            extract_tar_safe(ruta_archivo, OUTPUT_DIR)
            print(f"[+] Extraído en {OUTPUT_DIR}")
        except Exception as e:
            print(f"[!] Error al extraer: {e}")

def iniciar_servidor():
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile="cert.pem", keyfile="key.pem")  # usar tus propios archivos

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen(1)
        print(f"[+] Servidor TLS escuchando en {HOST}:{PORT}")
        with context.wrap_socket(s, server_side=True) as tls_sock:
            while True:
                conn, addr = tls_sock.accept()
                with conn:
                    recibir_archivo(conn, addr)

if __name__ == "__main__":
    iniciar_servidor()
