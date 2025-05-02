import socket
import os
import tarfile
import threading

def handle_client(conn, addr):
    """
    Maneja la conexión de un cliente: recibe el archivo .tar.gz y lo extrae.
    """
    print(f"Conexión establecida desde {addr}")
    
    try:
        # Recibir el nombre del archivo
        filename = conn.recv(1024).decode().strip()
        if not filename:
            raise ValueError("No se recibió nombre de archivo")
        
        # Asegurarse de que termina con .tar.gz
        if not filename.endswith('.tar.gz'):
            filename += '.tar.gz'
        
        print(f"Recibiendo archivo: {filename}")
        
        # Recibir el archivo en bloques
        with open(filename, 'wb') as f:
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                f.write(data)
        
        print(f"Archivo {filename} recibido correctamente")
        
        # Extraer el archivo .tar.gz
        with tarfile.open(filename, 'r:gz') as tar:
            tar.extractall()
            print(f"Archivo {filename} extraído correctamente")
            
        # Opcional: eliminar el archivo .tar.gz después de extraer
        # os.remove(filename)
        
    except Exception as e:
        print(f"Error con {addr}: {str(e)}")
    finally:
        conn.close()
        print(f"Conexión con {addr} cerrada")

def start_server(host='0.0.0.0', port=5000):
    """
    Inicia el servidor que escucha por archivos .tar.gz
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen(5)
        print(f"Servidor escuchando en {host}:{port}")
        
        while True:
            conn, addr = s.accept()
            # Manejar cada conexión en un hilo separado
            threading.Thread(target=handle_client, args=(conn, addr)).start()

if __name__ == '__main__':
    # Crear directorio de trabajo si no existe
    if not os.path.exists('received_files'):
        os.makedirs('received_files')
    os.chdir('received_files')
    
    start_server()