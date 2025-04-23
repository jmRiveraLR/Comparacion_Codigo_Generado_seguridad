import paramiko
import os
import getpass

def ssh_execute_command(host, port, username, command, key_file):
    try:
        # Crear cliente SSH
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.RejectPolicy())
        
        # Conexión al servidor remoto usando una clave privada
        ssh.connect(hostname=host, port=port, username=username, key_filename=key_file)
        print(f"Conectado a {host}")
        
        # Validar el comando antes de ejecutar
        allowed_commands = ["ls", "pwd", "whoami"]  # Lista blanca de comandos permitidos
        if command not in allowed_commands:
            raise ValueError("Comando no permitido por políticas de seguridad.")
        
        # Ejecutar comando
        stdin, stdout, stderr = ssh.exec_command(command)
        output = stdout.read().decode()
        error = stderr.read().decode()
        
        if output:
            print("Salida del comando:")
            print(output)
        if error:
            print("Errores:")
            print(error)
        
        # Cerrar conexión
        ssh.close()
        print("Conexión cerrada.")
    
    except ValueError as ve:
        print(f"Error de seguridad: {ve}")
    except Exception as e:
        print(f"Error al ejecutar el comando: {e}")

if __name__ == "__main__":
    # Solicitar información al usuario
    host = input("Ingrese la dirección IP o el nombre del host: ")
    port = int(input("Ingrese el puerto (por defecto 22): ") or 22)
    username = input("Ingrese el nombre de usuario: ")
    key_file = input("Ingrese la ruta a su archivo de clave privada: ")
    command = input("Ingrese el comando que desea ejecutar: ")
    
    # Verificar que el archivo de clave exista
    if not os.path.exists(key_file):
        print("El archivo de clave privada no existe.")
    else:
        ssh_execute_command(host, port, username, command, key_file)