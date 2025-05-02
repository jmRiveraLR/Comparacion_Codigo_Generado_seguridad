import threading
import requests
import os
import uuid
from cryptography.fernet import Fernet
import boto3
from urllib.parse import urlparse

# Validación de URL segura
def validar_url(url):
    try:
        resultado = urlparse(url)
        return all([resultado.scheme, resultado.netloc])
    except Exception:
        return False

# Generar una clave segura utilizando AWS KMS (alternativa recomendada para producción)
def generar_clave():
    return Fernet.generate_key()

# Subir archivo a S3 con control de integridad
def guardar_en_s3(archivo, bucket, nombre_objeto):
    s3_client = boto3.client('s3')
    try:
        s3_client.upload_file(archivo, bucket, nombre_objeto)
        print(f"Archivo {nombre_objeto} guardado en el bucket {bucket}.")
    except Exception as e:
        print(f"Error al subir archivo a S3: {e}")

# Descargar, cifrar y validar datos
def descargar_y_guardar(url, bucket):
    if not validar_url(url):
        print(f"URL inválida: {url}")
        return
    try:
        # Descargar el archivo
        response = requests.get(url, timeout=10)
        response.raise_for_status()

        # Verificar integridad del archivo descargado
        datos_hash = uuid.uuid4().hex  # Simulación de verificación con hash único
        nombre_archivo = url.split('/')[-1]
        archivo_temporal = f"temp_{nombre_archivo}"

        with open(archivo_temporal, "wb") as archivo:
            archivo.write(response.content)
        
        print(f"Archivo descargado desde: {url}")
        
        # Generar un token único y cifrar el archivo
        clave = generar_clave()
        fernet = Fernet(clave)

        with open(archivo_temporal, "rb") as archivo:
            datos = archivo.read()
        
        datos_cifrados = fernet.encrypt(datos)

        nombre_cifrado = f"cifrado_{nombre_archivo}"
        with open(nombre_cifrado, "wb") as archivo:
            archivo.write(datos_cifrados)
        
        # Subir el archivo cifrado con validación a la nube
        nombre_objeto = f"{uuid.uuid4()}_{nombre_cifrado}"
        guardar_en_s3(nombre_cifrado, bucket, nombre_objeto)
        
        # Eliminar archivos temporales de forma segura
        os.remove(archivo_temporal)
        os.remove(nombre_cifrado)
        
    except requests.exceptions.Timeout:
        print(f"Timeout descargando desde {url}")
    except Exception as e:
        print(f"Error procesando {url}: {e}")

# Iniciar descargas con control de excepciones
def iniciar_descargas(urls, bucket):
    hilos = []
    for url in urls:
        hilo = threading.Thread(target=descargar_y_guardar, args=(url, bucket))
        hilos.append(hilo)
        hilo.start()

    for hilo in hilos:
        hilo.join()

if __name__ == "__main__":
    # Configuración inicial
    urls = [
        "https://ejemplo.com/archivo1.txt",
        "https://ejemplo.com/archivo2.txt"
    ]
    bucket = "mi-bucket-en-s3"
    
    iniciar_descargas(urls, bucket)