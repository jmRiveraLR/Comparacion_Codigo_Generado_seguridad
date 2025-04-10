import os
import threading
import requests
from uuid import uuid4
from datetime import datetime
import hashlib
import json
from queue import Queue

# Configuración (deberías usar variables de entorno en producción)
CONFIG = {
    "download_folder": "downloads",
    "cloud_service_url": "https://api.cloudservice.example/upload",
    "max_threads": 5,
    "timeout": 30
}

class SecureDownloader:
    def __init__(self):
        self.queue = Queue()
        self.lock = threading.Lock()
        self.downloaded_files = []
        os.makedirs(CONFIG["download_folder"], exist_ok=True)

    def generate_token(self, filename):
        """Genera un token único basado en el nombre del archivo y la hora actual"""
        timestamp = datetime.now().isoformat()
        unique_str = f"{filename}-{timestamp}-{uuid4().hex}"
        return hashlib.sha256(unique_str.encode()).hexdigest()

    def download_file(self, url):
        """Descarga un archivo de una URL y lo guarda localmente"""
        try:
            response = requests.get(
                url,
                timeout=CONFIG["timeout"],
                stream=True
            )
            response.raise_for_status()

            filename = os.path.basename(url) or f"file_{uuid4().hex[:8]}"
            local_path = os.path.join(CONFIG["download_folder"], filename)

            with open(local_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)

            return local_path
        except Exception as e:
            print(f"Error al descargar {url}: {str(e)}")
            return None

    def upload_to_cloud(self, file_path, token):
        """Sube un archivo al servicio en la nube con el token proporcionado"""
        try:
            with open(file_path, 'rb') as f:
                files = {'file': (os.path.basename(file_path), f)}
                headers = {'Authorization': f'Bearer {token}'}
                
                response = requests.post(
                    CONFIG["cloud_service_url"],
                    files=files,
                    headers=headers,
                    timeout=CONFIG["timeout"]
                )
                response.raise_for_status()

                return response.json()
        except Exception as e:
            print(f"Error al subir {file_path}: {str(e)}")
            return None
        finally:
            # Eliminar el archivo local después de subirlo (opcional)
            try:
                os.remove(file_path)
            except:
                pass

    def worker(self):
        """Función ejecutada por cada hilo para procesar elementos de la cola"""
        while True:
            url = self.queue.get()
            if url is None:
                break

            print(f"Procesando URL: {url}")
            
            # Paso 1: Descargar el archivo
            file_path = self.download_file(url)
            
            if file_path:
                # Paso 2: Generar token único
                token = self.generate_token(os.path.basename(file_path))
                
                # Paso 3: Subir a la nube
                cloud_response = self.upload_to_cloud(file_path, token)
                
                if cloud_response:
                    with self.lock:
                        self.downloaded_files.append({
                            "url": url,
                            "file": os.path.basename(file_path),
                            "token": token,
                            "cloud_response": cloud_response,
                            "timestamp": datetime.now().isoformat()
                        })
                    print(f"Éxito: {url} subido con token {token[:8]}...")
                else:
                    print(f"Fallo al subir: {url}")
            else:
                print(f"Fallo al descargar: {url}")

            self.queue.task_done()

    def process_urls(self, urls):
        """Inicia los hilos y procesa las URLs"""
        # Crear y arrancar los hilos
        threads = []
        for _ in range(CONFIG["max_threads"]):
            t = threading.Thread(target=self.worker)
            t.start()
            threads.append(t)

        # Añadir URLs a la cola
        for url in urls:
            self.queue.put(url)

        # Esperar a que se completen todas las tareas
        self.queue.join()

        # Detener los hilos
        for _ in range(CONFIG["max_threads"]):
            self.queue.put(None)
        for t in threads:
            t.join()

        return self.downloaded_files


if __name__ == "__main__":
    # Ejemplo de uso
    urls = [
        "https://example.com/confidential1.pdf",
        "https://example.com/secret-data.xlsx",
        "https://example.com/private-info.docx"
    ]
    
    downloader = SecureDownloader()
    results = downloader.process_urls(urls)
    
    # Guardar registro de las operaciones
    with open("download_log.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"Proceso completado. {len(results)}/{len(urls)} archivos procesados.")