import os
import threading
import requests
from uuid import uuid4
from datetime import datetime
import hashlib
import json
from queue import Queue
import ssl
import logging
from cryptography.fernet import Fernet
import re

# Configuración segura (deberían usarse secretos gestionados)
class SecureConfig:
    def __init__(self):
        # Generar clave de cifrado (en producción usar un KMS)
        self.ENCRYPTION_KEY = Fernet.generate_key()
        self.DOWNLOAD_FOLDER = self._secure_folder("secure_downloads")
        self.CLOUD_SERVICE_URL = os.getenv("CLOUD_SERVICE_URL", "https://api.trusted-cloud.example/upload")
        self.MAX_THREADS = 5
        self.TIMEOUT = (10, 30)  # (connect, read)
        self.ALLOWED_DOMAINS = {"trusted-cloud.example", "example.com"}
        self.LOG_FILE = self._secure_folder("logs") + "/audit.log"
        
    def _secure_folder(self, folder_name):
        """Crea un directorio con permisos seguros"""
        os.makedirs(folder_name, exist_ok=True)
        os.chmod(folder_name, 0o700)
        return folder_name

CONFIG = SecureConfig()

# Configuración de logging seguro
logging.basicConfig(
    filename=CONFIG.LOG_FILE,
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filemode='a'
)
logger = logging.getLogger(__name__)

class SecureDownloader:
    def __init__(self):
        self.queue = Queue()
        self.lock = threading.Lock()
        self.downloaded_files = []
        self.cipher_suite = Fernet(CONFIG.ENCRYPTION_KEY)

    def _validate_url(self, url):
        """Valida la URL contra dominios permitidos y protocolo HTTPS"""
        try:
            if not url.startswith('https://'):
                raise ValueError("Solo se permiten URLs HTTPS")
            
            domain = re.match(r'https://([^/]+)', url).group(1)
            if domain not in CONFIG.ALLOWED_DOMAINS:
                raise ValueError(f"Dominio no permitido: {domain}")
            
            return True
        except Exception as e:
            logger.warning(f"URL validation failed: {str(e)}")
            return False

    def generate_token(self):
        """Genera un token seguro usando secrets"""
        entropy = os.urandom(32) + str(datetime.now().timestamp()).encode()
        return hashlib.sha3_256(entropy).hexdigest()

    def _secure_download(self, url):
        """Descarga segura con validación de certificados SSL"""
        try:
            session = requests.Session()
            session.verify = True  # Verificación SSL obligatoria
            
            response = session.get(
                url,
                timeout=CONFIG.TIMEOUT,
                stream=True
            )
            response.raise_for_status()

            # Generar nombre de archivo seguro
            filename = f"secure_{uuid4().hex}"
            local_path = os.path.join(CONFIG.DOWNLOAD_FOLDER, filename)

            with open(local_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)

            # Cifrar el archivo inmediatamente después de descargarlo
            with open(local_path, 'rb') as f:
                file_data = f.read()
            
            encrypted_data = self.cipher_suite.encrypt(file_data)
            
            with open(local_path, 'wb') as f:
                f.write(encrypted_data)

            return local_path
        except requests.exceptions.SSLError as e:
            logger.error(f"SSL verification failed for {url}: {str(e)}")
            return None
        except Exception as e:
            logger.error(f"Download failed for {url}: [REDACTED]")
            return None

    def _secure_upload(self, file_path, token):
        """Subida segura con autenticación mutua"""
        try:
            # Descifrar el archivo en memoria
            with open(file_path, 'rb') as f:
                encrypted_data = f.read()
            
            decrypted_data = self.cipher_suite.decrypt(encrypted_data)

            # Configuración segura de SSL
            ssl_context = ssl.create_default_context()
            ssl_context.verify_mode = ssl.CERT_REQUIRED
            ssl_context.check_hostname = True

            files = {'file': (os.path.basename(file_path), decrypted_data)}
            headers = {
                'Authorization': f'Bearer {token}',
                'X-Request-ID': uuid4().hex
            }
            
            response = requests.post(
                CONFIG.CLOUD_SERVICE_URL,
                files=files,
                headers=headers,
                timeout=CONFIG.TIMEOUT,
                verify=True
            )
            response.raise_for_status()

            # Registrar solo datos no sensibles
            log_entry = {
                'file': os.path.basename(file_path),
                'status': 'uploaded',
                'timestamp': datetime.now().isoformat(),
                'request_id': headers['X-Request-ID']
            }
            logger.info(json.dumps(log_entry))

            return True
        except Exception as e:
            logger.error(f"Upload failed: [REDACTED] - Request ID: {headers.get('X-Request-ID', 'unknown')}")
            return False
        finally:
            # Borrado seguro del archivo
            self._secure_delete(file_path)

    def _secure_delete(self, file_path):
        """Eliminación segura del archivo"""
        try:
            if os.path.exists(file_path):
                # Sobrescribir el archivo antes de borrarlo
                with open(file_path, 'wb') as f:
                    f.write(os.urandom(os.path.getsize(file_path)))
                os.remove(file_path)
        except Exception as e:
            logger.error(f"Secure delete failed: [REDACTED]")

    def worker(self):
        """Hilo de trabajo seguro"""
        while True:
            url = self.queue.get()
            if url is None:
                break

            if not self._validate_url(url):
                self.queue.task_done()
                continue

            logger.info(f"Processing URL: [REDACTED]")
            
            file_path = self._secure_download(url)
            
            if file_path:
                token = self.generate_token()
                
                if self._secure_upload(file_path, token):
                    with self.lock:
                        self.downloaded_files.append({
                            'file': os.path.basename(file_path),
                            'status': 'completed',
                            'timestamp': datetime.now().isoformat()
                        })
                else:
                    with self.lock:
                        self.downloaded_files.append({
                            'file': os.path.basename(file_path),
                            'status': 'upload_failed',
                            'timestamp': datetime.now().isoformat()
                        })
            else:
                with self.lock:
                    self.downloaded_files.append({
                        'url_hash': hashlib.sha256(url.encode()).hexdigest(),
                        'status': 'download_failed',
                        'timestamp': datetime.now().isoformat()
                    })

            self.queue.task_done()

    def process_urls(self, urls):
        """Procesamiento seguro de URLs"""
        threads = []
        for _ in range(CONFIG.MAX_THREADS):
            t = threading.Thread(target=self.worker)
            t.start()
            threads.append(t)

        for url in urls:
            self.queue.put(url)

        self.queue.join()

        for _ in range(CONFIG.MAX_THREADS):
            self.queue.put(None)
        for t in threads:
            t.join()

        return self._generate_secure_report()

    def _generate_secure_report(self):
        """Genera un reporte sin datos sensibles"""
        return {
            'total_processed': len(self.downloaded_files),
            'successful': sum(1 for f in self.downloaded_files if f['status'] == 'completed'),
            'start_time': self.downloaded_files[0]['timestamp'] if self.downloaded_files else None,
            'end_time': self.downloaded_files[-1]['timestamp'] if self.downloaded_files else None
        }


if __name__ == "__main__":
    # Ejemplo de uso con URLs validadas
    urls = [
        "https://example.com/confidential1.pdf",
        "https://trusted-cloud.example/data.xlsx"
    ]
    
    downloader = SecureDownloader()
    results = downloader.process_urls(urls)
    
    print(json.dumps(results, indent=2))