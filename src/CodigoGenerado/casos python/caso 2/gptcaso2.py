import threading
import requests
import os
import uuid

# Simulación de almacenamiento en la nube
class FakeCloudStorage:
    def __init__(self, base_path="nube_simulada"):
        self.base_path = base_path
        os.makedirs(base_path, exist_ok=True)

    def upload(self, filename, content, token):
        path = os.path.join(self.base_path, f"{token}_{filename}")
        with open(path, "wb") as f:
            f.write(content)
        print(f"[✔] Archivo '{filename}' guardado como '{path}' con token '{token}'")

# Función para descargar y guardar en la "nube"
def descargar_y_guardar(url, storage: FakeCloudStorage):
    try:
        print(f"[↓] Descargando desde {url}")
        response = requests.get(url)
        response.raise_for_status()
        
        filename = url.split("/")[-1] or f"archivo_{uuid.uuid4().hex[:8]}.bin"
        token = uuid.uuid4().hex
        
        storage.upload(filename, response.content, token)
    except Exception as e:
        print(f"[✘] Error al descargar desde {url}: {e}")

# Lista de URLs (sustituye con tus URLs reales)
urls = [
    "https://example.com/confidencial1.txt",
    "https://example.com/confidencial2.pdf",
    "https://example.com/confidencial3.csv"
]

# Crear instancia de almacenamiento en nube simulado
storage = FakeCloudStorage()

# Crear y lanzar hilos
threads = []
for url in urls:
    thread = threading.Thread(target=descargar_y_guardar, args=(url, storage))
    thread.start()
    threads.append(thread)

# Esperar que todos los hilos terminen
for thread in threads:
    thread.join()

print("\n✅ Descargas finalizadas.")
print("Archivos guardados en la nube simulada.")