import threading
import requests
import os
import uuid
import re
import base64
from cryptography.fernet import Fernet

# Simula una lista blanca de dominios permitidos
DOMINIOS_PERMITIDOS = ["example.com"]

# Límite de tamaño del archivo (en bytes)
TAMANIO_MAX = 5 * 1024 * 1024  # 5MB

# Tipos de archivo permitidos
EXTENSIONES_PERMITIDAS = [".txt", ".csv", ".pdf"]

# Clave de cifrado generada previamente y almacenada de forma segura
CLAVE_FERNET = Fernet.generate_key()
fernet = Fernet(CLAVE_FERNET)

# Simulación de almacenamiento en la nube con control de acceso
class SecureCloudStorage:
    def __init__(self, base_path="nube_segura"):
        self.base_path = base_path
        os.makedirs(base_path, exist_ok=True)

    def upload(self, filename, content, token):
        path = os.path.join(self.base_path, f"{token}_{filename}")
        with open(path, "wb") as f:
            f.write(content)
        print(f"[✔] Archivo cifrado '{filename}' almacenado de forma segura.")

# Validación de URL
def url_valida(url):
    try:
        domain = re.search(r"https?://([^/]+)", url).group(1)
        return any(dominio in domain for dominio in DOMINIOS_PERMITIDOS)
    except:
        return False

# Autenticación simulada
def autenticar():
    # Aquí se podría implementar OAuth, API keys, etc.
    return True  # Asumimos éxito por ahora

# Descargar, validar, cifrar y almacenar
def descargar_y_guardar(url, storage: SecureCloudStorage):
    try:
        if not url_valida(url):
            print(f"[✘] URL no permitida: {url}")
            return

        if not autenticar():
            print("[✘] Fallo de autenticación.")
            return

        print(f"[↓] Descargando desde {url}")
        response = requests.get(url, timeout=10)
        response.raise_for_status()

        content_type = response.headers.get("Content-Type", "")
        content_length = int(response.headers.get("Content-Length", "0"))

        if content_length > TAMANIO_MAX:
            print("[✘] El archivo excede el tamaño permitido.")
            return

        filename = url.split("/")[-1]
        if not any(filename.endswith(ext) for ext in EXTENSIONES_PERMITIDAS):
            print("[✘] Tipo de archivo no permitido.")
            return

        # Cifrado del contenido
        contenido_cifrado = fernet.encrypt(response.content)

        token = uuid.uuid4().hex
        storage.upload(filename, contenido_cifrado, token)

    except requests.RequestException as req_err:
        print(f"[✘] Error de red: {str(req_err)}")
    except Exception as e:
        print("[✘] Error general al procesar el archivo.")

# Lista de URLs válidas simuladas
urls = [
    "https://example.com/confidencial1.txt",
    "https://example.com/confidencial2.pdf",
    "https://example.com/confidencial3.csv"
]

# Instancia de almacenamiento seguro
storage = SecureCloudStorage()

# Crear y lanzar hilos
threads = []
for url in urls:
    thread = threading.Thread(target=descargar_y_guardar, args=(url, storage))
    thread.start()
    threads.append(thread)

# Esperar a que finalicen todos los hilos
for thread in threads:
    thread.join()

print("\n✅ Descargas seguras finalizadas.")
