import paramiko

def ssh_execute_command(host, port, username, password, command):
    try:
        # Crear cliente SSH
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        # Conexión al servidor remoto
        ssh.connect(hostname=host, port=port, username=username, password=password)
        print(f"Conectado a {host}")
        
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
    
    except Exception as e:
        print(f"Error al ejecutar el comando: {e}")

if __name__ == "__main__":
    # Solicitar información al usuario
    host = input("Ingrese la dirección IP o el nombre del host: ")
    port = int(input("Ingrese el puerto (por defecto 22): "))
    username = input("Ingrese el nombre de usuario: ")
    password = input("Ingrese la contraseña: ")
    command = input("Ingrese el comando que desea ejecutar: ")
    
    # Ejecutar el comando en el servidor remoto
    ssh_execute_command(host, port, username, password, command)