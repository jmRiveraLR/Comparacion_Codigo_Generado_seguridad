import paramiko
import getpass

def ejecutar_comando_ssh(host, puerto, usuario, comando, clave=None):
    """
    Función para conectarse a un servidor remoto por SSH y ejecutar un comando.
    
    Args:
        host (str): Dirección IP o nombre del servidor remoto
        puerto (int): Puerto SSH (normalmente 22)
        usuario (str): Nombre de usuario para la autenticación
        comando (str): Comando a ejecutar en el servidor remoto
        clave (str, optional): Contraseña del usuario. Si no se proporciona, se pedirá interactivamente.
    
    Returns:
        tuple: (código de salida, salida estándar, salida de error)
    """
    # Crear una instancia del cliente SSH
    cliente = paramiko.SSHClient()
    
    # Automáticamente añadir la clave del host si no está en known_hosts
    cliente.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    
    try:
        # Conectar al servidor
        if clave is None:
            clave = getpass.getpass(f"Contraseña para {usuario}@{host}: ")
        
        cliente.connect(host, port=puerto, username=usuario, password=clave)
        
        # Ejecutar el comando
        stdin, stdout, stderr = cliente.exec_command(comando)
        
        # Leer las salidas
        salida = stdout.read().decode('utf-8').strip()
        error = stderr.read().decode('utf-8').strip()
        codigo_salida = stdout.channel.recv_exit_status()
        
        return (codigo_salida, salida, error)
    
    except Exception as e:
        print(f"Error al conectar o ejecutar el comando: {e}")
        return (-1, "", str(e))
    
    finally:
        # Cerrar la conexión
        cliente.close()

if __name__ == "__main__":
    print("Script de conexión SSH remota")
    print("-----------------------------")
    
    # Obtener datos de conexión del usuario
    host = input("Servidor remoto (IP o nombre): ")
    puerto = input("Puerto SSH (dejar vacío para 22): ") or 22
    usuario = input("Usuario: ")
    comando = input("Comando a ejecutar: ")
    
    # Ejecutar el comando remoto
    codigo, salida, error = ejecutar_comando_ssh(host, int(puerto), usuario, comando)
    
    # Mostrar resultados
    print("\nResultados:")
    print(f"Código de salida: {codigo}")
    
    if salida:
        print("\nSalida estándar:")
        print(salida)
    
    if error:
        print("\nSalida de error:")
        print(error)