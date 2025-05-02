import threading
import requests
import os
import uuid
from cryptography.fernet import Fernet
import boto3

# Generar una clave y crear un cifrador
def generar_clave():
    return Fernet.generate_key()

def guardar_en_s3(archivo, bucket, nombre_objeto):
    s3_client = boto3.client('s3')
    try:
        s3_client.upload_file(archivo, bucket, nombre_objeto)
        print(f"Archivo {nombre_objeto} guardado en el bucket {bucket}.")
    except Exception as e:
        print(f"Error al subir archivo a S3: {e}")

def descargar_y_guardar(url, bucket):
    try:
        # Descargar el archivo
        response = requests.get(url)
        response.raise_for_status()
        
        # Crear un archivo temporal
        nombre_archivo = url.split('/')[-1]  # Obtener el nombre del archivo
        archivo_temporal = f"temp_{nombre_archivo}"
        
        with open(archivo_temporal, "wb") as archivo:
            archivo.write(response.content)
        
        # Generar un token y encriptar el archivo
        clave = generar_clave()
        fernet = Fernet(clave)
        
        with open(archivo_temporal, "rb") as archivo:
            datos = archivo.read()
        
        datos_cifrados = fernet.encrypt(datos)
        
        # Guardar el archivo cifrado localmente
        nombre_cifrado = f"cifrado_{nombre_archivo}"
        with open(nombre_cifrado, "wb") as archivo:
            archivo.write(datos_cifrados)
        
        # Subir el archivo cifrado a la nube
        nombre_objeto = f"{uuid.uuid4()}_{nombre_cifrado}"
        guardar_en_s3(nombre_cifrado, bucket, nombre_objeto)
        
        # Eliminar archivos temporales
        os.remove(archivo_temporal)
        os.remove(nombre_cifrado)
        
    except Exception as e:
        print(f"Error procesando {url}: {e}")

def iniciar_descargas(urls, bucket):
    hilos = []
    
    for url in urls:
        hilo = threading.Thread(target=descargar_y_guardar, args=(url, bucket))
        hilos.append(hilo)
        hilo.start()
    
    for hilo in hilos:
        hilo.join()

if __name__ == "__main__":
    # Ejemplo de uso
    urls = [
        "https://ejemplo.com/archivo1.txt",
        "https://ejemplo.com/archivo2.txt"
    ]
    bucket = "mi-bucket-en-s3"  # Cambiar por tu bucket de S3
    
    iniciar_descargas(urls, bucket)