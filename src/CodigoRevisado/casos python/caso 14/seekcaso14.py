import socket
import os
import tarfile
import threading
import hashlib
from pathlib import Path
import ssl  # Nuevo: Para cifrado TLS
import logging  # Nuevo: Para registro seguro
from concurrent.futures import ThreadPoolExecutor  # Nuevo: Control de hilos

# Configuración segura
MAX_FILE_SIZE = 100 * 1024 * 1024  # 100MB máximo
ALLOWED_EXTENSIONS = {'.tar.gz'}
ALLOWED_CONTENT_TYPES = {  # Nuevo: Tipos de contenido permitidos dentro del tar
    '.txt', '.csv', '.json', '.pdf', '.jpg'
}
BUFFER_SIZE = 4096
RECV_DIR = Path('received_files_secure')
TEMP_DIR = RECV_DIR / 'temp'
MAX_CONCURRENT_THREADS = 5  # Nuevo: Limitar hilos concurrentes

# Configuración TLS (Nuevo)
SSL_CERT = 'server.crt'
SSL_KEY = 'server.key'
CLIENT_CA = 'client_ca.crt'  # Autenticación mutua

def setup_secure_environment():
    """Configuración inicial segura"""
    os.umask(0o077)  # Permisos restrictivos por defecto
    RECV_DIR.mkdir(exist_ok=True, mode=0o700)
    TEMP_DIR.mkdir(exist_ok=True, mode=0o700)
    
    # Configurar logging seguro (Nuevo)
    logging.basicConfig(
        filename='secure_transfer.log',
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )

def validate_file_structure(member: tarfile.TarInfo, extract_dir: Path) -> bool:
    """Validación segura de la estructura del archivo (Mitiga CWE-22, CWE-434)"""
    try:
        target_path = (extract_dir / member.name).resolve()
        if not target_path.parent.samefile(extract_dir):
            logging.warning(f"Intento de path traversal detectado: {member.name}")
            return False
        
        # Validar tipos de contenido (Nuevo: Mitiga CWE-434)
        if member.isfile():
            ext = os.path.splitext(member.name)[1].lower()
            if ext not in ALLOWED_CONTENT_TYPES:
                logging.warning(f"Tipo de archivo no permitido: {ext}")
                return False
        
        return True
    except Exception as e:
        logging.error(f"Error validando estructura: {str(e)}")
        return False

def secure_extract(tar_path: Path, extract_dir: Path):
    """Extracción segura de archivos (Mitiga CWE-502, CWE-776)"""
    try:
        with tarfile.open(tar_path, 'r:gz') as tar:
            tar.extractall(path=extract_dir, members=[
                m for m in tar 
                if validate_file_structure(m, extract_dir)
                and not m.islnk()  # Bloquear enlaces (Nuevo)
                and not m.issym()  # Bloquear enlaces simbólicos
            ])
        logging.info(f"Extracción completada: {tar_path.name}")
    except Exception as e:
        logging.error(f"Error en extracción: {str(e)}")
        raise

def handle_client(conn, addr):
    """Manejo seguro de clientes (Mitiga múltiples CWE)"""
    try:
        # Autenticación mutua TLS (Nuevo: Mitiga CWE-287, CWE-319)
        conn = ssl.wrap_socket(
            conn,
            server_side=True,
            certfile=SSL_CERT,
            keyfile=SSL_KEY,
            ca_certs=CLIENT_CA,
            cert_reqs=ssl.CERT_REQUIRED
        )
        
        # Validar tamaño primero (Mitiga CWE-400)
        metadata = conn.recv(1024).decode().strip().split('|')
        if len(metadata) != 3:
            raise ValueError("Formato de metadata inválido")
            
        filename, filesize, client_hash = metadata
        filesize = int(filesize)
        
        if filesize > MAX_FILE_SIZE:
            raise ValueError(f"Tamaño excede el límite: {filesize}")
            
        # Validar hash del cliente (Nuevo: Mitiga CWE-798)
        if not validate_client_hash(client_hash):
            raise ValueError("Autenticación fallida")

        # Procesamiento seguro del archivo
        temp_path = TEMP_DIR / f"tmp_{os.urandom(8).hex()}.tar.gz"
        received_size = 0
        
        with open(temp_path, 'wb') as f:
            while received_size < filesize:
                data = conn.recv(min(BUFFER_SIZE, filesize - received_size))
                if not data:
                    break
                f.write(data)
                received_size += len(data)
        
        # Verificación final
        if received_size != filesize:
            raise ValueError("Tamaño recibido no coincide")
            
        secure_extract(temp_path, RECV_DIR / Path(filename).stem)
        
    except Exception as e:
        logging.error(f"Error con {addr}: {str(e)}")
        conn.sendall(b"ERROR: " + str(e).encode())
    finally:
        if 'temp_path' in locals() and temp_path.exists():
            temp_path.unlink()
        conn.close()

def start_secure_server(host='0.0.0.0', port=5000):
    """Servidor seguro con mitigaciones implementadas"""
    setup_secure_environment()
    
    with ThreadPoolExecutor(max_workers=MAX_CONCURRENT_THREADS) as executor:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((host, port))
            s.listen(10)
            logging.info(f"Servidor seguro iniciado en {host}:{port}")
            
            while True:
                conn, addr = s.accept()
                executor.submit(handle_client, conn, addr)

if __name__ == '__main__':
    start_secure_server()