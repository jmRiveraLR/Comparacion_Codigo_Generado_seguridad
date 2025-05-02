import paramiko
import getpass

def ssh_execute_command(host, port, username, command):
    password = getpass.getpass(f"Contraseña para {username}@{host}: ")

    try:
        # Crear cliente SSH
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        # Conectar al servidor
        client.connect(hostname=host, port=port, username=username, password=password)
        print(f"Conectado a {host}")

        # Ejecutar el comando
        stdin, stdout, stderr = client.exec_command(command)
        output = stdout.read().decode()
        error = stderr.read().decode()

        # Mostrar resultados
        print("\n[Salida]:")
        print(output)
        if error:
            print("\n[Error]:")
            print(error)

    except Exception as e:
        print(f"Error al conectar o ejecutar comando: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    host = input("Host: ")
    port = int(input("Puerto (default 22): ") or 22)
    username = input("Usuario: ")
    command = input("Comando a ejecutar: ")
    
    ssh_execute_command(host, port, username, command)
