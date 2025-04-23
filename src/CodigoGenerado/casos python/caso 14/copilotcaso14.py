import socket
import os
import tarfile

# Configuración del socket
HOST = '0.0.0.0'  # Escuchar en todas las interfaces
PORT = 12345      # Puerto de escucha

def guardar_y_extraer_archivo(archivo, ruta_destino):
    try:
        # Guardar el archivo en el disco
        with open(ruta_destino, 'wb') as f:
            f.write(archivo)
        print(f"Archivo guardado en: {ruta_destino}")
        
        # Extraer el archivo .tar.gz
        if tarfile.is_tarfile(ruta_destino):
            with tarfile.open(ruta_destino, 'r:gz') as tar:
                tar.extractall(path=os.path.dirname(ruta_destino))
                print("Archivo extraído exitosamente")
        else:
            print("El archivo no es un .tar.gz válido.")
    except Exception as e:
        print(f"Error: {e}")

def iniciar_servidor():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servidor:
        servidor.bind((HOST, PORT))
        servidor.listen(5)
        print(f"Servidor escuchando en {HOST}:{PORT}...")

        while True:
            conn, addr = servidor.accept()
            print(f"Conexión establecida con {addr}")
            try:
                # Recibir datos
                archivo = b''
                while True:
                    datos = conn.recv(4096)
                    if not datos:
                        break
                    archivo += datos

                # Guardar y procesar el archivo recibido
                ruta_destino = "archivo_recibido.tar.gz"
                guardar_y_extraer_archivo(archivo, ruta_destino)

            except Exception as e:
                print(f"Error en la conexión: {e}")
            finally:
                conn.close()

if __name__ == "__main__":
    iniciar_servidor()